#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Thread.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/IO/MemoryBuffer.h>

#include <rapidjson/document.h>

#include "Network.h"

#include <iostream>


NetworkTransport::NetworkTransport(NetworkConnection* connection) :
    connection_(connection)
{ }

NetworkTransport::~NetworkTransport()
{ }

void NetworkTransport::ClearIncomingPackets()
{
    MutexLock lock(packetQueueLock_);

    receivedPackets_ = false;
    incomingPackets_.Clear();
}


// Websocket Transport

NetworkWebTransport::NetworkWebTransport(NetworkConnection* connection) : NetworkTransport(connection)
{
    type_ = NT_WEBSOCKET;
    identity_ = connection->GetIdentity();
    id_ = connection->GetId();
}

NetworkWebTransport::~NetworkWebTransport()
{

}

void NetworkWebTransport::Connect(const String& adress, const String& type)
{
    if (state_ == NetworkConnectionState::Connecting)
        Disconnect(0);

    // WebSocket Initialization
    if (!websocket_)
    {
        // set the configuration
        rtc::WebSocket::Configuration config = {};
        config.disableTlsVerification = true;

        websocket_ = std::make_shared<rtc::WebSocket>(config);

        // set the callbacks
        websocket_->onOpen    (std::bind(&NetworkWebTransport::OnOpen, this));
        websocket_->onClosed  (std::bind(&NetworkWebTransport::OnClosed, this));
        websocket_->onError   (std::bind(&NetworkWebTransport::OnError, this, std::placeholders::_1));
        websocket_->onMessage (std::bind(&NetworkWebTransport::OnMessageBytes, this, std::placeholders::_1),
                               std::bind(&NetworkWebTransport::OnMessageString, this, std::placeholders::_1));

        state_ = NetworkConnectionState::Disconnected;
    }

    // Launch WebSocket
    if (!websocket_->isOpen())
    {
        if (state_ == NetworkConnectionState::Disconnected)
        {
            URHO3D_LOGINFO("NetworkWebTransport::Connect() ... connecting to adress=" + adress);

            state_ = NetworkConnectionState::Connecting;
            String url = adress + identity_;
            websocket_->open(url.CString());
        }
    }
}

void NetworkWebTransport::Disconnect(int waitMSec)
{
    state_ = NetworkConnectionState::Disconnecting;

    if (websocket_ && websocket_->isOpen())
        websocket_->close();
    else
        state_ == NetworkConnectionState::Disconnected;

    newAvailablePeers_ = false;
    newConnectedPeers_ = false;
    websocket_ = nullptr;
}

void NetworkWebTransport::Send(const String& order, const String& peer)
{
    if (!websocket_ || !websocket_->isOpen())
        return;

    preparedMessage_.Clear();
    preparedMessage_.WriteString(peer);      // Remote Id (used by the signaling server)
    preparedMessage_.WriteString(identity_); // Local Id
    preparedMessage_.WriteString(order);
    websocket_->send(reinterpret_cast<const std::byte*>(preparedMessage_.GetData()), preparedMessage_.GetSize());
}

void NetworkWebTransport::SendBuffer(const VectorBuffer& buffer, const String& peer)
{
    if (!websocket_ || !websocket_->isOpen())
        return;

    preparedMessage_.Clear();
    preparedMessage_.WriteString(peer);      // Remote Id (used by the signaling server)
    preparedMessage_.WriteString(identity_); // Local Id
    AddToPreparedMessage(buffer);
    websocket_->send(reinterpret_cast<const std::byte*>(preparedMessage_.GetData()), preparedMessage_.GetSize());
}

void NetworkWebTransport::PrepareMessage(const String& order, const String& peer)
{
    preparedMessage_.Clear();
    preparedMessage_.WriteString(peer);      // Remote Id (used by the signaling server)
    preparedMessage_.WriteString(identity_); // Local Id
    preparedMessage_.WriteString(order);
}

void NetworkWebTransport::AddToPreparedMessage(const String& data)
{
    preparedMessage_.WriteString(data);
}

void NetworkWebTransport::AddToPreparedMessage(const rtc::string& data)
{
    preparedMessage_.Write(data.data(), data.length()+1);
}

void NetworkWebTransport::AddToPreparedMessage(const VectorBuffer& buffer)
{
    preparedMessage_.WriteString("data");
    preparedMessage_.WriteBuffer(buffer.GetBuffer());
}

void NetworkWebTransport::SendPreparedMessage()
{
    if (!websocket_ || !websocket_->isOpen())
        return;

    websocket_->send(reinterpret_cast<const std::byte*>(preparedMessage_.GetData()), preparedMessage_.GetSize());
}

// WebSockets Callbacks

void NetworkWebTransport::OnOpen()
{
    URHO3D_LOGINFO("NetworkWebTransport::OnOpen() ... websocket open ... !");
    state_ = NetworkConnectionState::Connected;
    connection_->OnConnected(this);
}

void NetworkWebTransport::OnClosed()
{
    URHO3D_LOGINFO("NetworkWebTransport::OnClosed() ... websocket closed !");
    state_ = NetworkConnectionState::Disconnected;
}

void NetworkWebTransport::OnError(rtc::string error)
{
    URHO3D_LOGINFOF("NetworkWebTransport::OnError() ... %s !", error.c_str());
}

void NetworkWebTransport::OnMessageBytes(rtc::binary data)
{
    MemoryBuffer msg(data.data(), data.size());
    auto peer1 = msg.ReadString();
    auto peer2 = msg.ReadString();
    auto type  = msg.ReadString();

    auto remote = connection_->GetIdentity() == peer1 ? peer2 : peer1;
    NetworkPeerTransport* peerTransport = static_cast<NetworkPeerTransport*>(connection_->GetTransport(remote));

    URHO3D_LOGINFOF("NetworkWebTransport::OnMessageBytes() ... type=%s local=%s remote=%s state=%d", type.CString(), connection_->GetIdentity().CString(), remote.CString(), peerTransport ? peerTransport->GetState() : -1);

    if (!peerTransport && type == "offer")
    {
        // TODO : issue with this => comment
        /*
        if (!connection_->AcceptNewConnectPeers())
        {
            URHO3D_LOGINFOF("NetworkWebTransport::OnMessageBytes() : AutoconnectPeers don't need more %u peers : decline the offer !", connection_->GetTransports().Size()-1);
            SendMessage("decline", remote);
            return;
        }
*/
        URHO3D_LOGINFOF("NetworkWebTransport::OnMessageBytes() : answering to offer from %s", remote.CString());
        peerTransport = static_cast<NetworkPeerTransport*>(connection_->Connect(String::EMPTY, remote, "answer"));
    }

    if (peerTransport && peerTransport->GetState() != NetworkConnectionState::Connected)
    {
        // Receive an decline from a RemotePeer
        if (type == "decline")
        {
            connection_->Disconnect(peerTransport, 0);
        }
        // Receive an offer/answer from a Remote Peer
        else if (type == "offer" || type == "answer")
        {
            peerTransport->GetRTCConnection()->setRemoteDescription(rtc::Description(msg.ReadString().CString(), type.CString()));
        }
        // Receive a candidate (route) for this peerconnection
        else if (type == "candidate")
        {
            auto candidate = msg.ReadString();
            auto mid = msg.ReadString();
            peerTransport->GetRTCConnection()->addRemoteCandidate(rtc::Candidate(candidate.CString(), mid.CString()));
        }
    }

    if (type == "data")
    {
        MutexLock lock(packetQueueLock_);
        receivedPackets_ = true;
        incomingPackets_.Resize(incomingPackets_.Size()+1);
        VectorBuffer& buffer = incomingPackets_.Back();
        unsigned size = msg.ReadVLE();
        buffer.SetData(msg, size);
    }
}

void NetworkWebTransport::OnMessageString(rtc::string data)
{
    if (data.empty())
        return;

    URHO3D_LOGINFO("NetworkWebTransport::OnMessageString() ...");

    rapidjson::Document doc;
    doc.Parse(data.c_str());
    if (doc.HasMember("join") && doc["join"].IsString())
    {
        MutexLock lock(availablePeersLock_);
        availablePeers_ = String(doc["join"].GetString()).Split(',');
        newAvailablePeers_ = true;

        URHO3D_LOGINFOF("NetworkWebTransport::OnMessageString() : clients=%u", availablePeers_.Size());

        AutoConnectPeers();
    }
}

void NetworkWebTransport::AutoConnectPeers()
{
    // TODO : issue with this => comment
/*
    if (!connection_->AcceptNewConnectPeers())
    {
        URHO3D_LOGINFOF("NetworkWebTransport::AutoConnectPeers() : AutoconnectPeers don't need more %u peers !", connection_->GetTransports().Size()-1);
        return;
    }
*/
    int numConnectedPeers = connection_->GetTransports().Size() - 1;

    for (StringVector::Iterator it = availablePeers_.Begin(); it != availablePeers_.End(); ++it)
    {
        const String& peer = *it;
        if (peer > identity_ && !connection_->GetTransport(peer))
        {
            URHO3D_LOGINFOF(" ... autoconnect peer=%s ...", peer.CString());

            connection_->Connect(String::EMPTY, peer, "offer");
            numConnectedPeers++;

            if (numConnectedPeers == connection_->GetAutoConnectedPeers())
            {
                URHO3D_LOGINFOF(" ... num requested peers %u reached !", numConnectedPeers);
                break;
            }
        }
    }
}

void NetworkWebTransport::RefreshConnectedPeers()
{
    MutexLock lock(connectedPeersLock_);
    connectedPeers_.Clear();

    const Vector<NetworkTransport*>& transports = connection_->GetTransports();
    for (unsigned i = 0; i < transports.Size(); i++)
    {
        NetworkTransport* transport = transports[i];
        if (transport->GetType() == NT_PEER && transport->GetState() == NetworkConnectionState::Connected)
            connectedPeers_.Push(transport->GetIdentity());
    }
    newConnectedPeers_ = true;
}

// WebRTC Transport

NetworkPeerTransport::NetworkPeerTransport(NetworkConnection* connection) : NetworkTransport(connection)
{
    type_ = NT_PEER;
}

NetworkPeerTransport::~NetworkPeerTransport()
{

}

void NetworkPeerTransport::Connect(const String& identity, const String& type)
{
    if (peerconnection_)
    {
        // PeerConnection already established on the same remote
        if (identity == identity_ && peerconnection_->state() == rtc::PeerConnection::State::Connected)
        {
            URHO3D_LOGINFOF("NetworkPeerTransport() - Connect : already connected to %s ...", identity.CString());
            return;
        }

        URHO3D_LOGINFOF("NetworkPeerTransport() - Connect : reset connection to previous peer=%s...", identity_.CString());
        peerconnection_.reset();
    }

    // PeerConnection Initialization
    if (!peerconnection_)
    {
        URHO3D_LOGINFOF("NetworkPeerTransport() - Connect : Connecting to %s type=%s ...", identity.CString(), type.CString());

        state_ = NetworkConnectionState::Connecting;
        identity_ = identity;
        id_ = StringHash(identity_);

        CreatePeerConnection(type == "offer");
    }
}

void NetworkPeerTransport::Disconnect(int waitMSec)
{
    state_ = NetworkConnectionState::Disconnecting;

    if (peerconnection_)
        peerconnection_->close();

    for (HashMap<String, SharedPtr<DataChannelListener> >::Iterator it = channelListeners_.Begin(); it != channelListeners_.End(); ++it)
        if (it->second_ && it->second_->channel_)
            it->second_->channel_->close();
}

void NetworkPeerTransport::CreatePeerConnection(bool addchanels)
{
    // set the configuration
    rtc::Configuration config = {};
    peerconnection_ = std::make_shared<rtc::PeerConnection>(config);

    if (peerconnection_)
    {
        peerconnection_->onLocalDescription     (std::bind(&NetworkPeerTransport::OnLocalDescription, this, std::placeholders::_1));
        peerconnection_->onLocalCandidate       (std::bind(&NetworkPeerTransport::OnLocalCandidate, this, std::placeholders::_1));

        peerconnection_->onDataChannel          (std::bind(&NetworkPeerTransport::OnDataChannel, this, std::placeholders::_1));
        peerconnection_->onTrack                (std::bind(&NetworkPeerTransport::OnTrack, this, std::placeholders::_1));

        peerconnection_->onStateChange          (std::bind(&NetworkPeerTransport::OnStateChange, this, std::placeholders::_1));
        peerconnection_->onGatheringStateChange (std::bind(&NetworkPeerTransport::OnGatheringStateChange, this, std::placeholders::_1));
        peerconnection_->onSignalingStateChange (std::bind(&NetworkPeerTransport::OnSignalingStateChange, this, std::placeholders::_1));

        URHO3D_LOGINFOF("NetworkPeerTransport() - CreatePeerConnection !");

        if (addchanels)
        {
            const StringVector& channelnames = static_cast<NetworkPeer*>(Network::Get())->GetRegisteredChannels();
            for (StringVector::ConstIterator it = channelnames.Begin(); it != channelnames.End(); ++it)
                SetChannel(*it);
        }
    }
}

void NetworkPeerTransport::SetChannel(const String& channelname, std::optional<std::shared_ptr<rtc::DataChannel> > dc)
{
    SharedPtr<DataChannelListener>& listener = channelListeners_[channelname];
    if (!listener)
        listener = new DataChannelListener();

    if (dc)
    {
        URHO3D_LOGINFOF("NetworkPeerTransport() - SetChannel : channel=%s register received datachannel", channelname.CString());
        listener->Set(this, *dc);
    }
    else if (peerconnection_)
    {
        URHO3D_LOGINFOF("NetworkPeerTransport() - SetChannel : channel=%s register new datachannel", channelname.CString());
        listener->Set(this, peerconnection_->createDataChannel(channelname.CString()));
    }

    listener->channel_->onOpen    (std::bind(&DataChannelListener::OnChannelOpen, listener.Get()));
    listener->channel_->onClosed  (std::bind(&DataChannelListener::OnChannelClosed, listener.Get()));
    listener->channel_->onMessage (std::bind(&DataChannelListener::OnChannelMessageBytes, listener.Get(), std::placeholders::_1),
                                   std::bind(&DataChannelListener::OnChannelMessageString, listener.Get(), std::placeholders::_1));
}

void NetworkPeerTransport::Send(const String& data, const String& channelname)
{
    SharedPtr<DataChannelListener>& listener = channelListeners_[channelname];
    if (listener && listener->channel_ && listener->channel_->isOpen())
        listener->channel_->send(data.CString());
}

void NetworkPeerTransport::SendBuffer(const VectorBuffer& buffer, const String& channelname)
{
    SharedPtr<DataChannelListener>& listener = channelListeners_[channelname];
    if (listener && listener->channel_ && listener->channel_->isOpen())
        listener->channel_->send(reinterpret_cast<const std::byte*>(buffer.GetData()), buffer.GetSize());
}


// PeerConnection Callbacks

// When the local description is set, send the SDP to the remote peer (via the websocket to the signaling server)
void NetworkPeerTransport::OnLocalDescription(rtc::Description desc)
{
    URHO3D_LOGINFOF("onLocalDescription...");
    NetworkWebTransport* wsTransport = static_cast<NetworkWebTransport*>(connection_->GetTransport());
    if (wsTransport)
    {
        wsTransport->PrepareMessage(desc.typeString().c_str(), identity_);
        wsTransport->AddToPreparedMessage(std::string(desc));
        wsTransport->SendPreparedMessage();
        URHO3D_LOGINFOF("onLocalDescription: type=%s local=%s remote=%s", desc.typeString().c_str(),
                        connection_->GetIdentity().CString(), identity_.CString());
    }
}

// Send the candidate to the remote peer (via the websocket to the signaling server)
void NetworkPeerTransport::OnLocalCandidate(rtc::Candidate candidate)
{
    NetworkWebTransport* wsTransport = static_cast<NetworkWebTransport*>(connection_->GetTransport());
    if (wsTransport)
    {
        wsTransport->PrepareMessage("candidate", identity_);
        wsTransport->AddToPreparedMessage(std::string(candidate));
        wsTransport->AddToPreparedMessage(candidate.mid());
        wsTransport->SendPreparedMessage();

        URHO3D_LOGINFOF("onLocalCandidate: type=candidate local=%s remote=%s, mid=%s, sdp=%s",
                        connection_->GetIdentity().CString(), identity_.CString(), candidate.mid().c_str(), std::string(candidate).c_str());
    }
}

// When Receiving a New DataChannel from Remote peer
void NetworkPeerTransport::OnDataChannel(std::shared_ptr<rtc::DataChannel> dc)
{
    SetChannel(dc->label().c_str(), std::optional<std::shared_ptr<rtc::DataChannel> >(dc));

    std::cout << "OnDataChannel: " << dc->label().c_str() << std::endl;
}

// When Receiving a New Track from Remote peer
void NetworkPeerTransport::OnTrack(std::shared_ptr<rtc::Track> track)
{


}

void NetworkPeerTransport::OnStateChange(rtc::PeerConnection::State state)
{
    std::cout << "NetworkPeerTransport State: " << state << std::endl;

    if (peerconnection_->state() == rtc::PeerConnection::State::Connected)
    {
        state_ = NetworkConnectionState::Connected;
        connection_->OnConnected(this);
    }
    else if (peerconnection_->state() > rtc::PeerConnection::State::Connected)
    {
        connection_->Disconnect(this, 0);
        state_ = NetworkConnectionState::Disconnected;
    }

    if (connection_)
    {
        NetworkWebTransport* wsTransport = static_cast<NetworkWebTransport*>(connection_->GetTransport());
        if (wsTransport)
            wsTransport->RefreshConnectedPeers();
    }
}

void NetworkPeerTransport::OnGatheringStateChange(rtc::PeerConnection::GatheringState state)
{
    std::cout << "NetworkPeerTransport Gathering State: " << state << std::endl;
}

void NetworkPeerTransport::OnSignalingStateChange(rtc::PeerConnection::SignalingState state)
{
    std::cout << "NetworkPeerTransport SignalingState State: " << state << std::endl;
}


// DataChannelListener

void DataChannelListener::Set(NetworkPeerTransport* transport, std::shared_ptr<rtc::DataChannel> channel)
{
    transport_ = transport;
    channel_ = channel;
}

void DataChannelListener::OnChannelOpen()
{
    std::cout << "DataChannelListener OnChannelOpen: " << std::endl;

    if (channel_ && channel_->isOpen())
        channel_->send(String("Hello from " + transport_->connection_->GetIdentity()).CString());
}

void DataChannelListener::OnChannelClosed()
{
    std::cout << "DataChannelListener OnChannelClosed: " << std::endl;
}

void DataChannelListener::OnChannelMessageBytes(rtc::binary data)
{
    std::cout << "DataChannelListener OnChannelMessageBytes: " << std::endl;

    MutexLock lock(transport_->packetQueueLock_);
    transport_->receivedPackets_ = true;
    transport_->incomingPackets_.Resize(transport_->incomingPackets_.Size()+1);
    VectorBuffer& buffer = transport_->incomingPackets_.Back();
    buffer.SetData(data.data(), data.size());
}

void DataChannelListener::OnChannelMessageString(rtc::string data)
{
    std::cout << "DataChannelListener OnChannelMessageString: " << data.c_str();

    MutexLock lock(transport_->packetQueueLock_);
    transport_->receivedPackets_ = true;
    transport_->incomingPackets_.Resize(transport_->incomingPackets_.Size()+1);
    VectorBuffer& buffer = transport_->incomingPackets_.Back();
    buffer.SetData(data.c_str(), data.length());

    std::cout << " ... OK !" << std::endl;
}


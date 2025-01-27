#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Core/WorkQueue.h>

#include <Urho3D/IO/Log.h>

#include "NetworkConnection.h"
#include "NetworkTransport.h"
#include "Network.h"

#include <iostream>

Context* Network::context_ = nullptr;
int Network::netmode_ = -1;
SharedPtr<Network> Network::instance_;

void Network::RegisterLibrary(Context* context)
{
    Network::context_ = context;

    NetworkPeer::RegisterObject(context);
//    NetworkClient::RegisterObject(context);
//    NetworkServer::RegisterObject(context);

    NetworkConnection::RegisterObject(context);
}

Network::Network(Context* context) :
    Object(context),
    state_(NetworkConnectionState::Disconnected)
{

}

Network::~Network()
{
    DisconnectAll(0);

    Stop();
}

void Network::Init(int netmode)
{
    if (!context_)
        return;

    // Reset instance if netmode has changed and instance already exists
    if (netmode != -1 && netmode != netmode_)
    {
        netmode_ = netmode;
        instance_.Reset();
    }

    // Create new instance based on netmode
    if (!instance_ && netmode_ != -1)
    {
        if (netmode_ == NetPeering)
            instance_ = SharedPtr<Network>(new NetworkPeer(context_));
//        else if (netmode_ == NetClient)
//            instance_ = SharedPtr<Network>(new NetworkClient(context_));
//        else if (netmode_ == NetServer)
//            instance_ = SharedPtr<Network>(new NetServer(context_));
    }
}

void Network::Stop()
{
    connections_.Clear();

    WorkQueue* queue = GetSubsystem<WorkQueue>();
    if (queue)
        queue->Complete(0);
}

void Network::Connect(const String& adress, const String& identity)
{
    if (!adress.Empty())
    {
        // Web Socket Connection
        HashMap<String, SharedPtr<NetworkConnection> >::Iterator it = connections_.Find(adress);
        SharedPtr<NetworkConnection>& connection = it != connections_.End() ? it->second_ : connections_[adress];

        if (!connection)
        {
            connection = new NetworkConnection(GetContext());
            activeWsConnection_ = connection;
        }

        // check if already connected
        if (connection && connection->IsConnected())
            return;
    }

    // Web Socket connecting
    // Peer Connection on Existing Web Socket connecting
    if (activeWsConnection_)
    {
        SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Network, HandleBeginFrame));
        SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Network, HandleRenderUpdate));

        activeWsConnection_->Connect(adress, identity);
        state_ = NetworkConnectionState::Connecting;
        URHO3D_LOGINFOF("Network::Connect() - connecting to %s %s ...", adress.CString(), identity.CString());
    }
}

void Network::DisconnectAll(int waitMSec)
{
    if ((HasSubscribedToEvent(E_BEGINFRAME) && state_ > NetworkConnectionState::Disconnected) || !Thread::IsMainThread())
    {
        URHO3D_LOGINFOF("Network() - DisconnectAll : disconnecting ...");
        state_ = NetworkConnectionState::Disconnecting;
        return;
    }

    for (HashMap<String, SharedPtr<NetworkConnection> >::Iterator it = connections_.Begin(); it != connections_.End(); ++it)
        it->second_->Disconnect(waitMSec);

    state_ = NetworkConnectionState::Disconnected;
}

void Network::Disconnect(const String& adress, int waitMSec)
{
    HashMap<String, SharedPtr<NetworkConnection> >::Iterator it = connections_.Find(adress);
    if (it != connections_.End())
        it->second_->Disconnect(waitMSec);
}

void Network::Send(const String& data, const String& channel, const String& peer)
{
    if (GetConnection())
        GetConnection()->Send(data, channel, peer);
}

void Network::Send(NetworkConnection* connection, const String& data, const String& channel, const String& peer)
{
    if (connection)
        connection->Send(data, channel, peer);
}

void Network::SendBuffer(const VectorBuffer& buffer, const String& channel, const String& peer)
{
    if (GetConnection())
        GetConnection()->SendBuffer(buffer, channel, peer);
}

void Network::SendBuffer(NetworkConnection* connection, const VectorBuffer& buffer, const String& channel, const String& peer)
{
    if (connection)
        connection->SendBuffer(buffer, channel, peer);
}

void Network::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    if (state_ == NetworkConnectionState::Disconnecting)
    {
        DisconnectAll();
        URHO3D_LOGINFOF("Network() - HandleBeginFrame : disconnecting ... finishing !");
        UnsubscribeFromAllEvents();
        return;
    }

    if (state_ != NetworkConnectionState::Connected)
        return;

    for (HashMap<String, SharedPtr<NetworkConnection> >::Iterator it = connections_.Begin(); it != connections_.End(); ++it)
    {
        NetworkConnection* connection = it->second_.Get();

        if (connection->GetAdress().Empty())
            continue;

        const Vector<NetworkTransport*>& transports = connection->GetTransports();
        for (unsigned i = 0; i < transports.Size(); i++)
        {
            NetworkTransport* transport = transports[i];

            if (transport->GetType() == NT_WEBSOCKET)
            {
                NetworkWebTransport* wstransport = static_cast<NetworkWebTransport*>(transport);
                if (wstransport)
                {
                    if (wstransport->HasNewAvailablePeers())
                    {
                        StringVector* availablepeers;
                        MutexLock lock = wstransport->AcquireAvailablePeers(availablepeers);

                        if (onAvailablePeersUpdateCallBack_)
                        {
                            onAvailablePeersUpdateCallBack_(availablepeers);
                        }
                        else
                        {
                            VariantMap& newEventData = context_->GetEventDataMap();
                            newEventData[Network_WebSocket_AvailablePeersUpdated::P_CONNECTION] = connection;
                            newEventData[Network_WebSocket_AvailablePeersUpdated::P_AVAILABLEPEERS] = *availablepeers;
                            SendEvent(N_WEBSOCKET_AVAILABLEPEERSUPDATED, newEventData);
                        }
                    }
                    if (wstransport->HasNewConnectedPeers())
                    {
                        StringVector* connectedpeers;
                        MutexLock lock = wstransport->AcquireConnectedPeers(connectedpeers);

                        if (onConnectedPeersUpdateCallBack_)
                        {
                            onConnectedPeersUpdateCallBack_(connectedpeers);
                        }
                        else
                        {
                            VariantMap& newEventData = context_->GetEventDataMap();
                            newEventData[Network_WebSocket_ConnectedPeersUpdated::P_CONNECTION] = connection;
                            newEventData[Network_WebSocket_ConnectedPeersUpdated::P_CONNECTEDPEERS] = *connectedpeers;
                            SendEvent(N_WEBSOCKET_CONNECTEDPEERSUPDATED, newEventData);
                        }
                    }
                }
            }

            if (transport->HasIncomingPackets())
            {
                Vector<VectorBuffer >* packets = nullptr;
                MutexLock lock = transport->AcquireIncomingPackets(packets);
                if (packets)
                {
                    if (onMessageReceivedCallBack_)
                    {
                        onMessageReceivedCallBack_(transport, packets);
                    }
                    else
                    {
                        VariantMap& newEventData = context_->GetEventDataMap();
                        newEventData[Network_IncomingPacketsReceived::P_TRANSPORT] = transport;
                        SendEvent(N_INCOMINGPACKETSRECEIVED, newEventData);
                    }
                }
            }
        }
    }
}

void Network::HandleRenderUpdate(StringHash eventType, VariantMap& eventData)
{

}

void Network::OnConnected(NetworkConnection* connection)
{
    if (!activeWsConnection_ && connections_.Contains(connection->GetAdress()))
    {
        activeWsConnection_ = connection;
        URHO3D_LOGINFOF("Network::OnConnected() - this=%u active connection %u", this, connection);
    }

    if (activeWsConnection_)
        state_ = NetworkConnectionState::Connected;
}

void Network::OnDisconnected(NetworkConnection* connection)
{
    if (connections_.Size())
    {
        HashMap<String, SharedPtr<NetworkConnection> >::Iterator it = connections_.Find(connection->GetAdress());
        if (it != connections_.End())
        {
            MutexLock lock(connectionsMutex_);
            if (activeWsConnection_ == connection)
            {
                URHO3D_LOGINFOF("Network::OnDisconnected() - this=%u active connection lost %u", this, connection);
                activeWsConnection_.Reset();
            }
            connections_.Erase(it);

            std::cout << "disconnecting ws connection=" << connection;
        }
    }

    if (!connections_.Size())
        state_ = NetworkConnectionState::Disconnected;
}


/// Network Peering
// for p2p, just need this

void NetworkPeer::RegisterObject(Context* context)
{
    context->RegisterFactory<NetworkPeer>();
}

NetworkPeer::NetworkPeer(Context* context) :
    Network(context)
{
    URHO3D_LOGINFO("NetworkPeer::NetworkPeer()");
}

void NetworkPeer::RegisterChannel(const String& name)
{
    if (!registeredChannels_.Contains(name))
        registeredChannels_.Push(name);
}




#pragma once

#include <Urho3D/Container/RefCounted.h>

#include <atomic>
#include <optional>

#include "rtc/rtc.hpp"

#include "DefsCore.h"

namespace Urho3D
{
    class Context;
    class RefCounted;
}

using namespace Urho3D;


class NetworkConnection;
class NetworkTransport;
class NetworkWebTransport;
class NetworkPeerTransport;

enum NetworkTransportType : int
{
    NT_NONE = 0,
    NT_WEBSOCKET,
    NT_PEER
};

// Transport Interface
class GALAXIANMATCH_API NetworkTransport : public RefCounted
{
    friend class NetworkConnection;

public:
    NetworkTransport(NetworkConnection* connection);
    virtual ~NetworkTransport();

    virtual void Connect(const String& adress, const String& type=String::EMPTY) { }
    virtual void Disconnect(int waitMSec = 0) { }

    virtual void SendMessage(const String& data, const String& peer=String::EMPTY) { }
    virtual void SendBuffer(const VectorBuffer& buffer, const String& channel) { }

    void ClearIncomingPackets();

    NetworkTransportType GetType() const { return type_; }
    NetworkConnectionState GetState() const { return state_; }
    const String& GetIdentity() const { return identity_; }
    const StringHash& GetId() const { return id_; }

    bool HasIncomingPackets() const { return receivedPackets_; }
    MutexLock AcquireIncomingPackets(Vector<VectorBuffer >*& packets) { packets = &incomingPackets_; return MutexLock(packetQueueLock_); }

protected:
    NetworkTransportType type_ = NT_NONE;
    NetworkConnectionState state_ = NetworkConnectionState::Disconnected;
    String identity_;
    StringHash id_;

    VectorBuffer preparedMessage_;

    Mutex packetQueueLock_;
    Vector<VectorBuffer > incomingPackets_;
    std::atomic<bool> receivedPackets_;

    WeakPtr<NetworkConnection> connection_;
};


// WebSocket Transport Implementation
class GALAXIANMATCH_API NetworkWebTransport : public NetworkTransport
{
public:
    NetworkWebTransport(NetworkConnection* connection);
    ~NetworkWebTransport();

    void Connect(const String& wsadress, const String& type=String::EMPTY) override;
    void Disconnect(int waitMSec = 0) override;

    void SendMessage(const String& message, const String& peer=String::EMPTY) override;
    void SendBuffer(const VectorBuffer& buffer, const String& peer=String::EMPTY) override;

    void PrepareMessage(const String& order, const String& peer);
    void AddToPreparedMessage(const String& data);
    void AddToPreparedMessage(const rtc::string& data);
    void AddToPreparedMessage(const VectorBuffer& buffer);
    void SendPreparedMessage();

    // callbacks for Signaling (threadable)
    void OnOpen();
    void OnClosed();
    void OnError(rtc::string error);
    void OnMessageBytes(rtc::binary data);
    void OnMessageString(rtc::string data);

    bool HasNewAvailablePeers() const { return newAvailablePeers_; }
    MutexLock AcquireAvailablePeers(StringVector*& availablepeers) { availablepeers = &availablePeers_; newAvailablePeers_ = false; return MutexLock(availablePeersLock_); }

    void RefreshConnectedPeers();
    bool HasNewConnectedPeers() const { return newConnectedPeers_; }
    MutexLock AcquireConnectedPeers(StringVector*& connectedpeers) { connectedpeers = &connectedPeers_; newConnectedPeers_ = false; return MutexLock(connectedPeersLock_); }

    rtc::WebSocket* GetWebSocket() const { return websocket_.get(); }

private:
    void AutoConnectPeers();

    std::shared_ptr<rtc::WebSocket> websocket_ = {};

    std::atomic<bool> newAvailablePeers_;
    Mutex availablePeersLock_;
    StringVector availablePeers_;

    std::atomic<bool> newConnectedPeers_;
    Mutex connectedPeersLock_;
    StringVector connectedPeers_;
};


// WebRTC Transport Implementation
class NetworkPeerTransport;

class DataChannelListener : public RefCounted
{
    friend class NetworkPeerTransport;

public:
    DataChannelListener() { }
    virtual ~DataChannelListener() { }

    void Set(NetworkPeerTransport* transport, std::shared_ptr<rtc::DataChannel> channel);

    void OnChannelOpen();
    void OnChannelClosed();
    void OnChannelMessageBytes(rtc::binary data);
    void OnChannelMessageString(rtc::string data);

private:
    std::shared_ptr<rtc::DataChannel> channel_ = {};
    WeakPtr<NetworkPeerTransport> transport_;
};

class GALAXIANMATCH_API NetworkPeerTransport : public NetworkTransport
{
    friend class DataChannelListener;

public:
    NetworkPeerTransport(NetworkConnection* connection);
    ~NetworkPeerTransport();

    void Connect(const String& peeridentity, const String& type=String::EMPTY) override;
    void Disconnect(int waitMSec = 0) override;

    void SendMessage(const String& data, const String& channelname) override;
    void SendBuffer(const VectorBuffer& buffer, const String& channel) override;

    void SetChannel(const String& channelname, std::optional<std::shared_ptr<rtc::DataChannel> > dataChannel = std::nullopt);

    // callbacks for Peering (threadable)
    void OnDataChannel(std::shared_ptr<rtc::DataChannel> dataChannel);
    void OnTrack(std::shared_ptr<rtc::Track> track);

    void OnLocalDescription(rtc::Description description);
    void OnLocalCandidate(rtc::Candidate candidate);

    void OnStateChange(rtc::PeerConnection::State state);
    void OnGatheringStateChange(rtc::PeerConnection::GatheringState state);
    void OnSignalingStateChange(rtc::PeerConnection::SignalingState state);

    rtc::PeerConnection* GetRTCConnection() const { return peerconnection_.get(); }

private:
    void CreatePeerConnection(bool addchanel);

    std::shared_ptr<rtc::PeerConnection> peerconnection_ = {};

    HashMap<String, SharedPtr<DataChannelListener> > channelListeners_;
};

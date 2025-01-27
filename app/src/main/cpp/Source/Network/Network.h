#pragma once

#include <functional>

#include <Urho3D/Core/Object.h>

#include "DefsCore.h"

#include "NetworkConnection.h"
#include "NetworkTransport.h"

namespace Urho3D
{
    class Context;
    class Object;
}

using namespace Urho3D;

// Enumeration for different network modes
enum NetMode
{
    NetPeering,
    NetClient,
    NetServer,
};

URHO3D_EVENT(N_WEBSOCKET_AVAILABLEPEERSUPDATED, Network_WebSocket_AvailablePeersUpdated)
{
    URHO3D_PARAM(P_CONNECTION, Connection); // Ptr to Connection
    URHO3D_PARAM(P_AVAILABLEPEERS, AvailablePeers); // Ptr to StringVector
};
URHO3D_EVENT(N_WEBSOCKET_CONNECTEDPEERSUPDATED, Network_WebSocket_ConnectedPeersUpdated)
{
    URHO3D_PARAM(P_CONNECTION, Connection); // Ptr to Connection
    URHO3D_PARAM(P_CONNECTEDPEERS, ConnectedPeers); // Ptr to StringVector
};
URHO3D_EVENT(N_INCOMINGPACKETSRECEIVED, Network_IncomingPacketsReceived)
{
    URHO3D_PARAM(P_TRANSPORT, Transport);   // Ptr to Transport
};

class NetworkPeer;
class NetworkClient;
class NetworkServer;

// Network class inheriting from Object
class GALAXIANMATCH_API Network : public Object
{
    URHO3D_OBJECT(Network, Object);

    friend class NetworkConnection;

public:
    static void RegisterLibrary(Context* context);

    // Static method to get the instance of Network based on the mode
    static Network* Get(bool init=true, int netmode = -1)
    {
        if (init && (!instance_ || netmode != -1))
            Init(netmode);
        return instance_.Get();
    }
    static void Remove()
    {
        instance_.Reset();
    }

    Network(Context* context);
    virtual ~Network();

    void Connect(const String& adress, const String& identity);

    void DisconnectAll(int waitMSec = 0);
    void Disconnect(const String& adress, int waitMSec=0);

    void Send(const String& message, const String& channel, const String& peer=String::EMPTY);
    void Send(NetworkConnection* connection, const String& message, const String& channel, const String& peer=String::EMPTY);
    void SendBuffer(const VectorBuffer& buffer, const String& channel, const String& peer=String::EMPTY);
    void SendBuffer(NetworkConnection* connection, const VectorBuffer& buffer, const String& channel, const String& peer=String::EMPTY);

    NetworkConnection* GetConnection() const { return activeWsConnection_; }
    int GetState() const { return state_; }
    void OnAvailablePeersUpdate(std::function<void(const StringVector* peers)> callback) { onAvailablePeersUpdateCallBack_ = callback; }
    void OnConnectedPeersUpdate(std::function<void(const StringVector* peers)> callback) { onConnectedPeersUpdateCallBack_ = callback; }
    void OnMessageReceived(std::function<void(NetworkTransport* transport, Vector<VectorBuffer >* packets)> callback) { onMessageReceivedCallBack_ = callback; }

protected:
    virtual void OnConnected(NetworkConnection* connection);
    virtual void OnDisconnected(NetworkConnection* connection);

    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
    void HandleApplicationExit(StringHash eventType, VariantMap& eventData);

private:
    // Static method to initialize the instance based on the netmode
    static void Init(int netmode);

    void Stop();

protected:
    WeakPtr<NetworkConnection> activeWsConnection_;
    HashMap<String, SharedPtr<NetworkConnection> > connections_;

private:
    std::function<void(const StringVector* peers)> onAvailablePeersUpdateCallBack_;
    std::function<void(const StringVector* peers)> onConnectedPeersUpdateCallBack_;
    std::function<void(NetworkTransport* transport, Vector<VectorBuffer >* packets)> onMessageReceivedCallBack_;

    std::atomic<int> state_;
    Mutex connectionsMutex_;

    // Static member variables to hold the current netmode and instance
    static Context* context_;
    static int netmode_;
    static SharedPtr<Network> instance_;
};


/// NetworkPeer : client side of the network

class GALAXIANMATCH_API NetworkPeer : public Network
{
    URHO3D_OBJECT(NetworkPeer, Network);

public:
    static void RegisterObject(Context* context);

    NetworkPeer(Context* context);

    void RegisterChannel(const String& name);

    const StringVector& GetRegisteredChannels() const { return registeredChannels_; }

private:

    StringVector registeredChannels_;
};

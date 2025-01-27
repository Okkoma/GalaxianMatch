#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/VectorBuffer.h>

#include "DefsCore.h"

namespace Urho3D
{
    class Context;
    class Object;
}

using namespace Urho3D;

class Network;
class NetworkTransport;

enum NetworkConnectionState : int
{
    /// Disconnection was initiated and no data can be sent through the connection any more.
    Disconnecting = 0,
    /// Connection is fully disconnected and idle.
    Disconnected,
    /// Connection is initiated, but has not completed yet.
    Connecting,
    /// Connection is ready for sending and receiving data.
    Connected,
};

class GALAXIANMATCH_API NetworkConnection : public Object
{
    URHO3D_OBJECT(NetworkConnection, Object);

public:
    static void RegisterObject(Context* context);

    NetworkConnection(Context* context);
    ~NetworkConnection() override;

    NetworkTransport* Connect(const String& adress, const String& identity, const String& type=String::EMPTY);
    void SetAutoConnectPeers(int num) { numAutoConnectedPeers_ = num; }

    void Disconnect(int waitMSec = 0);
    void Disconnect(NetworkTransport* transport, int waitMSec=0);

    void Send(const String& data, const String& channel=String::EMPTY, const String& peer=String::EMPTY);
    void SendBuffer(const VectorBuffer& buffer, const String& channel=String::EMPTY, const String& peer=String::EMPTY);

    NetworkTransport* GetTransport() const;
    NetworkTransport* GetTransport(const String& peer) const;
    const Vector<NetworkTransport* >& GetTransports() const { return transports_; }

    const String& GetIdentity() const { return localIdentity_; }
    const String& GetAdress() const { return adress_; }
    const StringHash& GetId() const { return id_; }

    int GetAutoConnectedPeers() const { return numAutoConnectedPeers_; }
    bool AcceptNewConnectPeers() const;
    bool IsConnected() const;
    bool IsConnected(const String& peer) const;

    void OnConnected(NetworkTransport* transport);
    void OnDisconnected(NetworkTransport* transport);

private:
    String adress_;
    String localIdentity_;
    StringHash id_;

    int numAutoConnectedPeers_;

    /// network transports.
    Vector<NetworkTransport* > transports_;

    /// network
    WeakPtr<Network> network_;
};

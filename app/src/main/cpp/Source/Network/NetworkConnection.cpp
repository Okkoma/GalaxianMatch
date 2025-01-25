#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include "Network.h"

#include <iostream>


void NetworkConnection::RegisterObject(Context* context)
{
    context->RegisterFactory<NetworkConnection>();
}

NetworkConnection::NetworkConnection(Context* context) : Object(context), network_(Network::Get()), numAutoConnectedPeers_(0) { }

NetworkConnection::~NetworkConnection()
{
    while (transports_.Size())
    {
        transports_.Back()->Disconnect(0);
        OnDisconnected(transports_.Back());
    }
}

NetworkTransport* NetworkConnection::GetTransport() const
{
    for (Vector<NetworkTransport*>::ConstIterator it = transports_.Begin(); it != transports_.End(); ++it)
    {
        if ((*it)->GetIdentity() == localIdentity_)
            return *it;
    }
    return nullptr;
}

NetworkTransport* NetworkConnection::GetTransport(const String& identity) const
{
    for (Vector<NetworkTransport*>::ConstIterator it = transports_.Begin(); it != transports_.End(); ++it)
    {
        if ((*it)->GetIdentity() == identity)
            return *it;
    }
    return nullptr;
}

NetworkTransport* NetworkConnection::Connect(const String& adress, const String& identity, const String& type)
{
    if (!adress.Empty() && adress != adress_)
        Disconnect(0);

    // find the transport
    NetworkTransport* transport = GetTransport(identity);
    if (transport)
    {
        // try to reconnect
        if (transport->GetState() != NetworkConnectionState::Connected)
            transport->Connect(adress.Empty() ? identity : adress);

        return transport;
    }
    else if (identity != localIdentity_)
    {
        // websocket transport
        if (!adress.Empty())
        {
            localIdentity_ = identity;
            adress_ = adress;
            id_ = StringHash(adress+identity);

            transports_.Push(new NetworkWebTransport(this));
            transports_.Back()->Connect(adress_);
        }
        // peer transport
        else
        {
            transports_.Push(new NetworkPeerTransport(this));
            transports_.Back()->Connect(identity, type);
        }

        return transports_.Back();
    }

    return nullptr;
}

void NetworkConnection::Disconnect(int waitMSec)
{
//    for (Vector<NetworkTransport*>::Iterator it = transports_.Begin(); it != transports_.End(); ++it)
//        (*it)->Disconnect(waitMSec);
    while (transports_.Size())
    {
        transports_.Back()->Disconnect(0);
        OnDisconnected(transports_.Back());
    }
}

void NetworkConnection::Disconnect(NetworkTransport* transport, int waitMSec)
{
    if (transport && transport->GetState() > NetworkConnectionState::Disconnected)
    {
        transport->Disconnect(waitMSec);
        OnDisconnected(transport);
    }
}

bool NetworkConnection::AcceptNewConnectPeers() const
{
    return transports_.Size() && numAutoConnectedPeers_ >= transports_.Size();
}

bool NetworkConnection::IsConnected() const
{
    NetworkTransport* transport = GetTransport();
    return transport && transport->GetState() == NetworkConnectionState::Connected;
}

bool NetworkConnection::IsConnected(const String& identity) const
{
    NetworkTransport* transport = GetTransport(identity);
    return transport && transport->GetState() == NetworkConnectionState::Connected;
}

void NetworkConnection::SendMessage(const String& data, const String& channel, const String& peer)
{
    if (data.Empty())
        return;

    if (!channel.Empty())
    {
        if (peer == "*" || peer.Empty())
        {
            for (Vector<NetworkTransport*>::Iterator it = transports_.Begin(); it != transports_.End(); ++it)
            {
                NetworkTransport* transport = *it;
                if (transport->GetType() == NT_PEER)
                    transport->SendMessage(data, channel);
            }
        }
        else
        {
            NetworkTransport* peertransport = GetTransport(peer);
            if (peertransport)
                peertransport->SendMessage(data, channel);
        }
    }
    else
    {
        NetworkTransport* wstransport = GetTransport();
        if (wstransport)
            wstransport->SendMessage(data, peer);
    }
}

void NetworkConnection::SendBuffer(const VectorBuffer& buffer, const String& channel, const String& peer)
{
    if (!buffer.GetSize())
        return;

    if (!channel.Empty())
    {
        if (peer == "*" || peer.Empty())
        {
            for (Vector<NetworkTransport*>::Iterator it = transports_.Begin(); it != transports_.End(); ++it)
            {
                NetworkTransport* transport = *it;
                if (transport->GetType() == NT_PEER)
                    transport->SendBuffer(buffer, channel);
            }
        }
        else
        {
            NetworkTransport* peertransport = GetTransport(peer);
            if (peertransport)
                peertransport->SendBuffer(buffer, channel);
        }
    }
    else
    {
        NetworkTransport* wstransport = GetTransport();
        if (wstransport)
            wstransport->SendBuffer(buffer, peer);
    }
}

void NetworkConnection::OnConnected(NetworkTransport* transport)
{
    URHO3D_LOGINFOF("NetworkConnection::OnConnected() - this=%u transport %u", this, transport);

    if (network_ && transport && transports_.Size())
        network_->OnConnected(this);
}

void NetworkConnection::OnDisconnected(NetworkTransport* transport)
{
    if (!transport)
        return;

    transports_.RemoveSwap(transport);

    URHO3D_LOGINFOF("NetworkConnection::OnDisconnected() - this=%u transport %u deleted", this, transport);
    delete transport;

    if (network_ && !transports_.Size())
        network_->OnDisconnected(this);
}

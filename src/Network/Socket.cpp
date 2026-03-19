// Must be first to avoid Windows header conflicts
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include "Network/Socket.hpp"
#include <Util/Logger.hpp>
#include <cstring>

namespace Network {

bool Socket::s_ENetInitialized = false;

Socket::Socket() = default;

Socket::~Socket() {
    Close();
}

bool Socket::InitializeENet() {
    if (s_ENetInitialized) return true;

    if (enet_initialize() != 0) {
        LOG_ERROR("Failed to initialize ENet");
        return false;
    }

    s_ENetInitialized = true;
    LOG_INFO("ENet initialized successfully");
    return true;
}

void Socket::ShutdownENet() {
    if (s_ENetInitialized) {
        enet_deinitialize();
        s_ENetInitialized = false;
        LOG_INFO("ENet shutdown");
    }
}

bool Socket::CreateServer(uint16_t port, size_t maxClients) {
    if (!s_ENetInitialized) {
        LOG_ERROR("ENet not initialized");
        return false;
    }

    Close();

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_Host = enet_host_create(&address, maxClients, CHANNEL_COUNT, 0, 0);
    if (!m_Host) {
        LOG_ERROR("Failed to create ENet server on port {}", port);
        return false;
    }

    m_Peers.clear();
    LOG_INFO("ENet server created on port {}", port);
    return true;
}

bool Socket::CreateClient() {
    if (!s_ENetInitialized) {
        LOG_ERROR("ENet not initialized");
        return false;
    }

    Close();

    m_Host = enet_host_create(nullptr, 1, CHANNEL_COUNT, 0, 0);
    if (!m_Host) {
        LOG_ERROR("Failed to create ENet client");
        return false;
    }

    LOG_INFO("ENet client created");
    return true;
}

bool Socket::Connect(const std::string& address, uint16_t port) {
    if (!m_Host) {
        LOG_ERROR("Client not created");
        return false;
    }

    ENetAddress addr;
    enet_address_set_host(&addr, address.c_str());
    addr.port = port;

    m_ServerPeer = enet_host_connect(m_Host, &addr, CHANNEL_COUNT, 0);
    if (!m_ServerPeer) {
        LOG_ERROR("Failed to initiate connection to {}:{}", address, port);
        return false;
    }

    LOG_INFO("Connecting to {}:{}...", address, port);
    return true;
}

void Socket::Disconnect() {
    if (m_ServerPeer) {
        enet_peer_disconnect(m_ServerPeer, 0);
        m_ServerPeer = nullptr;
    }
}

void Socket::Close() {
    if (m_ServerPeer) {
        enet_peer_reset(m_ServerPeer);
        m_ServerPeer = nullptr;
    }

    for (auto* peer : m_Peers) {
        if (peer) {
            enet_peer_reset(peer);
        }
    }
    m_Peers.clear();

    if (m_Host) {
        enet_host_destroy(m_Host);
        m_Host = nullptr;
    }
}

bool Socket::PollEvent(SocketEvent& event, uint32_t timeoutMs) {
    if (!m_Host) return false;

    ENetEvent enetEvent;
    int result = enet_host_service(m_Host, &enetEvent, timeoutMs);

    if (result <= 0) {
        event.type = SocketEventType::None;
        return false;
    }

    switch (enetEvent.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            event.type = SocketEventType::Connect;

            // Assign peer ID (use array index)
            bool found = false;
            for (size_t i = 0; i < m_Peers.size(); ++i) {
                if (m_Peers[i] == nullptr) {
                    m_Peers[i] = enetEvent.peer;
                    enetEvent.peer->data = reinterpret_cast<void*>(i);
                    event.peerId = static_cast<uint32_t>(i);
                    found = true;
                    break;
                }
            }

            if (!found) {
                size_t idx = m_Peers.size();
                m_Peers.push_back(enetEvent.peer);
                enetEvent.peer->data = reinterpret_cast<void*>(idx);
                event.peerId = static_cast<uint32_t>(idx);
            }

            char hostStr[256];
            enet_address_get_host_ip(&enetEvent.peer->address, hostStr, sizeof(hostStr));
            LOG_INFO("Peer {} connected from {}:{}", event.peerId, hostStr, enetEvent.peer->address.port);
            return true;
        }

        case ENET_EVENT_TYPE_DISCONNECT: {
            event.type = SocketEventType::Disconnect;
            event.peerId = static_cast<uint32_t>(reinterpret_cast<size_t>(enetEvent.peer->data));

            // Clear peer slot
            if (event.peerId < m_Peers.size()) {
                m_Peers[event.peerId] = nullptr;
            }

            LOG_INFO("Peer {} disconnected", event.peerId);
            return true;
        }

        case ENET_EVENT_TYPE_RECEIVE: {
            event.type = SocketEventType::Receive;
            event.peerId = static_cast<uint32_t>(reinterpret_cast<size_t>(enetEvent.peer->data));
            event.channel = enetEvent.channelID;
            event.data.assign(enetEvent.packet->data,
                              enetEvent.packet->data + enetEvent.packet->dataLength);

            enet_packet_destroy(enetEvent.packet);
            return true;
        }

        default:
            event.type = SocketEventType::None;
            return false;
    }
}

void Socket::SendToAll(const void* data, size_t size, uint8_t channel, bool reliable) {
    if (!m_Host) return;

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);

    enet_host_broadcast(m_Host, channel, packet);
}

void Socket::SendToPeer(uint32_t peerId, const void* data, size_t size, uint8_t channel, bool reliable) {
    if (!m_Host || peerId >= m_Peers.size() || !m_Peers[peerId]) return;

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);

    enet_peer_send(m_Peers[peerId], channel, packet);
}

void Socket::SendToServer(const void* data, size_t size, uint8_t channel, bool reliable) {
    if (!m_ServerPeer) return;

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);

    enet_peer_send(m_ServerPeer, channel, packet);
}

void Socket::DisconnectPeer(uint32_t peerId) {
    if (peerId >= m_Peers.size() || !m_Peers[peerId]) return;

    enet_peer_disconnect(m_Peers[peerId], 0);
}

std::string Socket::GetPeerAddress(uint32_t peerId) const {
    if (peerId >= m_Peers.size() || !m_Peers[peerId]) return "";

    char hostStr[256];
    enet_address_get_host_ip(&m_Peers[peerId]->address, hostStr, sizeof(hostStr));
    return std::string(hostStr);
}

size_t Socket::GetConnectedPeerCount() const {
    size_t count = 0;
    for (auto* peer : m_Peers) {
        if (peer && peer->state == ENET_PEER_STATE_CONNECTED) {
            ++count;
        }
    }
    return count;
}

} // namespace Network

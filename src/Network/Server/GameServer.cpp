// Must be first to avoid Windows header conflicts
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include "Network/Server/GameServer.hpp"
#include "Network/Packet.hpp"
#include <Util/Logger.hpp>

namespace Network {

GameServer::GameServer() {
    m_UsedPlayerIds.fill(false);
}

GameServer::~GameServer() {
    Stop();
}

bool GameServer::Start(uint16_t port, const std::string& gameName) {
    if (m_IsRunning) {
        LOG_WARN("Server already running");
        return false;
    }

    if (!Socket::InitializeENet()) {
        return false;
    }

    if (!m_Socket.CreateServer(port, MAX_PLAYERS - 1)) {  // -1 because host is also a player
        return false;
    }

    // Start LAN discovery responder
    if (!m_Discovery.StartResponding(gameName, MAX_PLAYERS, port)) {
        LOG_WARN("Failed to start LAN discovery, but server is running");
    }

    m_GameName = gameName;
    m_Port = port;
    m_ServerTick = 0;
    m_IsRunning = true;

    // Reserve player ID 0 for host
    m_UsedPlayerIds[0] = true;

    LOG_INFO("Game server started: {} on port {}", gameName, port);
    return true;
}

void GameServer::Stop() {
    if (!m_IsRunning) return;

    // Notify all clients
    for (const auto& [playerId, client] : m_Clients) {
        auto packet = PacketBuilder::PlayerLeft(playerId);
        m_Socket.SendToPeer(client.peerId, packet.data(), packet.size(),
                            CHANNEL_RELIABLE, true);
    }

    m_Discovery.StopResponding();
    m_Socket.Close();
    m_Clients.clear();
    m_PeerToPlayer.clear();
    m_UsedPlayerIds.fill(false);
    m_IsRunning = false;

    LOG_INFO("Game server stopped");
}

void GameServer::Update(float dt) {
    if (!m_IsRunning) return;

    ++m_ServerTick;

    // Update LAN discovery
    m_Discovery.UpdateHost(static_cast<uint8_t>(m_Clients.size() + 1));  // +1 for host

    // Process network events
    SocketEvent event;
    while (m_Socket.PollEvent(event, 0)) {
        switch (event.type) {
            case SocketEventType::Connect:
                LOG_DEBUG("Peer {} connected, waiting for join request", event.peerId);
                break;

            case SocketEventType::Disconnect:
                HandleDisconnect(event.peerId);
                break;

            case SocketEventType::Receive:
                HandlePacket(event.peerId, event.data);
                break;

            default:
                break;
        }
    }

    m_BroadcastTimer += dt;
}

void GameServer::HandlePacket(uint32_t peerId, const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    PacketType type = static_cast<PacketType>(data[0]);

    switch (type) {
        case PacketType::C2S_JOIN_REQUEST: {
            auto packet = PacketParser::ParseJoinRequest(data);
            if (packet) {
                HandleJoinRequest(peerId, *packet);
            }
            break;
        }

        case PacketType::C2S_INPUT: {
            auto packet = PacketParser::ParseInput(data);
            if (packet) {
                HandleInput(peerId, *packet);
            }
            break;
        }

        case PacketType::C2S_DISCONNECT: {
            HandleDisconnect(peerId);
            break;
        }

        default:
            LOG_WARN("Unknown packet type: {}", static_cast<int>(type));
            break;
    }
}

void GameServer::HandleJoinRequest(uint32_t peerId, const JoinRequestPacket& packet) {
    // Check if server is full
    if (m_Clients.size() >= MAX_PLAYERS - 1) {  // -1 for host
        auto reject = PacketBuilder::JoinRejected(0);  // 0 = server full
        m_Socket.SendToPeer(peerId, reject.data(), reject.size(), CHANNEL_RELIABLE, true);
        LOG_INFO("Rejected join from {}: server full", packet.playerName);
        return;
    }

    // Allocate player ID
    uint8_t playerId = AllocatePlayerId();
    if (playerId == 0xFF) {
        auto reject = PacketBuilder::JoinRejected(0);
        m_Socket.SendToPeer(peerId, reject.data(), reject.size(), CHANNEL_RELIABLE, true);
        return;
    }

    // Create client connection
    ClientConnection client;
    client.peerId = peerId;
    client.playerId = playerId;
    client.playerName = packet.playerName;
    client.isConnected = true;

    // Initialize player state
    client.state.playerId = playerId;
    client.state.health = 100;
    client.state.currentAmmo = 30;
    client.state.flags = FLAG_IS_ALIVE;

    m_Clients[playerId] = client;
    m_PeerToPlayer[peerId] = playerId;

    // Send acceptance
    auto accept = PacketBuilder::JoinAccepted(playerId, static_cast<uint8_t>(m_Clients.size() + 1));
    m_Socket.SendToPeer(peerId, accept.data(), accept.size(), CHANNEL_RELIABLE, true);

    // Notify all other clients about the new player
    auto joinNotify = PacketBuilder::PlayerJoined(playerId, packet.playerName);
    for (const auto& [otherPlayerId, otherClient] : m_Clients) {
        if (otherPlayerId != playerId) {
            m_Socket.SendToPeer(otherClient.peerId, joinNotify.data(), joinNotify.size(),
                                CHANNEL_RELIABLE, true);
        }
    }

    // Notify the new player about existing players
    for (const auto& [existingPlayerId, existingClient] : m_Clients) {
        if (existingPlayerId != playerId) {
            auto existingNotify = PacketBuilder::PlayerJoined(existingPlayerId, existingClient.playerName.c_str());
            m_Socket.SendToPeer(peerId, existingNotify.data(), existingNotify.size(),
                                CHANNEL_RELIABLE, true);
        }
    }

    LOG_INFO("Player {} ({}) joined with ID {}", packet.playerName, peerId, playerId);

    // Callback
    if (m_OnPlayerJoined) {
        m_OnPlayerJoined(playerId, packet.playerName);
    }
}

void GameServer::HandleDisconnect(uint32_t peerId) {
    auto it = m_PeerToPlayer.find(peerId);
    if (it == m_PeerToPlayer.end()) return;

    uint8_t playerId = it->second;
    std::string playerName = m_Clients[playerId].playerName;

    // Free player ID
    FreePlayerId(playerId);

    // Remove from maps
    m_Clients.erase(playerId);
    m_PeerToPlayer.erase(it);

    // Notify other clients
    auto leftNotify = PacketBuilder::PlayerLeft(playerId);
    for (const auto& [otherPlayerId, otherClient] : m_Clients) {
        m_Socket.SendToPeer(otherClient.peerId, leftNotify.data(), leftNotify.size(),
                            CHANNEL_RELIABLE, true);
    }

    LOG_INFO("Player {} ({}) left", playerName, playerId);

    // Callback
    if (m_OnPlayerLeft) {
        m_OnPlayerLeft(playerId);
    }
}

void GameServer::HandleInput(uint32_t peerId, const InputPacket& packet) {
    auto it = m_PeerToPlayer.find(peerId);
    if (it == m_PeerToPlayer.end()) return;

    uint8_t playerId = it->second;
    auto& client = m_Clients[playerId];

    // Only accept newer inputs
    if (packet.sequence > client.lastInputSequence) {
        client.lastInput = packet;
        client.lastInputSequence = packet.sequence;

        // Queue for processing
        PendingInput pending;
        pending.playerId = playerId;
        pending.input = packet;
        m_PendingInputs.push(pending);

        // Callback
        if (m_OnInputReceived) {
            m_OnInputReceived(playerId, packet);
        }
    }
}

void GameServer::BroadcastGameState(const NetPlayerState* players, uint8_t playerCount) {
    if (!m_IsRunning || m_Clients.empty()) return;

    // Rate limiting
    if (m_BroadcastTimer < m_BroadcastInterval) return;
    m_BroadcastTimer = 0.0f;

    // Build state packet for each client (with their specific lastAckedInput)
    for (const auto& [playerId, client] : m_Clients) {
        auto packet = PacketBuilder::GameState(
            m_ServerTick,
            client.lastInputSequence,
            players,
            playerCount
        );

        m_Socket.SendToPeer(client.peerId, packet.data(), packet.size(),
                            CHANNEL_UNRELIABLE, false);
    }
}

void GameServer::BroadcastPlayerHit(uint8_t victimId, uint8_t attackerId,
                                     uint8_t newHealth, const glm::vec3& hitPos) {
    auto packet = PacketBuilder::PlayerHit(victimId, attackerId, newHealth, hitPos);
    m_Socket.SendToAll(packet.data(), packet.size(), CHANNEL_RELIABLE, true);
}

void GameServer::BroadcastPlayerDeath(uint8_t victimId, uint8_t killerId) {
    auto packet = PacketBuilder::PlayerDeath(victimId, killerId);
    m_Socket.SendToAll(packet.data(), packet.size(), CHANNEL_RELIABLE, true);
}

void GameServer::BroadcastBulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    auto packet = PacketBuilder::BulletEffect(pos, normal);
    m_Socket.SendToAll(packet.data(), packet.size(), CHANNEL_UNRELIABLE, false);
}

std::vector<PendingInput> GameServer::GetPendingInputs() {
    std::vector<PendingInput> inputs;
    while (!m_PendingInputs.empty()) {
        inputs.push_back(m_PendingInputs.front());
        m_PendingInputs.pop();
    }
    return inputs;
}

uint32_t GameServer::GetLastAckedInput(uint8_t playerId) const {
    auto it = m_Clients.find(playerId);
    if (it == m_Clients.end()) return 0;
    return it->second.lastInputSequence;
}

uint8_t GameServer::AllocatePlayerId() {
    // Start from 1 (0 is reserved for host)
    for (uint8_t i = 1; i < MAX_PLAYERS; ++i) {
        if (!m_UsedPlayerIds[i]) {
            m_UsedPlayerIds[i] = true;
            return i;
        }
    }
    return 0xFF;  // No available ID
}

void GameServer::FreePlayerId(uint8_t playerId) {
    if (playerId < MAX_PLAYERS) {
        m_UsedPlayerIds[playerId] = false;
    }
}

} // namespace Network

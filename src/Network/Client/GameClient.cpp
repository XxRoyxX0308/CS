// Must be first to avoid Windows header conflicts
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include "Network/Client/GameClient.hpp"
#include "Network/Packet.hpp"
#include <Util/Logger.hpp>

namespace Network {

GameClient::GameClient() = default;

GameClient::~GameClient() {
    Disconnect();
}

bool GameClient::Connect(const std::string& address, uint16_t port, const std::string& playerName) {
    if (m_ConnectionState != ConnectionState::Disconnected) {
        LOG_WARN("Already connected or connecting");
        return false;
    }

    if (!Socket::InitializeENet()) {
        return false;
    }

    if (!m_Socket.CreateClient()) {
        return false;
    }

    if (!m_Socket.Connect(address, port)) {
        m_Socket.Close();
        return false;
    }

    m_PlayerName = playerName;
    m_ConnectionState = ConnectionState::Connecting;
    m_ConnectionTimer = 0.0f;
    m_LocalTime = 0.0f;
    m_InputSequence = 0;
    m_LastAckedInput = 0;
    m_StateBuffer.Clear();

    LOG_INFO("Connecting to {}:{} as {}", address, port, playerName);
    return true;
}

void GameClient::Disconnect() {
    if (m_ConnectionState == ConnectionState::Disconnected) return;

    m_Socket.Disconnect();
    m_Socket.Close();
    m_ConnectionState = ConnectionState::Disconnected;
    m_LocalPlayerId = 0xFF;
    m_StateBuffer.Clear();

    LOG_INFO("Disconnected from server");

    if (m_OnDisconnected) {
        m_OnDisconnected();
    }
}

void GameClient::Update(float dt) {
    m_LocalTime += dt;

    // Update discovery if active
    if (m_Discovery.IsDiscovering()) {
        m_Discovery.UpdateClient(dt);
    }

    if (m_ConnectionState == ConnectionState::Disconnected) return;

    // Check for connection timeout
    if (m_ConnectionState == ConnectionState::Connecting) {
        m_ConnectionTimer += dt;
        if (m_ConnectionTimer > m_ConnectionTimeout) {
            LOG_ERROR("Connection timeout");
            Disconnect();
            return;
        }
    }

    // Process network events
    SocketEvent event;
    while (m_Socket.PollEvent(event, 0)) {
        switch (event.type) {
            case SocketEventType::Connect:
                // Connection established, send join request
                {
                    auto packet = PacketBuilder::JoinRequest(m_PlayerName.c_str());
                    m_Socket.SendToServer(packet.data(), packet.size(), CHANNEL_RELIABLE, true);
                    LOG_DEBUG("Sent join request");
                }
                break;

            case SocketEventType::Disconnect:
                LOG_INFO("Disconnected by server");
                m_ConnectionState = ConnectionState::Disconnected;
                m_LocalPlayerId = 0xFF;
                if (m_OnDisconnected) {
                    m_OnDisconnected();
                }
                break;

            case SocketEventType::Receive:
                HandlePacket(event.data);
                break;

            default:
                break;
        }
    }
}

void GameClient::HandlePacket(const std::vector<uint8_t>& data) {
    if (data.empty()) return;

    PacketType type = static_cast<PacketType>(data[0]);

    switch (type) {
        case PacketType::S2C_JOIN_ACCEPTED: {
            auto packet = PacketParser::ParseJoinAccepted(data);
            if (packet) HandleJoinAccepted(*packet);
            break;
        }

        case PacketType::S2C_JOIN_REJECTED: {
            JoinRejectedPacket packet;
            if (data.size() >= sizeof(packet)) {
                std::memcpy(&packet, data.data(), sizeof(packet));
                HandleJoinRejected(packet);
            }
            break;
        }

        case PacketType::S2C_PLAYER_JOINED: {
            PlayerJoinedPacket packet;
            if (data.size() >= sizeof(packet)) {
                std::memcpy(&packet, data.data(), sizeof(packet));
                HandlePlayerJoined(packet);
            }
            break;
        }

        case PacketType::S2C_PLAYER_LEFT: {
            PlayerLeftPacket packet;
            if (data.size() >= sizeof(packet)) {
                std::memcpy(&packet, data.data(), sizeof(packet));
                HandlePlayerLeft(packet);
            }
            break;
        }

        case PacketType::S2C_GAME_STATE: {
            auto packet = PacketParser::ParseGameState(data);
            if (packet) HandleGameState(*packet);
            break;
        }

        case PacketType::S2C_PLAYER_HIT: {
            auto packet = PacketParser::ParsePlayerHit(data);
            if (packet) HandlePlayerHit(*packet);
            break;
        }

        case PacketType::S2C_PLAYER_DEATH: {
            auto packet = PacketParser::ParsePlayerDeath(data);
            if (packet) HandlePlayerDeath(*packet);
            break;
        }

        case PacketType::S2C_BULLET_EFFECT: {
            auto packet = PacketParser::ParseBulletEffect(data);
            if (packet) HandleBulletEffect(*packet);
            break;
        }

        case PacketType::S2C_PLAYER_CONFIG: {
            auto packet = PacketParser::ParsePlayerConfig(data);
            if (packet) HandlePlayerConfig(*packet);
            break;
        }

        default:
            LOG_WARN("Unknown packet type: {}", static_cast<int>(type));
            break;
    }
}

void GameClient::HandleJoinAccepted(const JoinAcceptedPacket& packet) {
    m_LocalPlayerId = packet.playerId;
    m_ConnectionState = ConnectionState::Connected;

    LOG_INFO("Joined server as player {}, {} players online",
             m_LocalPlayerId, packet.currentPlayerCount);

    if (m_OnConnected) {
        m_OnConnected(m_LocalPlayerId);
    }
}

void GameClient::HandleJoinRejected(const JoinRejectedPacket& packet) {
    LOG_ERROR("Join rejected, reason: {}", packet.reason);
    m_ConnectionState = ConnectionState::Disconnected;
    m_Socket.Close();

    if (m_OnRejected) {
        m_OnRejected(packet.reason);
    }
}

void GameClient::HandlePlayerJoined(const PlayerJoinedPacket& packet) {
    LOG_INFO("Player {} ({}) joined", packet.playerName, packet.playerId);

    if (m_OnPlayerJoined) {
        m_OnPlayerJoined(packet.playerId, packet.playerName);
    }
}

void GameClient::HandlePlayerLeft(const PlayerLeftPacket& packet) {
    LOG_INFO("Player {} left", packet.playerId);

    if (m_OnPlayerLeft) {
        m_OnPlayerLeft(packet.playerId);
    }
}

void GameClient::HandleGameState(const GameStatePacket& packet) {
    // Update last acknowledged input
    m_LastAckedInput = packet.lastAckedInput;

    // Create snapshot for interpolation
    StateSnapshot snapshot;
    snapshot.serverTick = packet.serverTick;
    snapshot.localTime = m_LocalTime;

    for (uint8_t i = 0; i < packet.playerCount && i < MAX_PLAYERS; ++i) {
        const auto& state = packet.players[i];
        snapshot.players[state.playerId] = state;
    }

    m_StateBuffer.PushSnapshot(snapshot);
}

void GameClient::HandlePlayerHit(const PlayerHitPacket& packet) {
    glm::vec3 hitPos(packet.hitX, packet.hitY, packet.hitZ);

    if (m_OnPlayerHit) {
        m_OnPlayerHit(packet.victimId, packet.attackerId, packet.newHealth, hitPos);
    }
}

void GameClient::HandleBulletEffect(const BulletEffectPacket& packet) {
    glm::vec3 pos(packet.x, packet.y, packet.z);
    glm::vec3 normal(packet.nx, packet.ny, packet.nz);

    if (m_OnBulletEffect) {
        m_OnBulletEffect(pos, normal);
    }
}

void GameClient::HandlePlayerDeath(const PlayerDeathPacket& packet) {
    LOG_INFO("Player {} was killed by player {}", packet.victimId, packet.killerId);

    if (m_OnPlayerDeath) {
        m_OnPlayerDeath(packet.victimId, packet.killerId);
    }
}

void GameClient::HandlePlayerConfig(const PlayerConfigPacket& packet) {
    LOG_INFO("Received config for player {}: character={}, gun={}",
             packet.playerId, packet.characterType, packet.gunType);

    if (m_OnPlayerConfig) {
        m_OnPlayerConfig(packet.playerId, packet.characterType, packet.gunType);
    }
}

void GameClient::SendInput(const InputState& input) {
    if (m_ConnectionState != ConnectionState::Connected) return;

    ++m_InputSequence;

    auto packet = PacketBuilder::Input(m_InputSequence, input);
    m_Socket.SendToServer(packet.data(), packet.size(), CHANNEL_UNRELIABLE, false);
}

void GameClient::SendBulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    if (m_ConnectionState != ConnectionState::Connected) return;

    auto packet = PacketBuilder::ClientBulletEffect(pos, normal);
    m_Socket.SendToServer(packet.data(), packet.size(), CHANNEL_RELIABLE, true);
}

void GameClient::SendPlayerHit(uint8_t victimId, float damage, const glm::vec3& hitPos) {
    if (m_ConnectionState != ConnectionState::Connected) return;

    auto packet = PacketBuilder::ClientPlayerHit(victimId, damage, hitPos);
    m_Socket.SendToServer(packet.data(), packet.size(), CHANNEL_RELIABLE, true);

    LOG_INFO("Sent player hit: victim={}, damage={}", victimId, damage);
}

void GameClient::SendPlayerConfig(uint8_t characterType, uint8_t gunType) {
    if (m_ConnectionState != ConnectionState::Connected) return;

    auto packet = PacketBuilder::ClientPlayerConfig(characterType, gunType);
    m_Socket.SendToServer(packet.data(), packet.size(), CHANNEL_RELIABLE, true);

    LOG_INFO("Sent player config: character={}, gun={}", characterType, gunType);
}

std::optional<NetPlayerState> GameClient::GetLocalPlayerState() const {
    return m_StateBuffer.GetLatestState(m_LocalPlayerId);
}

std::optional<NetPlayerState> GameClient::GetInterpolatedState(uint8_t playerId, float renderTime) const {
    return m_StateBuffer.GetInterpolatedState(playerId, renderTime);
}

std::vector<uint8_t> GameClient::GetRemotePlayerIds() const {
    auto ids = m_StateBuffer.GetTrackedPlayerIds();
    // Remove local player
    ids.erase(std::remove(ids.begin(), ids.end(), m_LocalPlayerId), ids.end());
    return ids;
}

void GameClient::StartDiscovery() {
    m_Discovery.StartDiscovery();
}

void GameClient::StopDiscovery() {
    m_Discovery.StopDiscovery();
}

const std::vector<ServerInfo>& GameClient::GetDiscoveredServers() const {
    return m_Discovery.GetDiscoveredServers();
}

} // namespace Network

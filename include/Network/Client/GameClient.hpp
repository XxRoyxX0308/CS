#ifndef CS_NETWORK_CLIENT_GAMECLIENT_HPP
#define CS_NETWORK_CLIENT_GAMECLIENT_HPP

#include "Network/Socket.hpp"
#include "Network/Types.hpp"
#include "Network/Client/Interpolation.hpp"
#include "Network/Discovery/LANDiscovery.hpp"
#include <functional>
#include <string>
#include <optional>

namespace Network {

// Connection State

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected
};

// Game Client Class

class GameClient {
public:
    using OnConnectedCallback = std::function<void(uint8_t playerId)>;
    using OnDisconnectedCallback = std::function<void()>;
    using OnRejectedCallback = std::function<void(uint8_t reason)>;
    using OnPlayerJoinedCallback = std::function<void(uint8_t playerId, const std::string& name)>;
    using OnPlayerLeftCallback = std::function<void(uint8_t playerId)>;
    using OnPlayerHitCallback = std::function<void(uint8_t victimId, uint8_t attackerId, uint8_t newHealth, const glm::vec3& hitPos)>;
    using OnPlayerDeathCallback = std::function<void(uint8_t victimId, uint8_t killerId)>;
    using OnBulletEffectCallback = std::function<void(const glm::vec3& pos, const glm::vec3& normal)>;
    using OnPlayerConfigCallback = std::function<void(uint8_t playerId, uint8_t characterType, uint8_t gunType)>;

    GameClient();
    ~GameClient();

    // Connection
    bool Connect(const std::string& address, uint16_t port, const std::string& playerName);
    void Disconnect();
    ConnectionState GetConnectionState() const { return m_ConnectionState; }
    bool IsConnected() const { return m_ConnectionState == ConnectionState::Connected; }

    // Update (call each frame)
    void Update(float dt);

    // Send input to server
    void SendInput(const InputState& input);

    // Send bullet effect to server (client hit detection)
    void SendBulletEffect(const glm::vec3& pos, const glm::vec3& normal);

    // Send player hit to server (client detected hitting another player)
    void SendPlayerHit(uint8_t victimId, float damage, const glm::vec3& hitPos);

    // Send player config to server (character/gun type)
    void SendPlayerConfig(uint8_t characterType, uint8_t gunType);

    // Get local player ID (assigned by server)
    uint8_t GetLocalPlayerId() const { return m_LocalPlayerId; }

    // Get player state (for local player, from server)
    std::optional<NetPlayerState> GetLocalPlayerState() const;

    // Get interpolated state for remote players
    std::optional<NetPlayerState> GetInterpolatedState(uint8_t playerId, float renderTime) const;

    // Get all tracked player IDs (excluding local player)
    std::vector<uint8_t> GetRemotePlayerIds() const;

    // Get render time (current time - interpolation delay)
    float GetRenderTime() const { return m_LocalTime - INTERP_DELAY; }

    // Get last acknowledged input sequence
    uint32_t GetLastAckedInput() const { return m_LastAckedInput; }

    // LAN Discovery
    void StartDiscovery();
    void StopDiscovery();
    const std::vector<ServerInfo>& GetDiscoveredServers() const;

    // Callbacks
    void SetOnConnected(OnConnectedCallback cb) { m_OnConnected = std::move(cb); }
    void SetOnDisconnected(OnDisconnectedCallback cb) { m_OnDisconnected = std::move(cb); }
    void SetOnRejected(OnRejectedCallback cb) { m_OnRejected = std::move(cb); }
    void SetOnPlayerJoined(OnPlayerJoinedCallback cb) { m_OnPlayerJoined = std::move(cb); }
    void SetOnPlayerLeft(OnPlayerLeftCallback cb) { m_OnPlayerLeft = std::move(cb); }
    void SetOnPlayerHit(OnPlayerHitCallback cb) { m_OnPlayerHit = std::move(cb); }
    void SetOnPlayerDeath(OnPlayerDeathCallback cb) { m_OnPlayerDeath = std::move(cb); }
    void SetOnBulletEffect(OnBulletEffectCallback cb) { m_OnBulletEffect = std::move(cb); }
    void SetOnPlayerConfig(OnPlayerConfigCallback cb) { m_OnPlayerConfig = std::move(cb); }

private:
    void HandlePacket(const std::vector<uint8_t>& data);
    void HandleJoinAccepted(const JoinAcceptedPacket& packet);
    void HandleJoinRejected(const JoinRejectedPacket& packet);
    void HandlePlayerJoined(const PlayerJoinedPacket& packet);
    void HandlePlayerLeft(const PlayerLeftPacket& packet);
    void HandleGameState(const GameStatePacket& packet);
    void HandlePlayerHit(const PlayerHitPacket& packet);
    void HandlePlayerDeath(const PlayerDeathPacket& packet);
    void HandleBulletEffect(const BulletEffectPacket& packet);
    void HandlePlayerConfig(const PlayerConfigPacket& packet);

    Socket m_Socket;
    LANDiscovery m_Discovery;
    ConnectionState m_ConnectionState = ConnectionState::Disconnected;

    // Local player info
    uint8_t m_LocalPlayerId = 0xFF;
    std::string m_PlayerName;
    uint32_t m_InputSequence = 0;

    // Time tracking
    float m_LocalTime = 0.0f;

    // State buffer for interpolation
    StateBuffer m_StateBuffer;
    uint32_t m_LastAckedInput = 0;

    // Connection timeout
    float m_ConnectionTimeout = 5.0f;
    float m_ConnectionTimer = 0.0f;

    // Callbacks
    OnConnectedCallback m_OnConnected;
    OnDisconnectedCallback m_OnDisconnected;
    OnRejectedCallback m_OnRejected;
    OnPlayerJoinedCallback m_OnPlayerJoined;
    OnPlayerLeftCallback m_OnPlayerLeft;
    OnPlayerHitCallback m_OnPlayerHit;
    OnPlayerDeathCallback m_OnPlayerDeath;
    OnBulletEffectCallback m_OnBulletEffect;
    OnPlayerConfigCallback m_OnPlayerConfig;
};

} // namespace Network

#endif // CS_NETWORK_CLIENT_GAMECLIENT_HPP

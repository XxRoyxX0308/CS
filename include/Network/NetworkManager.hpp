#ifndef CS_NETWORK_NETWORKMANAGER_HPP
#define CS_NETWORK_NETWORKMANAGER_HPP

#include "Network/Types.hpp"
#include "Network/Server/GameServer.hpp"
#include "Network/Client/GameClient.hpp"
#include <memory>
#include <functional>

namespace Network {

// Network Mode

enum class NetworkMode {
    Offline,
    Host,
    Client
};

// Network Manager (Facade)

class NetworkManager {
public:
    // Callbacks
    using OnPlayerJoinedCallback = std::function<void(uint8_t playerId, const std::string& name)>;
    using OnPlayerLeftCallback = std::function<void(uint8_t playerId)>;
    using OnConnectedCallback = std::function<void(uint8_t playerId)>;
    using OnDisconnectedCallback = std::function<void()>;
    using OnPlayerHitCallback = std::function<void(uint8_t victimId, uint8_t attackerId, uint8_t newHealth, const glm::vec3& hitPos)>;
    using OnPlayerDeathCallback = std::function<void(uint8_t victimId, uint8_t killerId)>;
    using OnBulletEffectCallback = std::function<void(const glm::vec3& pos, const glm::vec3& normal)>;
    using OnPlayerConfigCallback = std::function<void(uint8_t playerId, uint8_t characterType, uint8_t gunType)>;
    using OnClientPlayerHitCallback = std::function<void(uint8_t attackerId, uint8_t victimId, float damage, const glm::vec3& hitPos)>;

    NetworkManager();
    ~NetworkManager();

    // Mode Control

    bool HostGame(uint16_t port, const std::string& gameName, const std::string& hostPlayerName = "Host");
    bool JoinGame(const std::string& address, uint16_t port, const std::string& playerName);
    void Disconnect();

    NetworkMode GetMode() const { return m_Mode; }
    bool IsHost() const { return m_Mode == NetworkMode::Host; }
    bool IsClient() const { return m_Mode == NetworkMode::Client; }
    bool IsOnline() const { return m_Mode != NetworkMode::Offline; }
    bool IsConnected() const;

    // Update

    void Update(float dt);

    // Input (Client mode)

    void SendInput(const InputState& input);
    uint32_t GetInputSequence() const { return m_InputSequence; }

    // State Access

    // Get local player ID
    uint8_t GetLocalPlayerId() const { return m_LocalPlayerId; }

    // For host: get pending inputs from remote players
    std::vector<PendingInput> GetPendingInputs();

    // For client: get interpolated state for remote players
    std::optional<NetPlayerState> GetInterpolatedState(uint8_t playerId) const;

    // For client: get local player state from server
    std::optional<NetPlayerState> GetLocalPlayerState() const;

    // For client: get last acknowledged input sequence
    uint32_t GetLastAckedInput() const;

    // For client: get render time
    float GetRenderTime() const;

    // Get all remote player IDs
    std::vector<uint8_t> GetRemotePlayerIds() const;

    // Broadcast (Host mode)

    void BroadcastGameState(const NetPlayerState* players, uint8_t playerCount);
    void BroadcastPlayerHit(uint8_t victimId, uint8_t attackerId, uint8_t newHealth, const glm::vec3& hitPos);
    void BroadcastPlayerDeath(uint8_t victimId, uint8_t killerId);
    void BroadcastBulletEffect(const glm::vec3& pos, const glm::vec3& normal);

    // Send Effects (Client mode)

    void SendBulletEffect(const glm::vec3& pos, const glm::vec3& normal);
    void SendPlayerConfig(uint8_t characterType, uint8_t gunType);
    void SendPlayerHit(uint8_t victimId, float damage, const glm::vec3& hitPos);

    // Broadcast (Host mode)

    void BroadcastPlayerConfig(uint8_t playerId, uint8_t characterType, uint8_t gunType);

    // LAN Discovery

    void StartDiscovery();
    void StopDiscovery();
    bool IsDiscovering() const;
    const std::vector<ServerInfo>& GetDiscoveredServers() const;

    // Callbacks

    void SetOnPlayerJoined(OnPlayerJoinedCallback cb) { m_OnPlayerJoined = std::move(cb); }
    void SetOnPlayerLeft(OnPlayerLeftCallback cb) { m_OnPlayerLeft = std::move(cb); }
    void SetOnConnected(OnConnectedCallback cb) { m_OnConnected = std::move(cb); }
    void SetOnDisconnected(OnDisconnectedCallback cb) { m_OnDisconnected = std::move(cb); }
    void SetOnPlayerHit(OnPlayerHitCallback cb) { m_OnPlayerHit = std::move(cb); }
    void SetOnPlayerDeath(OnPlayerDeathCallback cb) { m_OnPlayerDeath = std::move(cb); }
    void SetOnBulletEffect(OnBulletEffectCallback cb) { m_OnBulletEffect = std::move(cb); }
    void SetOnPlayerConfig(OnPlayerConfigCallback cb) { m_OnPlayerConfig = std::move(cb); }
    void SetOnClientPlayerHit(OnClientPlayerHitCallback cb) { m_OnClientPlayerHit = std::move(cb); }

    // Server Info

    uint8_t GetPlayerCount() const;
    const std::string& GetGameName() const;

private:
    void SetupServerCallbacks();
    void SetupClientCallbacks();

    NetworkMode m_Mode = NetworkMode::Offline;
    uint8_t m_LocalPlayerId = 0;
    uint32_t m_InputSequence = 0;

    std::unique_ptr<GameServer> m_Server;
    std::unique_ptr<GameClient> m_Client;

    // For discovery when not connected
    LANDiscovery m_Discovery;

    // Callbacks
    OnPlayerJoinedCallback m_OnPlayerJoined;
    OnPlayerLeftCallback m_OnPlayerLeft;
    OnConnectedCallback m_OnConnected;
    OnDisconnectedCallback m_OnDisconnected;
    OnPlayerHitCallback m_OnPlayerHit;
    OnPlayerDeathCallback m_OnPlayerDeath;
    OnBulletEffectCallback m_OnBulletEffect;
    OnPlayerConfigCallback m_OnPlayerConfig;
    OnClientPlayerHitCallback m_OnClientPlayerHit;
};

} // namespace Network

#endif // CS_NETWORK_NETWORKMANAGER_HPP

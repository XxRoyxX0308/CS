#ifndef NETWORK_GAME_SERVER_HPP
#define NETWORK_GAME_SERVER_HPP

#include "Network/Socket.hpp"
#include "Network/NetworkTypes.hpp"
#include "Network/Discovery/LANDiscovery.hpp"
#include <unordered_map>
#include <functional>
#include <string>
#include <queue>
#include <array>

namespace Network {

// ── Client Connection State ─────────────────────────────────────────────────

struct ClientConnection {
    uint32_t peerId = 0;
    uint8_t playerId = 0;
    std::string playerName;
    bool isConnected = false;

    // Latest input from client
    InputPacket lastInput{};
    uint32_t lastInputSequence = 0;

    // Player state (owned by server)
    NetPlayerState state{};
};

// ── Pending Input (for processing) ──────────────────────────────────────────

struct PendingInput {
    uint8_t playerId;
    InputPacket input;
};

// ── Game Server Class ───────────────────────────────────────────────────────

class GameServer {
public:
    using OnPlayerJoinedCallback = std::function<void(uint8_t playerId, const std::string& name)>;
    using OnPlayerLeftCallback = std::function<void(uint8_t playerId)>;
    using OnInputReceivedCallback = std::function<void(uint8_t playerId, const InputPacket& input)>;

    GameServer();
    ~GameServer();

    // Server lifecycle
    bool Start(uint16_t port, const std::string& gameName);
    void Stop();
    bool IsRunning() const { return m_IsRunning; }

    // Update (call each frame)
    void Update(float dt);

    // Broadcast game state to all clients
    void BroadcastGameState(const NetPlayerState* players, uint8_t playerCount);

    // Broadcast effects
    void BroadcastPlayerHit(uint8_t victimId, uint8_t attackerId, uint8_t newHealth, const glm::vec3& hitPos);
    void BroadcastPlayerDeath(uint8_t victimId, uint8_t killerId);
    void BroadcastBulletEffect(const glm::vec3& pos, const glm::vec3& normal);

    // Get pending inputs from clients (for host to process)
    std::vector<PendingInput> GetPendingInputs();

    // Get client info
    const std::unordered_map<uint8_t, ClientConnection>& GetClients() const { return m_Clients; }
    uint8_t GetClientCount() const { return static_cast<uint8_t>(m_Clients.size()); }

    // Get last acknowledged input sequence for a client
    uint32_t GetLastAckedInput(uint8_t playerId) const;

    // Callbacks
    void SetOnPlayerJoined(OnPlayerJoinedCallback cb) { m_OnPlayerJoined = std::move(cb); }
    void SetOnPlayerLeft(OnPlayerLeftCallback cb) { m_OnPlayerLeft = std::move(cb); }
    void SetOnInputReceived(OnInputReceivedCallback cb) { m_OnInputReceived = std::move(cb); }

    // Server info
    const std::string& GetGameName() const { return m_GameName; }
    uint16_t GetPort() const { return m_Port; }
    uint32_t GetServerTick() const { return m_ServerTick; }

private:
    void HandlePacket(uint32_t peerId, const std::vector<uint8_t>& data);
    void HandleJoinRequest(uint32_t peerId, const JoinRequestPacket& packet);
    void HandleDisconnect(uint32_t peerId);
    void HandleInput(uint32_t peerId, const InputPacket& packet);

    uint8_t AllocatePlayerId();
    void FreePlayerId(uint8_t playerId);

    Socket m_Socket;
    LANDiscovery m_Discovery;

    bool m_IsRunning = false;
    std::string m_GameName;
    uint16_t m_Port = DEFAULT_PORT;
    uint32_t m_ServerTick = 0;

    // Client management
    std::unordered_map<uint8_t, ClientConnection> m_Clients;  // playerId -> connection
    std::unordered_map<uint32_t, uint8_t> m_PeerToPlayer;     // peerId -> playerId
    std::array<bool, MAX_PLAYERS> m_UsedPlayerIds{};

    // Pending inputs from clients
    std::queue<PendingInput> m_PendingInputs;

    // Callbacks
    OnPlayerJoinedCallback m_OnPlayerJoined;
    OnPlayerLeftCallback m_OnPlayerLeft;
    OnInputReceivedCallback m_OnInputReceived;

    // State broadcast rate limiting
    float m_BroadcastTimer = 0.0f;
    float m_BroadcastInterval = 1.0f / STATE_BROADCAST_RATE;
};

} // namespace Network

#endif // NETWORK_GAME_SERVER_HPP

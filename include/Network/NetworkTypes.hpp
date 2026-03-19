#ifndef NETWORK_TYPES_HPP
#define NETWORK_TYPES_HPP

// Prevent Windows macros from interfering with std::min/max and other libs
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace Network {

// ── Constants ───────────────────────────────────────────────────────────────

constexpr uint16_t DEFAULT_PORT = 27015;
constexpr uint16_t DISCOVERY_PORT = 5555;
constexpr uint8_t MAX_PLAYERS = 8;
constexpr float STATE_BROADCAST_RATE = 20.0f;  // Hz
constexpr float INTERP_DELAY = 0.1f;           // 100ms interpolation delay
constexpr size_t STATE_BUFFER_SIZE = 32;       // ~1 second at 30Hz

// Input bitmask flags
constexpr uint8_t INPUT_W      = 0x01;
constexpr uint8_t INPUT_S      = 0x02;
constexpr uint8_t INPUT_A      = 0x04;
constexpr uint8_t INPUT_D      = 0x08;
constexpr uint8_t INPUT_JUMP   = 0x10;
constexpr uint8_t INPUT_FIRE   = 0x20;
constexpr uint8_t INPUT_RELOAD = 0x40;

// Player flags
constexpr uint8_t FLAG_ON_GROUND   = 0x01;
constexpr uint8_t FLAG_IS_RELOADING = 0x02;
constexpr uint8_t FLAG_IS_ALIVE    = 0x04;
constexpr uint8_t FLAG_IS_WALKING  = 0x08;

// Magic bytes for LAN discovery
constexpr uint8_t DISCOVERY_MAGIC[4] = {'C', 'S', 'F', 'P'};

// ── Packet Types ────────────────────────────────────────────────────────────

enum class PacketType : uint8_t {
    // Connection
    C2S_JOIN_REQUEST   = 0x01,
    S2C_JOIN_ACCEPTED  = 0x02,
    S2C_JOIN_REJECTED  = 0x03,
    C2S_DISCONNECT     = 0x04,
    S2C_PLAYER_JOINED  = 0x05,
    S2C_PLAYER_LEFT    = 0x06,

    // LAN Discovery (UDP broadcast)
    BROADCAST_QUERY    = 0x10,
    BROADCAST_RESPONSE = 0x11,

    // Input (Client → Server)
    C2S_INPUT          = 0x20,

    // State (Server → Client)
    S2C_GAME_STATE     = 0x30,
    S2C_PLAYER_HIT     = 0x31,
    S2C_PLAYER_DEATH   = 0x32,
    S2C_BULLET_EFFECT  = 0x33,

    // Sync
    S2C_FULL_SYNC      = 0x40,
};

// ── Packet Structures ───────────────────────────────────────────────────────

#pragma pack(push, 1)

struct PacketHeader {
    uint8_t type;
};

// ── Connection Packets ──

struct JoinRequestPacket {
    PacketHeader header;
    char playerName[32];
};

struct JoinAcceptedPacket {
    PacketHeader header;
    uint8_t playerId;
    uint8_t currentPlayerCount;
};

struct JoinRejectedPacket {
    PacketHeader header;
    uint8_t reason;  // 0 = server full, 1 = game in progress
};

struct PlayerJoinedPacket {
    PacketHeader header;
    uint8_t playerId;
    char playerName[32];
};

struct PlayerLeftPacket {
    PacketHeader header;
    uint8_t playerId;
};

// ── LAN Discovery Packets ──

struct DiscoveryQueryPacket {
    uint8_t magic[4];
    uint8_t type;
};

struct DiscoveryResponsePacket {
    uint8_t magic[4];
    uint8_t type;
    char gameName[32];
    uint8_t playerCount;
    uint8_t maxPlayers;
    uint16_t gamePort;
};

// ── Input Packet ──

struct InputPacket {
    PacketHeader header;
    uint32_t sequence;
    uint8_t keys;
    float yaw;
    float pitch;
};

// ── Player State (within GameState) ──

struct NetPlayerState {
    uint8_t playerId;
    float posX, posY, posZ;
    float yaw, pitch;
    float velocityY;
    uint8_t health;
    uint8_t currentAmmo;
    uint8_t flags;

    glm::vec3 GetPosition() const { return glm::vec3(posX, posY, posZ); }
    void SetPosition(const glm::vec3& pos) {
        posX = pos.x; posY = pos.y; posZ = pos.z;
    }

    bool IsOnGround() const { return flags & FLAG_ON_GROUND; }
    bool IsReloading() const { return flags & FLAG_IS_RELOADING; }
    bool IsAlive() const { return flags & FLAG_IS_ALIVE; }
    bool IsWalking() const { return flags & FLAG_IS_WALKING; }
};

// ── Game State Packet ──

struct GameStatePacket {
    PacketHeader header;
    uint32_t serverTick;
    uint32_t lastAckedInput;  // For client-side prediction reconciliation
    uint8_t playerCount;
    NetPlayerState players[MAX_PLAYERS];
};

// ── Hit/Effect Packets ──

struct PlayerHitPacket {
    PacketHeader header;
    uint8_t victimId;
    uint8_t attackerId;
    uint8_t newHealth;
    float hitX, hitY, hitZ;
};

struct PlayerDeathPacket {
    PacketHeader header;
    uint8_t victimId;
    uint8_t killerId;
};

struct BulletEffectPacket {
    PacketHeader header;
    float x, y, z;
    float nx, ny, nz;
};

#pragma pack(pop)

// ── Server Info (for LAN discovery) ──

struct ServerInfo {
    std::string name;
    std::string ip;
    uint16_t port;
    uint8_t playerCount;
    uint8_t maxPlayers;
    float lastSeen;  // Local time when last seen
};

// ── Input State (for local sampling) ──

struct InputState {
    uint8_t keys = 0;
    float yaw = 0.0f;
    float pitch = 0.0f;

    bool HasW() const { return keys & INPUT_W; }
    bool HasS() const { return keys & INPUT_S; }
    bool HasA() const { return keys & INPUT_A; }
    bool HasD() const { return keys & INPUT_D; }
    bool HasJump() const { return keys & INPUT_JUMP; }
    bool HasFire() const { return keys & INPUT_FIRE; }
    bool HasReload() const { return keys & INPUT_RELOAD; }
};

// ── Input Record (for prediction history) ──

struct InputRecord {
    uint32_t sequence;
    InputState input;
    float deltaTime;
};

} // namespace Network

#endif // NETWORK_TYPES_HPP

#ifndef NETWORK_PACKET_HPP
#define NETWORK_PACKET_HPP

#include "Network/NetworkTypes.hpp"
#include <vector>
#include <cstring>
#include <optional>

namespace Network {

// ── Packet Serialization Helpers ────────────────────────────────────────────

class PacketWriter {
public:
    PacketWriter() { m_Buffer.reserve(256); }

    template<typename T>
    void Write(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        size_t offset = m_Buffer.size();
        m_Buffer.resize(offset + sizeof(T));
        std::memcpy(m_Buffer.data() + offset, &value, sizeof(T));
    }

    void WriteBytes(const void* data, size_t size) {
        size_t offset = m_Buffer.size();
        m_Buffer.resize(offset + size);
        std::memcpy(m_Buffer.data() + offset, data, size);
    }

    void WriteString(const char* str, size_t maxLen) {
        size_t len = std::strlen(str);
        if (len > maxLen - 1) len = maxLen - 1;

        size_t offset = m_Buffer.size();
        m_Buffer.resize(offset + maxLen, 0);
        std::memcpy(m_Buffer.data() + offset, str, len);
    }

    const std::vector<uint8_t>& GetBuffer() const { return m_Buffer; }
    const uint8_t* GetData() const { return m_Buffer.data(); }
    size_t GetSize() const { return m_Buffer.size(); }

    void Clear() { m_Buffer.clear(); }

private:
    std::vector<uint8_t> m_Buffer;
};

class PacketReader {
public:
    PacketReader(const uint8_t* data, size_t size)
        : m_Data(data), m_Size(size), m_Offset(0) {}

    PacketReader(const std::vector<uint8_t>& buffer)
        : m_Data(buffer.data()), m_Size(buffer.size()), m_Offset(0) {}

    template<typename T>
    bool Read(T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        if (m_Offset + sizeof(T) > m_Size) return false;

        std::memcpy(&value, m_Data + m_Offset, sizeof(T));
        m_Offset += sizeof(T);
        return true;
    }

    bool ReadBytes(void* dest, size_t size) {
        if (m_Offset + size > m_Size) return false;
        std::memcpy(dest, m_Data + m_Offset, size);
        m_Offset += size;
        return true;
    }

    bool ReadString(char* dest, size_t maxLen) {
        if (m_Offset + maxLen > m_Size) return false;
        std::memcpy(dest, m_Data + m_Offset, maxLen);
        dest[maxLen - 1] = '\0';  // Ensure null-termination
        m_Offset += maxLen;
        return true;
    }

    bool Skip(size_t bytes) {
        if (m_Offset + bytes > m_Size) return false;
        m_Offset += bytes;
        return true;
    }

    size_t GetRemainingSize() const { return m_Size - m_Offset; }
    size_t GetOffset() const { return m_Offset; }
    bool IsEnd() const { return m_Offset >= m_Size; }

    PacketType PeekType() const {
        if (m_Size == 0) return static_cast<PacketType>(0);
        return static_cast<PacketType>(m_Data[0]);
    }

private:
    const uint8_t* m_Data;
    size_t m_Size;
    size_t m_Offset;
};

// ── Packet Building Utilities ───────────────────────────────────────────────

namespace PacketBuilder {

inline std::vector<uint8_t> JoinRequest(const char* playerName) {
    JoinRequestPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::C2S_JOIN_REQUEST);
    std::strncpy(packet.playerName, playerName, sizeof(packet.playerName) - 1);
    packet.playerName[sizeof(packet.playerName) - 1] = '\0';

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> JoinAccepted(uint8_t playerId, uint8_t playerCount) {
    JoinAcceptedPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_JOIN_ACCEPTED);
    packet.playerId = playerId;
    packet.currentPlayerCount = playerCount;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> JoinRejected(uint8_t reason) {
    JoinRejectedPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_JOIN_REJECTED);
    packet.reason = reason;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> PlayerJoined(uint8_t playerId, const char* playerName) {
    PlayerJoinedPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_PLAYER_JOINED);
    packet.playerId = playerId;
    std::strncpy(packet.playerName, playerName, sizeof(packet.playerName) - 1);
    packet.playerName[sizeof(packet.playerName) - 1] = '\0';

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> PlayerLeft(uint8_t playerId) {
    PlayerLeftPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_PLAYER_LEFT);
    packet.playerId = playerId;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> Input(uint32_t sequence, const InputState& input) {
    InputPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::C2S_INPUT);
    packet.sequence = sequence;
    packet.keys = input.keys;
    packet.yaw = input.yaw;
    packet.pitch = input.pitch;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> GameState(uint32_t serverTick, uint32_t lastAckedInput,
                                       const NetPlayerState* players, uint8_t playerCount) {
    GameStatePacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_GAME_STATE);
    packet.serverTick = serverTick;
    packet.lastAckedInput = lastAckedInput;
    packet.playerCount = playerCount;

    for (uint8_t i = 0; i < playerCount && i < MAX_PLAYERS; ++i) {
        packet.players[i] = players[i];
    }

    // Only send the actual number of players
    size_t packetSize = sizeof(PacketHeader) + sizeof(uint32_t) * 2 + sizeof(uint8_t)
                       + sizeof(NetPlayerState) * playerCount;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + packetSize
    );
}

inline std::vector<uint8_t> PlayerHit(uint8_t victimId, uint8_t attackerId,
                                       uint8_t newHealth, const glm::vec3& hitPos) {
    PlayerHitPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_PLAYER_HIT);
    packet.victimId = victimId;
    packet.attackerId = attackerId;
    packet.newHealth = newHealth;
    packet.hitX = hitPos.x;
    packet.hitY = hitPos.y;
    packet.hitZ = hitPos.z;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> PlayerDeath(uint8_t victimId, uint8_t killerId) {
    PlayerDeathPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_PLAYER_DEATH);
    packet.victimId = victimId;
    packet.killerId = killerId;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> BulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    BulletEffectPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::S2C_BULLET_EFFECT);
    packet.x = pos.x;
    packet.y = pos.y;
    packet.z = pos.z;
    packet.nx = normal.x;
    packet.ny = normal.y;
    packet.nz = normal.z;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> ClientBulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    BulletEffectPacket packet{};
    packet.header.type = static_cast<uint8_t>(PacketType::C2S_BULLET_EFFECT);
    packet.x = pos.x;
    packet.y = pos.y;
    packet.z = pos.z;
    packet.nx = normal.x;
    packet.ny = normal.y;
    packet.nz = normal.z;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> DiscoveryQuery() {
    DiscoveryQueryPacket packet{};
    std::memcpy(packet.magic, DISCOVERY_MAGIC, 4);
    packet.type = static_cast<uint8_t>(PacketType::BROADCAST_QUERY);

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

inline std::vector<uint8_t> DiscoveryResponse(const char* gameName,
                                               uint8_t playerCount,
                                               uint8_t maxPlayers,
                                               uint16_t gamePort) {
    DiscoveryResponsePacket packet{};
    std::memcpy(packet.magic, DISCOVERY_MAGIC, 4);
    packet.type = static_cast<uint8_t>(PacketType::BROADCAST_RESPONSE);
    std::strncpy(packet.gameName, gameName, sizeof(packet.gameName) - 1);
    packet.gameName[sizeof(packet.gameName) - 1] = '\0';
    packet.playerCount = playerCount;
    packet.maxPlayers = maxPlayers;
    packet.gamePort = gamePort;

    return std::vector<uint8_t>(
        reinterpret_cast<uint8_t*>(&packet),
        reinterpret_cast<uint8_t*>(&packet) + sizeof(packet)
    );
}

} // namespace PacketBuilder

// ── Packet Parsing Utilities ────────────────────────────────────────────────

namespace PacketParser {

inline std::optional<JoinRequestPacket> ParseJoinRequest(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(JoinRequestPacket)) return std::nullopt;
    JoinRequestPacket packet;
    std::memcpy(&packet, data.data(), sizeof(packet));
    return packet;
}

inline std::optional<JoinAcceptedPacket> ParseJoinAccepted(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(JoinAcceptedPacket)) return std::nullopt;
    JoinAcceptedPacket packet;
    std::memcpy(&packet, data.data(), sizeof(packet));
    return packet;
}

inline std::optional<InputPacket> ParseInput(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(InputPacket)) return std::nullopt;
    InputPacket packet;
    std::memcpy(&packet, data.data(), sizeof(packet));
    return packet;
}

inline std::optional<GameStatePacket> ParseGameState(const std::vector<uint8_t>& data) {
    // Minimum size check
    size_t minSize = sizeof(PacketHeader) + sizeof(uint32_t) * 2 + sizeof(uint8_t);
    if (data.size() < minSize) return std::nullopt;

    GameStatePacket packet{};
    size_t copySize = data.size() < sizeof(packet) ? data.size() : sizeof(packet);
    std::memcpy(&packet, data.data(), copySize);
    return packet;
}

inline std::optional<PlayerHitPacket> ParsePlayerHit(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(PlayerHitPacket)) return std::nullopt;
    PlayerHitPacket packet;
    std::memcpy(&packet, data.data(), sizeof(packet));
    return packet;
}

inline std::optional<BulletEffectPacket> ParseBulletEffect(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(BulletEffectPacket)) return std::nullopt;
    BulletEffectPacket packet;
    std::memcpy(&packet, data.data(), sizeof(packet));
    return packet;
}

inline std::optional<DiscoveryResponsePacket> ParseDiscoveryResponse(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(DiscoveryResponsePacket)) return std::nullopt;

    // Verify magic
    if (std::memcmp(data.data(), DISCOVERY_MAGIC, 4) != 0) return std::nullopt;

    DiscoveryResponsePacket packet;
    std::memcpy(&packet, data.data(), sizeof(packet));
    return packet;
}

} // namespace PacketParser

} // namespace Network

#endif // NETWORK_PACKET_HPP

#ifndef CS_NETWORK_CLIENT_INTERPOLATION_HPP
#define CS_NETWORK_CLIENT_INTERPOLATION_HPP

#include "Network/Types.hpp"
#include <array>
#include <unordered_map>
#include <optional>
#include <cmath>
#include <algorithm>

namespace Network {

// State Snapshot (stores all player states at a point in time)

struct StateSnapshot {
    uint32_t serverTick = 0;
    float localTime = 0.0f;
    std::unordered_map<uint8_t, NetPlayerState> players;
};

// State Buffer (circular buffer for interpolation)

class StateBuffer {
public:
    void PushSnapshot(const StateSnapshot& snapshot);
    void Clear();

    // Get interpolated state for a specific player at renderTime
    std::optional<NetPlayerState> GetInterpolatedState(uint8_t playerId, float renderTime) const;

    // Get the latest state for a player (no interpolation)
    std::optional<NetPlayerState> GetLatestState(uint8_t playerId) const;

    // Get all player IDs currently tracked
    std::vector<uint8_t> GetTrackedPlayerIds() const;

    float GetLatestTime() const;
    size_t GetSnapshotCount() const { return m_Count; }

private:
    static constexpr size_t BUFFER_SIZE = STATE_BUFFER_SIZE;

    std::array<StateSnapshot, BUFFER_SIZE> m_Buffer;
    size_t m_WriteIndex = 0;
    size_t m_Count = 0;

    // Helper to find surrounding snapshots for interpolation
    bool FindBracketingSnapshots(float renderTime,
                                  const StateSnapshot** before,
                                  const StateSnapshot** after) const;
};

// Interpolation Utilities

namespace Interpolation {

// Lerp for angles (handles wraparound)
inline float LerpAngle(float a, float b, float t) {
    float diff = std::fmod(b - a + 540.0f, 360.0f) - 180.0f;
    return a + diff * t;
}

// Cubic hermite interpolation for smoother movement
inline float Hermite(float t) {
    return t * t * (3.0f - 2.0f * t);
}

// Interpolate player state
inline NetPlayerState Lerp(const NetPlayerState& a, const NetPlayerState& b, float t) {
    NetPlayerState result;
    result.playerId = b.playerId;

    // Linear interpolation for position
    result.posX = a.posX + (b.posX - a.posX) * t;
    result.posY = a.posY + (b.posY - a.posY) * t;
    result.posZ = a.posZ + (b.posZ - a.posZ) * t;

    // Angle interpolation for rotation
    result.yaw = LerpAngle(a.yaw, b.yaw, t);
    result.pitch = a.pitch + (b.pitch - a.pitch) * t;

    // Other state from the newer snapshot
    result.velocityY = b.velocityY;
    result.health = b.health;
    result.currentAmmo = b.currentAmmo;
    result.flags = b.flags;

    return result;
}

} // namespace Interpolation

// Implementation

inline void StateBuffer::PushSnapshot(const StateSnapshot& snapshot) {
    m_Buffer[m_WriteIndex] = snapshot;
    m_WriteIndex = (m_WriteIndex + 1) % BUFFER_SIZE;
    if (m_Count < BUFFER_SIZE) {
        ++m_Count;
    }
}

inline void StateBuffer::Clear() {
    m_WriteIndex = 0;
    m_Count = 0;
}

inline float StateBuffer::GetLatestTime() const {
    if (m_Count == 0) return 0.0f;
    size_t latestIdx = (m_WriteIndex + BUFFER_SIZE - 1) % BUFFER_SIZE;
    return m_Buffer[latestIdx].localTime;
}

inline bool StateBuffer::FindBracketingSnapshots(float renderTime,
                                                  const StateSnapshot** before,
                                                  const StateSnapshot** after) const {
    if (m_Count == 0) return false;

    *before = nullptr;
    *after = nullptr;

    // Iterate from newest to oldest
    for (size_t i = 0; i < m_Count; ++i) {
        size_t idx = (m_WriteIndex + BUFFER_SIZE - 1 - i) % BUFFER_SIZE;
        const auto& snap = m_Buffer[idx];

        if (snap.localTime <= renderTime) {
            *before = &snap;

            // Get the snapshot after this one (if exists)
            if (i > 0) {
                size_t afterIdx = (m_WriteIndex + BUFFER_SIZE - i) % BUFFER_SIZE;
                *after = &m_Buffer[afterIdx];
            }
            return true;
        }
    }

    // All snapshots are in the future, use oldest
    size_t oldestIdx = (m_WriteIndex + BUFFER_SIZE - m_Count) % BUFFER_SIZE;
    *before = &m_Buffer[oldestIdx];
    return true;
}

inline std::optional<NetPlayerState> StateBuffer::GetInterpolatedState(uint8_t playerId, float renderTime) const {
    const StateSnapshot* before = nullptr;
    const StateSnapshot* after = nullptr;

    if (!FindBracketingSnapshots(renderTime, &before, &after)) {
        return std::nullopt;
    }

    auto itBefore = before->players.find(playerId);
    if (itBefore == before->players.end()) {
        return std::nullopt;
    }

    // No "after" snapshot, return "before" as-is
    if (!after) {
        return itBefore->second;
    }

    auto itAfter = after->players.find(playerId);
    if (itAfter == after->players.end()) {
        return itBefore->second;
    }

    // Calculate interpolation factor
    float timeDiff = after->localTime - before->localTime;
    if (timeDiff < 0.0001f) {
        return itAfter->second;
    }

    float t = (renderTime - before->localTime) / timeDiff;
    t = std::clamp(t, 0.0f, 1.0f);

    // Apply hermite smoothing
    t = Interpolation::Hermite(t);

    return Interpolation::Lerp(itBefore->second, itAfter->second, t);
}

inline std::optional<NetPlayerState> StateBuffer::GetLatestState(uint8_t playerId) const {
    if (m_Count == 0) return std::nullopt;

    size_t latestIdx = (m_WriteIndex + BUFFER_SIZE - 1) % BUFFER_SIZE;
    auto it = m_Buffer[latestIdx].players.find(playerId);
    if (it == m_Buffer[latestIdx].players.end()) {
        return std::nullopt;
    }
    return it->second;
}

inline std::vector<uint8_t> StateBuffer::GetTrackedPlayerIds() const {
    std::vector<uint8_t> ids;
    if (m_Count == 0) return ids;

    size_t latestIdx = (m_WriteIndex + BUFFER_SIZE - 1) % BUFFER_SIZE;
    for (const auto& [id, state] : m_Buffer[latestIdx].players) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace Network

#endif // CS_NETWORK_CLIENT_INTERPOLATION_HPP

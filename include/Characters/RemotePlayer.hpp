#ifndef CHARACTERS_REMOTE_PLAYER_HPP
#define CHARACTERS_REMOTE_PLAYER_HPP

#include "Characters/CharacterModel.hpp"
#include "Network/NetworkTypes.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace Scene {
class SceneGraph;
}

namespace Characters {

class RemotePlayer {
public:
    RemotePlayer() = default;
    ~RemotePlayer() = default;

    // Initialize with scene and character type
    void Init(Scene::SceneGraph& scene, CharacterType type);

    // Update from network state
    void UpdateFromNetworkState(const Network::NetPlayerState& state, float dt);

    // Set interpolated transform directly
    void SetInterpolatedTransform(const glm::vec3& position, float yaw, float pitch);

    // Getters
    uint8_t GetPlayerId() const { return m_PlayerId; }
    void SetPlayerId(uint8_t id) { m_PlayerId = id; }

    const glm::vec3& GetPosition() const { return m_Position; }
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }
    float GetHealth() const { return m_Health; }
    bool IsAlive() const { return m_IsAlive; }

    // Model control
    void SetVisible(bool visible);
    void SetCharacterType(CharacterType type);

    // Create capsule for hit detection
    struct Capsule {
        glm::vec3 base;
        glm::vec3 tip;
        float radius;
    };
    Capsule MakeCapsule() const;

private:
    uint8_t m_PlayerId = 0xFF;

    // Current visual state
    glm::vec3 m_Position{0.0f};
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    // Target state (for smoothing)
    glm::vec3 m_TargetPosition{0.0f};
    float m_TargetYaw = 0.0f;
    float m_TargetPitch = 0.0f;

    // Game state
    float m_Health = 100.0f;
    bool m_IsAlive = true;
    bool m_IsWalking = false;

    // Character model
    CharacterModel m_Model;
    bool m_ModelInitialized = false;

    // Smoothing factor
    static constexpr float SMOOTH_FACTOR = 0.2f;
    static constexpr float HEIGHT = 1.7f;
    static constexpr float RADIUS = 0.3f;
};

} // namespace Characters

#endif // CHARACTERS_REMOTE_PLAYER_HPP

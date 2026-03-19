#include "Characters/RemotePlayer.hpp"
#include "Scene/SceneGraph.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace Characters {

void RemotePlayer::Init(Scene::SceneGraph& scene, CharacterType type) {
    m_Model.Init(scene, type);
    m_Model.SetVisible(true);
    m_ModelInitialized = true;
}

void RemotePlayer::UpdateFromNetworkState(const Network::NetPlayerState& state, float dt) {
    // Update target values
    m_TargetPosition = state.GetPosition();
    m_TargetYaw = state.yaw;
    m_TargetPitch = state.pitch;

    // Update game state
    m_Health = static_cast<float>(state.health);
    m_IsAlive = state.IsAlive();
    m_IsWalking = state.IsWalking();

    // Smooth interpolation toward target
    float t = 1.0f - std::pow(1.0f - SMOOTH_FACTOR, dt * 60.0f);

    m_Position = glm::mix(m_Position, m_TargetPosition, t);

    // Angle interpolation (handle wraparound)
    float yawDiff = std::fmod(m_TargetYaw - m_Yaw + 540.0f, 360.0f) - 180.0f;
    m_Yaw += yawDiff * t;

    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);

    // Update model
    if (m_ModelInitialized) {
        m_Model.Update(dt, m_Position, m_Yaw, m_IsWalking);
    }
}

void RemotePlayer::SetInterpolatedTransform(const glm::vec3& position, float yaw, float pitch) {
    m_TargetPosition = position;
    m_TargetYaw = yaw;
    m_TargetPitch = pitch;
}

void RemotePlayer::SetVisible(bool visible) {
    if (m_ModelInitialized) {
        m_Model.SetVisible(visible);
    }
}

void RemotePlayer::SetCharacterType(CharacterType type) {
    // This would require re-initialization of the model
    // For now, just note the type change
}

RemotePlayer::Capsule RemotePlayer::MakeCapsule() const {
    Capsule cap;
    cap.radius = RADIUS;
    cap.base = m_Position - glm::vec3(0.0f, HEIGHT - RADIUS, 0.0f);
    cap.tip = m_Position + glm::vec3(0.0f, RADIUS, 0.0f);
    return cap;
}

} // namespace Characters

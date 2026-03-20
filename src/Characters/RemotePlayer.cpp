#include "Characters/RemotePlayer.hpp"
#include "Scene/SceneGraph.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace Characters {

// Gun configuration for remote players
static constexpr glm::vec3 GUN_SCALE{0.025f};
static constexpr glm::vec3 GUN_OFFSET{0.3f, -0.1f, 0.4f};  // right, down, forward from eye

void RemotePlayer::Init(Scene::SceneGraph& scene, CharacterType type) {
    m_Model.Init(scene, type, true);
    m_ModelInitialized = true;

    // Load gun model for remote player
    std::string gunPath = std::string(ASSETS_DIR) + "/guns/aac_honey_badger/scene.gltf";
    m_GunModel = std::make_shared<Core3D::Model>(gunPath, false);
    m_GunNode = std::make_shared<Scene::SceneNode>();
    m_GunNode->SetDrawable(m_GunModel);
    m_GunNode->SetScale(GUN_SCALE);
    m_GunNode->SetVisible(true);
    scene.GetRoot()->AddChild(m_GunNode);
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

    // First state received: snap to position immediately
    if (!m_HasReceivedState) {
        m_Position = m_TargetPosition;
        m_Yaw = m_TargetYaw;
        m_Pitch = m_TargetPitch;
        m_HasReceivedState = true;
    } else {
        // Smooth interpolation toward target
        float t = 1.0f - std::pow(1.0f - SMOOTH_FACTOR, dt * 60.0f);

        m_Position = glm::mix(m_Position, m_TargetPosition, t);

        // Angle interpolation (handle wraparound)
        float yawDiff = std::fmod(m_TargetYaw - m_Yaw + 540.0f, 360.0f) - 180.0f;
        m_Yaw += yawDiff * t;

        m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);
    }

    // Update model
    if (m_ModelInitialized) {
        m_Model.Update(dt, m_Position, m_Yaw, m_IsWalking);

        // Update gun position to follow character
        if (m_GunNode) {
            // Calculate gun position based on character position and facing direction
            float yawRad = glm::radians(m_Yaw);
            float pitchRad = glm::radians(m_Pitch);

            // Direction vectors
            glm::vec3 front;
            front.x = std::cos(pitchRad) * std::sin(yawRad);
            front.y = std::sin(pitchRad);
            front.z = std::cos(pitchRad) * std::cos(yawRad);
            front = glm::normalize(front);

            glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 up = glm::normalize(glm::cross(right, front));

            // Gun position relative to eye height
            glm::vec3 eyePos = m_Position + glm::vec3(0.0f, 0.5f, 0.0f);
            glm::vec3 gunPos = eyePos
                             + right * GUN_OFFSET.x
                             + up * GUN_OFFSET.y
                             + front * GUN_OFFSET.z;

            m_GunNode->SetPosition(gunPos);

            // Gun rotation to match player facing
            glm::mat3 basis(right, up, -front);
            glm::quat gunRot = glm::quat_cast(basis);
            glm::quat leftTurn90 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gunRot = gunRot * leftTurn90;
            m_GunNode->SetRotation(gunRot);
        }
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
    if (m_GunNode) {
        m_GunNode->SetVisible(visible);
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

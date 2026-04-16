#include "Entity/RemotePlayer.hpp"
#include "Scene/SceneGraph.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace Entity {

// Gun model configuration for third-person view (default)
static const std::string GUN_MODEL_PATH = std::string(ASSETS_DIR) + "/weapons/Pistols/m1895/scene.gltf";
static constexpr glm::vec3 GUN_SCALE = glm::vec3(0.12f);
// Offset from character center (right, up, forward in character's local space)
static constexpr glm::vec3 GUN_OFFSET = glm::vec3(0.4f, -0.45f, -0.2f);

void RemotePlayer::Init(Scene::SceneGraph &scene, CharacterType type) {
    // Remove existing gun node before reinitializing
    if (m_GunNode && m_Scene) {
        m_Scene->GetRoot()->RemoveChild(m_GunNode);
        m_GunNode.reset();
        m_GunModel.reset();
    }

    m_Scene = &scene;
    m_Model.Init(scene, type, true);
    m_ModelInitialized = true;

    // Load gun model for third-person view
    m_GunModel = std::make_shared<Core3D::Model>(GUN_MODEL_PATH, false);
    m_GunNode = std::make_shared<Scene::SceneNode>();
    m_GunNode->SetDrawable(m_GunModel);
    m_GunNode->SetScale(m_GunScale);
    m_GunNode->SetVisible(true);
    scene.GetRoot()->AddChild(m_GunNode);
}

void RemotePlayer::UpdateFromNetworkState(const Network::NetPlayerState &state, float dt) {
    // Update target values
    m_TargetPosition = state.GetPosition();
    m_TargetYaw = state.yaw;
    m_TargetPitch = state.pitch;

    // Update game state
    m_Health = static_cast<float>(state.health);
    m_IsAlive = state.IsAlive();
    m_IsWalking = state.IsWalking();
    m_IsCrouching = state.IsCrouching();

    // Smooth interpolation toward target
    float t = 1.0f - std::pow(1.0f - SMOOTH_FACTOR, dt * 60.0f);

    m_Position = glm::mix(m_Position, m_TargetPosition, t);
    glm::vec3 feetPos = m_Position;
    feetPos.y -= STAND_HEIGHT;

    // Angle interpolation (handle wraparound)
    float yawDiff = std::fmod(m_TargetYaw - m_Yaw + 540.0f, 360.0f) - 180.0f;
    m_Yaw += yawDiff * t;

    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);

    // Update model
    if (m_ModelInitialized) {
        m_Model.Update(dt, feetPos, m_Yaw, m_IsWalking);
        UpdateGunTransform();
    }
}

void RemotePlayer::SetInterpolatedTransform(const glm::vec3 &position, float yaw, float pitch) {
    m_TargetPosition = position;
    m_TargetYaw = yaw;
    m_TargetPitch = pitch;
}

void RemotePlayer::Update(float dt) {
    // Smooth interpolation toward target (same logic as UpdateFromNetworkState)
    float t = 1.0f - std::pow(1.0f - SMOOTH_FACTOR, dt * 60.0f);

    m_Position = glm::mix(m_Position, m_TargetPosition, t);
    glm::vec3 feetPos = m_Position;
    feetPos.y -= STAND_HEIGHT;

    // Angle interpolation (handle wraparound)
    float yawDiff = std::fmod(m_TargetYaw - m_Yaw + 540.0f, 360.0f) - 180.0f;
    m_Yaw += yawDiff * t;

    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, t);

    // Update model
    if (m_ModelInitialized) {
        m_Model.Update(dt, feetPos, m_Yaw, m_IsWalking);
        UpdateGunTransform();
    }
}

void RemotePlayer::UpdateGunTransform() {
    if (!m_GunNode) return;

    // Calculate gun position in world space based on character position and rotation
    float yawRad = glm::radians(m_Yaw);

    // Character's local coordinate axes
    glm::vec3 forward(glm::sin(yawRad), 0.0f, -glm::cos(yawRad));
    glm::vec3 right(glm::cos(yawRad), 0.0f, glm::sin(yawRad));
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // Gun follows m_Position directly; no extra crouchSink needed because
    // m_Position.y is already lower when crouching.
    glm::vec3 gunPos = m_Position
                     + right * GUN_OFFSET.x
                     + up * GUN_OFFSET.y
                     + forward * GUN_OFFSET.z;

    m_GunNode->SetPosition(gunPos);

    glm::mat3 cameraBasis(right, up, -forward);
    glm::quat gunRot = glm::quat_cast(cameraBasis);

    glm::quat pitchRotation = glm::angleAxis(glm::radians(-m_Pitch), forward);
    gunRot = pitchRotation * gunRot;

    m_GunNode->SetRotation(gunRot);
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

void RemotePlayer::SetGunModel(const std::string &modelPath, const glm::vec3 &scale) {
    if (!m_Scene) return;

    // Remove old gun node
    if (m_GunNode) {
        m_Scene->GetRoot()->RemoveChild(m_GunNode);
        m_GunNode.reset();
        m_GunModel.reset();
    }

    // Create new gun model
    m_GunModel = std::make_shared<Core3D::Model>(modelPath, false);
    m_GunNode = std::make_shared<Scene::SceneNode>();
    m_GunNode->SetDrawable(m_GunModel);
    m_GunScale = scale;
    m_GunNode->SetScale(m_GunScale);
    m_GunNode->SetVisible(true);
    m_Scene->GetRoot()->AddChild(m_GunNode);
}

Physics::Capsule RemotePlayer::MakeCapsule() const {
    Physics::Capsule cap;
    cap.radius = RADIUS;
    cap.height = HEIGHT - 2.0f * RADIUS;
    if (cap.height < 0.0f) cap.height = 0.0f;
    cap.base = m_Position - glm::vec3(0.0f, HEIGHT - RADIUS, 0.0f);
    return cap;
}

std::shared_ptr<Core3D::Model> RemotePlayer::GetCharacterModelPtr() const {
    if (!m_ModelInitialized) return nullptr;
    return m_Model.GetModel();
}

glm::mat4 RemotePlayer::GetModelWorldTransform() const {
    if (!m_ModelInitialized) return glm::mat4(1.0f);
    auto node = m_Model.GetNode();
    if (!node) return glm::mat4(1.0f);
    return node->GetWorldTransform();
}

bool RemotePlayer::TakeDamage(float damage) {
    if (damage <= 0.0f || !m_IsAlive) return m_IsAlive;
    m_Health = std::max(0.0f, m_Health - damage);
    if (m_Health <= 0.0f) {
        m_IsAlive = false;
    }
    return m_IsAlive;
}

void RemotePlayer::Respawn(const glm::vec3 &spawnPosition) {
    m_Health = 100.0f;
    m_IsAlive = true;
    m_Position = spawnPosition;
    m_TargetPosition = spawnPosition;
}

} // namespace Entity

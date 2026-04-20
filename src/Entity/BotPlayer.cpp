#include "Entity/BotPlayer.hpp"
#include "Weapon/RayCast.hpp"
#include "Weapon/WeaponDefs.hpp"
#include "Util/Logger.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>

namespace Entity {

void BotPlayer::Init(Scene::SceneGraph& scene, CharacterType type,
                     uint8_t botId, const std::string& name) {
    // Clean up previous model if re-initializing
    if (m_GunNode && m_Scene) {
        m_Scene->GetRoot()->RemoveChild(m_GunNode);
        m_GunNode.reset();
        m_GunModel.reset();
    }

    m_Scene = &scene;
    m_BotId = botId;
    m_Name = name;
    m_TeamId = (type == CharacterType::FBI) ? 0 : 1;

    // Use a larger skin width for bots to prevent getting stuck in floor seams
    m_SkinWidth = 0.01f;

    m_CharacterModel.Init(scene, type, true);
    m_ModelInitialized = true;

    // Use the default pistol (M1895) config from the weapon registry
    const auto& registry = Weapon::GetWeaponRegistry();
    std::string gunModelPath;
    glm::vec3 gunScale(0.12f);
    for (const auto& info : registry) {
        if (info.name == "M1895 Revolver") {
            gunModelPath = info.modelPath;
            gunScale = info.modelScale;
            break;
        }
    }
    if (gunModelPath.empty()) {
        gunModelPath = std::string(ASSETS_DIR) + "/weapons/Pistols/m1895/scene.gltf";
    }

    // Third-person gun prop
    m_GunModel = std::make_shared<Core3D::Model>(gunModelPath, false);
    m_GunNode = std::make_shared<Scene::SceneNode>();
    m_GunNode->SetDrawable(m_GunModel);
    m_GunNode->SetScale(gunScale);
    m_GunNode->SetVisible(true);
    scene.GetRoot()->AddChild(m_GunNode);

    // Reset state
    m_CurrentPath.clear();
    m_WaypointIndex = 0;
    m_PathTimer = 0.0f;
    m_Yaw = 0.0f;
    m_Pitch = 0.0f;
    m_TargetYaw = 0.0f;
    m_TargetPitch = 0.0f;
    m_CanSeePlayer = false;
    m_IsWalking = false;

    ResetHealth();

    LOG_INFO("Bot '{}' (id={}) initialized as {}", name, botId,
             type == CharacterType::FBI ? "CT" : "T");
}

void BotPlayer::Update(float dt,
                        const Physics::CollisionMesh& collisionMesh,
                        const Navigation::NavMesh& navMesh,
                        const glm::vec3& playerPos) {
    if (!IsAlive() || !m_ModelInitialized) return;

    // Path recalculation timer
    m_PathTimer -= dt;
    if (m_PathTimer <= 0.0f) {
        RecalculatePath(navMesh, playerPos);
        m_PathTimer = PATH_RECALC_INTERVAL;
    }

    // Move along path
    FollowPath(dt, collisionMesh);

    // Physics (gravity + ground detection)
    UpdatePhysics(dt, collisionMesh);

    // View direction
    UpdateView(dt, playerPos, collisionMesh);

    // Sync model
    UpdateModel(dt);
    UpdateGunTransform();
}

void BotPlayer::Respawn(const glm::vec3& spawnPosition) {
    ResetHealth();
    SetPosition(spawnPosition);
    m_VelocityY = 0.0f;
    m_OnGround = true;
    m_CurrentPath.clear();
    m_WaypointIndex = 0;
    m_PathTimer = 0.0f;
    m_IsWalking = false;

    LOG_INFO("Bot '{}' respawned at ({:.1f}, {:.1f}, {:.1f})",
             m_Name, spawnPosition.x, spawnPosition.y, spawnPosition.z);
}

void BotPlayer::Cleanup(Scene::SceneGraph& scene) {
    if (m_GunNode) {
        scene.GetRoot()->RemoveChild(m_GunNode);
        m_GunNode.reset();
        m_GunModel.reset();
    }
    m_CharacterModel.SetVisible(false);
    m_ModelInitialized = false;
}

void BotPlayer::RecalculatePath(const Navigation::NavMesh& navMesh,
                                const glm::vec3& targetPos) {
    m_CurrentPath = navMesh.FindPath(GetPosition(), targetPos);
    m_WaypointIndex = 0;

    // Skip the first waypoint if it's very close (our current position)
    if (m_CurrentPath.size() > 1) {
        float distToFirst = glm::distance(
            glm::vec2(GetPosition().x, GetPosition().z),
            glm::vec2(m_CurrentPath[0].x, m_CurrentPath[0].z));
        if (distToFirst < WAYPOINT_THRESHOLD) {
            m_WaypointIndex = 1;
        }
    }
}

void BotPlayer::FollowPath(float dt, const Physics::CollisionMesh& mesh) {
    if (m_CurrentPath.empty() || m_WaypointIndex >= m_CurrentPath.size()) {
        m_IsWalking = false;
        return;
    }

    const glm::vec3& target = m_CurrentPath[m_WaypointIndex];
    glm::vec3 myPos = GetPosition();

    // XZ distance to current waypoint
    glm::vec2 toTarget(target.x - myPos.x, target.z - myPos.z);
    float distXZ = glm::length(toTarget);

    if (distXZ < WAYPOINT_THRESHOLD) {
        ++m_WaypointIndex;
        if (m_WaypointIndex >= m_CurrentPath.size()) {
            m_IsWalking = false;
            return;
        }
    }

    // Recompute direction to (potentially new) waypoint
    const glm::vec3& wp = m_CurrentPath[m_WaypointIndex];
    glm::vec3 dir(wp.x - myPos.x, 0.0f, wp.z - myPos.z);
    float len = glm::length(dir);
    if (len < 0.001f) {
        m_IsWalking = false;
        return;
    }
    dir /= len;

    // Move
    float moveDist = BOT_SPEED * dt;
    glm::vec3 desiredPos = myPos + dir * moveDist;
    TryMove(desiredPos, mesh);
    m_IsWalking = true;

    // Set target yaw based on movement direction (for default view)
    if (!m_CanSeePlayer) {
        m_TargetYaw = glm::degrees(std::atan2(dir.x, -dir.z));
    }
}

void BotPlayer::UpdateView(float dt,
                            const glm::vec3& playerPos,
                            const Physics::CollisionMesh& mesh) {
    m_CanSeePlayer = CanSeePlayer(playerPos, mesh);

    if (m_CanSeePlayer) {
        // Look at player
        glm::vec3 eyePos = GetPosition();
        eyePos.y += EYE_HEIGHT_OFFSET;

        glm::vec3 toPlayer = playerPos - eyePos;
        float horizDist = glm::length(glm::vec2(toPlayer.x, toPlayer.z));

        m_TargetYaw = glm::degrees(std::atan2(toPlayer.x, -toPlayer.z));
        m_TargetPitch = glm::degrees(std::atan2(-toPlayer.y, horizDist));
    } else {
        // Face movement direction (TargetYaw set in FollowPath)
        m_TargetPitch = 0.0f;
    }

    // Smooth interpolation
    float lerpFactor = 1.0f - std::exp(-VIEW_LERP_SPEED * dt);

    // Wrap-aware yaw interpolation
    float yawDiff = std::fmod(m_TargetYaw - m_Yaw + 450.0f, 360.0f) - 180.0f;
    m_Yaw += yawDiff * lerpFactor;

    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, lerpFactor);
}

bool BotPlayer::CanSeePlayer(const glm::vec3& playerPos,
                              const Physics::CollisionMesh& mesh) const {
    glm::vec3 eyePos = GetPosition();
    eyePos.y += EYE_HEIGHT_OFFSET;

    glm::vec3 toPlayer = playerPos - eyePos;
    float dist = glm::length(toPlayer);
    if (dist < 0.1f) return true;

    glm::vec3 dir = toPlayer / dist;

    auto wallHit = Weapon::RayCast::Cast(eyePos, dir, mesh, dist);
    // If raycast hits a wall before reaching the player, player is not visible
    return !wallHit.hit || wallHit.distance >= dist - 0.5f;
}

void BotPlayer::UpdateModel(float dt) {
    if (!m_ModelInitialized) return;

    glm::vec3 feetPos = GetPosition();
    feetPos.y -= m_Height;

    m_CharacterModel.Update(dt, feetPos, m_Yaw, m_IsWalking);
}

void BotPlayer::UpdateGunTransform() {
    if (!m_GunNode) return;

    float yawRad = glm::radians(m_Yaw);

    glm::vec3 forward(std::sin(yawRad), 0.0f, -std::cos(yawRad));
    glm::vec3 right(std::cos(yawRad), 0.0f, std::sin(yawRad));
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::vec3 gunPos = GetPosition()
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

void BotPlayer::SetVisible(bool visible) {
    if (m_ModelInitialized) {
        m_CharacterModel.SetVisible(visible);
    }
    if (m_GunNode) {
        m_GunNode->SetVisible(visible);
    }
}

std::shared_ptr<Core3D::Model> BotPlayer::GetCharacterModelPtr() const {
    if (!m_ModelInitialized) return nullptr;
    return m_CharacterModel.GetModel();
}

glm::mat4 BotPlayer::GetModelWorldTransform() const {
    if (!m_ModelInitialized) return glm::mat4(1.0f);
    auto node = m_CharacterModel.GetNode();
    if (!node) return glm::mat4(1.0f);
    return node->GetWorldTransform();
}

} // namespace Entity

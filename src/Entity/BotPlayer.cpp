#include "Entity/BotPlayer.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Weapon/RayCast.hpp"
#include "Weapon/WeaponDefs.hpp"
#include "Util/Logger.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>
#include <random>

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
    m_NeedsNewTarget = true;

    ResetHealth();

    LOG_INFO("Bot '{}' (id={}) initialized as {}", name, botId,
             type == CharacterType::FBI ? "CT" : "T");
}

void BotPlayer::Update(float dt,
                        const Physics::CollisionMesh& collisionMesh,
                        const Navigation::NavMesh& navMesh,
                        const glm::vec3& playerPos) {
    if (!IsAlive() || !m_ModelInitialized) return;
    (void)playerPos; // player tracking disabled

    // Assign a new random walk target if needed
    if (m_NeedsNewTarget) {
        AssignRandomWalkTarget(navMesh);
        if (!m_NeedsNewTarget) { // target was assigned
            RecalculatePath(navMesh, m_WalkTarget);
            m_PathTimer = PATH_RECALC_INTERVAL;
        }
    }

    // Periodically re-run A* toward the same walk target (handles dynamic obstacles)
    m_PathTimer -= dt;
    if (m_PathTimer <= 0.0f) {
        RecalculatePath(navMesh, m_WalkTarget);
        m_PathTimer = PATH_RECALC_INTERVAL;
    }

    // Move along path
    FollowPath(dt, collisionMesh);

    // When the bot reaches its target, request a new one
    if (!m_IsWalking && !m_NeedsNewTarget) {
        m_NeedsNewTarget = true;
    }

    // Physics (gravity + ground detection)
    UpdatePhysics(dt, collisionMesh);

    // View direction (player tracking disabled)
    UpdateView(dt, glm::vec3(0.0f), collisionMesh);

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
    m_NeedsNewTarget = true;

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

void BotPlayer::AssignRandomWalkTarget(const Navigation::NavMesh& navMesh) {
    if (!navMesh.IsBuilt()) return;

    // Determine XZ bounds for this bot's walk mode
    float minX, maxX, minZ, maxZ;
    switch (m_WalkMode) {
        case WalkMode::ZONE_A:
            minX = 6.4f;  maxX = 31.3f;
            minZ = -53.9f; maxZ = -48.7f;
            break;
        case WalkMode::ZONE_B:
            minX = -40.5f; maxX = -28.1f;
            minZ = -56.4f; maxZ = -32.6f;
            break;
        case WalkMode::FULL_MAP:
        default:
            minX = navMesh.GetMinX(); maxX = navMesh.GetMaxX();
            minZ = navMesh.GetMinZ(); maxZ = navMesh.GetMaxZ();
            break;
    }

    // Collect all NavMesh nodes that fall within the zone bounds.
    // Nodes already passed capsule-probe walkability at build time, so this
    // guarantees a valid standing position. Multi-floor nodes at the same XZ
    // are included and a random one will be selected, providing natural
    // floor-layer randomisation.
    const auto& nodes = navMesh.GetNodes();
    std::vector<size_t> candidates;
    candidates.reserve(64);
    for (size_t i = 0; i < nodes.size(); ++i) {
        const glm::vec3& p = nodes[i].position;
        if (p.x >= minX && p.x <= maxX && p.z >= minZ && p.z <= maxZ) {
            candidates.push_back(i);
        }
    }

    if (candidates.empty()) {
        LOG_WARN("Bot '{}': no NavMesh nodes found in walk zone, retrying next frame", m_Name);
        return; // m_NeedsNewTarget stays true → retry next frame
    }

    // Thread-safe static RNG (one per bot would be cleaner but static is fine here)
    static std::mt19937 s_Rng{ std::random_device{}() };
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    m_WalkTarget = nodes[candidates[dist(s_Rng)]].position;
    m_NeedsNewTarget = false;

    LOG_DEBUG("Bot '{}': new walk target ({:.1f}, {:.1f}, {:.1f}), mode={}",
              m_Name, m_WalkTarget.x, m_WalkTarget.y, m_WalkTarget.z,
              static_cast<int>(m_WalkMode));
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

    // Wall-avoidance steering: probe laterally and blend in a push-away force
    // when a wall is detected within WALL_PROBE_DIST of the capsule surface.
    // This prevents the bot from grinding against walls while following its path.
    // A surface is treated as a wall when |normal.y| < 0.7 (not floor/ceiling).
    static constexpr float WALL_PROBE_DIST  = 0.6f;  // lateral scan range
    static constexpr float WALL_AVOID_SCALE = 1.5f;  // blend strength
    static constexpr float WALL_MIN_CLEARANCE = 0.01f; // keeps bot off wall surface
    (void)WALL_MIN_CLEARANCE; // expressed through WALL_PROBE_DIST tuning
    {
        Physics::Capsule avoidCap = MakeCapsule();
        glm::vec3 rightVec(dir.z, 0.0f, -dir.x);  // perpendicular to movement

        auto hitR = Physics::CapsuleCast::SweepCapsule(avoidCap,  rightVec * WALL_PROBE_DIST, mesh);
        auto hitL = Physics::CapsuleCast::SweepCapsule(avoidCap, -rightVec * WALL_PROBE_DIST, mesh);

        glm::vec3 avoidance(0.0f);
        // strength = 1 when touching (t=0), 0 when at probe edge (t=1)
        if (hitR.hit && std::abs(hitR.normal.y) < 0.7f)
            avoidance -= rightVec * (1.0f - hitR.t);
        if (hitL.hit && std::abs(hitL.normal.y) < 0.7f)
            avoidance += rightVec * (1.0f - hitL.t);

        if (glm::length(avoidance) > 0.001f)
            dir = glm::normalize(dir + avoidance * WALL_AVOID_SCALE);
    }

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
                            const glm::vec3& /*playerPos*/,
                            const Physics::CollisionMesh& /*mesh*/) {
    // Player tracking disabled: always face movement direction
    m_CanSeePlayer = false;
    m_TargetPitch = 0.0f;
    // m_TargetYaw is updated by FollowPath() when the bot is moving

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

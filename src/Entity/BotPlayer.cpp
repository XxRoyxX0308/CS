#include "Entity/BotPlayer.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Weapon/Pistols/M1895.hpp"
#include "Weapon/RayCast.hpp"
#include "Weapon/WeaponDefs.hpp"
#include "Util/Logger.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>

namespace Entity {

namespace {

struct WalkBounds {
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

const char* WalkModeToString(WalkMode mode) {
    switch (mode) {
        case WalkMode::ZONE_A:
            return "ZONE_A";
        case WalkMode::ZONE_B:
            return "ZONE_B";
        case WalkMode::FULL_MAP:
        default:
            return "FULL_MAP";
    }
}

WalkBounds GetWalkBounds(WalkMode mode, const Navigation::NavMesh& navMesh) {
    switch (mode) {
        case WalkMode::ZONE_A:
            return {6.4f, 31.3f, -53.9f, -48.7f};
        case WalkMode::ZONE_B:
            return {-40.5f, -28.1f, -56.4f, -32.6f};
        case WalkMode::FULL_MAP:
        default:
            return {navMesh.GetMinX(), navMesh.GetMaxX(),
                    navMesh.GetMinZ(), navMesh.GetMaxZ()};
    }
}

std::vector<size_t> CollectCandidatesInBounds(
    const std::vector<Navigation::NavNode>& nodes,
    const WalkBounds& bounds) {
    std::vector<size_t> result;
    result.reserve(64);

    for (size_t i = 0; i < nodes.size(); ++i) {
        const glm::vec3& position = nodes[i].position;
        if (position.x >= bounds.minX && position.x <= bounds.maxX &&
            position.z >= bounds.minZ && position.z <= bounds.maxZ) {
            result.push_back(i);
        }
    }

    return result;
}

float HashToUnitFloat(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return static_cast<float>(value & 0x00FFFFFFu) /
           static_cast<float>(0x01000000u);
}

float GetJitteredInterval(uint8_t botId,
                          uint32_t salt,
                          float baseInterval,
                          float maxExtraFraction) {
    const float jitter = HashToUnitFloat(static_cast<uint32_t>(botId) * 0x9e3779b9U + salt);
    return baseInterval * (1.0f + jitter * maxExtraFraction);
}

} // namespace

// ============================================================================
//  ResetRuntimeState - Clear transient movement and view state
// ============================================================================
void BotPlayer::ResetRuntimeState() {
    m_CurrentPath.clear();
    m_WaypointIndex = 0;
    m_PathTimer = 0.0f;
    m_ChaseTarget = glm::vec3(0.0f);
    m_ChaseTimer = 0.0f;
    m_ChasePathRefreshTimer = 0.0f;
    m_LastPathGoal = glm::vec3(0.0f);
    m_LastVisibilityCheckTarget = glm::vec3(0.0f);
    m_WalkTarget = glm::vec3(0.0f);
    m_LastWalkTarget = glm::vec3(0.0f);
    m_Yaw = 0.0f;
    m_Pitch = 0.0f;
    m_TargetYaw = 0.0f;
    m_TargetPitch = 0.0f;
    m_CanSeePlayer = false;
    m_IsWalking = false;
    m_BehaviorState = BehaviorState::PATROLLING;
    m_FiredShotThisFrame = false;
    m_HasLastWalkTarget = false;
    m_HasLastPathGoal = false;
    m_HasVisibilityCheck = false;
    m_CachedCanSeePlayer = false;
    m_VisibilityCheckTimer = 0.0f;
    m_StuckRepathCooldownTimer = 0.0f;
    m_NeedsNewTarget = true;
    ResetPathProgressTracking(GetPosition());
}

void BotPlayer::ResetPathProgressTracking(const glm::vec3& position) {
    m_LastProgressPositionXZ = glm::vec2(position.x, position.z);
    m_BestWaypointDistance = std::numeric_limits<float>::max();
    m_StuckTimer = 0.0f;
    m_TrackedWaypointIndex = m_WaypointIndex;
}

glm::vec3 BotPlayer::GetActiveNavigationTarget() const {
    if (m_BehaviorState == BehaviorState::PATROLLING) {
        if (m_WalkTarget != glm::vec3(0.0f)) {
            return m_WalkTarget;
        }
    } else if (m_ChaseTarget != glm::vec3(0.0f)) {
        return m_ChaseTarget;
    }

    if (!m_CurrentPath.empty()) {
        return m_CurrentPath.back();
    }

    return GetPosition();
}

bool BotPlayer::ShouldRefreshPathTo(const glm::vec3& targetPos,
                                    float moveThreshold,
                                    float currentPathGoalSlack) const {
    if (!m_HasLastPathGoal || m_CurrentPath.empty() || m_WaypointIndex >= m_CurrentPath.size()) {
        return true;
    }

    const glm::vec2 targetXZ(targetPos.x, targetPos.z);
    const glm::vec2 lastGoalXZ(m_LastPathGoal.x, m_LastPathGoal.z);
    if (glm::distance(targetXZ, lastGoalXZ) < moveThreshold) {
        return false;
    }

    const glm::vec2 activePathGoalXZ(m_CurrentPath.back().x, m_CurrentPath.back().z);
    return glm::distance(targetXZ, activePathGoalXZ) >= currentPathGoalSlack;
}

// ============================================================================
//  Init - Initialize bot visuals, identity, and runtime state
// ============================================================================
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

    m_Weapon = std::make_unique<Weapon::M1895>();
    m_Weapon->InitRuntimeOnly();

    ResetRuntimeState();

    ResetHealth();

    LOG_INFO("Bot '{}' (id={}) initialized as {}", name, botId,
             type == CharacterType::FBI ? "CT" : "T");
}

// ============================================================================
//  Update - Update navigation, movement, physics, and rendering state
// ============================================================================
void BotPlayer::Update(float dt,
                       const Physics::CollisionMesh& collisionMesh,
                       const Navigation::NavMesh& navMesh,
                       const glm::vec3& playerPos) {
    if (!IsAlive() || !m_ModelInitialized) return;

    m_FiredShotThisFrame = false;
    m_PathTimer = std::max(0.0f, m_PathTimer - dt);
    m_VisibilityCheckTimer = std::max(0.0f, m_VisibilityCheckTimer - dt);
    m_StuckRepathCooldownTimer = std::max(0.0f, m_StuckRepathCooldownTimer - dt);

    if (m_DebugFollowPlayerNoAttack) {
        m_BehaviorState = BehaviorState::CHASING;
        m_CanSeePlayer = false;
        m_ChaseTarget = playerPos;
        m_ChaseTimer = CHASE_DURATION;
        m_ChasePathRefreshTimer -= dt;

        const bool needsPath = m_CurrentPath.empty() || m_WaypointIndex >= m_CurrentPath.size();
        if (needsPath) {
            m_ChasePathRefreshTimer = 0.0f;
        }

        if (m_ChasePathRefreshTimer <= 0.0f &&
            (needsPath || ShouldRefreshPathTo(m_ChaseTarget,
                                              PATH_TARGET_RECALC_DISTANCE,
                                              DEBUG_FOLLOW_PATH_GOAL_SLACK_DISTANCE))) {
            RecalculatePath(navMesh, collisionMesh, m_ChaseTarget);
        }

        if (m_ChasePathRefreshTimer <= 0.0f) {
            m_ChasePathRefreshTimer = GetJitteredInterval(
                m_BotId,
                0x4f1bbcdcU,
                DEBUG_FOLLOW_PATH_RECALC_INTERVAL,
                PATH_REFRESH_INTERVAL_JITTER_FRACTION);
        }

        FollowPath(dt, collisionMesh, navMesh);
    } else {
        const bool canSeePlayer = CanSeePlayer(playerPos, collisionMesh);

        if (canSeePlayer) {
            m_BehaviorState = BehaviorState::ATTACKING;
            m_CanSeePlayer = true;
            m_ChaseTarget = playerPos;
            m_ChaseTimer = CHASE_DURATION;
            m_ChasePathRefreshTimer = 0.0f;
            m_IsWalking = false;
        } else if ((m_BehaviorState == BehaviorState::ATTACKING ||
                    m_BehaviorState == BehaviorState::CHASING) &&
                   m_ChaseTimer > 0.0f) {
            m_ChaseTimer = std::max(0.0f, m_ChaseTimer - dt);
            m_CanSeePlayer = false;

            if (m_ChaseTimer > 0.0f) {
                m_BehaviorState = BehaviorState::CHASING;
                m_ChaseTarget = playerPos;
                m_ChasePathRefreshTimer -= dt;

                if (m_ChasePathRefreshTimer <= 0.0f &&
                    ShouldRefreshPathTo(playerPos,
                                        PATH_TARGET_RECALC_DISTANCE,
                                        CHASE_PATH_GOAL_SLACK_DISTANCE)) {
                    RecalculatePath(navMesh, collisionMesh, m_ChaseTarget);
                }

                if (m_ChasePathRefreshTimer <= 0.0f) {
                    m_ChasePathRefreshTimer = GetJitteredInterval(
                        m_BotId,
                        0x7f4a7c15U,
                        CHASE_PATH_RECALC_INTERVAL,
                        PATH_REFRESH_INTERVAL_JITTER_FRACTION);
                }

                FollowPath(dt, collisionMesh, navMesh);
            } else {
                m_BehaviorState = BehaviorState::PATROLLING;
                m_CurrentPath.clear();
                m_WaypointIndex = 0;
                m_PathTimer = 0.0f;
                m_IsWalking = false;
                m_NeedsNewTarget = true;
            }
        } else {
            m_BehaviorState = BehaviorState::PATROLLING;
            m_CanSeePlayer = false;

            // Assign a new random walk target if needed
            if (m_NeedsNewTarget && m_PathTimer <= 0.0f) {
                AssignRandomWalkTarget(navMesh, collisionMesh);
                if (!m_NeedsNewTarget) { // target was assigned
                    RecalculatePath(navMesh, collisionMesh, m_WalkTarget);
                }
            }

            // Move along path
            FollowPath(dt, collisionMesh, navMesh);

            // Only request a new target after the current one is actually reached.
            const bool pathFinished = m_CurrentPath.empty() || m_WaypointIndex >= m_CurrentPath.size();
            if (!m_NeedsNewTarget && pathFinished) {
                const float distToWalkTarget = glm::distance(
                    glm::vec2(GetPosition().x, GetPosition().z),
                    glm::vec2(m_WalkTarget.x, m_WalkTarget.z));

                if (distToWalkTarget <= WAYPOINT_MAX_THRESHOLD) {
                    m_NeedsNewTarget = true;
                    m_PathTimer = 0.0f;
                } else if (m_PathTimer <= 0.0f) {
                    RecalculatePath(navMesh, collisionMesh, m_WalkTarget);

                    if (m_CurrentPath.empty()) {
                        m_LastWalkTarget = m_WalkTarget;
                        m_HasLastWalkTarget = true;
                        m_NeedsNewTarget = true;
                    }

                    m_PathTimer = GetJitteredInterval(
                        m_BotId,
                        0x62f0a7dbU,
                        PATROL_TARGET_RETRY_INTERVAL,
                        PATROL_TARGET_RETRY_JITTER_FRACTION);
                }
            }
        }
    }

    // Physics (gravity + ground detection)
    UpdatePhysics(dt, collisionMesh);

    // View direction
    UpdateView(dt, playerPos, collisionMesh);

    // Combat (shared weapon spread / cooldown / recoil)
    UpdateCombat(dt, collisionMesh, playerPos);

    // Sync model
    UpdateModel(dt);
    UpdateGunTransform();
}

// ============================================================================
//  Respawn - Reset bot state and place it at a spawn position
// ============================================================================
void BotPlayer::Respawn(const glm::vec3& spawnPosition) {
    ResetHealth();
    SetPosition(spawnPosition);
    m_VelocityY = 0.0f;
    m_OnGround = true;
    ResetRuntimeState();
    if (m_Weapon) {
        m_Weapon->ResetRuntimeState();
    }

    LOG_INFO("Bot '{}' respawned at ({:.1f}, {:.1f}, {:.1f})",
             m_Name, spawnPosition.x, spawnPosition.y, spawnPosition.z);
}

glm::vec3 BotPlayer::GetEyePosition() const {
    return GetPosition() + glm::vec3(0.0f, m_CameraYOffset, 0.0f);
}

bool BotPlayer::ConsumeShotThisFrame() {
    const bool fired = m_FiredShotThisFrame;
    m_FiredShotThisFrame = false;
    return fired;
}

void BotPlayer::SetDebugFollowPlayerNoAttack(bool enabled) {
    m_DebugFollowPlayerNoAttack = enabled;

    if (enabled) {
        m_BehaviorState = BehaviorState::CHASING;
        m_ChaseTimer = CHASE_DURATION;
        return;
    }

    m_BehaviorState = BehaviorState::PATROLLING;
    m_ChaseTimer = 0.0f;
    m_NeedsNewTarget = true;
}

// ============================================================================
//  Cleanup - Remove bot-owned scene resources
// ============================================================================
void BotPlayer::Cleanup(Scene::SceneGraph& scene) {
    if (m_GunNode) {
        scene.GetRoot()->RemoveChild(m_GunNode);
        m_GunNode.reset();
        m_GunModel.reset();
    }
    auto modelNode = m_CharacterModel.GetNode();
    if (modelNode) {
        scene.GetRoot()->RemoveChild(modelNode);
    }
    m_CharacterModel.SetVisible(false);
    m_ModelInitialized = false;
}

// ============================================================================
//  AssignRandomWalkTarget - Pick a reachable target in the current walk mode
// ============================================================================
void BotPlayer::AssignRandomWalkTarget(const Navigation::NavMesh& navMesh,
                                       const Physics::CollisionMesh& collisionMesh) {
    if (!navMesh.IsBuilt()) return;

    static constexpr float MIN_TARGET_XZ_DISTANCE = 2.0f;
    static constexpr float LAST_TARGET_XZ_DISTANCE = 1.5f;

    const auto& nodes = navMesh.GetNodes();
    const WalkBounds walkBounds = GetWalkBounds(m_WalkMode, navMesh);
    std::vector<size_t> candidates = CollectCandidatesInBounds(nodes, walkBounds);

    if (candidates.empty()) {
        LOG_WARN("Bot '{}': no NavMesh nodes found in walk zone {}, retrying next frame",
                 m_Name, WalkModeToString(m_WalkMode));
        return; // m_NeedsNewTarget stays true → retry next frame
    }

    static std::mt19937 s_Rng{ std::random_device{}() };
    std::shuffle(candidates.begin(), candidates.end(), s_Rng);

    const glm::vec2 currentXZ(GetPosition().x, GetPosition().z);
    const glm::vec2 activeTargetXZ(m_WalkTarget.x, m_WalkTarget.z);
    const glm::vec2 lastTargetXZ(m_LastWalkTarget.x, m_LastWalkTarget.z);

    auto tryAssignCandidate = [&](const std::vector<size_t>& candidatePool,
                                  bool avoidCloseTargets,
                                  bool usedFallback) -> bool {
        for (size_t candidateIdx : candidatePool) {
            const glm::vec3& candidatePos = nodes[candidateIdx].position;
            const glm::vec2 candidateXZ(candidatePos.x, candidatePos.z);

            if (m_WalkTarget != glm::vec3(0.0f) &&
                glm::distance(candidateXZ, activeTargetXZ) < LAST_TARGET_XZ_DISTANCE) {
                continue;
            }

            if (m_HasLastWalkTarget &&
                glm::distance(candidateXZ, lastTargetXZ) < LAST_TARGET_XZ_DISTANCE) {
                continue;
            }

            if (avoidCloseTargets) {
                if (glm::distance(candidateXZ, currentXZ) < MIN_TARGET_XZ_DISTANCE) {
                    continue;
                }
            }

            auto path = navMesh.FindPath(GetPosition(), candidatePos, collisionMesh);
            if (path.empty()) {
                continue;
            }

            if (!m_NeedsNewTarget) {
                m_LastWalkTarget = m_WalkTarget;
                m_HasLastWalkTarget = true;
            } else if (m_WalkTarget != glm::vec3(0.0f)) {
                m_LastWalkTarget = m_WalkTarget;
                m_HasLastWalkTarget = true;
            }

            m_WalkTarget = candidatePos;
            m_NeedsNewTarget = false;

            LOG_INFO(
                "Bot '{}': assigned reachable target ({:.1f}, {:.1f}, {:.1f}), mode={}, path_nodes={}, relaxed={}, fallback={}",
                m_Name, m_WalkTarget.x, m_WalkTarget.y, m_WalkTarget.z,
                WalkModeToString(m_WalkMode), path.size(), avoidCloseTargets ? 0 : 1, usedFallback ? 1 : 0);
            return true;
        }

        return false;
    };

    if (tryAssignCandidate(candidates, true, false)) {
        return;
    }

    if (tryAssignCandidate(candidates, false, false)) {
        return;
    }

    const WalkBounds fullMapBounds = GetWalkBounds(WalkMode::FULL_MAP, navMesh);
    std::vector<size_t> fallbackCandidates = CollectCandidatesInBounds(nodes, fullMapBounds);
    std::shuffle(fallbackCandidates.begin(), fallbackCandidates.end(), s_Rng);

    if (tryAssignCandidate(fallbackCandidates, true, true) ||
        tryAssignCandidate(fallbackCandidates, false, true)) {
        LOG_WARN(
            "Bot '{}': {} had {} standable candidates but none were reachable from ({:.1f}, {:.1f}, {:.1f}); using full-map fallback target",
            m_Name, WalkModeToString(m_WalkMode), candidates.size(),
            GetPosition().x, GetPosition().y, GetPosition().z);
        return;
    }

    LOG_WARN("Bot '{}': {} had {} standable candidates but none were reachable from ({:.1f}, {:.1f}, {:.1f})",
             m_Name, WalkModeToString(m_WalkMode), candidates.size(),
             GetPosition().x, GetPosition().y, GetPosition().z);
}

// ============================================================================
//  RecalculatePath - Rebuild path toward the current walk target
// ============================================================================
void BotPlayer::RecalculatePath(const Navigation::NavMesh& navMesh,
                                const Physics::CollisionMesh& collisionMesh,
                                const glm::vec3& targetPos) {
    m_CurrentPath = navMesh.FindPath(GetPosition(), targetPos, collisionMesh);
    m_WaypointIndex = 0;
    m_LastPathGoal = targetPos;
    m_HasLastPathGoal = true;
    ResetPathProgressTracking(GetPosition());

    if (m_CurrentPath.empty()) {
        LOG_WARN("Bot '{}': failed to build path to target ({:.1f}, {:.1f}, {:.1f}) from ({:.1f}, {:.1f}, {:.1f})",
                 m_Name,
                 targetPos.x, targetPos.y, targetPos.z,
                 GetPosition().x, GetPosition().y, GetPosition().z);
        return;
    }

    // Skip the first waypoint if it's very close (our current position)
    if (m_CurrentPath.size() > 1) {
        float distToFirst = glm::distance(
            glm::vec2(GetPosition().x, GetPosition().z),
            glm::vec2(m_CurrentPath[0].x, m_CurrentPath[0].z));
        if (distToFirst < WAYPOINT_THRESHOLD) {
            m_WaypointIndex = 1;
            ResetPathProgressTracking(GetPosition());
        }
    }
}

// ============================================================================
//  UpdateCombat - Update aim camera, spread, and fire control
// ============================================================================
void BotPlayer::UpdateCombat(float dt,
                             const Physics::CollisionMesh& collisionMesh,
                             const glm::vec3& /*playerPos*/) {
    if (!m_Weapon) return;

    SyncAimCamera();
    m_Weapon->Update(dt, m_AimCamera, m_IsWalking, false, m_OnGround);

    if (!m_CanSeePlayer) {
        m_Pitch = m_AimCamera.GetPitch();
        return;
    }

    int ammoBefore = m_Weapon->GetCurrentAmmo();
    m_Weapon->Fire(m_AimCamera, collisionMesh);
    m_FiredShotThisFrame = m_FiredShotThisFrame ||
                           (m_Weapon->GetCurrentAmmo() < ammoBefore);
    m_Pitch = m_AimCamera.GetPitch();
}

// ============================================================================
//  SyncAimCamera - Mirror bot pose to an internal firing camera
// ============================================================================
void BotPlayer::SyncAimCamera() {
    m_AimCamera.SetPosition(GetEyePosition());
    m_AimCamera.SetYaw(m_Yaw);
    m_AimCamera.SetPitch(m_Pitch);
    m_AimCamera.UpdateVectors();
}

// ============================================================================
//  FollowPath - Move toward the current waypoint with wall avoidance
// ============================================================================
void BotPlayer::FollowPath(float dt,
                           const Physics::CollisionMesh& mesh,
                           const Navigation::NavMesh& navMesh) {
    if (m_CurrentPath.empty() || m_WaypointIndex >= m_CurrentPath.size()) {
        m_IsWalking = false;
        return;
    }

    glm::vec3 myPos = GetPosition();

    auto shouldAdvanceWaypoint = [&](size_t waypointIndex) {
        const glm::vec3& waypoint = m_CurrentPath[waypointIndex];
        glm::vec2 toWaypoint(waypoint.x - myPos.x, waypoint.z - myPos.z);
        float waypointDist = glm::length(toWaypoint);

        float reachDistance = std::max(WAYPOINT_THRESHOLD,
                                       GetRadius() * WAYPOINT_RADIUS_SCALE);
        if (waypointIndex + 1 < m_CurrentPath.size()) {
            reachDistance = std::min(WAYPOINT_MAX_THRESHOLD,
                                     reachDistance + WAYPOINT_LOOKAHEAD_BONUS);

            glm::vec2 nextSegment(m_CurrentPath[waypointIndex + 1].x - waypoint.x,
                                  m_CurrentPath[waypointIndex + 1].z - waypoint.z);
            float nextSegmentLen = glm::length(nextSegment);
            if (nextSegmentLen > 0.001f) {
                glm::vec2 nextSegmentDir = nextSegment / nextSegmentLen;
                float passedDistance = glm::dot(
                    glm::vec2(myPos.x - waypoint.x, myPos.z - waypoint.z),
                    nextSegmentDir);
                if (passedDistance > WAYPOINT_PASS_DISTANCE) {
                    return true;
                }
            }
        }

        return waypointDist <= reachDistance;
    };

    while (m_WaypointIndex < m_CurrentPath.size() &&
           shouldAdvanceWaypoint(m_WaypointIndex)) {
        ++m_WaypointIndex;
        ResetPathProgressTracking(myPos);
    }

    if (m_WaypointIndex >= m_CurrentPath.size()) {
        m_IsWalking = false;
        return;
    }

    // Recompute direction to (potentially new) waypoint
    const glm::vec3& wp = m_CurrentPath[m_WaypointIndex];
    glm::vec2 toWaypointXZ(wp.x - myPos.x, wp.z - myPos.z);
    float distXZ = glm::length(toWaypointXZ);
    glm::vec3 dir(wp.x - myPos.x, 0.0f, wp.z - myPos.z);
    float len = glm::length(dir);
    if (len < 0.001f) {
        m_IsWalking = false;
        return;
    }
    dir /= len;

    if (m_WaypointIndex + 1 < m_CurrentPath.size() && distXZ < TURN_LOOKAHEAD_DISTANCE) {
        const glm::vec3& nextWp = m_CurrentPath[m_WaypointIndex + 1];
        glm::vec3 nextDir(nextWp.x - wp.x, 0.0f, nextWp.z - wp.z);
        float nextLen = glm::length(nextDir);
        if (nextLen > 0.001f) {
            nextDir /= nextLen;
            float lookaheadWeight = TURN_LOOKAHEAD_WEIGHT *
                std::clamp(1.0f - (distXZ / TURN_LOOKAHEAD_DISTANCE), 0.0f, 1.0f);
            dir = glm::normalize(glm::mix(dir, nextDir, lookaheadWeight));
        }
    }

    float sidePressureRight = 0.0f;
    float sidePressureLeft = 0.0f;
    float forwardPressure = 0.0f;
    {
        Physics::Capsule avoidCap = MakeCapsule();
        glm::vec3 rightVec(dir.z, 0.0f, -dir.x);  // perpendicular to movement

        auto hitR = Physics::CapsuleCast::SweepCapsule(avoidCap,  rightVec * WALL_PROBE_DIST, mesh);
        auto hitL = Physics::CapsuleCast::SweepCapsule(avoidCap, -rightVec * WALL_PROBE_DIST, mesh);
        auto hitF = Physics::CapsuleCast::SweepCapsule(avoidCap,  dir * FORWARD_PROBE_DIST, mesh);

        if (hitR.hit && std::abs(hitR.normal.y) < 0.7f) {
            sidePressureRight = 1.0f - hitR.t;
        }
        if (hitL.hit && std::abs(hitL.normal.y) < 0.7f) {
            sidePressureLeft = 1.0f - hitL.t;
        }
        if (hitF.hit && std::abs(hitF.normal.y) < 0.7f) {
            forwardPressure = 1.0f - hitF.t;
        }

        const float corridorPressure = std::min(sidePressureLeft, sidePressureRight);
        const float sideBias = sidePressureLeft - sidePressureRight;
        const float wallAvoidScale = glm::mix(WALL_AVOID_MAX_SCALE,
                                              WALL_AVOID_MIN_SCALE,
                                              corridorPressure);

        glm::vec3 steer = dir;
        if (std::abs(sideBias) > 0.001f) {
            steer += rightVec * sideBias * wallAvoidScale;
        }

        if (forwardPressure > 0.001f && std::abs(sideBias) > 0.001f) {
            steer += rightVec * sideBias * (forwardPressure * WALL_FORWARD_ESCAPE_SCALE);
        }

        if (glm::length(steer) > 0.001f) {
            dir = glm::normalize(steer);
        }
    }

    // Move
    float moveDist = BOT_SPEED * dt;
    glm::vec3 desiredPos = myPos + dir * moveDist;
    TryMove(desiredPos, mesh);
    m_IsWalking = true;

    glm::vec3 newPos = GetPosition();
    float newDistXZ = glm::distance(glm::vec2(newPos.x, newPos.z),
                                    glm::vec2(wp.x, wp.z));
    float movedDistXZ = glm::distance(glm::vec2(newPos.x, newPos.z),
                                      glm::vec2(myPos.x, myPos.z));
    bool improvedDistance = newDistXZ + STUCK_PROGRESS_EPSILON < m_BestWaypointDistance;
    bool advancedWaypoint = m_TrackedWaypointIndex != m_WaypointIndex;

    if (advancedWaypoint || improvedDistance) {
        m_BestWaypointDistance = newDistXZ;
        m_LastProgressPositionXZ = glm::vec2(newPos.x, newPos.z);
        m_StuckTimer = 0.0f;
        m_TrackedWaypointIndex = m_WaypointIndex;
    } else if (forwardPressure > 0.0f || std::max(sidePressureLeft, sidePressureRight) > 0.0f) {
        const float minProgressMove = std::max(STUCK_PROGRESS_EPSILON,
                                               moveDist * STUCK_MIN_PROGRESS_MOVE_RATIO);
        const bool barelyMoved = movedDistXZ <= minProgressMove;

        if (barelyMoved && m_StuckRepathCooldownTimer <= 0.0f) {
            m_StuckTimer += dt;
        } else {
            m_StuckTimer = 0.0f;
        }

        if (m_StuckRepathCooldownTimer <= 0.0f &&
            m_StuckTimer >= GetJitteredInterval(m_BotId,
                                                0x9e0d7c41U,
                                                STUCK_REPATH_DELAY,
                                                STUCK_REPATH_DELAY_JITTER_FRACTION)) {
            glm::vec3 activeTarget = GetActiveNavigationTarget();
            LOG_DEBUG(
                "Bot '{}': refreshing path near waypoint {} at ({:.2f}, {:.2f}, {:.2f}); dist={:.2f}, wallL={:.2f}, wallR={:.2f}, wallF={:.2f}",
                m_Name,
                static_cast<int>(m_WaypointIndex),
                newPos.x, newPos.y, newPos.z,
                newDistXZ,
                sidePressureLeft,
                sidePressureRight,
                forwardPressure);
            m_StuckTimer = 0.0f;
            m_StuckRepathCooldownTimer = GetJitteredInterval(
                m_BotId,
                0x51f15e5dU,
                STUCK_REPATH_COOLDOWN,
                STUCK_REPATH_COOLDOWN_JITTER_FRACTION);
            RecalculatePath(navMesh, mesh, activeTarget);
            return;
        }
    } else {
        m_StuckTimer = 0.0f;
    }

    // Set target yaw based on movement direction (for default view)
    if (m_BehaviorState != BehaviorState::ATTACKING) {
        m_TargetYaw = glm::degrees(std::atan2(dir.x, -dir.z));
    }
}

// ============================================================================
//  UpdateView - Smoothly face the player or movement direction
// ============================================================================
void BotPlayer::UpdateView(float dt,
                           const glm::vec3& playerPos,
                           const Physics::CollisionMesh& /*mesh*/) {
    if (m_BehaviorState == BehaviorState::ATTACKING) {
        glm::vec3 eyePos = GetEyePosition();
        glm::vec3 toPlayer = playerPos - eyePos;
        float horizDist = glm::length(glm::vec2(toPlayer.x, toPlayer.z));

        m_TargetYaw = glm::degrees(std::atan2(toPlayer.x, -toPlayer.z));
        m_TargetPitch = glm::degrees(std::atan2(toPlayer.y, horizDist));
    } else {
        m_TargetPitch = 0.0f;
    }

    // Smooth interpolation
    float lerpFactor = 1.0f - std::exp(-VIEW_LERP_SPEED * dt);

    // Wrap-aware yaw interpolation
    float yawDiff = std::fmod(m_TargetYaw - m_Yaw + 450.0f, 360.0f) - 180.0f;
    m_Yaw += yawDiff * lerpFactor;

    m_Pitch = glm::mix(m_Pitch, m_TargetPitch, lerpFactor);
}

// ============================================================================
//  CanSeePlayer - Visibility test (currently unused by active behavior)
// ============================================================================
bool BotPlayer::CanSeePlayer(const glm::vec3& playerPos,
                             const Physics::CollisionMesh& mesh) {
    auto cacheVisibilityResult = [&](bool canSeePlayer) {
        m_LastVisibilityCheckTarget = playerPos;
        m_HasVisibilityCheck = true;

        const float baseInterval = canSeePlayer
            ? VISIBILITY_CACHE_INTERVAL
            : VISIBILITY_CACHE_INTERVAL * VISIBILITY_CACHE_MISS_INTERVAL_SCALE;
        const uint32_t salt = canSeePlayer ? 0x5f356495U : 0x27d4eb2dU;
        m_VisibilityCheckTimer = GetJitteredInterval(
            m_BotId,
            salt,
            baseInterval,
            VISIBILITY_CACHE_JITTER_FRACTION);
        m_CachedCanSeePlayer = canSeePlayer;
        return canSeePlayer;
    };

    if (m_HasVisibilityCheck &&
        m_VisibilityCheckTimer > 0.0f &&
        glm::distance(glm::vec2(playerPos.x, playerPos.z),
                      glm::vec2(m_LastVisibilityCheckTarget.x, m_LastVisibilityCheckTarget.z)) <
            VISIBILITY_CACHE_MOVE_DISTANCE) {
        return m_CachedCanSeePlayer;
    }

    glm::vec3 eyePos = GetEyePosition();

    glm::vec3 toPlayer = playerPos - eyePos;
    float dist = glm::length(toPlayer);
    if (dist < 0.1f) {
        return cacheVisibilityResult(true);
    }

    glm::vec2 toPlayerXZ(toPlayer.x, toPlayer.z);
    float horizLen = glm::length(toPlayerXZ);
    if (horizLen < 0.001f) {
        return cacheVisibilityResult(true);
    }

    float yawRad = glm::radians(m_Yaw);
    glm::vec2 forwardXZ(std::cos(yawRad), std::sin(yawRad));
    glm::vec2 toPlayerDir = toPlayerXZ / horizLen;
    float fovDot = glm::dot(glm::normalize(forwardXZ), toPlayerDir);
    float minDot = std::cos(glm::radians(FIRE_FOV_HALF_ANGLE));
    bool canSeePlayer = false;
    if (fovDot >= minDot) {
        canSeePlayer = HasLineOfSightToPlayer(playerPos, mesh);
    }

    return cacheVisibilityResult(canSeePlayer);
}

bool BotPlayer::HasLineOfSightToPlayer(const glm::vec3& playerPos,
                                       const Physics::CollisionMesh& mesh) const {
    glm::vec3 eyePos = GetEyePosition();
    glm::vec3 toPlayer = playerPos - eyePos;
    float dist = glm::length(toPlayer);
    if (dist < 0.1f) return true;

    glm::vec3 dir = toPlayer / dist;

    auto wallHit = Weapon::RayCast::Cast(eyePos, dir, mesh, dist);
    // If raycast hits a wall before reaching the player, player is not visible
    return !wallHit.hit || wallHit.distance >= dist - 0.5f;
}

// ============================================================================
//  UpdateModel - Sync animated character model
// ============================================================================
void BotPlayer::UpdateModel(float dt) {
    if (!m_ModelInitialized) return;

    glm::vec3 feetPos = GetPosition();
    feetPos.y -= m_Height;

    m_CharacterModel.Update(dt, feetPos, m_Yaw, m_IsWalking);
}

// ============================================================================
//  UpdateGunTransform - Sync third-person gun prop with bot pose
// ============================================================================
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

// ============================================================================
//  SetVisible - Toggle bot renderable state
// ============================================================================
void BotPlayer::SetVisible(bool visible) {
    if (m_ModelInitialized) {
        m_CharacterModel.SetVisible(visible);
    }
    if (m_GunNode) {
        m_GunNode->SetVisible(visible);
    }
}

// ============================================================================
//  GetCharacterModelPtr - Return shared model for hit detection
// ============================================================================
std::shared_ptr<Core3D::Model> BotPlayer::GetCharacterModelPtr() const {
    if (!m_ModelInitialized) return nullptr;
    return m_CharacterModel.GetModel();
}

// ============================================================================
//  GetModelWorldTransform - Return world transform for hit detection
// ============================================================================
glm::mat4 BotPlayer::GetModelWorldTransform() const {
    if (!m_ModelInitialized) return glm::mat4(1.0f);
    auto node = m_CharacterModel.GetNode();
    if (!node) return glm::mat4(1.0f);
    return node->GetWorldTransform();
}

} // namespace Entity

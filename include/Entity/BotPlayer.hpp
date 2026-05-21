#ifndef CS_ENTITY_BOTPLAYER_HPP
#define CS_ENTITY_BOTPLAYER_HPP

#include "Entity/Character.hpp"
#include "Entity/CharacterModel.hpp"
#include "Navigation/NavMesh.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"
#include "Core3D/Model.hpp"
#include "Core3D/Camera.hpp"
#include "Weapon/Weapon.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace Entity {

/**
 * @brief Walk zone assigned to a bot.
 *
 * ZONE_A    — A point area: x∈[6.4,31.3], z∈[-53.9,-48.7]
 * ZONE_B    — B point area: x∈[-40.5,-28.1], z∈[-56.4,-32.6]
 * FULL_MAP  — Entire navigable map.
 */
enum class WalkMode { ZONE_A, ZONE_B, FULL_MAP };

/**
 * @brief AI-controlled bot character.
 *
 * Extends Character for physics/health/capsule movement.
 * Composes a CharacterModel for rendering and a gun model prop.
 *
 * Behavior:
 *   - Is assigned one of three walk modes (A site, B site, full map).
 *   - Picks a random reachable NavMesh node inside the active walk mode.
 *   - Follows the current path and requests a new target after arrival.
 *   - Stops patrol to face and fire at the player after acquiring them.
 *   - Chases the player's current position for a short time after losing sight.
 */
class BotPlayer : public Character {
public:
    BotPlayer() = default;
    ~BotPlayer() override = default;
    BotPlayer(const BotPlayer&) = delete;
    BotPlayer& operator=(const BotPlayer&) = delete;
    BotPlayer(BotPlayer&&) noexcept = default;
    BotPlayer& operator=(BotPlayer&&) noexcept = default;

    /**
     * @brief Initialize the bot with a model and identity.
     * @param scene   The scene graph to add the model to.
     * @param type    FBI (CT) or TERRORIST (T).
     * @param botId   Unique identifier for this bot.
     * @param name    Display name for UI.
     */
    void Init(Scene::SceneGraph& scene, CharacterType type,
              uint8_t botId, const std::string& name);

    /**
     * @brief Main per-frame update.
     *
     * Decrements path timer, recalculates path if needed,
     * moves toward current waypoint, updates physics,
     * updates view direction, and syncs the model.
     */
    void Update(float dt,
                const Physics::CollisionMesh& collisionMesh,
                const Navigation::NavMesh& navMesh,
                const glm::vec3& playerPos);

    /**
     * @brief Respawn the bot at a given position with full health.
     */
    void Respawn(const glm::vec3& spawnPosition);

    /**
     * @brief Remove the bot's model and gun node from the scene.
     */
    void Cleanup(Scene::SceneGraph& scene);

    // ── Walk mode ──
    void SetWalkMode(WalkMode mode) { m_WalkMode = mode; m_NeedsNewTarget = true; }
    WalkMode GetWalkMode() const { return m_WalkMode; }

    // ── Identity ──
    uint8_t GetBotId() const { return m_BotId; }
    uint8_t GetTeamId() const { return m_TeamId; }
    const std::string& GetName() const { return m_Name; }

    // ── View ──
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }
    glm::vec3 GetEyePosition() const;
    bool ConsumeShotThisFrame();
    const Weapon::Weapon* GetGameplayWeapon() const { return m_Weapon.get(); }
    void SetDebugFollowPlayerNoAttack(bool enabled);
    bool IsDebugFollowPlayerNoAttack() const { return m_DebugFollowPlayerNoAttack; }

    // ── Model access (for hit detection) ──
    CharacterType GetCharacterType() const { return m_CharacterModel.GetCharacterType(); }
    const CharacterModel& GetCharacterModelRef() const { return m_CharacterModel; }
    std::shared_ptr<Core3D::Model> GetCharacterModelPtr() const;
    glm::mat4 GetModelWorldTransform() const;
    bool IsModelInitialized() const { return m_ModelInitialized; }

    void SetVisible(bool visible);

private:
    enum class BehaviorState { PATROLLING, ATTACKING, CHASING };

    /** @brief Reset transient navigation/view state. */
    void ResetRuntimeState();

    /** @brief Update bot weapon/aim state and fire if the player is visible. */
    void UpdateCombat(float dt,
                      const Physics::CollisionMesh& collisionMesh,
                      const glm::vec3& playerPos);

    /** @brief Sync the internal aim camera from bot pose. */
    void SyncAimCamera();

    void RecalculatePath(const Navigation::NavMesh& navMesh,
                         const Physics::CollisionMesh& collisionMesh,
                         const glm::vec3& targetPos);

    void FollowPath(float dt, const Physics::CollisionMesh& mesh);

    void UpdateView(float dt,
                    const glm::vec3& playerPos,
                    const Physics::CollisionMesh& mesh);

    bool CanSeePlayer(const glm::vec3& playerPos,
                      const Physics::CollisionMesh& mesh) const;

    bool HasLineOfSightToPlayer(const glm::vec3& playerPos,
                                const Physics::CollisionMesh& mesh) const;

    void UpdateModel(float dt);
    void UpdateGunTransform();

    /**
        * @brief Pick a random reachable node for the current walk mode.
        *
        * The selection prefers targets that are not too close to the bot's
        * current position or its previous target. If a zone has no reachable
        * node from the bot's current connected component, a full-map fallback is
        * used to keep the bot moving instead of stalling in place.
     */
    void AssignRandomWalkTarget(const Navigation::NavMesh& navMesh,
                                const Physics::CollisionMesh& collisionMesh);

    // ── Identity ──
    uint8_t m_BotId = 0;
    uint8_t m_TeamId = 0; // 0 = CT, 1 = T
    std::string m_Name;

    // ── View state ──
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_TargetYaw = 0.0f;
    float m_TargetPitch = 0.0f;
    bool m_CanSeePlayer = false;

    // ── Walk mode / target ──
    WalkMode m_WalkMode = WalkMode::ZONE_A;
    glm::vec3 m_WalkTarget{};
    glm::vec3 m_LastWalkTarget{};
    bool m_HasLastWalkTarget = false;
    bool m_NeedsNewTarget = true;

    // ── Navigation state ──
    std::vector<glm::vec3> m_CurrentPath;
    size_t m_WaypointIndex = 0;
    float m_PathTimer = 0.0f;
    glm::vec3 m_ChaseTarget{};
    float m_ChaseTimer = 0.0f;
    float m_ChasePathRefreshTimer = 0.0f;
    bool m_DebugFollowPlayerNoAttack = false;

    // ── Model ──
    CharacterModel m_CharacterModel;
    bool m_ModelInitialized = false;
    bool m_IsWalking = false;
    BehaviorState m_BehaviorState = BehaviorState::PATROLLING;
    bool m_FiredShotThisFrame = false;
    Scene::SceneGraph* m_Scene = nullptr;

    // ── Combat ──
    std::unique_ptr<Weapon::Weapon> m_Weapon;
    Core3D::Camera m_AimCamera;

    // ── Third-person gun prop ──
    std::shared_ptr<Core3D::Model> m_GunModel;
    std::shared_ptr<Scene::SceneNode> m_GunNode;

    // ── Constants ──
    static constexpr float BOT_SPEED = 3.5f;
    static constexpr float WAYPOINT_THRESHOLD = 1.0f;
    static constexpr float PATH_RECALC_INTERVAL = 10.0f;
    static constexpr float CHASE_DURATION = 6.0f;
    static constexpr float CHASE_PATH_RECALC_INTERVAL = 2.0f;
    static constexpr float DEBUG_FOLLOW_PATH_RECALC_INTERVAL = 0.5f;
    static constexpr float VIEW_LERP_SPEED = 5.0f;
    static constexpr float EYE_HEIGHT_OFFSET = -0.1f;
    static constexpr float FIRE_FOV_HALF_ANGLE = 60.0f;
    static constexpr glm::vec3 GUN_OFFSET{0.4f, -0.45f, -0.2f};
};

} // namespace Entity

#endif // CS_ENTITY_BOTPLAYER_HPP

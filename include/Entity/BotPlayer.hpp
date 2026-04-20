#ifndef CS_ENTITY_BOTPLAYER_HPP
#define CS_ENTITY_BOTPLAYER_HPP

#include "Entity/Character.hpp"
#include "Entity/CharacterModel.hpp"
#include "Navigation/NavMesh.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"
#include "Core3D/Model.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace Entity {

/**
 * @brief AI-controlled bot character.
 *
 * Extends Character for physics/health/capsule movement.
 * Composes a CharacterModel for rendering and a gun model prop.
 *
 * Behavior:
 *   - Every PATH_RECALC_INTERVAL seconds, recalculates A* path
 *     toward the player position via the NavMesh.
 *   - Follows the path by moving toward successive waypoints.
 *   - Faces its movement direction by default.
 *   - If the player is visible (raycast with no wall obstruction),
 *     smoothly rotates to face the player instead.
 */
class BotPlayer : public Character {
public:
    BotPlayer() = default;
    ~BotPlayer() override = default;

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

    // ── Identity ──
    uint8_t GetBotId() const { return m_BotId; }
    uint8_t GetTeamId() const { return m_TeamId; }
    const std::string& GetName() const { return m_Name; }

    // ── View ──
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }

    // ── Model access (for hit detection) ──
    CharacterType GetCharacterType() const { return m_CharacterModel.GetCharacterType(); }
    const CharacterModel& GetCharacterModelRef() const { return m_CharacterModel; }
    std::shared_ptr<Core3D::Model> GetCharacterModelPtr() const;
    glm::mat4 GetModelWorldTransform() const;
    bool IsModelInitialized() const { return m_ModelInitialized; }

    void SetVisible(bool visible);

private:
    void RecalculatePath(const Navigation::NavMesh& navMesh,
                         const glm::vec3& targetPos);

    void FollowPath(float dt, const Physics::CollisionMesh& mesh);

    void UpdateView(float dt,
                    const glm::vec3& playerPos,
                    const Physics::CollisionMesh& mesh);

    bool CanSeePlayer(const glm::vec3& playerPos,
                      const Physics::CollisionMesh& mesh) const;

    void UpdateModel(float dt);
    void UpdateGunTransform();

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

    // ── Navigation state ──
    std::vector<glm::vec3> m_CurrentPath;
    size_t m_WaypointIndex = 0;
    float m_PathTimer = 0.0f;

    // ── Model ──
    CharacterModel m_CharacterModel;
    bool m_ModelInitialized = false;
    bool m_IsWalking = false;
    Scene::SceneGraph* m_Scene = nullptr;

    // ── Third-person gun prop ──
    std::shared_ptr<Core3D::Model> m_GunModel;
    std::shared_ptr<Scene::SceneNode> m_GunNode;

    // ── Constants ──
    static constexpr float BOT_SPEED = 3.5f;
    static constexpr float WAYPOINT_THRESHOLD = 1.0f;
    static constexpr float PATH_RECALC_INTERVAL = 1.0f;
    static constexpr float VIEW_LERP_SPEED = 5.0f;
    static constexpr float EYE_HEIGHT_OFFSET = -0.1f;
    static constexpr glm::vec3 GUN_OFFSET{0.4f, -0.45f, -0.2f};
};

} // namespace Entity

#endif // CS_ENTITY_BOTPLAYER_HPP

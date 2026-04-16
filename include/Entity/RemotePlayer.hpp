#ifndef CS_ENTITY_REMOTE_PLAYER_HPP
#define CS_ENTITY_REMOTE_PLAYER_HPP

#include "Entity/CharacterModel.hpp"
#include "Physics/CollisionTypes.hpp"
#include "Network/Types.hpp"
#include "Scene/SceneNode.hpp"
#include "Core3D/Model.hpp"

#include <glm/glm.hpp>
#include <memory>

namespace Scene { class SceneGraph; }

namespace Entity {

/**
 * @brief Network-synchronized remote player entity.
 * 
 * Represents other players in multiplayer games. Receives state updates
 * from the network and smoothly interpolates between states for
 * fluid visual representation.
 */
class RemotePlayer {
public:
    RemotePlayer() = default;
    ~RemotePlayer() = default;

    /**
     * @brief Initialize with scene and character type.
     * @param scene The scene graph.
     * @param type The character type to use.
     */
    void Init(Scene::SceneGraph &scene, CharacterType type);

    /**
     * @brief Update from network state (Client mode).
     * @param state The network state received from server.
     * @param dt Delta time in seconds.
     */
    void UpdateFromNetworkState(const Network::NetPlayerState &state, float dt);

    /**
     * @brief Update interpolation (Host mode).
     * @param dt Delta time in seconds.
     */
    void Update(float dt);

    /**
     * @brief Set interpolated transform directly.
     * @param position Target position.
     * @param yaw Target yaw angle.
     * @param pitch Target pitch angle.
     */
    void SetInterpolatedTransform(const glm::vec3 &position, float yaw, float pitch);

    // ── Direct Setters (Host Mode) ────────────────────────────────────────

    void SetPosition(const glm::vec3 &position) { m_TargetPosition = position; }
    void SetYaw(float yaw) { m_TargetYaw = yaw; }
    void SetPitch(float pitch) { m_TargetPitch = pitch; }
    void SetWalking(bool walking) { m_IsWalking = walking; }
    void SetCrouching(bool crouching) { m_IsCrouching = crouching; }
    bool IsCrouching() const { return m_IsCrouching; }

    // ── Getters ───────────────────────────────────────────────────────────

    uint8_t GetPlayerId() const { return m_PlayerId; }
    void SetPlayerId(uint8_t id) { m_PlayerId = id; }

    const glm::vec3 &GetPosition() const { return m_Position; }
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }
    CharacterType GetCharacterType() const { return m_Model.GetCharacterType(); }
    float GetHealth() const { return m_Health; }
    void SetHealth(float health) { m_Health = health; m_IsAlive = health > 0.0f; }
    bool IsAlive() const { return m_IsAlive; }

    int GetMoney() const { return m_Money; }
    void SetMoney(int money) { m_Money = money; }
    void SpendMoney(int amount) { m_Money -= amount; }

    // ── Model Control ─────────────────────────────────────────────────────

    void SetVisible(bool visible);
    void SetCharacterType(CharacterType type);
    /** @brief Change the third-person gun model using a GLTF path. */
    void SetGunModel(const std::string &modelPath, const glm::vec3 &scale);
    // ── Collision ─────────────────────────────────────────────────────────

    /** @brief Create capsule for hit detection. */
    Physics::Capsule MakeCapsule() const;

    /** @brief Get model for hit detection raycasting. */
    std::shared_ptr<Core3D::Model> GetCharacterModelPtr() const;
    
    /** @brief Get model world transform for raycasting. */
    glm::mat4 GetModelWorldTransform() const;

    // ── Damage System ─────────────────────────────────────────────────────

    /**
     * @brief Apply damage to the player.
     * @param damage Amount of damage.
     * @return True if still alive.
     */
    bool TakeDamage(float damage);

    /**
     * @brief Respawn the player.
     * @param spawnPosition The spawn position.
     */
    void Respawn(const glm::vec3 &spawnPosition);

private:
    uint8_t m_PlayerId = 0xFF;

    // ── Current Visual State ──────────────────────────────────────────────
    glm::vec3 m_Position{0.0f};
    float m_Height = 1.7f;
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    // ── Target State (for smoothing) ──────────────────────────────────────
    glm::vec3 m_TargetPosition{0.0f};
    float m_TargetYaw = 0.0f;
    float m_TargetPitch = 0.0f;

    // ── Game State ────────────────────────────────────────────────────────
    float m_Health = 100.0f;
    bool m_IsAlive = true;
    bool m_IsWalking = false;
    bool m_IsCrouching = false;
    int m_Money = 5000;

    // ── Character Model ───────────────────────────────────────────────────
    CharacterModel m_Model;
    bool m_ModelInitialized = false;

    // ── Gun Model (third-person view) ─────────────────────────────────────
    std::shared_ptr<Core3D::Model> m_GunModel;
    std::shared_ptr<Scene::SceneNode> m_GunNode;
    glm::vec3 m_GunScale = glm::vec3(0.020f);
    Scene::SceneGraph *m_Scene = nullptr;

    /** @brief Update gun position to match character. */
    void UpdateGunTransform();

    // ── Constants ─────────────────────────────────────────────────────────
    static constexpr float SMOOTH_FACTOR = 0.2f;
    static constexpr float HEIGHT = 1.5f;
    static constexpr float RADIUS = 0.5f;
    static constexpr float STAND_HEIGHT = 1.5f;
    static constexpr float CROUCH_HEIGHT = 1.3f;
};

} // namespace Entity

#endif // CS_ENTITY_REMOTE_PLAYER_HPP

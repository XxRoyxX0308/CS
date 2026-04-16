#ifndef CS_ENTITY_CHARACTER_HPP
#define CS_ENTITY_CHARACTER_HPP

#include "Physics/CollisionTypes.hpp"

#include <glm/glm.hpp>

namespace Physics { class CollisionMesh; }

namespace Entity {

/**
 * @brief Base class for all game characters.
 * 
 * Provides physics simulation (gravity, collision), health system,
 * and capsule-based collision detection. Characters are represented
 * as vertical capsules for movement and collision purposes.
 */
class Character {
public:
    Character() = default;
    virtual ~Character() = default;

    /**
     * @brief Update physics simulation (gravity, ground collision).
     * @param dt Delta time in seconds.
     * @param mesh The collision mesh for ground detection.
     */
    void UpdatePhysics(float dt, const Physics::CollisionMesh &mesh);

    /**
     * @brief Attempt horizontal movement with collision response.
     * 
     * Performs capsule sweep and wall sliding to reach the desired position.
     * 
     * @param desiredPos Target position to move towards.
     * @param mesh The collision mesh for obstacle detection.
     */
    void TryMove(const glm::vec3 &desiredPos, const Physics::CollisionMesh &mesh);

    // ── Position & Dimensions ─────────────────────────────────────────────

    glm::vec3 GetPosition() const { return m_Position; }
    void SetPosition(const glm::vec3 &pos) { m_Position = pos; }

    float GetHeight() const { return m_Height; }
    float GetFeetY() const { return m_Position.y - m_Height; }
    float GetRadius() const { return m_Radius; }

    float GetVelocityY() const { return m_VelocityY; }
    void SetVelocityY(float v) { m_VelocityY = v; }

    bool IsOnGround()    const { return m_OnGround; }
    bool IsCrouching()   const { return m_IsCrouching; }

    /** @brief Make the character jump if on ground. */
    void Jump();

    /**
     * @brief Build a capsule from current position and dimensions.
     * 
     * The capsule represents the character's collision volume:
     * - Base: bottom sphere center (feet + radius)
     * - Height: distance between bottom and top sphere centers
     * 
     * @return Capsule for collision queries.
     */
    Physics::Capsule MakeCapsule() const;

    // ── Health System ─────────────────────────────────────────────────────

    float GetHealth() const { return m_Health; }
    float GetMaxHealth() const { return m_MaxHealth; }
    void SetHealth(float health);
    void SetMaxHealth(float maxHealth) { m_MaxHealth = maxHealth; }

    /**
     * @brief Apply damage to the character.
     * @param damage Amount of damage to apply.
     * @return True if the character is still alive, false if dead.
     */
    bool TakeDamage(float damage);

    /**
     * @brief Heal the character.
     * @param amount Amount of health to restore.
     */
    void Heal(float amount);

    /** @brief Check if the character is alive. */
    bool IsAlive() const { return m_Health > 0.0f; }

    /** @brief Reset health to maximum. */
    void ResetHealth() { m_Health = m_MaxHealth; }

    // ── Economy ─────────────────────────────────────────────────────────

    int GetMoney() const { return m_Money; }
    void SetMoney(int money) { m_Money = money; }
    void SpendMoney(int amount) { m_Money -= amount; }

protected:
    glm::vec3 m_Position = glm::vec3(0.0f);

    float m_StandHeight  = 1.5f;     ///< Standing eye height in meters
    float m_CrouchHeight = 1.3f;     ///< Crouching eye height in meters
    float m_Height = 1.7f;          ///< Current eye height (changes with crouch)
    float m_CameraYOffset = -0.1f;  ///< Camera Y offset from position
    float m_Radius = 0.5f;          ///< Collision radius in meters
    bool  m_IsCrouching = false;    ///< Whether currently crouching
    float m_SkinWidth = 0.002f;     ///< Collision skin thickness
    float m_Gravity = 9.8f;         ///< Gravity acceleration
    float m_VelocityY = 0.0f;       ///< Vertical velocity
    float m_JumpSpeed = 5.0f;       ///< Jump initial velocity
    bool m_OnGround = true;         ///< Whether on ground

    // ── Health ──
    float m_Health = 100.0f;        ///< Current health
    float m_MaxHealth = 100.0f;     ///< Maximum health

    // ── Economy ──
    int m_Money = 50000;             ///< Current money for buy menu
};

} // namespace Entity

#endif // CS_ENTITY_CHARACTER_HPP

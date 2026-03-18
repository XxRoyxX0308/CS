#ifndef CS_CHARACTER_HPP
#define CS_CHARACTER_HPP

#include "Collision/CollisionTypes.hpp"

#include <glm/glm.hpp>

namespace Collision { class CollisionMesh; }

class Character {
public:
    Character() = default;
    virtual ~Character() = default;

    // 每幀更新（重力、地面碰撞）
    void UpdatePhysics(float dt, const Collision::CollisionMesh &mesh);

    // 嘗試水平移動（膠囊體掃掠 + 沿牆滑行）
    void TryMove(const glm::vec3 &desiredPos, const Collision::CollisionMesh &mesh);

    // ── Getters / Setters ──
    glm::vec3 GetPosition() const { return m_Position; }
    void      SetPosition(const glm::vec3 &pos) { m_Position = pos; }

    float GetHeight() const { return m_Height; }
    float GetFeetY()  const { return m_Position.y - m_Height; }
    float GetRadius() const { return m_Radius; }

    float GetVelocityY() const { return m_VelocityY; }
    void  SetVelocityY(float v) { m_VelocityY = v; }

    bool  IsOnGround() const { return m_OnGround; }

    void  Jump();

    // Build a Capsule from current position + dimensions.
    // base = bottom sphere center (feet + radius)
    // height = distance between bottom and top sphere centers
    Collision::Capsule MakeCapsule() const;

    // ── Health System ──
    float GetHealth() const { return m_Health; }
    float GetMaxHealth() const { return m_MaxHealth; }
    void  SetHealth(float health);
    void  SetMaxHealth(float maxHealth) { m_MaxHealth = maxHealth; }

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

    /**
     * @brief Check if the character is alive.
     */
    bool IsAlive() const { return m_Health > 0.0f; }

    /**
     * @brief Reset health to max.
     */
    void ResetHealth() { m_Health = m_MaxHealth; }

protected:
    glm::vec3 m_Position  = glm::vec3(0.0f);

    float m_Height        = 1.7f;   // 眼睛高度（公尺）
    float m_CameraYOffset = -0.1f;  // 攝影機相對 m_Position 的 Y 偏移
    float m_Radius        = 0.5f;   // 碰撞半徑（公尺）
    float m_SkinWidth     = 0.005f; // 皮膚厚度（公尺）
    float m_Gravity       = 9.8f;   // 重力加速度
    float m_VelocityY     = 0.0f;   // 垂直速度
    float m_JumpSpeed     = 5.0f;   // 跳躍初速
    bool  m_OnGround      = true;   // 是否在地面

    // ── Health ──
    float m_Health    = 100.0f;     // 目前血量
    float m_MaxHealth = 100.0f;     // 最大血量
};

#endif // CS_CHARACTER_HPP

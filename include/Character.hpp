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

    float GetVelocityY() const { return m_VelocityY; }
    void  SetVelocityY(float v) { m_VelocityY = v; }

    bool  IsOnGround() const { return m_OnGround; }

    void  Jump();
    
    // Build a Capsule from current position + dimensions.
    // base = bottom sphere center (feet + radius)
    // height = distance between bottom and top sphere centers
    Collision::Capsule MakeCapsule() const;

protected:
    glm::vec3 m_Position  = glm::vec3(0.0f);

    float m_Height    = 1.7f;   // 眼睛高度（公尺）
    float m_Radius    = 0.5f;   // 碰撞半徑（公尺）
    float m_skinWidth = 0.015f; // 皮膚厚度（公尺）
    float m_Gravity   = 9.8f;   // 重力加速度
    float m_VelocityY = 0.0f;   // 垂直速度
    float m_JumpSpeed = 5.0f;   // 跳躍初速
    bool  m_OnGround  = true;   // 是否在地面
};

#endif // CS_CHARACTER_HPP

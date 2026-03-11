#include "Character.hpp"
#include "MapCollider.hpp"

#include <cmath>

// ============================================================================
//  Jump
// ============================================================================
void Character::Jump() {
    if (m_OnGround) {
        m_VelocityY = m_JumpSpeed;
        m_OnGround = false;
    }
}

// ============================================================================
//  TryMove — 水平移動 + 牆壁碰撞 + 沿牆滑行
// ============================================================================
void Character::TryMove(const glm::vec3 &desiredPos, const MapCollider &collider) {
    glm::vec3 newPos = desiredPos;
    newPos.y = m_Position.y; // 鎖定 Y 軸

    float wallFeetY = m_Position.y - m_Height;
    if (IsBlockedByWall(newPos.x, newPos.z, wallFeetY, collider)) {
        // 嘗試只走 X 軸（沿 Z 牆面滑行）
        if (!IsBlockedByWall(newPos.x, m_Position.z, wallFeetY, collider)) {
            newPos.z = m_Position.z;
        }
        // 嘗試只走 Z 軸（沿 X 牆面滑行）
        else if (!IsBlockedByWall(m_Position.x, newPos.z, wallFeetY, collider)) {
            newPos.x = m_Position.x;
        }
        // 兩軸都被擋 → 完全不動
        else {
            newPos = m_Position;
        }
    }

    m_Position = newPos;
}

// ============================================================================
//  UpdatePhysics — 重力、地面碰撞
// ============================================================================
void Character::UpdatePhysics(float dt, const MapCollider &collider) {
    m_VelocityY -= m_Gravity * dt;
    float playerY = m_Position.y + m_VelocityY * dt;

    float feetY = playerY - m_Height;
    float groundY = collider.GetGroundHeight(m_Position.x, m_Position.z,
                                             feetY + m_MaxStepHeight);

    if (groundY > -9000.0f && playerY <= groundY + m_Height) {
        playerY = groundY + m_Height;
        m_VelocityY = 0.0f;
        m_OnGround = true;
    } else if (groundY <= -9000.0f) {
        if (playerY <= m_Height) {
            playerY = m_Height;
            m_VelocityY = 0.0f;
            m_OnGround = true;
        }
    }

    m_Position.y = playerY;
}

// ============================================================================
//  IsBlockedByWall — 地面高度 + 水平牆壁射線碰撞
// ============================================================================
bool Character::IsBlockedByWall(float x, float z, float feetY,
                                const MapCollider &collider) const {
    constexpr int   N       = 20;
    constexpr float TWO_PI  = 6.28318530f;
    const float wallThreshold = feetY + m_MaxStepHeight;
    const float bodyTop       = feetY + m_Height;
    float cx = m_Position.x;
    float cz = m_Position.z;

    for (int i = 0; i < N; ++i) {
        float angle = (TWO_PI * i) / static_cast<float>(N);
        float sx = x + m_Radius * std::cos(angle);
        float sz = z + m_Radius * std::sin(angle);

        // 1. 原有地面高度檢測（箱子、台階等可行走面太高）
        float wallY = collider.GetGroundHeight(sx, sz, bodyTop);
        if (wallY > wallThreshold) {
            return true;
        }

        // 2. 水平射線檢測（垂直牆壁）
        if (collider.IsWallBlocking(cx, cz, sx, sz, feetY, bodyTop)) {
            return true;
        }
    }
    return false;
}

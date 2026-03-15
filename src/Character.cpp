#include "Character.hpp"
#include "Collision/CollisionMesh.hpp"
#include "Collision/CapsuleCast.hpp"

// ============================================================================
//  MakeCapsule — build a Capsule from current position + dimensions
// ============================================================================
Collision::Capsule Character::MakeCapsule() const {
    // m_Position = eye position (top of the character)
    // Feet Y    = m_Position.y - m_Height
    // Capsule base (bottom sphere center) = feetY + radius
    // Capsule top (top sphere center)     = eyeY - radius (approx)
    // Capsule height = distance between sphere centers
    float feetY = m_Position.y - m_Height;

    Collision::Capsule cap;
    cap.radius = m_Radius;
    cap.base   = glm::vec3(m_Position.x, feetY + m_Radius, m_Position.z);
    cap.height = m_Height - 2.0f * m_Radius;
    if (cap.height < 0.0f) cap.height = 0.0f; // degenerate: sphere only
    return cap;
}

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
//  TryMove — 膠囊體掃掠 + 沿牆滑行 + 階梯攀升 (Capsule Sweep + Slide + Step-Up)
// ============================================================================
void Character::TryMove(const glm::vec3 &desiredPos,
                        const Collision::CollisionMesh &mesh) {
    // Horizontal-only velocity (lock Y)
    glm::vec3 velocity(desiredPos.x - m_Position.x,
                       0.0f,
                       desiredPos.z - m_Position.z);
    
    if (glm::dot(velocity, velocity) < 1e-8f) return;

    Collision::Capsule cap = MakeCapsule();
    glm::vec3 newBase = Collision::CapsuleCast::MoveAndSlide(cap, velocity, mesh, m_skinWidth);
    
    m_Position.x = newBase.x;
    m_Position.y = newBase.y - m_Radius + m_Height;
    m_Position.z = newBase.z;
}

// ============================================================================
//  UpdatePhysics — 重力 + 膠囊體垂直掃掠 (Gravity + Capsule Vertical Sweep)
// ============================================================================
void Character::UpdatePhysics(float dt, const Collision::CollisionMesh &mesh) {
    // Apply gravity
    m_VelocityY -= m_Gravity * dt;
    float deltaY = m_VelocityY * dt;
    float detectY = deltaY > 0 ? std::max(deltaY, 0.1f) : std::min(deltaY, -0.1f); 

    Collision::Capsule cap = MakeCapsule();
    auto hitY = Collision::CapsuleCast::SweepVertical(cap, mesh, detectY);

    m_Position.y += deltaY;
    float feetY = m_Position.y - m_Height;

    if (hitY.has_value()) {
        float hY = hitY.value();

        if (deltaY <= 0.0f) {
            // Falling / on ground — snap feet to ground
            if (feetY < hY + m_skinWidth + 0.05f) {
                m_Position.y = hY + m_skinWidth + m_Height;
                m_VelocityY = 0.0f;
                m_OnGround = true;
                return;
            }
        } else {
            // Rising — hit ceiling, clamp head
            float headHitY = hY + m_Height;
            if (m_Position.y > headHitY - m_skinWidth) {
                m_Position.y = headHitY - m_skinWidth;
                m_VelocityY = 0.0f;
            }
        }
    }

    // Absolute floor fallback (fell off the map)
    if (m_Position.y - m_Height < -50.0f) {
        m_Position.y = m_Height;
        m_VelocityY = 0.0f;
        m_OnGround = true;
    } else {
        m_OnGround = false;
    }
}

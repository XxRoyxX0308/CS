#include "Entity/Character.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Physics/CapsuleCast.hpp"

#include <algorithm>

namespace Entity {

// ============================================================================
//  MakeCapsule - Build a Capsule from current position and dimensions
// ============================================================================
Physics::Capsule Character::MakeCapsule() const {
    // m_Position = eye position (top of the character)
    // Feet Y = m_Position.y - m_Height
    // Capsule base (bottom sphere center) = feetY + radius
    // Capsule top (top sphere center) = eyeY - radius
    // Capsule height = distance between sphere centers
    float feetY = m_Position.y - m_Height;

    Physics::Capsule cap;
    cap.radius = m_Radius;
    cap.base = glm::vec3(m_Position.x, feetY + m_Radius, m_Position.z);
    cap.height = m_Height - 2.0f * m_Radius;
    if (cap.height < 0.0f) cap.height = 0.0f; // Degenerate: sphere only
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
//  Health System
// ============================================================================
void Character::SetHealth(float health) {
    m_Health = std::clamp(health, 0.0f, m_MaxHealth);
}

bool Character::TakeDamage(float damage) {
    if (damage <= 0.0f || !IsAlive()) return IsAlive();

    m_Health = std::max(0.0f, m_Health - damage);
    return IsAlive();
}

void Character::Heal(float amount) {
    if (amount <= 0.0f) return;

    m_Health = std::min(m_MaxHealth, m_Health + amount);
}

// ============================================================================
//  TryMove - Capsule sweep + wall sliding
// ============================================================================
void Character::TryMove(const glm::vec3 &desiredPos,
                        const Physics::CollisionMesh &mesh) {
    // Horizontal-only velocity (lock Y)
    glm::vec3 velocity(desiredPos.x - m_Position.x,
                       0.0f,
                       desiredPos.z - m_Position.z);
    
    if (glm::dot(velocity, velocity) < 1e-8f) return;

    Physics::Capsule cap = MakeCapsule();
    glm::vec3 newBase = Physics::CapsuleCast::MoveAndSlide(cap, velocity, mesh, m_SkinWidth);
    
    m_Position.x = newBase.x;
    m_Position.y = newBase.y - m_Radius + m_Height;
    m_Position.z = newBase.z;
}

// ============================================================================
//  UpdatePhysics - Gravity + capsule vertical sweep
// ============================================================================
void Character::UpdatePhysics(float dt, const Physics::CollisionMesh &mesh) {
    // Apply gravity
    m_VelocityY -= m_Gravity * dt;
    float deltaY = m_VelocityY * dt;
    float detectY = deltaY > 0 ? std::max(deltaY, 0.1f) : std::min(deltaY, -0.1f); 

    Physics::Capsule cap = MakeCapsule();
    auto hitY = Physics::CapsuleCast::SweepVertical(cap, mesh, detectY);

    m_Position.y += deltaY;
    float feetY = m_Position.y - m_Height;

    if (hitY.has_value()) {
        float hY = hitY.value();

        if (deltaY <= 0.0f) {
            // Falling or on ground - snap feet to ground
            if (feetY < hY + m_SkinWidth + 0.05f) {
                m_Position.y = hY + m_SkinWidth + m_Height;
                m_VelocityY = 0.0f;
                m_OnGround = true;
                return;
            }
        } else {
            // Rising - hit ceiling, clamp head
            float headHitY = hY + m_Height;
            if (m_Position.y > headHitY - m_SkinWidth) {
                m_Position.y = headHitY - m_SkinWidth;
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

} // namespace Entity

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
    glm::vec3 newBase = Collision::CapsuleCast::MoveAndSlide(cap, velocity, mesh);

    // ── Step-up: if mostly blocked and on ground, try up → forward → down ──
    // float movedX = newBase.x - cap.base.x;
    // float movedZ = newBase.z - cap.base.z;
    // float movedDist2 = movedX * movedX + movedZ * movedZ;
    // float wantedDist2 = velocity.x * velocity.x + velocity.z * velocity.z;

    // if (m_OnGround && movedDist2 < wantedDist2 * 0.25f) {
    //     // 1. Sweep capsule upward (check for ceiling)
    //     glm::vec3 upVel(0.0f, m_MaxStepHeight, 0.0f);
    //     auto upResult = Collision::CapsuleCast::SweepCapsule(cap, upVel, mesh);
    //     float lift = upResult.hit
    //         ? std::max(0.0f, m_MaxStepHeight * upResult.t - 1e-4f)
    //         : m_MaxStepHeight;

    //     if (lift > 0.01f) {
    //         // 2. Move horizontally at raised height
    //         Collision::Capsule raisedCap = cap;
    //         raisedCap.base.y += lift;
    //         glm::vec3 stepBase = Collision::CapsuleCast::MoveAndSlide(
    //             raisedCap, velocity, mesh);

    //         // 3. Drop back down to find ground
    //         Collision::Capsule dropCap = raisedCap;
    //         dropCap.base = stepBase;
    //         auto groundY = Collision::CapsuleCast::SnapToGround(
    //             dropCap, mesh, lift + 0.1f);

    //         if (groundY.has_value()) {
    //             float stepMovedX = stepBase.x - cap.base.x;
    //             float stepMovedZ = stepBase.z - cap.base.z;
    //             float stepDist2 = stepMovedX * stepMovedX + stepMovedZ * stepMovedZ;

    //             // Accept step-up only if it moved further
    //             if (stepDist2 > movedDist2) {
    //                 m_Position.x = stepBase.x;
    //                 m_Position.z = stepBase.z;
    //                 m_Position.y = groundY.value() + m_Height;
    //                 return;
    //             }
    //         }
    //     }
    // }

    // Update position (keep current Y, only change XZ from sweep result)
    m_Position.x = newBase.x;
    m_Position.z = newBase.z;
}

// ============================================================================
//  UpdatePhysics — 重力 + 膠囊體地面掃掠 (Gravity + Capsule Ground Sweep)
// ============================================================================
void Character::UpdatePhysics(float dt, const Collision::CollisionMesh &mesh) {
    // ── Apply gravity ──
    m_VelocityY -= m_Gravity * dt;
    m_Position.y += m_VelocityY * dt;

    // ── Ground detection ──
    // SnapToGround internally lifts the probe so it works even when
    // gravity has pushed the capsule slightly below the surface.
    Collision::Capsule cap = MakeCapsule();
    float sweepRange = 0.5f;
    auto groundY = Collision::CapsuleCast::SnapToGround(cap, mesh, sweepRange);

    if (groundY.has_value()) {
        float gY = groundY.value();
        float feetY = m_Position.y - m_Height;

        // Accept ground if falling/standing and feet are within step range
        if (m_VelocityY <= 0.0f && feetY <= gY + 0.1f) {
            m_Position.y = gY + m_Height;
            m_VelocityY = 0.0f;
            m_OnGround = true;
            return;
        }
    }

    // Absolute floor fallback (fell off the map)
    if (m_Position.y - m_Height < -100.0f) {
        m_Position.y = m_Height;
        m_VelocityY = 0.0f;
        m_OnGround = true;
    } else {
        m_OnGround = false;
    }
}

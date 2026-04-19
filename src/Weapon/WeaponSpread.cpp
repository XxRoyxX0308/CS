#include "Weapon/WeaponSpread.hpp"

#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace Weapon {

// ============================================================================
//  Configure — set all spread parameters
// ============================================================================
void WeaponSpread::Configure(float minSpread, float maxSpread,
                             float moveRate, float fireIncrement,
                             float jumpPenalty, float recoveryRate,
                             float crouchMult) {
    m_MinSpread               = minSpread;
    m_MaxSpread               = maxSpread;
    m_MoveSpreadRate          = moveRate;
    m_FireSpreadIncrement     = fireIncrement;
    m_JumpPenalty             = jumpPenalty;
    m_RecoveryRate            = recoveryRate;
    m_CrouchRecoveryMultiplier = crouchMult;
}

// ============================================================================
//  Reset — snap current spread to minimum
// ============================================================================
void WeaponSpread::Reset() {
    m_CurrentSpread = m_MinSpread;
}

// ============================================================================
//  Update — per-frame spread growth / recovery
// ============================================================================
void WeaponSpread::Update(float dt, bool isMoving, bool isCrouching,
                          bool isOnGround) {
    if (!isOnGround) {
        // In air — no recovery, spread stays elevated
        return;
    }

    if (isMoving) {
        // Moving on ground — spread increases
        m_CurrentSpread += m_MoveSpreadRate * dt;
    } else {
        // Standing still on ground — recover toward min
        float recovery = m_RecoveryRate * dt;
        if (isCrouching) {
            recovery *= m_CrouchRecoveryMultiplier;
        }
        m_CurrentSpread -= recovery;
    }

    m_CurrentSpread = std::clamp(m_CurrentSpread, m_MinSpread, m_MaxSpread);
}

// ============================================================================
//  OnFire — fixed spread addition per shot
// ============================================================================
void WeaponSpread::OnFire() {
    m_CurrentSpread += m_FireSpreadIncrement;
    m_CurrentSpread = std::min(m_CurrentSpread, m_MaxSpread);
}

// ============================================================================
//  OnJump — instant penalty when leaving the ground
// ============================================================================
void WeaponSpread::OnJump() {
    m_CurrentSpread += m_JumpPenalty;
    m_CurrentSpread = std::min(m_CurrentSpread, m_MaxSpread);
}

// ============================================================================
//  ApplySpread — deviate direction randomly within current spread cone
// ============================================================================
glm::vec3 WeaponSpread::ApplySpread(const glm::vec3 &direction) const {
    if (m_CurrentSpread <= 0.0f) {
        return direction;
    }

    // Thread-local RNG for performance
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distAngle(0.0f, glm::two_pi<float>());
    // Uniform distribution within the cone (sqrt for uniform area sampling)
    std::uniform_real_distribution<float> distRadius(0.0f, 1.0f);

    float halfAngle = glm::radians(m_CurrentSpread);
    float r = std::sqrt(distRadius(rng)) * std::tan(halfAngle);
    float theta = distAngle(rng);

    // Build a local coordinate frame around the direction vector
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 up = (std::abs(dir.y) < 0.999f)
                       ? glm::vec3(0.0f, 1.0f, 0.0f)
                       : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    glm::vec3 trueUp = glm::cross(right, dir);

    // Offset the direction within the cone
    glm::vec3 deviated = dir + right * (r * std::cos(theta))
                             + trueUp * (r * std::sin(theta));
    return glm::normalize(deviated);
}

} // namespace Weapon

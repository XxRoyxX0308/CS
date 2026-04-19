#ifndef CS_WEAPON_WEAPONSPREAD_HPP
#define CS_WEAPON_WEAPONSPREAD_HPP

#include <glm/glm.hpp>

namespace Weapon {

/**
 * @brief Computes and maintains dynamic bullet spread based on player state.
 *
 * Spread increases when moving or shooting, decreases when idle.
 * Crouching accelerates recovery; jumping adds an instant penalty.
 * Call ApplySpread() to get a randomly deviated direction within the
 * current spread cone.
 */
class WeaponSpread {
public:
    WeaponSpread() = default;

    /**
     * @brief Set all spread parameters.
     * @param minSpread     Minimum spread in degrees (resting accuracy).
     * @param maxSpread     Maximum spread cap in degrees.
     * @param moveRate      Spread increase per second while moving.
     * @param fireIncrement Fixed spread added per shot.
     * @param jumpPenalty   Instant spread penalty on leaving the ground.
     * @param recoveryRate  Spread recovery degrees/sec while standing still.
     * @param crouchMult    Multiplier for recovery speed while crouching.
     */
    void Configure(float minSpread, float maxSpread,
                   float moveRate, float fireIncrement,
                   float jumpPenalty, float recoveryRate,
                   float crouchMult);

    /** @brief Reset current spread to minimum. */
    void Reset();

    /** @brief Per-frame update: grow or recover spread based on player state. */
    void Update(float dt, bool isMoving, bool isCrouching, bool isOnGround);

    /** @brief Call when a shot is fired — adds fire spread increment. */
    void OnFire();

    /** @brief Call when the player leaves the ground — adds jump penalty. */
    void OnJump();

    /** @brief Return a direction randomly deviated within the current spread cone. */
    glm::vec3 ApplySpread(const glm::vec3 &direction) const;

    // ── Getters ──
    float GetCurrentSpread() const { return m_CurrentSpread; }
    float GetMinSpread()     const { return m_MinSpread; }
    float GetMaxSpread()     const { return m_MaxSpread; }

private:
    float m_MinSpread     = 0.0f;
    float m_MaxSpread     = 0.0f;
    float m_CurrentSpread = 0.0f;

    float m_MoveSpreadRate        = 0.0f;  // deg/sec increase while moving
    float m_FireSpreadIncrement   = 0.0f;  // deg added per shot
    float m_JumpPenalty           = 0.0f;  // deg added on jump
    float m_RecoveryRate          = 0.0f;  // deg/sec recovery
    float m_CrouchRecoveryMultiplier = 1.0f;
};

} // namespace Weapon

#endif // CS_WEAPON_WEAPONSPREAD_HPP

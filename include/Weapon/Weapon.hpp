#ifndef CS_WEAPON_WEAPON_HPP
#define CS_WEAPON_WEAPON_HPP

#include "pch.hpp"

#include "Core3D/Camera.hpp"
#include "Core3D/Model.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Weapon/RayCast.hpp"
#include "Weapon/WeaponSpread.hpp"

#include <memory>
#include <string>

namespace Weapon {

/**
 * @brief Abstract base class for all weapons.
 *
 * Subclasses override Configure() to set weapon-specific parameters
 * (fire rate, recoil, mag size, model path, scale, etc.).
 *
 * Usage:
 * @code
 * auto weapon = std::make_unique<AssaultRifle>();
 * weapon->Init(scene);
 * // each frame:
 * weapon->Update(dt, camera, mesh);
 * weapon->SyncToCamera(camera);
 * @endcode
 */
class Weapon {
public:
    virtual ~Weapon() = default;

    /** @brief Load the weapon model and attach its node to the scene. */
    void Init(Scene::SceneGraph &scene);

    /** @brief Remove the weapon node from the scene. */
    void Cleanup(Scene::SceneGraph &scene);

    /** @brief Per-frame update: cooldowns, reload timer, reload animation, recoil recovery, spread. */
    void Update(float dt, Core3D::Camera &camera,
               bool isMoving, bool isCrouching, bool isOnGround);

    /** @brief Fire the weapon: consume ammo, apply recoil, raycast for hit. */
    void Fire(Core3D::Camera &camera, const Physics::CollisionMesh &mesh);

    /** @brief Start reloading if not already reloading and magazine not full. */
    void StartReload();

    /** @brief Position/rotate the weapon node to follow the camera. */
    void SyncToCamera(const Core3D::Camera &camera);

    // ── Getters ──
    int  GetCurrentAmmo() const { return m_CurrentAmmo; }
    int  GetMagSize()     const { return m_MagSize; }
    bool IsReloading()    const { return m_IsReloading; }
    float GetDamage()     const { return m_Damage; }
    int  GetPrice()       const { return m_Price; }
    const std::string& GetModelPath() const { return m_ModelPath; }
    const glm::vec3& GetWeaponScale() const { return m_WeaponScale; }

    /** @brief Get the last fire ray hit result (for debug/effects). */
    const RayHitResult &GetLastHit() const { return m_LastHit; }

    /** @brief Get the actual direction the last bullet traveled (spread-applied, pre-recoil). */
    const glm::vec3 &GetLastFireDir() const { return m_LastFireDir; }

    /** @brief Get the weapon spread system (for UI visualization). */
    const WeaponSpread &GetSpread() const { return m_Spread; }

    /** @brief Mutable access to spread (e.g. for jump notification). */
    WeaponSpread &GetSpread() { return m_Spread; }

    /**
     * @brief Public wrapper that calls Configure() and returns this weapon.
     * Used by WeaponDefs to read modelPath/modelScale without a SceneGraph.
     */
    void ApplyConfig() { Configure(); }

protected:
    /**
     * @brief Set weapon-specific configuration.
     *
     * Subclasses must call this in their constructor or before Init().
     * Must set: m_ModelPath, m_WeaponScale, m_WeaponOffset,
     *           m_FireRate, m_RecoilStrength, m_RecoilRecovery,
     *           m_MagSize, m_ReloadTime, m_BulletRange.
     */
    virtual void Configure() = 0;

    // ── Configuration (set by subclasses via Configure()) ──
    std::string m_ModelPath;
    glm::vec3   m_WeaponScale  = glm::vec3(1.0f);
    glm::vec3   m_WeaponOffset = glm::vec3(0.2f, -0.25f, 0.4f); // right, down, forward

    float m_FireRate        = 10.0f;   // shots per second
    float m_RecoilStrength  = 1.5f;    // pitch kick degrees per shot
    float m_RecoilRecovery  = 8.0f;    // pitch recovery degrees/sec
    int   m_MagSize         = 30;      // magazine capacity
    float m_ReloadTime      = 2.0f;    // seconds to reload
    float m_BulletRange     = 200.0f;  // max hitscan range (meters)
    float m_Damage          = 25.0f;   // damage per hit
    int   m_Price           = 0;       // buy menu price

    // ── Runtime state ──
    int   m_CurrentAmmo   = 0;
    float m_FireCooldown  = 0.0f;
    bool  m_IsReloading   = false;
    float m_ReloadTimer   = 0.0f;
    float m_CurrentRecoil = 0.0f;      // accumulated recoil to recover

    // ── Rendering ──
    std::shared_ptr<Core3D::Model>    m_Model;
    std::shared_ptr<Scene::SceneNode> m_Node;

    // ── Last hit result ──
    RayHitResult m_LastHit;
    glm::vec3    m_LastFireDir = glm::vec3(0.0f, 0.0f, 1.0f);

    // ── Spread system ──
    WeaponSpread m_Spread;

    // ── Reload animation base rotation (set in SyncToCamera) ──
    float m_ReloadAnimAngle = 0.0f;    // current tilt angle in degrees
};

} // namespace Weapon

#endif // CS_WEAPON_WEAPON_HPP

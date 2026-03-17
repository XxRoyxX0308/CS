#ifndef CS_GUN_GUN_HPP
#define CS_GUN_GUN_HPP

#include "pch.hpp"

#include "Core3D/Camera.hpp"
#include "Core3D/Model.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"
#include "Collision/CollisionMesh.hpp"
#include "Gun/RayCast.hpp"

#include <memory>
#include <string>

namespace Gun {

/**
 * @brief Abstract base class for all guns.
 *
 * Subclasses override Configure() to set gun-specific parameters
 * (fire rate, recoil, mag size, model path, scale, etc.).
 *
 * Usage:
 * @code
 * auto gun = std::make_unique<AACHoneyBadger>();
 * gun->Init(scene);
 * // each frame:
 * gun->Update(dt, camera, mesh);
 * gun->SyncToCamera(camera);
 * @endcode
 */
class Gun {
public:
    virtual ~Gun() = default;

    /** @brief Load the gun model and attach its node to the scene. */
    void Init(Scene::SceneGraph &scene);

    /** @brief Per-frame update: cooldowns, reload timer, reload animation, recoil recovery. */
    void Update(float dt, Core3D::Camera &camera);

    /** @brief Fire the gun: consume ammo, apply recoil, raycast for hit. */
    void Fire(Core3D::Camera &camera, const Collision::CollisionMesh &mesh);

    /** @brief Start reloading if not already reloading and magazine not full. */
    void StartReload();

    /** @brief Position/rotate the gun node to follow the camera. */
    void SyncToCamera(const Core3D::Camera &camera);

    // ── Getters ──
    int  GetCurrentAmmo() const { return m_CurrentAmmo; }
    int  GetMagSize()     const { return m_MagSize; }
    bool IsReloading()    const { return m_IsReloading; }

    /** @brief Get the last fire ray hit result (for debug/effects). */
    const RayHitResult &GetLastHit() const { return m_LastHit; }

protected:
    /**
     * @brief Set gun-specific configuration.
     *
     * Subclasses must call this in their constructor or before Init().
     * Must set: m_ModelPath, m_GunScale, m_GunOffset,
     *           m_FireRate, m_RecoilStrength, m_RecoilRecovery,
     *           m_MagSize, m_ReloadTime, m_BulletRange.
     */
    virtual void Configure() = 0;

    // ── Configuration (set by subclasses via Configure()) ──
    std::string m_ModelPath;
    glm::vec3   m_GunScale  = glm::vec3(1.0f);
    glm::vec3   m_GunOffset = glm::vec3(0.2f, -0.25f, 0.4f); // right, down, forward

    float m_FireRate        = 10.0f;   // shots per second
    float m_RecoilStrength  = 1.5f;    // pitch kick degrees per shot
    float m_RecoilRecovery  = 8.0f;    // pitch recovery degrees/sec
    int   m_MagSize         = 30;      // magazine capacity
    float m_ReloadTime      = 2.0f;    // seconds to reload
    float m_BulletRange     = 200.0f;  // max hitscan range (meters)

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

    // ── Reload animation base rotation (set in SyncToCamera) ──
    float m_ReloadAnimAngle = 0.0f;    // current tilt angle in degrees
};

} // namespace Gun

#endif // CS_GUN_GUN_HPP

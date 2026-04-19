#include "Weapon/Weapon.hpp"

#include "Util/Logger.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace Weapon {

// ============================================================================
//  Init — load model, create scene node
// ============================================================================
void Weapon::Init(Scene::SceneGraph &scene) {
    Configure();

    m_CurrentAmmo = m_MagSize;
    m_FireCooldown = 0.0f;
    m_IsReloading = false;
    m_ReloadTimer = 0.0f;
    m_CurrentRecoil = 0.0f;

    m_Spread.Reset();

    m_Model = std::make_shared<Core3D::Model>(m_ModelPath, false);
    m_Node  = std::make_shared<Scene::SceneNode>();
    m_Node->SetDrawable(m_Model);
    m_Node->SetScale(m_WeaponScale);

    scene.GetRoot()->AddChild(m_Node);

    LOG_INFO("Weapon loaded: {} (ammo: {})", m_ModelPath, m_MagSize);
}

// ============================================================================
//  Cleanup — remove weapon node from scene
// ============================================================================
void Weapon::Cleanup(Scene::SceneGraph &scene) {
    if (m_Node) {
        scene.GetRoot()->RemoveChild(m_Node);
        m_Node.reset();
    }
    m_Model.reset();
}

// ============================================================================
//  Update — cooldowns, reload, recoil recovery
// ============================================================================
void Weapon::Update(float dt, Core3D::Camera &camera,
                    bool isMoving, bool isCrouching, bool isOnGround) {
    // ── Fire cooldown ──
    if (m_FireCooldown > 0.0f) {
        m_FireCooldown -= dt;
    }

    // ── Spread recovery / growth ──
    m_Spread.Update(dt, isMoving, isCrouching, isOnGround);

    // ── Reload timer ──
    if (m_IsReloading) {
        m_ReloadTimer -= dt;

        // Reload animation: tilt down in first half, tilt back up in second half
        float progress = 1.0f - (m_ReloadTimer / m_ReloadTime);
        if (progress < 0.5f) {
            // Tilting down: 0→30 degrees
            m_ReloadAnimAngle = progress * 2.0f * 30.0f;
        } else {
            // Tilting back up: 30→0 degrees
            m_ReloadAnimAngle = (1.0f - progress) * 2.0f * 30.0f;
        }

        if (m_ReloadTimer <= 0.0f) {
            m_IsReloading = false;
            m_ReloadTimer = 0.0f;
            m_ReloadAnimAngle = 0.0f;
            m_CurrentAmmo = m_MagSize;
            LOG_INFO("Reload complete (ammo: {})", m_CurrentAmmo);
        }
    }

    // ── Recoil recovery ──
    if (m_CurrentRecoil > 0.0f) {
        float recovery = m_RecoilRecovery * dt;
        if (recovery > m_CurrentRecoil) recovery = m_CurrentRecoil;

        camera.SetPitch(camera.GetPitch() - recovery);
        camera.UpdateVectors();
        m_CurrentRecoil -= recovery;
    }
}

// ============================================================================
//  Fire — shoot if possible
// ============================================================================
void Weapon::Fire(Core3D::Camera &camera, const Physics::CollisionMesh &mesh) {
    if (m_IsReloading) return;
    if (m_FireCooldown > 0.0f) return;
    if (m_CurrentAmmo <= 0) {
        StartReload();
        return;
    }

    m_CurrentAmmo--;
    m_FireCooldown = 1.0f / m_FireRate;
    m_JustFired    = true;

    // Apply spread to the fire direction, then notify spread system
    glm::vec3 spreadDir = m_Spread.ApplySpread(camera.GetFront());
    m_Spread.OnFire();

    // Raycast using spread-deviated direction
    m_LastHit = RayCast::Cast(camera.GetPosition(),
                              spreadDir,
                              mesh,
                              m_BulletRange);

    // Then apply recoil kick
    camera.SetPitch(camera.GetPitch() + m_RecoilStrength);
    camera.UpdateVectors();
    m_CurrentRecoil = m_RecoilStrength;

    if (m_LastHit.hit) {
        LOG_DEBUG("Hit at ({:.1f}, {:.1f}, {:.1f}) dist={:.1f}",
                  m_LastHit.point.x, m_LastHit.point.y, m_LastHit.point.z,
                  m_LastHit.distance);
    }
}

// ============================================================================
//  StartReload
// ============================================================================
void Weapon::StartReload() {
    if (m_IsReloading) return;
    if (m_CurrentAmmo == m_MagSize) return;

    m_IsReloading = true;
    m_ReloadTimer = m_ReloadTime;
    m_ReloadAnimAngle = 0.0f;
    LOG_INFO("Reloading...");
}

// ============================================================================
//  SyncToCamera — position weapon in front of the camera
// ============================================================================
void Weapon::SyncToCamera(const Core3D::Camera &camera) {
    if (!m_Node) return;

    const glm::vec3 &camPos   = camera.GetPosition();
    const glm::vec3 &camFront = camera.GetFront();
    const glm::vec3 &camRight = camera.GetRight();
    const glm::vec3 &camUp    = camera.GetUp();

    // Weapon position = camera + offset in camera local space
    glm::vec3 weaponPos = camPos
                        + camRight * m_WeaponOffset.x
                        + camUp    * m_WeaponOffset.y
                        + camFront * m_WeaponOffset.z;

    m_Node->SetPosition(weaponPos);

    glm::mat3 cameraBasis(camRight, camUp, -camFront);
    glm::quat weaponRot = glm::quat_cast(cameraBasis);
    glm::quat leftTurn90 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    weaponRot = weaponRot * leftTurn90;

    if (m_ReloadAnimAngle != 0.0f) {
        float tiltRad = glm::radians(-m_ReloadAnimAngle);
        glm::quat tiltQ = glm::angleAxis(tiltRad, glm::vec3(0.0f, 0.0f, 1.0f));
        weaponRot = weaponRot * tiltQ; 
    }

    m_Node->SetRotation(weaponRot);
}

} // namespace Weapon

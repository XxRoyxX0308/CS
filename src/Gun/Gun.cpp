#include "Gun/Gun.hpp"

#include "Util/Logger.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace Gun {

// ============================================================================
//  Init — load model, create scene node
// ============================================================================
void Gun::Init(Scene::SceneGraph &scene) {
    Configure();

    m_CurrentAmmo = m_MagSize;
    m_FireCooldown = 0.0f;
    m_IsReloading = false;
    m_ReloadTimer = 0.0f;
    m_CurrentRecoil = 0.0f;

    m_Model = std::make_shared<Core3D::Model>(m_ModelPath, false);
    m_Node  = std::make_shared<Scene::SceneNode>();
    m_Node->SetDrawable(m_Model);
    m_Node->SetScale(m_GunScale);

    scene.GetRoot()->AddChild(m_Node);

    LOG_INFO("Gun loaded: {} (ammo: {})", m_ModelPath, m_MagSize);
}

// ============================================================================
//  Update — cooldowns, reload, recoil recovery
// ============================================================================
void Gun::Update(float dt, Core3D::Camera &camera) {
    // ── Fire cooldown ──
    if (m_FireCooldown > 0.0f) {
        m_FireCooldown -= dt;
    }

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
void Gun::Fire(Core3D::Camera &camera, const Collision::CollisionMesh &mesh) {
    if (m_IsReloading) return;
    if (m_FireCooldown > 0.0f) return;
    if (m_CurrentAmmo <= 0) {
        StartReload();
        return;
    }

    m_CurrentAmmo--;
    m_FireCooldown = 1.0f / m_FireRate;

    // Recoil: kick camera pitch up
    camera.SetPitch(camera.GetPitch() + m_RecoilStrength);
    camera.UpdateVectors();
    m_CurrentRecoil += m_RecoilStrength;

    // Raycast
    m_LastHit = RayCast::Cast(camera.GetPosition(),
                              camera.GetFront(),
                              mesh,
                              m_BulletRange);

    if (m_LastHit.hit) {
        LOG_DEBUG("Hit at ({:.1f}, {:.1f}, {:.1f}) dist={:.1f}",
                  m_LastHit.point.x, m_LastHit.point.y, m_LastHit.point.z,
                  m_LastHit.distance);
    }
}

// ============================================================================
//  StartReload
// ============================================================================
void Gun::StartReload() {
    if (m_IsReloading) return;
    if (m_CurrentAmmo == m_MagSize) return;

    m_IsReloading = true;
    m_ReloadTimer = m_ReloadTime;
    m_ReloadAnimAngle = 0.0f;
    LOG_INFO("Reloading...");
}

// ============================================================================
//  SyncToCamera — position gun in front of the camera
// ============================================================================
void Gun::SyncToCamera(const Core3D::Camera &camera) {
    if (!m_Node) return;

    const glm::vec3 &camPos   = camera.GetPosition();
    const glm::vec3 &camFront = camera.GetFront();
    const glm::vec3 &camRight = camera.GetRight();
    const glm::vec3 &camUp    = camera.GetUp();

    // Gun position = camera + offset in camera local space
    glm::vec3 gunPos = camPos
                     + camRight * m_GunOffset.x
                     + camUp    * m_GunOffset.y
                     + camFront * m_GunOffset.z;

    m_Node->SetPosition(gunPos);

    // Gun rotation = camera orientation (yaw + pitch)
    float yawRad   = glm::radians(camera.GetYaw());
    float pitchRad = glm::radians(camera.GetPitch());

    // Build rotation: yaw around Y, then pitch around local X
    glm::quat yawQ   = glm::angleAxis(yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat pitchQ = glm::angleAxis(pitchRad, glm::vec3(1.0f, 0.0f, 0.0f));

    glm::quat gunRot = yawQ * pitchQ;

    // Apply reload tilt animation (rotate around local right axis)
    if (m_ReloadAnimAngle != 0.0f) {
        float tiltRad = glm::radians(-m_ReloadAnimAngle);
        glm::quat tiltQ = glm::angleAxis(tiltRad, glm::vec3(1.0f, 0.0f, 0.0f));
        gunRot = gunRot * tiltQ;
    }

    m_Node->SetRotation(gunRot);
}

} // namespace Gun

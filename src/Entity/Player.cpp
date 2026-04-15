#include "Entity/Player.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Scene/SceneGraph.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

#include <SDL.h>

namespace Entity {

// ============================================================================
//  Init - Initialize camera parameters
// ============================================================================
void Player::Init(Core3D::Camera &camera) {
    camera.SetPosition(glm::vec3(0.0f, m_Height + m_CameraYOffset, 0.0f));
    camera.SetYaw(-90.0f);
    camera.SetPitch(0.0f);
    camera.SetMovementSpeed(5.0f);
    camera.SetMouseSensitivity(0.1f);
    camera.UpdateVectors();

    m_Position = camera.GetPosition();
}

// ============================================================================
//  InitModel - Initialize the character model
// ============================================================================
void Player::InitModel(Scene::SceneGraph &scene,
                       CharacterType type,
                       bool visible) {
    m_CharacterModel.Init(scene, type);
    m_CharacterModel.SetVisible(visible);
}

// ============================================================================
//  SwitchCharacter - Switch to a different character type
// ============================================================================
void Player::SwitchCharacter(Scene::SceneGraph &scene,
                             CharacterType type) {
    m_CharacterModel.SwitchCharacter(scene, type);
}

// ============================================================================
//  SpawnOnMap - Find spawn point using capsule ground sweep
// ============================================================================
void Player::SpawnOnMap(Core3D::Camera &camera,
                        const Physics::CollisionMesh &mesh) {
    float spawnX = 10.0f;
    float spawnZ = 0.0f;

    // Build a capsule high up and sweep down to find ground
    Physics::Capsule cap;
    cap.radius = m_Radius;
    cap.height = m_Height - 2.0f * m_Radius;
    if (cap.height < 0.0f) cap.height = 0.0f;
    cap.base = glm::vec3(spawnX, 100.0f, spawnZ); // Start high

    auto groundY = Physics::CapsuleCast::SweepVertical(cap, mesh, -200.0f);
    if (groundY.has_value()) {
        m_Position = glm::vec3(spawnX, groundY.value() + m_Height, spawnZ);
    } else {
        // Fallback: try alternate spawn
        spawnX = -5.0f; spawnZ = -5.0f;
        cap.base = glm::vec3(spawnX, 100.0f, spawnZ);
        groundY = Physics::CapsuleCast::SweepVertical(cap, mesh, -200.0f);
        if (groundY.has_value()) {
            m_Position = glm::vec3(spawnX, groundY.value() + m_Height, spawnZ);
        }
    }
    camera.SetPosition(m_Position + glm::vec3(0.0f, m_CameraYOffset, 0.0f));
}

// ============================================================================
//  Respawn - Reset health and return to spawn point
// ============================================================================
void Player::Respawn(Core3D::Camera &camera,
                     const Physics::CollisionMesh &mesh,
                     const glm::vec3& spawnPosition) {
    m_Position = spawnPosition;
    // Keep player on valid ground if spawn Y is slightly off.
    Physics::Capsule cap;
    cap.radius = m_Radius;
    cap.height = m_Height - 2.0f * m_Radius;
    if (cap.height < 0.0f) cap.height = 0.0f;
    cap.base = glm::vec3(m_Position.x, m_Position.y - m_Height + m_Radius + 2.0f, m_Position.z);
    auto groundY = Physics::CapsuleCast::SweepVertical(cap, mesh, -20.0f);
    if (groundY.has_value()) {
        m_Position.y = groundY.value() + m_Height;
    }
    ResetHealth();
    m_VelocityY = 0.0f;
    m_OnGround = true;
    camera.SetPosition(m_Position + glm::vec3(0.0f, m_CameraYOffset, 0.0f));
}

// ============================================================================
//  EquipWeapon - Equip a weapon
// ============================================================================
void Player::EquipWeapon(std::unique_ptr<Weapon::Weapon> weapon,
                         Scene::SceneGraph &scene) {
    // Remove old weapon model from scene
    if (m_Weapon) {
        m_Weapon->Cleanup(scene);
    }
    m_Weapon = std::move(weapon);
    m_Weapon->Init(scene);
}

// ============================================================================
//  Update - Input, movement, physics, camera sync, weapon, model
// ============================================================================
void Player::Update(float dt, Core3D::Camera &camera,
                    const Physics::CollisionMesh &mesh) {
    // ── WASD horizontal movement ──
    camera.SetPosition(m_Position);

    bool wPressed = Util::Input::IsKeyPressed(Util::Keycode::W);
    bool sPressed = Util::Input::IsKeyPressed(Util::Keycode::S);
    bool aPressed = Util::Input::IsKeyPressed(Util::Keycode::A);
    bool dPressed = Util::Input::IsKeyPressed(Util::Keycode::D);

    if (wPressed)
        camera.ProcessKeyboard(Core3D::CameraMovement::FORWARD, dt);
    if (sPressed)
        camera.ProcessKeyboard(Core3D::CameraMovement::BACKWARD, dt);
    if (aPressed)
        camera.ProcessKeyboard(Core3D::CameraMovement::LEFT, dt);
    if (dPressed)
        camera.ProcessKeyboard(Core3D::CameraMovement::RIGHT, dt);

    // Track walking state for animation
    m_IsWalking = (wPressed || sPressed || aPressed || dPressed) && m_OnGround;

    glm::vec3 desiredPos = camera.GetPosition();
    desiredPos.y = m_Position.y; // Lock Y

    // Capsule sweep + wall sliding
    TryMove(desiredPos, mesh);

    // ── Jump ──
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        Jump();
    }

    // ── Gravity and ground collision ──
    UpdatePhysics(dt, mesh);

    // ── Sync camera ──
    camera.SetPosition(m_Position + glm::vec3(0.0f, m_CameraYOffset, 0.0f));

    // ── Update character model ──
    // Position is eye height, model needs feet position
    glm::vec3 feetPos = m_Position;
    feetPos.y -= m_Height;
    m_CharacterModel.Update(dt, feetPos, camera.GetYaw(), m_IsWalking);

    // ── Weapon system ──
    if (m_Weapon) {
        // Fire (left mouse button)
        if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
            m_Weapon->Fire(camera, mesh);
        }

        // Reload (R key)
        if (Util::Input::IsKeyDown(Util::Keycode::R)) {
            m_Weapon->StartReload();
        }

        // Update weapon state (cooldown, reload animation, recoil recovery)
        m_Weapon->Update(dt, camera);

        // Sync weapon position to camera
        m_Weapon->SyncToCamera(camera);
    }
}

} // namespace Entity

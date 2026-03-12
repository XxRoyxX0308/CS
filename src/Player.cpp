#include "Player.hpp"
#include "Collision/CapsuleCast.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

#include <SDL.h>

// ============================================================================
//  Init — 攝影機初始參數
// ============================================================================
void Player::Init(Core3D::Camera &camera) {
    camera.SetPosition(glm::vec3(0.0f, m_Height, 0.0f));
    camera.SetYaw(-90.0f);
    camera.SetPitch(0.0f);
    camera.SetMovementSpeed(5.0f);
    camera.SetMouseSensitivity(0.1f);
    camera.UpdateVectors();

    m_Position = camera.GetPosition();
}

// ============================================================================
//  SpawnOnMap — 找出生點 (膠囊體地面掃掠)
// ============================================================================
void Player::SpawnOnMap(Core3D::Camera &camera,
                        const Collision::CollisionMesh &mesh) {
    float spawnX = 10.0f;
    float spawnZ = 0.0f;

    // Build a capsule high up and sweep down to find ground
    Collision::Capsule cap;
    cap.radius = m_Radius;
    cap.height = m_Height - 2.0f * m_Radius;
    if (cap.height < 0.0f) cap.height = 0.0f;
    cap.base = glm::vec3(spawnX, 100.0f, spawnZ); // start high

    auto groundY = Collision::CapsuleCast::SnapToGround(cap, mesh, 200.0f);
    if (groundY.has_value()) {
        m_Position = glm::vec3(spawnX, groundY.value() + m_Height, spawnZ);
    } else {
        // Fallback: try alternate spawn
        spawnX = -5.0f; spawnZ = -5.0f;
        cap.base = glm::vec3(spawnX, 100.0f, spawnZ);
        groundY = Collision::CapsuleCast::SnapToGround(cap, mesh, 200.0f);
        if (groundY.has_value()) {
            m_Position = glm::vec3(spawnX, groundY.value() + m_Height, spawnZ);
        }
    }
    camera.SetPosition(m_Position);
}

// ============================================================================
//  Update — 輸入 → 移動 → 物理 → 同步攝影機
// ============================================================================
void Player::Update(float dt, Core3D::Camera &camera,
                    const Collision::CollisionMesh &mesh) {
    // ── WASD 水平移動 ──
    camera.SetPosition(m_Position);

    if (Util::Input::IsKeyPressed(Util::Keycode::W))
        camera.ProcessKeyboard(Core3D::CameraMovement::FORWARD, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::S))
        camera.ProcessKeyboard(Core3D::CameraMovement::BACKWARD, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::A))
        camera.ProcessKeyboard(Core3D::CameraMovement::LEFT, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::D))
        camera.ProcessKeyboard(Core3D::CameraMovement::RIGHT, dt);

    glm::vec3 desiredPos = camera.GetPosition();
    desiredPos.y = m_Position.y; // 鎖定 Y

    // 膠囊體掃掠 + 沿牆滑行
    TryMove(desiredPos, mesh);

    // ── 跳躍 ──
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        Jump();
    }

    // ── 重力 & 地面碰撞 ──
    UpdatePhysics(dt, mesh);

    // ── 同步攝影機 ──
    camera.SetPosition(m_Position);
}

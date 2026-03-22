#include "Characters/Player.hpp"
#include "Collision/CapsuleCast.hpp"
#include "Scene/SceneGraph.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

#include <SDL.h>

// ============================================================================
//  Init — 攝影機初始參數
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
//  InitModel — 初始化角色模型
// ============================================================================
void Player::InitModel(Scene::SceneGraph &scene,
                       Characters::CharacterType type,
                       bool visible) {
    m_CharacterModel.Init(scene, type);
    m_CharacterModel.SetVisible(visible);
}

// ============================================================================
//  SwitchCharacter — 切換角色類型
// ============================================================================
void Player::SwitchCharacter(Scene::SceneGraph &scene,
                             Characters::CharacterType type) {
    m_CharacterModel.SwitchCharacter(scene, type);
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

    auto groundY = Collision::CapsuleCast::SweepVertical(cap, mesh, -200.0f);
    if (groundY.has_value()) {
        m_Position = glm::vec3(spawnX, groundY.value() + m_Height, spawnZ);
    } else {
        // Fallback: try alternate spawn
        spawnX = -5.0f; spawnZ = -5.0f;
        cap.base = glm::vec3(spawnX, 100.0f, spawnZ);
        groundY = Collision::CapsuleCast::SweepVertical(cap, mesh, -200.0f);
        if (groundY.has_value()) {
            m_Position = glm::vec3(spawnX, groundY.value() + m_Height, spawnZ);
        }
    }
    camera.SetPosition(m_Position + glm::vec3(0.0f, m_CameraYOffset, 0.0f));
}

// ============================================================================
//  Respawn — 重生（死亡後傳送到出生點並恢復血量）
// ============================================================================
void Player::Respawn(Core3D::Camera &camera,
                     const Collision::CollisionMesh &mesh) {
    // Reset health
    ResetHealth();

    // Reset velocity
    m_VelocityY = 0.0f;
    m_OnGround = true;

    // Respawn at spawn point
    SpawnOnMap(camera, mesh);
}

// ============================================================================
//  EquipGun — 裝備武器
// ============================================================================
void Player::EquipGun(std::unique_ptr<Gun::Gun> gun,
                      Scene::SceneGraph &scene) {
    m_Gun = std::move(gun);
    m_Gun->Init(scene);
}

// ============================================================================
//  Update — 輸入 → 移動 → 物理 → 同步攝影機 → 武器 → 角色模型
// ============================================================================
void Player::Update(float dt, Core3D::Camera &camera,
                    const Collision::CollisionMesh &mesh) {
    // ── WASD 水平移動 ──
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
    camera.SetPosition(m_Position + glm::vec3(0.0f, m_CameraYOffset, 0.0f));

    // ── 更新角色模型 ──
    // Position is eye height, model needs feet position
    glm::vec3 feetPos = m_Position;
    feetPos.y -= m_Height;
    m_CharacterModel.Update(dt, feetPos, camera.GetYaw(), m_IsWalking);

    // ── 武器系統 ──
    if (m_Gun) {
        // 開火（左鍵）
        if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) {
            m_Gun->Fire(camera, mesh);
        }

        // 換彈（R 鍵）
        if (Util::Input::IsKeyDown(Util::Keycode::R)) {
            m_Gun->StartReload();
        }

        // 更新武器狀態（冷卻、換彈動畫、後座力恢復）
        m_Gun->Update(dt, camera);

        // 同步武器位置到攝影機
        m_Gun->SyncToCamera(camera);
    }
}

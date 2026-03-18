// ============================================================================
//  App.cpp — CS FPS 遊戲邏輯
// ============================================================================
//
//  功能：
//  ① FPS 攝影機：WASD 走路（鎖定 Y 軸）、空白鍵跳躍、滑鼠鎖定視角
//  ② 載入 de_dust2 3D 地圖（OBJ + MTL + TGA 材質）
//  ③ 地圖網格碰撞（Mesh collision）讓腳色站在地面上
//  ④ 方向光（太陽光）+ ForwardRenderer 渲染
//  ⑤ TAB 切換 Debug 面板 / 游標鎖定
// ============================================================================

#include "App.hpp"

#include "Core/Texture.hpp"
#include "Gun/AACHoneyBadger.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"

#include <SDL.h>

// ============================================================================
//  BuildMapTransform — Source engine Z-up → OpenGL Y-up, Hammer → meters
// ============================================================================
static glm::mat4 BuildMapTransform() {
    // Source / Hammer coordinate system: X-right, Y-forward, Z-up
    // OpenGL: X-right, Y-up, Z-backward
    // → Rotate -90° around X to swap Y↔Z, then scale Hammer units to meters.
    //   1 Hammer unit ≈ 0.01905 m → scale factor ~0.02

    constexpr float SCALE = 0.02f;

    glm::mat4 transform(1.0f);
    transform = glm::scale(transform, glm::vec3(SCALE));
    transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    return transform;
}

// ============================================================================
//  Start() — 場景初始化（僅執行一次）
// ============================================================================
void App::Start() {
    LOG_TRACE("App::Start");

    // ── 1. 攝影機 & 玩家設定 ──────────────────────────────────────────────
    auto &camera = m_Scene.GetCamera();
    m_Player.Init(camera);
    
    // ── 2. 鎖定游標（FPS 模式）──────────────────────────────────────────
    SDL_SetRelativeMouseMode(SDL_TRUE);
    m_CursorLocked = true;
    
    // ── 3. 方向光（太陽光）──────────────────────────────────────────────
    Scene::DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sun.color     = glm::vec3(1.0f, 0.98f, 0.92f);
    sun.intensity = 1.5f;
    m_Scene.SetDirectionalLight(sun);

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // 天空藍色背景

    // ── 4. 載入 de_dust2 地圖 ───────────────────────────────────────────
    const std::string mapPath =
        std::string(ASSETS_DIR) + "/de_dust2-map/source/de_dust2.obj";
    LOG_INFO("Loading map: {}", mapPath);

    m_MapModel = std::make_shared<Core3D::Model>(mapPath, false);
    m_MapNode = std::make_shared<Scene::SceneNode>();

    // 從 transform 矩陣拆出 scale 與 rotation 給 SceneNode
    constexpr float SCALE = 0.02f;
    m_MapNode->SetScale(glm::vec3(SCALE));
    m_MapNode->SetRotation(
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)));
    m_MapNode->SetDrawable(m_MapModel);
    m_Scene.GetRoot()->AddChild(m_MapNode);

    // 擴大陰影範圍以覆蓋整張地圖
    m_Renderer.SetSceneRadius(80.0f);

    // ── 5. 建立地圖碰撞 ────────────────────────────────────────────────
    glm::mat4 mapTransform = BuildMapTransform();
    m_CollisionMesh.Build(*m_MapModel, mapTransform);

    // ── 6. 設定出生點 ──────────────────────────────────────────────────
    m_Player.SpawnOnMap(camera, m_CollisionMesh);

    // ── 7. 裝備武器 ──────────────────────────────────────────────────
    auto gun = std::make_unique<Gun::AACHoneyBadger>();
    m_Player.EquipGun(std::move(gun), m_Scene);

    // ── 8. 初始化彈孔效果 ────────────────────────────────────────────
    m_BulletHoles.Init();

    // ── 狀態轉移 ──
    m_CurrentState = State::UPDATE;
    LOG_TRACE("App::Start complete");
}

// ============================================================================
//  Update() — 每幀執行
// ============================================================================
void App::Update() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    auto &camera = m_Scene.GetCamera();

    // ── 退出 ──
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
        return;
    }

    // ── TAB：切換游標鎖定 / Debug 面板 ──
    if (Util::Input::IsKeyDown(Util::Keycode::TAB)) {
        m_CursorLocked = !m_CursorLocked;
        SDL_SetRelativeMouseMode(m_CursorLocked ? SDL_TRUE : SDL_FALSE);
        m_ShowDebugPanel = !m_CursorLocked;
    }

    // ── 玩家移動 + 物理 ──
    m_Player.Update(dt, camera, m_CollisionMesh);

    // ── 檢查子彈命中並生成彈孔 ──
    if (auto *gun = m_Player.GetGun()) {
        const auto &hit = gun->GetLastHit();
        // Check if this is a new hit (simple check: distance > 0 means recent hit)
        static glm::vec3 lastHitPoint(0.0f);
        if (hit.hit && hit.point != lastHitPoint) {
            m_BulletHoles.SpawnHole(hit.point, hit.normal);
            lastHitPoint = hit.point;
        }
    }

    // ── 更新彈孔效果 ──
    m_BulletHoles.Update(dt);

    // ── 滑鼠視角（FPS 鎖定模式）──
    if (m_CursorLocked) {
        int xrel = 0, yrel = 0;
        SDL_GetRelativeMouseState(&xrel, &yrel);
        if (xrel != 0 || yrel != 0) {
            camera.ProcessMouseMovement(
                static_cast<float>(xrel),
                static_cast<float>(-yrel)); // SDL Y 向下，pitch 向上為正
        }
    }

    // // ── 滾輪縮放 FOV ──
    // if (Util::Input::IfScroll()) {
    //     auto scroll = Util::Input::GetScrollDistance();
    //     camera.ProcessMouseScroll(scroll.y);
    // }

    // ── 渲染 ──
    m_Renderer.Render(m_Scene);

    // ── 繪製彈孔效果 ──
    {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = camera.GetProjectionMatrix();
        m_BulletHoles.Draw(camera, view, proj);
    }

    // ── Debug 面板 ──
    if (m_ShowDebugPanel) {
        ImGui::Begin("CS Debug");

        auto camPos = camera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
        ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.GetYaw(), camera.GetPitch());
        ImGui::Text("FOV: %.1f", camera.GetFOV());
        ImGui::Separator();
        ImGui::Text("VelocityY: %.2f", m_Player.GetVelocityY());
        ImGui::Text("OnGround: %s", m_Player.IsOnGround() ? "true" : "false");

        // 地圖碰撞資訊
        {
            Collision::Capsule dbgCap = m_Player.MakeCapsule();
            auto dbgGround = Collision::CapsuleCast::SweepVertical(dbgCap, m_CollisionMesh, -6.0f);
            ImGui::Text("GroundY: %.2f", dbgGround.has_value() ? dbgGround.value() : -9999.0f);
        }
        ImGui::Text("Triangles: %zu", m_CollisionMesh.GetTriangleCount());

        ImGui::Separator();
        ImGui::Text("FPS: %.1f", dt > 0.0f ? 1.0f / dt : 0.0f);

        // 渲染統計 (來自優化後的 ForwardRenderer)
        const auto &stats = m_Renderer.GetStats();
        ImGui::Text("Draw Calls: %zu", stats.drawCalls);
        ImGui::Text("Nodes: %zu visible / %zu total (culled: %zu)",
                    stats.visibleNodes, stats.totalNodes, stats.culledNodes);

        ImGui::Text("[TAB] Toggle cursor  [ESC] Quit");

        // 武器資訊
        if (auto *gun = m_Player.GetGun()) {
            ImGui::Separator();
            ImGui::Text("Ammo: %d / %d", gun->GetCurrentAmmo(), gun->GetMagSize());
            ImGui::Text("Reloading: %s", gun->IsReloading() ? "YES" : "no");
            const auto &hit = gun->GetLastHit();
            if (hit.hit) {
                ImGui::Text("LastHit: (%.1f, %.1f, %.1f) d=%.1f",
                            hit.point.x, hit.point.y, hit.point.z, hit.distance);
            }
        }

        // 彈孔效果
        ImGui::Separator();
        ImGui::Text("Bullet Holes: %zu", m_BulletHoles.GetCount());

        ImGui::End();
    }
}

// ============================================================================
//  End() — 清理
// ============================================================================
void App::End() {
    LOG_TRACE("App::End");
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

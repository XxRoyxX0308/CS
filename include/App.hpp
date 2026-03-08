#ifndef CS2_APP_HPP
#define CS2_APP_HPP

// ============================================================================
//  App.hpp — CS2 FPS 遊戲主類別
// ============================================================================

#include "pch.hpp" // IWYU pragma: export

// --- 3D 核心 ---
#include "Core3D/Camera.hpp"
#include "Core3D/Material.hpp"
#include "Core3D/Mesh.hpp"
#include "Core3D/Model.hpp"

// --- 渲染 ---
#include "Render/ForwardRenderer.hpp"

// --- 場景 ---
#include "Scene/Light.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"

class App {
public:
    enum class State {
        START,
        UPDATE,
        END,
    };

    State GetCurrentState() const { return m_CurrentState; }

    void Start();
    void Update();
    void End();

private:
    // ── 產生棋盤格紋理（CPU → GPU） ──
    std::shared_ptr<Core::Texture> GenerateCheckerboardTexture();

    // ── 產生地板 Mesh ──
    std::shared_ptr<Core3D::Model> GenerateFloorModel(
        const std::shared_ptr<Core::Texture> &texture);

    State m_CurrentState = State::START;

    // ── 場景 & 渲染 ──
    Scene::SceneGraph m_Scene;
    Render::ForwardRenderer m_Renderer;

    // ── 地板 ──
    std::shared_ptr<Core3D::Model> m_FloorModel;
    std::shared_ptr<Scene::SceneNode> m_FloorNode;

    // ── FPS 攝影機物理 ──
    float m_PlayerHeight = 1.7f;  // 眼睛高度（公尺）
    float m_VelocityY    = 0.0f;  // 垂直速度
    float m_Gravity      = 9.81f; // 重力加速度
    float m_JumpSpeed    = 5.0f;  // 跳躍初速
    bool  m_OnGround     = true;  // 是否在地面

    // ── 滑鼠狀態 ──
    bool m_CursorLocked = true;

    // ── Debug ──
    bool m_ShowDebugPanel = false;
};

#endif // CS2_APP_HPP

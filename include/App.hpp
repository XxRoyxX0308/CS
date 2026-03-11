#ifndef CS_APP_HPP
#define CS_APP_HPP

// ============================================================================
//  App.hpp — CS FPS 遊戲主類別
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

#include "MapCollider.hpp"
#include "Player.hpp"

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
    State m_CurrentState = State::START;

    // ── 場景 & 渲染 ──
    Scene::SceneGraph m_Scene;
    Render::ForwardRenderer m_Renderer;

    // ── 地圖模型 ──
    std::shared_ptr<Core3D::Model> m_MapModel;
    std::shared_ptr<Scene::SceneNode> m_MapNode;

    // ── 地圖碰撞 ──
    MapCollider m_MapCollider;

    // ── 玩家 ──
    Player m_Player;

    // ── 滑鼠狀態 ──
    bool m_CursorLocked = true;

    // ── Debug ──
    bool m_ShowDebugPanel = false;
};

#endif // CS_APP_HPP

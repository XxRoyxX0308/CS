#ifndef CS_APP_HPP
#define CS_APP_HPP

// Prevent Windows min/max macros from interfering with std library and GLM
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

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

#include "Collision/CollisionMesh.hpp"
#include "Collision/CapsuleCast.hpp"
#include "Effects/BulletHole.hpp"
#include "Characters/Player.hpp"
#include "Characters/RemotePlayer.hpp"
#include "Network/NetworkManager.hpp"

#include <unordered_map>
#include <string>

class App {
public:
    enum class State {
        MAIN_MENU,    // 主選單（Create/Join）
        LOBBY,        // 等待連線/等待玩家
        GAME_START,   // 遊戲初始化
        GAME_UPDATE,  // 遊戲進行中
        GAME_END,     // 結束清理
    };

    State GetCurrentState() const { return m_CurrentState; }

    void MainMenu();
    void Lobby();
    void Start();
    void Update();
    void End();

private:
    State m_CurrentState = State::MAIN_MENU;

    // ── 場景 & 渲染 ──
    Scene::SceneGraph m_Scene;
    Render::ForwardRenderer m_Renderer;

    // ── 地圖模型 ──
    std::shared_ptr<Core3D::Model> m_MapModel;
    std::shared_ptr<Scene::SceneNode> m_MapNode;

    // ── 地圖碰撞 ──
    Collision::CollisionMesh m_CollisionMesh;

    // ── 玩家 ──
    Player m_Player;

    // ── 彈孔效果 ──
    Effects::BulletHoleManager m_BulletHoles;

    // ── 滑鼠狀態 ──
    bool m_CursorLocked = true;

    // ── Debug ──
    bool m_ShowDebugPanel = false;

    // ══════════════════════════════════════════════════════════════════════
    // 網路系統
    // ══════════════════════════════════════════════════════════════════════

    // ── 網路管理器 ──
    Network::NetworkManager m_Network;

    // ── 遠端玩家 ──
    std::unordered_map<uint8_t, Characters::RemotePlayer> m_RemotePlayers;

    // ── 選單狀態 ──
    struct MenuState {
        char serverName[64] = "CS Game";
        char playerName[32] = "Player";
        char ipAddress[32] = "127.0.0.1";
        int selectedServerIndex = -1;
        bool isConnecting = false;
        float connectionTimer = 0.0f;
    } m_MenuState;

    // ── 網路輔助方法 ──
    void SetupNetworkCallbacks();
    void UpdateNetworkHost(float dt);
    void UpdateNetworkClient(float dt);
    void BuildAndBroadcastGameState();
    void ProcessRemoteInputs(float dt);
    void HandleBulletEffect(const glm::vec3& pos, const glm::vec3& normal);

    // ── 輸入採樣（用於網路同步）──
    Network::InputState SampleLocalInput() const;

    // ── 玩家命中檢測結果 ──
    struct PlayerHitResult {
        bool hit = false;
        uint8_t playerId = 0xFF;
        float distance = std::numeric_limits<float>::max();
        glm::vec3 point{0.0f};
    };

    // ── 戰鬥系統 ──
    PlayerHitResult CheckPlayerHit(const glm::vec3& origin,
                                   const glm::vec3& direction,
                                   float maxDist);
    void HandlePlayerDamage(uint8_t victimId, float damage, const glm::vec3& hitPoint);
    void CheckAndHandleRespawn();
    glm::vec3 GetSpawnPoint() const;
};

#endif // CS_APP_HPP

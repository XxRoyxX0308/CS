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
//  App.hpp — CS FPS Game Main Class
// ============================================================================

#include "pch.hpp" // IWYU pragma: export

// ── 3D Core ──
#include "Core3D/Camera.hpp"
#include "Core3D/Material.hpp"
#include "Core3D/Mesh.hpp"
#include "Core3D/Model.hpp"

// ── Render ──
#include "Render/ForwardRenderer.hpp"

// ── Scene ──
#include "Scene/Light.hpp"
#include "Scene/SceneGraph.hpp"
#include "Scene/SceneNode.hpp"

// ── Physics ──
#include "Physics/CollisionMesh.hpp"
#include "Physics/CapsuleCast.hpp"

// ── Entity ──
#include "Entity/Player.hpp"
#include "Entity/RemotePlayer.hpp"

// ── Effects ──
#include "Effects/BulletHole.hpp"

// ── Network ──
#include "Network/NetworkManager.hpp"

#include <unordered_map>
#include <string>

class App {
public:
    enum class State {
        MAIN_MENU,    // Main menu (Create/Join)
        LOBBY,        // Waiting for connection/players
        GAME_START,   // Game initialization
        GAME_UPDATE,  // Game in progress
        GAME_END,     // Cleanup
    };

    State GetCurrentState() const { return m_CurrentState; }

    void MainMenu();
    void Lobby();
    void Start();
    void Update();
    void End();

private:
    State m_CurrentState = State::MAIN_MENU;

    // ── Scene & Rendering ──
    Scene::SceneGraph m_Scene;
    Render::ForwardRenderer m_Renderer;

    // ── Map Model ──
    std::shared_ptr<Core3D::Model> m_MapModel;
    std::shared_ptr<Scene::SceneNode> m_MapNode;

    // ── Map Collision ──
    Physics::CollisionMesh m_CollisionMesh;

    // ── Player ──
    Entity::Player m_Player;

    // ── Bullet Hole Effects ──
    Effects::BulletHoleManager m_BulletHoles;

    // ── Mouse State ──
    bool m_CursorLocked = true;

    // ── Debug ──
    bool m_ShowDebugPanel = false;

    // ══════════════════════════════════════════════════════════════════════
    // Network System
    // ══════════════════════════════════════════════════════════════════════

    // ── Network Manager ──
    Network::NetworkManager m_Network;

    // ── Remote Players ──
    std::unordered_map<uint8_t, Entity::RemotePlayer> m_RemotePlayers;

    // ── Menu State ──
    struct MenuState {
        char serverName[64] = "CS Game";
        char playerName[32] = "Player";
        char ipAddress[32] = "127.0.0.1";
        int selectedServerIndex = -1;
        bool isConnecting = false;
        float connectionTimer = 0.0f;
    } m_MenuState;

    // ── Network Helper Methods ──
    void SetupNetworkCallbacks();
    void UpdateNetworkHost(float dt);
    void UpdateNetworkClient(float dt);
    void BuildAndBroadcastGameState();
    void ProcessRemoteInputs(float dt);
    void HandleBulletEffect(const glm::vec3& pos, const glm::vec3& normal);

    // ── Input Sampling (for network sync) ──
    Network::InputState SampleLocalInput() const;

    // ── Player Hit Detection Result ──
    struct PlayerHitResult {
        bool hit = false;
        uint8_t playerId = 0xFF;
        float distance = std::numeric_limits<float>::max();
        glm::vec3 point{0.0f};
    };

    // ── Combat System ──
    PlayerHitResult CheckPlayerHit(const glm::vec3& origin,
                                   const glm::vec3& direction,
                                   float maxDist);
    void HandlePlayerDamage(uint8_t victimId, float damage, const glm::vec3& hitPoint);
    void CheckAndHandleRespawn();
    glm::vec3 GetSpawnPoint() const;
};

#endif // CS_APP_HPP

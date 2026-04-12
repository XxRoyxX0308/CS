#ifndef CS_APP_UIMANAGER_HPP
#define CS_APP_UIMANAGER_HPP

#include "Entity/Player.hpp"
#include "Network/NetworkManager.hpp"
#include "Render/ForwardRenderer.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Effects/BulletHole.hpp"

#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace App {

/**
 * @brief Menu state data for Main Menu UI.
 */
struct MenuState {
    char serverName[64] = "CS Game";
    char playerName[32] = "Player";
    char ipAddress[32] = "127.0.0.1";
    int selectedServerIndex = -1;
    bool isConnecting = false;
    float connectionTimer = 0.0f;
};

/**
 * @brief Callback types for UI actions.
 */
struct UICallbacks {
    std::function<void(const std::string& serverName, const std::string& playerName)> onCreateGame;
    std::function<void(const std::string& ip, const std::string& playerName)> onJoinGame;
    std::function<void()> onStartGame;
    std::function<void()> onCancel;
    std::function<void()> onQuit;
    std::function<void()> onStartDiscovery;
    std::function<void()> onStopDiscovery;
};

/**
 * @brief Manages all UI rendering: menus, lobby, HUD, debug panel.
 */
class UIManager {
public:
    UIManager() = default;

    /** @brief Set UI action callbacks. */
    void SetCallbacks(UICallbacks callbacks) { m_Callbacks = std::move(callbacks); }

    /** @brief Get/Set menu state. */
    MenuState& GetMenuState() { return m_MenuState; }
    const MenuState& GetMenuState() const { return m_MenuState; }

    /**
     * @brief Render Main Menu.
     * @param network Network manager for server discovery.
     * @param dt Delta time.
     */
    void RenderMainMenu(Network::NetworkManager& network, float dt);

    /**
     * @brief Render Lobby screen.
     * @param network Network manager for player info.
     * @param remotePlayerCount Number of remote players.
     */
    void RenderLobby(const Network::NetworkManager& network, size_t remotePlayerCount);

    /**
     * @brief Render Debug Panel.
     */
    void RenderDebugPanel(const Core3D::Camera& camera,
                          const Entity::Player& player,
                          const Physics::CollisionMesh& collisionMesh,
                          const Render::ForwardRenderer::RenderStats& renderStats,
                          const Network::NetworkManager& network,
                          size_t remotePlayerCount,
                          size_t bulletHoleCount,
                          float dt);

    /**
     * @brief Render in-game HUD (health/ammo).
     */
    void RenderHUD(const Entity::Player& player);

    /** @brief Check if debug panel should be shown. */
    bool IsDebugPanelVisible() const { return m_ShowDebugPanel; }

    /** @brief Toggle debug panel visibility. */
    void ToggleDebugPanel() { m_ShowDebugPanel = !m_ShowDebugPanel; }

    /** @brief Set debug panel visibility. */
    void SetDebugPanelVisible(bool visible) { m_ShowDebugPanel = visible; }

private:
    MenuState m_MenuState;
    UICallbacks m_Callbacks;
    bool m_ShowDebugPanel = false;
};

} // namespace App

#endif // CS_APP_UIMANAGER_HPP

#ifndef CS_APP_APP_HPP
#define CS_APP_APP_HPP

// Prevent Windows min/max macros from interfering with std library and GLM
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include "App/StateManager.hpp"
#include "App/InputManager.hpp"
#include "App/UIManager.hpp"
#include "App/CombatManager.hpp"
#include "App/NetworkController.hpp"
#include "App/GameManager.hpp"
#include "Network/NetworkManager.hpp"

namespace App {

/**
 * @brief Main application coordinator.
 *
 * Acts as a Mediator between all managers, delegating work to
 * specialized components for a clean separation of concerns.
 */
class Application {
public:
    /** @brief State enum exposed for main loop compatibility. */
    using State = GameState;

    Application();

    /** @brief Get current game state. */
    State GetCurrentState() const { return m_StateManager.GetState(); }

    /** @brief Main Menu state handler. */
    void MainMenu();

    /** @brief Lobby state handler. */
    void Lobby();

    /** @brief Game Start state handler. */
    void Start();

    /** @brief Game Update state handler. */
    void Update();

    /** @brief Game End state handler. */
    void End();

private:
    // ── Managers ──
    StateManager m_StateManager;
    InputManager m_InputManager;
    UIManager m_UIManager;
    CombatManager m_CombatManager;
    NetworkController m_NetworkController;
    GameManager m_GameManager;

    // ── Network ──
    Network::NetworkManager m_Network;

    // ── Initialization Flag ──
    bool m_CallbacksInitialized = false;

    // ── Helper Methods ──
    void SetupUICallbacks();
    void SetupNetworkCallbacks();
    void HandleCharacterSwitch();
    void HandleBulletHit();
    void SendCharacterConfig();
};

} // namespace App

// Backwards compatibility alias
using App = App::Application;

#endif // CS_APP_APP_HPP

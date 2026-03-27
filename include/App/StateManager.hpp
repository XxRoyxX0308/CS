#ifndef CS_APP_STATEMANAGER_HPP
#define CS_APP_STATEMANAGER_HPP

#include <functional>

namespace App {

/**
 * @brief Game state enumeration.
 */
enum class GameState {
    MAIN_MENU,    // Main menu (Create/Join)
    LOBBY,        // Waiting for connection/players
    GAME_START,   // Game initialization
    GAME_UPDATE,  // Game in progress
    GAME_END,     // Cleanup
};

/**
 * @brief Manages game state transitions.
 *
 * Provides a clean interface for state changes with optional callbacks.
 */
class StateManager {
public:
    using StateChangedCallback = std::function<void(GameState oldState, GameState newState)>;

    StateManager() = default;

    /** @brief Get the current game state. */
    GameState GetState() const { return m_CurrentState; }

    /** @brief Set a new game state. Triggers callback if registered. */
    void SetState(GameState newState);

    /** @brief Check if in a specific state. */
    bool IsInState(GameState state) const { return m_CurrentState == state; }

    /** @brief Check if game is actively running (GAME_START or GAME_UPDATE). */
    bool IsGameActive() const {
        return m_CurrentState == GameState::GAME_START ||
               m_CurrentState == GameState::GAME_UPDATE;
    }

    /** @brief Check if in menu states (MAIN_MENU or LOBBY). */
    bool IsInMenu() const {
        return m_CurrentState == GameState::MAIN_MENU ||
               m_CurrentState == GameState::LOBBY;
    }

    /** @brief Register a callback for state changes. */
    void SetOnStateChanged(StateChangedCallback callback) {
        m_OnStateChanged = std::move(callback);
    }

private:
    GameState m_CurrentState = GameState::MAIN_MENU;
    StateChangedCallback m_OnStateChanged;
};

} // namespace App

#endif // CS_APP_STATEMANAGER_HPP

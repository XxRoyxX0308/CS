#include "App/StateManager.hpp"

namespace App {

void StateManager::SetState(GameState newState) {
    if (m_CurrentState == newState) return;

    GameState oldState = m_CurrentState;
    m_CurrentState = newState;

    if (m_OnStateChanged) {
        m_OnStateChanged(oldState, newState);
    }
}

} // namespace App

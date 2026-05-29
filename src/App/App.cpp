// ============================================================================
//  App.cpp — CS FPS Game Application entry points
// ============================================================================
//
//  Application implementation is split across:
//  - AppSetup.cpp    (setup, menu/lobby/start lifecycle)
//  - AppGameplay.cpp (update loop and gameplay handlers)
//  - AppMatch.cpp    (match-state and post-match flow)
//
// ============================================================================

#include "App/App.hpp"

namespace App {

Application::Application() {
    SetupUICallbacks();
}

} // namespace App

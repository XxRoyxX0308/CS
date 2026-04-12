// ============================================================================
//  main.cpp — CS FPS Game Entry Point
// ============================================================================

#include "App/App.hpp"
#include "Core/Context.hpp"
#include "Util/Input.hpp"

int main(int, char **) {
    auto context = Core::Context::GetInstance();
    App::Application app;

    while (!context->GetExit()) {
        context->Setup();

        switch (app.GetCurrentState()) {
        case App::GameState::MAIN_MENU:
            app.MainMenu();
            break;

        case App::GameState::LOBBY:
            app.Lobby();
            break;

        case App::GameState::GAME_START:
            app.Start();
            break;

        case App::GameState::GAME_UPDATE:
            app.Update();
            break;

        case App::GameState::GAME_END:
            app.End();
            context->SetExit(true);
            break;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        context->Update();
    }

    return 0;
}

// ============================================================================
//  main.cpp — CS FPS Game Entry Point
// ============================================================================

#include "App.hpp"
#include "Core/Context.hpp"
#include "Util/Input.hpp"

int main(int, char **) {
    auto context = Core::Context::GetInstance();
    App app;

    while (!context->GetExit()) {
        context->Setup();

        switch (app.GetCurrentState()) {
        case App::State::MAIN_MENU:
            app.MainMenu();
            break;

        case App::State::LOBBY:
            app.Lobby();
            break;

        case App::State::GAME_START:
            app.Start();
            break;

        case App::State::GAME_UPDATE:
            app.Update();
            break;

        case App::State::GAME_END:
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

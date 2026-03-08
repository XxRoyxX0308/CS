// ============================================================================
//  main3D.cpp — PTSD 3D 框架範例：程式進入點
// ============================================================================
//
//  這是 Example3D 的 main()，負責：
//  ① 建立 OpenGL / SDL2 / ImGui 環境（Core::Context）
//  ② 進入主迴圈，根據 App3D 的狀態機分派到 Start / Update / End
//  ③ 每幀結束時，渲染 ImGui 並交換緩衝區
//
//  主迴圈流程（每幀一次）：
//
//    context->Setup()          ← 處理 SDL 事件、清除畫面、開始新的 ImGui 幀
//        ↓
//    app.Start() / Update()   ← 你的遊戲邏輯（僅其中一個會執行）
//        ↓
//    ImGui::Render()          ← 產生 ImGui 繪製資料
//    ImGui_ImplOpenGL3_RenderDrawData() ← 將 ImGui 繪製到 OpenGL
//        ↓
//    context->Update()        ← SDL_GL_SwapWindow（交換前後緩衝區、顯示畫面）
//
// ============================================================================

#include "App3D.hpp"        // 我們的 3D 範例應用程式

#include "Core/Context.hpp" // Core::Context — SDL2 + OpenGL + ImGui 的初始化與管理
#include "Util/Input.hpp"   // Util::Input — 鍵盤 / 滑鼠輸入偵測

int main(int, char **) {
    // ──────────────────────────────────────────────────────────────────────
    //  1. 取得 Context 單例（Singleton）
    // ──────────────────────────────────────────────────────────────────────
    //  Context::GetInstance() 會在首次呼叫時：
    //  - 初始化 SDL2（視窗、事件、音訊）
    //  - 建立 OpenGL 4.1 Core Profile 環境
    //  - 初始化 GLEW
    //  - 設定 ImGui（docking 分支）
    //  之後的呼叫直接回傳同一個實例的指標。
    auto context = Core::Context::GetInstance();

    // 建立我們的 3D 應用程式實例
    App3D app;

    // ──────────────────────────────────────────────────────────────────────
    //  2. 主迴圈 — 持續運行直到 Context 設定退出
    // ──────────────────────────────────────────────────────────────────────
    while (!context->GetExit()) {
        // Setup() 負責：
        //  - SDL_PollEvent（處理視窗事件、鍵盤、滑鼠）
        //  - glClear（清除顏色 / 深度緩衝區）
        //  - ImGui_ImplOpenGL3_NewFrame / ImGui::NewFrame
        context->Setup();

        // ── 狀態機分派 ──
        // 根據 App3D 目前的狀態呼叫對應方法
        switch (app.GetCurrentState()) {
        case App3D::State::START:
            // 初始化（僅執行一次）：建立攝影機、燈光、載入模型
            app.Start();
            break;

        case App3D::State::UPDATE:
            // 每幀更新：輸入處理 → 場景動畫 → 3D 渲染 → ImGui
            app.Update();
            break;

        case App3D::State::END:
            // 程式結束：釋放資源，設定 Context 退出標記
            app.End();
            context->SetExit(true);
            break;
        }

        // ── ImGui 渲染 ──
        // ImGui::Render() 產生繪製指令列表
        // ImGui_ImplOpenGL3_RenderDrawData() 透過 OpenGL 將 UI 繪製到畫面
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // ── 交換緩衝區 ──
        // context->Update() 呼叫 SDL_GL_SwapWindow()
        // 將後緩衝區的內容顯示到螢幕上（雙緩衝機制）
        context->Update();
    }

    return 0;
}

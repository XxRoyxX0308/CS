// ============================================================================
//  App.cpp — CS FPS 遊戲邏輯
// ============================================================================
//
//  功能：
//  ① FPS 攝影機：WASD 走路（鎖定 Y 軸）、空白鍵跳躍、滑鼠鎖定視角
//  ② 程序化棋盤格地板（100×100 m，50×50 格，每格 2 m）
//  ③ 方向光（太陽光）+ ForwardRenderer 渲染
//  ④ TAB 切換 Debug 面板 / 游標鎖定
// ============================================================================

#include "App.hpp"

#include "Core/Texture.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"

#include <SDL.h>

// ============================================================================
//  GenerateCheckerboardTexture — 產生 64×64 棋盤格紋理
// ============================================================================
std::shared_ptr<Core::Texture> App::GenerateCheckerboardTexture() {
    constexpr int TEX_SIZE   = 64;   // 紋理總像素
    constexpr int CELL_SIZE  = 8;    // 每格像素 → 8×8 = 64 格棋盤

    std::vector<unsigned char> pixels(TEX_SIZE * TEX_SIZE * 3);

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            bool isWhite = ((x / CELL_SIZE) + (y / CELL_SIZE)) % 2 == 0;
            unsigned char c = isWhite ? 200 : 80;
            int idx = (y * TEX_SIZE + x) * 3;
            pixels[idx + 0] = c;
            pixels[idx + 1] = c;
            pixels[idx + 2] = c;
        }
    }

    auto tex = std::make_shared<Core::Texture>(
        GL_RGB, TEX_SIZE, TEX_SIZE, pixels.data(), false);

    // 設定 GL_REPEAT 讓 UV > 1 時重複貼圖
    GLuint texId = tex->GetTextureId();
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

// ============================================================================
//  GenerateFloorModel — 產生 100×100 m 地板平面
// ============================================================================
std::shared_ptr<Core3D::Model> App::GenerateFloorModel(
    const std::shared_ptr<Core::Texture> &texture) {

    constexpr float HALF = 50.0f;  // 半邊長 → 總共 100 m
    constexpr float UV   = 50.0f;  // UV 重複次數 → 每格 2 m

    // 地板 4 頂點 (Y = 0 平面，法線朝上)
    std::vector<Core3D::Vertex3D> verts = {
        // position            normal        texCoords  tangent        bitangent      boneIDs              boneWeights
        {{-HALF, 0, -HALF}, {0, 1, 0}, {0,  0},  {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ HALF, 0, -HALF}, {0, 1, 0}, {UV, 0},  {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ HALF, 0,  HALF}, {0, 1, 0}, {UV, UV}, {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-HALF, 0,  HALF}, {0, 1, 0}, {0,  UV}, {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
    };

    std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

    // 棋盤格作為 diffuse 貼圖
    std::vector<Core3D::MeshTexture> textures;
    textures.push_back({texture, Core3D::TextureType::DIFFUSE, "checkerboard"});

    auto mesh = Core3D::Mesh(verts, indices, textures);

    std::vector<Core3D::Mesh> meshes;
    meshes.push_back(std::move(mesh));

    return std::make_shared<Core3D::Model>(std::move(meshes));
}

// ============================================================================
//  Start() — 場景初始化（僅執行一次）
// ============================================================================
void App::Start() {
    LOG_TRACE("App::Start");

    // ── 1. 攝影機設定 ─────────────────────────────────────────────────────
    auto &camera = m_Scene.GetCamera();
    camera.SetPosition(glm::vec3(0.0f, m_PlayerHeight, 0.0f));
    camera.SetYaw(-90.0f);
    camera.SetPitch(0.0f);
    camera.SetMovementSpeed(5.0f);
    camera.SetMouseSensitivity(0.1f);
    camera.UpdateVectors();

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // 天空藍色背景

    // ── 2. 鎖定游標（FPS 模式）──────────────────────────────────────────
    SDL_SetRelativeMouseMode(SDL_TRUE);
    m_CursorLocked = true;

    // ── 3. 方向光（太陽光）──────────────────────────────────────────────
    Scene::DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sun.color     = glm::vec3(1.0f, 0.98f, 0.92f);
    sun.intensity = 1.5f;
    m_Scene.SetDirectionalLight(sun);

    // ── 4. 棋盤格地板 ──────────────────────────────────────────────────
    auto checkerTex = GenerateCheckerboardTexture();
    m_FloorModel = GenerateFloorModel(checkerTex);

    m_FloorNode = std::make_shared<Scene::SceneNode>();
    m_FloorNode->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    m_FloorNode->SetDrawable(m_FloorModel);
    m_Scene.GetRoot()->AddChild(m_FloorNode);

    // ── 狀態轉移 ──
    m_CurrentState = State::UPDATE;
    LOG_TRACE("App::Start complete");
}

// ============================================================================
//  Update() — 每幀執行
// ============================================================================
void App::Update() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    auto &camera = m_Scene.GetCamera();

    // ── 退出 ──
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
        return;
    }

    // ── TAB：切換游標鎖定 / Debug 面板 ──
    if (Util::Input::IsKeyDown(Util::Keycode::TAB)) {
        m_CursorLocked = !m_CursorLocked;
        SDL_SetRelativeMouseMode(m_CursorLocked ? SDL_TRUE : SDL_FALSE);
        m_ShowDebugPanel = !m_CursorLocked;
    }

    // ── WASD 水平移動（鎖定 Y 軸）──
    float savedY = camera.GetPosition().y;

    if (Util::Input::IsKeyPressed(Util::Keycode::W))
        camera.ProcessKeyboard(Core3D::CameraMovement::FORWARD, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::S))
        camera.ProcessKeyboard(Core3D::CameraMovement::BACKWARD, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::A))
        camera.ProcessKeyboard(Core3D::CameraMovement::LEFT, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::D))
        camera.ProcessKeyboard(Core3D::CameraMovement::RIGHT, dt);

    // 移動後還原 Y 軸（走路不會飛起來）
    glm::vec3 pos = camera.GetPosition();
    pos.y = savedY;
    camera.SetPosition(pos);

    // ── 跳躍 ──
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE) && m_OnGround) {
        m_VelocityY = m_JumpSpeed;
        m_OnGround = false;
    }

    // 重力 & 垂直位移
    m_VelocityY -= m_Gravity * dt;
    float playerY = camera.GetPosition().y + m_VelocityY * dt;

    // 地面碰撞
    if (playerY <= m_PlayerHeight) {
        playerY = m_PlayerHeight;
        m_VelocityY = 0.0f;
        m_OnGround = true;
    }

    camera.SetPosition(glm::vec3(
        camera.GetPosition().x,
        playerY,
        camera.GetPosition().z));

    // ── 滑鼠視角（FPS 鎖定模式）──
    if (m_CursorLocked) {
        int xrel = 0, yrel = 0;
        SDL_GetRelativeMouseState(&xrel, &yrel);
        if (xrel != 0 || yrel != 0) {
            camera.ProcessMouseMovement(
                static_cast<float>(xrel),
                static_cast<float>(-yrel)); // SDL Y 向下，pitch 向上為正
        }
    }

    // ── 滾輪縮放 FOV ──
    if (Util::Input::IfScroll()) {
        auto scroll = Util::Input::GetScrollDistance();
        camera.ProcessMouseScroll(scroll.y);
    }

    // ── 渲染 ──
    m_Renderer.Render(m_Scene);

    // ── Debug 面板 ──
    if (m_ShowDebugPanel) {
        ImGui::Begin("CS Debug");

        auto camPos = camera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
        ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.GetYaw(), camera.GetPitch());
        ImGui::Text("FOV: %.1f", camera.GetFOV());
        ImGui::Separator();
        ImGui::Text("VelocityY: %.2f", m_VelocityY);
        ImGui::Text("OnGround: %s", m_OnGround ? "true" : "false");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", dt > 0.0f ? 1.0f / dt : 0.0f);
        ImGui::Text("[TAB] Toggle cursor  [ESC] Quit");

        ImGui::End();
    }
}

// ============================================================================
//  End() — 清理
// ============================================================================
void App::End() {
    LOG_TRACE("App::End");
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

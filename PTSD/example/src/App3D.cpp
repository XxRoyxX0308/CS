// ============================================================================
//  App3D.cpp — PTSD 3D 框架範例：完整 3D 場景初始化與主迴圈
// ============================================================================
//
//  本範例展示：
//  ① 攝影機初始化與 FPS 風格控制
//  ② 方向光（太陽光）與點光源的設定
//  ③ 程序化產生 3D 立方體（24 頂點 / 6 面 / 36 索引）
//  ④ 使用 SceneNode 將模型掛入場景圖
//  ⑤ ForwardRenderer 多 Pass 渲染
//  ⑥ ImGui 即時除錯面板
//
//  ※ 若要載入外部模型（.obj / .fbx / .gltf），將程序化方塊替換為：
//     m_CubeModel = std::make_shared<Core3D::Model>("path/to/model.obj");
// ============================================================================

#include "App3D.hpp"

#include "Util/Input.hpp"    // 鍵盤 / 滑鼠輸入偵測
#include "Util/Keycode.hpp"  // 鍵碼常數（WASD、ESC 等）
#include "Util/Logger.hpp"   // 日誌系統（LOG_TRACE / LOG_ERROR 等）
#include "Util/Time.hpp"     // DeltaTime（以毫秒為單位）

// ============================================================================
//  Start() — 場景初始化（僅執行一次）
// ============================================================================
void App3D::Start() {
    LOG_TRACE("App3D::Start");

    // ──────────────────────────────────────────────────────────────────────
    //  1. 攝影機設定
    // ──────────────────────────────────────────────────────────────────────
    //  SceneGraph 內建一個 Camera 實例。
    //  SetPosition() 設定攝影機在世界座標的位置。
    //  SetYaw(-90) 表示攝影機面朝 -Z 方向（OpenGL 慣例）。
    //  SetPitch(-10) 使攝影機稍微向下看，以便看到立方體。
    //  UpdateVectors() 根據 Yaw/Pitch 重新計算 Front / Right / Up 向量。
    auto &camera = m_Scene.GetCamera();
    camera.SetPosition(glm::vec3(0.0f, 2.0f, 8.0f)); // 位置：y=2 向上偏移, z=8 後退
    camera.SetYaw(-90.0f);    // 水平旋轉角（度），-90 = 面朝 -Z
    camera.SetPitch(-10.0f);  // 垂直俯仰角（度），-10 = 略微向下
    camera.UpdateVectors();   // 重要：設定 Yaw/Pitch 後必須呼叫以更新方向向量

    // 設定背景顏色（深灰藍色），方便辨識是否有 3D 物件繪製
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // ──────────────────────────────────────────────────────────────────────
    //  2. 方向光（Directional Light / 太陽光）
    // ──────────────────────────────────────────────────────────────────────
    //  方向光模擬無限遠光源（如太陽），只有方向沒有位置。
    //  direction：光照射的方向（從光源指向場景），需正規化。
    //  color：光的 RGB 顏色（0~1 範圍）。
    //  intensity：光照強度倍率，越大越亮。
    Scene::DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f)); // 從左上方照下
    sun.color = glm::vec3(1.0f, 0.98f, 0.92f); // 暖白色（模擬太陽光）
    sun.intensity = 1.5f;                        // 強度 1.5 倍
    m_Scene.SetDirectionalLight(sun);

    // ──────────────────────────────────────────────────────────────────────
    //  3. 點光源（Point Light）
    // ──────────────────────────────────────────────────────────────────────
    //  點光源從一個位置向四周發射光線，亮度隨距離衰減。
    //  衰減公式：attenuation = 1.0 / (constant + linear*d + quadratic*d²)
    //  - constant = 1.0（基礎，通常為 1）
    //  - linear   = 0.09（線性衰減，值越大近處越暗）
    //  - quadratic = 0.032（二次衰減，值越大遠處掉落越快）
    Scene::PointLight pointLight;
    pointLight.position  = glm::vec3(3.0f, 3.0f, 3.0f); // 初始位置
    pointLight.color     = glm::vec3(0.2f, 0.5f, 1.0f);  // 藍色光
    pointLight.intensity = 5.0f;                           // 較亮
    pointLight.constant  = 1.0f;
    pointLight.linear    = 0.09f;
    pointLight.quadratic = 0.032f;
    m_Scene.AddPointLight(pointLight);

    // ──────────────────────────────────────────────────────────────────────
    //  4. 建立 3D 模型 —— 程序化立方體
    // ──────────────────────────────────────────────────────────────────────
    //
    //  ★ 外部模型載入方式（使用 Assimp）：
    //     m_CubeModel = std::make_shared<Core3D::Model>(
    //         ASSETS_DIR "/models/backpack/backpack.obj");
    //
    //  此處以程序化方式建立一個單位立方體（邊長 2，中心在原點）。
    //
    //  Vertex3D 欄位說明：
    //    position   (vec3) — 頂點在模型空間的位置
    //    normal     (vec3) — 法線方向（用於光照計算）
    //    texCoords  (vec2) — UV 貼圖座標 (0~1)
    //    tangent    (vec3) — 切線（用於法線貼圖 / Normal Mapping）
    //    bitangent  (vec3) — 副切線
    //    boneIDs    [4]    — 骨骼動畫的骨骼索引（-1 表示無骨骼）
    //    boneWeights[4]    — 對應骨骼的權重（0 表示無影響）
    // ──────────────────────────────────────────────────────────────────────

    // 每個面 4 個頂點，6 個面共 24 個頂點
    // 格式：{ position, normal, texCoords, tangent, bitangent, boneIDs, boneWeights }
    std::vector<Core3D::Vertex3D> cubeVerts = {
        // ── 前面 (Front face, +Z) ──
        // 法線朝 +Z，切線朝 +X，副切線朝 +Y
        {{-1, -1,  1}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1, -1,  1}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1,  1,  1}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1,  1,  1}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        // ── 後面 (Back face, -Z) ──
        {{ 1, -1, -1}, {0, 0,-1}, {0, 0}, {-1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1, -1, -1}, {0, 0,-1}, {1, 0}, {-1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1,  1, -1}, {0, 0,-1}, {1, 1}, {-1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1,  1, -1}, {0, 0,-1}, {0, 1}, {-1, 0, 0}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        // ── 上面 (Top face, +Y) ──
        {{-1,  1,  1}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0,-1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1,  1,  1}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0,-1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1,  1, -1}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0,-1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1,  1, -1}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0,-1}, {-1,-1,-1,-1}, {0,0,0,0}},
        // ── 下面 (Bottom face, -Y) ──
        {{-1, -1, -1}, {0,-1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1, -1, -1}, {0,-1, 0}, {1, 0}, {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1, -1,  1}, {0,-1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1, -1,  1}, {0,-1, 0}, {0, 1}, {1, 0, 0}, {0, 0, 1}, {-1,-1,-1,-1}, {0,0,0,0}},
        // ── 右面 (Right face, +X) ──
        {{ 1, -1,  1}, {1, 0, 0}, {0, 0}, {0, 0,-1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1, -1, -1}, {1, 0, 0}, {1, 0}, {0, 0,-1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1,  1, -1}, {1, 0, 0}, {1, 1}, {0, 0,-1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{ 1,  1,  1}, {1, 0, 0}, {0, 1}, {0, 0,-1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        // ── 左面 (Left face, -X) ──
        {{-1, -1, -1}, {-1, 0, 0}, {0, 0}, {0, 0, 1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1, -1,  1}, {-1, 0, 0}, {1, 0}, {0, 0, 1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1,  1,  1}, {-1, 0, 0}, {1, 1}, {0, 0, 1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
        {{-1,  1, -1}, {-1, 0, 0}, {0, 1}, {0, 0, 1}, {0, 1, 0}, {-1,-1,-1,-1}, {0,0,0,0}},
    };

    // ── 索引陣列：每個面 2 個三角形（6 個索引），共 36 個索引 ──
    // 三角形繞線順序為逆時鐘（CCW），符合 OpenGL 正面判定
    std::vector<unsigned int> cubeIndices;
    for (unsigned int face = 0; face < 6; ++face) {
        unsigned int base = face * 4; // 每面 4 個頂點
        // 第一個三角形：v0 → v1 → v2
        cubeIndices.push_back(base + 0);
        cubeIndices.push_back(base + 1);
        cubeIndices.push_back(base + 2);
        // 第二個三角形：v0 → v2 → v3
        cubeIndices.push_back(base + 0);
        cubeIndices.push_back(base + 2);
        cubeIndices.push_back(base + 3);
    }

    // 沒有貼圖 → shader 會使用內建的 fallback 顏色
    std::vector<Core3D::MeshTexture> noTextures;

    // 使用頂點 / 索引 / 貼圖建立 Mesh（GPU 上傳在建構子中完成）
    auto cubeMesh = Core3D::Mesh(cubeVerts, cubeIndices, noTextures);

    // 將 Mesh 包裝進 Model（Model 可包含多個 Mesh，此處只有一個）
    std::vector<Core3D::Mesh> meshes;
    meshes.push_back(std::move(cubeMesh));
    m_CubeModel = std::make_shared<Core3D::Model>(std::move(meshes));

    // ──────────────────────────────────────────────────────────────────────
    //  5. 建立場景節點並掛入場景圖
    // ──────────────────────────────────────────────────────────────────────
    //  SceneNode 是場景圖中的一個節點，支援：
    //  - 位置（Position）、旋轉（Rotation, 四元數）、縮放（Scale）
    //  - 父子階層（AddChild / RemoveChild）
    //  - 可附加一個 Drawable3D（如 Model）
    //  - GetWorldTransform() 會自動乘上父節點的變換矩陣
    m_CubeNode = std::make_shared<Scene::SceneNode>();
    m_CubeNode->SetPosition(glm::vec3(0.0f, 1.0f, 0.0f)); // 稍微抬高，避免與地板重疊
    m_CubeNode->SetDrawable(m_CubeModel); // 將 Model 附加到節點（Model 實作了 Drawable3D）

    // 將節點加入場景圖的根節點下
    // 場景圖結構：Root → CubeNode → (未來可加更多子節點)
    m_Scene.GetRoot()->AddChild(m_CubeNode);

    // 狀態切換為 UPDATE，下一幀開始進入主迴圈
    m_CurrentState = State::UPDATE;
    LOG_TRACE("App3D::Start complete");
}

// ============================================================================
//  Update() — 每幀執行的主更新邏輯
// ============================================================================
//  執行順序：
//  ① 處理輸入（鍵盤移動 + 滑鼠旋轉 + 滾輪縮放）
//  ② 更新場景動畫（點光源繞行 + 立方體旋轉）
//  ③ 呼叫 ForwardRenderer::Render() 執行多 Pass 渲染
//  ④ 繪製 ImGui 除錯面板
// ============================================================================
void App3D::Update() {
    // ── 計算 DeltaTime ──
    // Time::GetDeltaTimeMs() 回傳上一幀到這一幀的毫秒數
    // 轉為秒，用於物理 / 動畫計算
    float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    auto &camera = m_Scene.GetCamera();

    // ──────────────────────────────────────────────────────────────────────
    //  1. ESC 離開程式
    // ──────────────────────────────────────────────────────────────────────
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
        return;
    }

    // ──────────────────────────────────────────────────────────────────────
    //  2. 鍵盤移動攝影機（WASD + Q/E）
    // ──────────────────────────────────────────────────────────────────────
    //  Camera::ProcessKeyboard() 依據攝影機的 Front / Right / Up 向量
    //  移動攝影機位置，速度會乘上 dt 確保幀率無關。
    if (Util::Input::IsKeyPressed(Util::Keycode::W))
        camera.ProcessKeyboard(Core3D::CameraMovement::FORWARD, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::S))
        camera.ProcessKeyboard(Core3D::CameraMovement::BACKWARD, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::A))
        camera.ProcessKeyboard(Core3D::CameraMovement::LEFT, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::D))
        camera.ProcessKeyboard(Core3D::CameraMovement::RIGHT, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::Q))
        camera.ProcessKeyboard(Core3D::CameraMovement::DOWN, dt);
    if (Util::Input::IsKeyPressed(Util::Keycode::E))
        camera.ProcessKeyboard(Core3D::CameraMovement::UP, dt);

    // ──────────────────────────────────────────────────────────────────────
    //  3. 滑鼠右鍵拖曳 → 旋轉攝影機（FPS 風格）
    // ──────────────────────────────────────────────────────────────────────
    //  計算滑鼠在 X/Y 方向的偏移量，傳給 Camera::ProcessMouseMovement()
    //  來更新 Yaw（水平旋轉）和 Pitch（垂直俯仰）。
    //  m_FirstMouse 用來避免首次按下時因為上一幀座標為 0 產生的跳動。
    auto cursorPos = Util::Input::GetCursorPosition();
    if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_RB)) {
        if (m_FirstMouse) {
            m_LastMouseX = cursorPos.x;
            m_LastMouseY = cursorPos.y;
            m_FirstMouse = false;
        }
        float xOffset = m_LastMouseX - cursorPos.x;            // 水平偏移
        float yOffset = m_LastMouseY + cursorPos.y;            // 垂直偏移（螢幕 Y 軸反轉）
        m_LastMouseX = cursorPos.x;
        m_LastMouseY = cursorPos.y;
        camera.ProcessMouseMovement(xOffset, yOffset);         // 內部會乘上靈敏度
    } else {
        m_FirstMouse = true; // 放開右鍵時重設，下次按下重新捕捉起始座標
    }

    // ──────────────────────────────────────────────────────────────────────
    //  4. 滑鼠滾輪 → 縮放（調整 FOV）
    // ──────────────────────────────────────────────────────────────────────
    if (Util::Input::IfScroll()) {
        auto scroll = Util::Input::GetScrollDistance();
        camera.ProcessMouseScroll(scroll.y); // scroll.y > 0 放大, < 0 縮小
    }

    // ──────────────────────────────────────────────────────────────────────
    //  5. 點光源動畫 — 繞場景中心旋轉
    // ──────────────────────────────────────────────────────────────────────
    //  以三角函數讓點光源在 XZ 平面上繞圈，半徑 5 單位。
    m_LightAngle += dt * 0.5f; // 旋轉速度（弧度/秒）
    if (!m_Scene.GetPointLights().empty()) {
        auto &pl = m_Scene.GetPointLights()[0]; // 取得第一個點光源的參照
        pl.position.x = 5.0f * std::cos(m_LightAngle);
        pl.position.z = 5.0f * std::sin(m_LightAngle);
    }

    // ──────────────────────────────────────────────────────────────────────
    //  6. 立方體旋轉動畫
    // ──────────────────────────────────────────────────────────────────────
    //  使用四元數（Quaternion）繞 Y 軸旋轉，速度為 dt * 0.5 弧度/幀。
    //  glm::rotate(quat, angle, axis) 回傳新的四元數。
    glm::quat rot = m_CubeNode->GetRotation();
    rot = glm::rotate(rot, dt * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    m_CubeNode->SetRotation(rot);

    // ──────────────────────────────────────────────────────────────────────
    //  7. 渲染 3D 場景
    // ──────────────────────────────────────────────────────────────────────
    //  ForwardRenderer::Render() 內部執行三個 Pass：
    //    Pass 1 — ShadowPass:   從方向光角度渲染深度貼圖（2048x2048）
    //    Pass 2 — GeometryPass: 使用 Phong 或 PBR shader 渲染所有場景節點
    //                           同時上傳 Matrices3D UBO (binding 0) 和
    //                           Lights UBO (binding 1)
    //    Pass 3 — SkyboxPass:   最後繪製天空盒（若有設定）
    m_Renderer.Render(m_Scene);

    // ──────────────────────────────────────────────────────────────────────
    //  8. ImGui 除錯面板
    // ──────────────────────────────────────────────────────────────────────
    //  按 TAB 鍵切換面板顯示 / 隱藏。
    //  面板可即時調整方向光的方向、顏色、強度，以及點光源的參數。
    if (Util::Input::IsKeyDown(Util::Keycode::TAB)) {
        m_ShowDebugPanel = !m_ShowDebugPanel;
    }

    if (m_ShowDebugPanel) {
        ImGui::Begin("3D Debug Panel"); // 建立 ImGui 視窗

        // --- 攝影機資訊 ---
        auto camPos = camera.GetPosition();
        ImGui::Text("Camera: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
        ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.GetYaw(), camera.GetPitch());
        ImGui::Text("FOV: %.1f", camera.GetFOV());

        ImGui::Separator();

        // --- FPS 顯示 ---
        ImGui::Text("FPS: %.1f", 1000.0 / Util::Time::GetDeltaTimeMs());

        // --- PBR / Phong 切換 ---
        if (ImGui::Checkbox("Use PBR", &m_UsePBR)) {
            // TODO: 將 m_UsePBR 傳給 ForwardRenderer 切換渲染模式
        }

        // --- 方向光即時調整 ---
        auto &sun = m_Scene.GetDirectionalLight();
        ImGui::SliderFloat3("Sun Dir", &sun.direction.x, -1.0f, 1.0f);
        ImGui::SliderFloat("Sun Intensity", &sun.intensity, 0.0f, 10.0f);
        ImGui::ColorEdit3("Sun Color", &sun.color.x);

        // --- 點光源即時調整 ---
        if (!m_Scene.GetPointLights().empty()) {
            auto &pl = m_Scene.GetPointLights()[0];
            ImGui::Separator();
            ImGui::Text("Point Light 0");
            ImGui::SliderFloat3("PL Position", &pl.position.x, -10.0f, 10.0f);
            ImGui::ColorEdit3("PL Color", &pl.color.x);
            ImGui::SliderFloat("PL Intensity", &pl.intensity, 0.0f, 20.0f);
        }

        ImGui::End(); // 結束 ImGui 視窗
    }
}

// ============================================================================
//  End() — 程式結束時的清理
// ============================================================================
//  目前大部分 OpenGL 資源由各類別的解構子自動釋放（RAII 設計）。
//  若有需要手動釋放的資源可在此處處理。
// ============================================================================
void App3D::End() {
    LOG_TRACE("App3D::End");
}

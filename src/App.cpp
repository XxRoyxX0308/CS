// ============================================================================
//  App.cpp — CS FPS 遊戲邏輯
// ============================================================================
//
//  功能：
//  ① FPS 攝影機：WASD 走路（鎖定 Y 軸）、空白鍵跳躍、滑鼠鎖定視角
//  ② 載入 de_dust2 3D 地圖（OBJ + MTL + TGA 材質）
//  ③ 地圖網格碰撞（Mesh collision）讓腳色站在地面上
//  ④ 方向光（太陽光）+ ForwardRenderer 渲染
//  ⑤ TAB 切換 Debug 面板 / 游標鎖定
//  ⑥ 網路連線 (LAN Listen Server)
// ============================================================================

#include "App.hpp"

#include "Core/Texture.hpp"
#include "Gun/AACHoneyBadger.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"

#include <SDL.h>
#include <imgui.h>

// ============================================================================
//  BuildMapTransform — Source engine Z-up → OpenGL Y-up, Hammer → meters
// ============================================================================
static glm::mat4 BuildMapTransform() {
    constexpr float SCALE = 0.02f;

    glm::mat4 transform(1.0f);
    transform = glm::scale(transform, glm::vec3(SCALE));
    transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    return transform;
}

// ============================================================================
//  SetupNetworkCallbacks — 設置網路事件回調
// ============================================================================
void App::SetupNetworkCallbacks() {
    m_Network.SetOnPlayerJoined([this](uint8_t playerId, const std::string& name) {
        LOG_INFO("Player {} ({}) joined the game", name, playerId);

        // Create remote player
        auto& remote = m_RemotePlayers[playerId];
        remote.SetPlayerId(playerId);
        remote.Init(m_Scene, Characters::CharacterType::TERRORIST);
    });

    m_Network.SetOnPlayerLeft([this](uint8_t playerId) {
        LOG_INFO("Player {} left the game", playerId);

        // Remove remote player
        auto it = m_RemotePlayers.find(playerId);
        if (it != m_RemotePlayers.end()) {
            it->second.SetVisible(false);
            m_RemotePlayers.erase(it);
        }
    });

    m_Network.SetOnConnected([this](uint8_t playerId) {
        LOG_INFO("Connected to server as player {}", playerId);
        m_MenuState.isConnecting = false;
        m_CurrentState = State::GAME_START;

        // Send our character/gun config to server after a short delay (after game starts)
        // This will be done in Update when the game starts
    });

    m_Network.SetOnDisconnected([this]() {
        LOG_INFO("Disconnected from server");
        m_RemotePlayers.clear();
        m_CurrentState = State::MAIN_MENU;
    });

    m_Network.SetOnBulletEffect([this](const glm::vec3& pos, const glm::vec3& normal) {
        HandleBulletEffect(pos, normal);
    });

    m_Network.SetOnPlayerConfig([this](uint8_t playerId, uint8_t characterType, uint8_t /*gunType*/) {
        LOG_INFO("Player {} changed to character type {}", playerId, characterType);

        auto it = m_RemotePlayers.find(playerId);
        if (it != m_RemotePlayers.end()) {
            // Re-initialize with correct character type
            auto type = (characterType == 0) ? Characters::CharacterType::FBI
                                              : Characters::CharacterType::TERRORIST;
            it->second.Init(m_Scene, type);
        }
    });
}

// ============================================================================
//  SampleLocalInput — 採樣本地輸入狀態
// ============================================================================
Network::InputState App::SampleLocalInput() const {
    Network::InputState input;
    input.keys = 0;

    if (Util::Input::IsKeyPressed(Util::Keycode::W)) input.keys |= Network::INPUT_W;
    if (Util::Input::IsKeyPressed(Util::Keycode::S)) input.keys |= Network::INPUT_S;
    if (Util::Input::IsKeyPressed(Util::Keycode::A)) input.keys |= Network::INPUT_A;
    if (Util::Input::IsKeyPressed(Util::Keycode::D)) input.keys |= Network::INPUT_D;
    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) input.keys |= Network::INPUT_JUMP;
    if (Util::Input::IsKeyPressed(Util::Keycode::MOUSE_LB)) input.keys |= Network::INPUT_FIRE;
    if (Util::Input::IsKeyDown(Util::Keycode::R)) input.keys |= Network::INPUT_RELOAD;

    auto& camera = m_Scene.GetCamera();
    input.yaw = camera.GetYaw();
    input.pitch = camera.GetPitch();

    // Include absolute position for accurate sync
    input.position = m_Player.GetPosition();

    // Set flags
    input.flags = 0;
    if (m_Player.IsWalking()) input.flags |= Network::FLAG_IS_WALKING;
    if (m_Player.IsOnGround()) input.flags |= Network::FLAG_ON_GROUND;

    return input;
}

// ============================================================================
//  HandleBulletEffect — 處理彈孔效果
// ============================================================================
void App::HandleBulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    m_BulletHoles.SpawnHole(pos, normal);
}

// ============================================================================
//  MainMenu() — 主選單
// ============================================================================
void App::MainMenu() {
    // 釋放游標
    if (m_CursorLocked) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        m_CursorLocked = false;
    }

    // 設置網路回調
    static bool callbacksSetup = false;
    if (!callbacksSetup) {
        SetupNetworkCallbacks();
        callbacksSetup = true;
    }

    // 清除螢幕
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 更新網路（用於 LAN 發現）
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_Network.Update(dt);

    // 主選單視窗
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 400));

    ImGui::Begin("Counter-Strike LAN", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CS:GO Style FPS - LAN Multiplayer");
    ImGui::Separator();
    ImGui::Spacing();

    // ── 玩家名稱 ──
    ImGui::Text("Player Name:");
    ImGui::InputText("##PlayerName", m_MenuState.playerName, sizeof(m_MenuState.playerName));
    ImGui::Spacing();

    ImGui::Separator();

    // ══════════════════════════════════════════════════════════════════════
    // 創建遊戲區塊
    // ══════════════════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Host a Game");

    ImGui::Text("Server Name:");
    ImGui::InputText("##ServerName", m_MenuState.serverName, sizeof(m_MenuState.serverName));

    if (ImGui::Button("Create Game", ImVec2(200, 40))) {
        if (m_Network.HostGame(Network::DEFAULT_PORT, m_MenuState.serverName, m_MenuState.playerName)) {
            m_CurrentState = State::LOBBY;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ══════════════════════════════════════════════════════════════════════
    // 加入遊戲區塊
    // ══════════════════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Join a Game");

    // 手動 IP 輸入
    ImGui::Text("IP Address:");
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##IPAddress", m_MenuState.ipAddress, sizeof(m_MenuState.ipAddress));
    ImGui::SameLine();
    if (ImGui::Button("Connect")) {
        if (m_Network.JoinGame(m_MenuState.ipAddress, Network::DEFAULT_PORT, m_MenuState.playerName)) {
            m_MenuState.isConnecting = true;
            m_MenuState.connectionTimer = 0.0f;
        }
    }

    // LAN 伺服器列表
    ImGui::Text("LAN Servers:");
    ImGui::BeginChild("ServerList", ImVec2(0, 80), true);

    const auto& servers = m_Network.GetDiscoveredServers();
    for (size_t i = 0; i < servers.size(); ++i) {
        const auto& server = servers[i];
        char label[128];
        snprintf(label, sizeof(label), "%s (%d/%d) - %s:%d",
                 server.name.c_str(), server.playerCount, server.maxPlayers,
                 server.ip.c_str(), server.port);

        if (ImGui::Selectable(label, m_MenuState.selectedServerIndex == static_cast<int>(i))) {
            m_MenuState.selectedServerIndex = static_cast<int>(i);
            strncpy(m_MenuState.ipAddress, server.ip.c_str(), sizeof(m_MenuState.ipAddress) - 1);
        }
    }

    ImGui::EndChild();

    // 探索按鈕
    if (m_Network.IsDiscovering()) {
        if (ImGui::Button("Stop Refresh")) {
            m_Network.StopDiscovery();
        }
    } else {
        if (ImGui::Button("Refresh LAN")) {
            m_Network.StartDiscovery();
        }
    }

    // 連線中狀態
    if (m_MenuState.isConnecting) {
        m_MenuState.connectionTimer += dt;
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Connecting...");

        if (m_MenuState.connectionTimer > 5.0f) {
            m_MenuState.isConnecting = false;
            m_Network.Disconnect();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ── 退出按鈕 ──
    if (ImGui::Button("Quit", ImVec2(100, 30))) {
        m_CurrentState = State::GAME_END;
    }

    ImGui::End();
}

// ============================================================================
//  Lobby() — 大廳（等待玩家）
// ============================================================================
void App::Lobby() {
    // 清除螢幕
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_Network.Update(dt);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 250));

    ImGui::Begin("Game Lobby", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (m_Network.IsHost()) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Hosting: %s", m_Network.GetGameName().c_str());
        ImGui::Text("Players: %d/%d", m_Network.GetPlayerCount(), Network::MAX_PLAYERS);
        ImGui::Separator();

        ImGui::Text("Players in lobby:");
        ImGui::BulletText("Host (You)");

        for (const auto& [playerId, info] : m_RemotePlayers) {
            ImGui::BulletText("Player %d", playerId);
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Start Game", ImVec2(150, 40))) {
            m_CurrentState = State::GAME_START;
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            m_Network.Disconnect();
            m_CurrentState = State::MAIN_MENU;
        }
    } else {
        // Client mode (should not normally be here, transitions directly to GAME_START)
        ImGui::Text("Waiting for connection...");

        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            m_Network.Disconnect();
            m_CurrentState = State::MAIN_MENU;
        }
    }

    ImGui::End();
}

// ============================================================================
//  Start() — 場景初始化（僅執行一次）
// ============================================================================
void App::Start() {
    LOG_TRACE("App::Start");

    // ── 1. 攝影機 & 玩家設定 ──────────────────────────────────────────────
    auto &camera = m_Scene.GetCamera();
    m_Player.Init(camera);

    // ── 2. 鎖定游標（FPS 模式）──────────────────────────────────────────
    SDL_SetRelativeMouseMode(SDL_TRUE);
    m_CursorLocked = true;

    // ── 3. 方向光（太陽光）──────────────────────────────────────────────
    Scene::DirectionalLight sun;
    sun.direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sun.color     = glm::vec3(1.0f, 0.98f, 0.92f);
    sun.intensity = 1.5f;
    m_Scene.SetDirectionalLight(sun);

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // 天空藍色背景

    // ── 4. 載入 de_dust2 地圖 ───────────────────────────────────────────
    const std::string mapPath =
        std::string(ASSETS_DIR) + "/de_dust2-map/source/de_dust2.obj";
    LOG_INFO("Loading map: {}", mapPath);

    m_MapModel = std::make_shared<Core3D::Model>(mapPath, false);
    m_MapNode = std::make_shared<Scene::SceneNode>();

    constexpr float SCALE = 0.02f;
    m_MapNode->SetScale(glm::vec3(SCALE));
    m_MapNode->SetRotation(
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, 0.0f)));
    m_MapNode->SetDrawable(m_MapModel);
    m_Scene.GetRoot()->AddChild(m_MapNode);

    m_Renderer.SetSceneRadius(80.0f);

    // ── 5. 建立地圖碰撞 ────────────────────────────────────────────────
    glm::mat4 mapTransform = BuildMapTransform();
    m_CollisionMesh.Build(*m_MapModel, mapTransform);

    // ── 6. 設定出生點 ──────────────────────────────────────────────────
    m_Player.SpawnOnMap(camera, m_CollisionMesh);

    // ── 7. 初始化角色模型 ────────────────────────────────────────────
    m_Player.InitModel(m_Scene, Characters::CharacterType::FBI, true);

    // ── 8. 裝備武器 ──────────────────────────────────────────────────
    auto gun = std::make_unique<Gun::AACHoneyBadger>();
    m_Player.EquipGun(std::move(gun), m_Scene);

    // ── 9. 初始化彈孔效果 ────────────────────────────────────────────
    m_BulletHoles.Init();

    // ── 10. 發送角色配置給其他玩家 ────────────────────────────────────
    if (m_Network.IsClient()) {
        auto charType = m_Player.GetCharacterModel().GetCharacterType();
        uint8_t charTypeId = (charType == Characters::CharacterType::FBI) ? 0 : 1;
        m_Network.SendPlayerConfig(charTypeId, 0);  // 0 = AAC Honey Badger
    } else if (m_Network.IsHost()) {
        // Host broadcasts its own config
        auto charType = m_Player.GetCharacterModel().GetCharacterType();
        uint8_t charTypeId = (charType == Characters::CharacterType::FBI) ? 0 : 1;
        m_Network.BroadcastPlayerConfig(0, charTypeId, 0);  // Player 0 = Host
    }

    // ── 狀態轉移 ──
    m_CurrentState = State::GAME_UPDATE;
    LOG_TRACE("App::Start complete");
}

// ============================================================================
//  BuildAndBroadcastGameState — 建立並廣播遊戲狀態（Host only）
// ============================================================================
void App::BuildAndBroadcastGameState() {
    if (!m_Network.IsHost()) return;

    std::vector<Network::NetPlayerState> states;

    // Add host player (player 0)
    Network::NetPlayerState hostState;
    hostState.playerId = 0;
    hostState.SetPosition(m_Player.GetPosition());

    auto& camera = m_Scene.GetCamera();
    hostState.yaw = camera.GetYaw();
    hostState.pitch = camera.GetPitch();
    hostState.velocityY = m_Player.GetVelocityY();
    hostState.health = static_cast<uint8_t>(m_Player.GetHealth());
    hostState.currentAmmo = m_Player.GetGun() ? m_Player.GetGun()->GetCurrentAmmo() : 0;

    hostState.flags = 0;
    if (m_Player.IsOnGround()) hostState.flags |= Network::FLAG_ON_GROUND;
    if (m_Player.GetGun() && m_Player.GetGun()->IsReloading()) hostState.flags |= Network::FLAG_IS_RELOADING;
    hostState.flags |= Network::FLAG_IS_ALIVE;
    if (m_Player.IsWalking()) hostState.flags |= Network::FLAG_IS_WALKING;

    states.push_back(hostState);

    // Add remote players
    for (const auto& [playerId, remote] : m_RemotePlayers) {
        Network::NetPlayerState state;
        state.playerId = playerId;
        state.SetPosition(remote.GetPosition());
        state.yaw = remote.GetYaw();
        state.pitch = remote.GetPitch();
        state.health = static_cast<uint8_t>(remote.GetHealth());
        state.flags = remote.IsAlive() ? Network::FLAG_IS_ALIVE : 0;
        states.push_back(state);
    }

    m_Network.BroadcastGameState(states.data(), static_cast<uint8_t>(states.size()));
}

// ============================================================================
//  ProcessRemoteInputs — 處理遠端玩家輸入（Host only）
// ============================================================================
void App::ProcessRemoteInputs(float /*dt*/) {
    auto inputs = m_Network.GetPendingInputs();

    for (const auto& pending : inputs) {
        auto it = m_RemotePlayers.find(pending.playerId);
        if (it == m_RemotePlayers.end()) continue;

        auto& remote = it->second;
        const auto& input = pending.input;

        // Use absolute position from client
        glm::vec3 position(input.posX, input.posY, input.posZ);
        bool isWalking = (input.flags & Network::FLAG_IS_WALKING) != 0;

        // Update remote player state with absolute position
        remote.SetPosition(position);
        remote.SetYaw(input.yaw);
        remote.SetPitch(input.pitch);
        remote.SetWalking(isWalking);
    }
}

// ============================================================================
//  UpdateNetworkHost — Host 模式更新
// ============================================================================
void App::UpdateNetworkHost(float dt) {
    // Process network events
    m_Network.Update(dt);

    // Process remote player inputs
    ProcessRemoteInputs(dt);

    // Update all remote players (interpolation and model)
    for (auto& [playerId, remote] : m_RemotePlayers) {
        remote.Update(dt);
    }

    // Broadcast game state
    BuildAndBroadcastGameState();
}

// ============================================================================
//  UpdateNetworkClient — Client 模式更新
// ============================================================================
void App::UpdateNetworkClient(float dt) {
    // Process network events
    m_Network.Update(dt);

    // Send local input
    Network::InputState input = SampleLocalInput();
    m_Network.SendInput(input);

    // Update remote players with interpolated state
    float renderTime = m_Network.GetRenderTime();
    for (auto& [playerId, remote] : m_RemotePlayers) {
        auto state = m_Network.GetInterpolatedState(playerId);
        if (state) {
            remote.UpdateFromNetworkState(*state, dt);
        }
    }
}

// ============================================================================
//  Update() — 每幀執行
// ============================================================================
void App::Update() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    auto &camera = m_Scene.GetCamera();

    // ── 退出 ──
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_Network.Disconnect();
        m_CurrentState = State::GAME_END;
        return;
    }

    // ── TAB：切換游標鎖定 / Debug 面板 ──
    if (Util::Input::IsKeyDown(Util::Keycode::TAB)) {
        m_CursorLocked = !m_CursorLocked;
        SDL_SetRelativeMouseMode(m_CursorLocked ? SDL_TRUE : SDL_FALSE);
        m_ShowDebugPanel = !m_CursorLocked;
    }

    // ── V：切換角色模型可見性 ──
    if (Util::Input::IsKeyDown(Util::Keycode::V)) {
        m_Player.ToggleModelVisibility();
    }

    // ── C：切換角色類型（FBI ↔ Terrorist）──
    if (Util::Input::IsKeyDown(Util::Keycode::C)) {
        auto currentType = m_Player.GetCharacterModel().GetCharacterType();
        auto newType = (currentType == Characters::CharacterType::FBI)
                           ? Characters::CharacterType::TERRORIST
                           : Characters::CharacterType::FBI;
        m_Player.SwitchCharacter(m_Scene, newType);

        // Notify other players about character change
        uint8_t charTypeId = (newType == Characters::CharacterType::FBI) ? 0 : 1;
        if (m_Network.IsClient()) {
            m_Network.SendPlayerConfig(charTypeId, 0);
        } else if (m_Network.IsHost()) {
            m_Network.BroadcastPlayerConfig(0, charTypeId, 0);
        }
    }

    // ══════════════════════════════════════════════════════════════════════
    // 網路更新
    // ══════════════════════════════════════════════════════════════════════
    if (m_Network.IsHost()) {
        UpdateNetworkHost(dt);
    } else if (m_Network.IsClient()) {
        UpdateNetworkClient(dt);
    }

    // ── 玩家移動 + 物理 ──
    m_Player.Update(dt, camera, m_CollisionMesh);

    // ── 檢查子彈命中並生成彈孔 ──
    if (auto *gun = m_Player.GetGun()) {
        const auto &hit = gun->GetLastHit();
        static glm::vec3 lastHitPoint(0.0f);
        if (hit.hit && hit.point != lastHitPoint) {
            m_BulletHoles.SpawnHole(hit.point, hit.normal);
            lastHitPoint = hit.point;

            // Sync bullet effect across network
            if (m_Network.IsHost()) {
                // Host broadcasts to all clients
                m_Network.BroadcastBulletEffect(hit.point, hit.normal);
            } else if (m_Network.IsClient()) {
                // Client sends to server (which will broadcast to others and notify host)
                m_Network.SendBulletEffect(hit.point, hit.normal);
            }
        }
    }

    // ── 更新彈孔效果 ──
    m_BulletHoles.Update(dt);

    // ── 更新遠端玩家 ──
    for (auto& [playerId, remote] : m_RemotePlayers) {
        // Remote players are updated via network state
    }

    // ── 滑鼠視角（FPS 鎖定模式）──
    if (m_CursorLocked) {
        int xrel = 0, yrel = 0;
        SDL_GetRelativeMouseState(&xrel, &yrel);
        if (xrel != 0 || yrel != 0) {
            camera.ProcessMouseMovement(
                static_cast<float>(xrel),
                static_cast<float>(-yrel));
        }
    }

    // ── 渲染 ──
    m_Renderer.Render(m_Scene);

    // ── 繪製彈孔效果 ──
    {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 proj = camera.GetProjectionMatrix();
        m_BulletHoles.Draw(camera, view, proj);
    }

    // ── Debug 面板 ──
    if (m_ShowDebugPanel) {
        ImGui::Begin("CS Debug");

        auto camPos = camera.GetPosition();
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
        ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.GetYaw(), camera.GetPitch());
        ImGui::Text("FOV: %.1f", camera.GetFOV());
        ImGui::Separator();
        ImGui::Text("VelocityY: %.2f", m_Player.GetVelocityY());
        ImGui::Text("OnGround: %s", m_Player.IsOnGround() ? "true" : "false");

        {
            Collision::Capsule dbgCap = m_Player.MakeCapsule();
            auto dbgGround = Collision::CapsuleCast::SweepVertical(dbgCap, m_CollisionMesh, -6.0f);
            ImGui::Text("GroundY: %.2f", dbgGround.has_value() ? dbgGround.value() : -9999.0f);
        }
        ImGui::Text("Triangles: %zu", m_CollisionMesh.GetTriangleCount());

        ImGui::Separator();
        ImGui::Text("FPS: %.1f", dt > 0.0f ? 1.0f / dt : 0.0f);

        const auto &stats = m_Renderer.GetStats();
        ImGui::Text("Draw Calls: %zu", stats.drawCalls);
        ImGui::Text("Nodes: %zu visible / %zu total (culled: %zu)",
                    stats.visibleNodes, stats.totalNodes, stats.culledNodes);

        ImGui::Text("[TAB] Toggle cursor  [ESC] Quit");
        ImGui::Text("[V] Toggle model  [C] Switch character");

        // 網路狀態
        ImGui::Separator();
        if (m_Network.IsHost()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mode: HOST");
            ImGui::Text("Players: %d", m_Network.GetPlayerCount());
        } else if (m_Network.IsClient()) {
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Mode: CLIENT");
            ImGui::Text("Player ID: %d", m_Network.GetLocalPlayerId());
        } else {
            ImGui::Text("Mode: OFFLINE");
        }
        ImGui::Text("Remote Players: %zu", m_RemotePlayers.size());

        // 角色血量資訊
        ImGui::Separator();
        ImGui::Text("Health: %.0f / %.0f", m_Player.GetHealth(), m_Player.GetMaxHealth());
        ImGui::Text("Walking: %s", m_Player.IsWalking() ? "YES" : "no");
        ImGui::Text("Model Visible: %s", m_Player.IsModelVisible() ? "YES" : "no");
        ImGui::Text("Character: %s",
                    m_Player.GetCharacterModel().GetCharacterType() ==
                            Characters::CharacterType::FBI
                        ? "FBI"
                        : "Terrorist");

        // 武器資訊
        if (auto *gun = m_Player.GetGun()) {
            ImGui::Separator();
            ImGui::Text("Ammo: %d / %d", gun->GetCurrentAmmo(), gun->GetMagSize());
            ImGui::Text("Reloading: %s", gun->IsReloading() ? "YES" : "no");
            const auto &hit = gun->GetLastHit();
            if (hit.hit) {
                ImGui::Text("LastHit: (%.1f, %.1f, %.1f) d=%.1f",
                            hit.point.x, hit.point.y, hit.point.z, hit.distance);
            }
        }

        // 彈孔效果
        ImGui::Separator();
        ImGui::Text("Bullet Holes: %zu", m_BulletHoles.GetCount());

        ImGui::End();
    }
}

// ============================================================================
//  End() — 清理
// ============================================================================
void App::End() {
    LOG_TRACE("App::End");
    m_Network.Disconnect();
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

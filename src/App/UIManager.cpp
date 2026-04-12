#include "App/UIManager.hpp"
#include "Physics/CapsuleCast.hpp"

#include <GL/glew.h>
#include <cstdio>

namespace App {

void UIManager::RenderMainMenu(Network::NetworkManager& network, float dt) {
    // Clear screen
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Main menu window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 400));

    ImGui::Begin("Counter-Strike LAN", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CS:GO Style FPS - LAN Multiplayer");
    ImGui::Separator();
    ImGui::Spacing();

    // ── Player Name ──
    ImGui::Text("Player Name:");
    ImGui::InputText("##PlayerName", m_MenuState.playerName, sizeof(m_MenuState.playerName));
    ImGui::Spacing();

    ImGui::Separator();

    // ══════════════════════════════════════════════════════════════════════
    // Create Game Section
    // ══════════════════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Host a Game");

    ImGui::Text("Server Name:");
    ImGui::InputText("##ServerName", m_MenuState.serverName, sizeof(m_MenuState.serverName));

    if (ImGui::Button("Create Game", ImVec2(200, 40))) {
        if (m_Callbacks.onCreateGame) {
            m_Callbacks.onCreateGame(m_MenuState.serverName, m_MenuState.playerName);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ══════════════════════════════════════════════════════════════════════
    // Join Game Section
    // ══════════════════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Join a Game");

    // Manual IP input
    ImGui::Text("IP Address:");
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##IPAddress", m_MenuState.ipAddress, sizeof(m_MenuState.ipAddress));
    ImGui::SameLine();
    if (ImGui::Button("Connect")) {
        if (m_Callbacks.onJoinGame) {
            m_Callbacks.onJoinGame(m_MenuState.ipAddress, m_MenuState.playerName);
        }
    }

    // LAN server list
    ImGui::Text("LAN Servers:");
    ImGui::BeginChild("ServerList", ImVec2(0, 80), true);

    const auto& servers = network.GetDiscoveredServers();
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

    // Discovery button
    if (network.IsDiscovering()) {
        if (ImGui::Button("Stop Refresh")) {
            if (m_Callbacks.onStopDiscovery) {
                m_Callbacks.onStopDiscovery();
            }
        }
    } else {
        if (ImGui::Button("Refresh LAN")) {
            if (m_Callbacks.onStartDiscovery) {
                m_Callbacks.onStartDiscovery();
            }
        }
    }

    // Connecting state
    if (m_MenuState.isConnecting) {
        m_MenuState.connectionTimer += dt;
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Connecting...");

        if (m_MenuState.connectionTimer > 5.0f) {
            m_MenuState.isConnecting = false;
            // Connection timeout handled by App
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ── Quit Button ──
    if (ImGui::Button("Quit", ImVec2(100, 30))) {
        if (m_Callbacks.onQuit) {
            m_Callbacks.onQuit();
        }
    }

    ImGui::End();
}

void UIManager::RenderLobby(const Network::NetworkManager& network, size_t remotePlayerCount) {
    // Clear screen
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 250));

    ImGui::Begin("Game Lobby", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (network.IsHost()) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Hosting: %s", network.GetGameName().c_str());
        ImGui::Text("Players: %d/%d", network.GetPlayerCount(), Network::MAX_PLAYERS);
        ImGui::Separator();

        ImGui::Text("Players in lobby:");
        ImGui::BulletText("Host (You)");

        // Show remote players count
        for (size_t i = 0; i < remotePlayerCount; ++i) {
            ImGui::BulletText("Player %zu", i + 1);
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Start Game", ImVec2(150, 40))) {
            if (m_Callbacks.onStartGame) {
                m_Callbacks.onStartGame();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            if (m_Callbacks.onCancel) {
                m_Callbacks.onCancel();
            }
        }
    } else {
        // Client mode
        ImGui::Text("Waiting for connection...");

        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            if (m_Callbacks.onCancel) {
                m_Callbacks.onCancel();
            }
        }
    }

    ImGui::End();
}

void UIManager::RenderHUD(const Entity::Player& player) {
    ImGuiIO& io = ImGui::GetIO();
    const float margin = 20.0f;
    const float hudY = io.DisplaySize.y - margin;

    // Bottom-left: Health
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::SetNextWindowPos(ImVec2(margin, hudY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::Begin("##HUDHealth", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav);
    ImGui::Text("HP: %.0f / %.0f", player.GetHealth(), player.GetMaxHealth());
    ImGui::End();

    // Bottom-right: Ammo
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - margin, hudY), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::Begin("##HUDAmmo", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav);
    if (const auto* gun = player.GetWeapon()) {
        ImGui::Text("Ammo: %d / %d", gun->GetCurrentAmmo(), gun->GetMagSize());
    } else {
        ImGui::Text("Ammo: -- / --");
    }
    ImGui::End();
}

void UIManager::RenderDebugPanel(const Core3D::Camera& camera,
                                  const Entity::Player& player,
                                  const Physics::CollisionMesh& collisionMesh,
                                  const Render::ForwardRenderer::RenderStats& renderStats,
                                  const Network::NetworkManager& network,
                                  size_t remotePlayerCount,
                                  size_t bulletHoleCount,
                                  float dt) {
    if (!m_ShowDebugPanel) return;

    ImGui::Begin("CS Debug");

    auto camPos = camera.GetPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
    ImGui::Text("Yaw: %.1f  Pitch: %.1f", camera.GetYaw(), camera.GetPitch());
    ImGui::Text("FOV: %.1f", camera.GetFOV());
    ImGui::Separator();
    ImGui::Text("VelocityY: %.2f", player.GetVelocityY());
    ImGui::Text("OnGround: %s", player.IsOnGround() ? "true" : "false");

    {
        Physics::Capsule dbgCap = player.MakeCapsule();
        auto dbgGround = Physics::CapsuleCast::SweepVertical(dbgCap, collisionMesh, -6.0f);
        ImGui::Text("GroundY: %.2f", dbgGround.has_value() ? dbgGround.value() : -9999.0f);
    }
    ImGui::Text("Triangles: %zu", collisionMesh.GetTriangleCount());

    ImGui::Separator();
    ImGui::Text("FPS: %.1f", dt > 0.0f ? 1.0f / dt : 0.0f);

    ImGui::Text("Draw Calls: %zu", renderStats.drawCalls);
    ImGui::Text("Nodes: %zu visible / %zu total (culled: %zu)",
                renderStats.visibleNodes, renderStats.totalNodes, renderStats.culledNodes);

    ImGui::Text("[TAB] Toggle cursor  [ESC] Quit");
    ImGui::Text("[V] Toggle model  [C] Switch character");

    // Network status
    ImGui::Separator();
    if (network.IsHost()) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Mode: HOST");
        ImGui::Text("Players: %d", network.GetPlayerCount());
    } else if (network.IsClient()) {
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Mode: CLIENT");
        ImGui::Text("Player ID: %d", network.GetLocalPlayerId());
    } else {
        ImGui::Text("Mode: OFFLINE");
    }
    ImGui::Text("Remote Players: %zu", remotePlayerCount);

    // Character health info
    ImGui::Separator();
    ImGui::Text("Health: %.0f / %.0f", player.GetHealth(), player.GetMaxHealth());
    ImGui::Text("Walking: %s", player.IsWalking() ? "YES" : "no");
    ImGui::Text("Model Visible: %s", player.IsModelVisible() ? "YES" : "no");
    ImGui::Text("Character: %s",
                player.GetCharacterModel().GetCharacterType() ==
                        Entity::CharacterType::FBI
                    ? "FBI"
                    : "Terrorist");

    // Weapon info
    if (auto *gun = player.GetWeapon()) {
        ImGui::Separator();
        ImGui::Text("Ammo: %d / %d", gun->GetCurrentAmmo(), gun->GetMagSize());
        ImGui::Text("Reloading: %s", gun->IsReloading() ? "YES" : "no");
        const auto &hit = gun->GetLastHit();
        if (hit.hit) {
            ImGui::Text("LastHit: (%.1f, %.1f, %.1f) d=%.1f",
                        hit.point.x, hit.point.y, hit.point.z, hit.distance);
        }
    }

    // Bullet hole effects
    ImGui::Separator();
    ImGui::Text("Bullet Holes: %zu", bulletHoleCount);

    ImGui::End();
}

} // namespace App

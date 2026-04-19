#include "App/UIManager.hpp"
#include "Weapon/WeaponDefs.hpp"
#include "Physics/CapsuleCast.hpp"

#include <GL/glew.h>
#include <glm/trigonometric.hpp>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace App {

void UIManager::RenderMainMenu(Network::NetworkManager& network, float dt) {
    // Clear screen
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Main menu window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 450));

    ImGui::Begin("Counter-Strike LAN", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CS Style FPS - LAN Multiplayer");
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

    if (ImGui::Button("Create Game", ImVec2(435, 40))) {
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
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 100.0f) * 0.5f);
    if (ImGui::Button("Quit", ImVec2(100, 30))) {
        if (m_Callbacks.onQuit) {
            m_Callbacks.onQuit();
        }
    }

    ImGui::End();
}

void UIManager::RenderLobby(const Network::NetworkManager& network,
                            const std::vector<LobbyPlayerRow>& players) {
    // Clear screen
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460, 360));

    ImGui::Begin("Game Lobby", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (network.IsHost()) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Hosting: %s", network.GetGameName().c_str());
    } else {
        ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Connected to host - waiting in lobby");
    }
    ImGui::Text("Players: %d/%d", network.GetPlayerCount(), Network::MAX_PLAYERS);
    ImGui::Separator();

    if (ImGui::BeginTable("teams", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("CT Team");
        ImGui::TableSetupColumn("T Team");

        // Colored header row (centered)
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        {
            float avail = ImGui::GetContentRegionAvail().x;
            float tw = ImGui::CalcTextSize("CT Team").x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) * 0.5f);
            ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "CT Team");
        }
        ImGui::TableSetColumnIndex(1);
        {
            float avail = ImGui::GetContentRegionAvail().x;
            float tw = ImGui::CalcTextSize("T Team").x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.4f, 1.0f), "T Team");
        }

        // Player rows (centered)
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        for (const auto& p : players) {
            if (p.teamId != 0) continue;
            char buf[128];
            snprintf(buf, sizeof(buf), "%s%s%s",
                     p.name.c_str(),
                     p.isLocal ? " (You)" : "",
                     p.isHost ? " [Host]" : "");
            float tw = ImGui::CalcTextSize(buf).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                  (ImGui::GetContentRegionAvail().x - tw) * 0.5f);
            ImGui::TextUnformatted(buf);
        }
        ImGui::TableSetColumnIndex(1);
        for (const auto& p : players) {
            if (p.teamId != 1) continue;
            char buf[128];
            snprintf(buf, sizeof(buf), "%s%s%s",
                     p.name.c_str(),
                     p.isLocal ? " (You)" : "",
                     p.isHost ? " [Host]" : "");
            float tw = ImGui::CalcTextSize(buf).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                  (ImGui::GetContentRegionAvail().x - tw) * 0.5f);
            ImGui::TextUnformatted(buf);
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ── Team selection buttons (centered) ──
    {
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float totalW  = 120.0f + spacing + 120.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalW) * 0.5f);
        if (ImGui::Button("Join CT", ImVec2(120, 36))) {
            if (m_Callbacks.onSelectCT) m_Callbacks.onSelectCT();
        }
        ImGui::SameLine();
        if (ImGui::Button("Join T", ImVec2(120, 36))) {
            if (m_Callbacks.onSelectT) m_Callbacks.onSelectT();
        }
    }
    ImGui::Spacing();

    if (network.IsHost()) {
        // ── Host: Start Game + Cancel (centered) ──
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float totalW  = 150.0f + spacing + 100.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalW) * 0.5f);
        if (ImGui::Button("Start Game", ImVec2(150, 40))) {
            if (m_Callbacks.onStartGame) m_Callbacks.onStartGame();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            if (m_Callbacks.onCancel) m_Callbacks.onCancel();
        }
    } else {
        // ── Client: status text + Cancel (centered) ──
        {
            const char* waitMsg = "Waiting for host to start the game...";
            float tw = ImGui::CalcTextSize(waitMsg).x;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - tw) * 0.5f);
            ImGui::TextUnformatted(waitMsg);
        }
        ImGui::Spacing();
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 100.0f) * 0.5f);
        if (ImGui::Button("Cancel", ImVec2(100, 40))) {
            if (m_Callbacks.onCancel) m_Callbacks.onCancel();
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

// ============================================================================
//  RenderCrosshair — Static crosshair + dynamic spread circle
// ============================================================================
void UIManager::RenderCrosshair(const Entity::Player& player) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    float cx = io.DisplaySize.x * 0.5f;
    float cy = io.DisplaySize.y * 0.5f;

    // ── Static crosshair (4 lines with center gap) ──
    const float gap  = 4.0f;
    const float len  = 10.0f;
    const float thick = 2.0f;
    const ImU32 crosshairColor = IM_COL32(0, 255, 60, 220);

    // Top
    drawList->AddLine(ImVec2(cx, cy - gap - len), ImVec2(cx, cy - gap),
                      crosshairColor, thick);
    // Bottom
    drawList->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + gap + len),
                      crosshairColor, thick);
    // Left
    drawList->AddLine(ImVec2(cx - gap - len, cy), ImVec2(cx - gap, cy),
                      crosshairColor, thick);
    // Right
    drawList->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + gap + len, cy),
                      crosshairColor, thick);

    // Center dot
    drawList->AddCircleFilled(ImVec2(cx, cy), 1.5f, crosshairColor);

    // ── Dynamic spread circle ──
    const auto* weapon = player.GetWeapon();
    if (!weapon) return;

    float spreadDeg = weapon->GetSpread().GetCurrentSpread();
    if (spreadDeg <= 0.0f) return;

    // Convert spread angle (degrees) to screen-space radius.
    // Approximate: radius_px = tan(spreadDeg) * (screenHeight / 2) / tan(fov/2)
    // Using a typical vertical FOV of ~45 degrees (matching most FPS cameras).
    const float fovDeg = 45.0f;
    float halfScreenH = io.DisplaySize.y * 0.5f;
    float fovTanHalf  = std::tan(glm::radians(fovDeg * 0.5f));
    float spreadRad   = std::tan(glm::radians(spreadDeg));
    float radius      = (spreadRad / fovTanHalf) * halfScreenH;

    // Clamp radius to reasonable bounds
    const float minRadius = 6.0f;
    const float maxRadius = halfScreenH * 0.8f;
    radius = std::clamp(radius, minRadius, maxRadius);

    // Circle color: transitions from green (min spread) to red (max spread)
    float minS = weapon->GetSpread().GetMinSpread();
    float maxS = weapon->GetSpread().GetMaxSpread();
    float t = (maxS > minS) ? (spreadDeg - minS) / (maxS - minS) : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    int r = static_cast<int>(60  + t * 195);  // 60 → 255
    int g = static_cast<int>(255 - t * 195);  // 255 → 60
    ImU32 circleColor = IM_COL32(r, g, 40, 160);

    drawList->AddCircle(ImVec2(cx, cy), radius, circleColor, 48, 1.5f);
}

// ============================================================================
//  RenderBuyMenu — Weapon purchase overlay
// ============================================================================
void UIManager::RenderBuyMenu(int playerMoney) {
    if (!m_ShowBuyMenu) return;

    const auto& registry = Weapon::GetWeaponRegistry();
    const auto& categories = Weapon::GetAllCategories();

    // ── Window setup: centered, fixed size ──
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(800, 480));
    ImGui::SetNextWindowBgAlpha(0.92f);

    ImGui::Begin("Buy Menu", nullptr,
                 ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoNav);

    // ── Player money display (top-right of window) ──
    {
        char moneyText[32];
        snprintf(moneyText, sizeof(moneyText), "$%d", playerMoney);
        float textWidth = ImGui::CalcTextSize(moneyText).x;
        float windowWidth = ImGui::GetWindowSize().x;
        ImGui::SameLine(windowWidth - textWidth - 20.0f);
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", moneyText);
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ── Category table ──
    const int numCols = static_cast<int>(categories.size());
    const float colWidth = 740.0f / static_cast<float>(numCols);

    if (ImGui::BeginTable("WeaponCategories", numCols,
                          ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
        for (int i = 0; i < numCols; ++i) {
            ImGui::TableSetupColumn(Weapon::GetCategoryName(categories[i]),
                                    ImGuiTableColumnFlags_WidthFixed, colWidth);
        }

        // ── Column headers (colored, centered) ──
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int c = 0; c < numCols; ++c) {
            ImGui::TableSetColumnIndex(c);
            const char* catName = Weapon::GetCategoryName(categories[c]);
            float avail = ImGui::GetContentRegionAvail().x;
            float tw    = ImGui::CalcTextSize(catName).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - tw) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", catName);
        }

        // ── Weapon content row ──
        ImGui::TableNextRow();
        for (int c = 0; c < numCols; ++c) {
            ImGui::TableSetColumnIndex(c);
            ImGui::Separator();

            int weaponIdx = 0;
            for (size_t w = 0; w < registry.size(); ++w) {
                if (registry[w].category != categories[c]) continue;

                const auto& weapon = registry[w];
                bool canAfford = (playerMoney >= weapon.price);

                // Color-coded border
                ImVec4 borderColor = canAfford
                    ? ImVec4(0.1f, 0.9f, 0.4f, 1.0f)   // bright green
                    : ImVec4(0.45f, 0.45f, 0.45f, 1.0f); // grey

                ImVec4 bgColor = canAfford
                    ? ImVec4(0.1f, 0.3f, 0.15f, 0.8f)   // dark green bg
                    : ImVec4(0.2f, 0.2f, 0.2f, 0.6f);    // dark grey bg

                ImVec4 textColor = canAfford
                    ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)     // white
                    : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);     // dim grey

                // Draw weapon entry: InvisibleButton + DrawList for per-line centering
                ImGui::PushID(static_cast<int>(w));

                const float   btnWidth = ImGui::GetContentRegionAvail().x;
                const ImVec2  btnSize(btnWidth, 50.0f);
                const ImVec2  btnPos = ImGui::GetCursorScreenPos();

                ImGui::InvisibleButton("##weapon", btnSize);
                const bool clicked = ImGui::IsItemClicked();
                const bool hovered = ImGui::IsItemHovered();
                const bool active  = ImGui::IsItemActive();

                // Background & border
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImVec2 btnEnd(btnPos.x + btnSize.x, btnPos.y + btnSize.y);
                ImVec4 actualBg = (active  && canAfford) ? ImVec4(0.2f,  0.6f,  0.3f,  1.0f)
                                : (hovered && canAfford) ? ImVec4(0.15f, 0.45f, 0.25f, 0.9f)
                                : bgColor;
                dl->AddRectFilled(btnPos, btnEnd,
                                  ImGui::ColorConvertFloat4ToU32(actualBg), 4.0f);
                dl->AddRect(btnPos, btnEnd,
                            ImGui::ColorConvertFloat4ToU32(borderColor), 4.0f, 0, 2.0f);

                // Weapon name — independently centered on upper half
                {
                    ImVec2 sz = ImGui::CalcTextSize(weapon.name.c_str());
                    ImVec2 pos(btnPos.x + (btnSize.x - sz.x) * 0.5f,
                               btnPos.y + btnSize.y * 0.25f - sz.y * 0.5f);
                    dl->AddText(pos, ImGui::ColorConvertFloat4ToU32(textColor),
                                weapon.name.c_str());
                }

                // Price — independently centered on lower half
                {
                    char priceStr[32];
                    snprintf(priceStr, sizeof(priceStr), "$%d", weapon.price);
                    ImVec2 sz = ImGui::CalcTextSize(priceStr);
                    ImVec2 pos(btnPos.x + (btnSize.x - sz.x) * 0.5f,
                               btnPos.y + btnSize.y * 0.75f - sz.y * 0.5f);
                    dl->AddText(pos, ImGui::ColorConvertFloat4ToU32(textColor), priceStr);
                }

                if (clicked && canAfford && m_Callbacks.onBuyWeapon) {
                    m_Callbacks.onBuyWeapon(static_cast<int>(w));
                }

                ImGui::PopID();

                ImGui::Spacing();
                ++weaponIdx;
            }

            if (weaponIdx == 0) {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 0.7f), "(empty)");
            }
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    {
        const char* hint = "Press [B] to close";
        float tw = ImGui::CalcTextSize(hint).x;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - tw) * 0.5f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", hint);
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

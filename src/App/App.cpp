// ============================================================================
//  App.cpp — CS FPS Game Application (Refactored)
// ============================================================================
//
//  This is the main application coordinator that delegates work to
//  specialized managers for clean separation of concerns.
//
// ============================================================================

#include "App/App.hpp"
#include "Weapon/WeaponDefs.hpp"
#include "Util/Time.hpp"
#include "Util/Logger.hpp"

#include <SDL.h>

namespace App {

Application::Application() {
    SetupUICallbacks();
}

// ============================================================================
//  SetupUICallbacks — Connect UI events to application actions
// ============================================================================
void Application::SetupUICallbacks() {
    UICallbacks callbacks;

    callbacks.onCreateGame = [this](const std::string& serverName, const std::string& playerName) {
        if (m_Network.HostGame(Network::DEFAULT_PORT, serverName, playerName)) {
            m_StateManager.SetState(GameState::LOBBY);
        }
    };

    callbacks.onJoinGame = [this](const std::string& ip, const std::string& playerName) {
        if (m_Network.JoinGame(ip, Network::DEFAULT_PORT, playerName)) {
            m_UIManager.GetMenuState().isConnecting = true;
            m_UIManager.GetMenuState().connectionTimer = 0.0f;
        }
    };

    callbacks.onStartGame = [this]() {
        if (m_Network.IsHost()) {
            m_Network.BroadcastGameStart();
        }
        m_StateManager.SetState(GameState::GAME_START);
    };

    callbacks.onCancel = [this]() {
        m_Network.Disconnect();
        m_StateManager.SetState(GameState::MAIN_MENU);
    };

    callbacks.onQuit = [this]() {
        m_StateManager.SetState(GameState::GAME_END);
    };

    callbacks.onStartDiscovery = [this]() {
        m_Network.StartDiscovery();
    };

    callbacks.onStopDiscovery = [this]() {
        m_Network.StopDiscovery();
    };

    callbacks.onSelectCT = [this]() {
        if (m_StateManager.IsGameActive()) return;
        if (m_Network.IsClient()) {
            m_NetworkController.SendConfig(m_Network, 0, 0);
        } else if (m_Network.IsHost()) {
            m_NetworkController.BroadcastConfig(m_Network, 0, 0, 0);
        }
    };

    callbacks.onSelectT = [this]() {
        if (m_StateManager.IsGameActive()) return;
        if (m_Network.IsClient()) {
            m_NetworkController.SendConfig(m_Network, 1, 0);
        } else if (m_Network.IsHost()) {
            m_NetworkController.BroadcastConfig(m_Network, 0, 1, 0);
        }
    };

    callbacks.onBuyWeapon = [this](int weaponIndex) {
        const auto& registry = Weapon::GetWeaponRegistry();
        if (weaponIndex < 0 || weaponIndex >= static_cast<int>(registry.size())) return;

        const auto& info = registry[weaponIndex];
        auto& player = m_GameManager.GetPlayer();

        if (player.GetMoney() < info.price) return;

        auto weapon = info.factory();
        if (!weapon) return;

        player.SpendMoney(info.price);
        player.EquipWeapon(std::move(weapon), m_GameManager.GetScene());

        // Broadcast weapon change to other players
        uint8_t charTypeId = m_GameManager.GetCharacterTypeId();
        uint8_t gunTypeId = static_cast<uint8_t>(weaponIndex);
        if (m_Network.IsClient()) {
            m_NetworkController.SendConfig(m_Network, charTypeId, gunTypeId);
        } else if (m_Network.IsHost()) {
            m_NetworkController.BroadcastConfig(m_Network, 0, charTypeId, gunTypeId);
        }

        // Close buy menu after purchase
        m_UIManager.SetBuyMenuVisible(false);
        m_InputManager.LockCursor();
    };

    m_UIManager.SetCallbacks(std::move(callbacks));
}

// ============================================================================
//  SetupNetworkCallbacks — Connect network events to game logic
// ============================================================================
void Application::SetupNetworkCallbacks() {
    auto bulletEffectCallback = [this](const glm::vec3& pos, const glm::vec3& normal) {
        m_GameManager.SpawnBulletHole(pos, normal);
    };

    m_NetworkController.SetupCallbacks(
        m_Network,
        m_GameManager.GetScene(),
        m_GameManager.GetPlayer(),
        m_GameManager.GetRemotePlayers(),
        m_StateManager,
        bulletEffectCallback
    );
}

// ============================================================================
//  MainMenu — Main Menu state
// ============================================================================
void Application::MainMenu() {
    // Release cursor
    if (m_InputManager.IsCursorLocked()) {
        m_InputManager.UnlockCursor();
    }

    // Setup network callbacks once
    if (!m_CallbacksInitialized) {
        SetupNetworkCallbacks();
        m_CallbacksInitialized = true;
    }

    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_Network.Update(dt);

    m_UIManager.RenderMainMenu(m_Network, dt);

    // Handle connection timeout
    auto& menuState = m_UIManager.GetMenuState();
    if (menuState.isConnecting && menuState.connectionTimer > 5.0f) {
        menuState.isConnecting = false;
        m_Network.Disconnect();
    }
}

// ============================================================================
//  Lobby — Lobby state
// ============================================================================
void Application::Lobby() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_Network.Update(dt);

    std::vector<UIManager::LobbyPlayerRow> rows;
    const auto lobbyPlayers = m_Network.GetLobbyPlayers();
    rows.reserve(lobbyPlayers.size());
    for (const auto& p : lobbyPlayers) {
        rows.push_back(UIManager::LobbyPlayerRow{
            p.name, p.teamId, p.isLocal, p.isHost
        });
    }

    m_UIManager.RenderLobby(m_Network, rows);
}

// ============================================================================
//  Start — Game initialization state
// ============================================================================
void Application::Start() {
    LOG_TRACE("Application::Start");

    // Initialize game
    m_GameManager.SetLocalCharacterType(
        (m_Network.GetLocalCharacterType() == 0)
            ? Entity::CharacterType::FBI
            : Entity::CharacterType::TERRORIST
    );
    m_GameManager.Initialize();

    // Lock cursor for FPS mode
    m_InputManager.LockCursor();

    // Send character config to other players
    SendCharacterConfig();

    // Transition to game update
    m_StateManager.SetState(GameState::GAME_UPDATE);

    LOG_TRACE("Application::Start complete");
}

// ============================================================================
//  Update — Main game loop state
// ============================================================================
void Application::Update() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    auto& camera = m_GameManager.GetCamera();
    auto& player = m_GameManager.GetPlayer();

    // ── Exit Check ──
    if (m_InputManager.IsExitRequested()) {
        m_Network.Disconnect();
        m_StateManager.SetState(GameState::GAME_END);
        return;
    }

    // ── TAB: Toggle cursor lock / Debug panel ──
    if (m_InputManager.IsTabPressed()) {
        m_InputManager.ToggleCursor();
        m_UIManager.SetDebugPanelVisible(!m_InputManager.IsCursorLocked());
    }

    // ── V: Toggle character model visibility ──
    if (m_InputManager.IsToggleModelPressed()) {
        player.ToggleModelVisibility();
    }

    // ── B: Toggle buy menu ──
    if (m_InputManager.IsBuyMenuPressed()) {
        m_UIManager.ToggleBuyMenu();
        if (m_UIManager.IsBuyMenuVisible()) {
            m_InputManager.UnlockCursor();
        } else {
            m_InputManager.LockCursor();
        }
    }

    // ── C: Switch character type ──
    if (m_InputManager.IsSwitchCharacterPressed()) {
        HandleCharacterSwitch();
    }

    // ══════════════════════════════════════════════════════════════════════
    // Network Update
    // ══════════════════════════════════════════════════════════════════════
    if (m_Network.IsHost()) {
        m_NetworkController.UpdateHost(
            dt, m_Network, player, camera,
            m_GameManager.GetRemotePlayers()
        );
    } else if (m_Network.IsClient()) {
        auto inputState = m_InputManager.SampleInput(
            camera, player.GetPosition(),
            player.IsWalking(), player.IsOnGround(), player.IsCrouching()
        );
        m_NetworkController.UpdateClient(
            dt, m_Network, inputState,
            m_GameManager.GetRemotePlayers()
        );
    }

    // ── Player Movement + Physics ──
    m_GameManager.UpdatePlayer(dt);

    // ── Check bullet hit and spawn bullet holes ──
    HandleBulletHit();

    // ── Check player death and respawn ──
    m_CombatManager.CheckLocalRespawn(player, camera, m_GameManager.GetCollisionMesh());

    if (m_Network.IsHost()) {
        m_CombatManager.CheckRemoteRespawns(
            m_GameManager.GetRemotePlayers(),
            m_GameManager.GetCollisionMesh()
        );
    }

    // ── Update effects ──
    m_GameManager.UpdateEffects(dt);

    // ── Mouse view (FPS locked mode) ──
    m_InputManager.ProcessMouseLook(camera);

    // ── Render ──
    m_GameManager.Render();
    m_GameManager.DrawEffects();
    m_UIManager.RenderHUD(player);

    // ── Buy Menu ──
    m_UIManager.RenderBuyMenu(player.GetMoney());

    // ── Debug Panel ──
    m_UIManager.RenderDebugPanel(
        camera, player,
        m_GameManager.GetCollisionMesh(),
        m_GameManager.GetRenderStats(),
        m_Network,
        m_GameManager.GetRemotePlayers().size(),
        m_GameManager.GetBulletHoleCount(),
        dt
    );
}

// ============================================================================
//  End — Cleanup state
// ============================================================================
void Application::End() {
    LOG_TRACE("Application::End");
    m_Network.Disconnect();
    m_GameManager.Cleanup();
}

// ============================================================================
//  HandleCharacterSwitch — Switch character and notify network
// ============================================================================
void Application::HandleCharacterSwitch() {
    // Team selection is limited to lobby via UI buttons.
    if (m_StateManager.IsGameActive()) {
        return;
    }

    auto& player = m_GameManager.GetPlayer();
    auto currentType = player.GetCharacterModel().GetCharacterType();
    auto newType = (currentType == Entity::CharacterType::FBI)
                       ? Entity::CharacterType::TERRORIST
                       : Entity::CharacterType::FBI;

    m_GameManager.SwitchPlayerCharacter(newType);

    uint8_t charTypeId = (newType == Entity::CharacterType::FBI) ? 0 : 1;
    if (m_Network.IsClient()) {
        m_NetworkController.SendConfig(m_Network, charTypeId, 0);
    } else if (m_Network.IsHost()) {
        m_NetworkController.BroadcastConfig(m_Network, 0, charTypeId, 0);
    }
}

// ============================================================================
//  HandleBulletHit — Process bullet hits for effects and damage
// ============================================================================
void Application::HandleBulletHit() {
    auto& player = m_GameManager.GetPlayer();
    auto* gun = player.GetWeapon();
    if (!gun) return;

    const auto& mapHit = gun->GetLastHit();
    static glm::vec3 lastHitPoint(0.0f);

    if (mapHit.hit && mapHit.point != lastHitPoint) {
        lastHitPoint = mapHit.point;

        auto& camera = m_GameManager.GetCamera();
        auto playerHit = m_CombatManager.CheckPlayerHit(
            camera.GetPosition(),
            camera.GetFront(),
            mapHit.distance,
            m_GameManager.GetRemotePlayers()
        );

        if (playerHit.hit && playerHit.distance < mapHit.distance) {
            // Hit a player
            m_CombatManager.HandleDamage(
                playerHit.playerId, gun->GetDamage(), playerHit.point,
                m_Network, m_GameManager.GetRemotePlayers()
            );
        } else {
            // Hit the map
            m_GameManager.SpawnBulletHole(mapHit.point, mapHit.normal);

            if (m_Network.IsHost()) {
                m_Network.BroadcastBulletEffect(mapHit.point, mapHit.normal);
            } else if (m_Network.IsClient()) {
                m_Network.SendBulletEffect(mapHit.point, mapHit.normal);
            }
        }
    }
}

// ============================================================================
//  SendCharacterConfig — Send character config to network
// ============================================================================
void Application::SendCharacterConfig() {
    uint8_t charTypeId = m_Network.GetLocalCharacterType();
    uint8_t gunTypeId  = m_GameManager.GetWeaponTypeId();

    if (m_Network.IsClient()) {
        m_NetworkController.SendConfig(m_Network, charTypeId, gunTypeId);
    } else if (m_Network.IsHost()) {
        m_NetworkController.BroadcastConfig(m_Network, 0, charTypeId, gunTypeId);
    }
}

} // namespace App

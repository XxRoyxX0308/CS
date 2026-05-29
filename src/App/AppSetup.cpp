// ============================================================================
//  AppSetup.cpp — Application setup and lifecycle flow
// ============================================================================

#include "App/App.hpp"

#include "Util/Logger.hpp"
#include "Util/Time.hpp"
#include "Weapon/WeaponDefs.hpp"

namespace App {

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

        uint8_t charTypeId = m_GameManager.GetCharacterTypeId();
        uint8_t gunTypeId = static_cast<uint8_t>(weaponIndex);
        if (m_Network.IsClient()) {
            m_NetworkController.SendConfig(m_Network, charTypeId, gunTypeId);
        } else if (m_Network.IsHost()) {
            m_NetworkController.BroadcastConfig(m_Network, 0, charTypeId, gunTypeId);
        }

        m_UIManager.SetBuyMenuVisible(false);
        m_InputManager.LockCursor();
    };

    callbacks.onAddBotCT = [this]() {
        if (m_CTBotCount < 5) {
            ++m_CTBotCount;
            m_UIManager.SetCTBotCount(m_CTBotCount);
        }
    };
    callbacks.onRemoveBotCT = [this]() {
        if (m_CTBotCount > 0) {
            --m_CTBotCount;
            m_UIManager.SetCTBotCount(m_CTBotCount);
        }
    };
    callbacks.onAddBotT = [this]() {
        if (m_TBotCount < 5) {
            ++m_TBotCount;
            m_UIManager.SetTBotCount(m_TBotCount);
        }
    };
    callbacks.onRemoveBotT = [this]() {
        if (m_TBotCount > 0) {
            --m_TBotCount;
            m_UIManager.SetTBotCount(m_TBotCount);
        }
    };
    callbacks.onSetBotsFollowPlayerNoAttack = [this](bool enabled) {
        m_GameManager.SetBotsFollowPlayerNoAttack(enabled);
    };

    m_UIManager.SetCallbacks(std::move(callbacks));
}

void Application::SetupNetworkCallbacks() {
    auto bulletEffectCallback = [this](const glm::vec3& pos, const glm::vec3& normal) {
        m_GameManager.SpawnBulletHole(pos, normal);
    };

    auto authoritativeKillCallback = [this](uint8_t killerId, uint8_t victimId) {
        RecordKill(killerId, victimId);
    };

    m_NetworkController.SetupCallbacks(
        m_Network,
        m_GameManager.GetScene(),
        m_GameManager.GetPlayer(),
        m_GameManager.GetRemotePlayers(),
        m_StateManager,
        bulletEffectCallback,
        authoritativeKillCallback
    );

    m_Network.SetOnClientMatchKill([this](uint8_t killerId, uint8_t victimId) {
        RecordKill(killerId, victimId);
    });

    m_Network.SetOnReturnToLobby([this]() {
        ReturnToLobby(false);
    });
}

void Application::MainMenu() {
    if (m_InputManager.IsCursorLocked()) {
        m_InputManager.UnlockCursor();
    }

    if (!m_CallbacksInitialized) {
        SetupNetworkCallbacks();
        m_CallbacksInitialized = true;
    }

    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_Network.Update(dt);

    m_UIManager.RenderMainMenu(m_Network, dt);

    auto& menuState = m_UIManager.GetMenuState();
    if (menuState.isConnecting && menuState.connectionTimer > 5.0f) {
        menuState.isConnecting = false;
        m_Network.Disconnect();
    }
}

void Application::Lobby() {
    const float dt = static_cast<float>(Util::Time::GetDeltaTimeMs()) / 1000.0f;
    m_Network.Update(dt);

    std::vector<UIManager::LobbyPlayerRow> rows;
    const auto lobbyPlayers = m_Network.GetLobbyPlayers();
    rows.reserve(lobbyPlayers.size() + m_CTBotCount + m_TBotCount);
    for (const auto& p : lobbyPlayers) {
        rows.push_back(UIManager::LobbyPlayerRow{
            p.name, p.teamId, p.isLocal, p.isHost
        });
    }

    for (int i = 0; i < m_CTBotCount; ++i) {
        rows.push_back(UIManager::LobbyPlayerRow{
            "Bot CT " + std::to_string(i + 1), 0, false, false, true
        });
    }
    for (int i = 0; i < m_TBotCount; ++i) {
        rows.push_back(UIManager::LobbyPlayerRow{
            "Bot T " + std::to_string(i + 1), 1, false, false, true
        });
    }

    m_UIManager.RenderLobby(m_Network, rows);
}

void Application::Start() {
    LOG_TRACE("Application::Start");

    m_GameManager.SetLocalCharacterType(
        (m_Network.GetLocalCharacterType() == 0)
            ? Entity::CharacterType::FBI
            : Entity::CharacterType::TERRORIST
    );
    m_GameManager.Initialize();
    m_GameManager.InitializeBots(m_CTBotCount, m_TBotCount);
    InitializeMatchState();

    m_InputManager.LockCursor();
    SendCharacterConfig();

    m_StateManager.SetState(GameState::GAME_UPDATE);

    LOG_TRACE("Application::Start complete");
}

void Application::End() {
    LOG_TRACE("Application::End");
    m_Network.Disconnect();
    m_GameManager.Cleanup();
    ResetMatchState();
}

void Application::HandleCharacterSwitch() {
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
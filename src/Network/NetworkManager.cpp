// Must be first to avoid Windows header conflicts
#ifdef _WIN32
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#include "Network/NetworkManager.hpp"
#include <Util/Logger.hpp>

namespace Network {

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() {
    Disconnect();
}

bool NetworkManager::HostGame(uint16_t port, const std::string& gameName, const std::string& hostPlayerName) {
    Disconnect();

    m_Server = std::make_unique<GameServer>();
    SetupServerCallbacks();

    if (!m_Server->Start(port, gameName, hostPlayerName)) {
        m_Server.reset();
        return false;
    }

    m_Mode = NetworkMode::Host;
    m_LocalPlayerId = 0;  // Host is always player 0
    m_LocalCharacterType = 0;
    m_LocalPlayerName = hostPlayerName;

    LOG_INFO("Hosting game: {} on port {} as {}", gameName, port, hostPlayerName);
    return true;
}

bool NetworkManager::JoinGame(const std::string& address, uint16_t port, const std::string& playerName) {
    Disconnect();

    m_Client = std::make_unique<GameClient>();
    SetupClientCallbacks();

    if (!m_Client->Connect(address, port, playerName)) {
        m_Client.reset();
        return false;
    }

    m_Mode = NetworkMode::Client;
    m_LocalPlayerId = 0xFF;  // Will be assigned by server
    m_LocalCharacterType = 0;
    m_LocalPlayerName = playerName;

    LOG_INFO("Joining game at {}:{} as {}", address, port, playerName);
    return true;
}

void NetworkManager::Disconnect() {
    if (m_Server) {
        m_Server->Stop();
        m_Server.reset();
    }

    if (m_Client) {
        m_Client->Disconnect();
        m_Client.reset();
    }

    m_Mode = NetworkMode::Offline;
    m_LocalPlayerId = 0;
    m_LocalCharacterType = 0;
    m_LocalPlayerName.clear();
    m_ClientLobbyPlayers.clear();
    m_InputSequence = 0;
}

bool NetworkManager::IsConnected() const {
    if (m_Mode == NetworkMode::Host) {
        return m_Server && m_Server->IsRunning();
    } else if (m_Mode == NetworkMode::Client) {
        return m_Client && m_Client->IsConnected();
    }
    return false;
}

void NetworkManager::Update(float dt) {
    if (m_Server) {
        m_Server->Update(dt);
    }

    if (m_Client) {
        m_Client->Update(dt);

        // Update local player ID when connected
        if (m_Client->IsConnected() && m_LocalPlayerId == 0xFF) {
            m_LocalPlayerId = m_Client->GetLocalPlayerId();
        }
    }

    // Update discovery if active
    if (m_Mode == NetworkMode::Offline && m_Discovery.IsDiscovering()) {
        m_Discovery.UpdateClient(dt);
    }
}

void NetworkManager::SendInput(const InputState& input) {
    if (m_Client && m_Client->IsConnected()) {
        ++m_InputSequence;
        m_Client->SendInput(input);
    }
}

std::vector<PendingInput> NetworkManager::GetPendingInputs() {
    if (m_Server) {
        return m_Server->GetPendingInputs();
    }
    return {};
}

std::optional<NetPlayerState> NetworkManager::GetInterpolatedState(uint8_t playerId) const {
    if (m_Client) {
        return m_Client->GetInterpolatedState(playerId, m_Client->GetRenderTime());
    }
    return std::nullopt;
}

std::optional<NetPlayerState> NetworkManager::GetLocalPlayerState() const {
    if (m_Client) {
        return m_Client->GetLocalPlayerState();
    }
    return std::nullopt;
}

uint32_t NetworkManager::GetLastAckedInput() const {
    if (m_Client) {
        return m_Client->GetLastAckedInput();
    }
    return 0;
}

float NetworkManager::GetRenderTime() const {
    if (m_Client) {
        return m_Client->GetRenderTime();
    }
    return 0.0f;
}

std::vector<uint8_t> NetworkManager::GetRemotePlayerIds() const {
    if (m_Client) {
        return m_Client->GetRemotePlayerIds();
    } else if (m_Server) {
        std::vector<uint8_t> ids;
        for (const auto& [playerId, client] : m_Server->GetClients()) {
            ids.push_back(playerId);
        }
        return ids;
    }
    return {};
}

void NetworkManager::BroadcastGameState(const NetPlayerState* players, uint8_t playerCount) {
    if (m_Server) {
        m_Server->BroadcastGameState(players, playerCount);
    }
}

void NetworkManager::BroadcastPlayerHit(uint8_t victimId, uint8_t attackerId,
                                         uint8_t newHealth, const glm::vec3& hitPos) {
    if (m_Server) {
        m_Server->BroadcastPlayerHit(victimId, attackerId, newHealth, hitPos);
    }
}

void NetworkManager::BroadcastPlayerDeath(uint8_t victimId, uint8_t killerId) {
    if (m_Server) {
        m_Server->BroadcastPlayerDeath(victimId, killerId);
    }
}

void NetworkManager::BroadcastBulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    if (m_Server) {
        m_Server->BroadcastBulletEffect(pos, normal);
    }
}

void NetworkManager::BroadcastGameStart() {
    if (m_Server) {
        m_Server->BroadcastGameStart();
    }
}

void NetworkManager::SendBulletEffect(const glm::vec3& pos, const glm::vec3& normal) {
    if (m_Client) {
        m_Client->SendBulletEffect(pos, normal);
    }
}

void NetworkManager::SendPlayerConfig(uint8_t characterType, uint8_t gunType) {
    if (m_Client) {
        m_LocalCharacterType = characterType;
        m_Client->SendPlayerConfig(characterType, gunType);
    }
}

void NetworkManager::SendPlayerHit(uint8_t victimId, float damage, const glm::vec3& hitPos) {
    if (m_Client) {
        m_Client->SendPlayerHit(victimId, damage, hitPos);
    }
}

void NetworkManager::BroadcastPlayerConfig(uint8_t playerId, uint8_t characterType, uint8_t gunType) {
    if (m_Server) {
        if (playerId == 0) {
            m_LocalCharacterType = characterType;
        }
        m_Server->BroadcastPlayerConfig(playerId, characterType, gunType);
    }
}

void NetworkManager::StartDiscovery() {
    if (m_Client) {
        m_Client->StartDiscovery();
    } else {
        m_Discovery.StartDiscovery();
    }
}

void NetworkManager::StopDiscovery() {
    if (m_Client) {
        m_Client->StopDiscovery();
    } else {
        m_Discovery.StopDiscovery();
    }
}

bool NetworkManager::IsDiscovering() const {
    if (m_Client) {
        return m_Client->GetDiscoveredServers().size() > 0 || m_Discovery.IsDiscovering();
    }
    return m_Discovery.IsDiscovering();
}

const std::vector<ServerInfo>& NetworkManager::GetDiscoveredServers() const {
    if (m_Client) {
        return m_Client->GetDiscoveredServers();
    }
    return m_Discovery.GetDiscoveredServers();
}

uint8_t NetworkManager::GetPlayerCount() const {
    if (m_Server) {
        return m_Server->GetClientCount() + 1;  // +1 for host
    }
    if (m_Client) {
        return static_cast<uint8_t>(m_ClientLobbyPlayers.size());
    }
    return 1;
}

const std::string& NetworkManager::GetGameName() const {
    static const std::string empty;
    if (m_Server) {
        return m_Server->GetGameName();
    }
    return empty;
}

std::vector<NetworkManager::LobbyPlayerInfo> NetworkManager::GetLobbyPlayers() const {
    std::vector<LobbyPlayerInfo> players;
    if (m_Server) {
        const auto serverPlayers = m_Server->GetLobbyPlayers();
        players.reserve(serverPlayers.size());
        for (const auto& p : serverPlayers) {
            players.push_back(LobbyPlayerInfo{
                p.playerId, p.name, p.teamId, p.characterType, p.isHost, p.playerId == m_LocalPlayerId
            });
        }
        return players;
    }

    if (m_Client) {
        players.reserve(m_ClientLobbyPlayers.size());
        for (const auto& [_, p] : m_ClientLobbyPlayers) {
            players.push_back(p);
        }
        return players;
    }
    return players;
}

void NetworkManager::SetupServerCallbacks() {
    if (!m_Server) return;

    m_Server->SetOnPlayerJoined([this](uint8_t playerId, const std::string& name) {
        if (m_OnPlayerJoined) {
            m_OnPlayerJoined(playerId, name);
        }
    });

    m_Server->SetOnPlayerLeft([this](uint8_t playerId) {
        if (m_OnPlayerLeft) {
            m_OnPlayerLeft(playerId);
        }
    });

    m_Server->SetOnBulletEffect([this](const glm::vec3& pos, const glm::vec3& normal) {
        if (m_OnBulletEffect) {
            m_OnBulletEffect(pos, normal);
        }
    });

    m_Server->SetOnPlayerConfig([this](uint8_t playerId, uint8_t characterType, uint8_t gunType) {
        if (playerId == 0) {
            m_LocalCharacterType = characterType;
        }
        if (m_OnPlayerConfig) {
            m_OnPlayerConfig(playerId, characterType, gunType);
        }
    });

    m_Server->SetOnPlayerHit([this](uint8_t attackerId, uint8_t victimId, float damage, const glm::vec3& hitPos) {
        if (m_OnClientPlayerHit) {
            m_OnClientPlayerHit(attackerId, victimId, damage, hitPos);
        }
    });
}

void NetworkManager::SetupClientCallbacks() {
    if (!m_Client) return;

    m_Client->SetOnConnected([this](uint8_t playerId) {
        m_LocalPlayerId = playerId;
        m_ClientLobbyPlayers[playerId].playerId = playerId;
        m_ClientLobbyPlayers[playerId].isLocal = true;
        m_ClientLobbyPlayers[playerId].name = m_LocalPlayerName;
        if (m_OnConnected) {
            m_OnConnected(playerId);
        }
    });

    m_Client->SetOnDisconnected([this]() {
        m_Mode = NetworkMode::Offline;
        m_LocalPlayerId = 0xFF;
        m_ClientLobbyPlayers.clear();
        if (m_OnDisconnected) {
            m_OnDisconnected();
        }
    });

    m_Client->SetOnPlayerJoined([this](uint8_t playerId, const std::string& name) {
        auto& p = m_ClientLobbyPlayers[playerId];
        p.playerId = playerId;
        p.name = name;
        p.isHost = (playerId == 0);
        p.isLocal = (playerId == m_LocalPlayerId);
        if (m_OnPlayerJoined) {
            m_OnPlayerJoined(playerId, name);
        }
    });

    m_Client->SetOnPlayerLeft([this](uint8_t playerId) {
        m_ClientLobbyPlayers.erase(playerId);
        if (m_OnPlayerLeft) {
            m_OnPlayerLeft(playerId);
        }
    });

    m_Client->SetOnPlayerHit([this](uint8_t victimId, uint8_t attackerId,
                                     uint8_t newHealth, const glm::vec3& hitPos) {
        if (m_OnPlayerHit) {
            m_OnPlayerHit(victimId, attackerId, newHealth, hitPos);
        }
    });

    m_Client->SetOnPlayerDeath([this](uint8_t victimId, uint8_t killerId) {
        if (m_OnPlayerDeath) {
            m_OnPlayerDeath(victimId, killerId);
        }
    });

    m_Client->SetOnBulletEffect([this](const glm::vec3& pos, const glm::vec3& normal) {
        if (m_OnBulletEffect) {
            m_OnBulletEffect(pos, normal);
        }
    });

    m_Client->SetOnPlayerConfig([this](uint8_t playerId, uint8_t characterType, uint8_t gunType) {
        auto& p = m_ClientLobbyPlayers[playerId];
        p.playerId = playerId;
        if (p.name.empty() && playerId == m_LocalPlayerId) {
            p.name = m_LocalPlayerName;
        }
        p.teamId = (characterType == 0) ? 0 : 1;
        p.characterType = characterType;
        p.isHost = (playerId == 0);
        p.isLocal = (playerId == m_LocalPlayerId);
        if (playerId == m_LocalPlayerId) {
            m_LocalCharacterType = characterType;
        }
        if (m_OnPlayerConfig) {
            m_OnPlayerConfig(playerId, characterType, gunType);
        }
    });

    m_Client->SetOnGameStart([this]() {
        if (m_OnGameStart) {
            m_OnGameStart();
        }
    });
}

} // namespace Network

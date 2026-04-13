#include "App/NetworkController.hpp"
#include "App/StateManager.hpp"
#include "Util/Logger.hpp"

namespace App {

void NetworkController::SetupCallbacks(
    Network::NetworkManager& network,
    Scene::SceneGraph& scene,
    Entity::Player& player,
    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers,
    StateManager& stateManager,
    BulletEffectCallback bulletEffectCallback) {

    network.SetOnPlayerJoined([&scene, &remotePlayers](uint8_t playerId, const std::string& name) {
        LOG_INFO("Player {} ({}) joined the game", name, playerId);

        auto& remote = remotePlayers[playerId];
        remote.SetPlayerId(playerId);
        remote.Init(scene, Entity::CharacterType::TERRORIST);
    });

    network.SetOnPlayerLeft([&remotePlayers](uint8_t playerId) {
        LOG_INFO("Player {} left the game", playerId);

        auto it = remotePlayers.find(playerId);
        if (it != remotePlayers.end()) {
            it->second.SetVisible(false);
            remotePlayers.erase(it);
        }
    });

    network.SetOnConnected([&stateManager](uint8_t playerId) {
        LOG_INFO("Connected to server as player {}", playerId);
        stateManager.SetState(GameState::LOBBY);
    });

    network.SetOnDisconnected([&stateManager, &remotePlayers]() {
        LOG_INFO("Disconnected from server");
        remotePlayers.clear();
        stateManager.SetState(GameState::MAIN_MENU);
    });

    network.SetOnBulletEffect([bulletEffectCallback](const glm::vec3& pos, const glm::vec3& normal) {
        if (bulletEffectCallback) {
            bulletEffectCallback(pos, normal);
        }
    });

    network.SetOnPlayerConfig([&scene, &remotePlayers](uint8_t playerId, uint8_t characterType, uint8_t /*gunType*/) {
        LOG_INFO("Player {} changed to character type {}", playerId, characterType);

        auto it = remotePlayers.find(playerId);
        if (it != remotePlayers.end()) {
            auto type = (characterType == 0) ? Entity::CharacterType::FBI
                                              : Entity::CharacterType::TERRORIST;
            it->second.Init(scene, type);
        }
    });

    network.SetOnGameStart([&stateManager]() {
        LOG_INFO("Host started game");
        stateManager.SetState(GameState::GAME_START);
    });

    // Host receives hit reports from clients
    network.SetOnClientPlayerHit([&player, &remotePlayers, &network](
        uint8_t attackerId, uint8_t victimId, float damage, const glm::vec3& hitPos) {

        LOG_INFO("Server received hit report: player {} hit player {} for {} damage",
                 attackerId, victimId, damage);

        if (victimId == 0) {
            // Host was hit
            bool stillAlive = player.TakeDamage(damage);
            uint8_t newHealth = static_cast<uint8_t>(player.GetHealth());
            network.BroadcastPlayerHit(victimId, attackerId, newHealth, hitPos);

            if (!stillAlive) {
                LOG_INFO("Host (player 0) was killed by player {}!", attackerId);
                network.BroadcastPlayerDeath(victimId, attackerId);
            }
        } else {
            auto it = remotePlayers.find(victimId);
            if (it != remotePlayers.end()) {
                auto& victim = it->second;
                bool stillAlive = victim.TakeDamage(damage);
                uint8_t newHealth = static_cast<uint8_t>(victim.GetHealth());
                network.BroadcastPlayerHit(victimId, attackerId, newHealth, hitPos);

                if (!stillAlive) {
                    LOG_INFO("Player {} was killed by player {}!", victimId, attackerId);
                    network.BroadcastPlayerDeath(victimId, attackerId);
                }
            }
        }
    });

    // Client receives hit notifications
    network.SetOnPlayerHit([&player, &remotePlayers, &network](
        uint8_t victimId, uint8_t attackerId, uint8_t newHealth, const glm::vec3& /*hitPos*/) {

        LOG_INFO("Player {} was hit by player {}, new health: {}", victimId, attackerId, newHealth);

        if (victimId == network.GetLocalPlayerId()) {
            player.SetHealth(static_cast<float>(newHealth));
            LOG_INFO("We were hit! Health now: {}", newHealth);
        } else {
            auto it = remotePlayers.find(victimId);
            if (it != remotePlayers.end()) {
                it->second.SetHealth(static_cast<float>(newHealth));
            }
        }
    });

    // Client receives death notifications
    network.SetOnPlayerDeath([&player, &remotePlayers, &network](uint8_t victimId, uint8_t killerId) {
        LOG_INFO("Player {} was killed by player {}!", victimId, killerId);

        if (victimId == network.GetLocalPlayerId()) {
            player.SetHealth(0.0f);
            LOG_INFO("We died! Respawning...");
        } else {
            auto it = remotePlayers.find(victimId);
            if (it != remotePlayers.end()) {
                it->second.SetHealth(0.0f);
            }
        }
    });
}

void NetworkController::UpdateHost(
    float dt,
    Network::NetworkManager& network,
    const Entity::Player& player,
    const Core3D::Camera& camera,
    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) {

    network.Update(dt);
    ProcessRemoteInputs(network, remotePlayers);

    for (auto& [playerId, remote] : remotePlayers) {
        remote.Update(dt);
    }

    BuildAndBroadcastGameState(network, player, camera, remotePlayers);
}

void NetworkController::UpdateClient(
    float dt,
    Network::NetworkManager& network,
    const Network::InputState& inputState,
    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) {

    network.Update(dt);
    network.SendInput(inputState);

    for (auto& [playerId, remote] : remotePlayers) {
        auto state = network.GetInterpolatedState(playerId);
        if (state) {
            remote.UpdateFromNetworkState(*state, dt);
        }
    }
}

void NetworkController::SendConfig(Network::NetworkManager& network,
                                    uint8_t characterType, uint8_t gunType) {
    if (network.IsClient()) {
        network.SendPlayerConfig(characterType, gunType);
    }
}

void NetworkController::BroadcastConfig(Network::NetworkManager& network,
                                         uint8_t playerId,
                                         uint8_t characterType,
                                         uint8_t gunType) {
    if (network.IsHost()) {
        network.BroadcastPlayerConfig(playerId, characterType, gunType);
    }
}

void NetworkController::BuildAndBroadcastGameState(
    Network::NetworkManager& network,
    const Entity::Player& player,
    const Core3D::Camera& camera,
    const std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) {

    if (!network.IsHost()) return;

    std::vector<Network::NetPlayerState> states;

    // Host player (player 0)
    Network::NetPlayerState hostState;
    hostState.playerId = 0;
    hostState.SetPosition(player.GetPosition());
    hostState.yaw = camera.GetYaw();
    hostState.pitch = camera.GetPitch();
    hostState.velocityY = player.GetVelocityY();
    hostState.health = static_cast<uint8_t>(player.GetHealth());
    hostState.currentAmmo = player.GetWeapon() ? player.GetWeapon()->GetCurrentAmmo() : 0;

    hostState.flags = 0;
    if (player.IsOnGround()) hostState.flags |= Network::FLAG_ON_GROUND;
    if (player.GetWeapon() && player.GetWeapon()->IsReloading()) hostState.flags |= Network::FLAG_IS_RELOADING;
    hostState.flags |= Network::FLAG_IS_ALIVE;
    if (player.IsWalking()) hostState.flags |= Network::FLAG_IS_WALKING;

    states.push_back(hostState);

    // Remote players
    for (const auto& [playerId, remote] : remotePlayers) {
        Network::NetPlayerState state;
        state.playerId = playerId;
        state.SetPosition(remote.GetPosition());
        state.yaw = remote.GetYaw();
        state.pitch = remote.GetPitch();
        state.health = static_cast<uint8_t>(remote.GetHealth());
        state.flags = remote.IsAlive() ? Network::FLAG_IS_ALIVE : 0;
        states.push_back(state);
    }

    network.BroadcastGameState(states.data(), static_cast<uint8_t>(states.size()));
}

void NetworkController::ProcessRemoteInputs(
    Network::NetworkManager& network,
    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) {

    auto inputs = network.GetPendingInputs();

    for (const auto& pending : inputs) {
        auto it = remotePlayers.find(pending.playerId);
        if (it == remotePlayers.end()) continue;

        auto& remote = it->second;
        const auto& input = pending.input;

        glm::vec3 position(input.posX, input.posY, input.posZ);
        bool isWalking = (input.flags & Network::FLAG_IS_WALKING) != 0;

        remote.SetPosition(position);
        remote.SetYaw(input.yaw);
        remote.SetPitch(input.pitch);
        remote.SetWalking(isWalking);
    }
}

} // namespace App

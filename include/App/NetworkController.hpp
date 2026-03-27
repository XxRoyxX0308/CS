#ifndef CS_APP_NETWORKCONTROLLER_HPP
#define CS_APP_NETWORKCONTROLLER_HPP

#include "Network/NetworkManager.hpp"
#include "Entity/Player.hpp"
#include "Entity/RemotePlayer.hpp"
#include "Scene/SceneGraph.hpp"

#include <unordered_map>
#include <functional>

namespace App {

// Forward declarations
class StateManager;

/**
 * @brief Callback for bullet effect.
 */
using BulletEffectCallback = std::function<void(const glm::vec3& pos, const glm::vec3& normal)>;

/**
 * @brief Bridges network callbacks to game logic.
 */
class NetworkController {
public:
    NetworkController() = default;

    /**
     * @brief Setup all network callbacks.
     * @param network Network manager.
     * @param scene Scene graph for creating remote players.
     * @param player Local player reference.
     * @param remotePlayers Remote players map.
     * @param stateManager State manager for state transitions.
     * @param bulletEffectCallback Callback for bullet effects.
     */
    void SetupCallbacks(Network::NetworkManager& network,
                        Scene::SceneGraph& scene,
                        Entity::Player& player,
                        std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers,
                        StateManager& stateManager,
                        BulletEffectCallback bulletEffectCallback);

    /**
     * @brief Update network as host: process inputs, update remotes, broadcast state.
     * @param dt Delta time.
     * @param network Network manager.
     * @param player Local player.
     * @param camera Camera for state.
     * @param remotePlayers Remote players.
     */
    void UpdateHost(float dt,
                    Network::NetworkManager& network,
                    const Entity::Player& player,
                    const Core3D::Camera& camera,
                    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers);

    /**
     * @brief Update network as client: send input, receive state.
     * @param dt Delta time.
     * @param network Network manager.
     * @param inputState Current input state.
     * @param remotePlayers Remote players to update.
     */
    void UpdateClient(float dt,
                      Network::NetworkManager& network,
                      const Network::InputState& inputState,
                      std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers);

    /**
     * @brief Send player configuration to network.
     * @param network Network manager.
     * @param characterType Character type ID.
     * @param gunType Gun type ID.
     */
    void SendConfig(Network::NetworkManager& network, uint8_t characterType, uint8_t gunType);

    /**
     * @brief Broadcast config as host.
     */
    void BroadcastConfig(Network::NetworkManager& network, uint8_t playerId,
                         uint8_t characterType, uint8_t gunType);

private:
    /**
     * @brief Build and broadcast game state (Host only).
     */
    void BuildAndBroadcastGameState(Network::NetworkManager& network,
                                    const Entity::Player& player,
                                    const Core3D::Camera& camera,
                                    const std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers);

    /**
     * @brief Process remote player inputs (Host only).
     */
    void ProcessRemoteInputs(Network::NetworkManager& network,
                             std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers);
};

} // namespace App

#endif // CS_APP_NETWORKCONTROLLER_HPP

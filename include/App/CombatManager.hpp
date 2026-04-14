#ifndef CS_APP_COMBATMANAGER_HPP
#define CS_APP_COMBATMANAGER_HPP

#include "Entity/Player.hpp"
#include "Entity/RemotePlayer.hpp"
#include "Network/NetworkManager.hpp"
#include "Physics/CollisionMesh.hpp"
#include "Weapon/RayCast.hpp"

#include <unordered_map>

namespace App {

/**
 * @brief Result of a player hit check.
 */
struct PlayerHitResult {
    bool hit = false;
    uint8_t playerId = 0xFF;
    float distance = std::numeric_limits<float>::max();
    glm::vec3 point{0.0f};
};

/**
 * @brief Manages combat logic: hit detection, damage, and respawn.
 */
class CombatManager {
public:
    CombatManager() = default;

    /**
     * @brief Check if a ray hits any remote player.
     * @param origin Ray origin.
     * @param direction Ray direction (normalized).
     * @param maxDist Maximum distance to check.
     * @param remotePlayers Map of remote players.
     * @return Hit result.
     */
    PlayerHitResult CheckPlayerHit(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDist,
        const std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) const;

    /**
     * @brief Handle damage to a player (Host or Client logic).
     * @param victimId ID of the player hit.
     * @param damage Amount of damage.
     * @param hitPoint World position of hit.
     * @param network Network manager for broadcasting.
     * @param remotePlayers Remote players map.
     */
    void HandleDamage(uint8_t victimId,
                      float damage,
                      const glm::vec3& hitPoint,
                      Network::NetworkManager& network,
                      std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers);

    /**
     * @brief Check and handle local player respawn.
     * @param player Local player.
     * @param camera Camera to update.
     * @param collisionMesh Collision mesh for spawn point.
     */
    void CheckLocalRespawn(Entity::Player& player,
                           Core3D::Camera& camera,
                           const Physics::CollisionMesh& collisionMesh);

    /**
     * @brief Check and handle remote player respawns (Host only).
     * @param remotePlayers Remote players map.
     * @param collisionMesh Collision mesh for spawn point.
     */
    void CheckRemoteRespawns(std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers,
                             const Physics::CollisionMesh& collisionMesh);

    /**
     * @brief Calculate a spawn point on the map.
     * @param collisionMesh Collision mesh for ground detection.
     * @param characterType Character type to choose team spawn area.
     * @return Spawn position.
     */
    glm::vec3 GetSpawnPoint(const Physics::CollisionMesh& collisionMesh,
                            Entity::CharacterType characterType) const;
};

} // namespace App

#endif // CS_APP_COMBATMANAGER_HPP

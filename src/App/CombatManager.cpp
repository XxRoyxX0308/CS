#include "App/CombatManager.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Util/Logger.hpp"

namespace App {

PlayerHitResult CombatManager::CheckPlayerHit(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDist,
    const std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) const {

    PlayerHitResult result;

    for (const auto& [playerId, remote] : remotePlayers) {
        if (!remote.IsAlive()) continue;

        auto model = remote.GetCharacterModelPtr();
        if (model) {
            glm::mat4 transform = remote.GetModelWorldTransform();
            auto hit = Weapon::RayCast::CastAgainstModel(origin, direction, *model, transform, maxDist);

            if (hit.hit && hit.distance < result.distance) {
                result.hit = true;
                result.playerId = playerId;
                result.distance = hit.distance;
                result.point = hit.point;
            }
        }
    }

    return result;
}

void CombatManager::HandleDamage(uint8_t victimId,
                                  float damage,
                                  const glm::vec3& hitPoint,
                                  Network::NetworkManager& network,
                                  std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers) {
    if (network.IsHost()) {
        // Host processes damage directly
        if (victimId == 0) {
            // Host was hit (shouldn't happen in normal gameplay)
            return;
        }

        auto it = remotePlayers.find(victimId);
        if (it == remotePlayers.end()) return;

        auto& victim = it->second;
        bool stillAlive = victim.TakeDamage(damage);
        uint8_t newHealth = static_cast<uint8_t>(victim.GetHealth());
        uint8_t attackerId = network.GetLocalPlayerId();

        LOG_INFO("Player {} hit for {} damage, health now: {}", victimId, damage, newHealth);

        network.BroadcastPlayerHit(victimId, attackerId, newHealth, hitPoint);

        if (!stillAlive) {
            LOG_INFO("Player {} was killed by player {}!", victimId, attackerId);
            network.BroadcastPlayerDeath(victimId, attackerId);
        }
    } else if (network.IsClient()) {
        // Client sends hit report to server
        network.SendPlayerHit(victimId, damage, hitPoint);
        LOG_INFO("Client reports hitting player {} for {} damage", victimId, damage);
    }
}

void CombatManager::CheckLocalRespawn(Entity::Player& player,
                                       Core3D::Camera& camera,
                                       const Physics::CollisionMesh& collisionMesh) {
    if (!player.IsAlive()) {
        player.Respawn(camera, collisionMesh);
        LOG_INFO("Player respawned");
    }
}

void CombatManager::CheckRemoteRespawns(
    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers,
    const Physics::CollisionMesh& collisionMesh) {

    for (auto& [playerId, remote] : remotePlayers) {
        if (!remote.IsAlive()) {
            glm::vec3 spawnPos = GetSpawnPoint(collisionMesh);
            remote.Respawn(spawnPos);
            LOG_INFO("Remote player {} respawned at ({}, {}, {})",
                     playerId, spawnPos.x, spawnPos.y, spawnPos.z);
        }
    }
}

glm::vec3 CombatManager::GetSpawnPoint(const Physics::CollisionMesh& collisionMesh) const {
    float spawnX = 10.0f;
    float spawnZ = 0.0f;

    Physics::Capsule cap;
    cap.radius = 0.3f;
    cap.height = 1.7f - 2.0f * 0.3f;
    if (cap.height < 0.0f) cap.height = 0.0f;
    cap.base = glm::vec3(spawnX, 100.0f, spawnZ);

    auto groundY = Physics::CapsuleCast::SweepVertical(cap, collisionMesh, -200.0f);
    if (groundY.has_value()) {
        return glm::vec3(spawnX, groundY.value() + 1.7f, spawnZ);
    }

    // Fallback
    spawnX = -5.0f;
    spawnZ = -5.0f;
    cap.base = glm::vec3(spawnX, 100.0f, spawnZ);
    groundY = Physics::CapsuleCast::SweepVertical(cap, collisionMesh, -200.0f);
    if (groundY.has_value()) {
        return glm::vec3(spawnX, groundY.value() + 1.7f, spawnZ);
    }

    return glm::vec3(10.0f, 5.0f, 0.0f);
}

} // namespace App

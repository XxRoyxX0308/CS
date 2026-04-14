#include "App/CombatManager.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Util/Logger.hpp"
#include <random>

namespace App {

namespace {
struct SpawnRect {
    float minX;
    float maxX;
    float minZ;
    float maxZ;
    float probeY;
};

constexpr SpawnRect CT_SPAWN{2.5f, 9.4f, -49.8f, -46.8f, -0.6f};
constexpr SpawnRect T_SPAWN{-17.0f, -11.0f, 13.6f, 17.0f, 4.6f};

float RandInRange(float a, float b) {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(a, b);
    return dist(rng);
}
} // namespace

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
        const auto type = player.GetCharacterModel().GetCharacterType();
        player.Respawn(camera, collisionMesh, GetSpawnPoint(collisionMesh, type));
        LOG_INFO("Player respawned");
    }
}

void CombatManager::CheckRemoteRespawns(
    std::unordered_map<uint8_t, Entity::RemotePlayer>& remotePlayers,
    const Physics::CollisionMesh& collisionMesh) {

    for (auto& [playerId, remote] : remotePlayers) {
        if (!remote.IsAlive()) {
            glm::vec3 spawnPos = GetSpawnPoint(collisionMesh, remote.GetCharacterType());
            remote.Respawn(spawnPos);
            LOG_INFO("Remote player {} respawned at ({}, {}, {})",
                     playerId, spawnPos.x, spawnPos.y, spawnPos.z);
        }
    }
}

glm::vec3 CombatManager::GetSpawnPoint(const Physics::CollisionMesh& collisionMesh,
                                       Entity::CharacterType characterType) const {
    const SpawnRect area = (characterType == Entity::CharacterType::FBI) ? CT_SPAWN : T_SPAWN;

    Physics::Capsule cap;
    cap.radius = 0.3f;
    cap.height = 1.7f - 2.0f * 0.3f;
    if (cap.height < 0.0f) cap.height = 0.0f;

    for (int i = 0; i < 12; ++i) {
        const float spawnX = RandInRange(area.minX, area.maxX);
        const float spawnZ = RandInRange(area.minZ, area.maxZ);
        cap.base = glm::vec3(spawnX, area.probeY, spawnZ);

        auto groundY = Physics::CapsuleCast::SweepVertical(cap, collisionMesh, -120.0f);
        if (groundY.has_value()) {
            return glm::vec3(spawnX, groundY.value() + 1.7f, spawnZ);
        }
    }

    // If no ground hit, use area center as deterministic fallback.
    return glm::vec3((area.minX + area.maxX) * 0.5f, area.probeY + 1.7f, (area.minZ + area.maxZ) * 0.5f);
}

} // namespace App

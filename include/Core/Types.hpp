#ifndef CS_CORE_TYPES_HPP
#define CS_CORE_TYPES_HPP

#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

namespace CS {

// ── Forward Declarations ──────────────────────────────────────────────────

// Entity module
class Character;
class Player;
class RemotePlayer;
class CharacterModel;

// Physics module
namespace Physics {
    class CollisionMesh;
    struct Capsule;
    struct Triangle;
    struct SweepResult;
}

// Weapon module
namespace Weapon {
    class Weapon;
    class AssaultRifle;
    class SniperRifle;
    class Pistol;
}

// Network module
namespace Network {
    class NetworkManager;
    class GameServer;
    class GameClient;
}

// Effects module
namespace Effects {
    class BulletHoleManager;
}

// ── Common Type Aliases ───────────────────────────────────────────────────

using PlayerId = uint8_t;
using Vec3 = glm::vec3;
using Vec2 = glm::vec2;
using Mat4 = glm::mat4;

} // namespace CS

#endif // CS_CORE_TYPES_HPP

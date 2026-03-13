#ifndef CS_COLLISION_TYPES_HPP
#define CS_COLLISION_TYPES_HPP

#include <glm/glm.hpp>

namespace Collision {

// ── Triangle with precomputed normal ────────────────────────────────────────
struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 normal; // Unit normal (precomputed at build time)
};

// ── Vertical capsule ────────────────────────────────────────────────────────
//  Two hemispheres connected by a cylinder along the Y axis.
//      top    = base + vec3(0, height, 0)   (top sphere center)
//      bottom = base                        (bottom sphere center)
//  Total extent: base.y - radius  to  base.y + height + radius
struct Capsule {
    glm::vec3 base;   // Center of the bottom hemisphere
    float     height; // Distance between bottom and top sphere centers
    float     radius; // Radius of hemispheres and cylinder
};

// ── Sweep result ────────────────────────────────────────────────────────────
struct SweepResult {
    bool      hit    = false;
    float     t      = 1.0f;   // Parametric hit time ∈ [0, 1]
    glm::vec3 point  = {};     // World-space hit point
    glm::vec3 normal = {};     // Surface normal at hit point
};

} // namespace Collision

#endif // CS_COLLISION_TYPES_HPP

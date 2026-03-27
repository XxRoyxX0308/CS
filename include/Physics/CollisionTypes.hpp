#ifndef CS_PHYSICS_COLLISION_TYPES_HPP
#define CS_PHYSICS_COLLISION_TYPES_HPP

#include <glm/glm.hpp>

namespace Physics {

/**
 * @brief Triangle primitive with precomputed normal.
 * 
 * Used for collision detection against mesh geometry.
 * The normal is precomputed during collision mesh build for efficiency.
 */
struct Triangle {
    glm::vec3 v0, v1, v2;   ///< Triangle vertices in world space
    glm::vec3 normal;       ///< Unit normal (precomputed at build time)
};

/**
 * @brief Vertical capsule collision primitive.
 * 
 * Represents a capsule oriented along the Y axis, consisting of
 * two hemispheres connected by a cylinder.
 * 
 * - Top sphere center:    base + vec3(0, height, 0)
 * - Bottom sphere center: base
 * - Total vertical extent: base.y - radius to base.y + height + radius
 */
struct Capsule {
    glm::vec3 base;   ///< Center of the bottom hemisphere
    float height;     ///< Distance between bottom and top sphere centers
    float radius;     ///< Radius of hemispheres and cylinder
};

/**
 * @brief Result of a sweep/cast query.
 * 
 * Contains information about a collision hit from sweep tests.
 */
struct SweepResult {
    bool hit = false;           ///< Whether a collision occurred
    float t = 1.0f;             ///< Parametric hit time in [0, 1]
    glm::vec3 point = {};       ///< World-space hit point
    glm::vec3 normal = {};      ///< Surface normal at hit point
};

} // namespace Physics

#endif // CS_PHYSICS_COLLISION_TYPES_HPP

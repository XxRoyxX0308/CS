#ifndef CS_GUN_RAYCAST_HPP
#define CS_GUN_RAYCAST_HPP

#include "Collision/CollisionMesh.hpp"
#include "Collision/CollisionTypes.hpp"

#include <glm/glm.hpp>

namespace Gun {

struct RayHitResult {
    bool      hit      = false;
    float     distance = 0.0f;
    glm::vec3 point    = {};
    glm::vec3 normal   = {};
};

namespace RayCast {

    /**
     * @brief Cast a ray against the collision mesh.
     *
     * Uses Möller–Trumbore ray–triangle intersection with the existing
     * CollisionMesh broadphase grid for acceleration.
     *
     * @param origin    Ray origin (world space).
     * @param direction Ray direction (must be normalized).
     * @param mesh      The collision mesh to test against.
     * @param maxDist   Maximum ray distance.
     * @return RayHitResult with closest hit info.
     */
    RayHitResult Cast(const glm::vec3 &origin,
                      const glm::vec3 &direction,
                      const Collision::CollisionMesh &mesh,
                      float maxDist = 200.0f);

    /**
     * @brief Cast a ray against a capsule (for character hit detection).
     *
     * Tests ray intersection with a capsule defined by two hemispheres
     * and a cylinder.
     *
     * @param origin    Ray origin (world space).
     * @param direction Ray direction (must be normalized).
     * @param capsule   The capsule to test against.
     * @param maxDist   Maximum ray distance.
     * @return RayHitResult with hit info (normal points outward from capsule).
     */
    RayHitResult CastAgainstCapsule(const glm::vec3 &origin,
                                    const glm::vec3 &direction,
                                    const Collision::Capsule &capsule,
                                    float maxDist = 200.0f);

} // namespace RayCast

} // namespace Gun

#endif // CS_GUN_RAYCAST_HPP

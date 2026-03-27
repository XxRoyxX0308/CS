#ifndef CS_WEAPON_RAYCAST_HPP
#define CS_WEAPON_RAYCAST_HPP

#include "Physics/CollisionMesh.hpp"
#include "Physics/CollisionTypes.hpp"
#include "Core3D/Model.hpp"

#include <glm/glm.hpp>

namespace Weapon {

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
                      const Physics::CollisionMesh &mesh,
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
                                    const Physics::Capsule &capsule,
                                    float maxDist = 200.0f);

    /**
     * @brief Cast a ray against a 3D model (for accurate character hit detection).
     *
     * Uses Möller–Trumbore ray–triangle intersection on all model triangles.
     * The model vertices are transformed to world space using the provided matrix.
     *
     * @param origin         Ray origin (world space).
     * @param direction      Ray direction (must be normalized).
     * @param model          The 3D model to test against.
     * @param worldTransform Model-to-world transformation matrix.
     * @param maxDist        Maximum ray distance.
     * @return RayHitResult with hit info.
     */
    RayHitResult CastAgainstModel(const glm::vec3 &origin,
                                  const glm::vec3 &direction,
                                  const Core3D::Model &model,
                                  const glm::mat4 &worldTransform,
                                  float maxDist = 200.0f);

} // namespace RayCast

} // namespace Weapon

#endif // CS_WEAPON_RAYCAST_HPP

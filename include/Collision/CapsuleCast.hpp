#ifndef CS_COLLISION_CAPSULE_CAST_HPP
#define CS_COLLISION_CAPSULE_CAST_HPP

#include "Collision/CollisionTypes.hpp"
#include "Collision/CollisionMesh.hpp"

#include <optional>

namespace Collision {

/**
 * @brief Capsule sweep / cast queries against a CollisionMesh.
 *
 * All functions are stateless — pure collision math + broadphase lookup.
 */
namespace CapsuleCast {

    /**
     * @brief Sweep a sphere along a velocity vector against one triangle.
     * @return SweepResult with parametric t ∈ [0, 1] if hit.
     */
    SweepResult SphereTriangleSweep(const glm::vec3 &center,
                                    float radius,
                                    const glm::vec3 &velocity,
                                    const Triangle &tri);

    /**
     * @brief Sweep a vertical capsule against one triangle.
     *
     * Approximates capsule by 3 spheres along its central axis
     * (bottom, middle, top) and returns the earliest hit.
     */
    SweepResult CapsuleTriangleSweep(const Capsule &capsule,
                                     const glm::vec3 &velocity,
                                     const Triangle &tri);

    /**
     * @brief Sweep a capsule through the full collision mesh.
     *
     * Broadphase (AABB grid query) + narrowphase (capsule-triangle sweep).
     * Returns the closest hit along the velocity vector.
     */
    SweepResult SweepCapsule(const Capsule &capsule,
                             const glm::vec3 &velocity,
                             const CollisionMesh &mesh);

    /**
     * @brief Move a capsule along velocity, sliding along surfaces.
     *
     * Iterative collision response (up to maxIterations).
     * Returns the final world position of the capsule base.
     */
    glm::vec3 MoveAndSlide(const Capsule &capsule,
                           const glm::vec3 &velocity,
                           const CollisionMesh &mesh,
                           float skinWidth = 0.05f,
                           int maxIterations = 3);

    /**
     * @brief Sweep a capsule vertically.
     *
     * Positive distance = upward, negative = downward.
     * Returns the feet-Y at the hit point, or std::nullopt.
     */
    std::optional<float> SweepVertical(const Capsule &capsule,
                                       const CollisionMesh &mesh,
                                       float distance);

} // namespace CapsuleCast

} // namespace Collision

#endif // CS_COLLISION_CAPSULE_CAST_HPP

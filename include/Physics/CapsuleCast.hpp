#ifndef CS_PHYSICS_CAPSULE_CAST_HPP
#define CS_PHYSICS_CAPSULE_CAST_HPP

#include "Physics/CollisionTypes.hpp"
#include "Physics/CollisionMesh.hpp"

#include <optional>

namespace Physics {

/**
 * @brief Capsule sweep/cast queries against collision geometry.
 *
 * All functions are stateless - pure collision math + broadphase lookup.
 * Used for character movement, physics simulation, and collision detection.
 */
namespace CapsuleCast {

    /**
     * @brief Sweep a sphere along a velocity vector against one triangle.
     * 
     * Tests sphere against triangle face, edges, and vertices.
     * 
     * @param center   Sphere center position.
     * @param radius   Sphere radius.
     * @param velocity Movement vector for this frame.
     * @param tri      Triangle to test against.
     * @return SweepResult with parametric t in [0, 1] if hit.
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
     * 
     * @param capsule  The capsule to sweep.
     * @param velocity Movement vector for this frame.
     * @param tri      Triangle to test against.
     * @return SweepResult with earliest hit.
     */
    SweepResult CapsuleTriangleSweep(const Capsule &capsule,
                                     const glm::vec3 &velocity,
                                     const Triangle &tri);

    /**
     * @brief Sweep a capsule through the full collision mesh.
     *
     * Performs broadphase (AABB grid query) followed by narrowphase
     * (capsule-triangle sweep). Returns the closest hit along the
     * velocity vector.
     * 
     * @param capsule  The capsule to sweep.
     * @param velocity Movement vector for this frame.
     * @param mesh     The collision mesh to test against.
     * @return SweepResult with closest hit.
     */
    SweepResult SweepCapsule(const Capsule &capsule,
                             const glm::vec3 &velocity,
                             const CollisionMesh &mesh);

    /**
     * @brief Move a capsule along velocity, sliding along surfaces.
     *
     * Performs iterative collision response with wall sliding.
     * Used for character movement to prevent penetration while
     * allowing smooth movement along walls.
     * 
     * @param capsule       The capsule to move.
     * @param velocity      Desired movement vector.
     * @param mesh          The collision mesh.
     * @param skinWidth     Separation distance from surfaces.
     * @param maxIterations Maximum collision response iterations.
     * @return Final world position of the capsule base.
     */
    glm::vec3 MoveAndSlide(const Capsule &capsule,
                           const glm::vec3 &velocity,
                           const CollisionMesh &mesh,
                           float skinWidth = 0.05f,
                           int maxIterations = 3);

    /**
     * @brief Sweep a capsule vertically.
     *
     * Used for ground detection and gravity application.
     * 
     * @param capsule  The capsule to sweep.
     * @param mesh     The collision mesh.
     * @param distance Sweep distance (positive = up, negative = down).
     * @return Feet Y coordinate at hit point, or nullopt if no hit.
     */
    std::optional<float> SweepVertical(const Capsule &capsule,
                                       const CollisionMesh &mesh,
                                       float distance);

} // namespace CapsuleCast

} // namespace Physics

#endif // CS_PHYSICS_CAPSULE_CAST_HPP

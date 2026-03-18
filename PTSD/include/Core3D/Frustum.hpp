#ifndef CORE3D_FRUSTUM_HPP
#define CORE3D_FRUSTUM_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/AABB.hpp"

namespace Core3D {

/**
 * @brief A plane in 3D space (ax + by + cz + d = 0).
 */
struct Plane {
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;

    Plane() = default;
    Plane(const glm::vec3 &n, float d) : normal(n), distance(d) {}

    /** @brief Normalize the plane equation. */
    void Normalize() {
        float mag = glm::length(normal);
        if (mag > 0.0001f) {
            normal /= mag;
            distance /= mag;
        }
    }

    /** @brief Get signed distance from a point to this plane. */
    float DistanceToPoint(const glm::vec3 &point) const {
        return glm::dot(normal, point) + distance;
    }
};

/**
 * @brief Camera view frustum for visibility culling.
 *
 * Extracts 6 planes from the view-projection matrix.
 * Use ContainsAABB() to test if objects are visible.
 *
 * @code
 * Core3D::Frustum frustum;
 * frustum.Update(camera.GetViewMatrix() * camera.GetProjectionMatrix());
 *
 * if (frustum.ContainsAABB(objectAABB)) {
 *     // Object is visible, render it
 * }
 * @endcode
 */
class Frustum {
public:
    enum PlaneIndex {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR,
        PLANE_COUNT
    };

    Frustum() = default;

    /**
     * @brief Extract frustum planes from a view-projection matrix.
     * @param viewProjection The combined View * Projection matrix.
     */
    void Update(const glm::mat4 &viewProjection) {
        // Gribb/Hartmann method for extracting frustum planes
        // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

        // Left plane
        m_Planes[LEFT].normal.x = viewProjection[0][3] + viewProjection[0][0];
        m_Planes[LEFT].normal.y = viewProjection[1][3] + viewProjection[1][0];
        m_Planes[LEFT].normal.z = viewProjection[2][3] + viewProjection[2][0];
        m_Planes[LEFT].distance = viewProjection[3][3] + viewProjection[3][0];

        // Right plane
        m_Planes[RIGHT].normal.x = viewProjection[0][3] - viewProjection[0][0];
        m_Planes[RIGHT].normal.y = viewProjection[1][3] - viewProjection[1][0];
        m_Planes[RIGHT].normal.z = viewProjection[2][3] - viewProjection[2][0];
        m_Planes[RIGHT].distance = viewProjection[3][3] - viewProjection[3][0];

        // Bottom plane
        m_Planes[BOTTOM].normal.x = viewProjection[0][3] + viewProjection[0][1];
        m_Planes[BOTTOM].normal.y = viewProjection[1][3] + viewProjection[1][1];
        m_Planes[BOTTOM].normal.z = viewProjection[2][3] + viewProjection[2][1];
        m_Planes[BOTTOM].distance = viewProjection[3][3] + viewProjection[3][1];

        // Top plane
        m_Planes[TOP].normal.x = viewProjection[0][3] - viewProjection[0][1];
        m_Planes[TOP].normal.y = viewProjection[1][3] - viewProjection[1][1];
        m_Planes[TOP].normal.z = viewProjection[2][3] - viewProjection[2][1];
        m_Planes[TOP].distance = viewProjection[3][3] - viewProjection[3][1];

        // Near plane
        m_Planes[NEAR].normal.x = viewProjection[0][3] + viewProjection[0][2];
        m_Planes[NEAR].normal.y = viewProjection[1][3] + viewProjection[1][2];
        m_Planes[NEAR].normal.z = viewProjection[2][3] + viewProjection[2][2];
        m_Planes[NEAR].distance = viewProjection[3][3] + viewProjection[3][2];

        // Far plane
        m_Planes[FAR].normal.x = viewProjection[0][3] - viewProjection[0][2];
        m_Planes[FAR].normal.y = viewProjection[1][3] - viewProjection[1][2];
        m_Planes[FAR].normal.z = viewProjection[2][3] - viewProjection[2][2];
        m_Planes[FAR].distance = viewProjection[3][3] - viewProjection[3][2];

        // Normalize all planes
        for (auto &plane : m_Planes) {
            plane.Normalize();
        }
    }

    /**
     * @brief Test if an AABB is inside or intersecting the frustum.
     * @param aabb The axis-aligned bounding box to test.
     * @return true if visible (inside or intersecting), false if completely outside.
     */
    bool ContainsAABB(const AABB &aabb) const {
        for (const auto &plane : m_Planes) {
            // Get the positive vertex (furthest in the direction of the plane normal)
            glm::vec3 pVertex = aabb.min;
            if (plane.normal.x >= 0) pVertex.x = aabb.max.x;
            if (plane.normal.y >= 0) pVertex.y = aabb.max.y;
            if (plane.normal.z >= 0) pVertex.z = aabb.max.z;

            // If the positive vertex is outside this plane, the AABB is culled
            if (plane.DistanceToPoint(pVertex) < 0) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Test if a sphere is inside or intersecting the frustum.
     * @param center The sphere center.
     * @param radius The sphere radius.
     * @return true if visible, false if completely outside.
     */
    bool ContainsSphere(const glm::vec3 &center, float radius) const {
        for (const auto &plane : m_Planes) {
            if (plane.DistanceToPoint(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Test if a point is inside the frustum.
     */
    bool ContainsPoint(const glm::vec3 &point) const {
        for (const auto &plane : m_Planes) {
            if (plane.DistanceToPoint(point) < 0) {
                return false;
            }
        }
        return true;
    }

    const Plane &GetPlane(PlaneIndex index) const { return m_Planes[index]; }

private:
    std::array<Plane, PLANE_COUNT> m_Planes;
};

} // namespace Core3D

#endif

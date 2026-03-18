#ifndef CORE3D_FRUSTUM_HPP
#define CORE3D_FRUSTUM_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/AABB.hpp"

namespace Core3D {

/**
 * @brief Frustum planes for view frustum culling.
 *
 * Extracts 6 planes from a View-Projection matrix.
 * Use to cull objects that are completely outside the camera's view.
 *
 * @code
 * Core3D::Frustum frustum;
 * frustum.Update(viewMatrix * projectionMatrix);
 *
 * if (frustum.IsAABBVisible(worldAABB)) {
 *     // Object is potentially visible, render it
 * }
 * @endcode
 */
class Frustum {
public:
    /**
     * @brief Plane indices for the 6 frustum planes.
     */
    enum PlaneIndex {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR,
        COUNT
    };

    /**
     * @brief Update frustum planes from a View-Projection matrix.
     *
     * Uses the Gribb-Hartmann method to extract planes directly from the matrix.
     * Planes are normalized for accurate distance calculations.
     *
     * @param viewProj The combined View * Projection matrix.
     */
    void Update(const glm::mat4 &viewProj) {
        // Extract planes using Gribb-Hartmann method
        // Each row gives us: plane.xyz = normal, plane.w = distance

        // Left:   row3 + row0
        m_Planes[LEFT] = glm::vec4(
            viewProj[0][3] + viewProj[0][0],
            viewProj[1][3] + viewProj[1][0],
            viewProj[2][3] + viewProj[2][0],
            viewProj[3][3] + viewProj[3][0]
        );

        // Right:  row3 - row0
        m_Planes[RIGHT] = glm::vec4(
            viewProj[0][3] - viewProj[0][0],
            viewProj[1][3] - viewProj[1][0],
            viewProj[2][3] - viewProj[2][0],
            viewProj[3][3] - viewProj[3][0]
        );

        // Bottom: row3 + row1
        m_Planes[BOTTOM] = glm::vec4(
            viewProj[0][3] + viewProj[0][1],
            viewProj[1][3] + viewProj[1][1],
            viewProj[2][3] + viewProj[2][1],
            viewProj[3][3] + viewProj[3][1]
        );

        // Top:    row3 - row1
        m_Planes[TOP] = glm::vec4(
            viewProj[0][3] - viewProj[0][1],
            viewProj[1][3] - viewProj[1][1],
            viewProj[2][3] - viewProj[2][1],
            viewProj[3][3] - viewProj[3][1]
        );

        // Near:   row3 + row2
        m_Planes[NEAR] = glm::vec4(
            viewProj[0][3] + viewProj[0][2],
            viewProj[1][3] + viewProj[1][2],
            viewProj[2][3] + viewProj[2][2],
            viewProj[3][3] + viewProj[3][2]
        );

        // Far:    row3 - row2
        m_Planes[FAR] = glm::vec4(
            viewProj[0][3] - viewProj[0][2],
            viewProj[1][3] - viewProj[1][2],
            viewProj[2][3] - viewProj[2][2],
            viewProj[3][3] - viewProj[3][2]
        );

        // Normalize all planes
        for (int i = 0; i < COUNT; ++i) {
            float length = glm::length(glm::vec3(m_Planes[i]));
            if (length > 0.0f) {
                m_Planes[i] /= length;
            }
        }
    }

    /**
     * @brief Test if a point is inside the frustum.
     */
    bool IsPointVisible(const glm::vec3 &point) const {
        for (int i = 0; i < COUNT; ++i) {
            if (SignedDistance(m_Planes[i], point) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Test if a sphere is visible (intersects or inside the frustum).
     */
    bool IsSphereVisible(const glm::vec3 &center, float radius) const {
        for (int i = 0; i < COUNT; ++i) {
            if (SignedDistance(m_Planes[i], center) < -radius) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Test if a BoundingSphere is visible.
     */
    bool IsSphereVisible(const BoundingSphere &sphere) const {
        return IsSphereVisible(sphere.center, sphere.radius);
    }

    /**
     * @brief Test if an AABB is visible (intersects or inside the frustum).
     *
     * Uses the "n-vertex" optimization: for each plane, test the vertex
     * that is most likely to be outside.
     */
    bool IsAABBVisible(const AABB &aabb) const {
        if (!aabb.IsValid()) return false;

        for (int i = 0; i < COUNT; ++i) {
            // Find the "positive vertex" (p-vertex) - the corner furthest
            // along the plane normal
            glm::vec3 pVertex = aabb.min;
            if (m_Planes[i].x >= 0) pVertex.x = aabb.max.x;
            if (m_Planes[i].y >= 0) pVertex.y = aabb.max.y;
            if (m_Planes[i].z >= 0) pVertex.z = aabb.max.z;

            // If the p-vertex is outside, the entire AABB is outside
            if (SignedDistance(m_Planes[i], pVertex) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Get the raw plane data (for debugging or custom tests).
     */
    const glm::vec4 &GetPlane(PlaneIndex index) const {
        return m_Planes[index];
    }

private:
    /**
     * @brief Calculate signed distance from a plane to a point.
     *
     * Positive = in front, Negative = behind, Zero = on plane.
     */
    static float SignedDistance(const glm::vec4 &plane, const glm::vec3 &point) {
        return glm::dot(glm::vec3(plane), point) + plane.w;
    }

    glm::vec4 m_Planes[COUNT];
};

} // namespace Core3D

#endif

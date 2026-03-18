#ifndef CORE3D_AABB_HPP
#define CORE3D_AABB_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

/**
 * @brief Axis-Aligned Bounding Box for spatial queries and culling.
 *
 * Stores min/max corners in local or world space.
 * Used for frustum culling, collision detection, and spatial partitioning.
 *
 * @code
 * Core3D::AABB box;
 * box.Expand(vertex.position);  // Build from vertices
 * box.Transform(worldMatrix);    // Convert to world space
 * @endcode
 */
struct AABB {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    /** @brief Check if the AABB is valid (min <= max). */
    bool IsValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    /** @brief Reset to invalid state. */
    void Reset() {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::lowest());
    }

    /** @brief Expand the AABB to include a point. */
    void Expand(const glm::vec3 &point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    /** @brief Expand the AABB to include another AABB. */
    void Expand(const AABB &other) {
        if (!other.IsValid()) return;
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    /** @brief Get the center point of the AABB. */
    glm::vec3 GetCenter() const {
        return (min + max) * 0.5f;
    }

    /** @brief Get the half-extents (size/2) of the AABB. */
    glm::vec3 GetExtents() const {
        return (max - min) * 0.5f;
    }

    /** @brief Get the full size of the AABB. */
    glm::vec3 GetSize() const {
        return max - min;
    }

    /**
     * @brief Transform the AABB by a matrix (returns new world-space AABB).
     *
     * Uses the 8-corner method for accurate transformation.
     * The result is still axis-aligned but may be larger than optimal.
     */
    AABB Transform(const glm::mat4 &matrix) const {
        if (!IsValid()) return *this;

        AABB result;

        // Transform all 8 corners and expand
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {min.x, max.y, max.z},
            {max.x, max.y, max.z}
        };

        for (const auto &corner : corners) {
            glm::vec4 transformed = matrix * glm::vec4(corner, 1.0f);
            result.Expand(glm::vec3(transformed));
        }

        return result;
    }

    /**
     * @brief Fast AABB transformation using Arvo's method.
     *
     * More efficient than the 8-corner method for pure affine transforms.
     */
    AABB TransformFast(const glm::mat4 &m) const {
        if (!IsValid()) return *this;

        AABB result;
        glm::vec3 translation(m[3]);
        result.min = translation;
        result.max = translation;

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                float a = m[j][i] * min[j];
                float b = m[j][i] * max[j];
                result.min[i] += (a < b) ? a : b;
                result.max[i] += (a < b) ? b : a;
            }
        }

        return result;
    }

    /** @brief Check if this AABB intersects another AABB. */
    bool Intersects(const AABB &other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    /** @brief Check if this AABB contains a point. */
    bool Contains(const glm::vec3 &point) const {
        return (point.x >= min.x && point.x <= max.x) &&
               (point.y >= min.y && point.y <= max.y) &&
               (point.z >= min.z && point.z <= max.z);
    }
};

/**
 * @brief Bounding Sphere for quick rejection tests.
 */
struct BoundingSphere {
    glm::vec3 center{0.0f};
    float radius = 0.0f;

    /** @brief Construct from AABB. */
    static BoundingSphere FromAABB(const AABB &aabb) {
        BoundingSphere sphere;
        sphere.center = aabb.GetCenter();
        sphere.radius = glm::length(aabb.GetExtents());
        return sphere;
    }

    /** @brief Transform the sphere by a uniform scale matrix. */
    BoundingSphere Transform(const glm::mat4 &matrix) const {
        BoundingSphere result;
        result.center = glm::vec3(matrix * glm::vec4(center, 1.0f));
        // Extract max scale factor for radius
        float scaleX = glm::length(glm::vec3(matrix[0]));
        float scaleY = glm::length(glm::vec3(matrix[1]));
        float scaleZ = glm::length(glm::vec3(matrix[2]));
        result.radius = radius * glm::max(scaleX, glm::max(scaleY, scaleZ));
        return result;
    }
};

} // namespace Core3D

#endif

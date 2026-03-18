#ifndef CORE3D_AABB_HPP
#define CORE3D_AABB_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

/**
 * @brief Axis-Aligned Bounding Box for spatial queries and culling.
 *
 * Stores min/max corners in world or local space.
 * Supports intersection tests with frustum planes.
 */
struct AABB {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    AABB() = default;
    AABB(const glm::vec3 &minPt, const glm::vec3 &maxPt)
        : min(minPt), max(maxPt) {}

    /** @brief Check if this AABB is valid (min <= max). */
    bool IsValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    /** @brief Get the center point. */
    glm::vec3 GetCenter() const { return (min + max) * 0.5f; }

    /** @brief Get the half-extents (size / 2). */
    glm::vec3 GetExtents() const { return (max - min) * 0.5f; }

    /** @brief Get the full size. */
    glm::vec3 GetSize() const { return max - min; }

    /** @brief Expand this AABB to include a point. */
    void ExpandToInclude(const glm::vec3 &point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    /** @brief Expand this AABB to include another AABB. */
    void ExpandToInclude(const AABB &other) {
        if (other.IsValid()) {
            min = glm::min(min, other.min);
            max = glm::max(max, other.max);
        }
    }

    /** @brief Transform this AABB by a matrix, returning a new axis-aligned box. */
    AABB Transform(const glm::mat4 &matrix) const {
        // Transform all 8 corners and compute new AABB
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        };

        AABB result;
        for (const auto &corner : corners) {
            glm::vec4 transformed = matrix * glm::vec4(corner, 1.0f);
            result.ExpandToInclude(glm::vec3(transformed));
        }
        return result;
    }

    /** @brief Get the 8 corner points. */
    std::array<glm::vec3, 8> GetCorners() const {
        return {{
            {min.x, min.y, min.z}, {max.x, min.y, min.z},
            {min.x, max.y, min.z}, {max.x, max.y, min.z},
            {min.x, min.y, max.z}, {max.x, min.y, max.z},
            {min.x, max.y, max.z}, {max.x, max.y, max.z}
        }};
    }
};

} // namespace Core3D

#endif

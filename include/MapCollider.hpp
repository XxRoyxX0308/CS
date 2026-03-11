#ifndef CS_MAP_COLLIDER_HPP
#define CS_MAP_COLLIDER_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Model.hpp"

#include <glm/glm.hpp>
#include <vector>

/**
 * @brief Mesh-based ground collision using a 2D spatial grid (XZ plane).
 *
 * Extracts all triangles from a loaded 3D model, transforms them into
 * world space, bins them into a uniform grid, and supports fast downward
 * ray-triangle queries to find the ground height at any (x, z) position.
 */
class MapCollider {
public:
    MapCollider() = default;

    /**
     * @brief Build the collision grid from a model.
     * @param model  The loaded 3D model (e.g., de_dust2 OBJ).
     * @param transform  The world-space transform applied to the model
     *                    (same rotation + scale as the SceneNode).
     */
    void Build(const Core3D::Model &model, const glm::mat4 &transform);

    /**
     * @brief Query the highest ground surface below a given point.
     * @param x  World X position.
     * @param z  World Z position.
     * @param maxY  Maximum Y to cast from (ray origin Y).
     * @return The Y coordinate of the ground surface,
     *         or -infinity if no ground is found.
     */
    float GetGroundHeight(float x, float z, float maxY) const;

    /**
     * @brief Check if a horizontal segment is blocked by wall geometry.
     * @param x1, z1  Start of segment (character center).
     * @param x2, z2  End of segment (octagon sample point).
     * @param minY    Bottom of body column to check.
     * @param maxY    Top of body column to check.
     * @return true if any wall triangle blocks the segment.
     */
    bool IsWallBlocking(float x1, float z1, float x2, float z2,
                        float minY, float maxY) const;

    /** @brief Number of collision triangles. */
    size_t GetTriangleCount() const { return m_Triangles.size(); }

    /** @brief Whether the collider has been built. */
    bool IsBuilt() const { return m_Built; }

private:
    struct Triangle {
        glm::vec3 v0, v1, v2;
    };

    // Möller-Trumbore ray-triangle intersection (downward ray)
    static bool RayTriangleIntersect(const glm::vec3 &origin,
                                     const Triangle &tri,
                                     float &outT);

    // Generic Möller-Trumbore ray-triangle intersection (any direction)
    static bool RayTriangleIntersectGeneric(const glm::vec3 &origin,
                                            const glm::vec3 &dir,
                                            const Triangle &tri,
                                            float &outT);

    // Grid helpers
    int GridIndex(float x, float z) const;

    // All walkable triangles in world space
    std::vector<Triangle> m_Triangles;

    // Wall triangles (steep/vertical surfaces)
    std::vector<Triangle> m_WallTriangles;

    // 2D uniform grid (XZ plane) — each cell holds indices into m_Triangles
    std::vector<std::vector<size_t>> m_Grid;

    // Wall grid — each cell holds indices into m_WallTriangles
    std::vector<std::vector<size_t>> m_WallGrid;
    int   m_GridResX = 0;
    int   m_GridResZ = 0;
    float m_MinX = 0.0f, m_MinZ = 0.0f;
    float m_MaxX = 0.0f, m_MaxZ = 0.0f;
    float m_CellSizeX = 1.0f, m_CellSizeZ = 1.0f;

    bool m_Built = false;
};

#endif // CS_MAP_COLLIDER_HPP

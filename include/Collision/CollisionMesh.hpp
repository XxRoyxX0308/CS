#ifndef CS_COLLISION_MESH_HPP
#define CS_COLLISION_MESH_HPP

#include "Collision/CollisionTypes.hpp"
#include "Core3D/Model.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Collision {

// ── Axis-aligned bounding box (XZ plane, with optional Y) ───────────────────
struct AABB_XZ {
    float minX, maxX;
    float minZ, maxZ;
};

/**
 * @brief Mesh-based collision data with packed 2D spatial grid (XZ plane).
 *
 * Extracts all non-degenerate triangles from a loaded 3D model, transforms
 * them into world space, precomputes normals, and bins them into a cache-
 * friendly packed uniform grid for fast broadphase queries.
 */
class CollisionMesh {
public:
    CollisionMesh() = default;

    /**
     * @brief Build the collision mesh and spatial grid from a model.
     * @param model      The loaded 3D model (e.g., de_dust2 OBJ).
     * @param transform  World-space transform (rotation + scale).
     */
    void Build(const Core3D::Model &model, const glm::mat4 &transform);

    /**
     * @brief Gather candidate triangle indices that overlap an XZ AABB.
     *
     * Returned indices may contain duplicates (a triangle can span
     * multiple cells). The caller is responsible for deduplication if
     * needed (typically via a visited bitset).
     *
     * @param aabb  Query region on the XZ plane.
     * @param out   Output vector — candidate indices are appended.
     */
    void GatherCandidates(const AABB_XZ &aabb,
                          std::vector<uint32_t> &out) const;

    // ── Accessors ───────────────────────────────────────────────────────
    const std::vector<Triangle> &GetTriangles() const { return m_Triangles; }
    size_t GetTriangleCount() const { return m_Triangles.size(); }
    bool   IsBuilt()          const { return m_Built; }

private:
    // Flat grid index from world XZ (clamped, returns -1 if out of bounds)
    int GridIndex(float x, float z) const;

    // All triangles in world space (unified — no ground/wall split)
    std::vector<Triangle> m_Triangles;

    // Packed flat grid: m_CellOffsets[i]..m_CellOffsets[i+1] index into
    // m_TriIndices for the triangle indices in cell i.
    std::vector<uint32_t> m_CellOffsets; // size = numCells + 1
    std::vector<uint32_t> m_TriIndices;  // packed triangle indices

    int   m_GridResX  = 0;
    int   m_GridResZ  = 0;
    float m_MinX = 0.0f, m_MinZ = 0.0f;
    float m_MaxX = 0.0f, m_MaxZ = 0.0f;
    float m_CellSizeX = 1.0f;
    float m_CellSizeZ = 1.0f;

    bool m_Built = false;
};

} // namespace Collision

#endif // CS_COLLISION_MESH_HPP

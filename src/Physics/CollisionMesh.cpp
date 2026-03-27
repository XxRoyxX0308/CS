#include "Physics/CollisionMesh.hpp"
#include "Util/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace Physics {

// ============================================================================
//  Build - Extract triangles, precompute normals, build packed grid
// ============================================================================
void CollisionMesh::Build(const Core3D::Model &model,
                          const glm::mat4 &transform) {
    m_Triangles.clear();
    m_CellOffsets.clear();
    m_TriIndices.clear();
    m_Built = false;

    // ── 1. Extract world-space triangles with precomputed normals ─────────
    for (const auto &mesh : model.GetMeshes()) {
        const auto &verts   = mesh.GetVertices();
        const auto &indices = mesh.GetIndices();

        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            Triangle tri;
            tri.v0 = glm::vec3(transform * glm::vec4(verts[indices[i + 0]].position, 1.0f));
            tri.v1 = glm::vec3(transform * glm::vec4(verts[indices[i + 1]].position, 1.0f));
            tri.v2 = glm::vec3(transform * glm::vec4(verts[indices[i + 2]].position, 1.0f));

            glm::vec3 edge1  = tri.v1 - tri.v0;
            glm::vec3 edge2  = tri.v2 - tri.v0;
            glm::vec3 normal = glm::cross(edge1, edge2);
            float len = glm::length(normal);
            if (len < 1e-8f) continue; // Skip degenerate triangles

            tri.normal = normal / len;
            m_Triangles.push_back(tri);
        }
    }

    if (m_Triangles.empty()) {
        LOG_WARN("CollisionMesh::Build: no triangles found!");
        return;
    }

    LOG_INFO("CollisionMesh: {} triangles extracted", m_Triangles.size());

    // ── 2. Compute XZ bounding box ────────────────────────────────────────
    m_MinX = m_MinZ =  std::numeric_limits<float>::max();
    m_MaxX = m_MaxZ = -std::numeric_limits<float>::max();

    for (const auto &tri : m_Triangles) {
        for (const auto *v : {&tri.v0, &tri.v1, &tri.v2}) {
            m_MinX = std::min(m_MinX, v->x);
            m_MaxX = std::max(m_MaxX, v->x);
            m_MinZ = std::min(m_MinZ, v->z);
            m_MaxZ = std::max(m_MaxZ, v->z);
        }
    }

    // Add padding
    m_MinX -= 1.0f; m_MinZ -= 1.0f;
    m_MaxX += 1.0f; m_MaxZ += 1.0f;

    // ── 3. Determine grid resolution (~2m per cell) ───────────────────────
    constexpr float TARGET_CELL = 2.0f;

    m_GridResX = std::max(1, static_cast<int>(std::ceil((m_MaxX - m_MinX) / TARGET_CELL)));
    m_GridResZ = std::max(1, static_cast<int>(std::ceil((m_MaxZ - m_MinZ) / TARGET_CELL)));
    m_GridResX = std::min(m_GridResX, 512);
    m_GridResZ = std::min(m_GridResZ, 512);

    m_CellSizeX = (m_MaxX - m_MinX) / static_cast<float>(m_GridResX);
    m_CellSizeZ = (m_MaxZ - m_MinZ) / static_cast<float>(m_GridResZ);

    const size_t numCells = static_cast<size_t>(m_GridResX) * m_GridResZ;

    // ── 4. Count triangles per cell (first pass) ──────────────────────────
    std::vector<uint32_t> counts(numCells, 0);

    for (size_t ti = 0; ti < m_Triangles.size(); ++ti) {
        const auto &tri = m_Triangles[ti];

        float triMinX = std::min({tri.v0.x, tri.v1.x, tri.v2.x});
        float triMaxX = std::max({tri.v0.x, tri.v1.x, tri.v2.x});
        float triMinZ = std::min({tri.v0.z, tri.v1.z, tri.v2.z});
        float triMaxZ = std::max({tri.v0.z, tri.v1.z, tri.v2.z});

        int cellMinX = std::max(0, static_cast<int>((triMinX - m_MinX) / m_CellSizeX));
        int cellMaxX = std::min(m_GridResX - 1, static_cast<int>((triMaxX - m_MinX) / m_CellSizeX));
        int cellMinZ = std::max(0, static_cast<int>((triMinZ - m_MinZ) / m_CellSizeZ));
        int cellMaxZ = std::min(m_GridResZ - 1, static_cast<int>((triMaxZ - m_MinZ) / m_CellSizeZ));

        for (int gz = cellMinZ; gz <= cellMaxZ; ++gz)
            for (int gx = cellMinX; gx <= cellMaxX; ++gx)
                ++counts[static_cast<size_t>(gz) * m_GridResX + gx];
    }

    // ── 5. Build prefix-sum offsets ───────────────────────────────────────
    m_CellOffsets.resize(numCells + 1);
    m_CellOffsets[0] = 0;
    for (size_t i = 0; i < numCells; ++i)
        m_CellOffsets[i + 1] = m_CellOffsets[i] + counts[i];

    // ── 6. Fill packed triangle indices (second pass) ─────────────────────
    m_TriIndices.resize(m_CellOffsets[numCells]);

    // Reuse counts as write cursors (reset to zero first)
    std::fill(counts.begin(), counts.end(), 0);

    for (uint32_t ti = 0; ti < static_cast<uint32_t>(m_Triangles.size()); ++ti) {
        const auto &tri = m_Triangles[ti];

        float triMinX = std::min({tri.v0.x, tri.v1.x, tri.v2.x});
        float triMaxX = std::max({tri.v0.x, tri.v1.x, tri.v2.x});
        float triMinZ = std::min({tri.v0.z, tri.v1.z, tri.v2.z});
        float triMaxZ = std::max({tri.v0.z, tri.v1.z, tri.v2.z});

        int cellMinX = std::max(0, static_cast<int>((triMinX - m_MinX) / m_CellSizeX));
        int cellMaxX = std::min(m_GridResX - 1, static_cast<int>((triMaxX - m_MinX) / m_CellSizeX));
        int cellMinZ = std::max(0, static_cast<int>((triMinZ - m_MinZ) / m_CellSizeZ));
        int cellMaxZ = std::min(m_GridResZ - 1, static_cast<int>((triMaxZ - m_MinZ) / m_CellSizeZ));

        for (int gz = cellMinZ; gz <= cellMaxZ; ++gz) {
            for (int gx = cellMinX; gx <= cellMaxX; ++gx) {
                size_t cell = static_cast<size_t>(gz) * m_GridResX + gx;
                m_TriIndices[m_CellOffsets[cell] + counts[cell]] = ti;
                ++counts[cell];
            }
        }
    }

    m_Built = true;
    LOG_INFO("CollisionMesh: grid {}x{} (cell {:.2f}x{:.2f} m), "
             "bounds X[{:.1f},{:.1f}] Z[{:.1f},{:.1f}], "
             "{} packed indices",
             m_GridResX, m_GridResZ, m_CellSizeX, m_CellSizeZ,
             m_MinX, m_MaxX, m_MinZ, m_MaxZ,
             m_TriIndices.size());
}

// ============================================================================
//  GridIndex - World XZ to flat cell index (-1 if out of bounds)
// ============================================================================
int CollisionMesh::GridIndex(float x, float z) const {
    if (x < m_MinX || x > m_MaxX || z < m_MinZ || z > m_MaxZ) return -1;

    int gx = static_cast<int>((x - m_MinX) / m_CellSizeX);
    int gz = static_cast<int>((z - m_MinZ) / m_CellSizeZ);
    gx = std::clamp(gx, 0, m_GridResX - 1);
    gz = std::clamp(gz, 0, m_GridResZ - 1);

    return gz * m_GridResX + gx;
}

// ============================================================================
//  GatherCandidates - Collect triangle indices overlapping an XZ AABB
// ============================================================================
void CollisionMesh::GatherCandidates(const AABB_XZ &aabb,
                                     std::vector<uint32_t> &out) const {
    if (!m_Built) return;

    int cellMinX = std::max(0, static_cast<int>((aabb.minX - m_MinX) / m_CellSizeX));
    int cellMaxX = std::min(m_GridResX - 1, static_cast<int>((aabb.maxX - m_MinX) / m_CellSizeX));
    int cellMinZ = std::max(0, static_cast<int>((aabb.minZ - m_MinZ) / m_CellSizeZ));
    int cellMaxZ = std::min(m_GridResZ - 1, static_cast<int>((aabb.maxZ - m_MinZ) / m_CellSizeZ));

    for (int gz = cellMinZ; gz <= cellMaxZ; ++gz) {
        for (int gx = cellMinX; gx <= cellMaxX; ++gx) {
            size_t cell = static_cast<size_t>(gz) * m_GridResX + gx;
            uint32_t begin = m_CellOffsets[cell];
            uint32_t end   = m_CellOffsets[cell + 1];
            for (uint32_t i = begin; i < end; ++i)
                out.push_back(m_TriIndices[i]);
        }
    }
}

} // namespace Physics

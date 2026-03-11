#include "MapCollider.hpp"
#include "Util/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

// ============================================================================
//  Build — extract triangles from every mesh, transform, and build grid
// ============================================================================
void MapCollider::Build(const Core3D::Model &model, const glm::mat4 &transform) {
    m_Triangles.clear();
    m_WallTriangles.clear();
    m_Grid.clear();
    m_WallGrid.clear();
    m_Built = false;

    // ── 1. Extract world-space triangles ────────────────────────────────
    for (const auto &mesh : model.GetMeshes()) {
        const auto &verts   = mesh.GetVertices();
        const auto &indices = mesh.GetIndices();

        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            Triangle tri;
            tri.v0 = glm::vec3(transform * glm::vec4(verts[indices[i + 0]].position, 1.0f));
            tri.v1 = glm::vec3(transform * glm::vec4(verts[indices[i + 1]].position, 1.0f));
            tri.v2 = glm::vec3(transform * glm::vec4(verts[indices[i + 2]].position, 1.0f));

            // Skip degenerate triangles
            glm::vec3 edge1 = tri.v1 - tri.v0;
            glm::vec3 edge2 = tri.v2 - tri.v0;
            glm::vec3 normal = glm::cross(edge1, edge2);
            if (glm::length(normal) < 1e-8f) continue;

            float upDot = glm::normalize(normal).y;

            if (upDot >= 0.1f) {
                // Walkable ground / ramp
                m_Triangles.push_back(tri);
            } else if (std::abs(upDot) < 0.7f) {
                // Steep / vertical wall (not floor, not ceiling)
                m_WallTriangles.push_back(tri);
            }
        }
    }

    if (m_Triangles.empty() && m_WallTriangles.empty()) {
        LOG_WARN("MapCollider::Build: no triangles found!");
        return;
    }

    LOG_INFO("MapCollider: {} walkable + {} wall triangles extracted",
             m_Triangles.size(), m_WallTriangles.size());

    // ── 2. Compute XZ bounding box (from both ground + wall) ────────────
    m_MinX = m_MinZ =  std::numeric_limits<float>::max();
    m_MaxX = m_MaxZ = -std::numeric_limits<float>::max();

    auto expandBounds = [&](const Triangle &tri) {
        for (const auto *v : {&tri.v0, &tri.v1, &tri.v2}) {
            m_MinX = std::min(m_MinX, v->x);
            m_MaxX = std::max(m_MaxX, v->x);
            m_MinZ = std::min(m_MinZ, v->z);
            m_MaxZ = std::max(m_MaxZ, v->z);
        }
    };
    for (const auto &tri : m_Triangles)     expandBounds(tri);
    for (const auto &tri : m_WallTriangles) expandBounds(tri);

    // Add small padding
    m_MinX -= 1.0f; m_MinZ -= 1.0f;
    m_MaxX += 1.0f; m_MaxZ += 1.0f;

    // ── 3. Build uniform grid ───────────────────────────────────────────
    // Target ~2m per cell for good granularity on a map this size
    constexpr float TARGET_CELL = 2.0f;

    m_GridResX = std::max(1, static_cast<int>(std::ceil((m_MaxX - m_MinX) / TARGET_CELL)));
    m_GridResZ = std::max(1, static_cast<int>(std::ceil((m_MaxZ - m_MinZ) / TARGET_CELL)));

    // Clamp to reasonable maximum to avoid excessive memory
    m_GridResX = std::min(m_GridResX, 512);
    m_GridResZ = std::min(m_GridResZ, 512);

    m_CellSizeX = (m_MaxX - m_MinX) / static_cast<float>(m_GridResX);
    m_CellSizeZ = (m_MaxZ - m_MinZ) / static_cast<float>(m_GridResZ);

    m_Grid.resize(static_cast<size_t>(m_GridResX) * m_GridResZ);
    m_WallGrid.resize(static_cast<size_t>(m_GridResX) * m_GridResZ);

    // ── 4. Bin triangles into grid cells ────────────────────────────────
    for (size_t ti = 0; ti < m_Triangles.size(); ++ti) {
        const auto &tri = m_Triangles[ti];

        // Triangle XZ AABB
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
                m_Grid[static_cast<size_t>(gz) * m_GridResX + gx].push_back(ti);
            }
        }
    }

    // ── 5. Bin wall triangles into grid cells ───────────────────────────
    for (size_t ti = 0; ti < m_WallTriangles.size(); ++ti) {
        const auto &tri = m_WallTriangles[ti];

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
                m_WallGrid[static_cast<size_t>(gz) * m_GridResX + gx].push_back(ti);
            }
        }
    }

    m_Built = true;
    LOG_INFO("MapCollider: grid {}x{} (cell {:.2f}x{:.2f} m), bounds X[{:.1f},{:.1f}] Z[{:.1f},{:.1f}]",
             m_GridResX, m_GridResZ, m_CellSizeX, m_CellSizeZ,
             m_MinX, m_MaxX, m_MinZ, m_MaxZ);
}

// ============================================================================
//  GridIndex — convert world XZ to flat grid index (-1 if out of bounds)
// ============================================================================
int MapCollider::GridIndex(float x, float z) const {
    if (x < m_MinX || x > m_MaxX || z < m_MinZ || z > m_MaxZ) return -1;

    int gx = static_cast<int>((x - m_MinX) / m_CellSizeX);
    int gz = static_cast<int>((z - m_MinZ) / m_CellSizeZ);
    gx = std::clamp(gx, 0, m_GridResX - 1);
    gz = std::clamp(gz, 0, m_GridResZ - 1);

    return gz * m_GridResX + gx;
}

// ============================================================================
//  RayTriangleIntersectGeneric — Möller-Trumbore for any ray direction
// ============================================================================
bool MapCollider::RayTriangleIntersectGeneric(const glm::vec3 &origin,
                                              const glm::vec3 &dir,
                                              const Triangle &tri,
                                              float &outT) {
    constexpr float EPSILON = 1e-6f;

    glm::vec3 edge1 = tri.v1 - tri.v0;
    glm::vec3 edge2 = tri.v2 - tri.v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) return false;

    float f = 1.0f / a;
    glm::vec3 s = origin - tri.v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * glm::dot(edge2, q);
    if (t > EPSILON) {
        outT = t;
        return true;
    }
    return false;
}

// ============================================================================
//  IsWallBlocking — horizontal segment vs wall triangles
// ============================================================================
bool MapCollider::IsWallBlocking(float x1, float z1, float x2, float z2,
                                 float minY, float maxY) const {
    if (!m_Built) return false;

    // Horizontal direction from center to sample point
    float dx = x2 - x1;
    float dz = z2 - z1;
    float segLen = std::sqrt(dx * dx + dz * dz);
    if (segLen < 1e-6f) return false;

    // Collect wall triangle candidates from cells along the segment
    // Sample several heights within the body column
    constexpr int HEIGHT_SAMPLES = 3;
    float heights[HEIGHT_SAMPLES];
    for (int i = 0; i < HEIGHT_SAMPLES; ++i) {
        heights[i] = minY + (maxY - minY) * static_cast<float>(i) / static_cast<float>(HEIGHT_SAMPLES - 1);
    }

    // Gather unique cell indices along the segment
    auto getCellsForSegment = [&](float sx, float sz, float ex, float ez,
                                  std::vector<int> &cells) {
        constexpr int STEPS = 4;
        for (int s = 0; s <= STEPS; ++s) {
            float t = static_cast<float>(s) / static_cast<float>(STEPS);
            float px = sx + (ex - sx) * t;
            float pz = sz + (ez - sz) * t;
            int idx = GridIndex(px, pz);
            if (idx >= 0) {
                bool found = false;
                for (int c : cells) { if (c == idx) { found = true; break; } }
                if (!found) cells.push_back(idx);
            }
        }
    };

    std::vector<int> cells;
    cells.reserve(8);
    getCellsForSegment(x1, z1, x2, z2, cells);

    // For each height sample, cast a horizontal ray and check wall triangles
    for (float y : heights) {
        glm::vec3 origin(x1, y, z1);
        glm::vec3 dir(dx, 0.0f, dz); // not normalized; length = segLen

        glm::vec3 dirNorm = dir / segLen;

        for (int cellIdx : cells) {
            for (size_t ti : m_WallGrid[static_cast<size_t>(cellIdx)]) {
                const auto &tri = m_WallTriangles[ti];

                // Quick Y-range AABB check
                float triMinY = std::min({tri.v0.y, tri.v1.y, tri.v2.y});
                float triMaxY = std::max({tri.v0.y, tri.v1.y, tri.v2.y});
                if (triMaxY < minY || triMinY > maxY) continue;

                float t = 0.0f;
                if (RayTriangleIntersectGeneric(origin, dirNorm, tri, t)) {
                    if (t <= segLen) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// ============================================================================
//  RayTriangleIntersect — Möller-Trumbore for a downward ray (0, -1, 0)
// ============================================================================
bool MapCollider::RayTriangleIntersect(const glm::vec3 &origin,
                                       const Triangle &tri,
                                       float &outT) {
    constexpr float EPSILON = 1e-6f;
    const glm::vec3 dir(0.0f, -1.0f, 0.0f); // downward ray

    glm::vec3 edge1 = tri.v1 - tri.v0;
    glm::vec3 edge2 = tri.v2 - tri.v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) return false; // parallel

    float f = 1.0f / a;
    glm::vec3 s = origin - tri.v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * glm::dot(edge2, q);
    if (t > EPSILON) { // intersection ahead of ray origin
        outT = t;
        return true;
    }
    return false;
}

// ============================================================================
//  GetGroundHeight — find highest ground surface below (x, ?, z)
// ============================================================================
float MapCollider::GetGroundHeight(float x, float z, float maxY) const {
    if (!m_Built) return -std::numeric_limits<float>::infinity();

    int idx = GridIndex(x, z);
    if (idx < 0) return -std::numeric_limits<float>::infinity();

    const glm::vec3 origin(x, maxY, z);
    float bestY = -std::numeric_limits<float>::infinity();

    for (size_t ti : m_Grid[static_cast<size_t>(idx)]) {
        float t = 0.0f;
        if (RayTriangleIntersect(origin, m_Triangles[ti], t)) {
            // hit point Y = origin.y + dir.y * t = maxY - t
            float hitY = maxY - t;
            if (hitY > bestY) {
                bestY = hitY;
            }
        }
    }

    return bestY;
}

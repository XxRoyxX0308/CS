#include "Navigation/NavMesh.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Util/Logger.hpp"

#include <cmath>
#include <limits>

namespace Navigation {

void NavMesh::Build(const Physics::CollisionMesh& mesh) {
    m_Nodes.clear();
    m_GridToNode.clear();
    m_Built = false;

    if (!mesh.IsBuilt()) {
        LOG_WARN("NavMesh::Build called on unbuilt CollisionMesh");
        return;
    }

    // Determine XZ bounds from the collision mesh triangles
    const auto& triangles = mesh.GetTriangles();
    if (triangles.empty()) return;

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& tri : triangles) {
        for (const auto& v : {tri.v0, tri.v1, tri.v2}) {
            minX = std::min(minX, v.x);
            maxX = std::max(maxX, v.x);
            minZ = std::min(minZ, v.z);
            maxZ = std::max(maxZ, v.z);
        }
    }

    // Pad bounds slightly
    minX -= 1.0f;
    maxX += 1.0f;
    minZ -= 1.0f;
    maxZ += 1.0f;

    m_MinX = minX;
    m_MinZ = minZ;
    m_GridResX = static_cast<int>(std::ceil((maxX - minX) / CELL_SIZE));
    m_GridResZ = static_cast<int>(std::ceil((maxZ - minZ) / CELL_SIZE));

    // Clamp grid resolution to avoid excessive memory
    const int MAX_RES = 512;
    if (m_GridResX > MAX_RES) m_GridResX = MAX_RES;
    if (m_GridResZ > MAX_RES) m_GridResZ = MAX_RES;

    const int totalCells = m_GridResX * m_GridResZ;
    m_GridToNode.assign(totalCells, -1);

    LOG_INFO("NavMesh: sampling {}x{} grid ({} cells), cell size {:.1f}m",
             m_GridResX, m_GridResZ, totalCells, CELL_SIZE);

    // Phase 1: Sample walkable positions
    Physics::Capsule probeCap;
    probeCap.radius = PROBE_RADIUS;
    probeCap.height = PROBE_CAP_HEIGHT - 2.0f * PROBE_RADIUS;
    if (probeCap.height < 0.0f) probeCap.height = 0.0f;

    for (int gz = 0; gz < m_GridResZ; ++gz) {
        for (int gx = 0; gx < m_GridResX; ++gx) {
            float worldX = m_MinX + (static_cast<float>(gx) + 0.5f) * CELL_SIZE;
            float worldZ = m_MinZ + (static_cast<float>(gz) + 0.5f) * CELL_SIZE;

            // Place capsule high up and sweep down
            probeCap.base = glm::vec3(worldX, PROBE_HEIGHT, worldZ);
            auto groundY = Physics::CapsuleCast::SweepVertical(
                probeCap, mesh, -(PROBE_HEIGHT + 60.0f));

            if (!groundY.has_value()) continue;

            float feetY = groundY.value();
            // Skip positions that are too low (fallen off the map)
            if (feetY < -40.0f) continue;

            NavNode node;
            node.position = glm::vec3(worldX, feetY + PROBE_CAP_HEIGHT, worldZ);

            int cellIdx = gz * m_GridResX + gx;
            m_GridToNode[cellIdx] = static_cast<int>(m_Nodes.size());
            m_Nodes.push_back(node);
        }
    }

    LOG_INFO("NavMesh: found {} walkable nodes", m_Nodes.size());

    // Phase 2: Connect neighbors (8-directional)
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dz[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    size_t edgeCount = 0;
    for (int gz = 0; gz < m_GridResZ; ++gz) {
        for (int gx = 0; gx < m_GridResX; ++gx) {
            int cellIdx = gz * m_GridResX + gx;
            int nodeIdx = m_GridToNode[cellIdx];
            if (nodeIdx < 0) continue;

            NavNode& node = m_Nodes[static_cast<size_t>(nodeIdx)];

            for (int d = 0; d < 8; ++d) {
                int nx = gx + dx[d];
                int nz = gz + dz[d];
                if (nx < 0 || nx >= m_GridResX || nz < 0 || nz >= m_GridResZ) continue;

                int neighborCellIdx = nz * m_GridResX + nx;
                int neighborNodeIdx = m_GridToNode[neighborCellIdx];
                if (neighborNodeIdx < 0) continue;

                const NavNode& neighbor = m_Nodes[static_cast<size_t>(neighborNodeIdx)];

                // Check Y difference (slope limit)
                float yDiff = std::abs(node.position.y - neighbor.position.y);
                if (yDiff > MAX_SLOPE_Y_DIFF) continue;

                // Check traversability (no cliffs/gaps between nodes)
                if (!CanTraverse(node.position, neighbor.position, mesh)) continue;

                float dist = glm::distance(node.position, neighbor.position);
                node.neighbors.push_back(static_cast<size_t>(neighborNodeIdx));
                node.neighborDistances.push_back(dist);
                ++edgeCount;
            }
        }
    }

    m_Built = true;
    LOG_INFO("NavMesh: built with {} nodes, {} edges", m_Nodes.size(), edgeCount);
}

bool NavMesh::CanTraverse(const glm::vec3& from, const glm::vec3& to,
                          const Physics::CollisionMesh& mesh) const {
    glm::vec3 dir = to - from;
    float totalDist = glm::length(dir);
    if (totalDist < 0.01f) return true;

    dir /= totalDist;
    int steps = static_cast<int>(totalDist / TRAVERSE_STEP);
    if (steps < 1) steps = 1;

    Physics::Capsule probeCap;
    probeCap.radius = PROBE_RADIUS;
    probeCap.height = PROBE_CAP_HEIGHT - 2.0f * PROBE_RADIUS;
    if (probeCap.height < 0.0f) probeCap.height = 0.0f;

    float prevY = from.y;

    for (int i = 1; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        glm::vec3 samplePos = glm::mix(from, to, t);

        // Probe downward from slightly above
        probeCap.base = glm::vec3(samplePos.x, samplePos.y + 2.0f, samplePos.z);
        auto groundY = Physics::CapsuleCast::SweepVertical(
            probeCap, mesh, -6.0f);

        if (!groundY.has_value()) return false;

        float stepY = groundY.value() + PROBE_CAP_HEIGHT;
        if (std::abs(stepY - prevY) > MAX_STEP_HEIGHT) return false;
        prevY = stepY;
    }

    return true;
}

size_t NavMesh::FindNearestNode(const glm::vec3& pos) const {
    if (m_Nodes.empty()) return SIZE_MAX;

    // Try grid-based lookup first for fast approximate result
    int gx = static_cast<int>((pos.x - m_MinX) / CELL_SIZE);
    int gz = static_cast<int>((pos.z - m_MinZ) / CELL_SIZE);

    // Search in expanding rings around the grid cell
    size_t bestIdx = SIZE_MAX;
    float bestDist = std::numeric_limits<float>::max();

    int searchRadius = 3;
    for (int r = 0; r <= searchRadius; ++r) {
        for (int dz = -r; dz <= r; ++dz) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) != r && std::abs(dz) != r && r > 0) continue;
                int cx = gx + dx;
                int cz = gz + dz;
                if (cx < 0 || cx >= m_GridResX || cz < 0 || cz >= m_GridResZ) continue;

                int nodeIdx = m_GridToNode[cz * m_GridResX + cx];
                if (nodeIdx < 0) continue;

                float dist = glm::distance(pos, m_Nodes[static_cast<size_t>(nodeIdx)].position);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = static_cast<size_t>(nodeIdx);
                }
            }
        }
        if (bestIdx != SIZE_MAX && r >= 1) break;
    }

    // If grid search failed, do brute force
    if (bestIdx == SIZE_MAX) {
        for (size_t i = 0; i < m_Nodes.size(); ++i) {
            float dist = glm::distance(pos, m_Nodes[i].position);
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = i;
            }
        }
    }

    return bestIdx;
}

std::vector<glm::vec3> NavMesh::FindPath(const glm::vec3& start,
                                         const glm::vec3& goal) const {
    if (!m_Built || m_Nodes.empty()) return {};

    size_t startIdx = FindNearestNode(start);
    size_t goalIdx = FindNearestNode(goal);

    if (startIdx == SIZE_MAX || goalIdx == SIZE_MAX) return {};
    if (startIdx == goalIdx) {
        return {m_Nodes[startIdx].position};
    }

    auto nodeIndices = PathFinder::Search(m_Nodes, startIdx, goalIdx);
    if (nodeIndices.empty()) return {};

    std::vector<glm::vec3> waypoints;
    waypoints.reserve(nodeIndices.size());
    for (size_t idx : nodeIndices) {
        waypoints.push_back(m_Nodes[idx].position);
    }

    return waypoints;
}

} // namespace Navigation

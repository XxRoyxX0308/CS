#include "Navigation/NavMesh.hpp"
#include "Physics/CapsuleCast.hpp"
#include "Util/Logger.hpp"

#include <cmath>
#include <limits>

namespace Navigation {

namespace {

constexpr float kLocalAnchorProbeDistance = 6.0f;
constexpr float kLocalAnchorMaxRise = 0.25f;
constexpr float kDuplicateWaypointDistance = 0.05f;
constexpr float kMaxSmoothLookaheadDistance = 12.0f;
constexpr size_t kMaxSmoothLookaheadNodes = 10;

void AppendDistinctWaypoint(std::vector<glm::vec3>& waypoints,
                            const glm::vec3& waypoint) {
    if (!waypoints.empty() &&
        glm::distance(waypoints.back(), waypoint) <= kDuplicateWaypointDistance) {
        return;
    }

    waypoints.push_back(waypoint);
}

} // namespace

// ============================================================================
//  Build - Sample walkable nodes and connect the graph
// ============================================================================
void NavMesh::Build(const Physics::CollisionMesh& mesh) {
    m_Nodes.clear();
    m_GridToNodes.clear();
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
    m_GridToNodes.resize(totalCells);

    LOG_INFO("NavMesh: sampling {}x{} grid ({} cells), cell size {:.1f}m",
             m_GridResX, m_GridResZ, totalCells, CELL_SIZE);

    // Phase 1: Sample walkable positions (multi-layer)
    Physics::Capsule probeCap;
    probeCap.radius = PROBE_RADIUS;
    probeCap.height = PROBE_CAP_HEIGHT - 2.0f * PROBE_RADIUS;
    if (probeCap.height < 0.0f) probeCap.height = 0.0f;

    for (int gz = 0; gz < m_GridResZ; ++gz) {
        for (int gx = 0; gx < m_GridResX; ++gx) {
            float worldX = m_MinX + (static_cast<float>(gx) + 0.5f) * CELL_SIZE;
            float worldZ = m_MinZ + (static_cast<float>(gz) + 0.5f) * CELL_SIZE;
            int cellIdx = gz * m_GridResX + gx;

            // Probe downward from the top, finding multiple ground layers
            float probeTop = PROBE_HEIGHT;
            for (int layer = 0; layer < MAX_LAYERS; ++layer) {
                probeCap.base = glm::vec3(worldX, probeTop, worldZ);
                float sweepDist = -(probeTop + 60.0f);
                auto groundY = Physics::CapsuleCast::SweepVertical(
                    probeCap, mesh, sweepDist);

                if (!groundY.has_value()) break;

                float feetY = groundY.value();
                if (feetY < -40.0f) break;

                NavNode node;
                node.position = glm::vec3(worldX, feetY + PROBE_CAP_HEIGHT, worldZ);

                m_GridToNodes[cellIdx].push_back(static_cast<int>(m_Nodes.size()));
                m_Nodes.push_back(node);

                // Move probe below this surface for the next layer
                // Go below the found ground by MIN_LAYER_GAP to skip thin surfaces
                probeTop = feetY - MIN_LAYER_GAP;
                if (probeTop < -40.0f) break;
            }
        }
    }

    LOG_INFO("NavMesh: found {} walkable nodes", m_Nodes.size());

    // Phase 2: Connect neighbors (8-directional, across layers)
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dz[] = {-1, -1, -1, 0, 0, 1, 1, 1};

    size_t edgeCount = 0;
    for (int gz = 0; gz < m_GridResZ; ++gz) {
        for (int gx = 0; gx < m_GridResX; ++gx) {
            int cellIdx = gz * m_GridResX + gx;
            const auto& cellNodes = m_GridToNodes[cellIdx];

            for (int nodeIdx : cellNodes) {
                NavNode& node = m_Nodes[static_cast<size_t>(nodeIdx)];

                for (int d = 0; d < 8; ++d) {
                    int nx = gx + dx[d];
                    int nz = gz + dz[d];
                    if (nx < 0 || nx >= m_GridResX || nz < 0 || nz >= m_GridResZ) continue;

                    int neighborCellIdx = nz * m_GridResX + nx;
                    const auto& neighborNodes = m_GridToNodes[neighborCellIdx];

                    // Find the best matching neighbor by Y proximity
                    for (int neighborNodeIdx : neighborNodes) {
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
        }
    }

    m_Built = true;
    LOG_INFO("NavMesh: built with {} nodes, {} edges", m_Nodes.size(), edgeCount);
}

// ============================================================================
//  CanTraverse - Validate ground continuity and mid-segment blockers
// ============================================================================
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

        auto stepY = SampleGroundAnchorY(samplePos,
                                         prevY + MAX_STEP_HEIGHT,
                                         6.0f,
                                         mesh);
        if (!stepY.has_value()) {
            LOG_DEBUG(
                "NavMesh: missing traversable ground from ({:.1f}, {:.1f}, {:.1f}) to ({:.1f}, {:.1f}, {:.1f}) at sample ({:.1f}, {:.1f}, {:.1f})",
                from.x, from.y, from.z,
                to.x, to.y, to.z,
                samplePos.x, samplePos.y, samplePos.z);
            return false;
        }

        float stepAnchorY = stepY.value();
        if (std::abs(stepAnchorY - prevY) > MAX_STEP_HEIGHT) return false;
        prevY = stepAnchorY;
    }

    // Check for thin walls / doors that block the horizontal path between the
    // two nodes. The original full-height capsule test was too aggressive and
    // severed many valid edges because start/end-point brushes also counted as
    // blockers. Use a small torso-height probe and only reject mid-segment
    // hits with near-vertical normals.
    {
        static constexpr float WALL_CHECK_RADIUS = 0.12f;
        static constexpr float WALL_CHECK_HEIGHT_ABOVE_FEET = 0.75f;
        static constexpr float WALL_BLOCK_MIN_T = 0.05f;
        static constexpr float WALL_BLOCK_MAX_T = 0.95f;
        static constexpr float WALL_NORMAL_Y_LIMIT = 0.3f;
        static constexpr float WALL_POINT_Y_TOLERANCE = 0.22f;

        Physics::Capsule wallCap;
        wallCap.radius = WALL_CHECK_RADIUS;
        wallCap.height = 0.0f; // sphere-like probe reduces false positives

        float feetY = from.y - PROBE_CAP_HEIGHT;
        wallCap.base = glm::vec3(from.x, feetY + WALL_CHECK_HEIGHT_ABOVE_FEET, from.z);
        glm::vec3 horizVel(to.x - from.x, 0.0f, to.z - from.z);
        auto wallHit = Physics::CapsuleCast::SweepCapsule(wallCap, horizVel, mesh);
        if (wallHit.hit &&
            wallHit.t > WALL_BLOCK_MIN_T &&
            wallHit.t < WALL_BLOCK_MAX_T &&
            std::abs(wallHit.normal.y) < WALL_NORMAL_Y_LIMIT &&
            std::abs(wallHit.point.y - wallCap.base.y) <= WALL_POINT_Y_TOLERANCE) {
            return false;
        }
    }

    return true;
}

std::optional<float> NavMesh::SampleGroundAnchorY(const glm::vec3& referencePos,
                                                  float maxAcceptedAnchorY,
                                                  float probeDistance,
                                                  const Physics::CollisionMesh& mesh) const {
    static constexpr float PROBE_BASE_OFFSETS[] = {0.15f, -0.15f, -0.45f, -0.75f};

    Physics::Capsule probeCap;
    probeCap.radius = PROBE_RADIUS;
    probeCap.height = PROBE_CAP_HEIGHT - 2.0f * PROBE_RADIUS;
    if (probeCap.height < 0.0f) probeCap.height = 0.0f;

    std::optional<float> bestAnchorY;
    for (float baseOffset : PROBE_BASE_OFFSETS) {
        probeCap.base = glm::vec3(referencePos.x,
                                  referencePos.y + baseOffset,
                                  referencePos.z);
        auto groundY = Physics::CapsuleCast::SweepVertical(probeCap, mesh, -probeDistance);
        if (!groundY.has_value()) {
            continue;
        }

        float anchorY = groundY.value() + PROBE_CAP_HEIGHT;
        if (anchorY > maxAcceptedAnchorY) {
            continue;
        }

        if (!bestAnchorY.has_value() || anchorY > *bestAnchorY) {
            bestAnchorY = anchorY;
        }
    }

    return bestAnchorY;
}

// ============================================================================
//  FindNearestNode - Find nearest node by distance only
// ============================================================================
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

                const auto& cellNodes = m_GridToNodes[cz * m_GridResX + cx];
                for (int nodeIdx : cellNodes) {
                    float dist = glm::distance(pos, m_Nodes[static_cast<size_t>(nodeIdx)].position);
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestIdx = static_cast<size_t>(nodeIdx);
                    }
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

// ============================================================================
//  FindNearestConnectedNode - Prefer locally reachable same-layer anchors
// ============================================================================
size_t NavMesh::FindNearestConnectedNode(const glm::vec3& pos,
                                         const Physics::CollisionMesh& mesh) const {
    if (m_Nodes.empty()) return SIZE_MAX;

    static constexpr float PREFERRED_LAYER_Y_DIFF = 1.25f;
    static constexpr float FALLBACK_LAYER_Y_DIFF = 1.75f;

    // Search a slightly wider local area than FindNearestNode and prefer nodes
    // that are directly traversable from the query position. This avoids
    // latching onto a node that is geometrically close but separated by a wall.
    int gx = static_cast<int>((pos.x - m_MinX) / CELL_SIZE);
    int gz = static_cast<int>((pos.z - m_MinZ) / CELL_SIZE);

    const std::optional<glm::vec3> localAnchor = SampleLocalAnchor(
        pos, kLocalAnchorMaxRise, kLocalAnchorProbeDistance, mesh);
    const glm::vec3 referenceAnchor = localAnchor.value_or(pos);

    auto searchLocal = [&](bool preferSameLayer) -> size_t {
        size_t bestIdx = SIZE_MAX;
        float bestScore = std::numeric_limits<float>::max();

        auto considerNode = [&](size_t nodeIdx) {
            const glm::vec3& nodePos = m_Nodes[nodeIdx].position;
            if (preferSameLayer &&
                std::abs(referenceAnchor.y - nodePos.y) > PREFERRED_LAYER_Y_DIFF) {
                return;
            }

            glm::vec3 anchor = referenceAnchor;
            if (!CanTraverse(anchor, nodePos, mesh)) {
                return;
            }

            float score = glm::distance(anchor, nodePos);
            if (score < bestScore) {
                bestScore = score;
                bestIdx = nodeIdx;
            }
        };

        int searchRadius = 5;
        for (int r = 0; r <= searchRadius; ++r) {
            for (int dz = -r; dz <= r; ++dz) {
                for (int dx = -r; dx <= r; ++dx) {
                    if (std::abs(dx) != r && std::abs(dz) != r && r > 0) continue;
                    int cx = gx + dx;
                    int cz = gz + dz;
                    if (cx < 0 || cx >= m_GridResX || cz < 0 || cz >= m_GridResZ) continue;

                    const auto& cellNodes = m_GridToNodes[cz * m_GridResX + cx];
                    for (int nodeIdx : cellNodes) {
                        considerNode(static_cast<size_t>(nodeIdx));
                    }
                }
            }

            if (bestIdx != SIZE_MAX && r >= 1) {
                return bestIdx;
            }
        }

        return bestIdx;
    };

    size_t bestIdx = searchLocal(true);
    if (bestIdx != SIZE_MAX) {
        return bestIdx;
    }

    bestIdx = searchLocal(false);
    if (bestIdx != SIZE_MAX) {
        return bestIdx;
    }

    // Final fallback: keep a nearest-node escape hatch for slightly off-mesh
    // positions, but do not silently jump to a far-away vertical layer.
    size_t fallbackIdx = FindNearestNode(referenceAnchor);
    if (fallbackIdx == SIZE_MAX) {
        return SIZE_MAX;
    }

    if (!localAnchor.has_value()) {
        return fallbackIdx;
    }

    const glm::vec3& fallbackPos = m_Nodes[fallbackIdx].position;
    const float fallbackYDiff = std::abs(referenceAnchor.y - fallbackPos.y);
    if (fallbackYDiff > FALLBACK_LAYER_Y_DIFF ||
        !CanTraverse(*localAnchor, fallbackPos, mesh)) {
        LOG_DEBUG(
            "NavMesh: rejecting disconnected fallback node for query ({:.1f}, {:.1f}, {:.1f}) -> node ({:.1f}, {:.1f}, {:.1f}), layer_dy={:.2f}",
            pos.x, pos.y, pos.z,
            fallbackPos.x, fallbackPos.y, fallbackPos.z,
            fallbackYDiff);
        return SIZE_MAX;
    }

    return fallbackIdx;
}

std::optional<glm::vec3> NavMesh::SampleLocalAnchor(
    const glm::vec3& referencePos,
    float maxRise,
    float probeDistance,
    const Physics::CollisionMesh& mesh) const {
    auto anchorY = SampleGroundAnchorY(referencePos,
                                       referencePos.y + maxRise,
                                       probeDistance,
                                       mesh);
    if (!anchorY.has_value()) {
        return std::nullopt;
    }

    return glm::vec3(referencePos.x, anchorY.value(), referencePos.z);
}

std::vector<glm::vec3> NavMesh::SmoothPath(
    const std::vector<glm::vec3>& rawWaypoints,
    const Physics::CollisionMesh& mesh) const {
    if (rawWaypoints.size() <= 2) {
        return rawWaypoints;
    }

    std::vector<glm::vec3> waypoints;
    waypoints.reserve(rawWaypoints.size());
    AppendDistinctWaypoint(waypoints, rawWaypoints.front());

    size_t anchorIndex = 0;
    while (anchorIndex + 1 < rawWaypoints.size()) {
        size_t bestReachableIndex = anchorIndex + 1;
        size_t candidateIndex = anchorIndex + 2;
        while (candidateIndex < rawWaypoints.size()) {
            if (candidateIndex - anchorIndex > kMaxSmoothLookaheadNodes) {
                break;
            }

            if (glm::distance(rawWaypoints[anchorIndex], rawWaypoints[candidateIndex]) >
                kMaxSmoothLookaheadDistance) {
                break;
            }

            if (!CanTraverse(rawWaypoints[anchorIndex], rawWaypoints[candidateIndex], mesh)) {
                break;
            }

            bestReachableIndex = candidateIndex;
            ++candidateIndex;
        }

        AppendDistinctWaypoint(waypoints, rawWaypoints[bestReachableIndex]);
        anchorIndex = bestReachableIndex;
    }

    return waypoints;
}

// ============================================================================
//  FindPath - Convert an A* node path into world-space waypoints
// ============================================================================
std::vector<glm::vec3> NavMesh::FindPath(const glm::vec3& start,
                                         const glm::vec3& goal,
                                         const Physics::CollisionMesh& mesh) const {
    if (!m_Built || m_Nodes.empty()) return {};

    size_t startIdx = FindNearestConnectedNode(start, mesh);
    size_t goalIdx = FindNearestConnectedNode(goal, mesh);

    if (startIdx == SIZE_MAX || goalIdx == SIZE_MAX) {
        LOG_DEBUG(
            "NavMesh: failed to resolve local nodes for path start ({:.1f}, {:.1f}, {:.1f}) -> goal ({:.1f}, {:.1f}, {:.1f}), start_idx={}, goal_idx={}",
            start.x, start.y, start.z,
            goal.x, goal.y, goal.z,
            startIdx == SIZE_MAX ? -1 : static_cast<int>(startIdx),
            goalIdx == SIZE_MAX ? -1 : static_cast<int>(goalIdx));
        return {};
    }

    auto nodeIndices = PathFinder::Search(m_Nodes, startIdx, goalIdx);
    if (nodeIndices.empty()) {
        LOG_DEBUG(
            "NavMesh: graph search failed start_idx={} neighbors={} at ({:.1f}, {:.1f}, {:.1f}), goal_idx={} neighbors={} at ({:.1f}, {:.1f}, {:.1f})",
            static_cast<int>(startIdx),
            static_cast<int>(m_Nodes[startIdx].neighbors.size()),
            m_Nodes[startIdx].position.x, m_Nodes[startIdx].position.y, m_Nodes[startIdx].position.z,
            static_cast<int>(goalIdx),
            static_cast<int>(m_Nodes[goalIdx].neighbors.size()),
            m_Nodes[goalIdx].position.x, m_Nodes[goalIdx].position.y, m_Nodes[goalIdx].position.z);
        return {};
    }

    const glm::vec3 startAnchor = SampleLocalAnchor(
        start, kLocalAnchorMaxRise, kLocalAnchorProbeDistance, mesh).value_or(m_Nodes[startIdx].position);
    const glm::vec3 goalAnchor = SampleLocalAnchor(
        goal, kLocalAnchorMaxRise, kLocalAnchorProbeDistance, mesh).value_or(m_Nodes[goalIdx].position);

    std::vector<glm::vec3> rawWaypoints;
    rawWaypoints.reserve(nodeIndices.size() + 2);
    AppendDistinctWaypoint(rawWaypoints, startAnchor);
    for (size_t idx : nodeIndices) {
        AppendDistinctWaypoint(rawWaypoints, m_Nodes[idx].position);
    }
    AppendDistinctWaypoint(rawWaypoints, goalAnchor);

    std::vector<glm::vec3> waypoints = SmoothPath(rawWaypoints, mesh);
    if (waypoints.size() != rawWaypoints.size()) {
        LOG_DEBUG(
            "NavMesh: smoothed path from {} raw waypoints to {} between ({:.1f}, {:.1f}, {:.1f}) and ({:.1f}, {:.1f}, {:.1f})",
            rawWaypoints.size(), waypoints.size(),
            start.x, start.y, start.z,
            goal.x, goal.y, goal.z);
    }

    return waypoints;
}

} // namespace Navigation

#include "Navigation/PathFinder.hpp"

#include <queue>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Navigation {

// ============================================================================
//  Heuristic - Euclidean distance for A* search
// ============================================================================
float PathFinder::Heuristic(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 diff = a - b;
    return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

// ============================================================================
//  Search - Find a shortest path through the NavNode graph
// ============================================================================
std::vector<size_t> PathFinder::Search(const std::vector<NavNode>& nodes,
                                       size_t startIdx,
                                       size_t goalIdx) {
    if (startIdx >= nodes.size() || goalIdx >= nodes.size()) {
        return {};
    }
    if (startIdx == goalIdx) {
        return {startIdx};
    }

    struct OpenEntry {
        float fScore;
        size_t nodeIdx;
        bool operator>(const OpenEntry& other) const { return fScore > other.fScore; }
    };

    std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> openSet;

    const size_t nodeCount = nodes.size();
    std::vector<float> gScore(nodeCount, std::numeric_limits<float>::infinity());
    std::vector<size_t> cameFrom(nodeCount, SIZE_MAX);
    std::vector<bool> closed(nodeCount, false);

    gScore[startIdx] = 0.0f;
    openSet.push({Heuristic(nodes[startIdx].position, nodes[goalIdx].position), startIdx});

    while (!openSet.empty()) {
        OpenEntry current = openSet.top();
        openSet.pop();

        if (closed[current.nodeIdx]) {
            continue;
        }

        if (current.nodeIdx == goalIdx) {
            // Reconstruct path
            std::vector<size_t> path;
            size_t node = goalIdx;
            while (node != startIdx) {
                path.push_back(node);
                node = cameFrom[node];
                if (node == SIZE_MAX) {
                    return {};
                }
            }
            path.push_back(startIdx);
            std::reverse(path.begin(), path.end());
            return path;
        }

        float currentG = gScore[current.nodeIdx];
        float currentH = Heuristic(nodes[current.nodeIdx].position,
                                   nodes[goalIdx].position);

        // Skip stale entries
        if (currentG < current.fScore - currentH - 0.001f) {
            continue;
        }

        closed[current.nodeIdx] = true;

        const NavNode& currentNode = nodes[current.nodeIdx];
        for (size_t i = 0; i < currentNode.neighbors.size(); ++i) {
            size_t neighborIdx = currentNode.neighbors[i];
            if (closed[neighborIdx]) {
                continue;
            }

            float edgeCost = currentNode.neighborDistances[i];
            float tentativeG = currentG + edgeCost;

            if (tentativeG < gScore[neighborIdx]) {
                gScore[neighborIdx] = tentativeG;
                cameFrom[neighborIdx] = current.nodeIdx;
                float f = tentativeG + Heuristic(nodes[neighborIdx].position,
                                                 nodes[goalIdx].position);
                openSet.push({f, neighborIdx});
            }
        }
    }

    return {}; // No path found
}

} // namespace Navigation

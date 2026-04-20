#include "Navigation/PathFinder.hpp"

#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Navigation {

float PathFinder::Heuristic(const glm::vec3& a, const glm::vec3& b) {
    glm::vec3 diff = a - b;
    return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

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

    std::unordered_map<size_t, float> gScore;
    std::unordered_map<size_t, size_t> cameFrom;

    gScore[startIdx] = 0.0f;
    openSet.push({Heuristic(nodes[startIdx].position, nodes[goalIdx].position), startIdx});

    while (!openSet.empty()) {
        OpenEntry current = openSet.top();
        openSet.pop();

        if (current.nodeIdx == goalIdx) {
            // Reconstruct path
            std::vector<size_t> path;
            size_t node = goalIdx;
            while (node != startIdx) {
                path.push_back(node);
                node = cameFrom[node];
            }
            path.push_back(startIdx);
            std::reverse(path.begin(), path.end());
            return path;
        }

        float currentG = gScore[current.nodeIdx];

        // Skip stale entries
        if (currentG < current.fScore - Heuristic(nodes[current.nodeIdx].position,
                                                    nodes[goalIdx].position) - 0.001f) {
            continue;
        }

        const NavNode& currentNode = nodes[current.nodeIdx];
        for (size_t i = 0; i < currentNode.neighbors.size(); ++i) {
            size_t neighborIdx = currentNode.neighbors[i];
            float edgeCost = currentNode.neighborDistances[i];
            float tentativeG = currentG + edgeCost;

            auto it = gScore.find(neighborIdx);
            if (it == gScore.end() || tentativeG < it->second) {
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

#ifndef CS_NAVIGATION_PATHFINDER_HPP
#define CS_NAVIGATION_PATHFINDER_HPP

#include <glm/glm.hpp>
#include <vector>
#include <cstddef>

namespace Navigation {

/**
 * @brief A node in the navigation mesh graph.
 *
 * Each node represents a walkable position in world space
 * with connections to neighboring walkable nodes.
 */
struct NavNode {
    glm::vec3 position{0.0f};
    std::vector<size_t> neighbors;
    std::vector<float> neighborDistances;
};

/**
 * @brief A* pathfinding algorithm operating on a NavNode graph.
 *
 * This class is stateless — all methods are static.
 * It finds the shortest path through a graph of NavNodes using
 * the A* algorithm with Euclidean distance as the heuristic.
 */
class PathFinder {
public:
    PathFinder() = delete;

    /**
     * @brief Find the shortest path between two nodes using A*.
     * @param nodes  The navigation graph (vector of NavNodes).
     * @param startIdx  Index of the starting node.
     * @param goalIdx   Index of the goal node.
     * @return Ordered list of node indices from start to goal (inclusive),
     *         or empty vector if no path exists.
     */
    static std::vector<size_t> Search(const std::vector<NavNode>& nodes,
                                      size_t startIdx,
                                      size_t goalIdx);

private:
    static float Heuristic(const glm::vec3& a, const glm::vec3& b);
};

} // namespace Navigation

#endif // CS_NAVIGATION_PATHFINDER_HPP

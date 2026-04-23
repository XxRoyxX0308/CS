#ifndef CS_NAVIGATION_NAVMESH_HPP
#define CS_NAVIGATION_NAVMESH_HPP

#include "Navigation/PathFinder.hpp"
#include "Physics/CollisionMesh.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstddef>

namespace Navigation {

/**
 * @brief Navigation mesh built by grid-sampling walkable surfaces.
 *
 * Samples the world XZ plane at a fixed interval, probes downward
 * using CapsuleCast to find ground, and builds a graph of walkable
 * NavNodes connected to their 8-directional neighbors.
 *
 * Used by BotPlayer for A* pathfinding toward targets.
 */
class NavMesh {
public:
    NavMesh() = default;

    /**
     * @brief Build the navigation mesh from a collision mesh.
     *
     * Iterates the XZ bounding box of the collision mesh at CELL_SIZE
     * intervals, sweeps a capsule downward from a high probe point to
     * find ground, and connects adjacent walkable nodes.
     *
     * @param mesh The built collision mesh of the map.
     */
    void Build(const Physics::CollisionMesh& mesh);

    /**
     * @brief Find the nearest navigation node to a world position.
     * @param pos World-space position.
     * @return Index into the nodes vector, or SIZE_MAX if no nodes exist.
     */
    size_t FindNearestNode(const glm::vec3& pos) const;

    /**
     * @brief Find a path between two world positions.
     *
     * Locates the nearest nodes to start and goal, runs A* via
     * PathFinder, and converts the result to world-space waypoints.
     *
     * @param start World-space start position.
     * @param goal  World-space goal position.
     * @return Ordered list of waypoints (vec3), or empty if no path.
     */
    std::vector<glm::vec3> FindPath(const glm::vec3& start,
                                    const glm::vec3& goal) const;

    /** @brief Get all navigation nodes. */
    const std::vector<NavNode>& GetNodes() const { return m_Nodes; }

    /** @brief Check if the NavMesh has been built. */
    bool IsBuilt() const { return m_Built; }

    /** @brief Get total node count. */
    size_t GetNodeCount() const { return m_Nodes.size(); }

private:
    /**
     * @brief Check if a capsule can walk between two positions.
     *
     * Steps along the line between the two positions and verifies
     * ground exists at each sample point (no cliffs or gaps).
     */
    bool CanTraverse(const glm::vec3& from, const glm::vec3& to,
                     const Physics::CollisionMesh& mesh) const;

    std::vector<NavNode> m_Nodes;
    bool m_Built = false;

    // Grid lookup acceleration (multi-layer: each cell can have multiple nodes)
    int m_GridResX = 0;
    int m_GridResZ = 0;
    float m_MinX = 0.0f;
    float m_MinZ = 0.0f;
    std::vector<std::vector<int>> m_GridToNodes; // grid cell -> list of node indices

    static constexpr float CELL_SIZE = 1.5f;
    static constexpr float PROBE_HEIGHT = 50.0f;
    static constexpr float PROBE_RADIUS = 0.4f;
    static constexpr float PROBE_CAP_HEIGHT = 1.0f;
    static constexpr float MAX_SLOPE_Y_DIFF = 1.5f;
    static constexpr float TRAVERSE_STEP = 0.5f;
    static constexpr float MAX_STEP_HEIGHT = 0.5f;
    static constexpr float MIN_LAYER_GAP = 2.0f;    ///< Min Y gap between layers
    static constexpr int   MAX_LAYERS = 4;          ///< Max vertical layers per cell
};

} // namespace Navigation

#endif // CS_NAVIGATION_NAVMESH_HPP

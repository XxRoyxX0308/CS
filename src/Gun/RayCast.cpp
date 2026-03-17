#include "Gun/RayCast.hpp"

#include <cmath>
#include <vector>
#include <algorithm>

namespace Gun {
namespace RayCast {

// ============================================================================
//  Möller–Trumbore ray–triangle intersection
// ============================================================================
static bool RayTriangleIntersect(const glm::vec3 &origin,
                                 const glm::vec3 &dir,
                                 const Collision::Triangle &tri,
                                 float &outT) {
    constexpr float EPSILON = 1e-7f;

    glm::vec3 edge1 = tri.v1 - tri.v0;
    glm::vec3 edge2 = tri.v2 - tri.v0;
    glm::vec3 h     = glm::cross(dir, edge2);
    float a         = glm::dot(edge1, h);

    if (std::fabs(a) < EPSILON) return false; // Parallel

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
//  Cast — raycast against collision mesh using broadphase grid
// ============================================================================
RayHitResult Cast(const glm::vec3 &origin,
                  const glm::vec3 &direction,
                  const Collision::CollisionMesh &mesh,
                  float maxDist) {
    RayHitResult result;

    if (!mesh.IsBuilt()) return result;

    // Build an AABB along the ray for broadphase query
    glm::vec3 endPoint = origin + direction * maxDist;

    Collision::AABB_XZ aabb;
    aabb.minX = std::min(origin.x, endPoint.x);
    aabb.maxX = std::max(origin.x, endPoint.x);
    aabb.minZ = std::min(origin.z, endPoint.z);
    aabb.maxZ = std::max(origin.z, endPoint.z);

    // Gather candidate triangles from the spatial grid
    std::vector<uint32_t> candidates;
    mesh.GatherCandidates(aabb, candidates);

    // Deduplicate
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()),
                     candidates.end());

    const auto &triangles = mesh.GetTriangles();
    float closestT = maxDist;

    for (uint32_t idx : candidates) {
        const auto &tri = triangles[idx];
        float t = 0.0f;
        if (RayTriangleIntersect(origin, direction, tri, t)) {
            if (t < closestT) {
                closestT       = t;
                result.hit      = true;
                result.distance = t;
                result.point    = origin + direction * t;
                result.normal   = tri.normal;
            }
        }
    }

    return result;
}

} // namespace RayCast
} // namespace Gun

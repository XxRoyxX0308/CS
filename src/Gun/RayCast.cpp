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
//  Ray-Sphere intersection
// ============================================================================
static bool RaySphereIntersect(const glm::vec3 &origin,
                               const glm::vec3 &dir,
                               const glm::vec3 &center,
                               float radius,
                               float &outT) {
    glm::vec3 oc = origin - center;
    float a = glm::dot(dir, dir);
    float b = 2.0f * glm::dot(oc, dir);
    float c = glm::dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) return false;

    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    if (t1 > 0.0f) {
        outT = t1;
        return true;
    }
    if (t2 > 0.0f) {
        outT = t2;
        return true;
    }
    return false;
}

// ============================================================================
//  Ray-Cylinder intersection (infinite cylinder along Y axis)
// ============================================================================
static bool RayCylinderIntersect(const glm::vec3 &origin,
                                 const glm::vec3 &dir,
                                 const glm::vec3 &cylinderBase,
                                 float cylinderHeight,
                                 float radius,
                                 float &outT,
                                 glm::vec3 &outNormal) {
    // Project to XZ plane (cylinder aligned with Y axis)
    glm::vec2 originXZ(origin.x - cylinderBase.x, origin.z - cylinderBase.z);
    glm::vec2 dirXZ(dir.x, dir.z);

    float a = glm::dot(dirXZ, dirXZ);
    float b = 2.0f * glm::dot(originXZ, dirXZ);
    float c = glm::dot(originXZ, originXZ) - radius * radius;
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0.0f) return false;

    float sqrtD = std::sqrt(discriminant);
    float t1 = (-b - sqrtD) / (2.0f * a);
    float t2 = (-b + sqrtD) / (2.0f * a);

    // Check both intersections
    for (float t : {t1, t2}) {
        if (t > 0.0f) {
            glm::vec3 hitPoint = origin + dir * t;
            float yMin = cylinderBase.y;
            float yMax = cylinderBase.y + cylinderHeight;

            if (hitPoint.y >= yMin && hitPoint.y <= yMax) {
                outT = t;
                // Normal points outward on XZ plane
                outNormal = glm::normalize(glm::vec3(
                    hitPoint.x - cylinderBase.x, 0.0f, hitPoint.z - cylinderBase.z));
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
//  CastAgainstCapsule — raycast against a capsule
// ============================================================================
RayHitResult CastAgainstCapsule(const glm::vec3 &origin,
                                const glm::vec3 &direction,
                                const Collision::Capsule &capsule,
                                float maxDist) {
    RayHitResult result;
    float closestT = maxDist;

    // Capsule is defined by:
    // - base: center of bottom hemisphere
    // - height: distance between hemisphere centers
    // - radius: radius of hemispheres and cylinder

    glm::vec3 bottomCenter = capsule.base;
    glm::vec3 topCenter = capsule.base + glm::vec3(0.0f, capsule.height, 0.0f);

    // Test bottom hemisphere
    float t;
    if (RaySphereIntersect(origin, direction, bottomCenter, capsule.radius, t)) {
        glm::vec3 hitPoint = origin + direction * t;
        // Only count if hit is in the bottom hemisphere (below base+0)
        if (hitPoint.y <= bottomCenter.y && t < closestT) {
            closestT = t;
            result.hit = true;
            result.distance = t;
            result.point = hitPoint;
            result.normal = glm::normalize(hitPoint - bottomCenter);
        }
    }

    // Test top hemisphere
    if (RaySphereIntersect(origin, direction, topCenter, capsule.radius, t)) {
        glm::vec3 hitPoint = origin + direction * t;
        // Only count if hit is in the top hemisphere (above top center)
        if (hitPoint.y >= topCenter.y && t < closestT) {
            closestT = t;
            result.hit = true;
            result.distance = t;
            result.point = hitPoint;
            result.normal = glm::normalize(hitPoint - topCenter);
        }
    }

    // Test cylinder body
    glm::vec3 cylinderNormal;
    if (RayCylinderIntersect(origin, direction, bottomCenter, capsule.height,
                             capsule.radius, t, cylinderNormal)) {
        if (t < closestT) {
            closestT = t;
            result.hit = true;
            result.distance = t;
            result.point = origin + direction * t;
            result.normal = cylinderNormal;
        }
    }

    return result;
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

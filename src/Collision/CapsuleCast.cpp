#include "Collision/CapsuleCast.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Collision {
namespace CapsuleCast {

// ── Helpers ─────────────────────────────────────────────────────────────────

static glm::vec3 ClosestPointOnSegment(const glm::vec3 &a,
                                        const glm::vec3 &b,
                                        const glm::vec3 &p) {
    glm::vec3 ab = b - a;
    float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
    t = std::clamp(t, 0.0f, 1.0f);
    return a + t * ab;
}

static glm::vec3 ClosestPointOnTriangle(const glm::vec3 &p,
                                         const Triangle &tri) {
    // Barycentric projection (Ericson "Real-Time Collision Detection" §5.1.5)
    glm::vec3 ab = tri.v1 - tri.v0;
    glm::vec3 ac = tri.v2 - tri.v0;
    glm::vec3 ap = p - tri.v0;

    float d1 = glm::dot(ab, ap);
    float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return tri.v0; // vertex v0

    glm::vec3 bp = p - tri.v1;
    float d3 = glm::dot(ab, bp);
    float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return tri.v1; // vertex v1

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return tri.v0 + v * ab; // edge v0-v1
    }

    glm::vec3 cp = p - tri.v2;
    float d5 = glm::dot(ab, cp);
    float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return tri.v2; // vertex v2

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return tri.v0 + w * ac; // edge v0-v2
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return tri.v1 + w * (tri.v2 - tri.v1); // edge v1-v2
    }

    // Inside face
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return tri.v0 + ab * v + ac * w;
}

// Solve at² + bt + c = 0, return smallest t in [0, tMax].
// Returns -1 if no valid solution.
static float SolveQuadratic(float a, float b, float c, float tMax) {
    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) return -1.0f;

    float sqrtDisc = std::sqrt(disc);
    float inv2a = 1.0f / (2.0f * a);

    float t0 = (-b - sqrtDisc) * inv2a;
    float t1 = (-b + sqrtDisc) * inv2a;

    // Prefer the smallest non-negative t within [0, tMax]
    if (t0 >= 0.0f && t0 <= tMax) return t0;
    if (t1 >= 0.0f && t1 <= tMax) return t1;
    return -1.0f;
}

// ============================================================================
//  SphereTriangleSweep — sweep sphere center along velocity vs one triangle
//
//  Based on Ericson "Real-Time Collision Detection" §5.3 (sphere vs triangle).
//  Tests sphere against:
//    1) Triangle face (offset by radius along normal)
//    2) Three edges
//    3) Three vertices
//  Returns the earliest valid hit.
// ============================================================================
SweepResult SphereTriangleSweep(const glm::vec3 &center,
                                float radius,
                                const glm::vec3 &velocity,
                                const Triangle &tri) {
    SweepResult result;
    float velLen2 = glm::dot(velocity, velocity);
    if (velLen2 < 1e-12f) return result;

    // ── 1. Sphere vs triangle plane ─────────────────────────────────────
    // Offset the plane by ±radius along the normal so the sphere center
    // hits when the sphere surface touches the plane.
    float dist = glm::dot(center - tri.v0, tri.normal);
    float denom = glm::dot(velocity, tri.normal);

    float bestT = 2.0f; // > 1 means no hit yet
    glm::vec3 bestNormal = tri.normal;
    glm::vec3 bestPoint{};

    if (std::abs(denom) > 1e-7f) {
        // t for sphere touching the front side of the plane
        float t = (radius - dist) / denom;
        if (t >= 0.0f && t <= 1.0f) {
            // Check if hit point is inside the triangle
            glm::vec3 hitCenter = center + velocity * t;
            glm::vec3 hitOnPlane = hitCenter - tri.normal * radius;

            // Barycentric containment test
            glm::vec3 e0 = tri.v1 - tri.v0;
            glm::vec3 e1 = tri.v2 - tri.v0;
            glm::vec3 e2 = hitOnPlane - tri.v0;

            float d00 = glm::dot(e0, e0);
            float d01 = glm::dot(e0, e1);
            float d11 = glm::dot(e1, e1);
            float d20 = glm::dot(e2, e0);
            float d21 = glm::dot(e2, e1);

            float baryDenom = d00 * d11 - d01 * d01;
            if (std::abs(baryDenom) > 1e-10f) {
                float invDenom = 1.0f / baryDenom;
                float bv = (d11 * d20 - d01 * d21) * invDenom;
                float bw = (d00 * d21 - d01 * d20) * invDenom;

                if (bv >= -1e-4f && bw >= -1e-4f && (bv + bw) <= 1.0f + 1e-4f) {
                    if (t < bestT) {
                        bestT = t;
                        bestNormal = tri.normal;
                        bestPoint = hitOnPlane;
                    }
                }
            }
        }
    }

    // ── 2. Sphere vs edges ──────────────────────────────────────────────
    const glm::vec3 *verts[3] = {&tri.v0, &tri.v1, &tri.v2};
    for (int i = 0; i < 3; ++i) {
        const glm::vec3 &A = *verts[i];
        const glm::vec3 &B = *verts[(i + 1) % 3];

        glm::vec3 edge = B - A;
        glm::vec3 oc = center - A;

        float edgeLen2  = glm::dot(edge, edge);
        float edgeDotV  = glm::dot(edge, velocity);
        float edgeDotOC = glm::dot(edge, oc);

        // Quadratic equation for distance from moving sphere center to
        // infinite line: |oc + v*t - edge * ((edgeDotOC + edgeDotV*t)/edgeLen2)|² = r²
        // Simplifies to: a·t² + b·t + c = 0 where components perpendicular
        // to the edge are used.
        float a = velLen2 - (edgeDotV * edgeDotV) / edgeLen2;
        float b = 2.0f * (glm::dot(oc, velocity) - (edgeDotOC * edgeDotV) / edgeLen2);
        float c = glm::dot(oc, oc) - (edgeDotOC * edgeDotOC) / edgeLen2 - radius * radius;

        float t = SolveQuadratic(a, b, c, 1.0f);
        if (t >= 0.0f && t < bestT) {
            // Check that the hit is within the edge segment [0, 1]
            float s = (edgeDotOC + edgeDotV * t) / edgeLen2;
            if (s >= 0.0f && s <= 1.0f) {
                glm::vec3 hitCenter = center + velocity * t;
                glm::vec3 closestOnEdge = A + s * edge;
                glm::vec3 n = hitCenter - closestOnEdge;
                float nLen = glm::length(n);
                if (nLen > 1e-7f) {
                    bestT = t;
                    bestNormal = n / nLen;
                    bestPoint = closestOnEdge;
                }
            }
        }
    }

    // ── 3. Sphere vs vertices ───────────────────────────────────────────
    for (int i = 0; i < 3; ++i) {
        const glm::vec3 &V = *verts[i];
        glm::vec3 oc = center - V;

        float a = velLen2;
        float b = 2.0f * glm::dot(oc, velocity);
        float c = glm::dot(oc, oc) - radius * radius;

        float t = SolveQuadratic(a, b, c, 1.0f);
        if (t >= 0.0f && t < bestT) {
            glm::vec3 hitCenter = center + velocity * t;
            glm::vec3 n = hitCenter - V;
            float nLen = glm::length(n);
            if (nLen > 1e-7f) {
                bestT = t;
                bestNormal = n / nLen;
                bestPoint = V;
            }
        }
    }

    if (bestT <= 1.0f) {
        result.hit    = true;
        result.t      = bestT;
        result.normal = bestNormal;
        result.point  = bestPoint;
    }

    return result;
}

// ============================================================================
//  CapsuleTriangleSweep — approximate capsule by 3 spheres on its axis
// ============================================================================
SweepResult CapsuleTriangleSweep(const Capsule &capsule,
                                 const glm::vec3 &velocity,
                                 const Triangle &tri) {
    // 3 sphere centers along the capsule's vertical axis
    glm::vec3 centers[3] = {
        capsule.base,                                              // bottom
        capsule.base + glm::vec3(0.0f, capsule.height * 0.5f, 0.0f), // middle
        capsule.base + glm::vec3(0.0f, capsule.height, 0.0f),     // top
    };

    SweepResult best;
    for (const auto &c : centers) {
        SweepResult r = SphereTriangleSweep(c, capsule.radius, velocity, tri);
        if (r.hit && r.t < best.t) {
            best = r;
        }
    }
    return best;
}

// ============================================================================
//  SweepCapsule — broadphase + narrowphase against full mesh
// ============================================================================
SweepResult SweepCapsule(const Capsule &capsule,
                         const glm::vec3 &velocity,
                         const CollisionMesh &mesh) {
    if (!mesh.IsBuilt()) return {};

    // Compute the AABB of the swept capsule path
    glm::vec3 bottomStart = capsule.base;
    glm::vec3 topStart    = capsule.base + glm::vec3(0.0f, capsule.height, 0.0f);
    glm::vec3 bottomEnd   = bottomStart + velocity;
    glm::vec3 topEnd      = topStart + velocity;

    AABB_XZ aabb;
    aabb.minX = std::min({bottomStart.x, topStart.x, bottomEnd.x, topEnd.x}) - capsule.radius;
    aabb.maxX = std::max({bottomStart.x, topStart.x, bottomEnd.x, topEnd.x}) + capsule.radius;
    aabb.minZ = std::min({bottomStart.z, topStart.z, bottomEnd.z, topEnd.z}) - capsule.radius;
    aabb.maxZ = std::max({bottomStart.z, topStart.z, bottomEnd.z, topEnd.z}) + capsule.radius;

    // Gather candidate triangles
    std::vector<uint32_t> candidates;
    candidates.reserve(64);
    mesh.GatherCandidates(aabb, candidates);

    if (candidates.empty()) return {};

    // Sort + deduplicate (candidates may contain duplicates across cells)
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()),
                     candidates.end());

    const auto &triangles = mesh.GetTriangles();

    // Y range of capsule path for quick rejection
    float capsuleMinY = std::min(bottomStart.y, bottomEnd.y) - capsule.radius;
    float capsuleMaxY = std::max(topStart.y, topEnd.y) + capsule.radius;

    SweepResult best;
    for (uint32_t ti : candidates) {
        const auto &tri = triangles[ti];

        // Quick Y AABB rejection
        float triMinY = std::min({tri.v0.y, tri.v1.y, tri.v2.y});
        float triMaxY = std::max({tri.v0.y, tri.v1.y, tri.v2.y});
        if (triMaxY < capsuleMinY || triMinY > capsuleMaxY) continue;

        SweepResult r = CapsuleTriangleSweep(capsule, velocity, tri);
        if (r.hit && r.t < best.t) {
            best = r;
        }
    }

    return best;
}

// ============================================================================
//  MoveAndSlide — iterative capsule sweep with collision response
// ============================================================================
glm::vec3 MoveAndSlide(const Capsule &capsule,
                       const glm::vec3 &velocity,
                       const CollisionMesh &mesh,
                       int maxIterations) {
    constexpr float EPSILON = 1e-4f;

    glm::vec3 pos = capsule.base;
    glm::vec3 vel = velocity;

    Capsule sweepCapsule = capsule;

    for (int iter = 0; iter < maxIterations; ++iter) {
        if (glm::dot(vel, vel) < EPSILON * EPSILON)
            break;

        sweepCapsule.base = pos;
        SweepResult result = SweepCapsule(sweepCapsule, vel, mesh);
        
        if (!result.hit) {
            pos += vel;
            break;
        }

        // Advance to just before the hit
        float safeT = std::max(0.0f, result.t - EPSILON);
        pos += vel * safeT;

        // Compute remaining velocity and project onto slide plane
        glm::vec3 remaining = vel * (1.0f - result.t);
        vel = remaining - glm::dot(remaining, result.normal) * result.normal;
    }

    return pos;
}

// ============================================================================
//  SnapToGround — downward capsule sweep to find walkable ground
//
//  Internally lifts the probe capsule before sweeping to handle cases
//  where gravity has already pushed the capsule below the ground surface
//  (which would cause the sphere-plane test to return negative t).
// ============================================================================
std::optional<float> SnapToGround(const Capsule &capsule,
                                  const CollisionMesh &mesh,
                                  float maxDrop) {
    // Lift the probe so it starts above any surface we might be penetrating.
    Capsule probe = capsule;

    glm::vec3 downVel(0.0f, -maxDrop, 0.0f);
    SweepResult result = SweepCapsule(probe, downVel, mesh);

    if (result.hit && result.normal.y >= 0.7f) {
        // Feet Y at hit = bottom sphere center after sweep − radius
        float groundY = probe.base.y - maxDrop * result.t - capsule.radius;
        return groundY;
    }

    return std::nullopt;
}

} // namespace CapsuleCast
} // namespace Collision

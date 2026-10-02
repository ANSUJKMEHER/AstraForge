#pragma once
// Narrowphase intersection tests.
//
// Contact semantics:
//   - normal: unit vector pointing FROM the first shape (a) TOWARD the
//     second shape (b). To separate the pair: move a by -normal*(depth/2)
//     and b by +normal*(depth/2).

#include <cmath>

#include "af/collision/Shapes.h"
#include "af/math/MathUtil.h"
#include "af/math/Vec3.h"

namespace af {

struct Contact {
    bool hit = false;
    Vec3 normal{0.0f, 0.0f, 0.0f};
    float depth = 0.0f;
};

inline Contact SphereSphere(const Sphere& a, const Sphere& b) {
    const Vec3 delta = b.center - a.center;
    const float distSq = LengthSq(delta);
    const float rSum = a.radius + b.radius;
    if (distSq >= rSum * rSum) return {};
    const float dist = std::sqrt(distSq);
    // Degenerate (coincident centers): pick an arbitrary stable normal.
    const Vec3 normal = dist > 1e-8f ? delta / dist : Vec3{0.0f, 1.0f, 0.0f};
    return {true, normal, rSum - dist};
}

inline Contact SphereAABB(const Sphere& s, const AABB& b) {
    const Vec3 closest{Clamp(s.center.x, b.min.x, b.max.x),
                       Clamp(s.center.y, b.min.y, b.max.y),
                       Clamp(s.center.z, b.min.z, b.max.z)};
    const Vec3 delta = s.center - closest;
    const float distSq = LengthSq(delta);
    if (distSq > s.radius * s.radius) return {};
    if (distSq > 1e-12f) {
        const float dist = std::sqrt(distSq);
        // From the sphere (a) toward the box (b): away from the closest point.
        return {true, -delta / dist, s.radius - dist};
    }
    // Sphere center inside the box: push out along the least-penetration face.
    const float dx = s.center.x - b.min.x;   // to the -X face
    const float dnx = b.max.x - s.center.x;  // to the +X face
    const float dy = s.center.y - b.min.y;
    const float dny = b.max.y - s.center.y;
    const float dz = s.center.z - b.min.z;
    const float dnz = b.max.z - s.center.z;
    const float m = Min(Min(Min(Min(Min(dx, dnx), dy), dny), dz), dnz);
    Vec3 normal{0.0f, 0.0f, 0.0f};
    if (m == dx) normal = {-1.0f, 0.0f, 0.0f};
    else if (m == dnx) normal = {1.0f, 0.0f, 0.0f};
    else if (m == dy) normal = {0.0f, -1.0f, 0.0f};
    else if (m == dny) normal = {0.0f, 1.0f, 0.0f};
    else if (m == dz) normal = {0.0f, 0.0f, -1.0f};
    else normal = {0.0f, 0.0f, 1.0f};
    return {true, normal, s.radius + m};
}

inline Contact AABBAABB(const AABB& a, const AABB& b) {
    const float dx1 = b.max.x - a.min.x;  // overlap of b over a's -X face
    const float dx2 = a.max.x - b.min.x;  // overlap of b over a's +X face
    if (dx1 <= 0.0f || dx2 <= 0.0f) return {};
    const float dy1 = b.max.y - a.min.y;
    const float dy2 = a.max.y - b.min.y;
    if (dy1 <= 0.0f || dy2 <= 0.0f) return {};
    const float dz1 = b.max.z - a.min.z;
    const float dz2 = a.max.z - b.min.z;
    if (dz1 <= 0.0f || dz2 <= 0.0f) return {};
    // Least-penetration axis; normal points from a toward b.
    const float m = Min(Min(Min(Min(Min(dx1, dx2), dy1), dy2), dz1), dz2);
    if (m == dx1) return {true, {1.0f, 0.0f, 0.0f}, m};   // b is to the +X side of a
    if (m == dx2) return {true, {-1.0f, 0.0f, 0.0f}, m};  // b is to the -X side of a
    if (m == dy1) return {true, {0.0f, 1.0f, 0.0f}, m};
    if (m == dy2) return {true, {0.0f, -1.0f, 0.0f}, m};
    if (m == dz1) return {true, {0.0f, 0.0f, 1.0f}, m};
    return {true, {0.0f, 0.0f, -1.0f}, m};
}

// Dispatch between the two collider kinds. Returns hit + normal/depth with the
// same "push a" convention.
inline Contact TestColliders(const Sphere& a, const Sphere& b) { return SphereSphere(a, b); }
inline Contact TestColliders(const Sphere& a, const AABB& b) { return SphereAABB(a, b); }
inline Contact TestColliders(const AABB& a, const Sphere& b) {
    // Flip the shapes, then flip the normal so it still points a → b.
    Contact c = SphereAABB(b, a);
    if (c.hit) c.normal = -c.normal;
    return c;
}
inline Contact TestColliders(const AABB& a, const AABB& b) { return AABBAABB(a, b); }

}  // namespace af

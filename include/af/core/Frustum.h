#pragma once
// View frustum: 6 planes extracted from a view-projection matrix
// (Gribb–Hartmann). Plane equation: normal · p + distance = 0, normal unit;
// a point is inside when the expression is ≥ 0 for all six planes.
// Pure math — unit-testable headless; the renderer uses it for culling.

#include "af/math/Mat4.h"
#include "af/math/Vec3.h"

namespace af {

struct Plane {
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
};

struct Frustum {
    Plane planes[6];

    static Frustum FromViewProjection(const Mat4& viewProj);

    bool ContainsPoint(const Vec3& p) const;

    // Conservative sphere test (false positives allowed, false negatives not).
    bool IntersectsSphere(const Vec3& center, float radius) const;
};

}  // namespace af

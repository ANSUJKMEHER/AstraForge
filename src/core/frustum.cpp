#include "af/core/Frustum.h"

#include <cmath>

#include "af/math/MathUtil.h"

namespace af {

namespace {

// Row i of a column-major matrix = (m(i,0), m(i,1), m(i,2), m(i,3)).
Vec4 Row(const Mat4& m, int i) {
    return {m(i, 0), m(i, 1), m(i, 2), m(i, 3)};
}

Plane MakePlane(const Vec4& row3, const Vec4& row, float sign) {
    const Vec3 normal{row3.x + sign * row.x, row3.y + sign * row.y,
                      row3.z + sign * row.z};
    const float len = Length(normal);
    const Vec3 n = normal / len;
    return {n, (row3.w + sign * row.w) / len};
}

}  // namespace

Frustum Frustum::FromViewProjection(const Mat4& viewProj) {
    const Vec4 r0 = Row(viewProj, 0);
    const Vec4 r1 = Row(viewProj, 1);
    const Vec4 r2 = Row(viewProj, 2);
    const Vec4 r3 = Row(viewProj, 3);
    Frustum f;
    // Inside = n·p + d ≥ 0. Sign conventions verified by tests: the identity
    // matrix yields the unit cube [−1,1]³.
    f.planes[0] = MakePlane(r3, r0, +1.0f);  // left   (x ≥ −1 in clip space)
    f.planes[1] = MakePlane(r3, r0, -1.0f);  // right
    f.planes[2] = MakePlane(r3, r1, +1.0f);  // bottom
    f.planes[3] = MakePlane(r3, r1, -1.0f);  // top
    f.planes[4] = MakePlane(r3, r2, +1.0f);  // near   (z ≥ −1)
    f.planes[5] = MakePlane(r3, r2, -1.0f);  // far
    return f;
}

bool Frustum::ContainsPoint(const Vec3& p) const {
    for (const Plane& plane : planes) {
        if (Dot(plane.normal, p) + plane.distance < -1e-5f) return false;
    }
    return true;
}

bool Frustum::IntersectsSphere(const Vec3& center, float radius) const {
    for (const Plane& plane : planes) {
        if (Dot(plane.normal, center) + plane.distance < -radius) return false;
    }
    return true;
}

}  // namespace af

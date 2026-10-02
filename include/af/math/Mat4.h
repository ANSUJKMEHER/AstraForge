#pragma once
// Column-major 4x4 matrix, OpenGL-compatible.
//
// Storage: v[col * 4 + row]; element (row, col) via operator()(row, col).
// Conventions (see docs/math-conventions.md):
//   - right-handed coordinate system, +Y up, cameras look down -Z
//   - matrices transform column vectors: (a * b) * v == a * (b * v)

#include <cmath>

#include "MathUtil.h"
#include "Vec3.h"
#include "Vec4.h"

namespace af {

struct Mat4 {
    // Default-initialized to identity.
    float v[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    constexpr Mat4() = default;
    explicit Mat4(const float* data) {
        for (int i = 0; i < 16; ++i) v[i] = data[i];
    }

    constexpr float& operator()(int row, int col) { return v[col * 4 + row]; }
    constexpr const float& operator()(int row, int col) const { return v[col * 4 + row]; }
    const float* Data() const { return v; }

    static Mat4 Identity() { return Mat4{}; }
    static Mat4 Zero() {
        Mat4 m{};
        for (int i = 0; i < 16; ++i) m.v[i] = 0.0f;
        return m;
    }

    static Mat4 Translation(const Vec3& t) {
        Mat4 m;
        m(0, 3) = t.x;
        m(1, 3) = t.y;
        m(2, 3) = t.z;
        return m;
    }

    static Mat4 Scaling(const Vec3& s) {
        Mat4 m;
        m(0, 0) = s.x;
        m(1, 1) = s.y;
        m(2, 2) = s.z;
        return m;
    }

    static Mat4 RotationX(float radians) {
        Mat4 m;
        const float c = std::cos(radians), s = std::sin(radians);
        m(1, 1) = c; m(1, 2) = -s;
        m(2, 1) = s; m(2, 2) = c;
        return m;
    }

    static Mat4 RotationY(float radians) {
        Mat4 m;
        const float c = std::cos(radians), s = std::sin(radians);
        m(0, 0) = c; m(0, 2) = s;
        m(2, 0) = -s; m(2, 2) = c;
        return m;
    }

    static Mat4 RotationZ(float radians) {
        Mat4 m;
        const float c = std::cos(radians), s = std::sin(radians);
        m(0, 0) = c; m(0, 1) = -s;
        m(1, 0) = s; m(1, 1) = c;
        return m;
    }
};

// a * b applies b first, then a.
inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 out = Mat4::Zero();
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a(r, k) * b(k, c);
            out(r, c) = sum;
        }
    return out;
}

inline Vec4 operator*(const Mat4& m, const Vec4& col) {
    Vec4 out;
    for (int r = 0; r < 4; ++r)
        out[r] = m(r, 0) * col.x + m(r, 1) * col.y + m(r, 2) * col.z + m(r, 3) * col.w;
    return out;
}

// Point transform (w = 1); performs the perspective divide when w != 1.
inline Vec3 TransformPoint(const Mat4& m, const Vec3& p) {
    const Vec4 r = m * Vec4{p.x, p.y, p.z, 1.0f};
    if (r.w != 0.0f) return {r.x / r.w, r.y / r.w, r.z / r.w};
    return {r.x, r.y, r.z};
}

// Direction transform (w = 0): ignores translation, as expected for vectors.
inline Vec3 TransformDirection(const Mat4& m, const Vec3& d) {
    const Vec4 r = m * Vec4{d.x, d.y, d.z, 0.0f};
    return {r.x, r.y, r.z};
}

inline Mat4 Transpose(const Mat4& m) {
    Mat4 out;
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) out(c, r) = m(r, c);
    return out;
}

namespace detail {
// Determinant of the 3x3 minor of m obtained by deleting skipRow and skipCol.
inline float MinorDet3(const Mat4& m, int skipRow, int skipCol) {
    int rows[3];
    int cols[3];
    int ri = 0, ci = 0;
    for (int r = 0; r < 4; ++r)
        if (r != skipRow) rows[ri++] = r;
    for (int c = 0; c < 4; ++c)
        if (c != skipCol) cols[ci++] = c;
    const float a = m(rows[0], cols[0]), b = m(rows[0], cols[1]), c = m(rows[0], cols[2]);
    const float d = m(rows[1], cols[0]), e = m(rows[1], cols[1]), f = m(rows[1], cols[2]);
    const float g = m(rows[2], cols[0]), h = m(rows[2], cols[1]), i = m(rows[2], cols[2]);
    return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
}
}  // namespace detail

inline float Determinant(const Mat4& m) {
    // Cofactor expansion along column 0.
    return m(0, 0) * detail::MinorDet3(m, 0, 0)
         - m(1, 0) * detail::MinorDet3(m, 1, 0)
         + m(2, 0) * detail::MinorDet3(m, 2, 0)
         - m(3, 0) * detail::MinorDet3(m, 3, 0);
}

// General inverse via adjugate/determinant. Constant-heavy but O(1); not for
// per-frame hot paths (rigid transforms have cheaper inverses). The result is
// garbage for singular matrices — callers must know the matrix is invertible
// (always true for rigid transforms; tests verify with well-conditioned input).
inline Mat4 Inverse(const Mat4& m) {
    const float det = Determinant(m);
    Mat4 out;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float cof = detail::MinorDet3(m, c, r);
            if ((r + c) & 1) cof = -cof;
            out(r, c) = cof / det;
        }
    return out;
}

// Right-handed perspective projection. Maps view-space z in [-zFar, -zNear]
// to NDC z in [-1, 1] (OpenGL clip convention).
inline Mat4 Perspective(float fovYRadians, float aspect, float zNear, float zFar) {
    const float t = 1.0f / std::tan(fovYRadians * 0.5f);
    Mat4 m = Mat4::Zero();
    m(0, 0) = t / aspect;
    m(1, 1) = t;
    m(2, 2) = -(zFar + zNear) / (zFar - zNear);
    m(2, 3) = -(2.0f * zFar * zNear) / (zFar - zNear);
    m(3, 2) = -1.0f;
    return m;
}

inline Mat4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    Mat4 m = Mat4::Zero();
    m(0, 0) = 2.0f / (right - left);
    m(1, 1) = 2.0f / (top - bottom);
    m(2, 2) = -2.0f / (zFar - zNear);
    m(0, 3) = -(right + left) / (right - left);
    m(1, 3) = -(top + bottom) / (top - bottom);
    m(2, 3) = -(zFar + zNear) / (zFar - zNear);
    m(3, 3) = 1.0f;
    return m;
}

// Right-handed view matrix; the camera looks down -Z in view space.
// Degenerate when `center - eye` is parallel to `up` (documented limitation;
// the gameplay camera never pitches a full 90 degrees).
inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 f = Normalize(center - eye);
    const Vec3 s = Normalize(Cross(f, up));
    const Vec3 u = Cross(s, f);
    Mat4 m = Mat4::Zero();
    m(0, 0) = s.x; m(0, 1) = s.y; m(0, 2) = s.z; m(0, 3) = -Dot(s, eye);
    m(1, 0) = u.x; m(1, 1) = u.y; m(1, 2) = u.z; m(1, 3) = -Dot(u, eye);
    m(2, 0) = -f.x; m(2, 1) = -f.y; m(2, 2) = -f.z; m(2, 3) = Dot(f, eye);
    m(3, 3) = 1.0f;
    return m;
}

// Composition helpers (post-multiply): Translate(m, t) == m * T(t).
inline Mat4 Translate(const Mat4& m, const Vec3& t) { return m * Mat4::Translation(t); }
inline Mat4 RotateX(const Mat4& m, float radians) { return m * Mat4::RotationX(radians); }
inline Mat4 RotateY(const Mat4& m, float radians) { return m * Mat4::RotationY(radians); }
inline Mat4 RotateZ(const Mat4& m, float radians) { return m * Mat4::RotationZ(radians); }
inline Mat4 Scale(const Mat4& m, const Vec3& s) { return m * Mat4::Scaling(s); }

}  // namespace af

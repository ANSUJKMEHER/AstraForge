#include <cstdint>

#include "af/math/Mat4.h"
#include "af/math/MathUtil.h"
#include "af/math/Vec3.h"
#include "af/math/Vec4.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

// Deterministic LCG; returns floats in [-0.5, 0.5).
float NextRand(uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f - 0.5f;
}

// Strictly column-diagonally-dominant matrix => invertible and well conditioned.
Mat4 RandomWellConditioned(uint32_t& seed) {
    Mat4 m = Mat4::Identity();
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (r == c) {
                m(r, c) = 0.6f + NextRand(seed);
            } else {
                m(r, c) = NextRand(seed) * 0.05f;
            }
        }
    }
    return m;
}

void CheckMatNear(const Mat4& a, const Mat4& b, float eps, const char* file, int line) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            if (!(Abs(a(r, c) - b(r, c)) <= eps)) {
                ++::af::test::CurrentState().checks;
                char buf[128];
                std::snprintf(buf, sizeof(buf), "matrix element (%d,%d): %.6f ~= %.6f",
                              r, c, static_cast<double>(a(r, c)), static_cast<double>(b(r, c)));
                ::af::test::ReportFailure(file, line, buf);
            }
            ++::af::test::CurrentState().checks;
        }
}

#define AF_CHECK_MAT_NEAR(a, b, eps) CheckMatNear((a), (b), (eps), __FILE__, __LINE__)

}  // namespace

AF_TEST("Mat4 default and identity") {
    const Mat4 m;
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) AF_CHECK_NEAR(m(r, c), r == c ? 1.0f : 0.0f, 1e-6f);

    // Multiplying by identity must not change a NON-trivial matrix.
    // (A transpose-symmetric matrix here would hide a transposed multiply.)
    const Mat4 t = Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f}) * Mat4::Scaling(Vec3{2.0f, 3.0f, 4.0f});
    const Mat4 i = Mat4::Identity();
    AF_CHECK_MAT_NEAR(i * t, t, 1e-6f);
    AF_CHECK_MAT_NEAR(t * i, t, 1e-6f);
}

AF_TEST("Mat4 column-major layout") {
    // Column 3 holds the translation; column-major storage means v[12..14].
    const Mat4 t = Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f});
    AF_CHECK_NEAR(t.v[12], 1.0f, 1e-6f);
    AF_CHECK_NEAR(t.v[13], 2.0f, 1e-6f);
    AF_CHECK_NEAR(t.v[14], 3.0f, 1e-6f);
    // Element (row, col) lives at v[col*4 + row].
    AF_CHECK_NEAR(t(0, 3), 1.0f, 1e-6f);
    AF_CHECK_NEAR(t(2, 2), 1.0f, 1e-6f);
}

AF_TEST("Mat4 multiplication associativity") {
    const Mat4 a = Mat4::RotationY(Radians(30.0f)) * Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f});
    const Mat4 b = Mat4::Scaling(Vec3{2.0f, 3.0f, 4.0f});
    const Vec3 v{0.3f, -0.6f, 1.1f};
    // (a*b)*v == a*(b*v)
    const Vec3 lhs = TransformPoint(a * b, v);
    const Vec3 rhs = TransformPoint(a, TransformPoint(b, v));
    AF_CHECK_NEAR(lhs.x, rhs.x, 1e-4f);
    AF_CHECK_NEAR(lhs.y, rhs.y, 1e-4f);
    AF_CHECK_NEAR(lhs.z, rhs.z, 1e-4f);
}

AF_TEST("Mat4 transpose of product") {
    uint32_t seed = 12345u;
    const Mat4 a = RandomWellConditioned(seed);
    const Mat4 b = RandomWellConditioned(seed);
    // (A*B)^T == B^T * A^T
    AF_CHECK_MAT_NEAR(Transpose(a * b), Transpose(b) * Transpose(a), 1e-4f);
}

AF_TEST("Mat4 determinant") {
    AF_CHECK_NEAR(Determinant(Mat4::Identity()), 1.0f, 1e-6f);
    AF_CHECK_NEAR(Determinant(Mat4::Scaling(Vec3{2.0f, 3.0f, 4.0f})), 24.0f, 1e-4f);
    AF_CHECK_NEAR(Determinant(Mat4::Translation(Vec3{5.0f, 6.0f, 7.0f})), 1.0f, 1e-5f);
    // det(rotation) == 1 for pure rotations.
    AF_CHECK_NEAR(Determinant(Mat4::RotationX(Radians(37.0f))), 1.0f, 1e-4f);
}

AF_TEST("Mat4 inverse") {
    uint32_t seed = 987654321u;
    for (int i = 0; i < 8; ++i) {
        const Mat4 m = RandomWellConditioned(seed);
        const Mat4 inv = Inverse(m);
        const Mat4 prod = m * inv;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                AF_CHECK_NEAR(prod(r, c), r == c ? 1.0f : 0.0f, 1e-3f);
    }

    // Inverse of a translation negates the translation.
    const Mat4 t = Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f});
    const Vec3 p = TransformPoint(Inverse(t), TransformPoint(t, Vec3{7.0f, -4.0f, 2.0f}));
    AF_CHECK_NEAR(p.x, 7.0f, 1e-4f);
    AF_CHECK_NEAR(p.y, -4.0f, 1e-4f);
    AF_CHECK_NEAR(p.z, 2.0f, 1e-4f);

    // Inverse of a pure rotation is its transpose.
    const Mat4 rot = Mat4::RotationZ(Radians(53.0f));
    AF_CHECK_MAT_NEAR(Inverse(rot), Transpose(rot), 1e-4f);
}

AF_TEST("Mat4 point and direction transforms") {
    const Mat4 t = Mat4::Translation(Vec3{1.0f, 2.0f, 3.0f});
    const Vec3 p = TransformPoint(t, Vec3{0.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(p.x, 1.0f, 1e-6f);
    AF_CHECK_NEAR(p.y, 2.0f, 1e-6f);
    AF_CHECK_NEAR(p.z, 3.0f, 1e-6f);
    // Directions ignore translation (w = 0).
    const Vec3 d = TransformDirection(t, Vec3{4.0f, 5.0f, 6.0f});
    AF_CHECK_NEAR(d.x, 4.0f, 1e-6f);
    AF_CHECK_NEAR(d.z, 6.0f, 1e-6f);

    const Mat4 s = Mat4::Scaling(Vec3{2.0f, 2.0f, 2.0f});
    const Vec3 sp = TransformPoint(s, Vec3{1.0f, 2.0f, 3.0f});
    AF_CHECK_NEAR(sp.x, 2.0f, 1e-6f);
    AF_CHECK_NEAR(sp.y, 4.0f, 1e-6f);
    AF_CHECK_NEAR(sp.z, 6.0f, 1e-6f);
}

AF_TEST("Mat4 rotation conventions (right-handed)") {
    // +90 degrees about Y takes +X to -Z.
    const Vec3 ry = TransformDirection(Mat4::RotationY(Radians(90.0f)), Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(ry.x, 0.0f, 1e-4f);
    AF_CHECK_NEAR(ry.z, -1.0f, 1e-4f);
    // +90 degrees about X takes +Y to +Z.
    const Vec3 rx = TransformDirection(Mat4::RotationX(Radians(90.0f)), Vec3{0.0f, 1.0f, 0.0f});
    AF_CHECK_NEAR(rx.z, 1.0f, 1e-4f);
    // +90 degrees about Z takes +X to +Y.
    const Vec3 rz = TransformDirection(Mat4::RotationZ(Radians(90.0f)), Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(rz.y, 1.0f, 1e-4f);
}

AF_TEST("Perspective projection mapping") {
    const Mat4 p = Perspective(Radians(90.0f), 1.0f, 0.1f, 100.0f);
    // Near plane maps to NDC z = -1; center stays centered.
    const Vec4 nearPt = p * Vec4{0.0f, 0.0f, -0.1f, 1.0f};
    AF_CHECK_NEAR(nearPt.z / nearPt.w, -1.0f, 1e-4f);
    AF_CHECK_NEAR(nearPt.x / nearPt.w, 0.0f, 1e-5f);
    AF_CHECK_NEAR(nearPt.y / nearPt.w, 0.0f, 1e-5f);
    // Far plane maps to NDC z = +1.
    const Vec4 farPt = p * Vec4{0.0f, 0.0f, -100.0f, 1.0f};
    AF_CHECK_NEAR(farPt.z / farPt.w, 1.0f, 1e-4f);
}

AF_TEST("Perspective aspect ratio") {
    const Mat4 p = Perspective(Radians(90.0f), 2.0f, 0.1f, 100.0f);
    // x_ndc = x / (|z| * aspect * tan(fov/2)) = 0.5 / (1 * 2 * 1) = 0.25
    const Vec4 pt = p * Vec4{0.5f, 0.0f, -1.0f, 1.0f};
    AF_CHECK_NEAR(pt.x / pt.w, 0.25f, 1e-4f);
}

AF_TEST("Orthographic projection mapping") {
    const Mat4 o = Ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    const Vec4 nearPt = o * Vec4{0.0f, 0.0f, -0.1f, 1.0f};
    AF_CHECK_NEAR(nearPt.z / nearPt.w, -1.0f, 1e-4f);
    const Vec4 farPt = o * Vec4{0.0f, 0.0f, -100.0f, 1.0f};
    AF_CHECK_NEAR(farPt.z / farPt.w, 1.0f, 1e-4f);
    // Left/bottom corners map to NDC (-1, -1); right/top to (+1, +1).
    const Vec4 corner = o * Vec4{-1.0f, 1.0f, -50.0f, 1.0f};
    AF_CHECK_NEAR(corner.x, -1.0f, 1e-4f);
    AF_CHECK_NEAR(corner.y, 1.0f, 1e-4f);
    AF_CHECK_NEAR(corner.w, 1.0f, 1e-5f);
}

AF_TEST("LookAt view matrix") {
    // Looking down -Z from the origin is the identity view.
    const Mat4 v = LookAt(Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 0.0f, -1.0f}, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 p = TransformPoint(v, Vec3{1.0f, 2.0f, 3.0f});
    AF_CHECK_NEAR(p.x, 1.0f, 1e-5f);
    AF_CHECK_NEAR(p.y, 2.0f, 1e-5f);
    AF_CHECK_NEAR(p.z, 3.0f, 1e-5f);

    // A camera at (0,0,5) looking at the origin sees the origin 5 units ahead
    // (view-space z = -5).
    const Mat4 v2 = LookAt(Vec3{0.0f, 0.0f, 5.0f}, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    const Vec3 o = TransformPoint(v2, Vec3{0.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(o.z, -5.0f, 1e-4f);
    // Right stays +X when up is +Y.
    AF_CHECK_NEAR(TransformDirection(v2, Vec3{1.0f, 0.0f, 0.0f}).x, 1.0f, 1e-4f);
}

AF_TEST("TransformPoint performs perspective divide") {
    const Mat4 p = Perspective(Radians(90.0f), 1.0f, 0.1f, 100.0f);
    // y_ndc = y / (|z| * tan(fov/2)) = 0.5 / 0.5 = 1.0
    const Vec3 ndc = TransformPoint(p, Vec3{0.0f, 0.5f, -0.5f});
    AF_CHECK_NEAR(ndc.y, 1.0f, 1e-4f);
}

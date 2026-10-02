#include "af/math/MathUtil.h"
#include "af/math/Vec2.h"
#include "af/math/Vec3.h"
#include "af/math/Vec4.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("Vec3 construction and indexing") {
    const Vec3 a{1.0f, 2.0f, 3.0f};
    AF_CHECK_EQ(a.x, 1.0f);
    AF_CHECK_EQ(a.y, 2.0f);
    AF_CHECK_EQ(a.z, 3.0f);
    AF_CHECK_EQ(a[0], 1.0f);
    AF_CHECK_EQ(a[2], 3.0f);

    Vec3 b;  // default construction is zero
    AF_CHECK_EQ(b.x, 0.0f);
    AF_CHECK_EQ(b.y, 0.0f);
    AF_CHECK_EQ(b.z, 0.0f);

    Vec3 c = a;  // value semantics: no aliasing
    c[1] = 9.0f;
    AF_CHECK_EQ(c.y, 9.0f);
    AF_CHECK_EQ(a.y, 2.0f);
}

AF_TEST("Vec3 arithmetic operators") {
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, 5.0f, 6.0f};

    const Vec3 s = a + b;
    AF_CHECK_EQ(s.x, 5.0f);
    AF_CHECK_EQ(s.y, 7.0f);
    AF_CHECK_EQ(s.z, 9.0f);

    const Vec3 d = b - a;
    AF_CHECK_EQ(d.x, 3.0f);
    AF_CHECK_EQ(d.y, 3.0f);
    AF_CHECK_EQ(d.z, 3.0f);

    const Vec3 neg = -a;
    AF_CHECK_EQ(neg.x, -1.0f);
    AF_CHECK_EQ(neg.z, -3.0f);

    const Vec3 scaled = a * 2.0f;
    AF_CHECK_EQ(scaled.x, 2.0f);
    AF_CHECK_EQ(scaled.z, 6.0f);
    const Vec3 scaledL = 2.0f * a;
    AF_CHECK_EQ(scaledL.x, 2.0f);
    AF_CHECK_EQ(scaledL.z, 6.0f);

    const Vec3 div = b / 2.0f;
    AF_CHECK_EQ(div.y, 2.5f);

    Vec3 e = a;
    e += b;
    AF_CHECK_EQ(e.x, 5.0f);
    e -= a;
    e *= 0.5f;
    AF_CHECK_EQ(e.y, 2.5f);
    e /= 0.5f;
    AF_CHECK_EQ(e.z, 6.0f);

    AF_CHECK(a == (Vec3{1.0f, 2.0f, 3.0f}));
    AF_CHECK(a != b);
}

AF_TEST("Vec3 dot product") {
    AF_CHECK_NEAR(Dot(Vec3{1.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}), 0.0f, 1e-6f);
    AF_CHECK_NEAR(Dot(Vec3{2.0f, 3.0f, 4.0f}, Vec3{5.0f, 6.0f, 7.0f}), 56.0f, 1e-4f);
    AF_CHECK_NEAR(Dot(Vec3{1.0f, 2.0f, 3.0f}, Vec3{-1.0f, 1.0f, 0.0f}), 1.0f, 1e-6f);
    // Dot with self equals length squared.
    AF_CHECK_NEAR(Dot(Vec3{3.0f, 4.0f, 0.0f}, Vec3{3.0f, 4.0f, 0.0f}), 25.0f, 1e-5f);
}

AF_TEST("Vec3 cross product is right-handed") {
    const Vec3 c = Cross(Vec3{1.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f});
    AF_CHECK_NEAR(c.x, 0.0f, 1e-6f);
    AF_CHECK_NEAR(c.y, 0.0f, 1e-6f);
    AF_CHECK_NEAR(c.z, 1.0f, 1e-6f);

    // Anticommutativity.
    const Vec3 c2 = Cross(Vec3{0.0f, 1.0f, 0.0f}, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(c2.z, -1.0f, 1e-6f);

    // Cross of parallel vectors is zero.
    const Vec3 c3 = Cross(Vec3{2.0f, 0.0f, 0.0f}, Vec3{-4.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(c3.x, 0.0f, 1e-6f);
    AF_CHECK_NEAR(c3.y, 0.0f, 1e-6f);
    AF_CHECK_NEAR(c3.z, 0.0f, 1e-6f);

    // Cross is orthogonal to both operands.
    const Vec3 u{-0.4f, 1.7f, 0.9f};
    const Vec3 w{2.1f, 0.3f, -1.2f};
    const Vec3 o = Cross(u, w);
    AF_CHECK_NEAR(Dot(o, u), 0.0f, 1e-4f);
    AF_CHECK_NEAR(Dot(o, w), 0.0f, 1e-4f);
}

AF_TEST("Vec3 length and normalization") {
    AF_CHECK_NEAR(Length(Vec3{3.0f, 4.0f, 0.0f}), 5.0f, 1e-5f);
    AF_CHECK_NEAR(LengthSq(Vec3{1.0f, 2.0f, 3.0f}), 14.0f, 1e-5f);

    const Vec3 n = Normalize(Vec3{3.0f, 4.0f, 0.0f});
    AF_CHECK_NEAR(Length(n), 1.0f, 1e-5f);
    AF_CHECK_NEAR(n.x, 0.6f, 1e-5f);
    AF_CHECK_NEAR(n.y, 0.8f, 1e-5f);

    // Zero-length input normalizes to zero (documented, no NaN).
    const Vec3 z = Normalize(Vec3{0.0f, 0.0f, 0.0f});
    AF_CHECK_EQ(z.x, 0.0f);
    AF_CHECK_EQ(z.y, 0.0f);
    AF_CHECK_EQ(z.z, 0.0f);
}

AF_TEST("Vec3 distance and lerp") {
    AF_CHECK_NEAR(Distance(Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 2.0f, 2.0f}), 3.0f, 1e-5f);
    const Vec3 l = Lerp(Vec3{0.0f, 0.0f, 0.0f}, Vec3{10.0f, 0.0f, 0.0f}, 0.25f);
    AF_CHECK_NEAR(l.x, 2.5f, 1e-5f);
    AF_CHECK_NEAR(Lerp(Vec3{1.0f, 2.0f, 3.0f}, Vec3{1.0f, 2.0f, 3.0f}, 0.7f).y, 2.0f, 1e-6f);
}

AF_TEST("Vec2 basics") {
    const Vec2 a{1.0f, 2.0f};
    const Vec2 b{3.0f, 4.0f};
    const Vec2 s = a + b;
    AF_CHECK_EQ(s.x, 4.0f);
    AF_CHECK_EQ(s.y, 6.0f);
    AF_CHECK_NEAR(Dot(a, b), 11.0f, 1e-5f);
    AF_CHECK_NEAR(Length(Vec2{3.0f, 4.0f}), 5.0f, 1e-5f);
    AF_CHECK(a == (Vec2{1.0f, 2.0f}));
}

AF_TEST("Vec4 basics") {
    const Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const Vec4 b{5.0f, 6.0f, 7.0f, 8.0f};
    const Vec4 s = a + b;
    AF_CHECK_EQ(s.w, 12.0f);
    AF_CHECK_NEAR(Dot(a, b), 70.0f, 1e-4f);
    const Vec4 scaled = a * 2.0f;
    AF_CHECK_EQ(scaled.z, 6.0f);
    AF_CHECK_EQ(a[3], 4.0f);
}

AF_TEST("scalar utilities") {
    AF_CHECK_NEAR(Radians(180.0f), Pi, 1e-6f);
    AF_CHECK_NEAR(Degrees(Pi), 180.0f, 1e-4f);
    AF_CHECK_NEAR(Clamp(5.0f, 0.0f, 1.0f), 1.0f, 0.0f);
    AF_CHECK_NEAR(Clamp(-5.0f, 0.0f, 1.0f), 0.0f, 0.0f);
    AF_CHECK_NEAR(Clamp(0.5f, 0.0f, 1.0f), 0.5f, 0.0f);
    AF_CHECK_NEAR(Lerp(0.0f, 10.0f, 0.5f), 5.0f, 1e-6f);
    AF_CHECK_NEAR(Abs(-3.5f), 3.5f, 0.0f);
    AF_CHECK_NEAR(Min(2.0f, -1.0f), -1.0f, 0.0f);
    AF_CHECK_NEAR(Max(2.0f, -1.0f), 2.0f, 0.0f);
}

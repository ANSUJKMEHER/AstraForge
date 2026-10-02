#include "af/collision/CollisionTests.h"
#include "af/collision/Shapes.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("sphere-sphere intersection") {
    const Sphere a{{0.0f, 0.0f, 0.0f}, 1.0f};
    const Sphere b{{1.5f, 0.0f, 0.0f}, 1.0f};
    const Contact c = SphereSphere(a, b);
    AF_CHECK(c.hit);
    AF_CHECK_NEAR(c.depth, 0.5f, 1e-5f);
    AF_CHECK_NEAR(c.normal.x, 1.0f, 1e-5f);  // push a away from b
    AF_CHECK_NEAR(c.normal.y, 0.0f, 1e-5f);
}

AF_TEST("sphere-sphere separation") {
    const Contact c = SphereSphere({{0.0f, 0.0f, 0.0f}, 1.0f}, {{2.1f, 0.0f, 0.0f}, 1.0f});
    AF_CHECK(!c.hit);
    // Touching counts as separated (dist == rSum is not a hit).
    const Contact t = SphereSphere({{0.0f, 0.0f, 0.0f}, 1.0f}, {{2.0f, 0.0f, 0.0f}, 1.0f});
    AF_CHECK(!t.hit);
}

AF_TEST("sphere-sphere coincident centers use a stable normal") {
    const Contact c = SphereSphere({{0.0f, 0.0f, 0.0f}, 1.0f}, {{0.0f, 0.0f, 0.0f}, 1.0f});
    AF_CHECK(c.hit);
    AF_CHECK_NEAR(c.depth, 2.0f, 1e-5f);
    AF_CHECK_NEAR(c.normal.y, 1.0f, 1e-6f);  // documented degenerate choice
}

AF_TEST("sphere-aabb outside face contact") {
    // Sphere approaching the +X face of a unit box at the origin.
    // Center at 1.4: closest point (0.5, 0, 0), distance 0.9 → depth 0.1.
    const Sphere s{{1.4f, 0.0f, 0.0f}, 1.0f};
    const AABB b{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    const Contact c = SphereAABB(s, b);
    AF_CHECK(c.hit);
    AF_CHECK_NEAR(c.depth, 0.1f, 1e-4f);
    // Normal points from sphere (a) toward box (b): -X.
    AF_CHECK_NEAR(c.normal.x, -1.0f, 1e-4f);
}

AF_TEST("sphere-aabb corner contact") {
    // Sphere near the (+X,+Y,+Z) corner: center (0.9,0.9,0.9), r = 1.
    // Closest point (0.5,0.5,0.5); delta (0.4,0.4,0.4), dist = 0.4*sqrt(3).
    const Sphere s{{0.9f, 0.9f, 0.9f}, 1.0f};
    const AABB b{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    const Contact c = SphereAABB(s, b);
    AF_CHECK(c.hit);
    const float cornerDist = 0.4f * std::sqrt(3.0f);
    AF_CHECK_NEAR(c.depth, 1.0f - cornerDist, 1e-4f);
    // Normal points from sphere toward box: away from the corner.
    AF_CHECK_NEAR(c.normal.x, -0.4f / cornerDist, 1e-3f);
    AF_CHECK_NEAR(c.normal.y, -0.4f / cornerDist, 1e-3f);
}

AF_TEST("sphere-aabb no contact") {
    const Contact c = SphereAABB({{3.0f, 0.0f, 0.0f}, 1.0f},
                                 {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}});
    AF_CHECK(!c.hit);
}

AF_TEST("sphere-aabb center inside box") {
    // Sphere centered inside the box, closest to the +X face.
    const Sphere s{{0.25f, 0.0f, 0.0f}, 1.0f};
    const AABB b{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    const Contact c = SphereAABB(s, b);
    AF_CHECK(c.hit);
    AF_CHECK_NEAR(c.normal.x, 1.0f, 1e-5f);  // push out the nearest face
    AF_CHECK_NEAR(c.depth, 1.0f + 0.25f, 1e-5f);  // radius + distance to face
}

AF_TEST("aabb-aabb overlap") {
    const AABB a{{0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 2.0f}};
    const AABB b{{1.0f, 1.0f, 1.0f}, {3.0f, 3.0f, 3.0f}};
    const Contact c = AABBAABB(a, b);
    AF_CHECK(c.hit);
    // Equal overlaps on all axes (1.0): ties resolve to the X axis (-X first
    // in the documented tie order).
    AF_CHECK_NEAR(c.depth, 1.0f, 1e-5f);
    AF_CHECK_NEAR(std::abs(c.normal.x), 1.0f, 1e-6f);
}

AF_TEST("aabb-aabb separated") {
    const Contact c = AABBAABB({{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                               {{2.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 1.0f}});
    AF_CHECK(!c.hit);
    // Touching on a face is not a hit (min == max boundary).
    const Contact t = AABBAABB({{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                               {{1.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 1.0f}});
    AF_CHECK(!t.hit);
}

AF_TEST("aabb-aabb containment uses least penetration face") {
    const AABB big{{-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f}};
    const AABB small{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
    const Contact c = AABBAABB(small, big);
    AF_CHECK(c.hit);
    // Face-to-face overlap per axis is 6 (b.max - a.min = 5 - (-1)); the
    // push-out depth to make the boxes touch is exactly that overlap.
    AF_CHECK_NEAR(c.depth, 6.0f, 1e-4f);
    AF_CHECK_NEAR(std::abs(c.normal.x), 1.0f, 1e-6f);
}

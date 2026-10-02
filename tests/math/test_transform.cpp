#include "af/math/Quat.h"
#include "af/math/Transform.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("transform composes T * R * S") {
    Transform t;
    t.position = Vec3{3.0f, 4.0f, 5.0f};
    t.scale = Vec3{2.0f, 2.0f, 2.0f};
    // Identity rotation: (1,0,0) scales to (2,0,0), then translates to (5,4,5).
    const Vec3 p = TransformPoint(t.ToMat4(), Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(p.x, 5.0f, 1e-5f);
    AF_CHECK_NEAR(p.y, 4.0f, 1e-5f);
    AF_CHECK_NEAR(p.z, 5.0f, 1e-5f);
}

AF_TEST("transform rotation applies before translation") {
    Transform t;
    t.position = Vec3{10.0f, 0.0f, 0.0f};
    t.rotation = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(90.0f));
    // (1,0,0) rotated +90Y -> (0,0,-1); then translated -> (10,0,-1).
    const Vec3 p = TransformPoint(t.ToMat4(), Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(p.x, 10.0f, 1e-4f);
    AF_CHECK_NEAR(p.y, 0.0f, 1e-4f);
    AF_CHECK_NEAR(p.z, -1.0f, 1e-4f);
}

AF_TEST("transform basis vectors") {
    Transform t;
    t.rotation = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(90.0f));
    const Vec3 f = t.Forward();
    AF_CHECK_NEAR(f.x, -1.0f, 1e-4f);  // forward rotated 90 degrees left
    AF_CHECK_NEAR(f.z, 0.0f, 1e-4f);
    const Vec3 r = t.Right();
    AF_CHECK_NEAR(r.z, -1.0f, 1e-4f);  // right rotated onto -Z
    const Vec3 u = t.Up();
    AF_CHECK_NEAR(u.y, 1.0f, 1e-4f);   // Y rotation keeps up unchanged
    // Basis is orthonormal.
    AF_CHECK_NEAR(Dot(f, u), 0.0f, 1e-5f);
    AF_CHECK_NEAR(Dot(r, u), 0.0f, 1e-5f);
    AF_CHECK_NEAR(Dot(f, r), 0.0f, 1e-5f);
}

AF_TEST("transform default is identity") {
    const Transform t;
    const Vec3 p = TransformPoint(t.ToMat4(), Vec3{1.0f, 2.0f, 3.0f});
    AF_CHECK_NEAR(p.x, 1.0f, 1e-6f);
    AF_CHECK_NEAR(p.y, 2.0f, 1e-6f);
    AF_CHECK_NEAR(p.z, 3.0f, 1e-6f);
    // Default basis vectors.
    AF_CHECK_NEAR(t.Forward().z, -1.0f, 1e-6f);
    AF_CHECK_NEAR(t.Right().x, 1.0f, 1e-6f);
    AF_CHECK_NEAR(t.Up().y, 1.0f, 1e-6f);
}

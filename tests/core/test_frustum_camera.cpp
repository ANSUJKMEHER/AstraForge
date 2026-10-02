#include "af/core/Frustum.h"
#include "af/core/Camera.h"
#include "af/math/Mat4.h"
#include "af/math/MathUtil.h"
#include "af/math/Vec3.h"
#include "framework/af_test.hpp"

using namespace af;

AF_TEST("identity matrix yields the unit cube frustum") {
    const Frustum f = Frustum::FromViewProjection(Mat4::Identity());
    // Inside: the origin and points near the cube faces.
    AF_CHECK(f.ContainsPoint(Vec3{0.0f, 0.0f, 0.0f}));
    AF_CHECK(f.ContainsPoint(Vec3{0.99f, -0.99f, 0.99f}));
    // Outside each axis.
    AF_CHECK(!f.ContainsPoint(Vec3{2.0f, 0.0f, 0.0f}));
    AF_CHECK(!f.ContainsPoint(Vec3{-2.0f, 0.0f, 0.0f}));
    AF_CHECK(!f.ContainsPoint(Vec3{0.0f, 2.0f, 0.0f}));
    AF_CHECK(!f.ContainsPoint(Vec3{0.0f, -2.0f, 0.0f}));
    AF_CHECK(!f.ContainsPoint(Vec3{0.0f, 0.0f, 2.0f}));
    AF_CHECK(!f.ContainsPoint(Vec3{0.0f, 0.0f, -2.0f}));
    // Corner outside two planes.
    AF_CHECK(!f.ContainsPoint(Vec3{2.0f, 2.0f, 0.0f}));
}

AF_TEST("sphere vs identity frustum") {
    const Frustum f = Frustum::FromViewProjection(Mat4::Identity());
    AF_CHECK(f.IntersectsSphere(Vec3{0.0f, 0.0f, 0.0f}, 0.5f));
    // Sphere straddling the right face still intersects.
    AF_CHECK(f.IntersectsSphere(Vec3{0.8f, 0.0f, 0.0f}, 0.5f));
    // Sphere fully past the face is culled.
    AF_CHECK(!f.IntersectsSphere(Vec3{2.0f, 0.0f, 0.0f}, 0.5f));
    AF_CHECK(!f.IntersectsSphere(Vec3{0.0f, 0.0f, 2.0f}, 0.5f));
}

AF_TEST("frustum of a real camera contains what it looks at") {
    Camera camera;
    camera.SetPerspective(60.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    camera.SetTransform(Vec3{0.0f, 0.0f, 5.0f}, 0.0f, 0.0f);  // looking at origin
    const Frustum f = camera.GetFrustum();

    // The origin is in front, inside the frustum.
    AF_CHECK(f.ContainsPoint(Vec3{0.0f, 0.0f, 0.0f}));
    // A point behind the camera is outside (near plane).
    AF_CHECK(!f.ContainsPoint(Vec3{0.0f, 0.0f, 10.0f}));
    // A point far to the side is outside.
    AF_CHECK(!f.ContainsPoint(Vec3{50.0f, 0.0f, 0.0f}));
    // A sphere around the origin intersects.
    AF_CHECK(f.IntersectsSphere(Vec3{0.0f, 0.0f, 0.0f}, 1.0f));
}

AF_TEST("camera orbit places the camera behind its forward") {
    Camera camera;
    camera.Orbit(Vec3{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 5.0f);
    // Looking down −Z from (0,0,5).
    AF_CHECK_NEAR(camera.Position().x, 0.0f, 1e-5f);
    AF_CHECK_NEAR(camera.Position().z, 5.0f, 1e-5f);
    AF_CHECK_NEAR(camera.Forward().z, -1.0f, 1e-5f);
    AF_CHECK_NEAR(camera.Right().x, 1.0f, 1e-5f);
    // The view matrix maps the target to −distance on the view Z axis.
    const Vec3 view = TransformPoint(camera.ViewMatrix(), Vec3{0.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(view.z, -5.0f, 1e-4f);
}

AF_TEST("camera yaw rotates position around the target") {
    Camera camera;
    camera.Orbit(Vec3{0.0f, 0.0f, 0.0f}, Radians(90.0f), 0.0f, 5.0f);
    // +90° yaw: forward is +X, so the camera sits at −X.
    AF_CHECK_NEAR(camera.Forward().x, 1.0f, 1e-4f);
    AF_CHECK_NEAR(camera.Position().x, -5.0f, 1e-3f);
    AF_CHECK_NEAR(camera.Position().z, 0.0f, 1e-3f);
}

AF_TEST("camera pitch is clamped to avoid degeneracy") {
    Camera camera;
    camera.Orbit(Vec3{0.0f, 0.0f, 0.0f}, 0.0f, Radians(120.0f), 5.0f);
    // Forward must never be parallel to world up: pitch clamped to 89°.
    AF_CHECK(camera.Forward().y < 0.9999f);  // sin(89°) ≈ 0.99985
    AF_CHECK_NEAR(camera.Forward().y, std::sin(Radians(89.0f)), 1e-3f);
}

AF_TEST("camera basis vectors are orthonormal") {
    Camera camera;
    camera.Orbit(Vec3{0.0f, 0.0f, 0.0f}, Radians(35.0f), Radians(20.0f), 8.0f);
    const Vec3 f = camera.Forward();
    const Vec3 r = camera.Right();
    const Vec3 u = camera.Up();
    AF_CHECK_NEAR(Length(f), 1.0f, 1e-5f);
    AF_CHECK_NEAR(Length(r), 1.0f, 1e-5f);
    AF_CHECK_NEAR(Length(u), 1.0f, 1e-5f);
    AF_CHECK_NEAR(Dot(f, r), 0.0f, 1e-5f);
    AF_CHECK_NEAR(Dot(f, u), 0.0f, 1e-5f);
    AF_CHECK_NEAR(Dot(r, u), 0.0f, 1e-5f);
}

AF_TEST("camera view-projection composes view after projection") {
    Camera camera;
    camera.SetPerspective(60.0f, 1.5f, 0.1f, 100.0f);
    camera.SetTransform(Vec3{0.0f, 2.0f, 8.0f}, 0.0f, 0.0f);
    const Mat4 vp = camera.ViewProjection();
    const Mat4 expected = camera.ProjectionMatrix() * camera.ViewMatrix();
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) AF_CHECK_NEAR(vp(r, c), expected(r, c), 1e-5f);
}

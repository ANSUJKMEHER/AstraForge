#include <cmath>
#include <cstdint>

#include "af/math/Mat4.h"
#include "af/math/MathUtil.h"
#include "af/math/Quat.h"
#include "af/math/Vec3.h"
#include "framework/af_test.hpp"

using namespace af;

namespace {

float NextRand(uint32_t& seed) {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f - 0.5f;
}

}  // namespace

AF_TEST("quaternion identity") {
    const Quat q = Quat::Identity();
    AF_CHECK_NEAR(Length(q), 1.0f, 1e-6f);
    const Vec3 v{0.3f, -1.2f, 0.7f};
    const Vec3 r = Rotate(q, v);
    AF_CHECK_NEAR(r.x, v.x, 1e-6f);
    AF_CHECK_NEAR(r.y, v.y, 1e-6f);
    AF_CHECK_NEAR(r.z, v.z, 1e-6f);
}

AF_TEST("axis-angle rotations") {
    const Quat qy = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(90.0f));
    const Vec3 r = Rotate(qy, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(r.x, 0.0f, 1e-4f);
    AF_CHECK_NEAR(r.y, 0.0f, 1e-4f);
    AF_CHECK_NEAR(r.z, -1.0f, 1e-4f);

    const Quat qz = FromAxisAngle(Vec3{0.0f, 0.0f, 1.0f}, Radians(90.0f));
    const Vec3 r2 = Rotate(qz, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(r2.y, 1.0f, 1e-4f);

    // 180 degrees about Y flips +X to -X.
    const Quat qh = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(180.0f));
    const Vec3 r3 = Rotate(qh, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(r3.x, -1.0f, 1e-4f);

    // Axis is normalized internally.
    const Quat qn = FromAxisAngle(Vec3{0.0f, 2.0f, 0.0f}, Radians(90.0f));
    AF_CHECK_NEAR(Length(qn), 1.0f, 1e-5f);
}

AF_TEST("quaternion composition applies right operand first") {
    const Quat qy = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(90.0f));
    const Quat qz = FromAxisAngle(Vec3{0.0f, 0.0f, 1.0f}, Radians(90.0f));
    // qz maps X -> Y; qy leaves Y alone: combined maps X -> Y.
    const Vec3 r = Rotate(qy * qz, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(r.y, 1.0f, 1e-4f);
    AF_CHECK_NEAR(r.z, 0.0f, 1e-4f);
}

AF_TEST("quaternion matches matrix rotation") {
    uint32_t seed = 424242u;
    const Vec3 v{0.5f, -0.8f, 0.3f};
    for (int i = 0; i < 12; ++i) {
        const Vec3 axis = Normalize(Vec3{NextRand(seed), NextRand(seed), NextRand(seed)});
        const float angle = Radians(NextRand(seed) * 720.0f);  // may exceed 360
        const Quat q = FromAxisAngle(axis, angle);
        const Vec3 viaQuat = Rotate(q, v);
        const Vec3 viaMat = TransformDirection(ToMat4(q), v);
        AF_CHECK_NEAR(viaQuat.x, viaMat.x, 1e-3f);
        AF_CHECK_NEAR(viaQuat.y, viaMat.y, 1e-3f);
        AF_CHECK_NEAR(viaQuat.z, viaMat.z, 1e-3f);
    }
}

AF_TEST("quaternion inverse and double cover") {
    const Quat q = FromAxisAngle(Normalize(Vec3{1.0f, 1.0f, 1.0f}), Radians(60.0f));
    const Quat ident = q * Inverse(q);
    AF_CHECK_NEAR(ident.w, 1.0f, 1e-4f);
    AF_CHECK_NEAR(ident.x, 0.0f, 1e-4f);
    AF_CHECK_NEAR(ident.y, 0.0f, 1e-4f);
    AF_CHECK_NEAR(ident.z, 0.0f, 1e-4f);

    // -q represents the same rotation (double cover).
    const Vec3 v{0.2f, 0.9f, -0.4f};
    const Vec3 r1 = Rotate(q, v);
    const Vec3 r2 = Rotate(-q, v);
    AF_CHECK_NEAR(r1.x, r2.x, 1e-4f);
    AF_CHECK_NEAR(r1.y, r2.y, 1e-4f);
    AF_CHECK_NEAR(r1.z, r2.z, 1e-4f);
}

AF_TEST("slerp midpoint") {
    const Quat a = Quat::Identity();
    const Quat b = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(90.0f));
    // Midpoint rotation is 45 degrees about Y.
    const Quat m = Slerp(a, b, 0.5f);
    const Vec3 r = Rotate(m, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(r.x, std::cos(Pi / 4.0f), 1e-4f);
    AF_CHECK_NEAR(r.z, -std::sin(Pi / 4.0f), 1e-4f);
    // Endpoints.
    const Vec3 e0 = Rotate(Slerp(a, b, 0.0f), Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(e0.x, 1.0f, 1e-4f);
    const Vec3 e1 = Rotate(Slerp(a, b, 1.0f), Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(e1.z, -1.0f, 1e-4f);
}

AF_TEST("slerp takes the shorter arc") {
    const Quat a = Quat::Identity();
    const Quat b = FromAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, Radians(270.0f));  // 270 == -90
    const Quat m = Slerp(a, b, 0.5f);
    // Midpoint must be -45 degrees about Y (X -> +Z side), not +135.
    const Vec3 r = Rotate(m, Vec3{1.0f, 0.0f, 0.0f});
    AF_CHECK_NEAR(r.x, std::cos(Pi / 4.0f), 1e-4f);
    AF_CHECK_NEAR(r.z, std::sin(Pi / 4.0f), 1e-4f);
}

AF_TEST("FromEuler matches YXZ matrix composition") {
    const float yaw = Radians(30.0f);
    const float pitch = Radians(20.0f);
    const float roll = Radians(10.0f);
    const Quat q = FromEuler(yaw, pitch, roll);
    const Mat4 m = Mat4::RotationY(yaw) * Mat4::RotationX(pitch) * Mat4::RotationZ(roll);
    const Vec3 v{0.4f, -0.2f, 0.9f};
    const Vec3 rq = Rotate(q, v);
    const Vec3 rm = TransformDirection(m, v);
    AF_CHECK_NEAR(rq.x, rm.x, 1e-3f);
    AF_CHECK_NEAR(rq.y, rm.y, 1e-3f);
    AF_CHECK_NEAR(rq.z, rm.z, 1e-3f);
}

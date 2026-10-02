#pragma once
// Unit quaternion for rotations. Storage order: (x, y, z, w), w = real part.
// Rotates in the same right-handed space as Mat4 (see docs/math-conventions.md).
// Convention: applying (a * b) to v equals applying b first, then a.

#include <cmath>

#include "Mat4.h"
#include "MathUtil.h"
#include "Vec3.h"

namespace af {

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    constexpr Quat() = default;
    constexpr Quat(float xv, float yv, float zv, float wv) : x(xv), y(yv), z(zv), w(wv) {}

    static constexpr Quat Identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
};

inline Quat operator-(const Quat& q) { return {-q.x, -q.y, -q.z, -q.w}; }
inline Quat operator+(const Quat& a, const Quat& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Quat operator-(const Quat& a, const Quat& b) { return a + (-b); }
inline Quat operator*(const Quat& a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
inline Quat operator*(float s, const Quat& a) { return a * s; }

inline float Dot(const Quat& a, const Quat& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

// Hamilton product. `a * b` rotates by b first, then a.
inline Quat operator*(const Quat& a, const Quat& b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

inline float LengthSq(const Quat& q) { return Dot(q, q); }
inline float Length(const Quat& q) { return std::sqrt(LengthSq(q)); }

inline Quat Normalize(const Quat& q) {
    const float len = Length(q);
    return len > 0.0f ? q * (1.0f / len) : Quat::Identity();
}

// Conjugate == inverse for unit quaternions.
inline Quat Conjugate(const Quat& q) { return {-q.x, -q.y, -q.z, q.w}; }
inline Quat Inverse(const Quat& q) { return Conjugate(q) * (1.0f / LengthSq(q)); }

inline Quat FromAxisAngle(const Vec3& axis, float radians) {
    const Vec3 a = Normalize(axis);
    const float half = radians * 0.5f;
    const float s = std::sin(half);
    return {a.x * s, a.y * s, a.z * s, std::cos(half)};
}

// Intrinsic YXZ Euler order (yaw about Y, pitch about X, roll about Z):
// q = qY(yaw) * qX(pitch) * qZ(roll). All inputs in radians.
inline Quat FromEuler(float yaw, float pitch, float roll) {
    const Quat qy = FromAxisAngle({0.0f, 1.0f, 0.0f}, yaw);
    const Quat qx = FromAxisAngle({1.0f, 0.0f, 0.0f}, pitch);
    const Quat qz = FromAxisAngle({0.0f, 0.0f, 1.0f}, roll);
    return qy * qx * qz;
}

// Rotate v by the (assumed unit) quaternion: v' = q v q*.
inline Vec3 Rotate(const Quat& q, const Vec3& v) {
    const Vec3 qv{q.x, q.y, q.z};
    const Vec3 t = 2.0f * Cross(qv, v);
    return v + q.w * t + Cross(qv, t);
}

inline Mat4 ToMat4(const Quat& q) {
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    Mat4 m;
    m(0, 0) = 1.0f - 2.0f * (yy + zz); m(0, 1) = 2.0f * (xy - wz);       m(0, 2) = 2.0f * (xz + wy);
    m(1, 0) = 2.0f * (xy + wz);       m(1, 1) = 1.0f - 2.0f * (xx + zz); m(1, 2) = 2.0f * (yz - wx);
    m(2, 0) = 2.0f * (xz - wy);       m(2, 1) = 2.0f * (yz + wx);       m(2, 2) = 1.0f - 2.0f * (xx + yy);
    return m;
}

inline Quat Nlerp(const Quat& a, const Quat& b, float t) { return Normalize(a + (b - a) * t); }

// Spherical linear interpolation along the shorter arc.
inline Quat Slerp(const Quat& a, const Quat& b, float t) {
    Quat qb = b;
    float d = Dot(a, b);
    if (d < 0.0f) { d = -d; qb = -qb; }  // -q represents the same rotation
    if (d > 0.9995f) return Nlerp(a, qb, t);
    const float angle = std::acos(Clamp(d, -1.0f, 1.0f));
    const float sa = std::sin(angle);
    const float wa = std::sin((1.0f - t) * angle) / sa;
    const float wb = std::sin(t * angle) / sa;
    return Normalize(a * wa + qb * wb);
}

}  // namespace af

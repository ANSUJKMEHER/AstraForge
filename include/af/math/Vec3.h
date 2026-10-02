#pragma once
// 3-component float vector — the workhorse of the engine.

#include <cmath>
#include <cstddef>

#include "MathUtil.h"

namespace af {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float xv, float yv, float zv) : x(xv), y(yv), z(zv) {}

    constexpr float& operator[](std::size_t i) {
        switch (i) {
            case 0: return x;
            case 1: return y;
            default: return z;
        }
    }
    constexpr const float& operator[](std::size_t i) const {
        switch (i) {
            case 0: return x;
            case 1: return y;
            default: return z;
        }
    }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(const Vec3& a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, const Vec3& a) { return a * s; }
inline Vec3 operator/(const Vec3& a, float s) { return {a.x / s, a.y / s, a.z / s}; }

inline bool operator==(const Vec3& a, const Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(const Vec3& a, const Vec3& b) { return !(a == b); }

inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// Right-handed cross product: X x Y = Z.
inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline float LengthSq(const Vec3& v) { return Dot(v, v); }
inline float Length(const Vec3& v) { return std::sqrt(LengthSq(v)); }

// Zero-length input normalizes to the zero vector (documented behavior — no
// NaN/assert; callers that require a direction must guard).
inline Vec3 Normalize(const Vec3& v) {
    const float len = Length(v);
    return len > 0.0f ? v / len : Vec3{0.0f, 0.0f, 0.0f};
}

inline float Distance(const Vec3& a, const Vec3& b) { return Length(b - a); }
inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }

}  // namespace af

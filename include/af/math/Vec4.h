#pragma once
// 4-component float vector (homogeneous points, RGBA colors).

#include <cstddef>

#include "MathUtil.h"

namespace af {

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr Vec4() = default;
    constexpr Vec4(float xv, float yv, float zv, float wv) : x(xv), y(yv), z(zv), w(wv) {}

    constexpr float& operator[](std::size_t i) {
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return w;
        }
    }
    constexpr const float& operator[](std::size_t i) const {
        switch (i) {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: return w;
        }
    }

    Vec4& operator+=(const Vec4& o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
    Vec4& operator-=(const Vec4& o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
    Vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
};

inline Vec4 operator+(const Vec4& a, const Vec4& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vec4 operator-(const Vec4& a, const Vec4& b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Vec4 operator*(const Vec4& a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
inline Vec4 operator*(float s, const Vec4& a) { return a * s; }

inline bool operator==(const Vec4& a, const Vec4& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
inline bool operator!=(const Vec4& a, const Vec4& b) { return !(a == b); }

inline float Dot(const Vec4& a, const Vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

}  // namespace af

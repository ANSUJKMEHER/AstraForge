#pragma once
// 2-component float vector (UI, texture coordinates, 2D queries).

#include <cmath>
#include <cstddef>

#include "MathUtil.h"

namespace af {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float xv, float yv) : x(xv), y(yv) {}

    constexpr float& operator[](std::size_t i) { return i == 0 ? x : y; }
    constexpr const float& operator[](std::size_t i) const { return i == 0 ? x : y; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }
};

inline Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator-(const Vec2& a) { return {-a.x, -a.y}; }
inline Vec2 operator*(const Vec2& a, float s) { return {a.x * s, a.y * s}; }
inline Vec2 operator*(float s, const Vec2& a) { return a * s; }
inline Vec2 operator/(const Vec2& a, float s) { return {a.x / s, a.y / s}; }

inline bool operator==(const Vec2& a, const Vec2& b) { return a.x == b.x && a.y == b.y; }
inline bool operator!=(const Vec2& a, const Vec2& b) { return !(a == b); }

inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float LengthSq(const Vec2& v) { return Dot(v, v); }
inline float Length(const Vec2& v) { return std::sqrt(LengthSq(v)); }
inline Vec2 Normalize(const Vec2& v) {
    const float len = Length(v);
    return len > 0.0f ? v / len : Vec2{0.0f, 0.0f};
}

}  // namespace af

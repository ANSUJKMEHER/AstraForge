#pragma once
// Common scalar math utilities shared by the vector/matrix types.

namespace af {

inline constexpr float Pi = 3.14159265358979323846f;

inline constexpr float Radians(float degrees) { return degrees * (Pi / 180.0f); }
inline constexpr float Degrees(float radians) { return radians * (180.0f / Pi); }

template <typename T>
inline constexpr T Abs(T v) { return v < T(0) ? -v : v; }

template <typename T>
inline constexpr T Min(T a, T b) { return a < b ? a : b; }

template <typename T>
inline constexpr T Max(T a, T b) { return a > b ? a : b; }

template <typename T>
inline constexpr T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

template <typename T>
inline constexpr T Lerp(T a, T b, T t) { return a + (b - a) * t; }

// Default tolerance for approximate float comparisons in tests.
inline constexpr float Epsilon = 1e-5f;

}  // namespace af

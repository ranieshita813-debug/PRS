#pragma once
#include "PRS/Core/Types.h"
#include <cmath>

// Scalar math shared by the vector/matrix/quaternion types.
// Angles are radians everywhere unless a function name says otherwise.
namespace PRS
{
inline constexpr f32 Pi = 3.14159265358979323846f;
inline constexpr f32 TwoPi = 6.28318530717958647692f;
inline constexpr f32 HalfPi = 1.57079632679489661923f;
inline constexpr f32 Epsilon = 1.0e-5f;

constexpr f32 Radians(f32 degrees) { return degrees * (Pi / 180.0f); }
constexpr f32 Degrees(f32 radians) { return radians * (180.0f / Pi); }

template <typename T> constexpr T Min(T a, T b) { return a < b ? a : b; }
template <typename T> constexpr T Max(T a, T b) { return a > b ? a : b; }
template <typename T> constexpr T Clamp(T value, T low, T high)
{
    return value < low ? low : (value > high ? high : value);
}
template <typename T> constexpr T Lerp(T a, T b, f32 t) { return static_cast<T>(a + (b - a) * t); }
template <typename T> constexpr T Square(T v) { return v * v; }

constexpr f32 Abs(f32 v) { return v < 0.0f ? -v : v; }
constexpr f32 Sign(f32 v) { return v < 0.0f ? -1.0f : (v > 0.0f ? 1.0f : 0.0f); }
inline f32 Sqrt(f32 v) { return v <= 0.0f ? 0.0f : std::sqrt(v); }
constexpr bool NearlyEqual(f32 a, f32 b, f32 eps = Epsilon) { return Abs(a - b) <= eps; }
inline bool NearlyZero(f32 v, f32 eps = Epsilon) { return Abs(v) <= eps; }
} // namespace PRS

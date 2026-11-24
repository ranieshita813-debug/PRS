#pragma once
#include "PRS/Core/Math/Common.h"

namespace PRS
{
struct Vec2
{
    f32 x = 0.0f;
    f32 y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(f32 s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(f32 s) const { return {x / s, y / s}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2& operator*=(f32 s) { x *= s; y *= s; return *this; }
};

inline constexpr f32 Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline constexpr f32 Cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
inline f32 Length(const Vec2& v) { return Sqrt(Dot(v, v)); }
inline constexpr f32 LengthSquared(const Vec2& v) { return Dot(v, v); }
inline Vec2 Normalize(const Vec2& v)
{
    const f32 len = Length(v);
    return len > Epsilon ? v / len : Vec2{};
}
inline constexpr Vec2 Lerp(const Vec2& a, const Vec2& b, f32 t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

struct Vec3
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
    constexpr explicit Vec3(f32 fill) : x(fill), y(fill), z(fill) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(f32 s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }
    constexpr Vec3 operator/(f32 s) const { return {x / s, y / s, z / s}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    constexpr Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    constexpr Vec3& operator*=(f32 s) { x *= s; y *= s; z *= s; return *this; }
    constexpr bool operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    constexpr bool operator!=(const Vec3& o) const { return !(*this == o); }
};

inline constexpr f32 Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline constexpr Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline constexpr f32 LengthSquared(const Vec3& v) { return Dot(v, v); }
inline f32 Length(const Vec3& v) { return Sqrt(LengthSquared(v)); }
inline f32 Distance(const Vec3& a, const Vec3& b) { return Length(b - a); }
inline constexpr f32 DistanceSquared(const Vec3& a, const Vec3& b) { return LengthSquared(b - a); }
inline Vec3 Normalize(const Vec3& v)
{
    const f32 len = Length(v);
    return len > Epsilon ? v / len : Vec3{};
}
inline constexpr Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t)
{
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}
inline constexpr Vec3 Min(const Vec3& a, const Vec3& b)
{
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z};
}
inline constexpr Vec3 Max(const Vec3& a, const Vec3& b)
{
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z};
}
inline constexpr Vec3 Reflect(const Vec3& incident, const Vec3& normal)
{
    return incident - normal * (2.0f * Dot(incident, normal));
}
inline Vec3 ProjectOntoPlane(const Vec3& v, const Vec3& planeNormal)
{
    const Vec3 n = Normalize(planeNormal);
    return v - n * Dot(v, n);
}
inline Vec3 CrossNormalized(const Vec3& a, const Vec3& b) { return Normalize(Cross(a, b)); }

// Right-handed camera basis helpers.
inline constexpr Vec3 AxisX() { return {1.0f, 0.0f, 0.0f}; }
inline constexpr Vec3 AxisY() { return {0.0f, 1.0f, 0.0f}; }
inline constexpr Vec3 AxisZ() { return {0.0f, 0.0f, 1.0f}; }
inline constexpr Vec3 Zero() { return {}; }

struct Vec4
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 w = 0.0f;

    constexpr Vec4() = default;
    constexpr Vec4(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3& v, f32 w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    constexpr Vec3 XYZ() const { return {x, y, z}; }
    constexpr Vec4 operator+(const Vec4& o) const { return {x + o.x, y + o.y, z + o.z, w + o.w}; }
    constexpr Vec4 operator-(const Vec4& o) const { return {x - o.x, y - o.y, z - o.z, w - o.w}; }
    constexpr Vec4 operator*(f32 s) const { return {x * s, y * s, z * s, w * s}; }
};

inline constexpr f32 Dot(const Vec4& a, const Vec4& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
} // namespace PRS

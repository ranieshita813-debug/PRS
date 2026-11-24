#pragma once
#include "PRS/Core/Types.h"
#include <cmath>

namespace PRS::UI
{
inline f32 Clamp(f32 v, f32 lo, f32 hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline f32 Lerp(f32 a, f32 b, f32 t) { return a + (b - a) * t; }
inline f32 Abs(f32 v) { return v < 0.0f ? -v : v; }
inline f32 Max(f32 a, f32 b) { return a > b ? a : b; }
inline f32 Min(f32 a, f32 b) { return a < b ? a : b; }
inline f32 Sqrt(f32 v) { return v <= 0.0f ? 0.0f : std::sqrt(v); }

struct Vec2
{
    f32 x = 0.0f;
    f32 y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(f32 x_, f32 y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(f32 s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }
};

inline f32 Length(const Vec2& v) { return Sqrt(v.x * v.x + v.y * v.y); }

struct Rect
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 w = 0.0f;
    f32 h = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(f32 x_, f32 y_, f32 w_, f32 h_) : x(x_), y(y_), w(w_), h(h_) {}

    f32 Left() const { return x; }
    f32 Top() const { return y; }
    f32 Right() const { return x + w; }
    f32 Bottom() const { return y + h; }
    Vec2 Center() const { return {x + w * 0.5f, y + h * 0.5f}; }
    bool Empty() const { return w <= 0.0f || h <= 0.0f; }

    bool Contains(const Vec2& p) const
    {
        return p.x >= x && p.x < x + w && p.y >= y && p.y < y + h;
    }

    Rect Inset(f32 amount) const { return {x + amount, y + amount, w - amount * 2.0f, h - amount * 2.0f}; }
    Rect InsetX(f32 amount) const { return {x + amount, y, w - amount * 2.0f, h}; }
    Rect Offset(f32 dx, f32 dy) const { return {x + dx, y + dy, w, h}; }

    Rect WithHeight(f32 height) const { return {x, y, w, height}; }
    Rect SliceTop(f32 height) const { return {x, y, w, height}; }
    Rect SliceBottom(f32 height) const { return {x, y + h - height, w, height}; }

    // Remaining area below a header of the given height.
    Rect Below(f32 height) const { return {x, y + height, w, h - height}; }

    Rect ClipTo(const Rect& other) const
    {
        f32 l = Max(x, other.x);
        f32 t = Max(y, other.y);
        f32 r = Min(Right(), other.Right());
        f32 b = Min(Bottom(), other.Bottom());
        if (r <= l || b <= t) return {};
        return {l, t, r - l, b - t};
    }
};

struct Color
{
    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    f32 a = 1.0f;

    constexpr Color() = default;
    constexpr Color(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static Color FromBytes(u8 r, u8 g, u8 b, u8 a = 255)
    {
        return {static_cast<f32>(r) / 255.0f, static_cast<f32>(g) / 255.0f,
                static_cast<f32>(b) / 255.0f, static_cast<f32>(a) / 255.0f};
    }

    Color WithAlpha(f32 alpha) const { return {r, g, b, alpha}; }
    Color ScaleAlpha(f32 k) const { return {r, g, b, a * k}; }
    Color Lerp(const Color& o, f32 t) const
    {
        f32 k = Clamp(t, 0.0f, 1.0f);
        return {r + (o.r - r) * k, g + (o.g - g) * k, b + (o.b - b) * k, a + (o.a - a) * k};
    }
};

enum class TextAlign : u8
{
    Left,
    Center,
    Right
};
} // namespace PRS::UI

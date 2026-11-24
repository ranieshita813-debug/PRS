#pragma once
#include "PRS/Core/Math/Matrix.h"

namespace PRS
{
struct Quat
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 w = 1.0f;

    constexpr Quat() = default;
    constexpr Quat(f32 x_, f32 y_, f32 z_, f32 w_) : x(x_), y(y_), z(z_), w(w_) {}

    static constexpr Quat Identity() { return Quat{}; }

    static Quat FromAxisAngle(const Vec3& axis, f32 angleRadians)
    {
        const Vec3 a = Normalize(axis);
        const f32 half = angleRadians * 0.5f;
        const f32 s = std::sin(half);
        return {a.x * s, a.y * s, a.z * s, std::cos(half)};
    }

    // Yaw (Y) then pitch (X) then roll (Z), angles in radians.
    static Quat FromEuler(f32 pitch, f32 yaw, f32 roll)
    {
        const Quat qx = FromAxisAngle(AxisX(), pitch);
        const Quat qy = FromAxisAngle(AxisY(), yaw);
        const Quat qz = FromAxisAngle(AxisZ(), roll);
        return (qy * qx * qz).Normalized();
    }

    // Returns {pitch, yaw, roll} in radians for a YXZ-ordered rotation.
    Vec3 ToEuler() const
    {
        const f32 sinPitch = Clamp(2.0f * (w * x - y * z), -1.0f, 1.0f);
        const f32 pitch = std::asin(sinPitch);
        f32 yaw = 0.0f;
        f32 roll = 0.0f;
        if (Abs(sinPitch) < 0.9999f)
        {
            yaw = std::atan2(2.0f * (w * y + x * z), 1.0f - 2.0f * (x * x + y * y));
            roll = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (x * x + z * z));
        }
        else
        {
            yaw = std::atan2(-2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z));
        }
        return {pitch, yaw, roll};
    }

    constexpr f32 LengthSquared() const { return x * x + y * y + z * z + w * w; }
    f32 Length() const { return Sqrt(LengthSquared()); }

    Quat Normalized() const
    {
        const f32 len = Length();
        return len > Epsilon ? Quat{x / len, y / len, z / len, w / len} : Quat{};
    }

    constexpr Quat Conjugate() const { return {-x, -y, -z, w}; }

    // Inverse of a unit quaternion is its conjugate.
    Quat Inverted() const { return Conjugate(); }

    constexpr Quat operator*(const Quat& o) const
    {
        return {w * o.x + x * o.w + y * o.z - z * o.y,
                w * o.y - x * o.z + y * o.w + z * o.x,
                w * o.z + x * o.y - y * o.x + z * o.w,
                w * o.w - x * o.x - y * o.y - z * o.z};
    }

    constexpr Vec3 Rotate(const Vec3& v) const
    {
        // v' = v + 2 * cross(q.xyz, cross(q.xyz, v) + w * v)
        const Vec3 q{x, y, z};
        const Vec3 t = Cross(q, v) + v * w;
        return v + Cross(q, t) * 2.0f;
    }

    static Quat Slerp(const Quat& a, const Quat& b, f32 t)
    {
        Quat from = a.Normalized();
        Quat to = b.Normalized();
        f32 cosTheta = from.x * to.x + from.y * to.y + from.z * to.z + from.w * to.w;
        if (cosTheta < 0.0f)
        {
            to = {-to.x, -to.y, -to.z, -to.w};
            cosTheta = -cosTheta;
        }
        if (cosTheta > 0.9995f)
        {
            Quat r{from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t,
                   from.z + (to.z - from.z) * t, from.w + (to.w - from.w) * t};
            return r.Normalized();
        }
        const f32 theta = std::acos(Clamp(cosTheta, -1.0f, 1.0f));
        const f32 sinTheta = std::sin(theta);
        const f32 scaleFrom = std::sin((1.0f - t) * theta) / sinTheta;
        const f32 scaleTo = std::sin(t * theta) / sinTheta;
        return Quat{from.x * scaleFrom + to.x * scaleTo, from.y * scaleFrom + to.y * scaleTo,
                    from.z * scaleFrom + to.z * scaleTo, from.w * scaleFrom + to.w * scaleTo}
            .Normalized();
    }

    // Cheaper than Slerp for small steps, same result for nearly parallel quaternions.
    static Quat Nlerp(const Quat& a, const Quat& b, f32 t)
    {
        Quat to = b;
        const f32 cosTheta = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        if (cosTheta < 0.0f) to = {-b.x, -b.y, -b.z, -b.w};
        return Quat{a.x + (to.x - a.x) * t, a.y + (to.y - a.y) * t, a.z + (to.z - a.z) * t,
                    a.w + (to.w - a.w) * t}
            .Normalized();
    }

    Mat4 ToMatrix() const
    {
        const Quat q = Normalized();
        const f32 xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        const f32 xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        const f32 wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        Mat4 r;
        r.At(0, 0) = 1.0f - 2.0f * (yy + zz);
        r.At(0, 1) = 2.0f * (xy - wz);
        r.At(0, 2) = 2.0f * (xz + wy);
        r.At(1, 0) = 2.0f * (xy + wz);
        r.At(1, 1) = 1.0f - 2.0f * (xx + zz);
        r.At(1, 2) = 2.0f * (yz - wx);
        r.At(2, 0) = 2.0f * (xz - wy);
        r.At(2, 1) = 2.0f * (yz + wx);
        r.At(2, 2) = 1.0f - 2.0f * (xx + yy);
        return r;
    }
};
} // namespace PRS

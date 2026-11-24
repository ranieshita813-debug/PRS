#pragma once
#include "PRS/Core/Math/Quat.h"

namespace PRS
{
// Translation, rotation, scale. Local space; parents compose by matrix multiply.
struct Transform
{
    Vec3 position{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    Mat4 Matrix() const { return Mat4::Translation(position) * rotation.ToMatrix() * Mat4::Scale(scale); }

    Vec3 TransformPoint(const Vec3& p) const
    {
        return position + rotation.Rotate(Vec3{p.x * scale.x, p.y * scale.y, p.z * scale.z});
    }

    Vec3 TransformDirection(const Vec3& d) const
    {
        return rotation.Rotate(Vec3{d.x * scale.x, d.y * scale.y, d.z * scale.z});
    }

    Vec3 Forward() const { return rotation.Rotate(AxisZ()); }
    Vec3 Right() const { return rotation.Rotate(AxisX()); }
    Vec3 Up() const { return rotation.Rotate(AxisY()); }

    Transform Inverse() const
    {
        const Vec3 invScale{scale.x != 0.0f ? 1.0f / scale.x : 0.0f,
                            scale.y != 0.0f ? 1.0f / scale.y : 0.0f,
                            scale.z != 0.0f ? 1.0f / scale.z : 0.0f};
        Transform r;
        r.rotation = rotation.Conjugate();
        r.position = -(r.rotation.Rotate(Vec3{position.x * invScale.x, position.y * invScale.y,
                                              position.z * invScale.z}));
        r.scale = invScale;
        return r;
    }

    static Transform Lerp(const Transform& a, const Transform& b, f32 t)
    {
        Transform r;
        r.position = PRS::Lerp(a.position, b.position, t);
        r.rotation = Quat::Slerp(a.rotation, b.rotation, t);
        r.scale = PRS::Lerp(a.scale, b.scale, t);
        return r;
    }

    static Transform Blend(const Transform& a, const Transform& b, f32 weight)
    {
        Transform r;
        r.position = PRS::Lerp(a.position, b.position, weight);
        r.rotation = Quat::Slerp(a.rotation, b.rotation, weight);
        r.scale = PRS::Lerp(a.scale, b.scale, weight);
        return r;
    }
};
} // namespace PRS

#pragma once
#include "PRS/Animation/Pose.h"

namespace PRS::Animation
{
// Vertex format expected by the skinning shader: up to four bone influences.
struct SkinnedVertex
{
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{};
    FixedArray<u32, 4> bones{{0, 0, 0, 0}};
    Vec4 weights{1.0f, 0.0f, 0.0f, 0.0f};
};

inline Vec3 SkinPosition(const SkinnedVertex& vertex, ConstSpan<Mat4> palette)
{
    Vec3 result{};
    f32 totalWeight = 0.0f;
    const f32 weights[4] = {vertex.weights.x, vertex.weights.y, vertex.weights.z, vertex.weights.w};
    for (size_t i = 0; i < 4; ++i)
    {
        const f32 weight = weights[i];
        if (weight <= 0.0f) continue;
        const u32 bone = vertex.bones[i];
        if (bone >= palette.size()) continue;
        result += palette[bone].TransformPoint(vertex.position) * weight;
        totalWeight += weight;
    }
    return totalWeight > Epsilon ? result / totalWeight : vertex.position;
}

inline Vec3 SkinNormal(const SkinnedVertex& vertex, ConstSpan<Mat4> palette)
{
    Vec3 result{};
    const f32 weights[4] = {vertex.weights.x, vertex.weights.y, vertex.weights.z, vertex.weights.w};
    for (size_t i = 0; i < 4; ++i)
    {
        const f32 weight = weights[i];
        if (weight <= 0.0f) continue;
        const u32 bone = vertex.bones[i];
        if (bone >= palette.size()) continue;
        result += palette[bone].TransformDirection(vertex.normal) * weight;
    }
    return Normalize(result);
}
} // namespace PRS::Animation

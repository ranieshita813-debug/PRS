#pragma once
#include "PRS/Animation/Skeleton.h"

namespace PRS::Animation
{
// A pose is one local transform per bone. Local (rather than world) space keeps
// blending cheap and lets a single pass compose the world transforms.
struct Pose
{
    Array<Transform> local;

    size_t Count() const { return local.size(); }
    bool Empty() const { return local.empty(); }

    void Resize(size_t count, const Transform& fill = Transform{})
    {
        local.resize(count, fill);
    }

    Transform& operator[](size_t index) { return local[index]; }
    const Transform& operator[](size_t index) const { return local[index]; }

    static Pose BindPose(const Skeleton& skeleton)
    {
        Pose pose;
        pose.local.reserve(skeleton.BoneCount());
        for (const Bone& bone : skeleton.Bones()) pose.local.push_back(bone.bindPose);
        return pose;
    }

    // Blends two poses. Mismatched sizes produce the shorter of the two.
    static Pose Blend(const Pose& a, const Pose& b, f32 weight)
    {
        Pose result;
        const size_t count = a.local.size() < b.local.size() ? a.local.size() : b.local.size();
        result.local.resize(count);
        for (size_t i = 0; i < count; ++i)
            result.local[i] = Transform::Blend(a.local[i], b.local[i], weight);
        return result;
    }

    // Composes local transforms into world matrices (root first).
    Array<Mat4> ComputeWorld(const Skeleton& skeleton) const
    {
        Array<Mat4> world;
        const size_t count = skeleton.BoneCount();
        world.resize(count, Mat4::Identity());
        for (size_t i = 0; i < count; ++i)
        {
            const Transform& localTransform =
                i < local.size() ? local[i] : skeleton.GetBone(static_cast<i32>(i)).bindPose;
            const Mat4 localMatrix = localTransform.Matrix();
            const i32 parent = skeleton.GetBone(static_cast<i32>(i)).parent;
            world[i] = (parent >= 0 && static_cast<size_t>(parent) < count)
                           ? world[static_cast<size_t>(parent)] * localMatrix
                           : localMatrix;
        }
        return world;
    }

    // Per-bone palette for GPU skinning: world * inverseBind.
    Array<Mat4> SkinningMatrices(const Skeleton& skeleton) const
    {
        const Array<Mat4> world = ComputeWorld(skeleton);
        Array<Mat4> palette;
        palette.reserve(world.size());
        for (size_t i = 0; i < world.size(); ++i)
            palette.push_back(world[i] * skeleton.GetBone(static_cast<i32>(i)).inverseBindMatrix);
        return palette;
    }
};
} // namespace PRS::Animation

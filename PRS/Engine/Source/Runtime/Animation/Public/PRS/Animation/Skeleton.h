#pragma once
#include "PRS/Core/Assert.h"
#include "PRS/Core/Containers/Containers.h"
#include "PRS/Core/Math/Transform.h"
#include <string>

namespace PRS::Animation
{
struct Bone
{
    std::string name;
    i32 parent = -1;
    Transform bindPose;
    Mat4 bindWorld = Mat4::Identity();
    Mat4 inverseBindMatrix = Mat4::Identity();
};

// A bone hierarchy with bind-pose data. Bones must be added parent-first.
class Skeleton
{
public:
    i32 AddBone(std::string name, i32 parent = -1, const Transform& bindPose = Transform{})
    {
        if (parent >= static_cast<i32>(m_bones.size()))
        {
            PRS_ASSERT(false, "Bone parent must be added before its child");
            return -1;
        }
        Bone bone;
        bone.name = std::move(name);
        bone.parent = parent;
        bone.bindPose = bindPose;
        m_bones.push_back(std::move(bone));
        const i32 index = static_cast<i32>(m_bones.size()) - 1;
        ComputeBindMatrices();
        return index;
    }

    void SetBindPose(i32 index, const Transform& transform)
    {
        if (!IsValidIndex(index)) return;
        m_bones[static_cast<size_t>(index)].bindPose = transform;
        ComputeBindMatrices();
    }

    size_t BoneCount() const { return m_bones.size(); }
    bool Empty() const { return m_bones.empty(); }
    bool IsValidIndex(i32 index) const
    {
        return index >= 0 && static_cast<size_t>(index) < m_bones.size();
    }

    const Bone& GetBone(i32 index) const { return m_bones[static_cast<size_t>(index)]; }
    Bone& GetBone(i32 index) { return m_bones[static_cast<size_t>(index)]; }

    const Array<Bone>& Bones() const { return m_bones; }

    i32 FindBone(std::string_view name) const
    {
        for (size_t i = 0; i < m_bones.size(); ++i)
            if (m_bones[i].name == name) return static_cast<i32>(i);
        return -1;
    }

    // Fills bindWorld/inverseBindMatrix for the current bind pose.
    void ComputeBindMatrices()
    {
        Array<bool> resolved(m_bones.size(), false);
        for (size_t i = 0; i < m_bones.size(); ++i)
            ResolveBindWorld(static_cast<i32>(i), resolved, 0);
        for (Bone& bone : m_bones) bone.inverseBindMatrix = bone.bindWorld.Inverted();
    }

private:
    Mat4 ResolveBindWorld(i32 index, Array<bool>& resolved, u32 depth)
    {
        if (!IsValidIndex(index)) return Mat4::Identity();
        Bone& bone = m_bones[static_cast<size_t>(index)];
        if (resolved[static_cast<size_t>(index)]) return bone.bindWorld;
        if (depth > m_bones.size()) return Mat4::Identity(); // broken hierarchy guard

        Mat4 world = bone.bindPose.Matrix();
        if (bone.parent >= 0 && IsValidIndex(bone.parent))
            world = ResolveBindWorld(bone.parent, resolved, depth + 1) * world;

        bone.bindWorld = world;
        resolved[static_cast<size_t>(index)] = true;
        return world;
    }

    Array<Bone> m_bones;
};
} // namespace PRS::Animation

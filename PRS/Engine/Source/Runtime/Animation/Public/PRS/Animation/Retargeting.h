#pragma once
#include "PRS/Animation/Pose.h"

namespace PRS::Animation
{
// Copies a pose from one skeleton to another by matching bone names. Bones with
// no match keep the target's bind pose. This handles same-proportion rigs; the
// scaled retargeting (bind-pose offsets, IK chains) belongs to the animation
// editor once an asset format exists.
inline Pose Retarget(const Skeleton& source, const Pose& sourcePose, const Skeleton& target)
{
    Pose result = Pose::BindPose(target);
    for (size_t i = 0; i < target.BoneCount(); ++i)
    {
        const i32 sourceIndex = source.FindBone(target.GetBone(static_cast<i32>(i)).name);
        if (sourceIndex < 0) continue;
        const size_t sourceSlot = static_cast<size_t>(sourceIndex);
        if (sourceSlot >= sourcePose.Count()) continue;
        result.local[i] = sourcePose.local[sourceSlot];
    }
    return result;
}
} // namespace PRS::Animation

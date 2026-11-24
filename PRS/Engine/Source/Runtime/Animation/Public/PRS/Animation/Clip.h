#pragma once
#include "PRS/Animation/Pose.h"
#include <string>

namespace PRS::Animation
{
struct Vec3Key
{
    f32 time = 0.0f;
    Vec3 value{};
};

struct QuatKey
{
    f32 time = 0.0f;
    Quat value{};
};

struct BoneTrack
{
    i32 bone = -1;
    Array<Vec3Key> positions;
    Array<QuatKey> rotations;
    Array<Vec3Key> scales;

    bool Empty() const { return positions.empty() && rotations.empty() && scales.empty(); }
};

namespace Detail
{
inline void InsertVec3Key(Array<Vec3Key>& keys, f32 time, const Vec3& value)
{
    Vec3Key key{time, value};
    auto it = keys.begin();
    while (it != keys.end() && it->time < time) ++it;
    if (it != keys.end() && NearlyEqual(it->time, time))
        *it = key;
    else
        keys.insert(it, key);
}

inline void InsertQuatKey(Array<QuatKey>& keys, f32 time, const Quat& value)
{
    QuatKey key{time, value.Normalized()};
    auto it = keys.begin();
    while (it != keys.end() && it->time < time) ++it;
    if (it != keys.end() && NearlyEqual(it->time, time))
        *it = key;
    else
        keys.insert(it, key);
}

inline Vec3 SampleVec3(const Array<Vec3Key>& keys, f32 time, const Vec3& fallback)
{
    if (keys.empty()) return fallback;
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;

    for (size_t i = 1; i < keys.size(); ++i)
    {
        if (time > keys[i].time) continue;
        const Vec3Key& a = keys[i - 1];
        const Vec3Key& b = keys[i];
        const f32 span = b.time - a.time;
        const f32 t = span > Epsilon ? (time - a.time) / span : 0.0f;
        return PRS::Lerp(a.value, b.value, t);
    }
    return keys.back().value;
}

inline Quat SampleQuat(const Array<QuatKey>& keys, f32 time, const Quat& fallback)
{
    if (keys.empty()) return fallback;
    if (time <= keys.front().time) return keys.front().value;
    if (time >= keys.back().time) return keys.back().value;

    for (size_t i = 1; i < keys.size(); ++i)
    {
        if (time > keys[i].time) continue;
        const QuatKey& a = keys[i - 1];
        const QuatKey& b = keys[i];
        const f32 span = b.time - a.time;
        const f32 t = span > Epsilon ? (time - a.time) / span : 0.0f;
        return Quat::Slerp(a.value, b.value, t);
    }
    return keys.back().value;
}
} // namespace Detail

// A keyframed animation for a skeleton. Tracks are sparse: a bone with no track
// keeps whatever value the output pose already had (normally the bind pose).
class Clip
{
public:
    std::string name = "Clip";
    f32 duration = 0.0f;
    bool looping = true;
    Array<BoneTrack> tracks;

    BoneTrack& AddTrack(i32 bone)
    {
        for (BoneTrack& track : tracks)
            if (track.bone == bone) return track;
        tracks.push_back(BoneTrack{bone, {}, {}, {}});
        return tracks.back();
    }

    void AddPosition(i32 bone, f32 time, const Vec3& value)
    {
        Detail::InsertVec3Key(AddTrack(bone).positions, time, value);
        duration = Max(duration, time);
    }

    void AddRotation(i32 bone, f32 time, const Quat& value)
    {
        Detail::InsertQuatKey(AddTrack(bone).rotations, time, value);
        duration = Max(duration, time);
    }

    void AddScale(i32 bone, f32 time, const Vec3& value)
    {
        Detail::InsertVec3Key(AddTrack(bone).scales, time, value);
        duration = Max(duration, time);
    }

    bool Empty() const
    {
        if (tracks.empty()) return true;
        for (const BoneTrack& track : tracks)
            if (!track.Empty()) return false;
        return true;
    }

    // Samples the clip at `time` and writes the result into `pose`. Missing
    // channels leave the pose value untouched so bind-pose bones stay valid.
    void Sample(f32 time, Pose& pose) const
    {
        f32 local = time;
        if (looping && duration > Epsilon)
        {
            local = std::fmod(time, duration);
            if (local < 0.0f) local += duration;
        }
        else
        {
            local = Clamp(time, 0.0f, duration);
        }

        for (const BoneTrack& track : tracks)
        {
            if (track.bone < 0) continue;
            const size_t index = static_cast<size_t>(track.bone);
            if (index >= pose.local.size()) continue;

            Transform& transform = pose.local[index];
            transform.position = Detail::SampleVec3(track.positions, local, transform.position);
            transform.rotation = Detail::SampleQuat(track.rotations, local, transform.rotation);
            transform.scale = Detail::SampleVec3(track.scales, local, transform.scale);
        }
    }
};
} // namespace PRS::Animation

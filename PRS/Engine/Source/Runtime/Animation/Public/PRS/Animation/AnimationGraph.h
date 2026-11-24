#pragma once
#include "PRS/Animation/Clip.h"
#include <string>

namespace PRS::Animation
{
// A small state machine over clips: states fade into each other over a fixed
// time. Enough for locomotion/idle/action blends; layered masks and additive
// nodes come with the animation editor.
class AnimationGraph
{
public:
    struct State
    {
        std::string name;
        const Clip* clip = nullptr;
        f32 speed = 1.0f;
        bool looping = true;
    };

    i32 AddState(std::string name, const Clip* clip, f32 speed = 1.0f, bool looping = true)
    {
        State state;
        state.name = std::move(name);
        state.clip = clip;
        state.speed = speed;
        state.looping = looping;
        m_states.push_back(std::move(state));
        if (m_current < 0) m_current = 0;
        return static_cast<i32>(m_states.size()) - 1;
    }

    i32 FindState(std::string_view name) const
    {
        for (size_t i = 0; i < m_states.size(); ++i)
            if (m_states[i].name == name) return static_cast<i32>(i);
        return -1;
    }

    bool Play(std::string_view name, f32 fadeSeconds = 0.2f)
    {
        const i32 index = FindState(name);
        return index >= 0 && Play(index, fadeSeconds);
    }

    bool Play(i32 index, f32 fadeSeconds = 0.2f)
    {
        if (index < 0 || static_cast<size_t>(index) >= m_states.size()) return false;
        if (index == m_current) return true;

        m_previous = m_current;
        m_previousTime = m_time;
        m_current = index;
        m_time = 0.0f;
        m_fadeDuration = Max(fadeSeconds, 0.0f);
        m_fadeElapsed = 0.0f;
        return true;
    }

    bool IsPlaying() const { return m_current >= 0 && m_states[static_cast<size_t>(m_current)].clip; }

    std::string_view CurrentState() const
    {
        return m_current >= 0 ? std::string_view(m_states[static_cast<size_t>(m_current)].name)
                              : std::string_view{};
    }

    bool IsFading() const { return m_fadeElapsed < m_fadeDuration && m_previous >= 0; }

    void Stop()
    {
        m_current = -1;
        m_previous = -1;
        m_time = 0.0f;
        m_fadeElapsed = m_fadeDuration = 0.0f;
    }

    // Evaluates the graph into `pose` and advances time.
    void Update(const Skeleton& skeleton, f32 deltaTime, Pose& pose)
    {
        if (pose.Count() != skeleton.BoneCount()) pose = Pose::BindPose(skeleton);
        if (m_current < 0) return;

        const State& current = m_states[static_cast<size_t>(m_current)];
        m_time = Advance(m_time, current, deltaTime);

        if (current.clip != nullptr) current.clip->Sample(m_time, pose);

        if (m_previous >= 0 && m_fadeDuration > Epsilon)
        {
            m_fadeElapsed += deltaTime;
            const f32 alpha = Clamp(m_fadeElapsed / m_fadeDuration, 0.0f, 1.0f);

            const State& previous = m_states[static_cast<size_t>(m_previous)];
            m_previousTime = Advance(m_previousTime, previous, deltaTime);

            if (previous.clip != nullptr)
            {
                Pose previousPose = Pose::BindPose(skeleton);
                previous.clip->Sample(m_previousTime, previousPose);
                pose = Pose::Blend(previousPose, pose, alpha);
            }
            if (alpha >= 1.0f) m_previous = -1;
        }
    }

private:
    static f32 Advance(f32 time, const State& state, f32 deltaTime)
    {
        const f32 clipDuration = state.clip != nullptr ? state.clip->duration : 0.0f;
        f32 next = time + deltaTime * state.speed;
        if (state.looping && clipDuration > Epsilon)
        {
            next = std::fmod(next, clipDuration);
            if (next < 0.0f) next += clipDuration;
        }
        return next;
    }

    Array<State> m_states;
    i32 m_current = -1;
    i32 m_previous = -1;
    f32 m_time = 0.0f;
    f32 m_previousTime = 0.0f;
    f32 m_fadeElapsed = 0.0f;
    f32 m_fadeDuration = 0.0f;
};
} // namespace PRS::Animation

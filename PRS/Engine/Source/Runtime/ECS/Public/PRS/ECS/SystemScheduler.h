#pragma once
#include "PRS/ECS/System.h"
#include <algorithm>
#include <functional>
#include <string>

namespace PRS::ECS
{
// Runs systems by stage, in explicit order. Keep this single threaded until the
// job system can express per-system read/write access; systems that already
// contain parallel work should use JobSystem internally.
class SystemScheduler
{
public:
    using Fn = std::function<void(SystemContext&)>;

    void Add(SystemStage stage, std::string name, Fn fn, i32 order = 0)
    {
        const size_t slot = static_cast<size_t>(stage);
        PRS_VERIFY(slot < static_cast<size_t>(SystemStage::Count), "Invalid system stage");
        m_stages[slot].push_back({std::move(name), order, std::move(fn)});
        m_sorted[slot] = false;
    }

    void AddSystem(SystemStage stage, Scope<System> system, i32 order = 0)
    {
        System* raw = system.get();
        m_systems.push_back(std::move(system));
        Add(stage, raw->Name(), [raw](SystemContext& context) { raw->OnUpdate(context); }, order);
    }

    void Run(SystemStage stage, World& world, f32 deltaTime)
    {
        const size_t slot = static_cast<size_t>(stage);
        if (slot >= static_cast<size_t>(SystemStage::Count)) return;

        auto& entries = m_stages[slot];
        if (!m_sorted[slot])
        {
            std::stable_sort(entries.begin(), entries.end(),
                             [](const Entry& a, const Entry& b) { return a.Order < b.Order; });
            m_sorted[slot] = true;
        }

        SystemContext context{world, deltaTime, m_totalTime, m_frame};
        for (auto& entry : entries)
            if (entry.Callback) entry.Callback(context);
    }

    // Runs every stage, advancing the frame counter and total time.
    void RunAll(World& world, f32 deltaTime)
    {
        m_totalTime += deltaTime;
        for (u8 stage = 0; stage < static_cast<u8>(SystemStage::Count); ++stage)
            Run(static_cast<SystemStage>(stage), world, deltaTime);
        ++m_frame;
    }

    size_t Count(SystemStage stage) const
    {
        const size_t slot = static_cast<size_t>(stage);
        return slot < static_cast<size_t>(SystemStage::Count) ? m_stages[slot].size() : 0;
    }

    size_t TotalSystems() const
    {
        size_t total = 0;
        for (const auto& stage : m_stages) total += stage.size();
        return total;
    }

    u64 Frame() const { return m_frame; }
    f32 TotalTime() const { return m_totalTime; }

    void Clear()
    {
        for (auto& stage : m_stages) stage.clear();
        m_systems.clear();
        m_totalTime = 0.0f;
        m_frame = 0;
    }

private:
    struct         Entry
    {
        std::string Name;
        i32 Order = 0;
        Fn Callback;
    };

    std::array<Array<Entry>, static_cast<size_t>(SystemStage::Count)> m_stages{};
    std::array<bool, static_cast<size_t>(SystemStage::Count)> m_sorted{};
    Array<Scope<System>> m_systems;
    f32 m_totalTime = 0.0f;
    u64 m_frame = 0;
};
} // namespace PRS::ECS

#pragma once
#include "PRS/ECS/World.h"
#include <functional>
#include <tuple>

namespace PRS::ECS
{
// Deferred structural changes. Systems never add or remove components in the
// middle of a query; they queue the change here and the frame flushes it once,
// between systems. Entity creation is immediate so the new handle can be
// referenced by later commands in the same batch.
class Commands
{
public:
    explicit Commands(World& world) : m_world(world) {}

    Entity Create() { return m_world.Create(); }

    template <typename T, typename... Args> void Add(Entity e, Args&&... args)
    {
        auto stored = std::make_tuple(std::forward<Args>(args)...);
        m_queue.push_back([e, stored = std::move(stored)](World& world) mutable {
            std::apply(
                [&](auto&&... unpacked) {
                    world.Emplace<T>(e, std::forward<decltype(unpacked)>(unpacked)...);
                },
                stored);
        });
    }

    template <typename T> void Remove(Entity e)
    {
        m_queue.push_back([e](World& world) { world.Remove<T>(e); });
    }

    void Destroy(Entity e) { m_queue.push_back([e](World& world) { world.Destroy(e); }); }

    void Flush()
    {
        Array<std::function<void(World&)>> queue;
        queue.swap(m_queue);
        for (auto& command : queue)
            if (command) command(m_world);
    }

    size_t Pending() const { return m_queue.size(); }

    void Discard() { m_queue.clear(); }

private:
    World& m_world;
    Array<std::function<void(World&)>> m_queue;
};
} // namespace PRS::ECS

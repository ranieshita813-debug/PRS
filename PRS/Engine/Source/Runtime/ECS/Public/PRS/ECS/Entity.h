#pragma once
#include "PRS/Core/Types.h"
#include <functional>

namespace PRS::ECS
{
inline constexpr u32 InvalidEntityIndex = 0xFFFFFFFFu;

// A entity handle is an index + generation pair. The generation makes stale
// handles detectable: destroying an entity bumps its generation, so an old copy
// of the handle no longer compares equal to the live entity.
struct Entity
{
    u32 index = InvalidEntityIndex;
    u32 generation = 0;

    constexpr bool IsValid() const { return index != InvalidEntityIndex; }
    constexpr explicit operator bool() const { return IsValid(); }

    friend constexpr bool operator==(Entity a, Entity b)
    {
        return a.index == b.index && a.generation == b.generation;
    }
    friend constexpr bool operator!=(Entity a, Entity b) { return !(a == b); }
};

struct EntityHash
{
    size_t operator()(Entity e) const noexcept
    {
        return (static_cast<size_t>(e.index) << 32) ^ static_cast<size_t>(e.generation);
    }
};
} // namespace PRS::ECS

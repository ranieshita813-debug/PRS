#pragma once
#include "PRS/Core/Types.h"
#include "PRS/ECS/Entity.h"
#include <atomic>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace PRS::ECS
{
// Maximum number of distinct component types a world can track (one bit each).
inline constexpr u32 MaxComponentTypes = 64;

// Archetype is the set of components an entity owns, as a bit mask over the
// component type ids. It is cheap to copy and to compare, which makes it a good
// key for grouping entities (render batches, serialisation filters, queries).
struct Archetype
{
    u64 mask = 0;

    constexpr bool Has(u32 typeId) const
    {
        return typeId < MaxComponentTypes && (mask & (u64{1} << typeId)) != 0;
    }
    constexpr void Set(u32 typeId)
    {
        if (typeId < MaxComponentTypes) mask |= (u64{1} << typeId);
    }
    constexpr void Unset(u32 typeId)
    {
        if (typeId < MaxComponentTypes) mask &= ~(u64{1} << typeId);
    }
    constexpr bool Empty() const { return mask == 0; }
    constexpr u32 Count() const
    {
        u32 n = 0;
        for (u64 bits = mask; bits != 0; bits &= bits - 1) ++n;
        return n;
    }
    constexpr bool Contains(const Archetype& other) const { return (mask & other.mask) == other.mask; }

    friend constexpr bool operator==(const Archetype& a, const Archetype& b) { return a.mask == b.mask; }
    friend constexpr bool operator!=(const Archetype& a, const Archetype& b) { return a.mask != b.mask; }
};

namespace Detail
{
inline u32 AllocateComponentTypeId()
{
    static std::atomic<u32> next{0};
    return next.fetch_add(1, std::memory_order_relaxed);
}

// One id per component type, assigned on first use and stable for the process.
template <typename T> u32 ComponentTypeId()
{
    static const u32 id = AllocateComponentTypeId();
    return id;
}

// Type-erased view over a typed component pool.
class IComponentPool
{
public:
    virtual ~IComponentPool() = default;
    virtual bool Has(Entity e) const = 0;
    virtual void Remove(Entity e) = 0;
    virtual Entity EntityAt(size_t denseIndex) const = 0;
    virtual size_t Size() const = 0;
    virtual u32 TypeId() const = 0;
    virtual const char* TypeName() const = 0;
};
} // namespace Detail

// Dense component storage: components live in a packed vector and a sparse
// lookup maps entity index to dense slot. Removal swaps the last element into
// the hole, so iteration is cache friendly and never leaves gaps.
template <typename T> class ComponentPool final : public Detail::IComponentPool
{
public:
    std::vector<T> data;
    std::vector<Entity> entities;
    std::unordered_map<u32, u32> lookup;

    template <typename... Args> T& Emplace(Entity e, Args&&... args)
    {
        const auto it = lookup.find(e.index);
        if (it != lookup.end())
        {
            data[it->second] = T(std::forward<Args>(args)...);
            return data[it->second];
        }
        lookup.emplace(e.index, static_cast<u32>(data.size()));
        entities.push_back(e);
        data.emplace_back(std::forward<Args>(args)...);
        return data.back();
    }

    bool Has(Entity e) const override { return lookup.find(e.index) != lookup.end(); }

    void Remove(Entity e) override
    {
        const auto it = lookup.find(e.index);
        if (it == lookup.end()) return;

        const u32 slot = it->second;
        const u32 last = static_cast<u32>(data.size()) - 1;
        if (slot != last)
        {
            data[slot] = std::move(data[last]);
            entities[slot] = entities[last];
            lookup[entities[slot].index] = slot;
        }
        data.pop_back();
        entities.pop_back();
        lookup.erase(it);
    }

    T* TryGet(Entity e)
    {
        const auto it = lookup.find(e.index);
        return it == lookup.end() ? nullptr : &data[it->second];
    }

    const T* TryGet(Entity e) const
    {
        const auto it = lookup.find(e.index);
        return it == lookup.end() ? nullptr : &data[it->second];
    }

    T& Get(Entity e) { return data[lookup.at(e.index)]; }
    const T& Get(Entity e) const { return data[lookup.at(e.index)]; }

    Entity EntityAt(size_t denseIndex) const override { return entities[denseIndex]; }
    size_t Size() const override { return data.size(); }
    u32 TypeId() const override { return Detail::ComponentTypeId<T>(); }
    const char* TypeName() const override { return typeid(T).name(); }

    void Clear()
    {
        data.clear();
        entities.clear();
        lookup.clear();
    }
};
} // namespace PRS::ECS

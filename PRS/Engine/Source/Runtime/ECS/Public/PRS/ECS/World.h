#pragma once
#include "PRS/Core/Assert.h"
#include "PRS/Core/Containers/Containers.h"
#include "PRS/ECS/Archetype.h"
#include "PRS/ECS/Entity.h"
#include "PRS/ECS/Query.h"

namespace PRS::ECS
{
// The entity/component database. Entities are indices with a generation; each
// component type has its own dense pool. Lookup by component is O(1), iteration
// is contiguous, and destroying an entity clears it from every pool.
class World
{
public:
    World() = default;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;
    ~World() = default;

    Entity Create()
    {
        if (!m_free.empty())
        {
            const u32 index = m_free.back();
            m_free.pop_back();
            m_alive[index] = true;
            return Entity{index, m_generations[index]};
        }
        const u32 index = static_cast<u32>(m_generations.size());
        m_generations.push_back(0);
        m_alive.push_back(true);
        return Entity{index, 0};
    }

    bool IsAlive(Entity e) const
    {
        return e.index < m_generations.size() && m_alive[e.index] &&
               m_generations[e.index] == e.generation;
    }

    void Destroy(Entity e)
    {
        if (!IsAlive(e)) return;
        for (Detail::IComponentPool* pool : m_poolList) pool->Remove(e);
        m_alive[e.index] = false;
        ++m_generations[e.index];
        m_free.push_back(e.index);
    }

    template <typename T, typename... Args> T& Emplace(Entity e, Args&&... args)
    {
        PRS_VERIFY(IsAlive(e), "Emplace on a dead entity");
        return GetOrCreatePool<T>().Emplace(e, std::forward<Args>(args)...);
    }

    template <typename T> bool Has(Entity e) const
    {
        if (!IsAlive(e)) return false;
        const ComponentPool<T>* pool = FindPool<T>();
        return pool != nullptr && pool->Has(e);
    }

    template <typename T> T* TryGet(Entity e)
    {
        if (!IsAlive(e)) return nullptr;
        ComponentPool<T>* pool = FindPool<T>();
        return pool != nullptr ? pool->TryGet(e) : nullptr;
    }

    template <typename T> const T* TryGet(Entity e) const
    {
        if (!IsAlive(e)) return nullptr;
        const ComponentPool<T>* pool = FindPool<T>();
        return pool != nullptr ? pool->TryGet(e) : nullptr;
    }

    template <typename T> T& Get(Entity e)
    {
        T* value = TryGet<T>(e);
        PRS_VERIFY(value != nullptr, "Get on a component the entity does not have");
        return *value;
    }

    template <typename T> const T& Get(Entity e) const
    {
        const T* value = TryGet<T>(e);
        PRS_VERIFY(value != nullptr, "Get on a component the entity does not have");
        return *value;
    }

    template <typename T> bool Remove(Entity e)
    {
        ComponentPool<T>* pool = FindPool<T>();
        if (pool == nullptr || !pool->Has(e)) return false;
        pool->Remove(e);
        return true;
    }

    template <typename... Cs> View<Cs...> Query()
    {
        return View<Cs...>(&GetOrCreatePool<Cs>()...);
    }

    template <typename... Cs> ConstView<Cs...> Query() const
    {
        return ConstView<Cs...>(FindPool<Cs>()...);
    }

    // Convenience: iterate matching entities without naming the view.
    template <typename... Cs, typename F> void ForEach(F&& fn)
    {
        Query<Cs...>().ForEach(std::forward<F>(fn));
    }

    template <typename... Cs> size_t Count()
    {
        return Query<Cs...>().Count();
    }

    Archetype SignatureOf(Entity e) const
    {
        Archetype signature;
        if (!IsAlive(e)) return signature;
        for (Detail::IComponentPool* pool : m_poolList)
            if (pool->Has(e)) signature.Set(pool->TypeId());
        return signature;
    }

    u32 AliveCount() const
    {
        u32 n = 0;
        for (bool alive : m_alive) n += alive ? 1u : 0u;
        return n;
    }

    u32 Capacity() const { return static_cast<u32>(m_generations.size()); }
    u32 ComponentTypeCount() const { return static_cast<u32>(m_poolList.size()); }

    // Drops every entity and every component pool; the world is reusable but
    // handles handed out before the clear become invalid.
    void Clear()
    {
        m_generations.clear();
        m_alive.clear();
        m_free.clear();
        m_pools.clear();
        m_poolList.clear();
    }

    template <typename T> ComponentPool<T>* FindPool()
    {
        const auto it = m_pools.find(Detail::ComponentTypeId<T>());
        return it == m_pools.end() ? nullptr : static_cast<ComponentPool<T>*>(it->second.get());
    }

    template <typename T> const ComponentPool<T>* FindPool() const
    {
        const auto it = m_pools.find(Detail::ComponentTypeId<T>());
        return it == m_pools.end() ? nullptr : static_cast<const ComponentPool<T>*>(it->second.get());
    }

    template <typename T> ComponentPool<T>& GetOrCreatePool()
    {
        ComponentPool<T>* existing = FindPool<T>();
        if (existing != nullptr) return *existing;

        auto pool = MakeScope<ComponentPool<T>>();
        ComponentPool<T>* raw = pool.get();
        m_pools.emplace(Detail::ComponentTypeId<T>(), std::move(pool));
        m_poolList.push_back(raw);
        return *raw;
    }

private:
    HashMap<u32, Scope<Detail::IComponentPool>> m_pools;
    Array<Detail::IComponentPool*> m_poolList;
    Array<u32> m_generations;
    Array<bool> m_alive;
    Array<u32> m_free;
};
} // namespace PRS::ECS

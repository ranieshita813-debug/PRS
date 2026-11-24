#pragma once
#include "PRS/ECS/Archetype.h"

namespace PRS::ECS
{
namespace Detail
{
template <typename... Ts> struct TypesAreUnique : std::true_type {};
template <typename T, typename... Ts>
struct TypesAreUnique<T, Ts...>
    : std::bool_constant<(!std::is_same_v<T, Ts> && ...) && TypesAreUnique<Ts...>::value>
{
};
} // namespace Detail

// A query over one or more component types. Iteration is driven by the first
// component pool (so list the rarest component first); the remaining components
// are checked per entity.
//
// Structural changes (adding/removing components, creating or destroying
// entities) while iterating invalidate the view. Use ECS::Commands to defer
// those changes to the end of the frame.
template <typename... Cs> class View;

template <typename C0, typename... Cs> class View<C0, Cs...>
{
    static_assert(Detail::TypesAreUnique<C0, Cs...>::value,
                  "A view cannot list the same component type twice");

public:
    View(ComponentPool<C0>* first, ComponentPool<Cs>*... rest) : m_first(first), m_rest(rest...) {}

    template <typename F> void ForEach(F&& fn) const
    {
        if (!m_first || !AllPoolsValid()) return;
        const size_t count = m_first->Size();
        for (size_t i = 0; i < count; ++i)
        {
            const Entity e = m_first->entities[i];
            if (!(std::get<ComponentPool<Cs>*>(m_rest)->Has(e) && ...)) continue;
            Invoke(i, e, fn, std::index_sequence_for<Cs...>{});
        }
    }

    size_t Count() const
    {
        if (!m_first || !AllPoolsValid()) return 0;
        size_t n = 0;
        for (size_t i = 0; i < m_first->Size(); ++i)
        {
            const Entity e = m_first->entities[i];
            (void)e;
            if ((std::get<ComponentPool<Cs>*>(m_rest)->Has(e) && ...)) ++n;
        }
        return n;
    }

    bool Empty() const { return Count() == 0; }

private:
    bool AllPoolsValid() const
    {
        return m_first != nullptr && ((std::get<ComponentPool<Cs>*>(m_rest) != nullptr) && ...);
    }

    template <typename F, size_t... I>
    void Invoke(size_t denseIndex, Entity e, F& fn, std::index_sequence<I...>) const
    {
        fn(e, m_first->data[denseIndex], std::get<I>(m_rest)->Get(e)...);
    }

    ComponentPool<C0>* m_first = nullptr;
    std::tuple<ComponentPool<Cs>*...> m_rest;
};

// Read-only view returned by a const World.
template <typename... Cs> class ConstView;

template <typename C0, typename... Cs> class ConstView<C0, Cs...>
{
    static_assert(Detail::TypesAreUnique<C0, Cs...>::value,
                  "A view cannot list the same component type twice");

public:
    ConstView(const ComponentPool<C0>* first, const ComponentPool<Cs>*... rest)
        : m_first(first), m_rest(rest...)
    {
    }

    template <typename F> void ForEach(F&& fn) const
    {
        if (!m_first || !AllPoolsValid()) return;
        const size_t count = m_first->Size();
        for (size_t i = 0; i < count; ++i)
        {
            const Entity e = m_first->entities[i];
            (void)e;
            if (!(std::get<const ComponentPool<Cs>*>(m_rest)->Has(e) && ...)) continue;
            Invoke(i, e, fn, std::index_sequence_for<Cs...>{});
        }
    }

    size_t Count() const
    {
        if (!m_first || !AllPoolsValid()) return 0;
        size_t n = 0;
        for (size_t i = 0; i < m_first->Size(); ++i)
        {
            const Entity e = m_first->entities[i];
            (void)e;
            if ((std::get<const ComponentPool<Cs>*>(m_rest)->Has(e) && ...)) ++n;
        }
        return n;
    }

private:
    bool AllPoolsValid() const
    {
        return m_first != nullptr &&
               ((std::get<const ComponentPool<Cs>*>(m_rest) != nullptr) && ...);
    }

    template <typename F, size_t... I>
    void Invoke(size_t denseIndex, Entity e, F& fn, std::index_sequence<I...>) const
    {
        fn(e, m_first->data[denseIndex], std::get<I>(m_rest)->Get(e)...);
    }

    const ComponentPool<C0>* m_first = nullptr;
    std::tuple<const ComponentPool<Cs>*...> m_rest;
};
} // namespace PRS::ECS

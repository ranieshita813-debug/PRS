#pragma once
#include "PRS/Core/Types.h"
#include "PRS/Core/Assert.h"
#include <cstdlib>
#include <new>

namespace PRS
{
// Fixed-size block pool with O(1) allocate/free via an intrusive free list.
template <typename T> class PoolAllocator
{
    union Slot
    {
        Slot* next;
        alignas(T) u8 storage[sizeof(T)];
    };

public:
    explicit PoolAllocator(size_t count) : m_count(count), m_slots(static_cast<Slot*>(std::malloc(sizeof(Slot) * count)))
    {
        PRS_VERIFY(m_slots != nullptr, "PoolAllocator out of memory");
        for (size_t i = 0; i < count; ++i) m_slots[i].next = (i + 1 < count) ? &m_slots[i + 1] : nullptr;
        m_free = m_slots;
    }
    ~PoolAllocator() { std::free(m_slots); }
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    template <typename... Args> T* Create(Args&&... args)
    {
        if (!m_free) return nullptr;
        Slot* s = m_free;
        m_free = s->next;
        ++m_used;
        return new (s->storage) T(static_cast<Args&&>(args)...);
    }

    void Destroy(T* obj)
    {
        if (!obj) return;
        obj->~T();
        Slot* s = reinterpret_cast<Slot*>(obj);
        s->next = m_free;
        m_free = s;
        --m_used;
    }

    size_t Used() const { return m_used; }
    size_t Capacity() const { return m_count; }

private:
    size_t m_count;
    Slot* m_slots;
    Slot* m_free = nullptr;
    size_t m_used = 0;
};
} // namespace PRS

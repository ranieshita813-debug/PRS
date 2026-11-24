#pragma once
#include "PRS/Core/Types.h"
#include "PRS/Core/Assert.h"
#include <cstdlib>
#include <new>

namespace PRS
{
// Bump allocator. Allocate freely during a frame, Reset() once at frame end. No per-object free.
class LinearAllocator
{
public:
    explicit LinearAllocator(size_t capacityBytes)
        : m_capacity(capacityBytes), m_buffer(static_cast<u8*>(std::malloc(capacityBytes)))
    {
        PRS_VERIFY(m_buffer != nullptr, "LinearAllocator out of memory");
    }
    ~LinearAllocator() { std::free(m_buffer); }
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

    void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t))
    {
        size_t aligned = (m_offset + alignment - 1) & ~(alignment - 1);
        if (aligned + size > m_capacity) return nullptr;
        m_offset = aligned + size;
        return m_buffer + aligned;
    }

    template <typename T, typename... Args> T* Create(Args&&... args)
    {
        void* mem = Allocate(sizeof(T), alignof(T));
        return mem ? new (mem) T(static_cast<Args&&>(args)...) : nullptr; // objects must be trivially destructible
    }

    void Reset() { m_offset = 0; }
    size_t Used() const { return m_offset; }
    size_t Capacity() const { return m_capacity; }

private:
    size_t m_capacity;
    size_t m_offset = 0;
    u8* m_buffer;
};
} // namespace PRS

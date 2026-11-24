#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
// GPU memory allocation. Host-visible buffers can be written directly; others
// are filled through Write(), which stages and copies on the graphics queue.
class Buffer
{
public:
    virtual ~Buffer() = default;

    virtual u64 Size() const = 0;
    virtual BufferUsage Usage() const = 0;
    virtual bool IsHostVisible() const = 0;

    // Persistent mapping for host-visible buffers (uniform data). Null otherwise.
    virtual void* MappedPointer() = 0;

    // Copies data into the buffer. For device-local buffers this performs a
    // staging upload and blocks until the copy has been submitted.
    virtual void Write(const void* data, u64 size, u64 offset = 0) = 0;
};
} // namespace PRS::RHI

#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
// GPU timeline synchronization point.
class Fence
{
public:
    virtual ~Fence() = default;

    // Blocks the CPU until the fence signals. timeoutNs = ~0ull waits forever.
    virtual void Wait(u64 timeoutNs = ~0ull) = 0;
    virtual void Reset() = 0;
    virtual bool IsSignaled() const = 0;
};
} // namespace PRS::RHI

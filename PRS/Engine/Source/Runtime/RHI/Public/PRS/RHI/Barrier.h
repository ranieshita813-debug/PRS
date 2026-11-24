#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
class Buffer;
class Texture;

enum class ResourceState : u32
{
    Undefined = 0,
    ShaderRead,
    ColorTarget,
    DepthTarget,
    TransferSrc,
    TransferDst,
    Present
};

// Explicit layout/state transition for a buffer or texture. The Vulkan backend
// maps these onto image layout transitions and memory barriers.
struct Barrier
{
    ResourceState before = ResourceState::Undefined;
    ResourceState after = ResourceState::Undefined;
    Buffer* buffer = nullptr;
    Texture* texture = nullptr;
};
} // namespace PRS::RHI

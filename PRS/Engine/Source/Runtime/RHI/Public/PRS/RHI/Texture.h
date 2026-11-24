#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
// A 2D image. Sampled textures are filled with Upload(); render targets and
// depth images come from the swapchain or a Device::CreateTexture call.
class Texture
{
public:
    virtual ~Texture() = default;

    virtual Extent2D Size() const = 0;
    virtual Format GetFormat() const = 0;
    virtual TextureUsage Usage() const = 0;

    // Uploads tightly packed texel data for one mip level.
    virtual void Upload(const void* pixels, u64 sizeBytes, u32 mipLevel = 0) = 0;

    // True if this texture can be used as the color target of a render pass.
    virtual bool IsRenderTarget() const = 0;
};
} // namespace PRS::RHI

#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
class Buffer;
class Texture;
class Sampler;

// A small descriptor set for the frame: one uniform buffer slot and one combined
// image sampler slot. The forward renderer only needs this much; a general
// bindless heap arrives with the render graph.
class DescriptorHeap
{
public:
    virtual ~DescriptorHeap() = default;

    // Binding 0 of set 0 in the default pipeline layout.
    virtual void SetUniformBuffer(u32 binding, Buffer& buffer, u64 offset = 0, u64 range = 0) = 0;

    // Binding 1 of set 0: sampled texture + sampler.
    virtual void SetCombinedImageSampler(u32 binding, Texture& texture, Sampler& sampler) = 0;
};
} // namespace PRS::RHI

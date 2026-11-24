#pragma once
#include "PRS/Core/Containers/Containers.h"

namespace PRS::Assets
{
// A decoded 2D image. Every loader converts to tightly packed RGBA8 so the
// renderer has exactly one upload path, whatever the file format was.
struct Image
{
    u32 width = 0;
    u32 height = 0;
    Array<u8> pixels; // width * height * 4 bytes
    String source;

    bool IsValid() const
    {
        return width > 0 && height > 0 &&
               pixels.size() == static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    }

    u64 SizeBytes() const { return static_cast<u64>(pixels.size()); }

    // RGBA texel lookup; out-of-range coordinates return (0,0,0,0).
    const u8* Texel(u32 x, u32 y) const
    {
        if (!IsValid() || x >= width || y >= height) return nullptr;
        return pixels.data() + (static_cast<size_t>(y) * width + x) * 4u;
    }
};
} // namespace PRS::Assets

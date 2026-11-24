#pragma once
#include "PRS/Core/Containers/Containers.h"
#include "PRS/Core/Types.h"

namespace PRS::RHI
{
enum class Format : u32
{
    Undefined = 0,
    RGBA8Unorm,
    RGBA8Srgb,
    BGRA8Unorm,
    BGRA8Srgb,
    RGBA16Float,
    R32Float,
    Depth32Float,
    Depth24Stencil8
};

enum class BufferUsage : u32
{
    None = 0,
    Vertex = 1u << 0,
    Index = 1u << 1,
    Uniform = 1u << 2,
    Storage = 1u << 3,
    TransferSrc = 1u << 4,
    TransferDst = 1u << 5
};

inline constexpr BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline constexpr bool HasFlag(BufferUsage value, BufferUsage flag)
{
    return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
}

enum class TextureUsage : u32
{
    None = 0,
    Sampled = 1u << 0,
    RenderTarget = 1u << 1,
    DepthStencil = 1u << 2,
    TransferDst = 1u << 3
};

inline constexpr TextureUsage operator|(TextureUsage a, TextureUsage b)
{
    return static_cast<TextureUsage>(static_cast<u32>(a) | static_cast<u32>(b));
}
inline constexpr bool HasFlag(TextureUsage value, TextureUsage flag)
{
    return (static_cast<u32>(value) & static_cast<u32>(flag)) != 0;
}

enum class Filter : u32
{
    Nearest = 0,
    Linear
};

enum class AddressMode : u32
{
    Repeat = 0,
    ClampToEdge,
    MirroredRepeat
};

enum class CullMode : u32
{
    None = 0,
    Front,
    Back
};

enum class PrimitiveTopology : u32
{
    TriangleList = 0,
    TriangleStrip
};

enum class VertexFormat : u32
{
    Float = 0, // one f32
    Float2,
    Float3,
    Float4
};

struct Extent2D
{
    u32 width = 0;
    u32 height = 0;
};

struct VertexAttribute
{
    u32 location = 0;
    VertexFormat format = VertexFormat::Float3;
    u32 offset = 0;
};

struct VertexBinding
{
    u32 stride = 0;
    Array<VertexAttribute> attributes;
};

struct BufferDesc
{
    u64 size = 0;
    BufferUsage usage = BufferUsage::None;
    bool hostVisible = false; // map/write from the CPU (uniforms, staging data)
    const char* debugName = nullptr;
};

struct TextureDesc
{
    Extent2D size{};
    Format format = Format::RGBA8Unorm;
    TextureUsage usage = TextureUsage::Sampled;
    u32 mipLevels = 1;
    const char* debugName = nullptr;
};

struct SamplerDesc
{
    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    bool anisotropy = true;
    f32 maxAnisotropy = 8.0f;
    f32 maxLod = 8.0f;
};

inline constexpr u32 FormatBytesPerPixel(Format format)
{
    switch (format)
    {
    case Format::RGBA8Unorm:
    case Format::RGBA8Srgb:
    case Format::BGRA8Unorm:
    case Format::BGRA8Srgb:
    case Format::Depth24Stencil8: return 4;
    case Format::RGBA16Float: return 8;
    case Format::R32Float:
    case Format::Depth32Float: return 4;
    case Format::Undefined: return 0;
    }
    return 0;
}

inline constexpr bool IsDepthFormat(Format format)
{
    return format == Format::Depth32Float || format == Format::Depth24Stencil8;
}

inline constexpr bool IsSrgbFormat(Format format)
{
    return format == Format::RGBA8Srgb || format == Format::BGRA8Srgb;
}
} // namespace PRS::RHI

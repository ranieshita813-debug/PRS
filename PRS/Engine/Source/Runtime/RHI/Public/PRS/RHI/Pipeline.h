#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
struct GraphicsPipelineDesc
{
    const char* vertexShaderPath = nullptr;   // SPIR-V (.spv), entry point "main"
    const char* fragmentShaderPath = nullptr; // SPIR-V (.spv), entry point "main"

    Format colorFormat = Format::BGRA8Srgb;
    Format depthFormat = Format::Depth32Float;

    VertexBinding vertexBinding{};
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;

    CullMode cullMode = CullMode::Back;
    bool depthTest = true;
    bool depthWrite = true;
    bool alphaBlend = false;

    u32 pushConstantSize = 0; // bytes exposed to vertex + fragment stages

    const char* debugName = nullptr;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;
};
} // namespace PRS::RHI

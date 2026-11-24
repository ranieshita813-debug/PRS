#pragma once
#include "PRS/RHI/Barrier.h"
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
class Buffer;
class Pipeline;
class Texture;
class DescriptorHeap;

struct RenderPassDesc
{
    Texture* color = nullptr;
    Texture* depth = nullptr;
    bool clearColor = true;
    bool clearDepth = true;
    f32 clearColorValue[4] = {0.05f, 0.06f, 0.09f, 1.0f};
    f32 clearDepthValue = 1.0f;
};

class CommandList
{
public:
    virtual ~CommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginRenderPass(const RenderPassDesc& desc) = 0;
    virtual void EndRenderPass() = 0;

    virtual void SetPipeline(Pipeline& pipeline) = 0;
    virtual void SetDescriptorHeap(DescriptorHeap& heap) = 0;
    virtual void SetVertexBuffer(Buffer& buffer) = 0;
    virtual void SetIndexBuffer(Buffer& buffer) = 0;

    virtual void PushConstants(const void* data, u32 size, u32 offset = 0) = 0;

    virtual void Draw(u32 vertexCount, u32 firstVertex = 0) = 0;
    virtual void DrawIndexed(u32 indexCount, u32 firstIndex = 0, i32 vertexOffset = 0) = 0;

    virtual void Barrier(const Barrier& barrier) = 0;
};
} // namespace PRS::RHI

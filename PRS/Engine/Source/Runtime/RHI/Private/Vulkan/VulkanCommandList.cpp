#include "VulkanInternal.h"
#include "PRS/Core/Log.h"

#if defined(PRS_HAS_VULKAN)

namespace PRS::RHI::Vulkan
{
CommandListImpl::~CommandListImpl()
{
    if (owner != nullptr && owner->device != VK_NULL_HANDLE && commandBuffer != VK_NULL_HANDLE)
        vkFreeCommandBuffers(owner->device, owner->commandPool, 1, &commandBuffer);
}

void CommandListImpl::Begin()
{
    if (owner == nullptr || commandBuffer == VK_NULL_HANDLE) return;

    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    const VkResult result = vkBeginCommandBuffer(commandBuffer, &info);
    if (result != VK_SUCCESS) PRS_ERROR("vkBeginCommandBuffer failed: %s", ResultName(result));
    recording = true;
}

void CommandListImpl::End()
{
    if (owner == nullptr || commandBuffer == VK_NULL_HANDLE || !recording) return;
    const VkResult result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) PRS_ERROR("vkEndCommandBuffer failed: %s", ResultName(result));
    recording = false;
}

void CommandListImpl::BeginRenderPass(const RenderPassDesc& desc)
{
    if (!recording || desc.color == nullptr) return;

    auto* color = static_cast<TextureImpl*>(desc.color);
    auto* depth = desc.depth != nullptr ? static_cast<TextureImpl*>(desc.depth) : nullptr;

    VkRenderPass renderPass = color->renderPass;
    if (renderPass == VK_NULL_HANDLE)
        renderPass = owner->GetOrCreateRenderPass(color->format,
                                                  depth != nullptr ? depth->format
                                                                   : Format::Undefined);
    if (renderPass == VK_NULL_HANDLE) return;

    const Extent2D extent = color->extent;

    VkClearValue clearValues[2]{};
    clearValues[0].color.float32[0] = desc.clearColorValue[0];
    clearValues[0].color.float32[1] = desc.clearColorValue[1];
    clearValues[0].color.float32[2] = desc.clearColorValue[2];
    clearValues[0].color.float32[3] = desc.clearColorValue[3];
    clearValues[1].depthStencil.depth = desc.clearDepthValue;
    clearValues[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = renderPass;
    info.framebuffer = color->framebuffer;
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = {extent.width, extent.height};
    info.clearValueCount = depth != nullptr ? 2u : 1u;
    info.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    inRenderPass = true;
    activeRenderPass = renderPass;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(extent.width);
    viewport.height = static_cast<f32>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {extent.width, extent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void CommandListImpl::EndRenderPass()
{
    if (!recording || !inRenderPass) return;
    vkCmdEndRenderPass(commandBuffer);
    inRenderPass = false;
    activeRenderPass = VK_NULL_HANDLE;
}

void CommandListImpl::SetPipeline(Pipeline& pipeline)
{
    if (!recording) return;
    auto& vulkanPipeline = static_cast<PipelineImpl&>(pipeline);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline.pipeline);
    boundLayout = vulkanPipeline.layout;
}

void CommandListImpl::SetDescriptorHeap(DescriptorHeap& heap)
{
    if (!recording) return;
    auto& vulkanHeap = static_cast<DescriptorHeapImpl&>(heap);
    if (boundLayout == VK_NULL_HANDLE || vulkanHeap.set == VK_NULL_HANDLE) return;
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, boundLayout, 0, 1,
                            &vulkanHeap.set, 0, nullptr);
}

void CommandListImpl::SetVertexBuffer(Buffer& buffer)
{
    if (!recording) return;
    VkBuffer vertexBuffer = static_cast<BufferImpl&>(buffer).buffer;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
}

void CommandListImpl::SetIndexBuffer(Buffer& buffer)
{
    if (!recording) return;
    vkCmdBindIndexBuffer(commandBuffer, static_cast<BufferImpl&>(buffer).buffer, 0,
                         VK_INDEX_TYPE_UINT32);
}

void CommandListImpl::PushConstants(const void* data, u32 size, u32 offset)
{
    if (!recording || boundLayout == VK_NULL_HANDLE || data == nullptr || size == 0) return;
    vkCmdPushConstants(commandBuffer, boundLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset, size,
                       data);
}

void CommandListImpl::Draw(u32 vertexCount, u32 firstVertex)
{
    if (!recording) return;
    vkCmdDraw(commandBuffer, vertexCount, 1, firstVertex, 0);
}

void CommandListImpl::DrawIndexed(u32 indexCount, u32 firstIndex, i32 vertexOffset)
{
    if (!recording) return;
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, firstIndex, vertexOffset, 0);
}

void CommandListImpl::Barrier(const PRS::RHI::Barrier& barrier)
{
    if (!recording || owner == nullptr) return;

    const VkPipelineStageFlags srcStage = ToVkPipelineStage(barrier.before);
    const VkPipelineStageFlags dstStage = ToVkPipelineStage(barrier.after);

    if (barrier.texture != nullptr)
    {
        auto* texture = static_cast<TextureImpl*>(barrier.texture);
        VkImageMemoryBarrier imageBarrier{};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier.oldLayout = ToVkImageLayout(barrier.before);
        imageBarrier.newLayout = ToVkImageLayout(barrier.after);
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = texture->image;
        imageBarrier.subresourceRange.aspectMask =
            IsDepthFormat(texture->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.layerCount = 1;
        imageBarrier.srcAccessMask = ToVkAccess(barrier.before);
        imageBarrier.dstAccessMask = ToVkAccess(barrier.after);

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                             &imageBarrier);
    }
    else if (barrier.buffer != nullptr)
    {
        auto* buffer = static_cast<BufferImpl*>(barrier.buffer);
        VkBufferMemoryBarrier bufferBarrier{};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.buffer = buffer->buffer;
        bufferBarrier.offset = 0;
        bufferBarrier.size = VK_WHOLE_SIZE;
        bufferBarrier.srcAccessMask = ToVkAccess(barrier.before);
        bufferBarrier.dstAccessMask = ToVkAccess(barrier.after);

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 1, &bufferBarrier, 0,
                             nullptr);
    }
}
} // namespace PRS::RHI::Vulkan

#endif // PRS_HAS_VULKAN

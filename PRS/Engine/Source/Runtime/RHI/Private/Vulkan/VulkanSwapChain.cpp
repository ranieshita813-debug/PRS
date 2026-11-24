#include "VulkanInternal.h"
#include "PRS/Core/Log.h"
#include <algorithm>
#include <vector>

#if defined(PRS_HAS_VULKAN)

namespace PRS::RHI::Vulkan
{
namespace
{
VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // The forward shader writes gamma-corrected values itself, so an UNORM target
    // avoids a second sRGB conversion.
    for (const VkSurfaceFormatKHR& format : formats)
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    for (const VkSurfaceFormatKHR& format : formats)
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    return formats.front();
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes,
                                   PresentMode requested)
{
    const auto has = [&](VkPresentModeKHR mode) {
        return std::find(modes.begin(), modes.end(), mode) != modes.end();
    };

    if (requested == PresentMode::Immediate && has(VK_PRESENT_MODE_IMMEDIATE_KHR))
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (requested == PresentMode::Mailbox && has(VK_PRESENT_MODE_MAILBOX_KHR))
        return VK_PRESENT_MODE_MAILBOX_KHR;
    return VK_PRESENT_MODE_FIFO_KHR; // always available
}

VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height)
{
    if (capabilities.currentExtent.width != 0xFFFFFFFFu) return capabilities.currentExtent;

    VkExtent2D extent{std::max(width, 1u), std::max(height, 1u)};
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
    return extent;
}
} // namespace

bool SwapChainImpl::Create(u32 width, u32 height, PresentMode mode)
{
    DeviceImpl* device = owner;
    if (device == nullptr) return false;

    VkSurfaceCapabilitiesKHR capabilities{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, device->surface,
                                                  &capabilities) != VK_SUCCESS)
    {
        PRS_ERROR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
        return false;
    }

    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, device->surface, &formatCount,
                                         nullptr);
    if (formatCount == 0) return false;
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, device->surface, &formatCount,
                                         formats.data());

    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, device->surface,
                                              &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, device->surface,
                                              &presentModeCount, presentModes.data());

    const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(formats);
    vkColorFormat = surfaceFormat.format;
    colorFormat = FromVkFormat(surfaceFormat.format);
    const VkExtent2D chosenExtent = ChooseExtent(capabilities, width, height);
    extent = {chosenExtent.width, chosenExtent.height};

    u32 imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = device->surface;
    info.minImageCount = imageCount;
    info.imageFormat = vkColorFormat;
    info.imageColorSpace = surfaceFormat.colorSpace;
    info.imageExtent = {extent.width, extent.height};
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const u32 families[2] = {device->graphicsFamily, device->presentFamily};
    if (device->graphicsFamily != device->presentFamily)
    {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = families;
    }
    else
    {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = ChoosePresentMode(presentModes, mode);
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device->device, &info, nullptr, &swapchain);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateSwapchainKHR failed: %s", ResultName(result));
        return false;
    }

    u32 actualImageCount = 0;
    vkGetSwapchainImagesKHR(device->device, swapchain, &actualImageCount, nullptr);
    std::vector<VkImage> images(actualImageCount);
    vkGetSwapchainImagesKHR(device->device, swapchain, &actualImageCount, images.data());

    renderPass = device->GetOrCreateRenderPass(colorFormat, Format::Depth32Float);
    if (renderPass == VK_NULL_HANDLE) return false;

    TextureDesc depthDesc;
    depthDesc.size = extent;
    depthDesc.format = Format::Depth32Float;
    depthDesc.usage = TextureUsage::DepthStencil;
    depthDesc.debugName = "SwapchainDepth";
    Scope<Texture> depth = device->CreateTexture(depthDesc);
    if (!depth) return false;
    depthTexture = static_cast<TextureImpl*>(depth.release());

    colorTextures.reserve(images.size());
    for (VkImage image : images)
    {
        auto texture = new TextureImpl();
        texture->owner = device;
        texture->image = image;
        texture->ownsImage = false;
        texture->format = colorFormat;
        texture->vkFormat = vkColorFormat;
        texture->extent = extent;
        texture->usage = TextureUsage::RenderTarget;
        texture->renderPass = renderPass;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = vkColorFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device->device, &viewInfo, nullptr, &texture->view) != VK_SUCCESS)
        {
            PRS_ERROR("Failed to create swapchain image view");
            delete texture;
            return false;
        }

        VkImageView attachments[2] = {texture->view, depthTexture->view};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &texture->framebuffer) !=
            VK_SUCCESS)
        {
            PRS_ERROR("vkCreateFramebuffer failed");
            delete texture;
            return false;
        }
        colorTextures.push_back(texture);
    }

    CreateSyncObjects();
    PRS_INFO("Swapchain: %ux%u, %zu images, format %d", extent.width, extent.height,
             colorTextures.size(), static_cast<int>(vkColorFormat));
    return imageAvailable != VK_NULL_HANDLE && frameFence != VK_NULL_HANDLE;
}

void SwapChainImpl::CreateSyncObjects()
{
    DeviceImpl* device = owner;

    if (imageAvailable == VK_NULL_HANDLE)
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &imageAvailable);
        vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &renderFinished);
    }

    if (frameFence == VK_NULL_HANDLE)
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(device->device, &fenceInfo, nullptr, &frameFence);
    }
}

bool SwapChainImpl::BeginFrame()
{
    DeviceImpl* device = owner;
    if (device == nullptr || swapchain == VK_NULL_HANDLE) return false;

    vkWaitForFences(device->device, 1, &frameFence, VK_TRUE, ~0ull);
    vkResetFences(device->device, 1, &frameFence);

    const VkResult result = vkAcquireNextImageKHR(device->device, swapchain, ~0ull, imageAvailable,
                                                  VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkAcquireNextImageKHR failed: %s", ResultName(result));
        return false;
    }

    frameAcquired = true;
    return true;
}

Texture& SwapChainImpl::CurrentColorTexture()
{
    return *colorTextures[imageIndex < colorTextures.size() ? imageIndex : 0];
}

Texture& SwapChainImpl::DepthTexture() { return *depthTexture; }

void SwapChainImpl::Submit(CommandList& commandList)
{
    DeviceImpl* device = owner;
    if (device == nullptr || !frameAcquired) return;

    VkCommandBuffer commandBuffer = static_cast<CommandListImpl*>(&commandList)->commandBuffer;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderFinished;

    const VkResult result = vkQueueSubmit(device->graphicsQueue, 1, &submit, frameFence);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkQueueSubmit(frame) failed: %s", ResultName(result));
        frameAcquired = false;
    }
}

bool SwapChainImpl::Present()
{
    DeviceImpl* device = owner;
    if (device == nullptr || !frameAcquired) return false;
    frameAcquired = false;

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &imageIndex;

    const VkResult result = vkQueuePresentKHR(device->presentQueue, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return false;
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkQueuePresentKHR failed: %s", ResultName(result));
        return false;
    }
    return true;
}

void SwapChainImpl::DestroySwapchainResources()
{
    DeviceImpl* device = owner;
    if (device == nullptr || device->device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device->device);

    for (TextureImpl* texture : colorTextures) delete texture;
    colorTextures.clear();

    delete depthTexture;
    depthTexture = nullptr;

    if (swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device->device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
}

bool SwapChainImpl::Resize(u32 width, u32 height)
{
    const bool wasReady = swapchain != VK_NULL_HANDLE;
    if (width == 0 || height == 0) return false;

    DestroySwapchainResources();
    const PresentMode mode = PresentMode::Fifo;
    const bool ok = Create(width, height, mode);
    if (!ok)
    {
        PRS_ERROR("Swapchain recreation for %ux%u failed", width, height);
        return false;
    }
    (void)wasReady;
    return true;
}

SwapChainImpl::~SwapChainImpl()
{
    DestroySwapchainResources();

    DeviceImpl* device = owner;
    if (device == nullptr || device->device == VK_NULL_HANDLE) return;

    if (renderFinished != VK_NULL_HANDLE)
        vkDestroySemaphore(device->device, renderFinished, nullptr);
    if (imageAvailable != VK_NULL_HANDLE)
        vkDestroySemaphore(device->device, imageAvailable, nullptr);
    if (frameFence != VK_NULL_HANDLE) vkDestroyFence(device->device, frameFence, nullptr);
}
} // namespace PRS::RHI::Vulkan

#endif // PRS_HAS_VULKAN

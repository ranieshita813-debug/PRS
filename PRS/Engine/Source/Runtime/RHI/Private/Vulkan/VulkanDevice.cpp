#include "VulkanInternal.h"
#include "PRS/Core/Log.h"
#include "PRS/Platform/Vulkan.h"
#include <algorithm>
#include <cstring>
#include <vector>

#if defined(PRS_HAS_VULKAN)

namespace PRS::RHI::Vulkan
{
namespace
{
VkBool32 VKAPI_PTR DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                 VkDebugUtilsMessageTypeFlagsEXT,
                                 const VkDebugUtilsMessengerCallbackDataEXT* data, void*)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        PRS_ERROR("[vulkan] %s", data->pMessage);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        PRS_WARN("[vulkan] %s", data->pMessage);
    else
        PRS_TRACE("[vulkan] %s", data->pMessage);
    return VK_FALSE;
}

bool InstanceLayerAvailable(const char* name)
{
    u32 count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) return false;
    std::vector<VkLayerProperties> layers(count);
    if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS) return false;
    for (const VkLayerProperties& layer : layers)
        if (std::strcmp(layer.layerName, name) == 0) return true;
    return false;
}

bool QueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, u32& graphicsFamily,
                   u32& presentFamily)
{
    graphicsFamily = static_cast<u32>(-1);
    presentFamily = static_cast<u32>(-1);

    u32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (u32 i = 0; i < count; ++i)
    {
        if (families[i].queueCount == 0) continue;
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphicsFamily == static_cast<u32>(-1))
            graphicsFamily = i;

        if (surface != VK_NULL_HANDLE)
        {
            VkBool32 supportsPresent = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent);
            if (supportsPresent && presentFamily == static_cast<u32>(-1)) presentFamily = i;
        }
    }

    if (surface != VK_NULL_HANDLE && presentFamily == static_cast<u32>(-1))
        presentFamily = graphicsFamily;

    return graphicsFamily != static_cast<u32>(-1);
}

VkFormat VertexFormatToVk(VertexFormat format)
{
    switch (format)
    {
    case VertexFormat::Float: return VK_FORMAT_R32_SFLOAT;
    case VertexFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
    case VertexFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
    case VertexFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return VK_FORMAT_R32G32B32_SFLOAT;
}
} // namespace

// ---------------------------------------------------------------------------
// Lifetime helpers
// ---------------------------------------------------------------------------

BufferImpl::~BufferImpl()
{
    if (owner == nullptr || owner->device == VK_NULL_HANDLE) return;
    if (mapped != nullptr) vkUnmapMemory(owner->device, memory);
    if (buffer != VK_NULL_HANDLE) vkDestroyBuffer(owner->device, buffer, nullptr);
    if (memory != VK_NULL_HANDLE) vkFreeMemory(owner->device, memory, nullptr);
}

TextureImpl::~TextureImpl()
{
    if (owner == nullptr || owner->device == VK_NULL_HANDLE) return;
    if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(owner->device, framebuffer, nullptr);
    if (view != VK_NULL_HANDLE) vkDestroyImageView(owner->device, view, nullptr);
    if (ownsImage && image != VK_NULL_HANDLE) vkDestroyImage(owner->device, image, nullptr);
    if (ownsImage && memory != VK_NULL_HANDLE) vkFreeMemory(owner->device, memory, nullptr);
}

SamplerImpl::~SamplerImpl()
{
    if (owner != nullptr && owner->device != VK_NULL_HANDLE && sampler != VK_NULL_HANDLE)
        vkDestroySampler(owner->device, sampler, nullptr);
}

FenceImpl::~FenceImpl()
{
    if (owner != nullptr && owner->device != VK_NULL_HANDLE && fence != VK_NULL_HANDLE)
        vkDestroyFence(owner->device, fence, nullptr);
}

PipelineImpl::~PipelineImpl()
{
    if (owner == nullptr || owner->device == VK_NULL_HANDLE) return;
    if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(owner->device, pipeline, nullptr);
    if (layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(owner->device, layout, nullptr);
}

void FenceImpl::Wait(u64 timeoutNs)
{
    if (owner == nullptr || fence == VK_NULL_HANDLE) return;
    const VkResult result = vkWaitForFences(owner->device, 1, &fence, VK_TRUE, timeoutNs);
    if (result == VK_TIMEOUT) PRS_WARN("Fence::Wait timed out");
}

void FenceImpl::Reset()
{
    if (owner != nullptr && fence != VK_NULL_HANDLE) vkResetFences(owner->device, 1, &fence);
}

bool FenceImpl::IsSignaled() const
{
    if (owner == nullptr || fence == VK_NULL_HANDLE) return true;
    return vkGetFenceStatus(owner->device, fence) == VK_SUCCESS;
}

// ---------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------

DeviceImpl::~DeviceImpl() { Shutdown(); }

bool DeviceImpl::Initialize(const DeviceDesc& desc)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = desc.applicationName;
    appInfo.applicationVersion = desc.applicationVersion;
    appInfo.pEngineName = "PRS Engine";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_1;

    Array<const char*> extensions;
    bool wantsSurface = desc.nativeWindow != nullptr;
    if (wantsSurface && !Platform::GetVulkanInstanceExtensions(extensions))
    {
        PRS_WARN("No Vulkan window surface available, creating a headless device");
        wantsSurface = false;
        extensions.clear();
    }

    const bool hasDebugUtilsExtension =
        std::find(extensions.begin(), extensions.end(),
                  std::string(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) != extensions.end();

    Array<const char*> layers;
    if (desc.enableValidation && InstanceLayerAvailable("VK_LAYER_KHRONOS_validation"))
    {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        if (!hasDebugUtilsExtension) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();
    instanceInfo.enabledLayerCount = static_cast<u32>(layers.size());
    instanceInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateInstance failed: %s", ResultName(result));
        return false;
    }

    if (!layers.empty())
    {
        auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (createMessenger != nullptr)
        {
            VkDebugUtilsMessengerCreateInfoEXT messengerInfo{};
            messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            messengerInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            messengerInfo.pfnUserCallback = DebugCallback;
            createMessenger(instance, &messengerInfo, nullptr, &debugMessenger);
        }
    }

    if (desc.nativeWindow != nullptr)
    {
        surface = static_cast<VkSurfaceKHR>(
            Platform::CreateVulkanSurface(instance, desc.nativeWindow));
        hasSurface = surface != VK_NULL_HANDLE;
    }

    u32 deviceCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (result != VK_SUCCESS || deviceCount == 0)
    {
        PRS_ERROR("No Vulkan physical devices found (%s)", ResultName(result));
        return false;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    i32 bestScore = -1;
    for (VkPhysicalDevice candidate : devices)
    {
        u32 graphics = 0;
        u32 present = 0;
        if (!QueueFamilies(candidate, hasSurface ? surface : VK_NULL_HANDLE, graphics, present))
            continue;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);

        i32 score = static_cast<i32>(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                                         ? 1000
                                         : 0);
        score += static_cast<i32>(properties.limits.maxImageDimension2D / 1024);
        if (score > bestScore)
        {
            bestScore = score;
            physicalDevice = candidate;
            graphicsFamily = graphics;
            presentFamily = present;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        PRS_ERROR("No Vulkan device supports the required graphics queue");
        return false;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    adapterInfo.name = properties.deviceName;
    adapterInfo.vendorId = properties.vendorID;
    adapterInfo.deviceId = properties.deviceID;
    adapterInfo.apiVersionMajor = VK_API_VERSION_MAJOR(properties.apiVersion);
    adapterInfo.apiVersionMinor = VK_API_VERSION_MINOR(properties.apiVersion);
    adapterInfo.apiVersionPatch = VK_API_VERSION_PATCH(properties.apiVersion);
    adapterInfo.discrete = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    for (u32 i = 0; i < memoryProperties.memoryHeapCount; ++i)
        if ((memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
            adapterInfo.deviceMemoryBytes += memoryProperties.memoryHeaps[i].size;

    VkPhysicalDeviceFeatures features{};
    VkPhysicalDeviceFeatures available{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &available);
    features.samplerAnisotropy = available.samplerAnisotropy;
    features.fillModeNonSolid = available.fillModeNonSolid;

    std::vector<u32> uniqueFamilies{graphicsFamily};
    if (presentFamily != graphicsFamily) uniqueFamilies.push_back(presentFamily);

    const f32 priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (u32 family : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueInfos.push_back(queueInfo);
    }

    Array<const char*> deviceExtensions;
    if (hasSurface) deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<u32>(queueInfos.size());
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames =
        deviceExtensions.empty() ? nullptr : deviceExtensions.data();

    result = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateDevice failed: %s", ResultName(result));
        return false;
    }

    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);

    graphicsQueueWrapper.owner = this;
    graphicsQueueWrapper.queue = graphicsQueue;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily;
    result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateCommandPool failed: %s", ResultName(result));
        return false;
    }

    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 32;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 32;

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.maxSets = 16;
    descriptorPoolInfo.poolSizeCount = 2;
    descriptorPoolInfo.pPoolSizes = poolSizes;
    result = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateDescriptorPool failed: %s", ResultName(result));
        return false;
    }

    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = 2;
    setLayoutInfo.pBindings = bindings;
    result = vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &frameSetLayout);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateDescriptorSetLayout failed: %s", ResultName(result));
        return false;
    }

    PRS_INFO("Vulkan device: %s (%s, API %u.%u.%u)", adapterInfo.name.c_str(),
             adapterInfo.discrete ? "discrete" : "integrated", adapterInfo.apiVersionMajor,
             adapterInfo.apiVersionMinor, adapterInfo.apiVersionPatch);
    return true;
}

void DeviceImpl::Shutdown()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device);
        swapchain.reset();

        for (auto& entry : renderPassCache)
            if (entry.second != VK_NULL_HANDLE)
                vkDestroyRenderPass(device, entry.second, nullptr);
        renderPassCache.clear();

        if (frameSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device, frameSetLayout, nullptr);
        frameSetLayout = VK_NULL_HANDLE;

        if (descriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;

        if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;

        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        Platform::DestroyVulkanSurface(instance, surface);
        surface = VK_NULL_HANDLE;
    }

    if (debugMessenger != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        auto destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyMessenger != nullptr)
            destroyMessenger(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}

VkRenderPass DeviceImpl::GetOrCreateRenderPass(Format colorFormat, Format depthFormat)
{
    const u64 key = (static_cast<u64>(colorFormat) << 8) | static_cast<u64>(depthFormat);
    const auto existing = renderPassCache.find(key);
    if (existing != renderPassCache.end()) return existing->second;

    const bool hasDepth = depthFormat != Format::Undefined;

    VkAttachmentDescription attachments[2]{};
    attachments[0].format = ToVkFormat(colorFormat);
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (hasDepth)
    {
        attachments[1].format = ToVkFormat(depthFormat);
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pDepthStencilAttachment = hasDepth ? &depthReference : nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = hasDepth ? 2u : 1u;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    const VkResult result = vkCreateRenderPass(device, &info, nullptr, &renderPass);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateRenderPass failed: %s", ResultName(result));
        return VK_NULL_HANDLE;
    }

    renderPassCache.emplace(key, renderPass);
    return renderPass;
}

SwapChain* DeviceImpl::CreateSwapChain(const SwapChainDesc& desc)
{
    if (!hasSurface || desc.nativeWindow == nullptr)
    {
        PRS_ERROR("CreateSwapChain called without a presentation surface");
        return nullptr;
    }

    swapchain = MakeScope<SwapChainImpl>();
    swapchain->owner = this;
    if (!swapchain->Create(desc.width, desc.height, desc.presentMode))
    {
        swapchain.reset();
        return nullptr;
    }
    framesInFlight = desc.framesInFlight > 0 ? desc.framesInFlight : 2;
    return swapchain.get();
}

Scope<Buffer> DeviceImpl::CreateBuffer(const BufferDesc& desc)
{
    auto buffer = MakeScope<BufferImpl>();
    buffer->owner = this;
    buffer->size = desc.size;
    buffer->usage = desc.usage;
    buffer->hostVisible = desc.hostVisible;

    VkBufferUsageFlags usage = 0;
    if (HasFlag(desc.usage, BufferUsage::Vertex)) usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (HasFlag(desc.usage, BufferUsage::Index)) usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (HasFlag(desc.usage, BufferUsage::Uniform)) usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (HasFlag(desc.usage, BufferUsage::Storage)) usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (HasFlag(desc.usage, BufferUsage::TransferSrc)) usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (HasFlag(desc.usage, BufferUsage::TransferDst)) usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (usage == 0) usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = desc.size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &info, nullptr, &buffer->buffer);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateBuffer failed: %s", ResultName(result));
        return nullptr;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer->buffer, &requirements);

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex =
        FindMemoryType(memoryProperties, requirements.memoryTypeBits,
                       desc.hostVisible ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                                        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = vkAllocateMemory(device, &allocation, nullptr, &buffer->memory);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkAllocateMemory(buffer) failed: %s", ResultName(result));
        return nullptr;
    }
    vkBindBufferMemory(device, buffer->buffer, buffer->memory, 0);

    if (desc.hostVisible)
    {
        result = vkMapMemory(device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped);
        if (result != VK_SUCCESS)
        {
            PRS_ERROR("vkMapMemory failed: %s", ResultName(result));
            buffer->mapped = nullptr;
        }
    }
    return buffer;
}

Scope<Texture> DeviceImpl::CreateTexture(const TextureDesc& desc)
{
    auto texture = MakeScope<TextureImpl>();
    texture->owner = this;
    texture->format = desc.format;
    texture->extent = desc.size;
    texture->usage = desc.usage;
    texture->vkFormat = ToVkFormat(desc.format);

    VkImageUsageFlags usage = 0;
    if (HasFlag(desc.usage, TextureUsage::Sampled))
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (HasFlag(desc.usage, TextureUsage::RenderTarget)) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (HasFlag(desc.usage, TextureUsage::DepthStencil))
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (HasFlag(desc.usage, TextureUsage::TransferDst)) usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage == 0) usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {desc.size.width > 0 ? desc.size.width : 1,
                        desc.size.height > 0 ? desc.size.height : 1, 1};
    imageInfo.mipLevels = desc.mipLevels > 0 ? desc.mipLevels : 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = texture->vkFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &texture->image);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateImage failed: %s", ResultName(result));
        return nullptr;
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device, texture->image, &requirements);

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex =
        FindMemoryType(memoryProperties, requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = vkAllocateMemory(device, &allocation, nullptr, &texture->memory);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkAllocateMemory(image) failed: %s", ResultName(result));
        return nullptr;
    }
    vkBindImageMemory(device, texture->image, texture->memory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = texture->vkFormat;
    viewInfo.subresourceRange.aspectMask =
        IsDepthFormat(desc.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device, &viewInfo, nullptr, &texture->view);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateImageView failed: %s", ResultName(result));
        return nullptr;
    }
    return texture;
}

Scope<Sampler> DeviceImpl::CreateSampler(const SamplerDesc& desc)
{
    auto sampler = MakeScope<SamplerImpl>();
    sampler->owner = this;

    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = desc.magFilter == Filter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    info.minFilter = desc.minFilter == Filter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    const auto toAddress = [](AddressMode mode) {
        switch (mode)
        {
        case AddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    };
    info.addressModeU = toAddress(desc.addressU);
    info.addressModeV = toAddress(desc.addressV);
    info.addressModeW = toAddress(desc.addressW);
    info.anisotropyEnable = desc.anisotropy ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy = desc.maxAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.maxLod = desc.maxLod;

    const VkResult result = vkCreateSampler(device, &info, nullptr, &sampler->sampler);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateSampler failed: %s", ResultName(result));
        return nullptr;
    }
    return sampler;
}

Scope<Fence> DeviceImpl::CreateFence(bool signaled)
{
    auto fence = MakeScope<FenceImpl>();
    fence->owner = this;

    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    const VkResult result = vkCreateFence(device, &info, nullptr, &fence->fence);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateFence failed: %s", ResultName(result));
        return nullptr;
    }
    return fence;
}

Scope<CommandList> DeviceImpl::CreateCommandList()
{
    auto list = MakeScope<CommandListImpl>();
    list->owner = this;

    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = commandPool;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = 1;

    const VkResult result = vkAllocateCommandBuffers(device, &info, &list->commandBuffer);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkAllocateCommandBuffers failed: %s", ResultName(result));
        return nullptr;
    }
    return list;
}

Scope<DescriptorHeap> DeviceImpl::CreateDescriptorHeap()
{
    auto heap = MakeScope<DescriptorHeapImpl>();
    heap->owner = this;

    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = descriptorPool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &frameSetLayout;

    const VkResult result = vkAllocateDescriptorSets(device, &info, &heap->set);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkAllocateDescriptorSets failed: %s", ResultName(result));
        return nullptr;
    }
    return heap;
}

Scope<Pipeline> DeviceImpl::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    if (desc.vertexShaderPath == nullptr || desc.fragmentShaderPath == nullptr)
    {
        PRS_ERROR("CreateGraphicsPipeline needs both shader paths");
        return nullptr;
    }

    VkShaderModule vertexModule = CreateShaderModule(device, desc.vertexShaderPath);
    VkShaderModule fragmentModule = CreateShaderModule(device, desc.fragmentShaderPath);
    if (vertexModule == VK_NULL_HANDLE || fragmentModule == VK_NULL_HANDLE) return nullptr;

    auto pipeline = MakeScope<PipelineImpl>();
    pipeline->owner = this;
    pipeline->desc = desc;

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = desc.pushConstantSize;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &frameSetLayout;
    layoutInfo.pushConstantRangeCount = desc.pushConstantSize > 0 ? 1u : 0u;
    layoutInfo.pPushConstantRanges = desc.pushConstantSize > 0 ? &pushRange : nullptr;

    VkResult result = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipeline->layout);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreatePipelineLayout failed: %s", ResultName(result));
        vkDestroyShaderModule(device, vertexModule, nullptr);
        vkDestroyShaderModule(device, fragmentModule, nullptr);
        return nullptr;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertexModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragmentModule;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = desc.vertexBinding.stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.vertexBinding.attributes.size());
    for (const VertexAttribute& attribute : desc.vertexBinding.attributes)
    {
        VkVertexInputAttributeDescription vkAttribute{};
        vkAttribute.location = attribute.location;
        vkAttribute.binding = 0;
        vkAttribute.format = VertexFormatToVk(attribute.format);
        vkAttribute.offset = attribute.offset;
        attributes.push_back(vkAttribute);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = desc.vertexBinding.stride > 0 ? 1u : 0u;
    vertexInput.pVertexBindingDescriptions = desc.vertexBinding.stride > 0 ? &binding : nullptr;
    vertexInput.vertexAttributeDescriptionCount = static_cast<u32>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.empty() ? nullptr : attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = desc.topology == PrimitiveTopology::TriangleStrip
                                 ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                                 : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = desc.cullMode == CullMode::None
                              ? VK_CULL_MODE_NONE
                              : (desc.cullMode == CullMode::Front ? VK_CULL_MODE_FRONT_BIT
                                                                  : VK_CULL_MODE_BACK_BIT);
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.depthClampEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable =
        desc.depthTest && desc.depthFormat != Format::Undefined ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorAttachment{};
    colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorAttachment.blendEnable = desc.alphaBlend ? VK_TRUE : VK_FALSE;
    if (desc.alphaBlend)
    {
        colorAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &colorAttachment;

    const VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkRenderPass renderPass = GetOrCreateRenderPass(desc.colorFormat, desc.depthFormat);
    if (renderPass == VK_NULL_HANDLE) return nullptr;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &inputAssembly;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &rasterizer;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depthStencil;
    info.pColorBlendState = &colorBlend;
    info.pDynamicState = &dynamicState;
    info.layout = pipeline->layout;
    info.renderPass = renderPass;
    info.subpass = 0;

    result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr,
                                       &pipeline->pipeline);

    vkDestroyShaderModule(device, vertexModule, nullptr);
    vkDestroyShaderModule(device, fragmentModule, nullptr);

    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateGraphicsPipelines('%s') failed: %s",
                  desc.debugName != nullptr ? desc.debugName : "pipeline", ResultName(result));
        return nullptr;
    }
    return pipeline;
}

void DeviceImpl::WaitIdle()
{
    if (device != VK_NULL_HANDLE) vkDeviceWaitIdle(device);
}

// ---------------------------------------------------------------------------
// Buffer writes and texture uploads
// ---------------------------------------------------------------------------

void BufferImpl::Write(const void* data, u64 bytes, u64 offset)
{
    if (owner == nullptr || data == nullptr || bytes == 0) return;
    if (owner->device == VK_NULL_HANDLE) return;

    const u64 copySize = (offset + bytes <= size) ? bytes : (size > offset ? size - offset : 0);
    if (copySize == 0) return;

    if (hostVisible && mapped != nullptr)
    {
        std::memcpy(static_cast<u8*>(mapped) + offset, data, copySize);
        return;
    }

    // Device-local: stage through a host-visible buffer and a one-shot upload.
    BufferDesc stagingDesc;
    stagingDesc.size = copySize;
    stagingDesc.usage = BufferUsage::TransferSrc;
    stagingDesc.hostVisible = true;
    auto staging = owner->CreateBuffer(stagingDesc);
    if (!staging) return;
    std::memcpy(staging->MappedPointer(), data, copySize);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = owner->commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(owner->device, &allocateInfo, &commandBuffer) != VK_SUCCESS) return;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copy{};
    copy.srcOffset = 0;
    copy.dstOffset = offset;
    copy.size = copySize;
    vkCmdCopyBuffer(commandBuffer, static_cast<BufferImpl*>(staging.get())->buffer, buffer, 1, &copy);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(owner->graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(owner->graphicsQueue);

    vkFreeCommandBuffers(owner->device, owner->commandPool, 1, &commandBuffer);
}

void TextureImpl::Upload(const void* pixels, u64 sizeBytes, u32 mipLevel)
{
    if (owner == nullptr || pixels == nullptr || owner->device == VK_NULL_HANDLE) return;
    (void)mipLevel;

    const u64 expected = static_cast<u64>(extent.width) * extent.height *
                         FormatBytesPerPixel(format);
    const u64 copySize = expected > 0 && sizeBytes >= expected ? expected : sizeBytes;

    BufferDesc stagingDesc;
    stagingDesc.size = copySize;
    stagingDesc.usage = BufferUsage::TransferSrc;
    stagingDesc.hostVisible = true;
    auto staging = owner->CreateBuffer(stagingDesc);
    if (!staging) return;
    std::memcpy(staging->MappedPointer(), pixels, copySize);

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = owner->commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(owner->device, &allocateInfo, &commandBuffer) != VK_SUCCESS) return;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier toTransfer{};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = image;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.layerCount = 1;
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &toTransfer);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyBufferToImage(commandBuffer, static_cast<BufferImpl*>(staging.get())->buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier toShaderRead = toTransfer;
    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &toShaderRead);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(owner->graphicsQueue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(owner->graphicsQueue);

    vkFreeCommandBuffers(owner->device, owner->commandPool, 1, &commandBuffer);
}

// ---------------------------------------------------------------------------
// Descriptors and submission
// ---------------------------------------------------------------------------

void DescriptorHeapImpl::SetUniformBuffer(u32 binding, Buffer& buffer, u64 offset, u64 range)
{
    if (owner == nullptr || set == VK_NULL_HANDLE) return;

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = static_cast<BufferImpl*>(&buffer)->buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range > 0 ? range : VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(owner->device, 1, &write, 0, nullptr);
}

void DescriptorHeapImpl::SetCombinedImageSampler(u32 binding, Texture& texture, Sampler& sampler)
{
    if (owner == nullptr || set == VK_NULL_HANDLE) return;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = static_cast<TextureImpl*>(&texture)->view;
    imageInfo.sampler = static_cast<SamplerImpl*>(&sampler)->sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(owner->device, 1, &write, 0, nullptr);
}

void QueueImpl::Submit(CommandList& commandList, Fence* signalFence)
{
    if (owner == nullptr || queue == VK_NULL_HANDLE) return;

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = static_cast<CommandListImpl*>(&commandList)->commandBuffer;
    submit.pCommandBuffers = &commandBuffer;

    VkFence fence = VK_NULL_HANDLE;
    if (signalFence != nullptr) fence = static_cast<FenceImpl*>(signalFence)->fence;

    const VkResult result = vkQueueSubmit(queue, 1, &submit, fence);
    if (result != VK_SUCCESS) PRS_ERROR("vkQueueSubmit failed: %s", ResultName(result));
}

void QueueImpl::WaitIdle()
{
    if (queue != VK_NULL_HANDLE) vkQueueWaitIdle(queue);
}
} // namespace PRS::RHI::Vulkan

namespace PRS::RHI
{
Scope<Device> CreateDevice(const DeviceDesc& desc)
{
    auto device = MakeScope<Vulkan::DeviceImpl>();
    if (!device->Initialize(desc))
    {
        PRS_ERROR("Vulkan device creation failed; running without rendering");
        return nullptr;
    }
    return device;
}
} // namespace PRS::RHI

#endif // PRS_HAS_VULKAN

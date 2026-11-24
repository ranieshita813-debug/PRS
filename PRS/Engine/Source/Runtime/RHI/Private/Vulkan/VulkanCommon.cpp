#include "VulkanInternal.h"
#include "PRS/Core/Log.h"
#include <cstring>
#include <fstream>
#include <vector>

namespace PRS::RHI::Vulkan
{
u32 FindMemoryType(const VkPhysicalDeviceMemoryProperties& properties, u32 typeBits,
                   VkMemoryPropertyFlags required)
{
    for (u32 i = 0; i < properties.memoryTypeCount; ++i)
    {
        if ((typeBits & (1u << i)) == 0) continue;
        if ((properties.memoryTypes[i].propertyFlags & required) == required) return i;
    }
    return 0;
}

VkFormat ToVkFormat(Format format)
{
    switch (format)
    {
    case Format::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::RGBA8Srgb: return VK_FORMAT_R8G8B8A8_SRGB;
    case Format::BGRA8Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::BGRA8Srgb: return VK_FORMAT_B8G8R8A8_SRGB;
    case Format::RGBA16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format::R32Float: return VK_FORMAT_R32_SFLOAT;
    case Format::Depth32Float: return VK_FORMAT_D32_SFLOAT;
    case Format::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::Undefined: return VK_FORMAT_UNDEFINED;
    }
    return VK_FORMAT_UNDEFINED;
}

Format FromVkFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8G8B8A8_UNORM: return Format::RGBA8Unorm;
    case VK_FORMAT_R8G8B8A8_SRGB: return Format::RGBA8Srgb;
    case VK_FORMAT_B8G8R8A8_UNORM: return Format::BGRA8Unorm;
    case VK_FORMAT_B8G8R8A8_SRGB: return Format::BGRA8Srgb;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return Format::RGBA16Float;
    case VK_FORMAT_R32_SFLOAT: return Format::R32Float;
    case VK_FORMAT_D32_SFLOAT: return Format::Depth32Float;
    case VK_FORMAT_D24_UNORM_S8_UINT: return Format::Depth24Stencil8;
    default: return Format::Undefined;
    }
}

VkImageLayout ToVkImageLayout(ResourceState state)
{
    switch (state)
    {
    case ResourceState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
    case ResourceState::ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ResourceState::ColorTarget: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case ResourceState::DepthTarget: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case ResourceState::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ResourceState::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ResourceState::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlags ToVkAccess(ResourceState state)
{
    switch (state)
    {
    case ResourceState::ShaderRead: return VK_ACCESS_SHADER_READ_BIT;
    case ResourceState::ColorTarget: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case ResourceState::DepthTarget: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case ResourceState::TransferSrc: return VK_ACCESS_TRANSFER_READ_BIT;
    case ResourceState::TransferDst: return VK_ACCESS_TRANSFER_WRITE_BIT;
    default: return 0;
    }
}

VkPipelineStageFlags ToVkPipelineStage(ResourceState state)
{
    switch (state)
    {
    case ResourceState::ShaderRead: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case ResourceState::ColorTarget: return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case ResourceState::DepthTarget:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
               VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    case ResourceState::TransferSrc:
    case ResourceState::TransferDst: return VK_PIPELINE_STAGE_TRANSFER_BIT;
    default: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
}

const char* ResultName(VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    default: return "VK_ERROR_UNKNOWN";
    }
}

VkShaderModule CreateShaderModule(VkDevice device, const char* path)
{
    if (path == nullptr)
    {
        PRS_ERROR("Pipeline shader path is null");
        return VK_NULL_HANDLE;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        PRS_ERROR("Could not open shader '%s' (compile the .glsl sources with ShaderCompiler)", path);
        return VK_NULL_HANDLE;
    }

    const std::streamsize size = file.tellg();
    if (size <= 0 || size % 4 != 0)
    {
        PRS_ERROR("Shader '%s' is empty or not a SPIR-V binary", path);
        return VK_NULL_HANDLE;
    }
    file.seekg(0);

    std::vector<u32> code(static_cast<size_t>(size) / sizeof(u32));
    file.read(reinterpret_cast<char*>(code.data()), size);

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = static_cast<size_t>(size);
    info.pCode = code.data();

    VkShaderModule module = VK_NULL_HANDLE;
    const VkResult result = vkCreateShaderModule(device, &info, nullptr, &module);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateShaderModule('%s') failed: %s", path, ResultName(result));
        return VK_NULL_HANDLE;
    }
    return module;
}
} // namespace PRS::RHI::Vulkan

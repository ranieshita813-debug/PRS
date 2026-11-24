#include "PRS/Platform/Vulkan.h"
#include "PRS/Core/Log.h"
#include "PRS/Core/Platform.h"

#if !defined(PRS_HAS_VULKAN)

// Built without Vulkan: the window is still usable, there is just no surface.
namespace PRS::Platform
{
bool GetVulkanInstanceExtensions(Array<const char*>& outExtensions)
{
    (void)outExtensions;
    return false;
}
void* CreateVulkanSurface(void* vkInstance, void* nativeWindow)
{
    (void)vkInstance;
    (void)nativeWindow;
    return nullptr;
}
void DestroyVulkanSurface(void* vkInstance, void* surface)
{
    (void)vkInstance;
    (void)surface;
}
} // namespace PRS::Platform

#elif defined(PRS_PLATFORM_ANDROID)
#include <android/native_window.h>
#include <vulkan/vulkan.h>
#else
#include <GLFW/glfw3.h>
#endif

namespace PRS::Platform
{
#if defined(PRS_PLATFORM_ANDROID)

bool GetVulkanInstanceExtensions(Array<const char*>& outExtensions)
{
    outExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    outExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    return true;
}

void* CreateVulkanSurface(void* vkInstance, void* nativeWindow)
{
    if (vkInstance == nullptr || nativeWindow == nullptr) return nullptr;

    VkAndroidSurfaceCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    info.window = static_cast<ANativeWindow*>(nativeWindow);

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = vkCreateAndroidSurfaceKHR(static_cast<VkInstance>(vkInstance), &info,
                                                      nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("vkCreateAndroidSurfaceKHR failed: %d", static_cast<int>(result));
        return nullptr;
    }
    return surface;
}

void DestroyVulkanSurface(void* vkInstance, void* surface)
{
    if (vkInstance != nullptr && surface != nullptr)
        vkDestroySurfaceKHR(static_cast<VkInstance>(vkInstance), static_cast<VkSurfaceKHR>(surface),
                            nullptr);
}

#else // desktop: GLFW owns the window and knows the right WSI extension for this machine

bool GetVulkanInstanceExtensions(Array<const char*>& outExtensions)
{
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    if (extensions == nullptr || count == 0)
    {
        PRS_ERROR("GLFW did not report Vulkan instance extensions (is the Vulkan loader installed?)");
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) outExtensions.push_back(extensions[i]);
    return true;
}

void* CreateVulkanSurface(void* vkInstance, void* nativeWindow)
{
    if (vkInstance == nullptr || nativeWindow == nullptr) return nullptr;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(vkInstance),
                                                    static_cast<GLFWwindow*>(nativeWindow), nullptr,
                                                    &surface);
    if (result != VK_SUCCESS)
    {
        PRS_ERROR("glfwCreateWindowSurface failed: %d", static_cast<int>(result));
        return nullptr;
    }
    return surface;
}

void DestroyVulkanSurface(void* vkInstance, void* surface)
{
    if (vkInstance != nullptr && surface != nullptr)
        vkDestroySurfaceKHR(static_cast<VkInstance>(vkInstance), static_cast<VkSurfaceKHR>(surface),
                            nullptr);
}

#endif
} // namespace PRS::Platform

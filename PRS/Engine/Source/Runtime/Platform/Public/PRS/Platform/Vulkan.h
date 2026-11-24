#pragma once
#include "PRS/Core/Containers/Containers.h"

namespace PRS::Platform
{
// The platform layer owns the native window, so it also knows how to turn that
// window into a Vulkan presentation surface. Handles stay as void* so no Vulkan
// headers leak into this module.

// Appends the instance extensions required to present to a Window created by
// this platform. Returns false when the platform has no Vulkan support.
bool GetVulkanInstanceExtensions(Array<const char*>& outExtensions);

// Creates a VkSurfaceKHR (returned as void*). Requires a created Window.
void* CreateVulkanSurface(void* vkInstance, void* nativeWindow);
void DestroyVulkanSurface(void* vkInstance, void* surface);
} // namespace PRS::Platform

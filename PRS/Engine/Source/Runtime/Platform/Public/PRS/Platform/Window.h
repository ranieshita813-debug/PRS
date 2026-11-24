#pragma once
#include "PRS/Core/Types.h"

namespace PRS
{
struct WindowDesc
{
    const char* title = "PRS";
    u32 width = 1280;
    u32 height = 720;
    bool resizable = true;
};

class Window
{
public:
    explicit Window(const WindowDesc& desc);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void PollEvents();
    bool ShouldClose() const;
    void RequestClose();
    u32 Width() const { return m_width; }
    u32 Height() const { return m_height; }
    f32 AspectRatio() const { return m_height == 0 ? 1.0f : static_cast<f32>(m_width) / static_cast<f32>(m_height); }
    // Opaque native handle. Desktop: GLFWwindow*. Android: ANativeWindow*.
    // Used by the Vulkan swapchain, and on Android by the software presenter.
    void* NativeHandle() const { return m_handle; }

private:
    void* m_handle = nullptr;
    u32 m_width, m_height;
};
} // namespace PRS

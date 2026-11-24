#include "PRS/Platform/Window.h"
#include "PRS/Core/Platform.h"

#if !defined(PRS_PLATFORM_ANDROID)

#include "PRS/Core/Assert.h"
#include <GLFW/glfw3.h>

namespace PRS
{
namespace { int g_windowCount = 0; }

Window::Window(const WindowDesc& desc) : m_width(desc.width), m_height(desc.height)
{
    if (g_windowCount == 0)
    {
        PRS_VERIFY(glfwInit() == GLFW_TRUE, "glfwInit failed");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan-ready: no GL context
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
    GLFWwindow* w = glfwCreateWindow(static_cast<int>(desc.width), static_cast<int>(desc.height), desc.title, nullptr, nullptr);
    PRS_VERIFY(w != nullptr, "Failed to create window");
    m_handle = w;
    ++g_windowCount;
    PRS_INFO("Window created: %ux%u \"%s\"", desc.width, desc.height, desc.title);
}

Window::~Window()
{
    if (m_handle) glfwDestroyWindow(static_cast<GLFWwindow*>(m_handle));
    if (--g_windowCount == 0) glfwTerminate();
}

void Window::PollEvents()
{
    glfwPollEvents();
    int w, h;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_handle), &w, &h);
    m_width = static_cast<u32>(w);
    m_height = static_cast<u32>(h);
}

bool Window::ShouldClose() const { return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_handle)) != 0; }
void Window::RequestClose() { glfwSetWindowShouldClose(static_cast<GLFWwindow*>(m_handle), GLFW_TRUE); }
} // namespace PRS

#endif // !PRS_PLATFORM_ANDROID

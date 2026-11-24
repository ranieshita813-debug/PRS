#include "PRS/Platform/Input/Input.h"
#include "PRS/Core/Platform.h"

#if defined(PRS_PLATFORM_ANDROID)

namespace PRS::Input
{
bool IsKeyDown(const Window&, Key) { return false; }
bool IsMouseDown(const Window&, MouseButton) { return false; }
void GetMousePosition(const Window&, double& x, double& y)
{
    x = 0.0;
    y = 0.0;
}
} // namespace PRS::Input

#else

#include "PRS/Platform/Window.h"
#include <GLFW/glfw3.h>

namespace PRS::Input
{
bool IsKeyDown(const Window& window, Key key)
{
    return glfwGetKey(static_cast<GLFWwindow*>(window.NativeHandle()), static_cast<int>(key)) == GLFW_PRESS;
}

bool IsMouseDown(const Window& window, MouseButton button)
{
    return glfwGetMouseButton(static_cast<GLFWwindow*>(window.NativeHandle()), static_cast<int>(button)) ==
           GLFW_PRESS;
}

void GetMousePosition(const Window& window, double& x, double& y)
{
    glfwGetCursorPos(static_cast<GLFWwindow*>(window.NativeHandle()), &x, &y);
}
} // namespace PRS::Input

#endif // PRS_PLATFORM_ANDROID

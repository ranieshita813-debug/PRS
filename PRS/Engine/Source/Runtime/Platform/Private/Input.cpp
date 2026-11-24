#include "PRS/Platform/Input/Input.h"
#include "PRS/Platform/Window.h"
#include <GLFW/glfw3.h>

namespace PRS::Input
{
bool IsKeyDown(const Window& window, Key key)
{
    return glfwGetKey(static_cast<GLFWwindow*>(window.NativeHandle()), static_cast<int>(key)) == GLFW_PRESS;
}
void GetMousePosition(const Window& window, double& x, double& y)
{
    glfwGetCursorPos(static_cast<GLFWwindow*>(window.NativeHandle()), &x, &y);
}
} // namespace PRS::Input

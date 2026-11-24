#pragma once
#include "PRS/Core/Types.h"

namespace PRS
{
class Window;

// Values match GLFW key codes so no translation table is needed.
enum class Key : i32
{
    Space = 32, A = 65, D = 68, E = 69, Q = 81, S = 83, W = 87,
    Escape = 256, Enter = 257, Tab = 258,
    Right = 262, Left = 263, Down = 264, Up = 265,
    LeftShift = 340, LeftControl = 341
};

// Values match GLFW mouse button codes. Touch-only platforms always report "up".
enum class MouseButton : i32
{
    Left = 0,
    Right = 1,
    Middle = 2
};

namespace Input
{
bool IsKeyDown(const Window& window, Key key);
bool IsMouseDown(const Window& window, MouseButton button = MouseButton::Left);
void GetMousePosition(const Window& window, double& x, double& y);
} // namespace Input
} // namespace PRS

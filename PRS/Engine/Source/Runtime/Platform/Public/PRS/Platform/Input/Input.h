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

namespace Input
{
bool IsKeyDown(const Window& window, Key key);
void GetMousePosition(const Window& window, double& x, double& y);
} // namespace Input
} // namespace PRS

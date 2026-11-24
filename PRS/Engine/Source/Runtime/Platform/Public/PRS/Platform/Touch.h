#pragma once
#include "PRS/Core/Types.h"

namespace PRS::Platform::Touch
{
enum class Phase : u8
{
    Down,
    Move,
    Up
};

// One touch event in window pixel coordinates.
struct Point
{
    i32 id = 0;
    f32 x = 0.0f;
    f32 y = 0.0f;
    Phase phase = Phase::Move;
};

// Drains the touch events collected since the last call. Returns the number of
// events written (may be 0). Call this once per frame after Window::PollEvents().
u32 Poll(Point* outPoints, u32 maxPoints);

// True once per Android back gesture / back button press.
bool BackPressed();

// True when the platform has touch input (Android, iOS) rather than a mouse.
bool HasTouchInput();
} // namespace PRS::Platform::Touch

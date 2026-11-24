#include "PRS/Platform/Touch.h"
#include "PRS/Core/Platform.h"

#if defined(PRS_PLATFORM_ANDROID)

#include "AndroidState.h"

namespace PRS::Platform::Touch
{
u32 Poll(Point* outPoints, u32 maxPoints)
{
    Android::State& state = Android::Get();
    if (!outPoints || maxPoints == 0)
    {
        state.touchCount = 0;
        return 0;
    }

    const u32 count = state.touchCount < maxPoints ? state.touchCount : maxPoints;
    for (u32 i = 0; i < count; ++i) outPoints[i] = state.touchEvents[i];
    state.touchCount = 0;
    return count;
}

bool BackPressed()
{
    Android::State& state = Android::Get();
    const bool pressed = state.backPressed;
    state.backPressed = false;
    return pressed;
}

bool HasTouchInput() { return true; }
} // namespace PRS::Platform::Touch

#endif // PRS_PLATFORM_ANDROID

#pragma once
#include "PRS/Core/Platform.h"

#if defined(PRS_PLATFORM_ANDROID)

#include "PRS/Platform/Touch.h"
#include <android/native_window.h>
#include <android_native_app_glue.h>

namespace PRS::Android
{
struct State
{
    android_app* app = nullptr;
    ANativeWindow* window = nullptr;
    bool windowReady = false;
    bool resumed = false;
    bool closeRequested = false;
    bool backPressed = false;
    u32 width = 0;
    u32 height = 0;

    static constexpr u32 MaxTouchEvents = 64;
    Touch::Point touchEvents[MaxTouchEvents];
    u32 touchCount = 0;

    void PushTouch(i32 id, f32 x, f32 y, Touch::Phase phase)
    {
        if (touchCount >= MaxTouchEvents) return;
        touchEvents[touchCount++] = Touch::Point{id, x, y, phase};
    }
};

State& Get();
void SetPendingWindow(ANativeWindow* window);
ANativeWindow* ConsumePendingWindow();

void InstallCallbacks(android_app* app);
void PumpEvents();
} // namespace PRS::Android

#endif // PRS_PLATFORM_ANDROID

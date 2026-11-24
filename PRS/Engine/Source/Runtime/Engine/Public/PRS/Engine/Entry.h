#pragma once
#include "PRS/Core/Platform.h"
#include "PRS/Engine/Application.h"

// Include this in exactly one .cpp of your game, and define PRS::CreateApplication().
// On Android the entry point is android_main() in the platform layer, which calls
// CreateApplication() -> Run() for you.
#if !defined(PRS_PLATFORM_ANDROID)
int main()
{
    auto app = PRS::CreateApplication();
    return app->Run();
}
#endif

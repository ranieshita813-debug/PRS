#pragma once
#include "PRS/UI/Context.h"

namespace MobileShell
{
using PRS::f32;
using PRS::i32;

enum class Tab : i32
{
    Home = 0,
    Settings = 1,
    About = 2
};

// All UI state lives here so the screen can be driven by touch on a device or
// replayed by the offline preview tool.
struct State
{
    Tab tab = Tab::Home;

    bool shadows = true;
    bool bloom = true;
    bool haptics = true;
    f32 renderScale = 1.0f; // 0.5 .. 2.0
    f32 musicVolume = 0.6f;
    i32 selectedWorld = 0;

    char toast[96] = "READY";
    f32 toastTimer = 0.0f;

    void Notify(const char* message);
};

// Lays out and draws one frame of the interface into ui, using area as the
// whole surface (full-screen, in pixels).
void Draw(PRS::UI::Context& ui, const PRS::UI::Rect& area, State& state);
} // namespace MobileShell

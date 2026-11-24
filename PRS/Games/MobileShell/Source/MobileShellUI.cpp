#include "MobileShellUI.h"
#include "PRS/UI/Font.h"
#include <cstdio>

namespace MobileShell
{
using namespace PRS;
using namespace PRS::UI;

void State::Notify(const char* message)
{
    if (!message) return;
    std::snprintf(toast, sizeof(toast), "%s", message);
    toastTimer = 2.5f;
}

namespace
{
const char* const kTabLabels[] = {"HOME", "SETTINGS", "ABOUT"};

void DrawHeader(Context& ui, const Rect& header, const State& state)
{
    const Theme& theme = ui.GetTheme();
    const f32 pad = theme.PaddingPx();

    ui.Draws().AddRect(header, theme.backgroundAlt);

    const f32 titleSize = theme.Dp(20.0f);
    const Rect titleRect(header.x + pad, header.y + theme.Dp(18.0f),
                         header.w - pad * 2.0f - theme.Dp(72.0f),
                         BitmapFont::Default().LineHeight(titleSize));
    ui.Draws().AddTextAligned(titleRect, "PRS MOBILE", titleSize, theme.text, TextAlign::Left);

    // Build chip on the right of the title row.
    const f32 chipWidth = theme.Dp(64.0f);
    const f32 chipHeight = theme.Dp(26.0f);
    const Rect chip(header.Right() - pad - chipWidth, titleRect.y + theme.Dp(2.0f), chipWidth, chipHeight);
    ui.Draws().AddRoundedRect(chip, chipHeight * 0.5f, theme.primary.WithAlpha(0.18f));
    ui.Draws().AddTextAligned(chip, "ALPHA", theme.Dp(10.0f), theme.primary, TextAlign::Center);

    const f32 subtitleSize = theme.Dp(11.0f);
    const Rect subtitleRect(header.x + pad, titleRect.Bottom() + theme.Dp(2.0f), header.w - pad * 2.0f,
                            BitmapFont::Default().LineHeight(subtitleSize));
    ui.Draws().AddTextAligned(subtitleRect, "TOUCH SHELL / ANDROID", subtitleSize, theme.textDim,
                              TextAlign::Left);

    // Toast gets its own row under the subtitle so text never overlaps.
    if (state.toastTimer > 0.0f && state.toast[0])
    {
        const f32 toastSize = theme.Dp(11.0f);
        const f32 toastHeight = BitmapFont::Default().LineHeight(toastSize);
        const f32 toastY = subtitleRect.Bottom() + theme.Dp(4.0f);
        ui.Draws().AddRect({header.x + pad, toastY + theme.Dp(2.0f), theme.Dp(4.0f),
                            Max(0.0f, toastHeight - theme.Dp(4.0f))},
                           theme.success);
        const Rect toastRect(header.x + pad + theme.Dp(10.0f), toastY,
                             header.w - pad * 2.0f - theme.Dp(10.0f), toastHeight);
        ui.Draws().AddTextAligned(toastRect, state.toast, toastSize, theme.success, TextAlign::Left);
    }

    ui.Draws().AddRect({header.x, header.Bottom() - Max(1.0f, theme.Dp(1.0f)), header.w,
                        Max(1.0f, theme.Dp(1.0f))},
                       theme.border);
}

void DrawHome(Context& ui, State& state)
{
    ui.Heading("WORLD");
    ui.LabelDim("PICK UP WHERE YOU LEFT OFF");

    static const char* const kWorlds[] = {"NEO HARBOUR", "DUST VALLEY", "COLD RIDGE"};
    static const char* const kProgress[] = {"42% COMPLETE", "12% COMPLETE", "LOCKED"};

    ui.Card("continue", "CONTINUE", "NEO HARBOUR - 42% COMPLETE");

    for (i32 i = 0; i < 3; ++i)
    {
        char id[32];
        std::snprintf(id, sizeof(id), "world.%d", i);
        if (ui.ListRow(id, kWorlds[i], kProgress[i], state.selectedWorld == i))
        {
            state.selectedWorld = i;
            state.Notify(kWorlds[i]);
        }
    }

    ui.Spacer(4.0f);
    ui.Heading("SESSION");
    if (ui.Button("start", "START SESSION")) state.Notify("LAUNCHING WORLD");
    if (ui.Button("garage", "GARAGE")) state.Notify("GARAGE IS EMPTY");

    ui.Spacer(4.0f);
    ui.Heading("STREAMING");
    ui.LabelDim("WORLD CELLS BUDGET");
    ui.Progress(0.62f);
}

void DrawSettings(Context& ui, State& state)
{
    ui.Heading("GRAPHICS");
    ui.Toggle("shadows", "SHADOWS", state.shadows);
    ui.Toggle("bloom", "BLOOM", state.bloom);

    f32 scale = state.renderScale;
    if (ui.Slider("scale", "RENDER SCALE", scale, 0.5f, 2.0f))
    {
        state.renderScale = scale;
        char message[64];
        std::snprintf(message, sizeof(message), "RENDER SCALE %.2f", static_cast<double>(scale));
        state.Notify(message);
    }

    ui.Spacer(4.0f);
    ui.Heading("AUDIO");
    ui.Slider("music", "MUSIC", state.musicVolume, 0.0f, 1.0f);

    ui.Spacer(4.0f);
    ui.Heading("DEVICE");
    ui.Toggle("haptics", "HAPTICS", state.haptics);
    ui.LabelDim("48DP MINIMUM TOUCH TARGETS");
    ui.LabelDim("FRAME PACING: FIXED 60HZ TICK");

    if (ui.Button("reset", "RESET DEFAULTS"))
    {
        state.shadows = true;
        state.bloom = true;
        state.haptics = true;
        state.renderScale = 1.0f;
        state.musicVolume = 0.6f;
        state.Notify("DEFAULTS RESTORED");
    }
}

void DrawAbout(Context& ui, State& state)
{
    ui.Heading("ENGINE");
    ui.Label("PRS ENGINE 0.1");
    ui.LabelDim("C++20 / VULKAN-FIRST RHI");
    ui.LabelDim("MODULES: CORE PLATFORM JOBS");

    ui.Spacer(4.0f);
    ui.Heading("TARGET");
    ui.Label("ANDROID / ARM64");
    ui.LabelDim("UI LAYER: PRS::UI (TOUCH)");
    ui.LabelDim("TEXT: BUILT-IN 5X7 ATLAS");

    ui.Spacer(4.0f);
    ui.Heading("BENCHMARK");
    ui.LabelDim("GPU PATH NOT LINKED YET");
    ui.Progress(0.34f);

    if (ui.Button("benchmark", "RUN BENCHMARK")) state.Notify("BENCHMARK QUEUED");
    if (ui.Button("back", "BACK TO HOME"))
    {
        state.tab = Tab::Home;
        state.Notify("HOME");
    }
}
} // namespace

void Draw(Context& ui, const Rect& area, State& state)
{
    Theme& theme = ui.GetTheme();

    const f32 tabBarHeight = theme.Dp(68.0f);
    const f32 headerHeight = theme.Dp(112.0f);
    const Rect header(area.x, area.y, area.w, headerHeight);
    const Rect content(area.x, area.y + headerHeight, area.w, Max(0.0f, area.h - headerHeight - tabBarHeight));
    const Rect tabs(area.x, area.Bottom() - tabBarHeight, area.w, tabBarHeight);

    ui.Draws().AddRect(area, theme.background);
    DrawHeader(ui, header, state);

    ui.BeginView("content", content, true);
    switch (state.tab)
    {
    case Tab::Home: DrawHome(ui, state); break;
    case Tab::Settings: DrawSettings(ui, state); break;
    case Tab::About: DrawAbout(ui, state); break;
    }
    ui.EndView();

    const i32 clicked = ui.TabBar("tabs", tabs, kTabLabels, 3, static_cast<i32>(state.tab));
    if (clicked >= 0) state.tab = static_cast<Tab>(clicked);
}
} // namespace MobileShell

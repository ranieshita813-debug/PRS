#pragma once
#include "PRS/UI/DrawList.h"
#include "PRS/UI/Input.h"
#include "PRS/UI/Theme.h"
#include <vector>

namespace PRS::UI
{
// Immediate-mode, touch-first UI. Typical frame:
//
//   input.BeginFrame(dt);                 // drain touch events into input
//   ui.BeginFrame(screenSize, input, dt);
//   ui.BeginView("root", area, true);
//     if (ui.Button("play", "PLAY")) { ... }
//   ui.EndView();
//   ui.EndFrame();
//   canvas.Render(ui.Draws(), BitmapFont::Default());
//
// Widget ids are strings hashed internally; keep them stable between frames.
class Context
{
public:
    Context();

    void SetTheme(const Theme& theme) { m_theme = theme; }
    Theme& GetTheme() { return m_theme; }
    const Theme& GetTheme() const { return m_theme; }

    void BeginFrame(const Vec2& screenSize, const InputState& input, f32 dt);
    void EndFrame();

    const DrawList& Draws() const { return m_draws; }
    const Vec2& ScreenSize() const { return m_screenSize; }

    // --- Layout -------------------------------------------------------------
    void BeginView(const char* id, const Rect& area, bool scrollable = false);
    void EndView();
    f32 ScrollOffset(const char* id) const;
    void SetScrollOffset(const char* id, f32 offset);
    f32 ContentHeight(const char* id) const;
    f32 VisibleHeight(const char* id) const;

    // --- Content ------------------------------------------------------------
    void Spacer(f32 dp);
    void Separator();
    void Heading(const char* text);
    void Label(const char* text);
    void LabelDim(const char* text);
    void LabelRight(const char* text);
    void Progress(f32 t);

    bool Button(const char* id, const char* label);
    bool Card(const char* id, const char* title, const char* subtitle);
    bool ListRow(const char* id, const char* title, const char* trailing, bool selected);
    bool Toggle(const char* id, const char* label, bool& value);
    bool Slider(const char* id, const char* label, f32& value, f32 minValue, f32 maxValue);

    // Fixed-position bottom tab bar. Returns the newly selected index, or -1.
    i32 TabBar(const char* id, const Rect& area, const char* const* labels, u32 count, i32 selected);

    // --- Escape hatches -----------------------------------------------------
    DrawList& Draws() { return m_draws; }
    Rect Allocate(f32 height); // reserves a full-width row in the current view

private:
    struct WidgetState
    {
        bool pressed = false;
        bool hovered = false;
        f32 pressTime = 0.0f;
        f32 anim = 0.0f;
        f32 scroll = 0.0f;
        f32 contentHeight = 0.0f;
        f32 visibleHeight = 0.0f;
    };

    struct View
    {
        u32 id = 0;
        Rect area;
        f32 cursorY = 0.0f;
        f32 contentWidth = 0.0f;
        f32 padding = 0.0f;
        f32 gap = 0.0f;
        f32 scrollY = 0.0f;
        bool scrollable = false;
    };

    WidgetState& StateOf(u32 id);
    bool Interact(u32 id, const Rect& rect, WidgetState& state);
    View& CurrentView();
    void DrawRowBackground(const Rect& rect, const Color& base, f32 anim);

    Theme m_theme;
    DrawList m_draws;
    InputState m_input;
    Vec2 m_screenSize;
    f32 m_dt = 0.0f;
    bool m_frameActive = false;
    bool m_scrollGesture = false;
    f32 m_scrollAnchor = 0.0f;
    f32 m_scrollStart = 0.0f;

    std::vector<View> m_views;
    std::vector<WidgetState> m_states;
    std::vector<u32> m_stateIds;
};
} // namespace PRS::UI

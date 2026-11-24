#include "PRS/UI/Context.h"
#include "PRS/UI/Font.h"
#include <cstdio>

namespace PRS::UI
{
namespace
{
u32 HashId(const char* text)
{
    u32 hash = 2166136261u;
    for (const char* p = text; p && *p; ++p)
    {
        hash ^= static_cast<u32>(static_cast<u8>(*p));
        hash *= 16777619u;
    }
    return hash;
}
} // namespace

Context::Context() : m_theme(Theme::PhoneDark())
{
    m_draws.Reset();
}

void Context::BeginFrame(const Vec2& screenSize, const InputState& input, f32 dt)
{
    m_screenSize = screenSize;
    m_input = input;
    m_dt = dt > 0.0f ? dt : 1.0f / 60.0f;
    m_frameActive = true;
    m_scrollGesture = false;
    m_views.clear();
    m_draws.Reset();
}

void Context::EndFrame()
{
    m_frameActive = false;
    m_scrollGesture = false;
}

Context::WidgetState& Context::StateOf(u32 id)
{
    for (size_t i = 0; i < m_stateIds.size(); ++i)
    {
        if (m_stateIds[i] == id) return m_states[i];
    }
    m_stateIds.push_back(id);
    m_states.push_back({});
    return m_states.back();
}

Context::View& Context::CurrentView()
{
    static View fallback;
    if (m_views.empty())
    {
        fallback = View{};
        fallback.contentWidth = m_screenSize.x;
        return fallback;
    }
    return m_views.back();
}

void Context::BeginView(const char* id, const Rect& area, bool scrollable)
{
    View view;
    view.id = HashId(id);
    view.area = area;
    view.scrollable = scrollable;
    view.padding = m_theme.PaddingPx();
    view.gap = m_theme.GapPx();

    WidgetState& state = StateOf(view.id);
    state.visibleHeight = area.h;

    if (scrollable)
    {
        state.scroll -= m_input.ScrollY();

        const PointerState* pointer = m_input.Primary();
        const bool startedInside = pointer && area.Contains(pointer->startPos);

        if (pointer && pointer->down && startedInside)
        {
            if (!m_scrollGesture && Length(pointer->pos - pointer->startPos) > m_theme.Dp(10.0f))
            {
                m_scrollGesture = true;
                m_scrollAnchor = pointer->startPos.y;
                m_scrollStart = state.scroll;
            }
            if (m_scrollGesture) state.scroll = m_scrollStart - (pointer->pos.y - m_scrollAnchor);
        }
        else
        {
            m_scrollGesture = false;
        }

        const f32 maxScroll = Max(0.0f, state.contentHeight + view.padding * 2.0f - area.h);
        state.scroll = Clamp(state.scroll, 0.0f, maxScroll);
    }

    view.scrollY = scrollable ? state.scroll : 0.0f;
    view.cursorY = area.y + view.padding - view.scrollY;
    view.contentWidth = area.w - view.padding * 2.0f;

    m_views.push_back(view);
    m_draws.PushClip(area);
}

void Context::EndView()
{
    if (m_views.empty()) return;

    const View view = m_views.back();
    m_views.pop_back();
    m_draws.PopClip();

    f32 contentHeight = (view.cursorY - view.gap) - (view.area.y + view.padding - view.scrollY);
    if (contentHeight < 0.0f) contentHeight = 0.0f;

    WidgetState& state = StateOf(view.id);
    state.contentHeight = contentHeight;
    state.visibleHeight = view.area.h;
    if (view.scrollable)
    {
        state.scroll = Clamp(state.scroll, 0.0f,
                             Max(0.0f, contentHeight + view.padding * 2.0f - view.area.h));
    }
}

f32 Context::ScrollOffset(const char* id) const
{
    const u32 key = HashId(id);
    for (size_t i = 0; i < m_stateIds.size(); ++i)
    {
        if (m_stateIds[i] == key) return m_states[i].scroll;
    }
    return 0.0f;
}

void Context::SetScrollOffset(const char* id, f32 offset)
{
    WidgetState& state = StateOf(HashId(id));
    state.scroll = Max(0.0f, offset);
}

f32 Context::ContentHeight(const char* id) const
{
    const u32 key = HashId(id);
    for (size_t i = 0; i < m_stateIds.size(); ++i)
    {
        if (m_stateIds[i] == key) return m_states[i].contentHeight;
    }
    return 0.0f;
}

f32 Context::VisibleHeight(const char* id) const
{
    const u32 key = HashId(id);
    for (size_t i = 0; i < m_stateIds.size(); ++i)
    {
        if (m_stateIds[i] == key) return m_states[i].visibleHeight;
    }
    return 0.0f;
}

Rect Context::Allocate(f32 height)
{
    View& view = CurrentView();
    const Rect row(view.area.x + view.padding, view.cursorY, view.contentWidth, height);
    view.cursorY += height + view.gap;
    return row;
}

bool Context::Interact(u32 id, const Rect& rect, WidgetState& state)
{
    const PointerState* pointer = m_input.Primary();
    const bool inside = pointer && rect.Contains(pointer->pos);
    bool activated = false;

    if (pointer && pointer->pressed && inside && !m_scrollGesture)
    {
        state.pressed = true;
        state.pressTime = 0.0f;
    }

    if (state.pressed)
    {
        state.pressTime += m_dt;

        // A drag means the user is scrolling or swiping, not tapping the widget.
        if (m_scrollGesture) state.pressed = false;
        if (pointer && pointer->down && Length(pointer->pos - pointer->startPos) > m_theme.Dp(12.0f))
        {
            state.pressed = false;
        }

        if (pointer && pointer->released)
        {
            activated = rect.Contains(pointer->pos);
            state.pressed = false;
        }
        else if (!pointer || (!pointer->down && !pointer->released))
        {
            state.pressed = false;
        }
    }

    state.hovered = inside;
    const f32 target = state.pressed ? 1.0f : (inside ? 0.35f : 0.0f);
    state.anim = Lerp(state.anim, target, Min(1.0f, m_dt * 16.0f));

    (void)id;
    return activated;
}

void Context::Spacer(f32 dp)
{
    const f32 height = m_theme.Dp(dp);
    if (height <= 0.0f) return;
    View& view = CurrentView();
    view.cursorY += height;
}

void Context::Separator()
{
    const Rect row = Allocate(0.0f);
    const f32 thickness = Max(1.0f, m_theme.Dp(m_theme.hairline));
    m_draws.AddRect({row.x, row.y, row.w, thickness}, m_theme.border);
}

void Context::Heading(const char* text)
{
    const f32 size = m_theme.HeadingPx();
    const Rect row = Allocate(BitmapFont::Default().LineHeight(size));
    m_draws.AddTextAligned(row, text, size, m_theme.text, TextAlign::Left);
}

void Context::Label(const char* text)
{
    const f32 size = m_theme.FontPx();
    const Rect row = Allocate(BitmapFont::Default().LineHeight(size));
    m_draws.AddTextAligned(row, text, size, m_theme.text, TextAlign::Left);
}

void Context::LabelDim(const char* text)
{
    const f32 size = m_theme.FontPx() * 0.9f;
    const Rect row = Allocate(BitmapFont::Default().LineHeight(size));
    m_draws.AddTextAligned(row, text, size, m_theme.textDim, TextAlign::Left);
}

void Context::LabelRight(const char* text)
{
    const f32 size = m_theme.FontPx();
    const Rect row = Allocate(BitmapFont::Default().LineHeight(size));
    m_draws.AddTextAligned(row, text, size, m_theme.textDim, TextAlign::Right);
}

void Context::Progress(f32 t)
{
    const Rect row = Allocate(m_theme.Dp(8.0f));
    const f32 radius = row.h * 0.5f;
    m_draws.AddRoundedRect(row, radius, m_theme.surface);

    const f32 fillWidth = row.w * Clamp(t, 0.0f, 1.0f);
    if (fillWidth > 1.0f)
    {
        m_draws.AddRoundedRect({row.x, row.y, fillWidth, row.h}, radius, m_theme.primary);
    }
}

bool Context::Button(const char* id, const char* label)
{
    const u32 key = HashId(id);
    WidgetState& state = StateOf(key);

    const Rect row = Allocate(m_theme.TouchTargetPx());
    const bool activated = Interact(key, row, state);

    const Color fill = m_theme.primary.Lerp(m_theme.primaryPressed, state.anim);
    m_draws.AddRoundedRect(row, m_theme.RadiusPx(), fill);
    m_draws.AddTextAligned(row, label, m_theme.FontPx() * 1.15f, m_theme.onPrimary, TextAlign::Center);
    return activated;
}

bool Context::Card(const char* id, const char* title, const char* subtitle)
{
    const u32 key = HashId(id);
    WidgetState& state = StateOf(key);

    const Rect row = Allocate(Max(m_theme.TouchTargetPx(), m_theme.Dp(88.0f)));
    const bool activated = Interact(key, row, state);

    const Color fill = m_theme.surface.Lerp(m_theme.surfacePressed, state.anim);
    m_draws.AddRoundedRect(row, m_theme.RadiusPx(), fill);
    m_draws.AddRectOutline(row, m_theme.RadiusPx(), Max(1.0f, m_theme.Dp(1.0f)), m_theme.border);

    const f32 pad = m_theme.Dp(16.0f);
    const Rect inner = row.Inset(pad);
    const f32 titleSize = m_theme.FontPx() * 1.15f;
    const f32 subtitleSize = m_theme.FontPx() * 0.9f;
    const f32 titleHeight = BitmapFont::Default().LineHeight(titleSize);

    m_draws.AddTextAligned({inner.x, inner.y, inner.w, titleHeight}, title, titleSize, m_theme.text,
                           TextAlign::Left);

    if (subtitle && *subtitle)
    {
        const f32 y = inner.y + titleHeight + m_theme.Dp(2.0f);
        m_draws.AddTextAligned({inner.x, y, inner.w, Max(0.0f, inner.Bottom() - y)}, subtitle,
                               subtitleSize, m_theme.textDim, TextAlign::Left);
    }
    return activated;
}

bool Context::ListRow(const char* id, const char* title, const char* trailing, bool selected)
{
    const u32 key = HashId(id);
    WidgetState& state = StateOf(key);

    const Rect row = Allocate(Max(m_theme.TouchTargetPx(), m_theme.Dp(60.0f)));
    const bool activated = Interact(key, row, state);

    Color fill = m_theme.surface.Lerp(m_theme.surfacePressed, state.anim);
    if (selected) fill = fill.Lerp(m_theme.primary, 0.22f);
    m_draws.AddRoundedRect(row, m_theme.RadiusPx(), fill);

    const Rect inner = row.InsetX(m_theme.Dp(16.0f));
    const f32 size = m_theme.FontPx();
    m_draws.AddTextAligned(inner, title, size, m_theme.text, TextAlign::Left);
    if (trailing && *trailing)
    {
        m_draws.AddTextAligned(inner, trailing, size * 0.95f, m_theme.textDim, TextAlign::Right);
    }
    if (selected)
    {
        const f32 barWidth = Max(2.0f, m_theme.Dp(3.0f));
        m_draws.AddRoundedRect({row.x, row.y + row.h * 0.2f, barWidth, row.h * 0.6f}, barWidth * 0.5f,
                               m_theme.primary);
    }
    return activated;
}

bool Context::Toggle(const char* id, const char* label, bool& value)
{
    const u32 key = HashId(id);
    WidgetState& state = StateOf(key);

    const Rect row = Allocate(m_theme.TouchTargetPx());
    const bool activated = Interact(key, row, state);
    if (activated) value = !value;

    const f32 switchWidth = m_theme.Dp(52.0f);
    const f32 switchHeight = m_theme.Dp(30.0f);
    const Rect labelRect(row.x, row.y, Max(0.0f, row.w - switchWidth - m_theme.Dp(12.0f)), row.h);
    m_draws.AddTextAligned(labelRect, label, m_theme.FontPx(), m_theme.text, TextAlign::Left);

    const Rect track(row.Right() - switchWidth, row.Center().y - switchHeight * 0.5f, switchWidth,
                     switchHeight);
    m_draws.AddRoundedRect(track, switchHeight * 0.5f, value ? m_theme.primary : m_theme.surface);
    if (!value)
    {
        m_draws.AddRectOutline(track, switchHeight * 0.5f, Max(1.0f, m_theme.Dp(1.0f)), m_theme.border);
    }

    const f32 knobSize = switchHeight - m_theme.Dp(6.0f);
    const f32 knobX = value ? track.Right() - knobSize - m_theme.Dp(3.0f) : track.x + m_theme.Dp(3.0f);
    m_draws.AddRoundedRect({knobX, track.y + m_theme.Dp(3.0f), knobSize, knobSize}, knobSize * 0.5f,
                           value ? m_theme.onPrimary : m_theme.textDim);
    return activated;
}

bool Context::Slider(const char* id, const char* label, f32& value, f32 minValue, f32 maxValue)
{
    const u32 key = HashId(id);
    WidgetState& state = StateOf(key);

    const f32 labelSize = m_theme.FontPx();
    const f32 labelHeight = BitmapFont::Default().LineHeight(labelSize);
    const f32 trackHeight = m_theme.Dp(10.0f);
    const Rect row = Allocate(Max(m_theme.TouchTargetPx(), labelHeight + m_theme.Dp(34.0f)));

    const Rect labelRect(row.x, row.y, row.w, labelHeight);
    const Rect track(row.x, row.y + labelHeight + m_theme.Dp(14.0f), row.w, trackHeight);
    const Rect hitArea(row.x, track.y - m_theme.Dp(16.0f), row.w, trackHeight + m_theme.Dp(32.0f));

    const PointerState* pointer = m_input.Primary();
    bool changed = false;

    if (pointer && pointer->pressed && hitArea.Contains(pointer->pos) && !m_scrollGesture)
    {
        state.pressed = true;
    }

    if (state.pressed)
    {
        if (m_scrollGesture) state.pressed = false;
        if (pointer && pointer->down)
        {
            const f32 t = Clamp((pointer->pos.x - track.x) / Max(1.0f, track.w), 0.0f, 1.0f);
            const f32 next = Lerp(minValue, maxValue, t);
            if (next != value)
            {
                value = next;
                changed = true;
            }
        }
        if (!pointer || pointer->released) state.pressed = false;
    }

    const f32 t = Clamp((value - minValue) / Max(1.0f, maxValue - minValue), 0.0f, 1.0f);

    m_draws.AddTextAligned(labelRect, label, labelSize, m_theme.text, TextAlign::Left);

    // Fractional ranges (volume, opacity) read as percentages; absolute ranges
    // (scale factors) read as the value itself.
    char buffer[32];
    if (minValue >= 0.0f && maxValue <= 1.0f)
    {
        std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(value * 100.0f + 0.5f));
    }
    else
    {
        std::snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(value));
    }
    m_draws.AddTextAligned(labelRect, buffer, labelSize * 0.95f, m_theme.textDim, TextAlign::Right);

    const f32 trackRadius = trackHeight * 0.5f;
    m_draws.AddRoundedRect(track, trackRadius, m_theme.surface);
    const f32 fillWidth = track.w * t;
    if (fillWidth > 1.0f)
    {
        m_draws.AddRoundedRect({track.x, track.y, fillWidth, track.h}, trackRadius, m_theme.primary);
    }

    const f32 knobSize = m_theme.Dp(28.0f);
    const f32 knobX = track.x + fillWidth - knobSize * 0.5f;
    m_draws.AddRoundedRect({knobX, track.Center().y - knobSize * 0.5f, knobSize, knobSize},
                           knobSize * 0.5f, state.pressed ? m_theme.onPrimary : m_theme.text);
    return changed;
}

i32 Context::TabBar(const char* id, const Rect& area, const char* const* labels, u32 count, i32 selected)
{
    if (count == 0 || area.Empty()) return -1;

    i32 clicked = -1;
    m_draws.AddRect(area, m_theme.backgroundAlt);
    m_draws.AddRect({area.x, area.y, area.w, Max(1.0f, m_theme.Dp(m_theme.hairline))}, m_theme.border);

    const f32 tabWidth = area.w / static_cast<f32>(count);
    for (u32 i = 0; i < count; ++i)
    {
        char key[160];
        std::snprintf(key, sizeof(key), "%s.tab.%u", id, i);
        const u32 hash = HashId(key);
        WidgetState& state = StateOf(hash);
        const Rect tab(area.x + tabWidth * static_cast<f32>(i), area.y, tabWidth, area.h);

        if (Interact(hash, tab, state) && static_cast<i32>(i) != selected) clicked = static_cast<i32>(i);

        const Color color = (static_cast<i32>(i) == selected)
                                ? m_theme.primary
                                : m_theme.textDim.Lerp(m_theme.text, state.anim);
        m_draws.AddTextAligned(tab, labels[i], m_theme.FontPx() * 0.9f, color, TextAlign::Center);

        if (static_cast<i32>(i) == selected)
        {
            const f32 indicatorWidth = tabWidth * 0.4f;
            m_draws.AddRoundedRect({tab.x + (tabWidth - indicatorWidth) * 0.5f,
                                    tab.Bottom() - m_theme.Dp(5.0f), indicatorWidth,
                                    Max(2.0f, m_theme.Dp(3.0f))},
                                   m_theme.Dp(2.0f), m_theme.primary);
        }
    }
    return clicked;
}
} // namespace PRS::UI

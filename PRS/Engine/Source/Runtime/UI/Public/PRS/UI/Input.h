#pragma once
#include "PRS/UI/Types.h"
#include <vector>

namespace PRS::UI
{
// One finger (or, on desktop, the mouse as a single pointer).
struct PointerState
{
    i32 id = 0;
    Vec2 pos;
    Vec2 prevPos;
    Vec2 startPos;
    bool down = false;
    bool pressed = false;  // went down this frame
    bool released = false; // went up this frame
    f32 downTime = 0.0f;
};

// Feed events between BeginFrame() and the UI's BeginFrame(): platform code calls
// OnPointerDown/Move/Up as touch (or mouse) events arrive, and AddScroll for
// wheel / two-finger scrolling.
class InputState
{
public:
    // Call once per frame before feeding events: clears edge flags and ages timers.
    void BeginFrame(f32 dt);

    void OnPointerDown(i32 id, const Vec2& pos);
    void OnPointerMove(i32 id, const Vec2& pos);
    void OnPointerUp(i32 id, const Vec2& pos);

    void AddScroll(f32 dy) { m_scrollY += dy; }

    f32 ScrollY() const { return m_scrollY; }

    // The pointer that is currently down (lowest id), otherwise the pointer that
    // was released this frame, otherwise nullptr.
    const PointerState* Primary() const;
    const PointerState* Find(i32 id) const;
    bool AnyDown() const;

    u32 PointerCount() const { return static_cast<u32>(m_pointers.size()); }

private:
    std::vector<PointerState> m_pointers;
    i32 m_lastReleasedId = -1;
    f32 m_scrollY = 0.0f;
};
} // namespace PRS::UI

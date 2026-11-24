#include "PRS/UI/Input.h"

namespace PRS::UI
{
void InputState::BeginFrame(f32 dt)
{
    m_scrollY = 0.0f;

    for (size_t i = 0; i < m_pointers.size();)
    {
        PointerState& p = m_pointers[i];
        if (p.released)
        {
            m_pointers.erase(m_pointers.begin() + static_cast<long>(i));
            continue;
        }
        p.pressed = false;
        p.released = false;
        p.prevPos = p.pos;
        if (p.down) p.downTime += dt;
        ++i;
    }
}

void InputState::OnPointerDown(i32 id, const Vec2& pos)
{
    for (PointerState& p : m_pointers)
    {
        if (p.id == id)
        {
            p.pos = pos;
            p.startPos = pos;
            p.down = true;
            p.pressed = true;
            p.downTime = 0.0f;
            return;
        }
    }

    PointerState p;
    p.id = id;
    p.pos = pos;
    p.prevPos = pos;
    p.startPos = pos;
    p.down = true;
    p.pressed = true;
    m_pointers.push_back(p);
}

void InputState::OnPointerMove(i32 id, const Vec2& pos)
{
    for (PointerState& p : m_pointers)
    {
        if (p.id == id)
        {
            p.pos = pos;
            return;
        }
    }
}

void InputState::OnPointerUp(i32 id, const Vec2& pos)
{
    for (PointerState& p : m_pointers)
    {
        if (p.id == id)
        {
            p.pos = pos;
            p.down = false;
            p.released = true;
            return;
        }
    }
}

const PointerState* InputState::Primary() const
{
    for (const PointerState& p : m_pointers)
    {
        if (p.down) return &p;
    }
    for (const PointerState& p : m_pointers)
    {
        if (p.released) return &p;
    }
    return nullptr;
}

const PointerState* InputState::Find(i32 id) const
{
    for (const PointerState& p : m_pointers)
    {
        if (p.id == id) return &p;
    }
    return nullptr;
}

bool InputState::AnyDown() const
{
    for (const PointerState& p : m_pointers)
    {
        if (p.down) return true;
    }
    return false;
}
} // namespace PRS::UI

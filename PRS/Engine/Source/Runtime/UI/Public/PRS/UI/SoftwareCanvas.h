#pragma once
#include "PRS/UI/DrawList.h"
#include <vector>

namespace PRS::UI
{
// Straightforward CPU rasterizer for the draw list. It exists so the interface
// can be shown on Android through ANativeWindow_lock() while the Vulkan RHI is
// still a stub, and so UI screenshots can be produced headlessly.
//
// Quads are axis-aligned by construction in this module, which lets the
// rasterizer work on rects instead of arbitrary triangles.
class SoftwareCanvas
{
public:
    void Resize(u32 width, u32 height);
    u32 Width() const { return m_width; }
    u32 Height() const { return m_height; }
    u32 Stride() const { return m_width * 4u; }
    const u8* Pixels() const { return m_pixels.empty() ? nullptr : m_pixels.data(); }
    u8* Pixels() { return m_pixels.empty() ? nullptr : m_pixels.data(); }

    void Clear(const Color& color);
    void Render(const DrawList& list, const BitmapFont& font);

private:
    void BlendPixel(i32 x, i32 y, const Color& color);
    void FillRoundedRect(const Rect& rect, f32 radius, const Color& color, const Rect& clip);
    void FillTextQuad(const Rect& rect, const Vec2& uv0, const Vec2& uv1, const Color& color,
                      const Rect& clip, const BitmapFont& font);

    std::vector<u8> m_pixels;
    u32 m_width = 0;
    u32 m_height = 0;
};
} // namespace PRS::UI

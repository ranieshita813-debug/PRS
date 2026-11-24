#include "PRS/UI/SoftwareCanvas.h"
#include <cmath>

namespace PRS::UI
{
namespace
{
u8 ToByte(f32 v)
{
    f32 scaled = v * 255.0f + 0.5f;
    if (scaled < 0.0f) scaled = 0.0f;
    if (scaled > 255.0f) scaled = 255.0f;
    return static_cast<u8>(scaled);
}
} // namespace

void SoftwareCanvas::Resize(u32 width, u32 height)
{
    if (width == m_width && height == m_height && !m_pixels.empty()) return;
    m_width = width;
    m_height = height;
    m_pixels.assign(static_cast<size_t>(width) * height * 4u, 0);
}

void SoftwareCanvas::Clear(const Color& color)
{
    if (m_pixels.empty()) return;
    const u8 r = ToByte(color.r);
    const u8 g = ToByte(color.g);
    const u8 b = ToByte(color.b);
    const u8 a = ToByte(color.a);
    for (size_t i = 0; i + 3 < m_pixels.size(); i += 4)
    {
        m_pixels[i + 0] = r;
        m_pixels[i + 1] = g;
        m_pixels[i + 2] = b;
        m_pixels[i + 3] = a;
    }
}

void SoftwareCanvas::BlendPixel(i32 x, i32 y, const Color& color)
{
    if (x < 0 || y < 0 || x >= static_cast<i32>(m_width) || y >= static_cast<i32>(m_height)) return;
    if (color.a <= 0.0f || m_pixels.empty()) return;

    const f32 a = Clamp(color.a, 0.0f, 1.0f);
    const size_t index = (static_cast<size_t>(y) * m_width + static_cast<size_t>(x)) * 4u;
    u8* pixel = &m_pixels[index];

    const f32 dr = static_cast<f32>(pixel[0]) / 255.0f;
    const f32 dg = static_cast<f32>(pixel[1]) / 255.0f;
    const f32 db = static_cast<f32>(pixel[2]) / 255.0f;

    pixel[0] = ToByte(color.r * a + dr * (1.0f - a));
    pixel[1] = ToByte(color.g * a + dg * (1.0f - a));
    pixel[2] = ToByte(color.b * a + db * (1.0f - a));
    pixel[3] = 255;
}

void SoftwareCanvas::FillRoundedRect(const Rect& rect, f32 radius, const Color& color, const Rect& clip)
{
    Rect target = rect.ClipTo(clip).ClipTo(Rect(0.0f, 0.0f, static_cast<f32>(m_width),
                                                static_cast<f32>(m_height)));
    if (target.Empty()) return;

    const f32 radiusClamped = Min(radius, Min(rect.w, rect.h) * 0.5f);
    const Vec2 half(rect.w * 0.5f, rect.h * 0.5f);
    const Vec2 center = rect.Center();

    const i32 x0 = static_cast<i32>(std::floor(target.x));
    const i32 y0 = static_cast<i32>(std::floor(target.y));
    const i32 x1 = static_cast<i32>(std::ceil(target.Right()));
    const i32 y1 = static_cast<i32>(std::ceil(target.Bottom()));

    for (i32 y = y0; y < y1; ++y)
    {
        for (i32 x = x0; x < x1; ++x)
        {
            const Vec2 p(static_cast<f32>(x) + 0.5f, static_cast<f32>(y) + 0.5f);
            f32 coverage = 1.0f;

            if (radiusClamped > 0.0f)
            {
                // Signed distance to a rounded box, used as an analytic AA mask.
                const Vec2 q(Abs(p.x - center.x) - (half.x - radiusClamped),
                             Abs(p.y - center.y) - (half.y - radiusClamped));
                const Vec2 outside(Max(q.x, 0.0f), Max(q.y, 0.0f));
                const f32 distance = Length(outside) + Min(Max(q.x, q.y), 0.0f) - radiusClamped;
                coverage = Clamp(0.5f - distance, 0.0f, 1.0f);
            }

            if (coverage > 0.0f) BlendPixel(x, y, color.WithAlpha(color.a * coverage));
        }
    }
}

void SoftwareCanvas::FillTextQuad(const Rect& rect, const Vec2& uv0, const Vec2& uv1,
                                  const Color& color, const Rect& clip, const BitmapFont& font)
{
    const u8* atlas = font.AtlasPixels();
    if (!atlas || rect.w <= 0.0f || rect.h <= 0.0f) return;

    Rect target = rect.ClipTo(clip).ClipTo(Rect(0.0f, 0.0f, static_cast<f32>(m_width),
                                                static_cast<f32>(m_height)));
    if (target.Empty()) return;

    const f32 atlasW = static_cast<f32>(font.AtlasWidth());
    const f32 atlasH = static_cast<f32>(font.AtlasHeight());

    const i32 x0 = static_cast<i32>(std::floor(target.x));
    const i32 y0 = static_cast<i32>(std::floor(target.y));
    const i32 x1 = static_cast<i32>(std::ceil(target.Right()));
    const i32 y1 = static_cast<i32>(std::ceil(target.Bottom()));

    for (i32 y = y0; y < y1; ++y)
    {
        const f32 fy = (static_cast<f32>(y) + 0.5f - rect.y) / rect.h;
        const f32 v = Lerp(uv0.y, uv1.y, Clamp(fy, 0.0f, 1.0f));
        for (i32 x = x0; x < x1; ++x)
        {
            const f32 fx = (static_cast<f32>(x) + 0.5f - rect.x) / rect.w;
            const f32 u = Lerp(uv0.x, uv1.x, Clamp(fx, 0.0f, 1.0f));

            const u32 ax = static_cast<u32>(Clamp(u * atlasW, 0.0f, atlasW - 1.0f));
            const u32 ay = static_cast<u32>(Clamp(v * atlasH, 0.0f, atlasH - 1.0f));
            const f32 alpha = static_cast<f32>(atlas[static_cast<size_t>(ay) * font.AtlasWidth() + ax]) /
                              255.0f;
            if (alpha > 0.0f) BlendPixel(x, y, color.WithAlpha(color.a * alpha));
        }
    }
}

void SoftwareCanvas::Render(const DrawList& list, const BitmapFont& font)
{
    if (m_pixels.empty()) return;

    const Rect fullScreen(0.0f, 0.0f, static_cast<f32>(m_width), static_cast<f32>(m_height));
    const auto& vertices = list.Vertices();
    const auto& indices = list.Indices();

    for (const DrawCmd& cmd : list.Commands())
    {
        const Rect clip = cmd.clip.Empty() ? fullScreen : cmd.clip;
        for (u32 i = 0; i + 5 < cmd.indexCount; i += 6)
        {
            const u32 base = cmd.indexOffset + i;
            const Vertex& v0 = vertices[indices[base + 0]];
            const Vertex& v1 = vertices[indices[base + 1]];
            const Vertex& v2 = vertices[indices[base + 2]];

            const Rect quad(v0.pos.x, v0.pos.y, v1.pos.x - v0.pos.x, v2.pos.y - v0.pos.y);

            if (cmd.kind == CmdKind::Shape)
            {
                FillRoundedRect(quad, cmd.radius, v0.color, clip);
            }
            else
            {
                FillTextQuad(quad, v0.uv, v2.uv, v0.color, clip, font);
            }
        }
    }
}
} // namespace PRS::UI

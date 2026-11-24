#include "PRS/UI/DrawList.h"

namespace PRS::UI
{
namespace
{
constexpr f32 kLargeCoord = 1.0e6f;
}

void DrawList::Reset()
{
    m_vertices.clear();
    m_indices.clear();
    m_commands.clear();
    m_clipStack.clear();
    m_clip = Rect(-kLargeCoord, -kLargeCoord, kLargeCoord * 2.0f, kLargeCoord * 2.0f);
}

Rect DrawList::CurrentClip() const { return m_clip; }

void DrawList::PushClip(const Rect& rect)
{
    m_clipStack.push_back(m_clip);
    m_clip = m_clip.ClipTo(rect);
}

void DrawList::PopClip()
{
    if (m_clipStack.empty()) return;
    m_clip = m_clipStack.back();
    m_clipStack.pop_back();
}

DrawCmd& DrawList::PrepareCommand(CmdKind kind, f32 radius, u32 texture)
{
    if (!m_commands.empty())
    {
        DrawCmd& last = m_commands.back();
        if (last.kind == kind && last.radius == radius && last.texture == texture &&
            last.clip.x == m_clip.x && last.clip.y == m_clip.y && last.clip.w == m_clip.w &&
            last.clip.h == m_clip.h)
        {
            return last;
        }
    }

    DrawCmd cmd;
    cmd.kind = kind;
    cmd.indexOffset = static_cast<u32>(m_indices.size());
    cmd.indexCount = 0;
    cmd.clip = m_clip;
    cmd.radius = radius;
    cmd.texture = texture;
    m_commands.push_back(cmd);
    return m_commands.back();
}

void DrawList::PushQuad(const Rect& rect, const Vec2& uv0, const Vec2& uv1, const Color& color)
{
    if (rect.w <= 0.0f || rect.h <= 0.0f) return;

    const u32 base = static_cast<u32>(m_vertices.size());
    m_vertices.push_back({{rect.x, rect.y}, {uv0.x, uv0.y}, color});
    m_vertices.push_back({{rect.Right(), rect.y}, {uv1.x, uv0.y}, color});
    m_vertices.push_back({{rect.Right(), rect.Bottom()}, {uv1.x, uv1.y}, color});
    m_vertices.push_back({{rect.x, rect.Bottom()}, {uv0.x, uv1.y}, color});

    m_indices.push_back(base + 0);
    m_indices.push_back(base + 1);
    m_indices.push_back(base + 2);
    m_indices.push_back(base + 0);
    m_indices.push_back(base + 2);
    m_indices.push_back(base + 3);

    m_commands.back().indexCount += 6;
}

void DrawList::AddRect(const Rect& rect, const Color& color) { AddRoundedRect(rect, 0.0f, color); }

void DrawList::AddRoundedRect(const Rect& rect, f32 radius, const Color& color)
{
    if (rect.Empty() || color.a <= 0.0f) return;
    PrepareCommand(CmdKind::Shape, radius, 0);
    PushQuad(rect, {0.0f, 0.0f}, {1.0f, 1.0f}, color);
}

void DrawList::AddRectOutline(const Rect& rect, f32 radius, f32 thickness, const Color& color)
{
    (void)radius;
    if (rect.Empty() || thickness <= 0.0f) return;
    f32 t = thickness;
    AddRect({rect.x, rect.y, rect.w, t}, color);
    AddRect({rect.x, rect.Bottom() - t, rect.w, t}, color);
    AddRect({rect.x, rect.y + t, t, rect.h - t * 2.0f}, color);
    AddRect({rect.Right() - t, rect.y + t, t, rect.h - t * 2.0f}, color);
}

void DrawList::AddLine(const Vec2& a, const Vec2& b, f32 thickness, const Color& color)
{
    if (Abs(b.x - a.x) >= Abs(b.y - a.y))
    {
        const f32 x = Min(a.x, b.x);
        AddRect({x, a.y - thickness * 0.5f, Abs(b.x - a.x), thickness}, color);
    }
    else
    {
        const f32 y = Min(a.y, b.y);
        AddRect({a.x - thickness * 0.5f, y, thickness, Abs(b.y - a.y)}, color);
    }
}

void DrawList::AddText(const char* text, const Vec2& topLeft, f32 pixelSize, const Color& color)
{
    if (!text || !*text || color.a <= 0.0f) return;

    const BitmapFont& font = BitmapFont::Default();
    const f32 scale = BitmapFont::ScaleForPixelSize(pixelSize);
    const f32 atlasW = static_cast<f32>(font.AtlasWidth());
    const f32 atlasH = static_cast<f32>(font.AtlasHeight());

    PrepareCommand(CmdKind::Text, 0.0f, 1);

    f32 penX = topLeft.x;
    for (const char* p = text; *p; ++p)
    {
        const Glyph* glyph = font.Find(*p);
        if (!glyph)
        {
            penX += 6.0f * scale;
            continue;
        }

        if (glyph->width > 0)
        {
            const Rect quad(penX, topLeft.y, static_cast<f32>(glyph->width) * scale,
                            static_cast<f32>(glyph->height) * scale);
            const Vec2 uv0(static_cast<f32>(glyph->atlasX) / atlasW,
                           static_cast<f32>(glyph->atlasY) / atlasH);
            const Vec2 uv1(static_cast<f32>(glyph->atlasX + glyph->width) / atlasW,
                           static_cast<f32>(glyph->atlasY + glyph->height) / atlasH);
            PushQuad(quad, uv0, uv1, color);
        }

        penX += glyph->advance * scale;
    }
}

void DrawList::AddTextAligned(const Rect& box, const char* text, f32 pixelSize, const Color& color,
                              TextAlign align)
{
    if (!text || !*text) return;

    const BitmapFont& font = BitmapFont::Default();
    const f32 scale = BitmapFont::ScaleForPixelSize(pixelSize);
    const f32 textWidth = font.Measure(text, pixelSize);
    const f32 textHeight = 7.0f * scale;

    f32 x = box.x;
    if (align == TextAlign::Center) x = box.x + (box.w - textWidth) * 0.5f;
    else if (align == TextAlign::Right) x = box.Right() - textWidth;

    const f32 y = box.y + (box.h - textHeight) * 0.5f;
    AddText(text, {x, y}, pixelSize, color);
}
} // namespace PRS::UI

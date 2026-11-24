#pragma once
#include "PRS/UI/Font.h"
#include "PRS/UI/Types.h"
#include <vector>

namespace PRS::UI
{
struct Vertex
{
    Vec2 pos;
    Vec2 uv; // Shape: normalised local coords (0..1). Text: normalised atlas UV.
    Color color;
};

using DrawIndex = u32;

enum class CmdKind : u8
{
    Shape, // axis-aligned rounded quad, corner radius in DrawCmd::radius
    Text   // textured glyph quads sampling the font atlas
};

struct DrawCmd
{
    CmdKind kind = CmdKind::Shape;
    u32 indexOffset = 0;
    u32 indexCount = 0;
    Rect clip;
    f32 radius = 0.0f;
    u32 texture = 0; // 0 = none, 1 = font atlas
};

// Records one frame of UI geometry. Backends (software canvas now, Vulkan later)
// consume Vertices()/Indices()/Commands(). Consecutive primitives that share a
// clip rect, radius and texture are merged into a single command.
class DrawList
{
public:
    void Reset();

    const std::vector<Vertex>& Vertices() const { return m_vertices; }
    const std::vector<DrawIndex>& Indices() const { return m_indices; }
    const std::vector<DrawCmd>& Commands() const { return m_commands; }

    void PushClip(const Rect& rect);
    void PopClip();
    Rect CurrentClip() const;

    void AddRect(const Rect& rect, const Color& color);
    void AddRoundedRect(const Rect& rect, f32 radius, const Color& color);
    void AddRectOutline(const Rect& rect, f32 radius, f32 thickness, const Color& color);
    void AddLine(const Vec2& a, const Vec2& b, f32 thickness, const Color& color);

    void AddText(const char* text, const Vec2& topLeft, f32 pixelSize, const Color& color);
    void AddTextAligned(const Rect& box, const char* text, f32 pixelSize, const Color& color,
                        TextAlign align = TextAlign::Left);

private:
    DrawCmd& PrepareCommand(CmdKind kind, f32 radius, u32 texture);
    void PushQuad(const Rect& rect, const Vec2& uv0, const Vec2& uv1, const Color& color);

    std::vector<Vertex> m_vertices;
    std::vector<DrawIndex> m_indices;
    std::vector<DrawCmd> m_commands;
    std::vector<Rect> m_clipStack;
    Rect m_clip;
};
} // namespace PRS::UI

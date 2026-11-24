#pragma once
#include "PRS/UI/Types.h"
#include <vector>

namespace PRS::UI
{
// A glyph in the font atlas. Atlas coordinates are in texels; convert to
// normalised UVs when producing vertices so GPU and software backends agree.
struct Glyph
{
    char code = ' ';
    u32 atlasX = 0;
    u32 atlasY = 0;
    u32 width = 0;
    u32 height = 0;
    f32 advance = 0.0f; // in font units (1 unit = 1 texel at the glyph's design size)
};

// Built-in 5x7 bitmap font (uppercase + digits + punctuation).
// ASCII is mapped to uppercase; lowercase input is accepted.
// The atlas is a single-channel (8-bit coverage) image, so it works as a
// R8 texture on mobile GPUs and as a plain alpha mask in the software canvas.
class BitmapFont
{
public:
    static const BitmapFont& Default();

    f32 DesignHeight() const { return 7.0f; }
    f32 LineAdvance() const { return 9.0f; }
    u32 AtlasWidth() const { return m_atlasWidth; }
    u32 AtlasHeight() const { return m_atlasHeight; }
    const u8* AtlasPixels() const { return m_atlas.empty() ? nullptr : m_atlas.data(); }

    // Empty glyph (space) is still valid; returns nullptr for unknown characters.
    const Glyph* Find(char c) const;

    // Integer scale keeps the bitmap crisp on high-density screens.
    static f32 ScaleForPixelSize(f32 pixelSize);

    // Width of text in pixels for a given glyph height in pixels.
    f32 Measure(const char* text, f32 pixelSize) const;
    f32 MeasureChar(char c, f32 pixelSize) const;
    f32 LineHeight(f32 pixelSize) const;

private:
    BitmapFont();

    std::vector<Glyph> m_glyphs;
    std::vector<u8> m_atlas;
    u32 m_atlasWidth = 0;
    u32 m_atlasHeight = 0;
};
} // namespace PRS::UI

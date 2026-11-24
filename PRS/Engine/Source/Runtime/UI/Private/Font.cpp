#include "PRS/UI/Font.h"

namespace PRS::UI
{
namespace
{
struct GlyphArt
{
    char code;
    const char* rows[7];
};

// 5x7 bitmap glyphs, readable in source so they are easy to tweak.
const GlyphArt kGlyphs[] = {
    {' ', {".....", ".....", ".....", ".....", ".....", ".....", "....."}},
    {'A', {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'B', {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."}},
    {'C', {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."}},
    {'D', {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."}},
    {'E', {"#####", "#....", "#....", "####.", "#....", "#....", "#####"}},
    {'F', {"#####", "#....", "#....", "####.", "#....", "#....", "#...."}},
    {'G', {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###."}},
    {'H', {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"}},
    {'I', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "#####"}},
    {'J', {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."}},
    {'K', {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"}},
    {'L', {"#....", "#....", "#....", "#....", "#....", "#....", "#####"}},
    {'M', {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"}},
    {'N', {"#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"}},
    {'O', {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'P', {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."}},
    {'Q', {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"}},
    {'R', {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"}},
    {'S', {".####", "#....", "#....", ".###.", "....#", "....#", "####."}},
    {'T', {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."}},
    {'U', {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."}},
    {'V', {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."}},
    {'W', {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#"}},
    {'X', {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"}},
    {'Y', {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."}},
    {'Z', {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"}},
    {'0', {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."}},
    {'1', {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."}},
    {'2', {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"}},
    {'3', {"####.", "....#", "....#", ".###.", "....#", "....#", "####."}},
    {'4', {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."}},
    {'5', {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."}},
    {'6', {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."}},
    {'7', {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."}},
    {'8', {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."}},
    {'9', {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."}},
    {'.', {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."}},
    {',', {".....", ".....", ".....", ".....", ".##..", ".##..", ".#..."}},
    {':', {".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."}},
    {'-', {".....", ".....", ".....", ".###.", ".....", ".....", "....."}},
    {'+', {".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."}},
    {'/', {"....#", "....#", "...#.", "..#..", ".#...", "#....", "#...."}},
    {'%', {"##..#", "##.#.", "...#.", "..#..", ".#...", "#.##.", "#.##."}},
    {'!', {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."}},
    {'?', {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."}},
    {'(', {"...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#."}},
    {')', {".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..."}},
    {'=', {".....", ".....", "#####", ".....", "#####", ".....", "....."}},
    {'_', {".....", ".....", ".....", ".....", ".....", ".....", "#####"}},
    {'"', {".#.#.", ".#.#.", ".....", ".....", ".....", ".....", "....."}},
    {'>', {".#...", "..#..", "...#.", "....#", "...#.", "..#..", ".#..."}},
};

constexpr u32 kGlyphWidth = 5;
constexpr u32 kGlyphHeight = 7;
constexpr u32 kCellWidth = 6;  // 1px padding to avoid bleeding between glyphs
constexpr u32 kCellHeight = 8;
constexpr u32 kColumns = 16;

constexpr u32 kGlyphCount = sizeof(kGlyphs) / sizeof(kGlyphs[0]);
constexpr u32 kAtlasRows = (kGlyphCount + kColumns - 1) / kColumns;
} // namespace

BitmapFont::BitmapFont()
{
    m_atlasWidth = kColumns * kCellWidth;
    m_atlasHeight = kAtlasRows * kCellHeight;
    m_atlas.assign(static_cast<size_t>(m_atlasWidth) * m_atlasHeight, 0);
    m_glyphs.reserve(kGlyphCount);

    for (u32 i = 0; i < kGlyphCount; ++i)
    {
        Glyph glyph;
        glyph.code = kGlyphs[i].code;
        glyph.atlasX = (i % kColumns) * kCellWidth;
        glyph.atlasY = (i / kColumns) * kCellHeight;
        glyph.width = kGlyphWidth;
        glyph.height = kGlyphHeight;
        glyph.advance = static_cast<f32>(kGlyphWidth + 1);

        if (glyph.code == ' ')
        {
            glyph.width = 0;
            glyph.advance = 4.0f;
        }

        for (u32 y = 0; y < kGlyphHeight; ++y)
        {
            const char* row = kGlyphs[i].rows[y];
            for (u32 x = 0; x < kGlyphWidth; ++x)
            {
                if (row[x] == '#')
                {
                    const u32 px = glyph.atlasX + x;
                    const u32 py = glyph.atlasY + y;
                    m_atlas[static_cast<size_t>(py) * m_atlasWidth + px] = 255;
                }
            }
        }

        m_glyphs.push_back(glyph);
    }
}

const BitmapFont& BitmapFont::Default()
{
    static const BitmapFont font;
    return font;
}

const Glyph* BitmapFont::Find(char c) const
{
    char code = c;
    if (code >= 'a' && code <= 'z') code = static_cast<char>(code - 'a' + 'A');
    for (const Glyph& glyph : m_glyphs)
    {
        if (glyph.code == code) return &glyph;
    }
    return nullptr;
}

f32 BitmapFont::ScaleForPixelSize(f32 pixelSize)
{
    const f32 wanted = pixelSize / 7.0f;
    f32 scale = static_cast<f32>(static_cast<i32>(wanted + 0.5f));
    if (scale < 1.0f) scale = 1.0f;
    return scale;
}

f32 BitmapFont::MeasureChar(char c, f32 pixelSize) const
{
    const f32 scale = ScaleForPixelSize(pixelSize);
    const Glyph* glyph = Find(c);
    return (glyph ? glyph->advance : 6.0f) * scale;
}

f32 BitmapFont::Measure(const char* text, f32 pixelSize) const
{
    if (!text) return 0.0f;
    const f32 scale = ScaleForPixelSize(pixelSize);
    f32 width = 0.0f;
    for (const char* p = text; *p; ++p) width += MeasureChar(*p, pixelSize);
    if (width > 0.0f) width -= scale; // trailing glyph spacing
    return width;
}

f32 BitmapFont::LineHeight(f32 pixelSize) const
{
    return LineAdvance() * ScaleForPixelSize(pixelSize);
}
} // namespace PRS::UI

#pragma once
#include "PRS/Assets/Image.h"

namespace PRS::Assets
{
// Decoders for the uncompressed formats that need no third-party library.
// Both return false and fill `error` on malformed input; nothing partially
// decoded is left in `out`.
bool LoadTGA(ConstSpan<u8> data, Image& out, String& error);
bool LoadBMP(ConstSpan<u8> data, Image& out, String& error);

// Picks the decoder from the file extension (".tga", ".bmp"). TGA has no magic
// number, so it cannot be sniffed from the bytes alone.
bool LoadImage(ConstSpan<u8> data, StringView sourceName, Image& out, String& error);

// Reads the file and decodes it. `path` is only used for the extension and the
// Image::source label.
bool LoadImageFile(const String& path, Image& out, String& error);
} // namespace PRS::Assets

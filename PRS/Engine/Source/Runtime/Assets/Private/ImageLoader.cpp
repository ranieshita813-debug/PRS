#include "PRS/Assets/ImageLoader.h"
#include <cstring>
#include <fstream>

namespace PRS::Assets
{
namespace
{
u16 ReadU16(const u8* p) { return static_cast<u16>(p[0] | (p[1] << 8)); }
u32 ReadU32(const u8* p)
{
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) |
           (static_cast<u32>(p[2]) << 16) | (static_cast<u32>(p[3]) << 24);
}

String LowerExtension(StringView name)
{
    const size_t dot = name.find_last_of('.');
    if (dot == StringView::npos) return {};
    String ext(name.substr(dot + 1));
    for (char& c : ext)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return ext;
}

// TGA rows are stored bottom-up unless the descriptor says otherwise.
bool BottomUp(const u8* header) { return (header[17] & 0x20u) == 0; }

void WriteTexel(Image& image, u32 x, u32 y, u8 r, u8 g, u8 b, u8 a, bool flipY)
{
    const u32 row = flipY ? image.height - 1u - y : y;
    u8* dst = image.pixels.data() + (static_cast<size_t>(row) * image.width + x) * 4u;
    dst[0] = r;
    dst[1] = g;
    dst[2] = b;
    dst[3] = a;
}
} // namespace

bool LoadTGA(ConstSpan<u8> data, Image& out, String& error)
{
    if (data.size() < 18)
    {
        error = "TGA: truncated header";
        return false;
    }

    const u8* header = data.data();
    const u8 idLength = header[0];
    const u8 colorMapType = header[1];
    const u8 imageType = header[2];

    const bool uncompressed = imageType == 2 || imageType == 3;
    const bool rle = imageType == 10 || imageType == 11;
    const bool grayscale = imageType == 3 || imageType == 11;
    if (!uncompressed && !rle)
    {
        error = "TGA: only true-colour/grayscale, uncompressed or RLE images are supported";
        return false;
    }
    if (colorMapType != 0)
    {
        error = "TGA: colour-mapped images are not supported";
        return false;
    }

    const u32 width = ReadU16(header + 12);
    const u32 height = ReadU16(header + 14);
    const u32 bitsPerPixel = header[16];
    const u32 bytesPerPixel = bitsPerPixel / 8u;
    const u32 expectedBpp = grayscale ? 8u : bytesPerPixel;
    if (width == 0 || height == 0 || (bytesPerPixel != 3 && bytesPerPixel != 4 && !grayscale) ||
        (grayscale && expectedBpp != 1))
    {
        error = "TGA: unsupported dimensions or bit depth";
        return false;
    }

    const size_t pixelCount = static_cast<size_t>(width) * height;
    const size_t pixelBytes = pixelCount * bytesPerPixel;
    size_t offset = 18u + idLength;
    if (offset > data.size())
    {
        error = "TGA: truncated image id";
        return false;
    }

    Array<u8> raw;
    raw.resize(pixelBytes);
    size_t written = 0;

    if (uncompressed)
    {
        if (offset + pixelBytes > data.size())
        {
            error = "TGA: truncated pixel data";
            return false;
        }
        std::memcpy(raw.data(), data.data() + offset, pixelBytes);
    }
    else
    {
        while (written < pixelBytes)
        {
            if (offset >= data.size())
            {
                error = "TGA: truncated RLE packet";
                return false;
            }
            const u8 packet = data[offset++];
            const size_t count = static_cast<size_t>(packet & 0x7Fu) + 1u;
            if ((packet & 0x80u) != 0)
            {
                if (offset + bytesPerPixel > data.size() || written + count * bytesPerPixel > pixelBytes)
                {
                    error = "TGA: RLE run overruns the image";
                    return false;
                }
                const u8* texel = data.data() + offset;
                offset += bytesPerPixel;
                for (size_t i = 0; i < count; ++i)
                {
                    std::memcpy(raw.data() + written, texel, bytesPerPixel);
                    written += bytesPerPixel;
                }
            }
            else
            {
                const size_t bytes = count * bytesPerPixel;
                if (offset + bytes > data.size() || written + bytes > pixelBytes)
                {
                    error = "TGA: RLE packet overruns the image";
                    return false;
                }
                std::memcpy(raw.data() + written, data.data() + offset, bytes);
                offset += bytes;
                written += bytes;
            }
        }
    }

    Image image;
    image.width = width;
    image.height = height;
    image.pixels.resize(pixelCount * 4u);
    const bool flip = BottomUp(header);

    for (u32 y = 0; y < height; ++y)
    {
        for (u32 x = 0; x < width; ++x)
        {
            const size_t index = (static_cast<size_t>(y) * width + x) * bytesPerPixel;
            if (grayscale)
            {
                const u8 v = raw[index];
                WriteTexel(image, x, y, v, v, v, 255, flip);
            }
            else
            {
                const u8 b = raw[index + 0];
                const u8 g = raw[index + 1];
                const u8 r = raw[index + 2];
                const u8 a = bytesPerPixel == 4 ? raw[index + 3] : 255;
                WriteTexel(image, x, y, r, g, b, a, flip);
            }
        }
    }

    out = std::move(image);
    return true;
}

bool LoadBMP(ConstSpan<u8> data, Image& out, String& error)
{
    if (data.size() < 54 || data[0] != 'B' || data[1] != 'M')
    {
        error = "BMP: missing BM signature";
        return false;
    }

    const u32 pixelOffset = ReadU32(data.data() + 10);
    const u32 dibSize = ReadU32(data.data() + 14);
    if (dibSize < 40)
    {
        error = "BMP: unsupported header size";
        return false;
    }

    const i32 signedWidth = static_cast<i32>(ReadU32(data.data() + 18));
    const i32 signedHeight = static_cast<i32>(ReadU32(data.data() + 22));
    const u32 bitsPerPixel = ReadU16(data.data() + 28);
    const u32 compression = ReadU32(data.data() + 30);
    if (compression != 0)
    {
        error = "BMP: only uncompressed images are supported";
        return false;
    }
    if (bitsPerPixel != 24 && bitsPerPixel != 32)
    {
        error = "BMP: only 24/32-bit images are supported";
        return false;
    }
    if (signedWidth <= 0 || signedHeight == 0)
    {
        error = "BMP: invalid dimensions";
        return false;
    }

    const bool topDown = signedHeight < 0;
    const u32 width = static_cast<u32>(signedWidth);
    const u32 height = static_cast<u32>(topDown ? -signedHeight : signedHeight);
    const u32 bytesPerPixel = bitsPerPixel / 8u;
    const u32 stride = ((width * bitsPerPixel + 31u) / 32u) * 4u;

    if (pixelOffset + static_cast<u64>(stride) * height > data.size())
    {
        error = "BMP: truncated pixel data";
        return false;
    }

    Image image;
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<size_t>(width) * height * 4u);

    for (u32 y = 0; y < height; ++y)
    {
        const u8* row = data.data() + pixelOffset + static_cast<size_t>(y) * stride;
        // BMP rows are bottom-up unless the height is negative.
        const u32 targetRow = topDown ? y : height - 1u - y;
        for (u32 x = 0; x < width; ++x)
        {
            const u8 b = row[x * bytesPerPixel + 0];
            const u8 g = row[x * bytesPerPixel + 1];
            const u8 r = row[x * bytesPerPixel + 2];
            const u8 a = bytesPerPixel == 4 ? row[x * bytesPerPixel + 3] : 255;
            u8* dst = image.pixels.data() + (static_cast<size_t>(targetRow) * width + x) * 4u;
            dst[0] = r;
            dst[1] = g;
            dst[2] = b;
            dst[3] = a;
        }
    }

    out = std::move(image);
    return true;
}

bool LoadImage(ConstSpan<u8> data, StringView sourceName, Image& out, String& error)
{
    const String ext = LowerExtension(sourceName);
    bool ok = false;
    if (ext == "tga") ok = LoadTGA(data, out, error);
    else if (ext == "bmp") ok = LoadBMP(data, out, error);
    else
    {
        error = "Image: unsupported extension '" + ext + "' (supported: .tga, .bmp)";
        return false;
    }
    if (ok) out.source = String(sourceName);
    return ok;
}

bool LoadImageFile(const String& path, Image& out, String& error)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.good())
    {
        error = "Image: cannot open '" + path + "'";
        return false;
    }
    const std::streamoff size = file.tellg();
    if (size <= 0)
    {
        error = "Image: empty file '" + path + "'";
        return false;
    }
    file.seekg(0, std::ios::beg);
    Array<u8> bytes(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size))
    {
        error = "Image: failed to read '" + path + "'";
        return false;
    }
    return LoadImage(ConstSpan<u8>(bytes.data(), bytes.size()), path, out, error);
}
} // namespace PRS::Assets

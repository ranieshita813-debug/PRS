#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace MobileShell
{
namespace PngDetail
{
using u8 = std::uint8_t;
using u32 = std::uint32_t;

inline u32 Crc32(const u8* data, size_t length)
{
    static u32 table[256];
    static bool ready = false;
    if (!ready)
    {
        for (u32 i = 0; i < 256; ++i)
        {
            u32 c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }

    u32 crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

inline u32 Adler32(const u8* data, size_t length)
{
    u32 a = 1;
    u32 b = 0;
    for (size_t i = 0; i < length; ++i)
    {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

inline void PutU32(std::vector<u8>& out, u32 value)
{
    out.push_back(static_cast<u8>((value >> 24) & 0xFF));
    out.push_back(static_cast<u8>((value >> 16) & 0xFF));
    out.push_back(static_cast<u8>((value >> 8) & 0xFF));
    out.push_back(static_cast<u8>(value & 0xFF));
}

inline void WriteChunk(std::vector<u8>& out, const char* type, const std::vector<u8>& payload)
{
    PutU32(out, static_cast<u32>(payload.size()));

    std::vector<u8> crcInput;
    crcInput.reserve(payload.size() + 4);
    crcInput.insert(crcInput.end(), type, type + 4);
    crcInput.insert(crcInput.end(), payload.begin(), payload.end());

    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), payload.begin(), payload.end());
    PutU32(out, Crc32(crcInput.data(), crcInput.size()));
}
} // namespace PngDetail

// Writes 8-bit RGBA pixels as a PNG. Uses stored (uncompressed) deflate blocks so
// the tool has no zlib dependency.
inline bool WritePng(const char* path, const std::uint8_t* rgba, std::uint32_t width,
                     std::uint32_t height)
{
    using namespace PngDetail;
    if (!rgba || width == 0 || height == 0) return false;

    std::vector<u8> raw;
    raw.reserve((static_cast<size_t>(width) * 4u + 1u) * height);
    for (u32 y = 0; y < height; ++y)
    {
        raw.push_back(0); // filter: none
        const u8* row = rgba + static_cast<size_t>(y) * width * 4u;
        raw.insert(raw.end(), row, row + static_cast<size_t>(width) * 4u);
    }

    std::vector<u8> zlib;
    zlib.push_back(0x78);
    zlib.push_back(0x01);

    size_t offset = 0;
    while (offset < raw.size())
    {
        const size_t chunk = std::min<size_t>(65535, raw.size() - offset);
        const bool last = offset + chunk >= raw.size();
        const std::uint16_t length = static_cast<std::uint16_t>(chunk);
        const std::uint16_t complement = static_cast<std::uint16_t>(~length);

        zlib.push_back(last ? 1 : 0);
        zlib.push_back(static_cast<u8>(length & 0xFF));
        zlib.push_back(static_cast<u8>((length >> 8) & 0xFF));
        zlib.push_back(static_cast<u8>(complement & 0xFF));
        zlib.push_back(static_cast<u8>((complement >> 8) & 0xFF));
        zlib.insert(zlib.end(), raw.begin() + static_cast<long>(offset),
                    raw.begin() + static_cast<long>(offset + chunk));
        offset += chunk;
    }
    PutU32(zlib, Adler32(raw.data(), raw.size()));

    std::vector<u8> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    std::vector<u8> header;
    PutU32(header, width);
    PutU32(header, height);
    header.push_back(8); // bit depth
    header.push_back(6); // colour type: RGBA
    header.push_back(0); // compression
    header.push_back(0); // filter
    header.push_back(0); // interlace

    WriteChunk(png, "IHDR", header);
    WriteChunk(png, "IDAT", zlib);
    WriteChunk(png, "IEND", {});

    std::FILE* file = std::fopen(path, "wb");
    if (!file) return false;
    const size_t written = std::fwrite(png.data(), 1, png.size(), file);
    std::fclose(file);
    return written == png.size();
}
} // namespace MobileShell

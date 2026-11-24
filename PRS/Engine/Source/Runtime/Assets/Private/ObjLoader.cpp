#include "PRS/Assets/ObjLoader.h"
#include "PRS/Core/Log.h"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace PRS::Assets
{
namespace
{
constexpr f32 kDefaultNormalFallback = 0.0f;

bool ReadFileBytes(const String& path, Array<u8>& out)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.good()) return false;
    const std::streamoff size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    return static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()), size));
}

// Splits a face vertex token ("1", "1/2", "1//3", "1/2/3") into 1-based indices.
struct FaceRef
{
    i32 position = 0;
    i32 uv = 0;
    i32 normal = 0;
};

bool ParseFaceRef(StringView token, FaceRef& out)
{
    const char* cursor = token.data();
    const char* end = token.data() + token.size();
    i32 values[3] = {0, 0, 0};
    for (int slot = 0; slot < 3 && cursor <= end; ++slot)
    {
        const char* slash = cursor;
        while (slash < end && *slash != '/') ++slash;
        if (slash > cursor)
        {
            i32 value = 0;
            const auto result = std::from_chars(cursor, slash, value);
            if (result.ec != std::errc()) return false;
            values[slot] = value;
        }
        cursor = slash + 1;
        if (slash == end) break;
    }
    out.position = values[0];
    out.uv = values[1];
    out.normal = values[2];
    return out.position != 0;
}

// OBJ indices are 1-based; negative values are relative to the count so far.
i32 ResolveIndex(i32 value, size_t count)
{
    if (value > 0) return value - 1;
    if (value < 0) return static_cast<i32>(count) + value;
    return -1;
}

struct PartBuilder
{
    MeshPart part;
    HashMap<u64, u32> lookup;
};

u64 MakeKey(i32 position, i32 uv, i32 normal)
{
    return (static_cast<u64>(static_cast<u32>(position)) << 42) ^
           (static_cast<u64>(static_cast<u32>(uv)) << 21) ^ static_cast<u64>(static_cast<u32>(normal));
}

void ComputeSmoothNormals(MeshPart& part)
{
    for (MeshVertex& vertex : part.vertices) vertex.normal = Vec3{};
    for (size_t i = 0; i + 2 < part.indices.size(); i += 3)
    {
        MeshVertex& a = part.vertices[part.indices[i + 0]];
        MeshVertex& b = part.vertices[part.indices[i + 1]];
        MeshVertex& c = part.vertices[part.indices[i + 2]];
        const Vec3 faceNormal = Cross(b.position - a.position, c.position - a.position);
        a.normal += faceNormal;
        b.normal += faceNormal;
        c.normal += faceNormal;
    }
    for (MeshVertex& vertex : part.vertices)
        vertex.normal = LengthSquared(vertex.normal) > Epsilon ? Normalize(vertex.normal)
                                                              : Vec3{0.0f, 1.0f, 0.0f};
}
} // namespace

bool LoadMTL(ConstSpan<u8> text, HashMap<String, MaterialData>& out, String& error)
{
    (void)error;
    std::istringstream stream(String(text.begin(), text.end()));
    String line;
    MaterialData current;
    bool haveMaterial = false;

    auto flush = [&]() {
        if (!haveMaterial) return;
        out[current.name] = current;
    };

    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::istringstream tokens(line);
        String keyword;
        tokens >> keyword;
        if (keyword.empty()) continue;

        if (keyword == "newmtl")
        {
            flush();
            current = MaterialData{};
            tokens >> current.name;
            if (current.name.empty()) current.name = "material";
            haveMaterial = true;
        }
        else if (!haveMaterial)
        {
            continue;
        }
        else if (keyword == "Kd")
        {
            f32 r = 1.0f, g = 1.0f, b = 1.0f;
            tokens >> r >> g >> b;
            current.baseColor = Vec4(r, g, b, current.baseColor.w);
        }
        else if (keyword == "Ks")
        {
            f32 r = 0.0f, g = 0.0f, b = 0.0f;
            tokens >> r >> g >> b;
            current.specularColor = Vec3(r, g, b);
        }
        else if (keyword == "Ns")
        {
            f32 shininess = 32.0f;
            tokens >> shininess;
            current.shininess = shininess > 0.0f ? shininess : 1.0f;
        }
        else if (keyword == "d" || keyword == "Tr")
        {
            f32 value = 1.0f;
            tokens >> value;
            if (keyword == "Tr") value = 1.0f - value;
            current.baseColor.w = value;
        }
        else if (keyword == "map_Kd")
        {
            String path;
            tokens >> path;
            if (!path.empty()) current.baseColorTexture = path;
        }
    }
    flush();
    return true;
}

bool LoadOBJ(ConstSpan<u8> text, StringView name, MeshData& out, String& error)
{
    Array<Vec3> positions;
    Array<Vec2> uvs;
    Array<Vec3> normals;
    Array<String> materialLibraries;

    HashMap<String, MaterialData> materials;
    HashMap<String, PartBuilder> builders;
    Array<String> order; // part order, for deterministic output
    String currentMaterial = "default";

    std::istringstream stream(String(text.begin(), text.end()));
    String line;

    auto builderFor = [&](const String& material) -> PartBuilder& {
        auto it = builders.find(material);
        if (it != builders.end()) return it->second;
        PartBuilder builder;
        builder.part.material.name = material;
        order.push_back(material);
        return builders.emplace(material, std::move(builder)).first->second;
    };

    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::istringstream tokens(line);
        String keyword;
        tokens >> keyword;
        if (keyword.empty()) continue;

        if (keyword == "v")
        {
            Vec3 p;
            tokens >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (keyword == "vt")
        {
            Vec2 uv;
            tokens >> uv.x >> uv.y;
            uvs.push_back(uv);
        }
        else if (keyword == "vn")
        {
            Vec3 n;
            tokens >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (keyword == "usemtl")
        {
            tokens >> currentMaterial;
            if (currentMaterial.empty()) currentMaterial = "default";
        }
        else if (keyword == "mtllib")
        {
            String library;
            while (tokens >> library) materialLibraries.push_back(library);
        }
        else if (keyword == "f")
        {
            Array<FaceRef> face;
            String token;
            while (tokens >> token)
            {
                FaceRef ref;
                if (!ParseFaceRef(token, ref)) continue;
                ref.position = ResolveIndex(ref.position, positions.size());
                ref.uv = ResolveIndex(ref.uv, uvs.size());
                ref.normal = ResolveIndex(ref.normal, normals.size());
                if (ref.position < 0 || ref.position >= static_cast<i32>(positions.size())) continue;
                if (ref.uv >= static_cast<i32>(uvs.size())) ref.uv = -1;
                if (ref.normal >= static_cast<i32>(normals.size())) ref.normal = -1;
                face.push_back(ref);
            }
            if (face.size() < 3) continue;

            PartBuilder& builder = builderFor(currentMaterial);
            Array<u32> fan;
            for (const FaceRef& ref : face)
            {
                const u64 key = MakeKey(ref.position, ref.uv, ref.normal);
                auto found = builder.lookup.find(key);
                if (found != builder.lookup.end())
                {
                    fan.push_back(found->second);
                    continue;
                }
                MeshVertex vertex;
                vertex.position = positions[static_cast<size_t>(ref.position)];
                if (ref.uv >= 0) vertex.uv = uvs[static_cast<size_t>(ref.uv)];
                vertex.normal = ref.normal >= 0 ? normals[static_cast<size_t>(ref.normal)]
                                                : Vec3{kDefaultNormalFallback, kDefaultNormalFallback,
                                                       kDefaultNormalFallback};
                const u32 index = static_cast<u32>(builder.part.vertices.size());
                builder.part.vertices.push_back(vertex);
                builder.lookup.emplace(key, index);
                fan.push_back(index);
            }
            for (size_t i = 1; i + 1 < fan.size(); ++i)
            {
                builder.part.indices.push_back(fan[0]);
                builder.part.indices.push_back(fan[i]);
                builder.part.indices.push_back(fan[i + 1]);
            }
        }
    }
    (void)materialLibraries;

    MeshData mesh;
    mesh.name = String(name);
    for (const String& materialName : order)
    {
        PartBuilder& builder = builders[materialName];
        if (builder.part.indices.empty()) continue;
        const auto found = materials.find(materialName);
        if (found != materials.end()) builder.part.material = found->second;
        // A generated normal of exactly (0,0,0) means the file had none.
        bool needsNormals = false;
        for (const MeshVertex& vertex : builder.part.vertices)
            if (LengthSquared(vertex.normal) < Epsilon) needsNormals = true;
        if (needsNormals) ComputeSmoothNormals(builder.part);
        mesh.parts.push_back(std::move(builder.part));
    }

    if (!mesh.IsValid())
    {
        error = "OBJ: '" + String(name) + "' contains no drawable faces";
        return false;
    }
    out = std::move(mesh);
    return true;
}

bool LoadMeshFile(const String& path, MeshData& out, String& error)
{
    Array<u8> bytes;
    if (!ReadFileBytes(path, bytes))
    {
        error = "OBJ: cannot open '" + path + "'";
        return false;
    }

    const std::filesystem::path filePath(path);
    const std::filesystem::path directory = filePath.parent_path();

    // Collect mtllib names first so materials exist while parts are built.
    String text(bytes.begin(), bytes.end());
    HashMap<String, MaterialData> materials;
    {
        std::istringstream stream(text);
        String line;
        while (std::getline(stream, line))
        {
            if (line.rfind("mtllib ", 0) != 0) continue;
            std::istringstream tokens(line.substr(7));
            String library;
            while (tokens >> library)
            {
                const std::filesystem::path mtlPath = directory / library;
                Array<u8> mtlBytes;
                if (!ReadFileBytes(mtlPath.string(), mtlBytes))
                {
                    PRS_WARN("OBJ: material library '%s' not found", mtlPath.string().c_str());
                    continue;
                }
                HashMap<String, MaterialData> parsed;
                String mtlError;
                if (!LoadMTL(ConstSpan<u8>(mtlBytes.data(), mtlBytes.size()), parsed, mtlError))
                {
                    PRS_WARN("OBJ: %s", mtlError.c_str());
                    continue;
                }
                for (auto& [name, material] : parsed)
                {
                    if (!material.baseColorTexture.empty())
                    {
                        // Texture paths in MTL are relative to the MTL file.
                        const std::filesystem::path texture =
                            (directory / material.baseColorTexture).lexically_normal();
                        material.baseColorTexture = texture.generic_string();
                    }
                    materials[name] = material;
                }
            }
        }
    }

    if (!LoadOBJ(ConstSpan<u8>(bytes.data(), bytes.size()), path, out, error)) return false;

    for (MeshPart& part : out.parts)
    {
        const auto found = materials.find(part.material.name);
        if (found != materials.end()) part.material = found->second;
    }
    return true;
}
} // namespace PRS::Assets

#include "PRS/Assets/AssetManager.h"
#include "PRS/Assets/ImageLoader.h"
#include "PRS/Assets/ObjLoader.h"
#include "PRS/Core/Log.h"
#include <filesystem>
#include <fstream>

namespace PRS::Assets
{
namespace fs = std::filesystem;

void AssetManager::AddSearchPath(String path)
{
    if (path.empty()) return;
    m_searchPaths.push_back(std::move(path));
}

void AssetManager::ClearSearchPaths() { m_searchPaths.clear(); }

bool AssetManager::ReadFile(const String& path, Array<u8>& out)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.good()) return false;
    const std::streamoff size = file.tellg();
    if (size < 0) return false;
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (size == 0) return true;
    return static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()), size));
}

Optional<String> AssetManager::Resolve(StringView relative) const
{
    if (relative.empty()) return Optional<String>();

    const fs::path requested{String(relative)};
    std::error_code ec;
    if (requested.is_absolute())
    {
        if (fs::exists(requested, ec) && !ec) return Optional<String>(requested.string());
        return Optional<String>();
    }

    for (const String& root : m_searchPaths)
    {
        fs::path candidate = (fs::path(root) / requested).lexically_normal();
        ec.clear();
        if (fs::exists(candidate, ec) && !ec) return Optional<String>(candidate.string());
    }
    return Optional<String>();
}

bool AssetManager::Exists(StringView relative) const { return Resolve(relative).has_value(); }

const MeshData* AssetManager::LoadMesh(StringView relative)
{
    const String key(relative);
    const auto cached = m_meshes.find(key);
    if (cached != m_meshes.end()) return &cached->second;

    const Optional<String> resolved = Resolve(relative);
    if (!resolved)
    {
        if (m_missing.insert(key).second)
            PRS_WARN("Assets: mesh '%s' not found in %zu search path(s)", key.c_str(),
                     m_searchPaths.size());
        return nullptr;
    }

    MeshData mesh;
    String error;
    if (!LoadMeshFile(*resolved, mesh, error))
    {
        PRS_ERROR("Assets: %s", error.c_str());
        return nullptr;
    }

    PRS_INFO("Assets: loaded mesh '%s' (%u parts, %u triangles)", key.c_str(),
             static_cast<u32>(mesh.parts.size()), mesh.TriangleCount());
    return &m_meshes.emplace(key, std::move(mesh)).first->second;
}

const Image* AssetManager::LoadImage(StringView relative)
{
    const String key(relative);
    const auto cached = m_images.find(key);
    if (cached != m_images.end()) return &cached->second;

    const Optional<String> resolved = Resolve(relative);
    if (!resolved)
    {
        if (m_missing.insert(key).second)
            PRS_WARN("Assets: image '%s' not found in %zu search path(s)", key.c_str(),
                     m_searchPaths.size());
        return nullptr;
    }

    Image image;
    String error;
    if (!LoadImageFile(*resolved, image, error))
    {
        PRS_ERROR("Assets: %s", error.c_str());
        return nullptr;
    }

    PRS_INFO("Assets: loaded image '%s' (%ux%u)", key.c_str(), image.width, image.height);
    return &m_images.emplace(key, std::move(image)).first->second;
}

void AssetManager::Clear()
{
    m_meshes.clear();
    m_images.clear();
    m_missing.clear();
}
} // namespace PRS::Assets

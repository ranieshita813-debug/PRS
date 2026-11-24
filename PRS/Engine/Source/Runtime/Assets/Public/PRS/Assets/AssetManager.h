#pragma once
#include "PRS/Assets/Image.h"
#include "PRS/Assets/MeshData.h"

namespace PRS::Assets
{
// Resolves relative asset names against an ordered list of roots and caches the
// CPU-side data so a mesh or texture is read and decoded once per session.
//
// The renderer owns one of these; games point it at their content directory:
//   assets.AddSearchPath("Assets");
//   const MeshData* mesh = assets.LoadMesh("Models/sphere.obj");
class AssetManager
{
public:
    void AddSearchPath(String path);
    void ClearSearchPaths();
    const Array<String>& SearchPaths() const { return m_searchPaths; }

    // Absolute paths pass straight through when the file exists.
    Optional<String> Resolve(StringView relative) const;
    bool Exists(StringView relative) const;

    // Returns null (after logging) when the asset is missing or invalid.
    const MeshData* LoadMesh(StringView relative);
    const Image* LoadImage(StringView relative);

    // Cached assets loaded so far; used by stats overlays and tests.
    size_t MeshCount() const { return m_meshes.size(); }
    size_t ImageCount() const { return m_images.size(); }
    void Clear();

    // Reads a whole file. Public so tools and tests can reuse it.
    static bool ReadFile(const String& path, Array<u8>& out);

private:
    Array<String> m_searchPaths;
    HashMap<String, MeshData> m_meshes;
    HashMap<String, Image> m_images;
    HashSet<String> m_missing; // warned once per name, not every frame
};
} // namespace PRS::Assets

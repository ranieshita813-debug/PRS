#pragma once
#include "PRS/Assets/MeshData.h"

namespace PRS::Assets
{
// Wavefront OBJ + MTL import. Supports v/vt/vn, every face index form
// ("v", "v/vt", "v//vn", "v/vt/vn"), negative (relative) indices, n-gon faces
// (fan triangulation) and usemtl groups, each of which becomes a MeshPart.
// Faces without normals get smooth normals computed from the geometry.
bool LoadOBJ(ConstSpan<u8> text, StringView name, MeshData& out, String& error);

// Parses an MTL file into `out`, keyed by material name. Texture paths are left
// exactly as written; LoadMeshFile rewrites them relative to the OBJ directory.
bool LoadMTL(ConstSpan<u8> text, HashMap<String, MaterialData>& out, String& error);

// Loads an .obj from disk, including its mtllib materials. Returns false and
// fills `error` when the file is missing or malformed.
bool LoadMeshFile(const String& path, MeshData& out, String& error);
} // namespace PRS::Assets

#pragma once
#include "PRS/Core/Containers/Containers.h"
#include "PRS/Core/Math/Vector.h"

namespace PRS::Assets
{
// CPU-side vertex, identical in layout to the renderer's GPU vertex so a mesh
// uploads with no conversion. Keep the member order in sync with Renderer::Vertex.
struct MeshVertex
{
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{};
};

struct MaterialData
{
    String name = "default";
    Vec4 baseColor{0.82f, 0.82f, 0.86f, 1.0f};
    Vec3 specularColor{0.25f, 0.25f, 0.25f};
    f32 shininess = 48.0f;
    // Base colour texture path, relative to the mesh's directory (already joined
    // by the loader) or absolute. Empty means "untextured".
    String baseColorTexture;

    bool HasTexture() const { return !baseColorTexture.empty(); }
};

// One draw call's worth of geometry: a single material over one index range.
struct MeshPart
{
    MaterialData material;
    Array<MeshVertex> vertices;
    Array<u32> indices;
};

struct MeshData
{
    String name;
    Array<MeshPart> parts;

    bool IsValid() const
    {
        for (const MeshPart& part : parts)
            if (!part.indices.empty() && !part.vertices.empty()) return true;
        return false;
    }

    u32 VertexCount() const
    {
        u32 total = 0;
        for (const MeshPart& part : parts) total += static_cast<u32>(part.vertices.size());
        return total;
    }

    u32 IndexCount() const
    {
        u32 total = 0;
        for (const MeshPart& part : parts) total += static_cast<u32>(part.indices.size());
        return total;
    }

    u32 TriangleCount() const { return IndexCount() / 3; }
};
} // namespace PRS::Assets

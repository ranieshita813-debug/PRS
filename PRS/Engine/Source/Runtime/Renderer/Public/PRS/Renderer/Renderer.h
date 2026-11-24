#pragma once
#include "PRS/Assets/AssetManager.h"
#include "PRS/Assets/MeshData.h"
#include "PRS/Core/Containers/Containers.h"
#include "PRS/Core/Math/Transform.h"
#include "PRS/Core/Math/Vector.h"
#include "PRS/RHI/CommandList.h"
#include "PRS/RHI/DescriptorHeap.h"
#include "PRS/RHI/Device.h"
#include "PRS/RHI/Pipeline.h"
#include "PRS/RHI/SwapChain.h"
#include "PRS/Scene/Scene.h"
#include <string>

namespace PRS::Renderer
{
// Interleaved vertex layout used by the forward shaders. Matches
// Assets::MeshVertex so loaded meshes upload without conversion.
struct Vertex
{
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{};
};

// One index range with its own material: the unit the renderer draws.
struct MeshPart
{
    u32 firstIndex = 0;
    u32 indexCount = 0;
    struct Material* material = nullptr;
};

struct Mesh
{
    Scope<RHI::Buffer> vertexBuffer;
    Scope<RHI::Buffer> indexBuffer;
    u32 indexCount = 0;
    Array<MeshPart> parts;
};

// GPU material: base colour, specular response and an optional albedo texture
// with its own descriptor set (camera uniform + texture).
struct Material
{
    Vec4 baseColor{0.85f, 0.85f, 0.88f, 1.0f};
    Vec3 specular{0.25f, 0.25f, 0.25f};
    f32 shininess = 48.0f;
    String albedoPath;
    Scope<RHI::Texture> albedo;
    Scope<RHI::DescriptorHeap> heap;
};

// Uniform block, layout must match CameraBlock in the shaders (112 bytes).
struct CameraData
{
    Mat4 viewProjection;
    Vec4 cameraPosition;
    Vec4 lightDirection;
    Vec4 lightColor;
};

// Push constant block, layout must match ObjectBlock (96 bytes, the guaranteed
// minimum push constant size).
struct ObjectData
{
    Mat4 model;
    Vec4 baseColor;
    Vec4 params; // x = specular exponent, y = specular strength
};

struct RenderStats
{
    u32 drawCalls = 0;
    u32 triangles = 0;
    u32 meshes = 0;
    u32 materials = 0;
    u32 frame = 0;
};

struct RendererConfig
{
    std::string shaderDirectory = "Shaders";
    f32 clearColor[4] = {0.09f, 0.11f, 0.16f, 1.0f};
    f32 ambientIntensity = 0.18f;
    bool wireframe = false; // ignored by the first pipeline (fill mode is fixed)
};

// Forward renderer: clears the swapchain, draws every MeshRenderer entity in the
// scene from a camera entity with one directional light, presents.
//
// Meshes come from the built-in "cube"/"plane" primitives or from the asset
// pipeline (.obj files resolved through Assets()). Materials carry an optional
// albedo texture; MeshRenderer::material overrides the mesh's own material and
// is treated as a texture path.
class Renderer
{
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Initialize(RHI::Device& device, RHI::SwapChain& swapchain,
                    const RendererConfig& config = RendererConfig{});
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Recreates the swapchain after a window resize.
    bool Resize(u32 width, u32 height);

    // CPU asset registry used for meshes and textures. Call AddSearchPath()
    // before Initialize() to override the default roots.
    Assets::AssetManager& Assets() { return m_assets; }

    // Mesh cache. Built-in names ("cube", "plane") are generated; anything else
    // is loaded as an .obj through the asset manager, falling back to a cube.
    Mesh& CreateMesh(std::string_view name, ConstSpan<Vertex> vertices,
                     ConstSpan<u32> indices);
    Mesh& GetOrCreateMesh(std::string_view name);
    Mesh& GetCube() { return GetOrCreateMesh("cube"); }
    Mesh& GetPlane() { return GetOrCreateMesh("plane"); }

    // Material cache, keyed by albedo texture path + tint. Null path or a
    // missing file gives the untextured default material.
    Material* GetOrCreateMaterial(const Assets::MaterialData& data);
    Material* GetOrCreateTextureMaterial(std::string_view albedoPath);
    Material* DefaultMaterial() { return m_defaultMaterial; }

    // Draws and presents one frame. `camera` may be null, in which case a default
    // orbit camera is used so something is always on screen.
    void Render(Scene::Scene& scene, ECS::Entity camera, f32 elapsedSeconds);

    const RenderStats& Stats() const { return m_stats; }
    RHI::SwapChain& SwapChainHandle() { return *m_swapchain; }
    void SetShaderDirectory(std::string directory);

private:
    bool CreatePipeline();
    bool CreateFrameResources();
    std::string FindShader(const char* name) const;
    Mesh& CreateMeshFromData(std::string_view name, const Assets::MeshData& data);
    Material* CreateMaterial(const Assets::MaterialData& data, const String& key);
    void AddDefaultSearchPaths();

    RHI::Device* m_device = nullptr;
    RHI::SwapChain* m_swapchain = nullptr;
    RendererConfig m_config;

    Scope<RHI::Pipeline> m_pipeline;
    Scope<RHI::Buffer> m_cameraBuffer;
    Scope<RHI::DescriptorHeap> m_frameHeap;
    Scope<RHI::CommandList> m_commandList;
    Scope<RHI::Sampler> m_defaultSampler;
    Scope<RHI::Texture> m_whiteTexture;

    Assets::AssetManager m_assets;
    HashMap<std::string, Scope<Mesh>> m_meshes;
    HashMap<std::string, Scope<Material>> m_materials;
    HashSet<std::string> m_missingMeshes;
    Material* m_defaultMaterial = nullptr;

    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
    RenderStats m_stats;
    bool m_initialized = false;
    u64 m_frameCounter = 0;
};
} // namespace PRS::Renderer

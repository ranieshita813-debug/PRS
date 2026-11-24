#pragma once
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
// Interleaved vertex layout used by the forward shaders.
struct Vertex
{
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{};
};

struct Mesh
{
    Scope<RHI::Buffer> vertexBuffer;
    Scope<RHI::Buffer> indexBuffer;
    u32 indexCount = 0;
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

    // Mesh cache. Unknown names fall back to the built-in cube so a scene is
    // always visible before the asset pipeline exists.
    Mesh& CreateMesh(std::string_view name, ConstSpan<Vertex> vertices,
                     ConstSpan<u32> indices);
    Mesh& GetOrCreateMesh(std::string_view name);
    Mesh& GetCube() { return GetOrCreateMesh("cube"); }
    Mesh& GetPlane() { return GetOrCreateMesh("plane"); }

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

    RHI::Device* m_device = nullptr;
    RHI::SwapChain* m_swapchain = nullptr;
    RendererConfig m_config;

    Scope<RHI::Pipeline> m_pipeline;
    Scope<RHI::Buffer> m_cameraBuffer;
    Scope<RHI::DescriptorHeap> m_frameHeap;
    Scope<RHI::CommandList> m_commandList;
    Scope<RHI::Sampler> m_defaultSampler;
    Scope<RHI::Texture> m_whiteTexture;

    HashMap<std::string, Scope<Mesh>> m_meshes;
    std::string m_vertexShaderPath;
    std::string m_fragmentShaderPath;
    RenderStats m_stats;
    f32 m_lastAmbient = 0.18f;
    bool m_initialized = false;
    u64 m_frameCounter = 0;
};
} // namespace PRS::Renderer

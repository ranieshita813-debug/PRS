#include "PRS/Renderer/Renderer.h"
#include "PRS/Core/Log.h"
#include "PRS/Core/Math/Matrix.h"
#include <cstring>
#include <fstream>
#include <vector>

namespace PRS::Renderer
{
namespace Sc = PRS::Scene;

namespace
{
void AddQuad(std::vector<Vertex>& vertices, std::vector<u32>& indices, const Vec3& a, const Vec3& b,
             const Vec3& c, const Vec3& d, const Vec3& normal)
{
    const u32 base = static_cast<u32>(vertices.size());
    vertices.push_back({a, normal, {0.0f, 0.0f}});
    vertices.push_back({b, normal, {1.0f, 0.0f}});
    vertices.push_back({c, normal, {1.0f, 1.0f}});
    vertices.push_back({d, normal, {0.0f, 1.0f}});
    indices.insert(indices.end(), {base + 0, base + 1, base + 2, base + 0, base + 2, base + 3});
}

void BuildCube(std::vector<Vertex>& vertices, std::vector<u32>& indices)
{
    const Vec3 p000{-0.5f, -0.5f, -0.5f};
    const Vec3 p100{0.5f, -0.5f, -0.5f};
    const Vec3 p110{0.5f, 0.5f, -0.5f};
    const Vec3 p010{-0.5f, 0.5f, -0.5f};
    const Vec3 p001{-0.5f, -0.5f, 0.5f};
    const Vec3 p101{0.5f, -0.5f, 0.5f};
    const Vec3 p111{0.5f, 0.5f, 0.5f};
    const Vec3 p011{-0.5f, 0.5f, 0.5f};

    AddQuad(vertices, indices, p001, p101, p111, p011, {0.0f, 0.0f, 1.0f});   // +Z
    AddQuad(vertices, indices, p100, p000, p010, p110, {0.0f, 0.0f, -1.0f});  // -Z
    AddQuad(vertices, indices, p101, p100, p110, p111, {1.0f, 0.0f, 0.0f});   // +X
    AddQuad(vertices, indices, p000, p001, p011, p010, {-1.0f, 0.0f, 0.0f});  // -X
    AddQuad(vertices, indices, p011, p111, p110, p010, {0.0f, 1.0f, 0.0f});   // +Y
    AddQuad(vertices, indices, p000, p100, p101, p001, {0.0f, -1.0f, 0.0f});  // -Y
}

void BuildPlane(std::vector<Vertex>& vertices, std::vector<u32>& indices)
{
    const f32 extent = 10.0f;
    AddQuad(vertices, indices, {-extent, 0.0f, extent}, {extent, 0.0f, extent},
            {extent, 0.0f, -extent}, {-extent, 0.0f, -extent}, {0.0f, 1.0f, 0.0f});
}

bool FileExists(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return file.good();
}
} // namespace

Renderer::Renderer() = default;
Renderer::~Renderer() { Shutdown(); }

std::string Renderer::FindShader(const char* name) const
{
    const std::string candidates[] = {
        m_config.shaderDirectory + "/" + name,
        m_config.shaderDirectory + "/Compiled/" + name,
        std::string("../Shaders/") + name,
        std::string("Engine/Shaders/Compiled/") + name,
        std::string("..") + "/Engine/Shaders/Compiled/" + name,
    };
    for (const std::string& candidate : candidates)
        if (FileExists(candidate)) return candidate;
    return m_config.shaderDirectory + "/" + name; // returned so the error names it
}

void Renderer::SetShaderDirectory(std::string directory)
{
    m_config.shaderDirectory = std::move(directory);
    if (m_initialized)
    {
        m_vertexShaderPath = FindShader("forward.vert.spv");
        m_fragmentShaderPath = FindShader("forward.frag.spv");
    }
}

bool Renderer::Initialize(RHI::Device& device, RHI::SwapChain& swapchain,
                          const RendererConfig& config)
{
    Shutdown();
    m_device = &device;
    m_swapchain = &swapchain;
    m_config = config;
    m_lastAmbient = config.ambientIntensity;

    m_vertexShaderPath = FindShader("forward.vert.spv");
    m_fragmentShaderPath = FindShader("forward.frag.spv");

    if (!CreatePipeline()) return false;
    if (!CreateFrameResources()) return false;

    // A 1x1 white texture keeps the combined image sampler binding valid for
    // pipelines that eventually sample material textures.
    RHI::TextureDesc whiteDesc;
    whiteDesc.size = {1, 1};
    whiteDesc.format = RHI::Format::RGBA8Unorm;
    whiteDesc.usage = RHI::TextureUsage::Sampled;
    whiteDesc.debugName = "White";
    m_whiteTexture = device.CreateTexture(whiteDesc);
    const u8 white[4] = {255, 255, 255, 255};
    if (m_whiteTexture) m_whiteTexture->Upload(white, sizeof(white));

    RHI::SamplerDesc samplerDesc;
    samplerDesc.addressU = RHI::AddressMode::ClampToEdge;
    samplerDesc.addressV = RHI::AddressMode::ClampToEdge;
    samplerDesc.addressW = RHI::AddressMode::ClampToEdge;
    m_defaultSampler = device.CreateSampler(samplerDesc);

    if (m_frameHeap && m_whiteTexture && m_defaultSampler)
        m_frameHeap->SetCombinedImageSampler(1, *m_whiteTexture, *m_defaultSampler);

    m_initialized = true;
    PRS_INFO("Renderer ready: %s, %s shaders", device.BackendName(), m_vertexShaderPath.c_str());
    return true;
}

bool Renderer::CreatePipeline()
{
    RHI::GraphicsPipelineDesc desc;
    desc.vertexShaderPath = m_vertexShaderPath.c_str();
    desc.fragmentShaderPath = m_fragmentShaderPath.c_str();
    desc.colorFormat = m_swapchain->ColorFormat();
    desc.depthFormat = RHI::Format::Depth32Float;
    desc.cullMode = RHI::CullMode::Back;
    desc.depthTest = true;
    desc.depthWrite = true;
    desc.alphaBlend = false;
    desc.pushConstantSize = static_cast<u32>(sizeof(ObjectData));
    desc.debugName = "ForwardScene";

    desc.vertexBinding.stride = static_cast<u32>(sizeof(Vertex));
    desc.vertexBinding.attributes = {
        {0, RHI::VertexFormat::Float3, static_cast<u32>(offsetof(Vertex, position))},
        {1, RHI::VertexFormat::Float3, static_cast<u32>(offsetof(Vertex, normal))},
        {2, RHI::VertexFormat::Float2, static_cast<u32>(offsetof(Vertex, uv))},
    };

    m_pipeline = m_device->CreateGraphicsPipeline(desc);
    if (!m_pipeline)
    {
        PRS_ERROR("Renderer: pipeline creation failed (missing %s / %s?)",
                  m_vertexShaderPath.c_str(), m_fragmentShaderPath.c_str());
        return false;
    }
    return true;
}

bool Renderer::CreateFrameResources()
{
    RHI::BufferDesc cameraDesc;
    cameraDesc.size = sizeof(CameraData);
    cameraDesc.usage = RHI::BufferUsage::Uniform;
    cameraDesc.hostVisible = true;
    cameraDesc.debugName = "CameraUniform";
    m_cameraBuffer = m_device->CreateBuffer(cameraDesc);
    if (!m_cameraBuffer) return false;

    m_frameHeap = m_device->CreateDescriptorHeap();
    if (!m_frameHeap) return false;
    m_frameHeap->SetUniformBuffer(0, *m_cameraBuffer, 0, sizeof(CameraData));

    m_commandList = m_device->CreateCommandList();
    if (!m_commandList) return false;

    return true;
}

void Renderer::Shutdown()
{
    // The swapchain and device may already be gone; drop GPU objects first.
    m_meshes.clear();
    m_commandList.reset();
    m_frameHeap.reset();
    m_cameraBuffer.reset();
    m_pipeline.reset();
    m_whiteTexture.reset();
    m_defaultSampler.reset();
    m_initialized = false;
    m_device = nullptr;
    m_swapchain = nullptr;
    m_stats = RenderStats{};
}

bool Renderer::Resize(u32 width, u32 height)
{
    if (!m_initialized || m_swapchain == nullptr) return false;
    if (width == 0 || height == 0) return false;
    if (m_swapchain->Size().width == width && m_swapchain->Size().height == height) return true;

    if (!m_swapchain->Resize(width, height)) return false;

    // The pipeline is tied to the swapchain's formats; recreate it if they changed.
    if (!CreatePipeline()) return false;
    return true;
}

Mesh& Renderer::CreateMesh(std::string_view name, ConstSpan<Vertex> vertices,
                           ConstSpan<u32> indices)
{
    auto mesh = MakeScope<Mesh>();

    if (!vertices.empty())
    {
        RHI::BufferDesc vertexDesc;
        vertexDesc.size = static_cast<u64>(vertices.size_bytes());
        vertexDesc.usage = RHI::BufferUsage::Vertex | RHI::BufferUsage::TransferDst;
        vertexDesc.debugName = "MeshVertices";
        mesh->vertexBuffer = m_device->CreateBuffer(vertexDesc);
        if (mesh->vertexBuffer) mesh->vertexBuffer->Write(vertices.data(), vertexDesc.size);
    }

    if (!indices.empty())
    {
        RHI::BufferDesc indexDesc;
        indexDesc.size = static_cast<u64>(indices.size_bytes());
        indexDesc.usage = RHI::BufferUsage::Index | RHI::BufferUsage::TransferDst;
        indexDesc.debugName = "MeshIndices";
        mesh->indexBuffer = m_device->CreateBuffer(indexDesc);
        if (mesh->indexBuffer) mesh->indexBuffer->Write(indices.data(), indexDesc.size);
        mesh->indexCount = static_cast<u32>(indices.size());
    }

    Mesh& stored = *(mesh.get());
    m_meshes[std::string(name)] = std::move(mesh);
    return stored;
}

Mesh& Renderer::GetOrCreateMesh(std::string_view name)
{
    const std::string key(name);
    const auto existing = m_meshes.find(key);
    if (existing != m_meshes.end()) return *existing->second;

    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    if (key == "plane")
        BuildPlane(vertices, indices);
    else
        BuildCube(vertices, indices);

    return CreateMesh(key, vertices, indices);
}

void Renderer::Render(Scene::Scene& scene, ECS::Entity cameraEntity, f32 elapsedSeconds)
{
    if (!m_initialized || m_device == nullptr || m_swapchain == nullptr || !m_commandList) return;

    if (!m_swapchain->BeginFrame()) return; // out of date; the app resizes next frame

    m_stats = RenderStats{};
    m_stats.frame = static_cast<u32>(++m_frameCounter);

    RHI::Texture& color = m_swapchain->CurrentColorTexture();
    RHI::Texture& depth = m_swapchain->DepthTexture();
    const RHI::Extent2D extent = m_swapchain->Size();
    const f32 aspect = extent.height > 0 ? static_cast<f32>(extent.width) / extent.height : 1.0f;

    // --- Camera -------------------------------------------------------------
    CameraData cameraData{};
    Mat4 view = Mat4::Identity();
    bool usedSceneCamera = false;

    const Sc::Transform* cameraTransform =
        scene.IsAlive(cameraEntity) ? scene.World().TryGet<Sc::Transform>(cameraEntity) : nullptr;
    const Sc::Camera* cameraComponent =
        scene.IsAlive(cameraEntity) ? scene.World().TryGet<Sc::Camera>(cameraEntity) : nullptr;

    if (cameraTransform != nullptr)
    {
        const Vec3 forward = cameraTransform->Forward();
        const Vec3 eye = cameraTransform->position;
        view = Mat4::LookAt(eye, eye + forward, cameraTransform->Up());
        cameraData.cameraPosition = Vec4(eye, 1.0f);
        usedSceneCamera = true;
    }
    else
    {
        // Default orbit camera so an empty scene still shows something.
        const f32 angle = elapsedSeconds * 0.4f;
        const Vec3 eye{6.0f * std::cos(angle), 4.0f, 6.0f * std::sin(angle)};
        view = Mat4::LookAt(eye, {0.0f, 0.5f, 0.0f}, AxisY());
        cameraData.cameraPosition = Vec4(eye, 1.0f);
    }

    if (!usedSceneCamera) cameraComponent = nullptr;

    Mat4 projection = cameraComponent != nullptr
                          ? cameraComponent->Projection()
                          : Mat4::Perspective(Radians(60.0f), aspect, 0.1f, 500.0f);
    if (cameraComponent != nullptr)
    {
        Sc::Camera adjusted = *cameraComponent;
        adjusted.aspect = aspect;
        projection = adjusted.Projection();
    }

    cameraData.viewProjection = projection * view;

    // --- Light --------------------------------------------------------------
    Vec3 lightDirection = Normalize(Vec3{0.45f, 0.8f, 0.3f});
    Vec3 lightColor{1.0f, 0.98f, 0.94f};
    scene.World().Query<Sc::Light, Sc::Transform>().ForEach(
        [&](ECS::Entity, Sc::Light& light, Sc::Transform& transform) {
            if (light.type != Sc::LightType::Directional) return;
            lightColor = light.color * light.intensity;
            // A directional light shines along its forward axis; the shading
            // term needs the direction towards the light.
            lightDirection = Normalize(-transform.Forward());
        });
    cameraData.lightDirection = Vec4(lightDirection, 0.0f);
    cameraData.lightColor = Vec4(lightColor, 1.0f);

    if (m_cameraBuffer && m_cameraBuffer->MappedPointer() != nullptr)
        std::memcpy(m_cameraBuffer->MappedPointer(), &cameraData, sizeof(CameraData));

    // --- Record -------------------------------------------------------------
    RHI::CommandList& commands = *m_commandList;
    commands.Begin();

    RHI::RenderPassDesc pass;
    pass.color = &color;
    pass.depth = &depth;
    pass.clearColor = true;
    pass.clearDepth = true;
    for (int i = 0; i < 4; ++i) pass.clearColorValue[i] = m_config.clearColor[i];
    commands.BeginRenderPass(pass);

    commands.SetPipeline(*m_pipeline);
    commands.SetDescriptorHeap(*m_frameHeap);

    scene.World().Query<Sc::Transform, Sc::MeshRenderer>().ForEach(
        [&](ECS::Entity entity, Sc::Transform&, Sc::MeshRenderer& meshRenderer) {
            if (!meshRenderer.visible) return;

            Mesh& mesh = GetOrCreateMesh(meshRenderer.mesh);
            if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indexCount == 0) return;

            ObjectData object{};
            object.model = scene.WorldMatrix(entity);

            // Vary colour per object so untextured scenes still read as 3D.
            const f32 tint = static_cast<f32>((entity.index * 37u) % 100u) / 100.0f;
            object.baseColor = Vec4(0.35f + tint * 0.55f, 0.45f + (1.0f - tint) * 0.45f, 0.75f,
                                    1.0f);
            object.params = Vec4(48.0f, 0.35f, 0.0f, 0.0f);

            commands.PushConstants(&object, sizeof(ObjectData));
            commands.SetVertexBuffer(*mesh.vertexBuffer);
            commands.SetIndexBuffer(*mesh.indexBuffer);
            commands.DrawIndexed(mesh.indexCount);

            ++m_stats.drawCalls;
            ++m_stats.meshes;
            m_stats.triangles += mesh.indexCount / 3;
        });

    commands.EndRenderPass();
    commands.End();

    m_swapchain->Submit(commands);
    m_swapchain->Present();
}
} // namespace PRS::Renderer

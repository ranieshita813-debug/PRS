#include "PRS/Renderer/Renderer.h"
#include "PRS/Core/Log.h"
#include "PRS/Core/Math/Matrix.h"
#include <cstdio>
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

std::string MaterialKey(const Assets::MaterialData& data)
{
    char header[96];
    std::snprintf(header, sizeof(header), "%.3f_%.3f_%.3f_%.3f_%.3f_%.1f|", data.baseColor.x,
                  data.baseColor.y, data.baseColor.z, data.baseColor.w, data.specularColor.x,
                  data.shininess);
    return std::string(header) + data.baseColorTexture;
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
        m_shadowShaderPath = FindShader("shadow.vert.spv");
    }
}

void Renderer::AddDefaultSearchPaths()
{
    m_assets.AddSearchPath("Assets");
    m_assets.AddSearchPath("../Assets");
    m_assets.AddSearchPath("../../Assets");
    m_assets.AddSearchPath("Games/MyOpenWorld/Assets");
    m_assets.AddSearchPath("../Games/MyOpenWorld/Assets");
}

bool Renderer::Initialize(RHI::Device& device, RHI::SwapChain& swapchain,
                          const RendererConfig& config)
{
    Shutdown();
    m_device = &device;
    m_swapchain = &swapchain;
    m_config = config;

    if (m_assets.SearchPaths().empty()) AddDefaultSearchPaths();

    m_vertexShaderPath = FindShader("forward.vert.spv");
    m_fragmentShaderPath = FindShader("forward.frag.spv");
    m_shadowShaderPath = FindShader("shadow.vert.spv");

    if (!CreatePipeline()) return false;
    if (!CreateFrameResources()) return false;

    // A 1x1 white texture keeps the material binding valid for untextured
    // materials, so the shader always samples something.
    RHI::TextureDesc whiteDesc;
    whiteDesc.size = {1, 1};
    whiteDesc.format = RHI::Format::RGBA8Unorm;
    whiteDesc.usage = RHI::TextureUsage::Sampled;
    whiteDesc.debugName = "White";
    m_whiteTexture = device.CreateTexture(whiteDesc);
    const u8 white[4] = {255, 255, 255, 255};
    if (m_whiteTexture) m_whiteTexture->Upload(white, sizeof(white));

    m_defaultSampler = device.CreateSampler(RHI::SamplerDesc{});

    // The shadow map exists before any material, because every material
    // descriptor set binds it at set 0 binding 2.
    m_shadowsActive = CreateShadowPipeline();
    if (!m_shadowsActive && m_config.shadowsEnabled)
        PRS_WARN("Renderer: directional shadows disabled (%s missing or the depth pass failed)",
                 m_shadowShaderPath.c_str());

    if (m_frameHeap && m_whiteTexture && m_defaultSampler)
    {
        m_frameHeap->SetCombinedImageSampler(1, *m_whiteTexture, *m_defaultSampler);
        RHI::Texture* shadowTexture = ShadowTexture();
        RHI::Sampler* shadowSampler =
            m_shadowSampler ? m_shadowSampler.get() : m_defaultSampler.get();
        if (shadowTexture != nullptr && shadowSampler != nullptr)
            m_frameHeap->SetCombinedImageSampler(2, *shadowTexture, *shadowSampler);
    }

    Assets::MaterialData defaultMaterial;
    m_defaultMaterial = GetOrCreateMaterial(defaultMaterial);
    if (m_defaultMaterial == nullptr) return false;

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

bool Renderer::CreateShadowPipeline()
{
    if (!m_config.shadowsEnabled || m_device == nullptr) return false;

    const u32 size = m_config.shadowMapSize > 0 ? m_config.shadowMapSize : 2048;

    RHI::TextureDesc mapDesc;
    mapDesc.size = {size, size};
    mapDesc.format = RHI::Format::Depth32Float;
    mapDesc.usage = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::Sampled;
    mapDesc.debugName = "ShadowMap";
    m_shadowMap = m_device->CreateTexture(mapDesc);
    if (!m_shadowMap) return false;

    // Nearest filtering: the fragment shader does the depth comparison itself,
    // and clamping keeps lookups outside the map from wrapping around.
    RHI::SamplerDesc samplerDesc;
    samplerDesc.minFilter = RHI::Filter::Nearest;
    samplerDesc.magFilter = RHI::Filter::Nearest;
    samplerDesc.addressU = RHI::AddressMode::ClampToEdge;
    samplerDesc.addressV = RHI::AddressMode::ClampToEdge;
    samplerDesc.addressW = RHI::AddressMode::ClampToEdge;
    samplerDesc.anisotropy = false;
    samplerDesc.maxLod = 0.0f;
    m_shadowSampler = m_device->CreateSampler(samplerDesc);
    if (!m_shadowSampler)
    {
        m_shadowMap.reset();
        return false;
    }

    RHI::GraphicsPipelineDesc desc;
    desc.vertexShaderPath = m_shadowShaderPath.c_str();
    desc.fragmentShaderPath = nullptr; // depth-only pass, nothing is shaded
    desc.colorFormat = RHI::Format::Undefined;
    desc.depthFormat = RHI::Format::Depth32Float;
    // Culling front faces and drawing backs keeps the recorded depth just
    // behind lit surfaces, which removes most shadow acne.
    desc.cullMode = RHI::CullMode::Front;
    desc.depthTest = true;
    desc.depthWrite = true;
    desc.alphaBlend = false;
    desc.pushConstantSize = static_cast<u32>(sizeof(ShadowObjectData));
    desc.debugName = "ShadowDepth";

    desc.vertexBinding.stride = static_cast<u32>(sizeof(Vertex));
    desc.vertexBinding.attributes = {
        {0, RHI::VertexFormat::Float3, static_cast<u32>(offsetof(Vertex, position))},
        {1, RHI::VertexFormat::Float3, static_cast<u32>(offsetof(Vertex, normal))},
        {2, RHI::VertexFormat::Float2, static_cast<u32>(offsetof(Vertex, uv))},
    };

    m_shadowPipeline = m_device->CreateGraphicsPipeline(desc);
    if (!m_shadowPipeline)
    {
        m_shadowMap.reset();
        m_shadowSampler.reset();
        return false;
    }

    m_shadowMapWritten = false;
    PRS_INFO("Renderer: directional shadow map %ux%u", size, size);
    return true;
}

RHI::Texture* Renderer::ShadowTexture() const
{
    if (m_shadowsActive && m_shadowMap) return m_shadowMap.get();
    return m_whiteTexture ? m_whiteTexture.get() : nullptr;
}

ShadowBounds Renderer::ComputeShadowBounds(Scene::Scene& scene) const
{
    Vec3 minimum{1.0e30f, 1.0e30f, 1.0e30f};
    Vec3 maximum{-1.0e30f, -1.0e30f, -1.0e30f};
    bool any = false;

    scene.World().Query<Sc::Transform, Sc::MeshRenderer>().ForEach(
        [&](ECS::Entity entity, Sc::Transform&, Sc::MeshRenderer& meshRenderer) {
            if (!meshRenderer.visible || !meshRenderer.castShadows) return;

            const Mat4 world = scene.WorldMatrix(entity);
            const Vec3 center = world.TranslationPart();

            // Built-in meshes are unit sized, so the world-space scale gives a
            // usable half extent without walking the vertices.
            const f32 sx = Length(world.Column(0));
            const f32 sy = Length(world.Column(1));
            const f32 sz = Length(world.Column(2));
            f32 extent = sx > sy ? sx : sy;
            if (sz > extent) extent = sz;
            extent *= 0.75f;

            const Vec3 half{extent, extent, extent};
            minimum = Min(minimum, center - half);
            maximum = Max(maximum, center + half);
            any = true;
        });

    if (!any) return ShadowBounds{};
    return MakeShadowBounds(minimum, maximum);
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
    m_materials.clear();
    m_missingMeshes.clear();
    m_defaultMaterial = nullptr;
    m_assets.Clear();
    m_commandList.reset();
    m_frameHeap.reset();
    m_cameraBuffer.reset();
    m_shadowPipeline.reset();
    m_shadowMap.reset();
    m_shadowSampler.reset();
    m_shadowsActive = false;
    m_shadowMapWritten = false;
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

Material* Renderer::CreateMaterial(const Assets::MaterialData& data, const String& key)
{
    auto material = MakeScope<Material>();
    material->baseColor = data.baseColor;
    material->specular = data.specularColor;
    material->shininess = data.shininess;
    material->albedoPath = data.baseColorTexture;

    const Assets::Image* image =
        data.HasTexture() ? m_assets.LoadImage(data.baseColorTexture) : nullptr;
    if (image != nullptr && image->IsValid())
    {
        RHI::TextureDesc desc;
        desc.size = {image->width, image->height};
        desc.format = RHI::Format::RGBA8Unorm;
        desc.usage = RHI::TextureUsage::Sampled;
        desc.debugName = "Albedo";
        material->albedo = m_device->CreateTexture(desc);
        if (material->albedo) material->albedo->Upload(image->pixels.data(), image->SizeBytes());
    }

    material->heap = m_device->CreateDescriptorHeap();
    if (material->heap)
    {
        material->heap->SetUniformBuffer(0, *m_cameraBuffer, 0, sizeof(CameraData));
        RHI::Texture* texture = material->albedo ? material->albedo.get() : m_whiteTexture.get();
        if (texture != nullptr && m_defaultSampler != nullptr)
            material->heap->SetCombinedImageSampler(1, *texture, *m_defaultSampler);

        RHI::Texture* shadowTexture = ShadowTexture();
        RHI::Sampler* shadowSampler =
            m_shadowSampler ? m_shadowSampler.get() : m_defaultSampler.get();
        if (shadowTexture != nullptr && shadowSampler != nullptr)
            material->heap->SetCombinedImageSampler(2, *shadowTexture, *shadowSampler);
    }

    Material* stored = material.get();
    m_materials.emplace(key, std::move(material));
    return stored;
}

Material* Renderer::GetOrCreateMaterial(const Assets::MaterialData& data)
{
    const String key = MaterialKey(data);
    const auto existing = m_materials.find(key);
    if (existing != m_materials.end()) return existing->second.get();
    return CreateMaterial(data, key);
}

Material* Renderer::GetOrCreateTextureMaterial(std::string_view albedoPath)
{
    if (albedoPath.empty()) return m_defaultMaterial;
    Assets::MaterialData data;
    data.baseColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    data.baseColorTexture = String(albedoPath);
    return GetOrCreateMaterial(data);
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
        mesh->parts.push_back({0, mesh->indexCount, m_defaultMaterial});
    }

    Mesh& stored = *mesh;
    m_meshes[String(name)] = std::move(mesh);
    return stored;
}

Mesh& Renderer::CreateMeshFromData(std::string_view name, const Assets::MeshData& data)
{
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    Array<MeshPart> parts;

    for (const Assets::MeshPart& part : data.parts)
    {
        if (part.vertices.empty() || part.indices.empty()) continue;

        MeshPart gpuPart;
        gpuPart.firstIndex = static_cast<u32>(indices.size());
        gpuPart.indexCount = static_cast<u32>(part.indices.size());
        gpuPart.material = GetOrCreateMaterial(part.material);

        const u32 baseVertex = static_cast<u32>(vertices.size());
        vertices.reserve(vertices.size() + part.vertices.size());
        for (const Assets::MeshVertex& vertex : part.vertices)
            vertices.push_back({vertex.position, vertex.normal, vertex.uv});
        indices.reserve(indices.size() + part.indices.size());
        for (u32 index : part.indices) indices.push_back(baseVertex + index);

        parts.push_back(gpuPart);
    }

    Mesh& mesh = CreateMesh(name, vertices, indices);
    mesh.parts = std::move(parts);
    return mesh;
}

Mesh& Renderer::GetOrCreateMesh(std::string_view name)
{
    const String key(name);
    const auto existing = m_meshes.find(key);
    if (existing != m_meshes.end()) return *existing->second;

    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    if (key == "cube")
    {
        BuildCube(vertices, indices);
        return CreateMesh(key, vertices, indices);
    }
    if (key == "plane")
    {
        BuildPlane(vertices, indices);
        return CreateMesh(key, vertices, indices);
    }

    if (const Assets::MeshData* data = m_assets.LoadMesh(name))
        return CreateMeshFromData(name, *data);

    if (m_missingMeshes.insert(key).second)
        PRS_WARN("Renderer: mesh '%s' not found through the asset paths, using a cube",
                 key.c_str());

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

    // --- Shadows ------------------------------------------------------------
    // Fit the sun's orthographic camera around every caster in the scene and
    // hand the same matrix to the shader that sampled the depth pass.
    const ShadowBounds shadowBounds = ComputeShadowBounds(scene);
    const Mat4 lightViewProjection = MakeDirectionalLightMatrix(
        lightDirection, shadowBounds, shadowBounds.radius * 0.5f + 5.0f);
    cameraData.lightViewProjection = lightViewProjection;

    const f32 shadowMapSize = m_shadowMap && m_shadowMap->Size().width > 0
                                  ? static_cast<f32>(m_shadowMap->Size().width)
                                  : 1.0f;
    cameraData.shadowParams =
        Vec4(1.0f / shadowMapSize, m_config.shadowBias, m_config.shadowNormalBias,
             m_shadowsActive ? 1.0f : 0.0f);

    if (m_cameraBuffer && m_cameraBuffer->MappedPointer() != nullptr)
        std::memcpy(m_cameraBuffer->MappedPointer(), &cameraData, sizeof(CameraData));

    // --- Record -------------------------------------------------------------
    RHI::CommandList& commands = *m_commandList;
    commands.Begin();

    // --- Depth pass ---------------------------------------------------------
    if (m_shadowsActive && m_shadowPipeline && m_shadowMap)
    {
        RHI::Barrier toDepth;
        toDepth.before = m_shadowMapWritten ? RHI::ResourceState::ShaderRead
                                            : RHI::ResourceState::Undefined;
        toDepth.after = RHI::ResourceState::DepthTarget;
        toDepth.texture = m_shadowMap.get();
        commands.Barrier(toDepth);

        RHI::RenderPassDesc shadowPass;
        shadowPass.color = nullptr; // depth-only render pass
        shadowPass.depth = m_shadowMap.get();
        shadowPass.clearDepth = true;
        commands.BeginRenderPass(shadowPass);
        commands.SetPipeline(*m_shadowPipeline);

        scene.World().Query<Sc::Transform, Sc::MeshRenderer>().ForEach(
            [&](ECS::Entity entity, Sc::Transform&, Sc::MeshRenderer& meshRenderer) {
                if (!meshRenderer.visible || !meshRenderer.castShadows) return;

                Mesh& mesh = GetOrCreateMesh(meshRenderer.mesh);
                if (!mesh.vertexBuffer || !mesh.indexBuffer) return;

                ShadowObjectData object{};
                object.lightViewProjection = lightViewProjection;
                object.model = scene.WorldMatrix(entity);

                commands.PushConstants(&object, sizeof(ShadowObjectData));
                commands.SetVertexBuffer(*mesh.vertexBuffer);
                commands.SetIndexBuffer(*mesh.indexBuffer);

                for (const MeshPart& part : mesh.parts)
                {
                    if (part.indexCount == 0) continue;
                    commands.DrawIndexed(part.indexCount, part.firstIndex);
                    ++m_stats.shadowDrawCalls;
                }
                ++m_stats.shadowCasters;
            });

        commands.EndRenderPass();

        RHI::Barrier toSample;
        toSample.before = RHI::ResourceState::DepthTarget;
        toSample.after = RHI::ResourceState::ShaderRead;
        toSample.texture = m_shadowMap.get();
        commands.Barrier(toSample);

        m_shadowMapWritten = true;
    }

    RHI::RenderPassDesc pass;
    pass.color = &color;
    pass.depth = &depth;
    pass.clearColor = true;
    pass.clearDepth = true;
    for (int i = 0; i < 4; ++i) pass.clearColorValue[i] = m_config.clearColor[i];
    commands.BeginRenderPass(pass);

    commands.SetPipeline(*m_pipeline);

    scene.World().Query<Sc::Transform, Sc::MeshRenderer>().ForEach(
        [&](ECS::Entity entity, Sc::Transform&, Sc::MeshRenderer& meshRenderer) {
            if (!meshRenderer.visible) return;

            Mesh& mesh = GetOrCreateMesh(meshRenderer.mesh);
            if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.parts.empty()) return;

            Material* overrideMaterial = meshRenderer.material.empty()
                                             ? nullptr
                                             : GetOrCreateTextureMaterial(meshRenderer.material);

            ++m_stats.meshes;
            for (const MeshPart& part : mesh.parts)
            {
                if (part.indexCount == 0) continue;
                Material* material = overrideMaterial ? overrideMaterial : part.material;
                if (material == nullptr) material = m_defaultMaterial;

                ObjectData object{};
                object.model = scene.WorldMatrix(entity);
                object.baseColor = material != nullptr ? material->baseColor
                                                       : Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                const Vec3 specular = material != nullptr ? material->specular : Vec3{0.25f};
                const f32 specularStrength = Max(specular.x, Max(specular.y, specular.z));
                object.params = Vec4(material != nullptr ? material->shininess : 48.0f,
                                     specularStrength, 0.0f, 0.0f);

                if (material != nullptr && material->heap) commands.SetDescriptorHeap(*material->heap);

                commands.PushConstants(&object, sizeof(ObjectData));
                commands.SetVertexBuffer(*mesh.vertexBuffer);
                commands.SetIndexBuffer(*mesh.indexBuffer);
                commands.DrawIndexed(part.indexCount, part.firstIndex);

                ++m_stats.drawCalls;
                m_stats.triangles += part.indexCount / 3;
            }
        });

    m_stats.materials = static_cast<u32>(m_materials.size());

    commands.EndRenderPass();
    commands.End();

    m_swapchain->Submit(commands);
    m_swapchain->Present();
}
} // namespace PRS::Renderer

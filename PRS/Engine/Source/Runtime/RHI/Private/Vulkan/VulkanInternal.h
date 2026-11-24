#pragma once
#if defined(PRS_HAS_VULKAN)

#include "PRS/RHI/Barrier.h"
#include "PRS/RHI/Device.h"
#include "PRS/RHI/Queue.h"
#include <vulkan/vulkan.h>

namespace PRS::RHI::Vulkan
{
u32 FindMemoryType(const VkPhysicalDeviceMemoryProperties& properties, u32 typeBits,
                   VkMemoryPropertyFlags required);
VkFormat ToVkFormat(Format format);
Format FromVkFormat(VkFormat format);
VkImageLayout ToVkImageLayout(ResourceState state);
VkAccessFlags ToVkAccess(ResourceState state);
VkPipelineStageFlags ToVkPipelineStage(ResourceState state);
const char* ResultName(VkResult result);

struct DeviceImpl;

struct BufferImpl final : Buffer
{
    DeviceImpl* owner = nullptr;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    u64 size = 0;
    BufferUsage usage = BufferUsage::None;
    bool hostVisible = false;
    void* mapped = nullptr;

    ~BufferImpl() override;
    u64 Size() const override { return size; }
    BufferUsage Usage() const override { return usage; }
    bool IsHostVisible() const override { return hostVisible; }
    void* MappedPointer() override { return mapped; }
    void Write(const void* data, u64 bytes, u64 offset = 0) override;
};

struct TextureImpl final : Texture
{
    DeviceImpl* owner = nullptr;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkFormat vkFormat = VK_FORMAT_UNDEFINED;
    Format format = Format::Undefined;
    Extent2D extent{};
    TextureUsage usage = TextureUsage::None;
    bool ownsImage = true;

    ~TextureImpl() override;
    Extent2D Size() const override { return extent; }
    Format GetFormat() const override { return format; }
    TextureUsage Usage() const override { return usage; }
    bool IsRenderTarget() const override { return HasFlag(usage, TextureUsage::RenderTarget); }
    void Upload(const void* pixels, u64 sizeBytes, u32 mipLevel = 0) override;
};

struct SamplerImpl final : Sampler
{
    DeviceImpl* owner = nullptr;
    VkSampler sampler = VK_NULL_HANDLE;
    ~SamplerImpl() override;
};

struct FenceImpl final : Fence
{
    DeviceImpl* owner = nullptr;
    VkFence fence = VK_NULL_HANDLE;
    ~FenceImpl() override;
    void Wait(u64 timeoutNs = ~0ull) override;
    void Reset() override;
    bool IsSignaled() const override;
};

struct PipelineImpl final : Pipeline
{
    DeviceImpl* owner = nullptr;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    GraphicsPipelineDesc desc{};
    ~PipelineImpl() override;
};

struct DescriptorHeapImpl final : DescriptorHeap
{
    DeviceImpl* owner = nullptr;
    VkDescriptorSet set = VK_NULL_HANDLE;
    void SetUniformBuffer(u32 binding, Buffer& buffer, u64 offset = 0, u64 range = 0) override;
    void SetCombinedImageSampler(u32 binding, Texture& texture, Sampler& sampler) override;
};

struct CommandListImpl final : CommandList
{
    DeviceImpl* owner = nullptr;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkPipelineLayout boundLayout = VK_NULL_HANDLE;
    VkRenderPass activeRenderPass = VK_NULL_HANDLE;
    bool recording = false;
    bool inRenderPass = false;

    ~CommandListImpl() override;
    void Begin() override;
    void End() override;
    void BeginRenderPass(const RenderPassDesc& desc) override;
    void EndRenderPass() override;
    void SetPipeline(Pipeline& pipeline) override;
    void SetDescriptorHeap(DescriptorHeap& heap) override;
    void SetVertexBuffer(Buffer& buffer) override;
    void SetIndexBuffer(Buffer& buffer) override;
    void PushConstants(const void* data, u32 size, u32 offset = 0) override;
    void Draw(u32 vertexCount, u32 firstVertex = 0) override;
    void DrawIndexed(u32 indexCount, u32 firstIndex = 0, i32 vertexOffset = 0) override;
    void Barrier(const PRS::RHI::Barrier& barrier) override;
};

struct QueueImpl final : Queue
{
    DeviceImpl* owner = nullptr;
    VkQueue queue = VK_NULL_HANDLE;
    void Submit(CommandList& commandList, Fence* signalFence = nullptr) override;
    void WaitIdle() override;
};

struct SwapChainImpl final : SwapChain
{
    DeviceImpl* owner = nullptr;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat vkColorFormat = VK_FORMAT_UNDEFINED;
    Format colorFormat = Format::Undefined;
    Extent2D extent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    Array<TextureImpl*> colorTextures;
    TextureImpl* depthTexture = nullptr;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence frameFence = VK_NULL_HANDLE;
    u32 imageIndex = 0;
    bool frameAcquired = false;

    ~SwapChainImpl() override;
    Extent2D Size() const override { return extent; }
    Format ColorFormat() const override { return colorFormat; }
    u32 ImageCount() const override { return static_cast<u32>(colorTextures.size()); }
    bool BeginFrame() override;
    Texture& CurrentColorTexture() override;
    Texture& DepthTexture() override;
    void Submit(CommandList& commandList) override;
    bool Present() override;
    bool Resize(u32 width, u32 height) override;
    bool NeedsResize() const override { return false; }

    bool Create(u32 width, u32 height, PresentMode mode);
    void DestroySwapchainResources();
    void CreateSyncObjects();
};

struct DeviceImpl final : Device
{
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    u32 graphicsFamily = 0;
    u32 presentFamily = 0;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout frameSetLayout = VK_NULL_HANDLE;
    HashMap<u64, VkRenderPass> renderPassCache;
    AdapterInfo adapterInfo;
    bool hasSurface = false;
    u32 framesInFlight = 2;
    QueueImpl graphicsQueueWrapper;
    Scope<SwapChainImpl> swapchain;

    ~DeviceImpl() override;

    const AdapterInfo& Adapter() const override { return adapterInfo; }
    const char* BackendName() const override { return "Vulkan"; }
    SwapChain* CreateSwapChain(const SwapChainDesc& desc) override;
    Scope<Buffer> CreateBuffer(const BufferDesc& desc) override;
    Scope<Texture> CreateTexture(const TextureDesc& desc) override;
    Scope<Sampler> CreateSampler(const SamplerDesc& desc) override;
    Scope<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    Scope<CommandList> CreateCommandList() override;
    Scope<DescriptorHeap> CreateDescriptorHeap() override;
    Scope<Fence> CreateFence(bool signaled) override;
    Queue& GraphicsQueue() override { return graphicsQueueWrapper; }
    void WaitIdle() override;
    u32 FramesInFlight() const override { return framesInFlight; }

    VkRenderPass GetOrCreateRenderPass(Format colorFormat, Format depthFormat);
    bool Initialize(const DeviceDesc& desc);
    void Shutdown();
};

VkShaderModule CreateShaderModule(VkDevice device, const char* path);
} // namespace PRS::RHI::Vulkan
#endif // PRS_HAS_VULKAN

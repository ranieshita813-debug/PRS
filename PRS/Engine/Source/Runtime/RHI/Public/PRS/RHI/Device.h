#pragma once
#include "PRS/RHI/Buffer.h"
#include "PRS/RHI/CommandList.h"
#include "PRS/RHI/DescriptorHeap.h"
#include "PRS/RHI/Fence.h"
#include "PRS/RHI/Pipeline.h"
#include "PRS/RHI/Queue.h"
#include "PRS/RHI/Sampler.h"
#include "PRS/RHI/SwapChain.h"
#include "PRS/RHI/Texture.h"
#include "PRS/RHI/Types.h"
#include <string>

namespace PRS::RHI
{
struct DeviceDesc
{
    const char* applicationName = "PRS Application";
    u32 applicationVersion = 1;
    bool enableValidation = false;   // needs the Vulkan validation layers installed
    void* nativeWindow = nullptr;    // GLFWwindow* / ANativeWindow*; null = headless
    u32 width = 1280;
    u32 height = 720;
};

struct AdapterInfo
{
    std::string name = "unknown";
    u32 vendorId = 0;
    u32 deviceId = 0;
    u32 apiVersionMajor = 0;
    u32 apiVersionMinor = 0;
    u32 apiVersionPatch = 0;
    u64 deviceMemoryBytes = 0;
    bool discrete = false;
};

// The GPU and its resource factories. Create one with CreateDevice(); every
// resource the renderer uses comes from here, and the backend stays hidden
// behind these interfaces (Renderer never includes Vulkan headers).
class Device
{
public:
    virtual ~Device() = default;

    virtual const AdapterInfo& Adapter() const = 0;
    virtual const char* BackendName() const = 0;

    virtual SwapChain* CreateSwapChain(const SwapChainDesc& desc) = 0;
    virtual Scope<Buffer> CreateBuffer(const BufferDesc& desc) = 0;
    virtual Scope<Texture> CreateTexture(const TextureDesc& desc) = 0;
    virtual Scope<Sampler> CreateSampler(const SamplerDesc& desc) = 0;
    virtual Scope<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual Scope<CommandList> CreateCommandList() = 0;
    virtual Scope<DescriptorHeap> CreateDescriptorHeap() = 0;
    virtual Scope<Fence> CreateFence(bool signaled = false) = 0;

    virtual Queue& GraphicsQueue() = 0;
    virtual void WaitIdle() = 0;
    virtual u32 FramesInFlight() const = 0;
};

// Returns null (after logging why) when no backend is available, so an app can
// still run headless on machines without a Vulkan driver.
Scope<Device> CreateDevice(const DeviceDesc& desc);
} // namespace PRS::RHI

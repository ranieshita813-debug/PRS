#pragma once
#include "PRS/RHI/Types.h"

namespace PRS::RHI
{
class CommandList;
class Texture;

enum class PresentMode : u32
{
    Fifo = 0, // vsync
    Mailbox,
    Immediate
};

struct SwapChainDesc
{
    void* nativeWindow = nullptr; // GLFWwindow* / ANativeWindow*
    u32 width = 1280;
    u32 height = 720;
    PresentMode presentMode = PresentMode::Fifo;
    u32 framesInFlight = 2;
};

class SwapChain
{
public:
    virtual ~SwapChain() = default;

    virtual Extent2D Size() const = 0;
    virtual Format ColorFormat() const = 0;
    virtual u32 ImageCount() const = 0;

    // Waits for the frame that used this command list to finish, then acquires
    // the next presentable image. False means the swapchain is out of date.
    virtual bool BeginFrame() = 0;

    virtual Texture& CurrentColorTexture() = 0;
    virtual Texture& DepthTexture() = 0;

    // Submits the recorded frame with the acquire/present semaphores.
    virtual void Submit(CommandList& commandList) = 0;
    virtual bool Present() = 0;

    // Recreates the swapchain for a new window size.
    virtual bool Resize(u32 width, u32 height) = 0;

    virtual bool NeedsResize() const = 0;
};
} // namespace PRS::RHI

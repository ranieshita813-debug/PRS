#include "PRS/Engine/Application.h"
#include "PRS/Core/Log.h"
#include "PRS/Core/Platform.h"
#include "PRS/Core/Time/Clock.h"
#include "PRS/RHI/Device.h"
#include "PRS/RHI/SwapChain.h"
#include "PRS/Renderer/Renderer.h"
#include <algorithm>

namespace PRS
{
Application::Application(const AppConfig& config) : m_config(config)
{
    PRS_INFO("PRS Engine starting: %s", config.name);
    WindowDesc wd;
    wd.title = config.name;
    wd.width = config.width;
    wd.height = config.height;
    m_window = MakeScope<Window>(wd);
    m_jobs = MakeScope<JobSystem>();
    PRS_INFO("Job system: %u workers", m_jobs->WorkerCount());

    if (m_config.enableRendering) InitializeRendering();
}

Application::~Application()
{
    ShutdownRendering();
    m_jobs.reset();
    m_window.reset();
}

bool Application::InitializeRendering()
{
    RHI::DeviceDesc deviceDesc;
    deviceDesc.applicationName = m_config.name;
    deviceDesc.enableValidation = m_config.enableValidation;
    deviceDesc.nativeWindow = m_window->NativeHandle();
    deviceDesc.width = m_window->Width();
    deviceDesc.height = m_window->Height();

    m_device = RHI::CreateDevice(deviceDesc);
    if (!m_device)
    {
        PRS_WARN("Rendering disabled: no Vulkan device (check your driver / Vulkan SDK)");
        return false;
    }

    RHI::SwapChainDesc swapChainDesc;
    swapChainDesc.nativeWindow = m_window->NativeHandle();
    swapChainDesc.width = m_window->Width();
    swapChainDesc.height = m_window->Height();
    swapChainDesc.framesInFlight = 2;

    m_swapchain = m_device->CreateSwapChain(swapChainDesc);
    if (m_swapchain == nullptr)
    {
        PRS_WARN("Rendering disabled: could not create a swapchain");
        m_device.reset();
        return false;
    }

    Renderer::RendererConfig rendererConfig;
    if (m_config.shaderDirectory != nullptr) rendererConfig.shaderDirectory = m_config.shaderDirectory;

    m_renderer = MakeScope<Renderer::Renderer>();
    if (!m_renderer->Initialize(*m_device, *m_swapchain, rendererConfig))
    {
        PRS_WARN("Rendering disabled: renderer failed to initialise");
        m_renderer.reset();
        m_device->WaitIdle();
        m_swapchain = nullptr;
        m_device.reset();
        return false;
    }

    m_renderWidth = m_window->Width();
    m_renderHeight = m_window->Height();
    PRS_INFO("Rendering enabled on %s (%s)", m_device->Adapter().name.c_str(),
             m_device->BackendName());
    return true;
}

void Application::ShutdownRendering()
{
    if (m_renderer)
    {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    if (m_device)
    {
        m_device->WaitIdle();
        m_swapchain = nullptr;
        m_device.reset();
    }
}

int Application::Run()
{
    m_running = true;
    OnInit();

    Clock clock;
    f32 accumulator = 0.0f;

    while (m_running && !m_window->ShouldClose())
    {
        m_window->PollEvents();

        if (m_renderer)
        {
            const u32 width = m_window->Width();
            const u32 height = m_window->Height();
            if (width > 0 && height > 0 && (width != m_renderWidth || height != m_renderHeight))
            {
                if (m_renderer->Resize(width, height)) 
                {
                    m_renderWidth = width;
                    m_renderHeight = height;
                }
            }
        }

        f32 frameTime = std::min(clock.Tick(), m_config.maxFrameTime);
        accumulator += frameTime;

        while (accumulator >= m_config.fixedTimestep)
        {
            OnFixedUpdate(m_config.fixedTimestep);
            accumulator -= m_config.fixedTimestep;
        }

        OnUpdate(frameTime);
        OnRender(accumulator / m_config.fixedTimestep);
        // The swapchain fences the previous frame in BeginFrame(), so no
        // full-device wait is needed here.
    }

    m_jobs->WaitIdle();
    OnShutdown();
    ShutdownRendering();
    PRS_INFO("PRS Engine shut down cleanly");
    return 0;
}
} // namespace PRS

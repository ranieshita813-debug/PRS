#pragma once
#include "PRS/Core/Types.h"
#include "PRS/Jobs/JobSystem.h"
#include "PRS/Platform/Window.h"

namespace PRS
{
namespace RHI
{
class Device;
class SwapChain;
} // namespace RHI

namespace Renderer
{
class Renderer;
} // namespace Renderer

struct AppConfig
{
    const char* name = "PRS Application";
    u32 width = 1280;
    u32 height = 720;
    f32 fixedTimestep = 1.0f / 60.0f; // physics / gameplay tick
    f32 maxFrameTime = 0.25f;         // clamp to avoid spiral of death

    // Rendering is optional: if no Vulkan device can be created the app keeps
    // running and OnRender is still called (useful for headless/tests).
    bool enableRendering = true;
    bool enableValidation = false;      // Vulkan validation layers
    const char* shaderDirectory = nullptr; // null = search the default locations
};

// Derive from this in your game and implement the hooks.
class Application
{
public:
    explicit Application(const AppConfig& config);
    virtual ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int Run();
    void Quit() { m_running = false; }

    Window& GetWindow() { return *m_window; }
    JobSystem& Jobs() { return *m_jobs; }

    RHI::Device* GetDevice() { return m_device.get(); }
    Renderer::Renderer* GetRenderer() { return m_renderer.get(); }
    bool IsRenderingEnabled() const { return m_renderer != nullptr; }

protected:
    virtual void OnInit() {}
    virtual void OnFixedUpdate(f32 /*dt*/) {} // fixed rate
    virtual void OnUpdate(f32 /*dt*/) {}      // every frame
    virtual void OnRender(f32 /*alpha*/) {}   // alpha = interpolation factor between fixed steps
    virtual void OnShutdown() {}

private:
    bool InitializeRendering();
    void ShutdownRendering();

    AppConfig m_config;
    Scope<Window> m_window;
    Scope<JobSystem> m_jobs;
    Scope<RHI::Device> m_device;
    RHI::SwapChain* m_swapchain = nullptr;
    Scope<Renderer::Renderer> m_renderer;
    u32 m_renderWidth = 0;
    u32 m_renderHeight = 0;
    bool m_running = false;
};

// Implemented by the game: return a new instance of your Application subclass.
Scope<Application> CreateApplication();
} // namespace PRS

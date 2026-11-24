#pragma once
#include "PRS/Core/Types.h"
#include "PRS/Platform/Window.h"
#include "PRS/Jobs/JobSystem.h"

namespace PRS
{
struct AppConfig
{
    const char* name = "PRS Application";
    u32 width = 1280;
    u32 height = 720;
    f32 fixedTimestep = 1.0f / 60.0f; // physics / gameplay tick
    f32 maxFrameTime = 0.25f;          // clamp to avoid spiral of death
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

protected:
    virtual void OnInit() {}
    virtual void OnFixedUpdate(f32 /*dt*/) {}  // fixed rate
    virtual void OnUpdate(f32 /*dt*/) {}       // every frame
    virtual void OnRender(f32 /*alpha*/) {}    // alpha = interpolation factor between fixed steps
    virtual void OnShutdown() {}

private:
    AppConfig m_config;
    Scope<Window> m_window;
    Scope<JobSystem> m_jobs;
    bool m_running = false;
};

// Implemented by the game: return a new instance of your Application subclass.
Scope<Application> CreateApplication();
} // namespace PRS

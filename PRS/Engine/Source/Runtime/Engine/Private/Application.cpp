#include "PRS/Engine/Application.h"
#include "PRS/Core/Log.h"
#include "PRS/Core/Time/Clock.h"
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
}

Application::~Application() = default;

int Application::Run()
{
    m_running = true;
    OnInit();

    Clock clock;
    f32 accumulator = 0.0f;

    while (m_running && !m_window->ShouldClose())
    {
        m_window->PollEvents();

        f32 frameTime = std::min(clock.Tick(), m_config.maxFrameTime);
        accumulator += frameTime;

        while (accumulator >= m_config.fixedTimestep)
        {
            OnFixedUpdate(m_config.fixedTimestep);
            accumulator -= m_config.fixedTimestep;
        }

        OnUpdate(frameTime);
        OnRender(accumulator / m_config.fixedTimestep);
    }

    m_jobs->WaitIdle();
    OnShutdown();
    PRS_INFO("PRS Engine shut down cleanly");
    return 0;
}
} // namespace PRS

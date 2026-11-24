#include "PRS/Engine/Application.h"
#include "PRS/Engine/Entry.h"   // provides main()
#include "PRS/Platform/Input/Input.h"
#include "PRS/Core/Log.h"
#include "PRS/Core/Memory/LinearAllocator.h"

using namespace PRS;

class MyOpenWorldApp : public Application
{
public:
    MyOpenWorldApp() : Application(MakeConfig()), m_frameMemory(1024 * 1024) {}

protected:
    void OnInit() override { PRS_INFO("MyOpenWorld initialised. Press ESC to quit."); }

    void OnFixedUpdate(f32 /*dt*/) override { ++m_fixedTicks; }

    void OnUpdate(f32 dt) override
    {
        m_frameMemory.Reset(); // per-frame scratch memory
        m_timer += dt;
        ++m_frames;

        Window& w = GetWindow();
        if (Input::IsKeyDown(w, Key::Escape)) Quit();
        if (Input::IsKeyDown(w, Key::W)) PRS_TRACE("move forward");

        if (m_timer >= 1.0f)
        {
            PRS_INFO("FPS: %u | fixed ticks/s: %u", m_frames, m_fixedTicks);
            m_timer = 0; m_frames = 0; m_fixedTicks = 0;
        }
    }

    void OnRender(f32 /*alpha*/) override { /* Phase 1: Renderer draws the frame here */ }

private:
    static AppConfig MakeConfig()
    {
        AppConfig c;
        c.name = "MyOpenWorld (PRS)";
        return c;
    }
    LinearAllocator m_frameMemory;
    f32 m_timer = 0;
    u32 m_frames = 0, m_fixedTicks = 0;
};

Scope<Application> PRS::CreateApplication() { return MakeScope<MyOpenWorldApp>(); }

#include "PRS/Core/Platform.h"
#include "PRS/Core/Log.h"
#include "PRS/Engine/Application.h"
#include "PRS/Engine/Entry.h" // desktop main(); Android uses android_main in Platform
#include "PRS/Platform/Input/Input.h"
#include "PRS/Platform/Touch.h"
#include "PRS/UI/Context.h"
#include "PRS/UI/Font.h"
#include "PRS/UI/SoftwareCanvas.h"

#include "MobileShellUI.h"
#include "Presenter.h"

namespace
{
using namespace PRS;

class MobileShellApp final : public Application
{
public:
    MobileShellApp() : Application(MakeConfig()) {}

protected:
    void OnInit() override
    {
        m_state.Notify("SHELL READY");
        PRS_INFO("MobileShell: %ux%u @ density %.2f", GetWindow().Width(), GetWindow().Height(),
                 DensityForWindow());
        PRS_INFO("Desktop note: run the UIPreview tool to render the interface to PNG");
    }

    void OnUpdate(f32 dt) override
    {
        m_state.toastTimer = UI::Max(0.0f, m_state.toastTimer - dt);

        Window& window = GetWindow();
        if (window.Width() == 0 || window.Height() == 0) return;

        PumpPointer();

        m_ui.SetTheme(UI::Theme::PhoneDark(DensityForWindow()));
        const UI::Vec2 size(static_cast<f32>(window.Width()), static_cast<f32>(window.Height()));
        const UI::Rect surface(0.0f, 0.0f, size.x, size.y);

        m_ui.BeginFrame(size, m_input, dt);
        MobileShell::Draw(m_ui, surface, m_state);
        m_ui.EndFrame();

        // Rasterise + present. Phase 1 replaces this with the Vulkan UI pass.
        m_canvas.Resize(window.Width(), window.Height());
        m_canvas.Clear(m_ui.GetTheme().background);
        m_canvas.Render(m_ui.Draws(), UI::BitmapFont::Default());
        MobileShell::PresentToWindow(window.NativeHandle(), m_canvas);
    }

    void OnRender(f32 /*alpha*/) override
    {
        // The Vulkan renderer will draw m_ui.Draws() here once the RHI lands.
    }

    void OnShutdown() override { PRS_INFO("MobileShell shut down"); }

private:
    static AppConfig MakeConfig()
    {
        AppConfig config;
        config.name = "PRS Mobile Shell";
        config.width = 1080;
        config.height = 2400;
        return config;
    }

    f32 DensityForWindow()
    {
        // Treat 800dp of height as the design target so the layout keeps the same
        // physical proportions on phones and tablets.
        const f32 raw = static_cast<f32>(GetWindow().Height()) / 800.0f;
        const f32 rounded = static_cast<f32>(static_cast<i32>(raw * 2.0f + 0.5f)) * 0.5f;
        return UI::Clamp(rounded, 1.0f, 4.0f);
    }

    void PumpPointer()
    {
#if defined(PRS_PLATFORM_ANDROID)
        Platform::Touch::Point points[16];
        const u32 count = Platform::Touch::Poll(points, 16);
        for (u32 i = 0; i < count; ++i)
        {
            const UI::Vec2 position(points[i].x, points[i].y);
            switch (points[i].phase)
            {
            case Platform::Touch::Phase::Down: m_input.OnPointerDown(points[i].id, position); break;
            case Platform::Touch::Phase::Move: m_input.OnPointerMove(points[i].id, position); break;
            case Platform::Touch::Phase::Up: m_input.OnPointerUp(points[i].id, position); break;
            }
        }

        if (Platform::Touch::BackPressed()) HandleBack();
#else
        // Desktop: the mouse drives the same single-pointer path as a finger.
        Window& window = GetWindow();
        double mx = 0.0;
        double my = 0.0;
        Input::GetMousePosition(window, mx, my);
        const UI::Vec2 position(static_cast<f32>(mx), static_cast<f32>(my));
        const bool down = Input::IsMouseDown(window, MouseButton::Left);

        if (down && !m_pointerDown)
        {
            m_input.OnPointerDown(0, position);
            m_pointerDown = true;
        }
        else if (down)
        {
            m_input.OnPointerMove(0, position);
        }
        else if (!down && m_pointerDown)
        {
            m_input.OnPointerUp(0, position);
            m_pointerDown = false;
        }

        if (Input::IsKeyDown(window, Key::Escape)) GetWindow().RequestClose();
#endif
    }

    void HandleBack()
    {
        if (m_state.tab != MobileShell::Tab::Home)
        {
            m_state.tab = MobileShell::Tab::Home;
            m_state.Notify("HOME");
            return;
        }
        GetWindow().RequestClose();
    }

    UI::Context m_ui;
    UI::InputState m_input;
    UI::SoftwareCanvas m_canvas;
    MobileShell::State m_state;
    bool m_pointerDown = false;
};
} // namespace

Scope<Application> PRS::CreateApplication() { return MakeScope<MobileShellApp>(); }

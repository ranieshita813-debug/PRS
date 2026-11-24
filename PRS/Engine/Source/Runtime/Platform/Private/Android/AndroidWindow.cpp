#include "PRS/Core/Platform.h"

#if defined(PRS_PLATFORM_ANDROID)

#include "AndroidState.h"
#include "PRS/Core/Log.h"
#include "PRS/Engine/Application.h"
#include "PRS/Platform/Window.h"

#include <android/window.h>
#include <cstring>

namespace PRS::Android
{
namespace
{
State g_state;
ANativeWindow* g_pendingWindow = nullptr;

void PushPointerEvent(AInputEvent* event, size_t index, Touch::Phase phase)
{
    const i32 id = AMotionEvent_getPointerId(event, index);
    const f32 x = AMotionEvent_getX(event, index);
    const f32 y = AMotionEvent_getY(event, index);
    g_state.PushTouch(id, x, y, phase);
}

i32 HandleInput(android_app*, AInputEvent* event)
{
    const i32 type = AInputEvent_getType(event);

    if (type == AINPUT_EVENT_TYPE_MOTION)
    {
        const i32 action = AMotionEvent_getAction(event);
        const i32 masked = action & AMOTION_EVENT_ACTION_MASK;
        const size_t pointerIndex =
            static_cast<size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

        switch (masked)
        {
        case AMOTION_EVENT_ACTION_DOWN:
            PushPointerEvent(event, 0, Touch::Phase::Down);
            break;
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            PushPointerEvent(event, pointerIndex, Touch::Phase::Down);
            break;
        case AMOTION_EVENT_ACTION_MOVE:
        {
            const size_t count = AMotionEvent_getPointerCount(event);
            for (size_t i = 0; i < count; ++i) PushPointerEvent(event, i, Touch::Phase::Move);
            break;
        }
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            PushPointerEvent(event, pointerIndex, Touch::Phase::Up);
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
        {
            const size_t count = AMotionEvent_getPointerCount(event);
            for (size_t i = 0; i < count; ++i) PushPointerEvent(event, i, Touch::Phase::Up);
            break;
        }
        default:
            break;
        }
        return 1;
    }

    if (type == AINPUT_EVENT_TYPE_KEY)
    {
        const i32 code = AKeyEvent_getKeyCode(event);
        if (code == AKEYCODE_BACK && AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP)
        {
            // Consumed here so the app can handle back navigation itself; the
            // game calls Window::RequestClose() when it wants to exit.
            g_state.backPressed = true;
            return 1;
        }
    }

    return 0;
}

void HandleCommand(android_app* app, i32 command)
{
    switch (command)
    {
    case APP_CMD_INIT_WINDOW:
        g_state.window = app->window;
        g_state.windowReady = app->window != nullptr;
        PRS_INFO("Android window ready");
        break;
    case APP_CMD_TERM_WINDOW:
        g_state.windowReady = false;
        g_state.window = app->window;
        break;
    case APP_CMD_GAINED_FOCUS:
    case APP_CMD_RESUME:
        g_state.resumed = true;
        break;
    case APP_CMD_LOST_FOCUS:
    case APP_CMD_PAUSE:
        g_state.resumed = false;
        break;
    case APP_CMD_DESTROY:
        g_state.closeRequested = true;
        break;
    default:
        break;
    }
}
} // namespace

State& Get() { return g_state; }

void SetPendingWindow(ANativeWindow* window) { g_pendingWindow = window; }

ANativeWindow* ConsumePendingWindow()
{
    ANativeWindow* window = g_pendingWindow ? g_pendingWindow : g_state.window;
    g_pendingWindow = nullptr;
    return window;
}

void InstallCallbacks(android_app* app)
{
    g_state.app = app;
    app->onAppCmd = HandleCommand;
    app->onInputEvent = HandleInput;
}

void PumpEvents()
{
    if (!g_state.app) return;

    // While the app is paused, block on the looper instead of spinning so the
    // device can sleep. Events resume as soon as the window comes back.
    for (;;)
    {
        int events = 0;
        android_poll_source* source = nullptr;
        const int timeout = g_state.resumed ? 0 : -1;
        const int ident = ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source));

        if (ident < 0) break;
        if (source) source->process(g_state.app, source);

        if (g_state.app->destroyRequested)
        {
            g_state.closeRequested = true;
            break;
        }
        if (timeout == 0 && ident == ALOOPER_POLL_TIMEOUT) break;
        if (g_state.resumed) continue;
        if (ident == ALOOPER_POLL_TIMEOUT || ident == ALOOPER_POLL_WAKE) break;
    }

    if (g_state.window)
    {
        const u32 width = static_cast<u32>(ANativeWindow_getWidth(g_state.window));
        const u32 height = static_cast<u32>(ANativeWindow_getHeight(g_state.window));
        if (width > 0 && height > 0)
        {
            g_state.width = width;
            g_state.height = height;
        }
    }
}
} // namespace PRS::Android

namespace PRS
{
Window::Window(const WindowDesc& desc) : m_width(desc.width), m_height(desc.height)
{
    m_handle = Android::ConsumePendingWindow();

    ANativeWindow* window = static_cast<ANativeWindow*>(m_handle);
    if (window)
    {
        m_width = static_cast<u32>(ANativeWindow_getWidth(window));
        m_height = static_cast<u32>(ANativeWindow_getHeight(window));
        Android::Get().width = m_width;
        Android::Get().height = m_height;
    }

    PRS_INFO("Android window: %ux%u", m_width, m_height);
}

Window::~Window() { m_handle = nullptr; }

void Window::PollEvents()
{
    Android::PumpEvents();

    Android::State& state = Android::Get();
    if (state.window && state.windowReady)
    {
        m_handle = state.window;
        const u32 width = static_cast<u32>(ANativeWindow_getWidth(state.window));
        const u32 height = static_cast<u32>(ANativeWindow_getHeight(state.window));
        if (width > 0 && height > 0)
        {
            m_width = width;
            m_height = height;
        }
    }
}

bool Window::ShouldClose() const { return Android::Get().closeRequested; }

void Window::RequestClose() { Android::Get().closeRequested = true; }
} // namespace PRS

// Native entry point provided by android_native_app_glue. The game supplies
// PRS::CreateApplication() exactly as it does on desktop.
extern "C" void android_main(android_app* app)
{
    PRS::Android::InstallCallbacks(app);
    ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    // Wait for the first surface so the engine's Window can bind it.
    while (!app->window && !app->destroyRequested)
    {
        int events = 0;
        android_poll_source* source = nullptr;
        const int ident = ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source));
        if (ident >= 0 && source) source->process(app, source);
    }

    if (app->destroyRequested) return;

    PRS::Android::SetPendingWindow(app->window);
    PRS::Android::Get().window = app->window;
    PRS::Android::Get().windowReady = true;

    {
        auto application = PRS::CreateApplication();
        application->Run();
    }

    ANativeActivity_finish(app->activity);
}

#endif // PRS_PLATFORM_ANDROID

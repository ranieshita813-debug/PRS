#include "Presenter.h"
#include "PRS/Core/Platform.h"

#if defined(PRS_PLATFORM_ANDROID)
#include <android/native_window.h>
#include <cstring>
#endif

namespace MobileShell
{
#if defined(PRS_PLATFORM_ANDROID)
void PresentToWindow(void* nativeHandle, const PRS::UI::SoftwareCanvas& canvas)
{
    if (!nativeHandle || !canvas.Pixels()) return;

    ANativeWindow* window = static_cast<ANativeWindow*>(nativeHandle);
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, nullptr) != 0) return;

    if (buffer.format == WINDOW_FORMAT_RGBA_8888 || buffer.format == WINDOW_FORMAT_RGBX_8888)
    {
        const PRS::u32 width = canvas.Width() < static_cast<PRS::u32>(buffer.width)
                                   ? canvas.Width()
                                   : static_cast<PRS::u32>(buffer.width);
        const PRS::u32 height = canvas.Height() < static_cast<PRS::u32>(buffer.height)
                                    ? canvas.Height()
                                    : static_cast<PRS::u32>(buffer.height);

        for (PRS::u32 y = 0; y < height; ++y)
        {
            const PRS::u8* source = canvas.Pixels() + static_cast<size_t>(y) * canvas.Stride();
            PRS::u8* destination = static_cast<PRS::u8*>(buffer.bits) +
                                   static_cast<size_t>(y) * buffer.stride * 4u;
            std::memcpy(destination, source, static_cast<size_t>(width) * 4u);
        }
    }

    ANativeWindow_unlockAndPost(window);
}
#else
void PresentToWindow(void*, const PRS::UI::SoftwareCanvas&) {}
#endif
} // namespace MobileShell

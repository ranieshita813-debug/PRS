#pragma once
#include "PRS/UI/SoftwareCanvas.h"

namespace MobileShell
{
// Copies the rasterised interface into the window's native surface.
// Android: ANativeWindow_lock()/unlockAndPost() (software presentation, no GPU
// needed while the Vulkan RHI is still a stub). Desktop: no-op.
void PresentToWindow(void* nativeHandle, const PRS::UI::SoftwareCanvas& canvas);
} // namespace MobileShell

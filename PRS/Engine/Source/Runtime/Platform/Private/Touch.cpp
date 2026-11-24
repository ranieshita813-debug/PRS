#include "PRS/Platform/Touch.h"
#include "PRS/Core/Platform.h"

#if !defined(PRS_PLATFORM_ANDROID)

namespace PRS::Platform::Touch
{
u32 Poll(Point*, u32) { return 0; }
bool BackPressed() { return false; }
bool HasTouchInput() { return false; }
} // namespace PRS::Platform::Touch

#endif // !PRS_PLATFORM_ANDROID

#include "PRS/Core/Log.h"
#include "PRS/RHI/Device.h"

namespace PRS::RHI
{
const char* ModuleName() { return "RHI"; }

#if !defined(PRS_HAS_VULKAN)
Scope<Device> CreateDevice(const DeviceDesc& desc)
{
    (void)desc;
    PRS_WARN("PRS was built without Vulkan support; rendering is disabled");
    return nullptr;
}
#endif
} // namespace PRS::RHI

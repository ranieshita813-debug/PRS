#pragma once
#include "PRS/Core/Containers/Containers.h"
#include "PRS/ECS/World.h"

namespace PRS::ECS
{
// Fixed stages every frame runs through, in order.
enum class SystemStage : u8
{
    PreUpdate = 0,
    Update,
    PostUpdate,
    Render,
    Count
};

struct SystemContext
{
    World& world;
    f32 deltaTime = 0.0f;
    f32 totalTime = 0.0f;
    u64 frame = 0;
};

// Interface for a named, stateful system. Function systems (lambda based) can be
// registered directly on the scheduler; this is for systems that own data.
class System
{
public:
    virtual ~System() = default;
    virtual const char* Name() const { return "System"; }
    virtual void OnUpdate(SystemContext& context) = 0;
};
} // namespace PRS::ECS

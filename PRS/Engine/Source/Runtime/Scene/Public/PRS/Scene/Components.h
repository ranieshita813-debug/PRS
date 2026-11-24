#pragma once
#include "PRS/Core/Math/Transform.h"
#include "PRS/ECS/Entity.h"
#include "PRS/ECS/World.h"
#include <string>
#include <vector>

namespace PRS::Scene
{
using Transform = PRS::Transform;

struct Name
{
    std::string value = "Entity";
};

struct Parent
{
    ECS::Entity entity{};
};

struct Children
{
    std::vector<ECS::Entity> list;
};

struct Camera
{
    f32 fovYDegrees = 60.0f;
    f32 nearZ = 0.1f;
    f32 farZ = 1000.0f;
    f32 aspect = 16.0f / 9.0f;
    bool primary = false;

    Mat4 Projection() const
    {
        return Mat4::Perspective(Radians(fovYDegrees), aspect > 0.0f ? aspect : 1.0f, nearZ, farZ);
    }

    Mat4 View(const Transform& cameraTransform) const
    {
        const Vec3 eye = cameraTransform.position;
        return Mat4::LookAt(eye, eye + cameraTransform.Forward(), cameraTransform.Up());
    }
};

struct MeshRenderer
{
    std::string mesh;
    std::string material;
    bool visible = true;
    bool castShadows = true;
};

enum class LightType : u8
{
    Directional = 0,
    Point,
    Spot
};

struct Light
{
    LightType type = LightType::Directional;
    Vec3 color{1.0f, 1.0f, 1.0f};
    f32 intensity = 1.0f;
    f32 range = 25.0f;
    f32 innerConeDegrees = 20.0f;
    f32 outerConeDegrees = 30.0f;
    bool castShadows = true;
};

// Marks an entity as excluded from rendering and simulation (pooled/disabled).
struct Disabled
{
};
} // namespace PRS::Scene

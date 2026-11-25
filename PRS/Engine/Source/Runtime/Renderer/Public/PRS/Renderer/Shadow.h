#pragma once
#include "PRS/Core/Math/Matrix.h"
#include "PRS/Core/Math/Vector.h"

namespace PRS::Renderer
{
// The volume one directional shadow map covers: a sphere around `center`. A
// sphere keeps the light matrix stable while the camera moves, which avoids the
// shimmering a camera-fitted box produces.
struct ShadowBounds
{
    Vec3 center{};
    f32 radius = 12.0f;
};

// Fits an orthographic light matrix around `bounds`.
//
// `directionTowardsLight` is the unit vector the shading term uses (from the
// scene towards the sun). The light "camera" sits radius + depthPadding behind
// the centre and looks back at it, so everything inside the sphere, plus
// casters depthPadding outside it, lands inside the depth range.
inline Mat4 MakeDirectionalLightMatrix(const Vec3& directionTowardsLight, const ShadowBounds& bounds,
                                       f32 depthPadding = 10.0f)
{
    Vec3 direction = Normalize(directionTowardsLight);
    if (LengthSquared(direction) < 1.0e-8f) direction = AxisY();

    const f32 radius = bounds.radius > 0.1f ? bounds.radius : 0.1f;
    const f32 padding = depthPadding > 0.1f ? depthPadding : 0.1f;

    // Looking straight down or up would make LookAt build a degenerate basis.
    const Vec3 up = Abs(direction.y) > 0.99f ? AxisZ() : AxisY();

    const Vec3 eye = bounds.center + direction * (radius + padding);
    const Mat4 view = Mat4::LookAt(eye, bounds.center, up);

    // nearZ = 0 puts the light camera at the eye; the range reaches twice the
    // distance so the centre sits in the middle of [0, 1] depth.
    const Mat4 projection =
        Mat4::Orthographic(-radius, radius, -radius, radius, 0.0f, 2.0f * (radius + padding));
    return projection * view;
}

// Bounds containing an axis-aligned box (world-space caster extents).
inline ShadowBounds MakeShadowBounds(const Vec3& minimum, const Vec3& maximum)
{
    ShadowBounds bounds;
    bounds.center = (minimum + maximum) * 0.5f;
    const Vec3 halfExtent = maximum - bounds.center;
    const f32 radius = Length(halfExtent);
    bounds.radius = radius > 1.0f ? radius : 1.0f;
    return bounds;
}
} // namespace PRS::Renderer

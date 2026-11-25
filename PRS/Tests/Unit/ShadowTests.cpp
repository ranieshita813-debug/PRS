#include "PRS/Renderer/Shadow.h"
#include "TestFramework.h"

using namespace PRS;
using namespace PRS::Renderer;

namespace
{
// Depth as the rasterizer stores it: clip z / w, mapped to [0, 1] by Vulkan.
f32 ProjectedDepth(const Mat4& matrix, const Vec3& position)
{
    const Vec4 clip = matrix * Vec4(position, 1.0f);
    return clip.z / clip.w;
}

Vec2 ProjectedXY(const Mat4& matrix, const Vec3& position)
{
    const Vec4 clip = matrix * Vec4(position, 1.0f);
    return {clip.x / clip.w, clip.y / clip.w};
}
} // namespace

TEST(Shadow_LightMatrixKeepsCenterInsideTheMap)
{
    ShadowBounds bounds;
    bounds.center = {0.0f, 1.0f, 0.0f};
    bounds.radius = 10.0f;

    const Mat4 matrix = MakeDirectionalLightMatrix(Normalize(Vec3{0.4f, 0.8f, 0.3f}), bounds);

    const Vec2 ndc = ProjectedXY(matrix, bounds.center);
    CHECK(NearlyEqual(ndc.x, 0.0f, 1.0e-4f));
    CHECK(NearlyEqual(ndc.y, 0.0f, 1.0e-4f));

    const f32 depth = ProjectedDepth(matrix, bounds.center);
    CHECK(depth > 0.0f);
    CHECK(depth < 1.0f);
}

TEST(Shadow_EveryPointInBoundsCoversTheMap)
{
    ShadowBounds bounds;
    bounds.center = {3.0f, 2.0f, -1.0f};
    bounds.radius = 6.0f;

    const Mat4 matrix = MakeDirectionalLightMatrix(Normalize(Vec3{-0.3f, 0.7f, 0.5f}), bounds);

    // Points on the sphere's surface must land inside the [-1, 1] box, otherwise
    // the shadow map would clip casters.
    const Vec3 offsets[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const Vec3& offset : offsets)
    {
        const Vec3 point = bounds.center + Normalize(offset) * bounds.radius;
        const Vec2 ndc = ProjectedXY(matrix, point);
        CHECK(ndc.x >= -1.0001f && ndc.x <= 1.0001f);
        CHECK(ndc.y >= -1.0001f && ndc.y <= 1.0001f);

        const f32 depth = ProjectedDepth(matrix, point);
        CHECK(depth >= 0.0f && depth <= 1.0001f);
    }
}

TEST(Shadow_PointsTowardsTheLightGetSmallerDepth)
{
    ShadowBounds bounds;
    bounds.center = {};
    bounds.radius = 8.0f;

    const Vec3 towardsLight = Normalize(Vec3{0.0f, 1.0f, 0.0f});
    const Mat4 matrix = MakeDirectionalLightMatrix(towardsLight, bounds);

    const f32 lower = ProjectedDepth(matrix, Vec3{0.0f, -4.0f, 0.0f});
    const f32 upper = ProjectedDepth(matrix, Vec3{0.0f, 4.0f, 0.0f});
    CHECK(upper < lower); // closer to the sun means a smaller stored depth
}

TEST(Shadow_StraightDownLightStillHasAValidBasis)
{
    ShadowBounds bounds;
    bounds.center = {1.0f, 0.0f, 1.0f};
    bounds.radius = 5.0f;

    const Mat4 matrix = MakeDirectionalLightMatrix(Vec3{0.0f, 1.0f, 0.0f}, bounds);
    const Vec2 ndc = ProjectedXY(matrix, bounds.center);
    CHECK(NearlyEqual(ndc.x, 0.0f, 1.0e-4f));
    CHECK(NearlyEqual(ndc.y, 0.0f, 1.0e-4f));
}

TEST(Shadow_ZeroDirectionFallsBackInsteadOfProducingNaN)
{
    ShadowBounds bounds;
    const Mat4 matrix = MakeDirectionalLightMatrix(Vec3{0.0f, 0.0f, 0.0f}, bounds);
    const Vec4 clip = matrix * Vec4{bounds.center, 1.0f};
    CHECK(clip.x == clip.x); // no NaNs
    CHECK(clip.y == clip.y);
    CHECK(clip.z == clip.z);
    CHECK(clip.w == clip.w);
    CHECK(clip.w != 0.0f);
}

TEST(Shadow_BoundsContainTheWholeBox)
{
    const ShadowBounds bounds =
        MakeShadowBounds(Vec3{-2.0f, 0.0f, -4.0f}, Vec3{6.0f, 3.0f, 2.0f});

    CHECK(NearlyEqual(bounds.center.x, 2.0f));
    CHECK(NearlyEqual(bounds.center.y, 1.5f));
    CHECK(NearlyEqual(bounds.center.z, -1.0f));

    const f32 expectedRadius = Length(Vec3{4.0f, 1.5f, 3.0f});
    CHECK(NearlyEqual(bounds.radius, expectedRadius, 1.0e-4f));
}

#include "TestFramework.h"
#include "PRS/Core/Math/Matrix.h"
#include "PRS/Core/Math/Quat.h"
#include "PRS/Core/Math/Transform.h"
#include "PRS/Core/Math/Vector.h"

using namespace PRS;

TEST(Vec3_BasicGeometry)
{
    const Vec3 a{1.0f, 0.0f, 0.0f};
    const Vec3 b{0.0f, 1.0f, 0.0f};
    CHECK(NearlyEqual(Dot(a, b), 0.0f));
    CHECK(Cross(a, b) == Vec3(0.0f, 0.0f, 1.0f));
    CHECK(NearlyEqual(Length(Vec3{3.0f, 4.0f, 0.0f}), 5.0f));
    CHECK(NearlyEqual(Length(Normalize(Vec3{2.0f, 0.0f, 0.0f})), 1.0f));
    CHECK(NearlyEqual(Distance(Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 2.0f, 2.0f}), 3.0f));
}

TEST(Mat4_TransformPointAndInverse)
{
    const Mat4 translation = Mat4::Translation({2.0f, 3.0f, -1.0f});
    const Vec3 moved = translation.TransformPoint({1.0f, 1.0f, 1.0f});
    CHECK(NearlyEqual(moved.x, 3.0f) && NearlyEqual(moved.y, 4.0f) && NearlyEqual(moved.z, 0.0f));

    const Mat4 model = Mat4::Translation({1.0f, -2.0f, 0.5f}) * Mat4::Rotation(0.7f, AxisY()) *
                       Mat4::Scale({2.0f, 2.0f, 2.0f});
    const Mat4 inverse = model.Inverted();
    const Mat4 identity = model * inverse;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            CHECK(NearlyEqual(identity.At(row, column), row == column ? 1.0f : 0.0f));

    const Vec3 point{0.3f, -1.2f, 4.0f};
    const Vec3 roundTrip = inverse.TransformPoint(model.TransformPoint(point));
    CHECK(NearlyEqual(roundTrip.x, point.x) && NearlyEqual(roundTrip.y, point.y) &&
          NearlyEqual(roundTrip.z, point.z));
}

TEST(Quat_RotatesAndSlerps)
{
    const Quat quarterTurn = Quat::FromAxisAngle(AxisY(), HalfPi);
    const Vec3 rotated = quarterTurn.Rotate(Vec3{1.0f, 0.0f, 0.0f});
    CHECK(NearlyEqual(rotated.x, 0.0f, 1.0e-4f));
    CHECK(NearlyEqual(rotated.z, -1.0f, 1.0e-4f));

    const Quat halfway = Quat::Slerp(Quat::Identity(), quarterTurn, 0.5f);
    CHECK(NearlyEqual(halfway.Length(), 1.0f, 1.0e-4f));
    const Vec3 diagonal = halfway.Rotate(Vec3{1.0f, 0.0f, 0.0f});
    CHECK(NearlyEqual(diagonal.x, diagonal.z * -1.0f, 1.0e-4f));

    const Mat4 matrix = quarterTurn.ToMatrix();
    const Vec3 fromMatrix = matrix.TransformDirection(Vec3{1.0f, 0.0f, 0.0f});
    CHECK(NearlyEqual(fromMatrix.z, -1.0f, 1.0e-4f));
}

TEST(Transform_ComposesLikeMatrices)
{
    Transform parent;
    parent.position = {10.0f, 0.0f, 0.0f};

    Transform child;
    child.position = {0.0f, 2.0f, 0.0f};
    child.rotation = Quat::FromAxisAngle(AxisY(), HalfPi);
    child.scale = {2.0f, 2.0f, 2.0f};

    const Mat4 childMatrix = child.Matrix();
    const Vec3 viaMatrix = parent.TransformPoint(childMatrix.TransformPoint({1.0f, 0.0f, 0.0f}));
    const Vec3 viaTransforms = parent.TransformPoint(child.TransformPoint({1.0f, 0.0f, 0.0f}));
    CHECK(NearlyEqual(viaMatrix.x, viaTransforms.x, 1.0e-4f));
    CHECK(NearlyEqual(viaMatrix.z, viaTransforms.z, 1.0e-4f));
}

TEST(Mat4_LookAtMapsTheCameraFrameCorrectly)
{
    const Vec3 eye{3.0f, 2.0f, -4.0f};
    const Vec3 target{1.0f, 0.5f, 2.0f};
    const Mat4 view = Mat4::LookAt(eye, target, AxisY());

    const Vec3 viewSpaceEye = (view * Vec4{eye, 1.0f}).XYZ();
    CHECK(NearlyEqual(viewSpaceEye.x, 0.0f, 1.0e-4f));
    CHECK(NearlyEqual(viewSpaceEye.y, 0.0f, 1.0e-4f));
    CHECK(NearlyEqual(viewSpaceEye.z, 0.0f, 1.0e-4f));

    // The target sits straight ahead, down the negative z axis.
    const Vec3 viewSpaceTarget = (view * Vec4{target, 1.0f}).XYZ();
    CHECK(NearlyEqual(viewSpaceTarget.x, 0.0f, 1.0e-4f));
    CHECK(NearlyEqual(viewSpaceTarget.y, 0.0f, 1.0e-4f));
    CHECK(viewSpaceTarget.z < 0.0f);
    CHECK(NearlyEqual(viewSpaceTarget.z, -Distance(eye, target), 1.0e-4f));

    // Moving along the camera's right axis moves the point along +x only.
    const Vec3 forward = Normalize(target - eye);
    const Vec3 right = Normalize(Cross(forward, AxisY()));
    const Vec3 viewSpaceRight = (view * Vec4{eye + right * 2.0f, 1.0f}).XYZ();
    CHECK(NearlyEqual(viewSpaceRight.x, 2.0f, 1.0e-4f));
    CHECK(NearlyEqual(viewSpaceRight.y, 0.0f, 1.0e-4f));
}

TEST(Mat4_PerspectiveMapsDepthToZeroOne)
{
    const Mat4 projection = Mat4::Perspective(Radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    const Vec4 nearClip = projection * Vec4{0.0f, 0.0f, -0.1f, 1.0f};
    const Vec4 farClip = projection * Vec4{0.0f, 0.0f, -100.0f, 1.0f};
    CHECK(NearlyEqual(nearClip.z / nearClip.w, 0.0f, 1.0e-3f));
    CHECK(NearlyEqual(farClip.z / farClip.w, 1.0f, 1.0e-3f));
}

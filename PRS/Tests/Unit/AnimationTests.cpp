#include "TestFramework.h"
#include "PRS/Animation/AnimationGraph.h"
#include "PRS/Animation/Clip.h"
#include "PRS/Animation/GPUSkinning.h"
#include "PRS/Animation/Retargeting.h"

using namespace PRS;
using namespace PRS::Animation;

namespace
{
Skeleton MakeSkeleton()
{
    Skeleton skeleton;
    Transform root;
    skeleton.AddBone("root", -1, root);

    Transform spine;
    spine.position = {0.0f, 1.0f, 0.0f};
    skeleton.AddBone("spine", 0, spine);

    Transform head;
    head.position = {0.0f, 0.5f, 0.0f};
    skeleton.AddBone("head", 1, head);
    return skeleton;
}
} // namespace

TEST(Skeleton_BuildsBindMatrices)
{
    const Skeleton skeleton = MakeSkeleton();
    CHECK(skeleton.BoneCount() == 3);
    CHECK(skeleton.FindBone("spine") == 1);
    CHECK(skeleton.FindBone("missing") == -1);

    const Vec3 headWorld = skeleton.GetBone(2).bindWorld.TransformPoint({0.0f, 0.0f, 0.0f});
    CHECK(NearlyEqual(headWorld.y, 1.5f, 1.0e-4f));

    const Mat4 identity = skeleton.GetBone(2).bindWorld * skeleton.GetBone(2).inverseBindMatrix;
    for (int i = 0; i < 16; ++i) CHECK(NearlyEqual(identity.m[i], Mat4::Identity().m[i], 1.0e-4f));
}

TEST(Clip_SamplesAndLoops)
{
    const Skeleton skeleton = MakeSkeleton();

    Clip clip;
    clip.name = "Turn";
    clip.duration = 1.0f;
    clip.looping = true;
    clip.AddRotation(0, 0.0f, Quat::Identity());
    clip.AddRotation(0, 1.0f, Quat::FromAxisAngle(AxisY(), HalfPi));
    CHECK(NearlyEqual(clip.duration, 1.0f));

    Pose pose = Pose::BindPose(skeleton);
    clip.Sample(0.5f, pose);
    const Vec3 halfway = pose.local[0].rotation.Rotate(Vec3{1.0f, 0.0f, 0.0f});
    CHECK(NearlyEqual(halfway.x, 0.7071f, 1.0e-3f));
    CHECK(NearlyEqual(halfway.z, -0.7071f, 1.0e-3f));

    clip.Sample(0.0f, pose);
    CHECK(NearlyEqual(pose.local[0].rotation.w, 1.0f, 1.0e-4f));
    clip.Sample(1.25f, pose); // wraps back to 0.25s, a quarter of the way to a half turn
    CHECK(NearlyEqual(pose.local[0].rotation.w, std::cos(HalfPi * 0.125f), 1.0e-3f));
}

TEST(Pose_BlendAndSkinningMatrices)
{
    const Skeleton skeleton = MakeSkeleton();
    Pose a = Pose::BindPose(skeleton);
    Pose b = Pose::BindPose(skeleton);
    b.local[1].position = {0.0f, 3.0f, 0.0f};

    const Pose blended = Pose::Blend(a, b, 0.5f);
    CHECK(NearlyEqual(blended.local[1].position.y, 2.0f, 1.0e-4f));

    const Array<Mat4> restPalette = a.SkinningMatrices(skeleton);
    CHECK(restPalette.size() == 3);
    for (const Mat4& matrix : restPalette)
        for (int i = 0; i < 16; ++i)
            CHECK(NearlyEqual(matrix.m[i], Mat4::Identity().m[i], 1.0e-4f));

    const Array<Mat4> movedPalette = b.SkinningMatrices(skeleton);
    const Vec3 movedHead = movedPalette[2].TransformPoint({0.0f, 1.5f, 0.0f});
    CHECK(NearlyEqual(movedHead.y, 3.5f, 1.0e-4f));
}

TEST(AnimationGraph_CrossfadesBetweenStates)
{
    const Skeleton skeleton = MakeSkeleton();

    Clip idle;
    idle.duration = 1.0f;
    idle.AddPosition(1, 0.0f, {0.0f, 1.0f, 0.0f});

    Clip run;
    run.duration = 1.0f;
    run.AddPosition(1, 0.0f, {0.0f, 2.0f, 0.0f});

    AnimationGraph graph;
    graph.AddState("idle", &idle);
    graph.AddState("run", &run);

    Pose pose = Pose::BindPose(skeleton);
    CHECK(graph.Play("idle", 0.0f));
    graph.Update(skeleton, 0.1f, pose);
    CHECK(NearlyEqual(pose.local[1].position.y, 1.0f, 1.0e-4f));

    CHECK(graph.Play("run", 0.2f));
    CHECK(graph.IsFading());
    graph.Update(skeleton, 0.1f, pose);
    CHECK(pose.local[1].position.y > 1.0f);
    CHECK(pose.local[1].position.y < 2.0f);

    for (int i = 0; i < 5; ++i) graph.Update(skeleton, 0.1f, pose);
    CHECK(NearlyEqual(pose.local[1].position.y, 2.0f, 1.0e-3f));
    CHECK(!graph.IsFading());
    CHECK(graph.CurrentState() == "run");
    CHECK(!graph.Play("missing"));
}

TEST(Retargeting_MatchesBonesByName)
{
    const Skeleton source = MakeSkeleton();
    Pose sourcePose = Pose::BindPose(source);
    sourcePose.local[1].position = {1.0f, 0.0f, 0.0f};

    Skeleton target;
    target.AddBone("root", -1, Transform{});
    Transform spineBind;
    spineBind.position = {0.0f, 2.0f, 0.0f};
    target.AddBone("spine", 0, spineBind);
    target.AddBone("tail", 1, Transform{});

    const Pose retargeted = Retarget(source, sourcePose, target);
    CHECK(retargeted.Count() == 3);
    CHECK(NearlyEqual(retargeted.local[1].position.x, 1.0f));
    CHECK(NearlyEqual(retargeted.local[2].position.y, 0.0f)); // unmatched bone kept its bind pose
}

TEST(GPUSkinning_WeightsVertices)
{
    const Skeleton skeleton = MakeSkeleton();
    Pose pose = Pose::BindPose(skeleton);
    pose.local[1].position = {0.0f, 3.0f, 0.0f};
    const Array<Mat4> palette = pose.SkinningMatrices(skeleton);

    SkinnedVertex vertex;
    vertex.position = {0.0f, 1.5f, 0.0f};
    vertex.bones = {2, 0, 0, 0};
    vertex.weights = {1.0f, 0.0f, 0.0f, 0.0f};

    const Vec3 skinned = SkinPosition(vertex, palette);
    CHECK(NearlyEqual(skinned.y, 3.5f, 1.0e-4f));

    vertex.weights = {0.5f, 0.5f, 0.0f, 0.0f};
    vertex.bones = {2, 0, 0, 0};
    const Vec3 blended = SkinPosition(vertex, palette);
    CHECK(blended.y > 1.5f && blended.y < 3.5f);
}

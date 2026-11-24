#include "TestFramework.h"
#include "PRS/Physics/PhysicsWorld.h"
#include "PRS/Scene/Components.h"

using namespace PRS;
using namespace PRS::Physics;

namespace
{
ECS::Entity MakeGround(ECS::World& world)
{
    const ECS::Entity ground = world.Create();
    Transform transform;
    transform.position = {0.0f, -0.5f, 0.0f};
    world.Emplace<Transform>(ground, transform);
    world.Emplace<BoxCollider>(ground, BoxCollider{Vec3{20.0f, 0.5f, 20.0f}});
    return ground;
}

ECS::Entity MakeBall(ECS::World& world, const Vec3& position, f32 radius = 0.5f)
{
    const ECS::Entity ball = world.Create();
    Transform transform;
    transform.position = position;
    world.Emplace<Transform>(ball, transform);
    world.Emplace<SphereCollider>(ball, SphereCollider{radius});
    RigidBody body;
    body.SetMass(1.0f);
    world.Emplace<RigidBody>(ball, body);
    return ball;
}
} // namespace

TEST(Narrowphase_SphereSphere)
{
    Vec3 normal{};
    f32 depth = 0.0f;
    CHECK(IntersectSphereSphere({0, 0, 0}, 1.0f, {1.5f, 0, 0}, 1.0f, normal, depth));
    CHECK(NearlyEqual(depth, 0.5f, 1.0e-4f));
    CHECK(NearlyEqual(normal.x, 1.0f, 1.0e-4f));
    CHECK(!IntersectSphereSphere({0, 0, 0}, 1.0f, {3.0f, 0, 0}, 1.0f, normal, depth));
}

TEST(Narrowphase_SphereBoxAndBoxBox)
{
    Vec3 normal{};
    f32 depth = 0.0f;
    CHECK(IntersectSphereBox({0.0f, 0.6f, 0.0f}, 0.5f, {0, 0, 0}, {1, 1, 1}, normal, depth));
    CHECK(NearlyEqual(depth, 0.9f, 1.0e-4f)); // sphere reaches 0.1 into the box
    CHECK(IntersectBoxBox({0, 0, 0}, {1, 1, 1}, {1.5f, 0, 0}, {1, 1, 1}, normal, depth));
    CHECK(NearlyEqual(depth, 0.5f, 1.0e-4f));
    CHECK(NearlyEqual(normal.x, 1.0f, 1.0e-4f));
    CHECK(!IntersectBoxBox({0, 0, 0}, {1, 1, 1}, {5.0f, 0, 0}, {1, 1, 1}, normal, depth));
}

TEST(Physics_GravityDropRestsOnGround)
{
    ECS::World world;
    MakeGround(world);
    const ECS::Entity ball = MakeBall(world, {0.0f, 5.0f, 0.0f});

    PhysicsWorld physics;
    for (int i = 0; i < 240; ++i) physics.Step(world, 1.0f / 60.0f);

    const f32 restY = world.Get<Transform>(ball).position.y;
    CHECK(restY > 0.35f); // ball radius 0.5 sitting on a floor whose top is y = 0
    CHECK(restY < 0.75f);
    CHECK(NearlyEqual(world.Get<RigidBody>(ball).velocity.y, 0.0f, 0.1f));
}

TEST(Physics_KinematicPushMovesDynamicBody)
{
    ECS::World world;
    const ECS::Entity pusher = world.Create();
    Transform pusherTransform;
    pusherTransform.position = {-2.0f, 0.0f, 0.0f};
    world.Emplace<Transform>(pusher, pusherTransform);
    world.Emplace<BoxCollider>(pusher, BoxCollider{Vec3{0.5f, 0.5f, 0.5f}});
    RigidBody kinematic;
    kinematic.kinematic = true;
    kinematic.velocity = {4.0f, 0.0f, 0.0f};
    world.Emplace<RigidBody>(pusher, kinematic);

    const ECS::Entity box = world.Create();
    Transform boxTransform;
    boxTransform.position = {0.0f, 0.0f, 0.0f};
    world.Emplace<Transform>(box, boxTransform);
    world.Emplace<BoxCollider>(box, BoxCollider{Vec3{0.5f, 0.5f, 0.5f}});
    RigidBody dynamic;
    dynamic.SetMass(1.0f);
    world.Emplace<RigidBody>(box, dynamic);

    PhysicsWorld physics;
    for (int i = 0; i < 90; ++i) physics.Step(world, 1.0f / 60.0f);

    CHECK(world.Get<Transform>(box).position.x > 0.25f);
    CHECK(!physics.Contacts().empty() || world.Get<Transform>(box).position.x > 0.25f);
}

TEST(Physics_RaycastAndOverlap)
{
    ECS::World world;
    const ECS::Entity ground = MakeGround(world);
    const ECS::Entity ball = MakeBall(world, {0.0f, 2.0f, 0.0f});

    PhysicsWorld physics;
    RaycastHit hit;
    CHECK(physics.Raycast(world, {0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 100.0f, hit));
    CHECK(hit.hit);
    CHECK(hit.entity == ball);
    CHECK(NearlyEqual(hit.point.y, 2.5f, 0.05f));

    ECS::Entity found{};
    CHECK(physics.OverlapSphere(world, {0.0f, 2.0f, 0.0f}, 0.4f, &found));
    CHECK(found == ball);
    CHECK(!physics.OverlapSphere(world, {50.0f, 50.0f, 50.0f}, 1.0f, nullptr));

    RaycastHit groundHit;
    CHECK(physics.Raycast(world, {10.0f, 10.0f, 10.0f}, {0.0f, -1.0f, 0.0f}, 100.0f, groundHit));
    CHECK(groundHit.entity == ground);
}

TEST(Physics_FixedStepAccumulator)
{
    ECS::World world;
    PhysicsWorld physics;
    CHECK(physics.Step(world, 1.0f / 120.0f) == 0);
    CHECK(physics.Step(world, 1.0f / 120.0f) == 1);
    CHECK(NearlyEqual(physics.Accumulator(), 0.0f, 1.0e-5f));
    // A full second is clamped to maxAccumulatedTime (0.25s) => 15 fixed steps.
    CHECK(physics.Step(world, 1.0f) == 15);
}

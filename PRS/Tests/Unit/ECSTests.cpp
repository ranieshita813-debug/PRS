#include "TestFramework.h"
#include "PRS/Core/Math/Common.h"
#include "PRS/ECS/Commands.h"
#include "PRS/ECS/SystemScheduler.h"
#include "PRS/ECS/World.h"

using namespace PRS;

namespace
{
struct Position
{
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Velocity
{
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Tag
{
};
} // namespace

TEST(Entity_GenerationInvalidatesStaleHandles)
{
    ECS::World world;
    const ECS::Entity first = world.Create();
    CHECK(world.IsAlive(first));
    CHECK(world.AliveCount() == 1);

    world.Destroy(first);
    CHECK(!world.IsAlive(first));
    CHECK(world.AliveCount() == 0);

    const ECS::Entity second = world.Create();
    CHECK(second.index == first.index); // slot reused
    CHECK(second.generation != first.generation);
    CHECK(!world.Has<Tag>(first)); // stale handle sees nothing
    CHECK(world.IsAlive(second));
}

TEST(Component_EmplaceGetRemove)
{
    ECS::World world;
    const ECS::Entity entity = world.Create();
    world.Emplace<Position>(entity, Position{1.0f, 2.0f});
    CHECK(world.Has<Position>(entity));
    CHECK(!world.Has<Velocity>(entity));
    world.Emplace<Velocity>(entity, Velocity{0.5f, 0.25f});

    world.Get<Position>(entity).x = 7.0f;
    CHECK(NearlyEqual(world.Get<Position>(entity).x, 7.0f));

    CHECK(world.Remove<Velocity>(entity));
    CHECK(!world.Has<Velocity>(entity));
    CHECK(!world.Remove<Velocity>(entity));
    CHECK(world.Has<Position>(entity));
}

TEST(Query_VisitsOnlyMatchingEntities)
{
    ECS::World world;
    for (int i = 0; i < 5; ++i)
    {
        const ECS::Entity entity = world.Create();
        world.Emplace<Position>(entity, Position{static_cast<f32>(i), 0.0f});
        if (i % 2 == 0) world.Emplace<Velocity>(entity, Velocity{1.0f, 0.0f});
    }

    CHECK(world.Count<Position>() == 5);
    CHECK((world.Count<Position, Velocity>() == 3));

    world.Query<Position, Velocity>().ForEach(
        [](ECS::Entity, Position& position, Velocity& velocity) {
            position.x += velocity.x;
            velocity.x = 0.0f;
        });

    int visited = 0;
    world.Query<Position, Velocity>().ForEach([&](ECS::Entity, Position&, Velocity& velocity) {
        CHECK(NearlyEqual(velocity.x, 0.0f));
        ++visited;
    });
    CHECK(visited == 3);
}

TEST(Commands_DefersStructuralChanges)
{
    ECS::World world;
    const ECS::Entity existing = world.Create();
    world.Emplace<Position>(existing, Position{0.0f, 0.0f});

    ECS::Commands commands(world);
    commands.Add<Velocity>(existing, Velocity{2.0f, 0.0f});
    commands.Remove<Position>(existing);
    CHECK(!world.Has<Velocity>(existing)); // still deferred
    CHECK(world.Has<Position>(existing));
    CHECK(commands.Pending() == 2);

    commands.Flush();
    CHECK(world.Has<Velocity>(existing));
    CHECK(!world.Has<Position>(existing));
    CHECK(commands.Pending() == 0);
}

TEST(SystemScheduler_RunsStagesInOrder)
{
    ECS::World world;
    ECS::SystemScheduler scheduler;
    Array<int> order;

    scheduler.Add(ECS::SystemStage::PostUpdate, "post", [&](ECS::SystemContext&) { order.push_back(3); }, 0);
    scheduler.Add(ECS::SystemStage::PreUpdate, "pre", [&](ECS::SystemContext&) { order.push_back(1); }, 0);
    scheduler.Add(ECS::SystemStage::Update, "late", [&](ECS::SystemContext&) { order.push_back(2); }, 10);
    scheduler.Add(ECS::SystemStage::Update, "early", [&](ECS::SystemContext&) { order.push_back(2); }, -5);

    scheduler.RunAll(world, 0.016f);
    CHECK(order.size() == 4);
    CHECK(order[0] == 1);
    CHECK(order[1] == 2); // "early" before "late"
    CHECK(order[2] == 2);
    CHECK(order[3] == 3);
    CHECK(scheduler.Frame() == 1);
    CHECK(NearlyEqual(scheduler.TotalTime(), 0.016f, 1.0e-5f));
}

TEST(Archetype_TracksComponentSignature)
{
    ECS::World world;
    const ECS::Entity entity = world.Create();
    world.Emplace<Position>(entity, Position{});
    world.Emplace<Velocity>(entity, Velocity{});

    const ECS::Archetype signature = world.SignatureOf(entity);
    CHECK(signature.Count() == 2);
    CHECK(signature.Has(ECS::Detail::ComponentTypeId<Position>()));
    CHECK(signature.Has(ECS::Detail::ComponentTypeId<Velocity>()));
    CHECK(!signature.Has(ECS::Detail::ComponentTypeId<Tag>()));

    ECS::Archetype filter;
    filter.Set(ECS::Detail::ComponentTypeId<Position>());
    CHECK(signature.Contains(filter));
}

#include "TestFramework.h"
#include "PRS/Scene/Prefab.h"
#include "PRS/Scene/Scene.h"
#include "PRS/Scene/SceneSerializer.h"

using namespace PRS;
namespace Sc = PRS::Scene;

TEST(Scene_BuildsHierarchy)
{
    Sc::Scene scene("World");
    const ECS::Entity root = scene.CreateEntity("Root");
    const ECS::Entity childA = scene.CreateEntity("A", root);
    const ECS::Entity childB = scene.CreateEntity("B", root);
    const ECS::Entity grandChild = scene.CreateEntity("A1", childA);

    CHECK(scene.EntityCount() == 4);
    CHECK(scene.Roots().size() == 1);
    CHECK(scene.Roots()[0] == root);

    scene.World().Get<Transform>(childA).position = {1.0f, 0.0f, 0.0f};
    scene.World().Get<Transform>(grandChild).position = {0.0f, 2.0f, 0.0f};

    const Vec3 world = scene.WorldMatrix(grandChild).TransformPoint({0.0f, 0.0f, 0.0f});
    CHECK(NearlyEqual(world.x, 1.0f));
    CHECK(NearlyEqual(world.y, 2.0f));

    CHECK(scene.IsAncestor(root, grandChild));
    CHECK(!scene.IsAncestor(grandChild, root));
    CHECK(childB.IsValid());
}

TEST(Scene_RejectsCyclesAndSupportsReparenting)
{
    Sc::Scene scene("World");
    const ECS::Entity a = scene.CreateEntity("A");
    const ECS::Entity b = scene.CreateEntity("B", a);
    const ECS::Entity c = scene.CreateEntity("C", b);

    CHECK(!scene.SetParent(a, c)); // would form a cycle
    CHECK(!scene.SetParent(a, a));

    CHECK(scene.SetParent(c, a)); // b -> a, c moved to a
    const Sc::Children* children = scene.World().TryGet<Sc::Children>(a);
    CHECK(children != nullptr && children->list.size() == 2);

    scene.DetachParent(c);
    CHECK(!scene.World().Has<Sc::Parent>(c));
    CHECK(scene.Roots().size() == 2);
}

TEST(Scene_DestroyIsRecursive)
{
    Sc::Scene scene("World");
    const ECS::Entity root = scene.CreateEntity("Root");
    const ECS::Entity child = scene.CreateEntity("Child", root);
    const ECS::Entity grandChild = scene.CreateEntity("GrandChild", child);
    CHECK(scene.EntityCount() == 3);

    scene.DestroyEntity(root);
    CHECK(scene.EntityCount() == 0);
    CHECK(!scene.IsAlive(child));
    CHECK(!scene.IsAlive(grandChild));
    CHECK(scene.Roots().empty());
}

TEST(SceneSerializer_RoundTripsHierarchyAndComponents)
{
    Sc::Scene scene("OpenWorld");
    const ECS::Entity player = scene.CreateEntity("Player");
    Transform playerTransform;
    playerTransform.position = {3.0f, -1.5f, 8.25f};
    playerTransform.rotation = Quat::FromAxisAngle(AxisY(), Radians(45.0f));
    playerTransform.scale = {1.0f, 2.0f, 1.0f};
    scene.World().Emplace<Transform>(player, playerTransform);
    scene.World().Emplace<Sc::MeshRenderer>(
        player, Sc::MeshRenderer{"meshes/player.glb", "mat/hero", true, true});

    const ECS::Entity camera = scene.CreateEntity("MainCamera", player);
    scene.World().Emplace<Sc::Camera>(camera, Sc::Camera{70.0f, 0.01f, 500.0f, 1.777f, true});
    scene.World().Emplace<Sc::Light>(
        camera, Sc::Light{Sc::LightType::Spot, Vec3{1.0f, 0.9f, 0.8f}, 3.5f, 20.0f, 15.0f, 25.0f,
                          true});

    const std::string text = Sc::SceneSerializer::ToString(scene);
    CHECK(!text.empty());
    CHECK(text.find("PRSSCENE") != std::string::npos);

    Sc::Scene loaded;
    CHECK(Sc::SceneSerializer::FromString(loaded, text));
    CHECK(loaded.GetName() == "OpenWorld");
    CHECK(loaded.EntityCount() == 2);
    CHECK(loaded.Roots().size() == 1);

    const ECS::Entity loadedPlayer = loaded.Roots()[0];
    const Transform* loadedTransform = loaded.World().TryGet<Transform>(loadedPlayer);
    CHECK(loadedTransform != nullptr);
    CHECK(NearlyEqual(loadedTransform->position.x, 3.0f, 1.0e-4f));
    CHECK(NearlyEqual(loadedTransform->position.y, -1.5f, 1.0e-4f));
    CHECK(NearlyEqual(loadedTransform->scale.y, 2.0f, 1.0e-4f));

    const Sc::MeshRenderer* mesh = loaded.World().TryGet<Sc::MeshRenderer>(loadedPlayer);
    CHECK(mesh != nullptr && mesh->mesh == "meshes/player.glb");

    const Sc::Children* children = loaded.World().TryGet<Sc::Children>(loadedPlayer);
    CHECK(children != nullptr && children->list.size() == 1);
    const ECS::Entity loadedCamera = children->list[0];
    const Sc::Camera* cameraComponent = loaded.World().TryGet<Sc::Camera>(loadedCamera);
    CHECK(cameraComponent != nullptr && cameraComponent->primary);
    CHECK(NearlyEqual(cameraComponent->fovYDegrees, 70.0f, 1.0e-4f));
    const Sc::Light* light = loaded.World().TryGet<Sc::Light>(loadedCamera);
    CHECK(light != nullptr && light->type == Sc::LightType::Spot);
}

TEST(SceneSerializer_RejectsMalformedInput)
{
    Sc::Scene scene;
    CHECK(!Sc::SceneSerializer::FromString(scene, ""));
    CHECK(!Sc::SceneSerializer::FromString(scene, "NOTASCENE 1\n"));
    CHECK(!Sc::SceneSerializer::FromString(scene, "PRSSCENE 99\n"));
    CHECK(!Sc::SceneSerializer::FromString(scene, "PRSSCENE 2\nbogus\n"));
}

TEST(Prefab_CapturesAndInstantiatesSubtree)
{
    Sc::Scene scene("World");
    const ECS::Entity root = scene.CreateEntity("Car");
    const ECS::Entity wheel = scene.CreateEntity("Wheel", root);
    scene.World().Get<Transform>(wheel).position = {1.0f, 0.5f, 0.0f};
    scene.World().Emplace<Sc::MeshRenderer>(
        wheel, Sc::MeshRenderer{"meshes/wheel.glb", "mat/rubber", true, true});

    const Sc::Prefab prefab = Sc::Prefab::Capture(scene, root, "CarPrefab");
    CHECK(prefab.NodeCount() == 2);
    CHECK(prefab.nodes[0].parent == -1);
    CHECK(prefab.nodes[1].parent == 0);

    const ECS::Entity first = prefab.Instantiate(scene, {}, Vec3{5.0f, 0.0f, 0.0f});
    const ECS::Entity second = prefab.Instantiate(scene, {}, Vec3{-5.0f, 0.0f, 0.0f});
    // Original car + wheel, plus two instances of two nodes each.
    CHECK(scene.EntityCount() == 6);
    CHECK(first != second);
    CHECK(NearlyEqual(scene.World().Get<Transform>(first).position.x, 5.0f));
    CHECK(NearlyEqual(scene.World().Get<Transform>(second).position.x, -5.0f));

    const Sc::Children* children = scene.World().TryGet<Sc::Children>(first);
    CHECK(children != nullptr && children->list.size() == 1);
    CHECK(scene.World().Has<Sc::MeshRenderer>(children->list[0]));
    CHECK(NearlyEqual(scene.World().Get<Transform>(children->list[0]).position.x, 1.0f));
}

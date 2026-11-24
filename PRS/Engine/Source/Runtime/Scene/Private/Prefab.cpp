#include "PRS/Scene/Prefab.h"
#include <unordered_map>

namespace PRS::Scene
{
namespace
{
void CaptureNode(const Scene& scene, ECS::Entity entity, i32 parentSlot,
                 std::unordered_map<u64, i32>& slotOf, Prefab& prefab)
{
    if (!scene.IsAlive(entity)) return;

    Prefab::Node node;
    node.parent = parentSlot;
    if (const Name* name = scene.World().TryGet<Name>(entity)) node.name = name->value;
    if (const Transform* transform = scene.World().TryGet<Transform>(entity))
        node.transform = *transform;
    if (const Camera* camera = scene.World().TryGet<Camera>(entity))
    {
        node.camera = *camera;
        node.hasCamera = true;
    }
    if (const MeshRenderer* mesh = scene.World().TryGet<MeshRenderer>(entity))
    {
        node.mesh = *mesh;
        node.hasMesh = true;
    }
    if (const Light* light = scene.World().TryGet<Light>(entity))
    {
        node.light = *light;
        node.hasLight = true;
    }

    const i32 slot = static_cast<i32>(prefab.nodes.size());
    const u64 key = (static_cast<u64>(entity.index) << 32) | entity.generation;
    slotOf[key] = slot;
    prefab.nodes.push_back(node);

    if (const Children* children = scene.World().TryGet<Children>(entity))
        for (ECS::Entity child : children->list) CaptureNode(scene, child, slot, slotOf, prefab);
}
} // namespace

Prefab Prefab::Capture(const Scene& scene, ECS::Entity root) { return Capture(scene, root, "Prefab"); }

Prefab Prefab::Capture(const Scene& scene, ECS::Entity root, std::string prefabName)
{
    Prefab prefab;
    prefab.name = std::move(prefabName);
    std::unordered_map<u64, i32> slotOf;
    CaptureNode(scene, root, -1, slotOf, prefab);
    return prefab;
}

ECS::Entity Prefab::Instantiate(Scene& scene, ECS::Entity parent,
                                const Vec3& positionOffset) const
{
    if (nodes.empty()) return ECS::Entity{};

    std::vector<ECS::Entity> created;
    created.reserve(nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const Node& node = nodes[i];
        const ECS::Entity parentEntity =
            node.parent >= 0 ? created[static_cast<size_t>(node.parent)] : parent;

        const ECS::Entity entity = scene.CreateEntity(node.name, parentEntity);
        created.push_back(entity);

        Transform transform = node.transform;
        if (node.parent < 0) transform.position = transform.position + positionOffset;
        scene.World().Emplace<Transform>(entity, transform);

        if (node.hasCamera) scene.World().Emplace<Camera>(entity, node.camera);
        if (node.hasMesh) scene.World().Emplace<MeshRenderer>(entity, node.mesh);
        if (node.hasLight) scene.World().Emplace<Light>(entity, node.light);
    }
    return created.front();
}
} // namespace PRS::Scene

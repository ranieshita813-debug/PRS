#pragma once
#include "PRS/Scene/Scene.h"
#include <string>
#include <vector>

namespace PRS::Scene
{
// A prefab is a snapshot of an entity subtree that can be stamped into any
// scene any number of times. Instances are independent copies (no shared state)
// until a proper prefab-linking system lands.
class Prefab
{
public:
    struct Node
    {
        std::string name = "Entity";
        Transform transform;
        bool hasCamera = false;
        Camera camera;
        bool hasMesh = false;
        MeshRenderer mesh;
        bool hasLight = false;
        Light light;
        i32 parent = -1; // index into Prefab::nodes, -1 for the root
    };

    std::string name = "Prefab";
    std::vector<Node> nodes;

    // Captures the subtree rooted at `root` (root first, children in order).
    static Prefab Capture(const Scene& scene, ECS::Entity root);
    static Prefab Capture(const Scene& scene, ECS::Entity root, std::string prefabName);

    // Creates the nodes under `parent` (or at the scene root). `positionOffset`
    // is added to the root node's position, so instances can be spread apart.
    ECS::Entity Instantiate(Scene& scene, ECS::Entity parent = {},
                            const Vec3& positionOffset = Vec3{}) const;

    size_t NodeCount() const { return nodes.size(); }
    bool Empty() const { return nodes.empty(); }
};
} // namespace PRS::Scene

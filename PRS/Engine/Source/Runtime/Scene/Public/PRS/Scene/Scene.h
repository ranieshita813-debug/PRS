#pragma once
#include "PRS/Core/Containers/Containers.h"
#include "PRS/ECS/World.h"
#include "PRS/Scene/Components.h"

namespace PRS::Scene
{
// A scene owns an ECS world plus the transform hierarchy and the editor-facing
// metadata (names, camera, renderable components).
class Scene
{
public:
    explicit Scene(std::string name = "Scene");

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name) { m_name = std::move(name); }

    ECS::World& World() { return m_world; }
    const ECS::World& World() const { return m_world; }

    ECS::Entity CreateEntity(std::string name = "Entity", ECS::Entity parent = {});
    void DestroyEntity(ECS::Entity entity);
    bool IsAlive(ECS::Entity entity) const { return m_world.IsAlive(entity); }

    // Re-parenting keeps Children in sync. Returns false for cycles or dead handles.
    bool SetParent(ECS::Entity child, ECS::Entity parent);
    void DetachParent(ECS::Entity child);
    bool IsAncestor(ECS::Entity ancestor, ECS::Entity entity) const;

    Mat4 LocalMatrix(ECS::Entity entity) const;
    Mat4 WorldMatrix(ECS::Entity entity) const;

    const Array<ECS::Entity>& Roots() const { return m_roots; }
    size_t EntityCount() const { return m_world.AliveCount(); }

    // Destroys every entity, keeping the scene name.
    void Clear();

private:
    void RemoveChildLink(ECS::Entity parent, ECS::Entity child);

    std::string m_name;
    ECS::World m_world;
    Array<ECS::Entity> m_roots;
};
} // namespace PRS::Scene

#include "PRS/Scene/Scene.h"
#include "PRS/Core/Assert.h"
#include <algorithm>

namespace PRS::Scene
{
Scene::Scene(std::string name) : m_name(std::move(name)) {}

ECS::Entity Scene::CreateEntity(std::string name, ECS::Entity parent)
{
    const ECS::Entity entity = m_world.Create();
    m_world.Emplace<Name>(entity, Name{std::move(name)});
    m_world.Emplace<Transform>(entity, Transform{});

    if (parent.IsValid() && m_world.IsAlive(parent) && parent != entity)
    {
        m_world.Emplace<Parent>(entity, Parent{parent});
        if (Children* children = m_world.TryGet<Children>(parent))
            children->list.push_back(entity);
        else
            m_world.Emplace<Children>(parent, Children{{entity}});
    }
    else
    {
        m_roots.push_back(entity);
    }
    return entity;
}

void Scene::DestroyEntity(ECS::Entity entity)
{
    if (!m_world.IsAlive(entity)) return;

    // Destroy children depth-first from a copy; the live list is mutated below.
    if (Children* children = m_world.TryGet<Children>(entity))
    {
        const Array<ECS::Entity> copy = children->list;
        for (ECS::Entity child : copy) DestroyEntity(child);
    }

    if (Parent* parent = m_world.TryGet<Parent>(entity))
        RemoveChildLink(parent->entity, entity);

    m_roots.erase(std::remove(m_roots.begin(), m_roots.end(), entity), m_roots.end());
    m_world.Destroy(entity);
}

void Scene::RemoveChildLink(ECS::Entity parent, ECS::Entity child)
{
    if (!parent.IsValid() || !m_world.IsAlive(parent)) return;
    if (Children* children = m_world.TryGet<Children>(parent))
        children->list.erase(std::remove(children->list.begin(), children->list.end(), child),
                             children->list.end());
}

bool Scene::IsAncestor(ECS::Entity ancestor, ECS::Entity entity) const
{
    if (!ancestor.IsValid() || !entity.IsValid()) return false;
    ECS::Entity cursor = entity;
    for (u32 guard = 0; guard < 4096; ++guard)
    {
        const Parent* parent = m_world.TryGet<Parent>(cursor);
        if (parent == nullptr) return false;
        if (parent->entity == ancestor) return true;
        cursor = parent->entity;
    }
    return false;
}

bool Scene::SetParent(ECS::Entity child, ECS::Entity parent)
{
    if (!m_world.IsAlive(child)) return false;
    if (parent.IsValid() && (!m_world.IsAlive(parent))) return false;
    if (parent == child) return false;
    if (parent.IsValid() && IsAncestor(child, parent)) return false; // would form a cycle

    if (Parent* oldParent = m_world.TryGet<Parent>(child))
        RemoveChildLink(oldParent->entity, child);
    else
        m_roots.erase(std::remove(m_roots.begin(), m_roots.end(), child), m_roots.end());

    if (parent.IsValid())
    {
        m_world.Emplace<Parent>(child, Parent{parent});
        if (Children* children = m_world.TryGet<Children>(parent))
            children->list.push_back(child);
        else
            m_world.Emplace<Children>(parent, Children{{child}});
    }
    else
    {
        m_world.Remove<Parent>(child);
        m_roots.push_back(child);
    }
    return true;
}

void Scene::DetachParent(ECS::Entity child) { SetParent(child, ECS::Entity{}); }

Mat4 Scene::LocalMatrix(ECS::Entity entity) const
{
    const Transform* transform = m_world.TryGet<Transform>(entity);
    return transform != nullptr ? transform->Matrix() : Mat4::Identity();
}

Mat4 Scene::WorldMatrix(ECS::Entity entity) const
{
    Mat4 result = Mat4::Identity();
    ECS::Entity cursor = entity;
    // Walk up to the root, then apply the chain root-first.
    Array<Mat4> chain;
    for (u32 guard = 0; guard < 4096; ++guard)
    {
        if (!m_world.IsAlive(cursor)) break;
        chain.push_back(LocalMatrix(cursor));
        const Parent* parent = m_world.TryGet<Parent>(cursor);
        if (parent == nullptr) break;
        cursor = parent->entity;
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) result = result * (*it);
    return result;
}

void Scene::Clear()
{
    m_world.Clear();
    m_roots.clear();
}
} // namespace PRS::Scene

#include "PRS/Physics/PhysicsWorld.h"
#include "PRS/Core/Assert.h"
#include "PRS/Scene/Components.h"
#include <algorithm>
#include <cmath>

namespace PRS::Physics
{
namespace
{
void IntegrateRotation(Scene::Transform& transform, const RigidBody& body, f32 h)
{
    const f32 speed = Length(body.angularVelocity);
    if (speed <= Epsilon) return;
    const Quat delta = Quat::FromAxisAngle(body.angularVelocity / speed, speed * h);
    transform.rotation = (delta * transform.rotation).Normalized();
}

struct ColliderInfo
{
    ECS::Entity entity{};
    bool isBox = false;
    f32 radius = 0.0f;
    Vec3 halfExtents{};
};
} // namespace

bool IntersectSphereSphere(const Vec3& ca, f32 ra, const Vec3& cb, f32 rb, Vec3& normal,
                           f32& depth)
{
    const Vec3 delta = cb - ca;
    const f32 radiusSum = ra + rb;
    const f32 distanceSquared = LengthSquared(delta);
    if (distanceSquared >= radiusSum * radiusSum) return false;

    const f32 distance = Sqrt(distanceSquared);
    normal = distance > Epsilon ? delta / distance : Vec3{0.0f, 1.0f, 0.0f};
    depth = radiusSum - distance;
    return true;
}

bool IntersectSphereBox(const Vec3& center, f32 radius, const Vec3& boxCenter,
                        const Vec3& halfExtents, Vec3& normal, f32& depth)
{
    const Vec3 relative = center - boxCenter;
    const Vec3 closest{Clamp(relative.x, -halfExtents.x, halfExtents.x),
                       Clamp(relative.y, -halfExtents.y, halfExtents.y),
                       Clamp(relative.z, -halfExtents.z, halfExtents.z)};
    const Vec3 delta = relative - closest;
    const f32 distanceSquared = LengthSquared(delta);

    if (distanceSquared > radius * radius) return false;

    if (distanceSquared > 1.0e-12f)
    {
        const f32 distance = Sqrt(distanceSquared);
        normal = delta / distance; // box -> sphere center
        depth = radius - distance;
        return true;
    }

    // Center is inside the box: push out along the shallowest axis.
    const Vec3 gaps{halfExtents.x - Abs(relative.x), halfExtents.y - Abs(relative.y),
                    halfExtents.z - Abs(relative.z)};
    if (gaps.x <= gaps.y && gaps.x <= gaps.z)
    {
        normal = Vec3{relative.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f};
        depth = radius + gaps.x;
    }
    else if (gaps.y <= gaps.z)
    {
        normal = Vec3{0.0f, relative.y < 0.0f ? -1.0f : 1.0f, 0.0f};
        depth = radius + gaps.y;
    }
    else
    {
        normal = Vec3{0.0f, 0.0f, relative.z < 0.0f ? -1.0f : 1.0f};
        depth = radius + gaps.z;
    }
    return true;
}

bool IntersectBoxBox(const Vec3& ca, const Vec3& ha, const Vec3& cb, const Vec3& hb, Vec3& normal,
                     f32& depth)
{
    const Vec3 delta = cb - ca;
    const f32 overlapX = ha.x + hb.x - Abs(delta.x);
    if (overlapX <= 0.0f) return false;
    const f32 overlapY = ha.y + hb.y - Abs(delta.y);
    if (overlapY <= 0.0f) return false;
    const f32 overlapZ = ha.z + hb.z - Abs(delta.z);
    if (overlapZ <= 0.0f) return false;

    if (overlapX <= overlapY && overlapX <= overlapZ)
    {
        normal = Vec3{delta.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f};
        depth = overlapX;
    }
    else if (overlapY <= overlapZ)
    {
        normal = Vec3{0.0f, delta.y < 0.0f ? -1.0f : 1.0f, 0.0f};
        depth = overlapY;
    }
    else
    {
        normal = Vec3{0.0f, 0.0f, delta.z < 0.0f ? -1.0f : 1.0f};
        depth = overlapZ;
    }
    return true;
}

bool RaySphere(const Vec3& origin, const Vec3& dir, const Vec3& center, f32 radius, f32& t,
               Vec3& normal)
{
    const Vec3 oc = origin - center;
    const f32 b = Dot(oc, dir);
    const f32 c = LengthSquared(oc) - radius * radius;
    const f32 discriminant = b * b - c;
    if (discriminant < 0.0f) return false;

    const f32 root = Sqrt(discriminant);
    f32 nearest = -b - root;
    if (nearest < 0.0f) nearest = -b + root;
    if (nearest < 0.0f) return false;

    t = nearest;
    normal = Normalize(origin + dir * t - center);
    return true;
}

bool RayBox(const Vec3& origin, const Vec3& dir, const Vec3& center, const Vec3& halfExtents,
            f32& t, Vec3& normal)
{
    const f32 o[3] = {origin.x, origin.y, origin.z};
    const f32 d[3] = {dir.x, dir.y, dir.z};
    const f32 c[3] = {center.x, center.y, center.z};
    const f32 h[3] = {halfExtents.x, halfExtents.y, halfExtents.z};

    f32 tMin = 0.0f;
    f32 tMax = 1.0e30f;
    Vec3 hitNormal{};

    for (int axis = 0; axis < 3; ++axis)
    {
        if (Abs(d[axis]) < 1.0e-8f)
        {
            if (Abs(o[axis] - c[axis]) > h[axis]) return false;
            continue;
        }
        const f32 inv = 1.0f / d[axis];
        f32 t1 = (c[axis] - h[axis] - o[axis]) * inv;
        f32 t2 = (c[axis] + h[axis] - o[axis]) * inv;
        Vec3 nearNormal{};
        if (axis == 0) nearNormal = Vec3{-1.0f, 0.0f, 0.0f};
        if (axis == 1) nearNormal = Vec3{0.0f, -1.0f, 0.0f};
        if (axis == 2) nearNormal = Vec3{0.0f, 0.0f, -1.0f};
        if (t1 > t2)
        {
            const f32 swap = t1;
            t1 = t2;
            t2 = swap;
            nearNormal = -nearNormal;
        }
        if (t1 > tMin)
        {
            tMin = t1;
            hitNormal = nearNormal;
        }
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return false;
    }

    t = tMin;
    normal = hitNormal;
    return true;
}

u32 PhysicsWorld::Step(ECS::World& world, f32 deltaTime)
{
    if (deltaTime <= 0.0f || m_settings.fixedStep <= 0.0f) return 0;

    m_accumulator += Min(deltaTime, m_settings.maxAccumulatedTime);
    u32 steps = 0;
    while (m_accumulator >= m_settings.fixedStep)
    {
        StepFixed(world, m_settings.fixedStep);
        m_accumulator -= m_settings.fixedStep;
        ++steps;
    }
    return steps;
}

void PhysicsWorld::StepFixed(ECS::World& world, f32 h)
{
    if (h <= 0.0f) return;

    // 1. Integrate forces and motion.
    world.Query<Scene::Transform, RigidBody>().ForEach(
        [&](ECS::Entity, Scene::Transform& transform, RigidBody& body) {
            if (body.sleeping) return;

            if (body.kinematic)
            {
                transform.position += body.velocity * h;
                IntegrateRotation(transform, body, h);
                return;
            }

            Vec3 acceleration = body.force * body.inverseMass;
            if (body.useGravity) acceleration += m_settings.gravity;

            body.velocity += acceleration * h;
            body.velocity *= 1.0f / (1.0f + body.linearDamping * h);
            transform.position += body.velocity * h;
            IntegrateRotation(transform, body, h);
            body.force = Vec3{};
        });

    // 2. Collect colliders (sphere or box; a bounded number of shapes is fine
    //    until the broadphase BVH lands with the world-streaming phase).
    Array<ColliderInfo> colliders;
    world.Query<Scene::Transform>().ForEach([&](ECS::Entity entity, Scene::Transform&) {
        if (const BoxCollider* box = world.TryGet<BoxCollider>(entity))
        {
            ColliderInfo info;
            info.entity = entity;
            info.isBox = true;
            info.halfExtents = box->halfExtents;
            colliders.push_back(info);
        }
        else if (const SphereCollider* sphere = world.TryGet<SphereCollider>(entity))
        {
            ColliderInfo info;
            info.entity = entity;
            info.isBox = false;
            info.radius = sphere->radius;
            colliders.push_back(info);
        }
    });

    // 3. Narrowphase.
    m_contacts.clear();
    for (size_t i = 0; i < colliders.size(); ++i)
    {
        for (size_t j = i + 1; j < colliders.size(); ++j)
        {
            const ColliderInfo& a = colliders[i];
            const ColliderInfo& b = colliders[j];

            const Scene::Transform* ta = world.TryGet<Scene::Transform>(a.entity);
            const Scene::Transform* tb = world.TryGet<Scene::Transform>(b.entity);
            if (ta == nullptr || tb == nullptr) continue;

            RigidBody* ra = world.TryGet<RigidBody>(a.entity);
            RigidBody* rb = world.TryGet<RigidBody>(b.entity);

            const bool aDynamic = ra != nullptr && !ra->kinematic && ra->inverseMass > 0.0f;
            const bool bDynamic = rb != nullptr && !rb->kinematic && rb->inverseMass > 0.0f;
            if (!aDynamic && !bDynamic) continue;
            if (ra != nullptr && ra->sleeping && rb != nullptr && rb->sleeping) continue;

            Vec3 normal{};
            f32 depth = 0.0f;
            bool hit = false;

            if (!a.isBox && !b.isBox)
            {
                hit = IntersectSphereSphere(ta->position, a.radius, tb->position, b.radius, normal,
                                            depth);
            }
            else if (!a.isBox && b.isBox)
            {
                // Normal comes back pointing from the box (b) to the sphere (a).
                hit = IntersectSphereBox(ta->position, a.radius, tb->position, b.halfExtents,
                                         normal, depth);
                normal = -normal;
            }
            else if (a.isBox && !b.isBox)
            {
                // Normal points from the box (a) to the sphere (b): already a -> b.
                hit = IntersectSphereBox(tb->position, b.radius, ta->position, a.halfExtents,
                                         normal, depth);
            }
            else
            {
                hit = IntersectBoxBox(ta->position, a.halfExtents, tb->position, b.halfExtents,
                                      normal, depth);
            }

            if (!hit) continue;

            Contact contact;
            contact.a = a.entity;
            contact.b = b.entity;
            contact.normal = normal;
            contact.depth = depth;
            contact.point = (ta->position + tb->position) * 0.5f;
            m_contacts.push_back(contact);

            // A moving body wakes whatever it touches.
            if (ra != nullptr && rb != nullptr)
            {
                if (LengthSquared(ra->velocity) > 0.25f)
                {
                    rb->sleeping = false;
                    rb->sleepTimer = 0.0f;
                }
                if (LengthSquared(rb->velocity) > 0.25f)
                {
                    ra->sleeping = false;
                    ra->sleepTimer = 0.0f;
                }
            }
        }
    }

    // 4. Sequential impulse solve, then positional correction.
    for (u32 iteration = 0; iteration < Max(1u, m_settings.solverIterations); ++iteration)
    {
        for (const Contact& contact : m_contacts)
        {
            Scene::Transform* ta = world.TryGet<Scene::Transform>(contact.a);
            Scene::Transform* tb = world.TryGet<Scene::Transform>(contact.b);
            if (ta == nullptr || tb == nullptr) continue;

            RigidBody* ra = world.TryGet<RigidBody>(contact.a);
            RigidBody* rb = world.TryGet<RigidBody>(contact.b);

            const f32 invA =
                (ra != nullptr && !ra->kinematic && ra->inverseMass > 0.0f) ? ra->inverseMass : 0.0f;
            const f32 invB =
                (rb != nullptr && !rb->kinematic && rb->inverseMass > 0.0f) ? rb->inverseMass : 0.0f;
            const f32 invSum = invA + invB;
            if (invSum <= 0.0f) continue;

            const Vec3 velocityA = (ra != nullptr && !ra->kinematic) ? ra->velocity : Vec3{};
            const Vec3 velocityB = (rb != nullptr && !rb->kinematic) ? rb->velocity : Vec3{};
            const Vec3 relativeVelocity = velocityB - velocityA;
            const f32 normalVelocity = Dot(relativeVelocity, contact.normal);
            if (normalVelocity > 0.0f) continue; // separating already

            const f32 restitutionA = ra != nullptr ? ra->restitution : 0.0f;
            const f32 restitutionB = rb != nullptr ? rb->restitution : 0.0f;
            const f32 restitution = Min(restitutionA, restitutionB);

            const f32 impulseMagnitude =
                -(1.0f + restitution) * normalVelocity / invSum;
            const Vec3 impulse = contact.normal * impulseMagnitude;

            if (ra != nullptr && invA > 0.0f) ra->velocity -= impulse * invA;
            if (rb != nullptr && invB > 0.0f) rb->velocity += impulse * invB;

            // Coulomb friction along the contact tangent.
            const Vec3 tangentVelocity = relativeVelocity - contact.normal * normalVelocity;
            const f32 tangentSpeed = Length(tangentVelocity);
            if (tangentSpeed > Epsilon)
            {
                const Vec3 tangent = tangentVelocity / tangentSpeed;
                const f32 frictionA = ra != nullptr ? ra->friction : 0.6f;
                const f32 frictionB = rb != nullptr ? rb->friction : 0.6f;
                const f32 friction = std::sqrt(frictionA * frictionB);
                f32 tangentialImpulse = -Dot(relativeVelocity, tangent) / invSum;
                const f32 maxFriction = Abs(impulseMagnitude) * friction;
                tangentialImpulse = Clamp(tangentialImpulse, -maxFriction, maxFriction);
                const Vec3 frictionImpulse = tangent * tangentialImpulse;
                if (ra != nullptr && invA > 0.0f) ra->velocity -= frictionImpulse * invA;
                if (rb != nullptr && invB > 0.0f) rb->velocity += frictionImpulse * invB;
            }

            // Split the penetration correction by inverse mass.
            const f32 correction = Max(contact.depth - m_settings.penetrationSlop, 0.0f) *
                                   m_settings.positionCorrection;
            if (invA > 0.0f) ta->position -= contact.normal * (correction * invA / invSum);
            if (invB > 0.0f) tb->position += contact.normal * (correction * invB / invSum);
        }
    }

    // 5. Sleep bodies that have come to rest.
    if (m_settings.enableSleeping)
    {
        const f32 thresholdSquared = m_settings.sleepSpeedThreshold * m_settings.sleepSpeedThreshold;
        world.Query<Scene::Transform, RigidBody>().ForEach(
            [&](ECS::Entity, Scene::Transform&, RigidBody& body) {
                if (body.kinematic || body.inverseMass <= 0.0f) return;
                if (LengthSquared(body.velocity) < thresholdSquared &&
                    LengthSquared(body.angularVelocity) < thresholdSquared)
                {
                    body.sleepTimer += h;
                    if (body.sleepTimer >= m_settings.sleepTime)
                    {
                        body.sleeping = true;
                        body.velocity = Vec3{};
                        body.angularVelocity = Vec3{};
                    }
                }
                else
                {
                    body.sleepTimer = 0.0f;
                    body.sleeping = false;
                }
            });
    }
}

bool PhysicsWorld::Raycast(const ECS::World& world, const Vec3& origin, const Vec3& direction,
                           f32 maxDistance, RaycastHit& hit) const
{
    hit = RaycastHit{};
    const Vec3 dir = Normalize(direction);
    if (LengthSquared(dir) <= 0.0f) return false;

    f32 bestDistance = maxDistance;

    world.Query<Scene::Transform>().ForEach(
        [&](ECS::Entity entity, const Scene::Transform& transform) {
        if (const BoxCollider* box = world.TryGet<BoxCollider>(entity))
        {
            f32 t = 0.0f;
            Vec3 normal{};
            if (RayBox(origin, dir, transform.position, box->halfExtents, t, normal) &&
                t <= bestDistance)
            {
                bestDistance = t;
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = normal;
                hit.entity = entity;
            }
        }
        else if (const SphereCollider* sphere = world.TryGet<SphereCollider>(entity))
        {
            f32 t = 0.0f;
            Vec3 normal{};
            if (RaySphere(origin, dir, transform.position, sphere->radius, t, normal) &&
                t <= bestDistance)
            {
                bestDistance = t;
                hit.hit = true;
                hit.distance = t;
                hit.point = origin + dir * t;
                hit.normal = normal;
                hit.entity = entity;
            }
        }
    });
    return hit.hit;
}

bool PhysicsWorld::OverlapSphere(const ECS::World& world, const Vec3& center, f32 radius,
                                 ECS::Entity* entity) const
{
    bool found = false;
    world.Query<Scene::Transform>().ForEach(
        [&](ECS::Entity candidate, const Scene::Transform& transform) {
        if (found) return;
        if (const BoxCollider* box = world.TryGet<BoxCollider>(candidate))
        {
            Vec3 normal{};
            f32 depth = 0.0f;
            if (IntersectSphereBox(center, radius, transform.position, box->halfExtents, normal,
                                   depth))
            {
                found = true;
                if (entity != nullptr) *entity = candidate;
            }
        }
        else if (const SphereCollider* sphere = world.TryGet<SphereCollider>(candidate))
        {
            Vec3 normal{};
            f32 depth = 0.0f;
            if (IntersectSphereSphere(center, radius, transform.position, sphere->radius, normal,
                                      depth))
            {
                found = true;
                if (entity != nullptr) *entity = candidate;
            }
        }
    });
    return found;
}
} // namespace PRS::Physics

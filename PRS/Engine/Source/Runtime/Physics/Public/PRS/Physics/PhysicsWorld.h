#pragma once
#include "PRS/Core/Containers/Containers.h"
#include "PRS/Core/Math/Vector.h"
#include "PRS/ECS/World.h"

namespace PRS::Physics
{
// A dynamic body. Entities without a RigidBody but with a collider are static
// (infinite mass) geometry such as level collision.
struct RigidBody
{
    f32 mass = 1.0f;
    f32 inverseMass = 1.0f;
    Vec3 velocity{};
    Vec3 angularVelocity{}; // radians per second
    Vec3 force{};
    f32 restitution = 0.2f;
    f32 friction = 0.5f;
    f32 linearDamping = 0.05f;
    bool useGravity = true;
    bool kinematic = false; // moved by game code, pushes others but is not pushed
    bool sleeping = false;
    f32 sleepTimer = 0.0f;

    void SetMass(f32 newMass)
    {
        mass = newMass;
        inverseMass = newMass > 0.0f ? 1.0f / newMass : 0.0f;
    }

    void ApplyForce(const Vec3& f) { force += f; }
    void ApplyImpulse(const Vec3& impulse) { velocity += impulse * inverseMass; }
};

struct SphereCollider
{
    f32 radius = 0.5f;
};

// Axis-aligned box in the entity's local space. Rotation is not applied to the
// box yet: the narrowphase treats it as world-axis-aligned and the renderer uses
// the transform. OBB support lands with the Jolt integration.
struct BoxCollider
{
    Vec3 halfExtents{0.5f, 0.5f, 0.5f};
};

struct Contact
{
    ECS::Entity a{};
    ECS::Entity b{};
    Vec3 normal{0.0f, 1.0f, 0.0f}; // points from a to b
    f32 depth = 0.0f;
    Vec3 point{};
};

struct RaycastHit
{
    bool hit = false;
    f32 distance = 0.0f;
    Vec3 point{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    ECS::Entity entity{};
};

struct PhysicsSettings
{
    Vec3 gravity{0.0f, -9.81f, 0.0f};
    f32 fixedStep = 1.0f / 60.0f;
    u32 solverIterations = 8;
    f32 maxAccumulatedTime = 0.25f; // clamp so a long stall cannot spiral
    f32 positionCorrection = 0.8f;
    f32 penetrationSlop = 0.01f;
    bool enableSleeping = true;
    f32 sleepSpeedThreshold = 0.02f;
    f32 sleepTime = 0.5f;
};

class PhysicsWorld
{
public:
    PhysicsWorld() = default;
    explicit PhysicsWorld(const PhysicsSettings& settings) : m_settings(settings) {}

    PhysicsSettings& Settings() { return m_settings; }
    const PhysicsSettings& Settings() const { return m_settings; }

    // Accumulates real time into fixed steps. Returns how many steps ran.
    u32 Step(ECS::World& world, f32 deltaTime);
    void StepFixed(ECS::World& world, f32 fixedDeltaTime);

    // Sphere cast style query: closest ray hit against colliders.
    bool Raycast(const ECS::World& world, const Vec3& origin, const Vec3& direction,
                 f32 maxDistance, RaycastHit& hit) const;
    bool OverlapSphere(const ECS::World& world, const Vec3& center, f32 radius,
                       ECS::Entity* entity) const;

    const Array<Contact>& Contacts() const { return m_contacts; }
    f32 Accumulator() const { return m_accumulator; }
    void Reset()
    {
        m_accumulator = 0.0f;
        m_contacts.clear();
    }

private:
    PhysicsSettings m_settings{};
    Array<Contact> m_contacts;
    f32 m_accumulator = 0.0f;
};

// Narrowphase helpers, exposed for tests and for gameplay code (triggers,
// character controllers) that needs a one-off test.
bool IntersectSphereSphere(const Vec3& ca, f32 ra, const Vec3& cb, f32 rb, Vec3& normal,
                           f32& depth);
bool IntersectSphereBox(const Vec3& center, f32 radius, const Vec3& boxCenter,
                        const Vec3& halfExtents, Vec3& normal, f32& depth);
bool IntersectBoxBox(const Vec3& ca, const Vec3& ha, const Vec3& cb, const Vec3& hb, Vec3& normal,
                     f32& depth);
bool RaySphere(const Vec3& origin, const Vec3& dir, const Vec3& center, f32 radius, f32& t,
               Vec3& normal);
bool RayBox(const Vec3& origin, const Vec3& dir, const Vec3& center, const Vec3& halfExtents,
            f32& t, Vec3& normal);
} // namespace PRS::Physics

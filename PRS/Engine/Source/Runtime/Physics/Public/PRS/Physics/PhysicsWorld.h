#pragma once
#include "PRS/Core/Math/Math.h"
namespace PRS { namespace Physics {
struct Body { Math::Vec3 pos{0,0,0}; Math::Vec3 vel{0,0,0}; float mass=1.f; };
class PhysicsWorld { public: void Step(float dt); void AddBody(Body*b); };
}}
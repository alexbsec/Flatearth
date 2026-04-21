#ifndef _FLATEARTH_ENGINE_PHYSICS_RIGID_BODY_HPP
#define _FLATEARTH_ENGINE_PHYSICS_RIGID_BODY_HPP

#include "Defines.hpp"
#include "Physics/PhysicsHandle.hpp"

namespace flatearth::physics {

enum class BodyType : uint8 {
  Static,
  Dynamic,
  Kinematic,
};

struct RigidBody {
  BodyType type{BodyType::Dynamic};
  float32  density{1.0f};
  float32  friction{0.3f};
  float32  restitution{0.0f};
  float32  linearDamping{0.0f};
  float32  angularDamping{0.0f};
  bool     fixedRotation{FeFalse};

  // Runtime only — managed by PhysicsSystem
  FeBodyHandle bodyId{};
};

} // namespace flatearth::physics

#endif // _FLATEARTH_ENGINE_PHYSICS_RIGID_BODY_HPP

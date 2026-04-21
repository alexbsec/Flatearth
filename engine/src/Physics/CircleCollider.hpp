#ifndef _FLATEARTH_ENGINE_PHYSICS_CIRCLE_COLLIDER_HPP
#define _FLATEARTH_ENGINE_PHYSICS_CIRCLE_COLLIDER_HPP

#include "Defines.hpp"
#include "Math/Vector2D.hpp"
#include "Physics/PhysicsHandle.hpp"

namespace flatearth::physics {

struct CircleCollider {
  float32 radius{0.5f};
  math::Vec2D offset{0.0f, 0.0f};
  bool isSensor{FeFalse};

  FeShapeHandle shapeId{};
};

}

#endif // _FLATEARTH_ENGINE_PHYSICS_CIRCLE_COLLIDER_HPP

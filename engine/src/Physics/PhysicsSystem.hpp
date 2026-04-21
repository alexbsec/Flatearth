#ifndef _FLATEARTH_ENGINE_PHYSICS_PHYSICS_SYSTEM_HPP
#define _FLATEARTH_ENGINE_PHYSICS_PHYSICS_SYSTEM_HPP

#include "ECS/SystemInterface.hpp"
#include "Physics/FlatearthWorld.hpp"

namespace flatearth::physics {

class PhysicsSystem : public ecs::ISystem {
public:
  explicit PhysicsSystem(FlatearthWorld &world);
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  void CreateBodies(ecs::Registry &registry);
  void SyncTransforms(ecs::Registry &registry);

private:
  FlatearthWorld &_world;
};

}

#endif // _FLATEARTH_ENGINE_PHYSICS_PHYSICS_SYSTEM_HPP

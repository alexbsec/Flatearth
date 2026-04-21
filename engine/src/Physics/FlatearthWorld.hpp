#ifndef _FLATEARTH_ENGINE_PHYSICS_FLATEARTH_WORLD_HPP
#define _FLATEARTH_ENGINE_PHYSICS_FLATEARTH_WORLD_HPP

#include "Defines.hpp"
#include "PhysicsTypes.hpp"
#include "Physics/PhysicsHandle.hpp"
#include "Core/FeMemory.hpp"

namespace flatearth::physics {

class FlatearthWorld {
public:
  explicit FlatearthWorld(memory::MemoryManager &memManager);

  void Initialize(float32 gravityX = cFlatearthGravityForce.x(),
                  float32 gravityY = cFlatearthGravityForce.y());
  void Step(float32 deltaTime, int32 subSteps = 4);
  void Shutdown();

  FeWorldHandle WorldId() const;

private:
  FeWorldHandle _worldHandle{};
  bool _initialized{FeFalse};
};

} // namespace flatearth::physics

#endif // _FLATEARTH_ENGINE_PHYSICS_FLATEARTH_WORLD_HPP

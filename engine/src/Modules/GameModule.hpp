#ifndef _FLATEARTH_ENGINE_MODULES_GAME_MODULE_HPP
#define _FLATEARTH_ENGINE_MODULES_GAME_MODULE_HPP

#include "ECS/Registry.hpp"
#include "Physics/FlatearthWorld.hpp"

namespace flatearth::modules {

class Project {
public:
  explicit Project(memory::MemoryManager &mm, ecs::Registry &reg)
      : _registry(reg), _world(mm) {}

  FeExpect<bool, Error> Initialize(float32 gravityX = 0.0f, float32 gravityY = -9.8f);
  void Shutdown();

  FEAPI ecs::Registry &Registry() { return _registry; }
  FEAPI const ecs::Registry &Registry() const { return _registry; }

  FEAPI physics::FlatearthWorld &World() { return _world; }
  FEAPI const physics::FlatearthWorld &World() const { return _world; }

private:
  ecs::Registry &_registry;

  physics::FlatearthWorld _world;
  bool _initialized{FeFalse};
};

} // namespace flatearth::modules

#endif // _FLATEARTH_ENGINE_MODULES_GAME_MODULE_HPP

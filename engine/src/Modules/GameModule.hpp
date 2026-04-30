#ifndef _FLATEARTH_ENGINE_MODULES_GAME_MODULE_HPP
#define _FLATEARTH_ENGINE_MODULES_GAME_MODULE_HPP

#include "Containers/HashMap.hpp"
#include "ECS/Registry.hpp"
#include "Physics/FlatearthWorld.hpp"
#include "Scene/Scene.hpp"

namespace flatearth::modules {

class Project {
public:
  explicit Project(memory::MemoryManager &mm, ecs::Registry &reg)
      : _memoryManager(mm), _registry(reg), _world(mm), _sceneNameMap(mm) {}

  FeExpect<void, Error> Initialize(float32 gravityX = 0.0f, float32 gravityY = -9.8f);
  void Shutdown();

  FEAPI scene::SceneId RegisterScene(stringv name);
  FEAPI void DestroyScene(scene::SceneId id);
  FEAPI scene::SceneId GetSceneId(stringv name);

  FEAPI ecs::Registry &Registry() { return _registry; }
  FEAPI const ecs::Registry &Registry() const { return _registry; }

  FEAPI physics::FlatearthWorld &World() { return _world; }
  FEAPI const physics::FlatearthWorld &World() const { return _world; }

private:
  memory::MemoryManager &_memoryManager;
  ecs::Registry &_registry;

  physics::FlatearthWorld _world;
  containers::HashMap<string, scene::SceneId> _sceneNameMap;
  scene::SceneId _nextSceneId{1};
  bool _initialized{FeFalse};
};

} // namespace flatearth::modules

#endif // _FLATEARTH_ENGINE_MODULES_GAME_MODULE_HPP

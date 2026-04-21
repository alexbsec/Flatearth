#ifndef _FLATEARTH_ENGINE_SCENE_SCENE_MANAGER_HPP
#define _FLATEARTH_ENGINE_SCENE_SCENE_MANAGER_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/Registry.hpp"
#include "Scene/Scene.hpp"

namespace flatearth::scene {

class SceneManager {
public:
  FEAPI explicit SceneManager(memory::MemoryManager &memManager, ecs::Registry &registry);

  FEAPI FeExpect<Scene *, Error> Load(const string &name);
  FEAPI void Unload(const string &name);
  FEAPI Scene *Get(const string &name);

private:
  memory::MemoryManager &_memoryManager;
  ecs::Registry &_registry;
  containers::HashMap<string, Scene> _scenesMap;
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_SCENE_MANAGER_HPP

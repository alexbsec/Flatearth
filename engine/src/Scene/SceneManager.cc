#include "Scene/SceneManager.hpp"

#include "Core/Logger.hpp"

namespace flatearth::scene {

SceneManager::SceneManager(memory::MemoryManager &memManager, ecs::Registry &registry)
    : _memoryManager(memManager), _registry(registry), _scenesMap(memManager) {
}

FeExpect<Scene *, Error> SceneManager::Load(const string &name) {
  if (Scene *existing = Get(name)) {
    FLOG_WARN("scene '{}' already exists", name);
    return existing;
  }

  Scene scene;
  scene.name = name;
  auto res = _scenesMap.Insert(name, std::move(scene));
  if (!res.has_value()) {
    return FeErr{res.error()};
  }

  return Get(name);
}

void SceneManager::Unload(const string &name) {
  Scene *pScene = Get(name);
  if (!pScene)
    return;

  for (ecs::EntityId id : pScene->allEntities) {
    _registry.Destroy(id);
  }

  _scenesMap.Erase(name);
}

Scene *SceneManager::Get(const string &name) {
  return _scenesMap.Retrieve(name);
}

} // namespace flatearth::scene

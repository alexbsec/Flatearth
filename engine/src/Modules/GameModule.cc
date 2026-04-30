#include "GameModule.hpp"

#include <vector>

namespace flatearth::modules {

FeExpect<void, Error> Project::Initialize(float32 gravityX, float32 gravityY) {
  if (_initialized) {
    return {};
  }

  _world.Initialize(gravityX, gravityY);
  _initialized = FeTrue;
  return {};
}

void Project::Shutdown() {
  if (!_initialized) {
    return;
  }

  _world.Shutdown();
  _initialized = FeFalse;
}

scene::SceneId Project::RegisterScene(stringv name) {
  const string key{name};
  scene::SceneId *pExisting = _sceneNameMap.Retrieve(key);
  if (pExisting != nullptr) {
    return *pExisting;
  }

  scene::SceneId id = _nextSceneId++;
  auto _ = _sceneNameMap.Insert(key, id);
  return id;
}

void Project::DestroyScene(scene::SceneId id) {
  if (id == scene::cNullScene) {
    return;
  }

  std::vector<ecs::EntityId> toDestroy;
  for (auto [eid, ownership] : _registry.ViewOf<scene::SceneOwnership>()) {
    if (ownership.sceneId == id) {
      toDestroy.push_back(eid);
    }
  }
  for (auto eid : toDestroy) {
    _registry.Destroy(eid);
  }
}

scene::SceneId Project::GetSceneId(stringv name) {
  const string key{name};
  scene::SceneId *p = _sceneNameMap.Retrieve(key);
  return p ? *p : scene::cNullScene;
}

} // namespace flatearth::modules

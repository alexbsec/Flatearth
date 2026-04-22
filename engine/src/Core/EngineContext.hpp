#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

#include "Assets/AssetManager.hpp"
#include "Assets/PrefabManager.hpp"
#include "Assets/TilemapManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "ECS/Registry.hpp"
#include "Physics/FlatearthWorld.hpp"
#include "Scene/SceneManager.hpp"

namespace flatearth {

struct EngineContext {
  struct {
    assets::AssetManager &manager;
    assets::TilemapManager &tilemap;
    assets::PrefabManager &prefab;
  } assets;

  struct {
    memory::MemoryManager &memory;
    input::InputManager &input;
  } core;

  ecs::Registry &registry;
  scene::SceneManager &sceneManager;
  physics::FlatearthWorld &world;

  explicit EngineContext(memory::MemoryManager &memManager,
                         assets::AssetManager &assetManager,
                         assets::TilemapManager &tilemapManager,
                         assets::PrefabManager &prefabManager,
                         input::InputManager &inputManager,
                         ecs::Registry &registry,
                         scene::SceneManager &sceneManager,
                         physics::FlatearthWorld &world)
      : assets(assetManager, tilemapManager, prefabManager), core(memManager, inputManager),
        registry(registry), sceneManager(sceneManager), world(world) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

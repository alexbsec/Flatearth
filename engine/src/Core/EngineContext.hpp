#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

#include "Assets/AssetManager.hpp"
#include "Assets/PrefabManager.hpp"
#include "Assets/TilemapManager.hpp"
#include "Audio/AudioManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Core/KVarRegistry.hpp"
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
    audio::AudioManager &audio;
    KVarRegistry &kvars;
  } core;

  ecs::Registry &registry;
  scene::SceneManager &sceneManager;
  physics::FlatearthWorld &world;
  uint32 drawCallCount{0};

  explicit EngineContext(memory::MemoryManager &memManager,
                         assets::AssetManager &assetManager,
                         assets::TilemapManager &tilemapManager,
                         assets::PrefabManager &prefabManager,
                         input::InputManager &inputManager,
                         audio::AudioManager &audioManager,
                         KVarRegistry &kvarRegistry,
                         ecs::Registry &registry,
                         scene::SceneManager &sceneManager,
                         physics::FlatearthWorld &world)
      : assets(assetManager, tilemapManager, prefabManager),
        core(memManager, inputManager, audioManager, kvarRegistry),
        registry(registry), sceneManager(sceneManager), world(world) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

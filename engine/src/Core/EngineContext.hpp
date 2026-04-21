#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

#include "Assets/AssetManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "ECS/Registry.hpp"
#include "Physics/FlatearthWorld.hpp"
#include "Scene/SceneManager.hpp"

namespace flatearth {

struct EngineContext {
  assets::AssetManager &assetManager;
  memory::MemoryManager &memoryManager;
  input::InputManager &inputManager;
  ecs::Registry &registry;
  scene::SceneManager &sceneManager;
  physics::FlatearthWorld &world;

  explicit EngineContext(memory::MemoryManager &memManager,
                         assets::AssetManager &assetManager,
                         input::InputManager &inputManager,
                         ecs::Registry &registry,
                         scene::SceneManager &sceneManager,
                         physics::FlatearthWorld &world)
      : assetManager(assetManager), memoryManager(memManager), inputManager(inputManager),
        registry(registry), sceneManager(sceneManager), world(world) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

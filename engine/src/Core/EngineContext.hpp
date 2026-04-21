#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

#include "Assets/AssetManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "ECS/Registry.hpp"

namespace flatearth {

struct EngineContext {
  assets::AssetManager &assetManager;
  memory::MemoryManager &memoryManager;
  input::InputManager &inputManager;
  ecs::Registry &registry;

  explicit EngineContext(memory::MemoryManager &memManager,
                         assets::AssetManager &assetManager,
                         input::InputManager &inputManager,
                         ecs::Registry &registry)
      : assetManager(assetManager), memoryManager(memManager), inputManager(inputManager),
        registry(registry) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

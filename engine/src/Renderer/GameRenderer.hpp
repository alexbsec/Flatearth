#ifndef _FLATEARTH_ENGINE_RENDERER_GAME_RENDERER_HPP
#define _FLATEARTH_ENGINE_RENDERER_GAME_RENDERER_HPP

#include "Core/FeMemory.hpp"
#include "ECS/Registry.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
namespace flatearth::renderer {

class GameRenderer {
public:
  explicit GameRenderer(ApplicationState *pAppState, memory::MemoryManager &memManager,
                              ecs::Registry &registry, platform::FileSystem &fs);

  FeExpect<bool, Error> Initialize();
  FeExpect<bool, Error> Draw(float32 deltaTime);
  void Shutdown();

  FrontendRenderer &FrontendReference();

private:
  renderer::FrontendRenderer _frontendRenderer;
  memory::MemoryManager &_memoryManager;
  ecs::Registry &_registry;
};

} // namespace flatearth::renderer

#endif // _FLATEARTH_ENGINE_RENDERER_GAME_RENDERER_HPP

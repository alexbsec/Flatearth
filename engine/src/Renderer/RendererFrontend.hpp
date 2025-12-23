#ifndef _FLATEARTH_ENGINE_RENDERER_FRONTEND_HPP
#define _FLATEARTH_ENGINE_RENDERER_FRONTEND_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "Renderer/RendererTypes.hpp"

namespace flatearth::renderer {

class FrontendRenderer {
public:
  explicit FrontendRenderer(ApplicationState *appState,
                            memory::MemoryManager &memManager);
  ~FrontendRenderer() = default;

  FeExpect<bool, Error> Initialize();
  FeExpect<bool, Error> BeginFrame(float32 deltaTime);
  FeExpect<bool, Error> EndFrame(float32 deltaTime);
  FeExpect<bool, Error> DrawFrame(RenderPacket *pRenderPacket);
  FeExpect<void, Error> OnResize(uint32 width, uint32 height);

private:
  FeExpect<void, Error> MakeBackends();

private:
  std::array<FePtr<IRendererBackend>, scMaxBackends> _pBackends;
  IRendererBackend *_pActiveBackend;

  memory::MemoryManager &_memoryManager;
  ApplicationState *_pAppState;
  string _applicationName;
};

} // namespace flatearth::renderer

#endif // _FLATEARTH_ENGINE_RENDERER_FRONTEND_HPP

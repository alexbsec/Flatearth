#ifndef _FLATEARTH_ENGINE_RENDERER_FRONTEND_HPP
#define _FLATEARTH_ENGINE_RENDERER_FRONTEND_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererInterface.hpp"

namespace flatearth::renderer {

struct RendererState {
  IRendererBackend *pActiveBackend;
  math::Mat4D projection, view;
  float32 nearClip{-1.0f}, farClip{1.0f};
};

class FrontendRenderer {
public:
  explicit FrontendRenderer(ApplicationState *appState,
                            memory::MemoryManager &memManager,
                            platform::FileSystem &fs);
  ~FrontendRenderer();

  FeExpect<bool, Error> Initialize();
  FeExpect<bool, Error> BeginFrame(float32 deltaTime);
  FeExpect<bool, Error> EndFrame(float32 deltaTime);
  FeExpect<bool, Error> DrawFrame(RenderPacket *pRenderPacket);
  FeExpect<void, Error> OnResize(uint32 width, uint32 height);

  FeExpect<void, Error> CreateTexture(const string &name,
                                      bool autoRelease,
                                      int32 width,
                                      int32 height,
                                      int32 channelCount,
                                      const uint8 *pPixels,
                                      bool hasTransparency,
                                      resources::Texture *pTexture);
  FeExpect<void, Error> DestroyTexture(resources::Texture *pTexture);

  FEAPI FeExpect<void, Error> CreateMaterial(resources::Material *pMaterial,
                                             const resources::Texture *pTexture);
  FEAPI FeExpect<void, Error> DestroyMaterial(resources::Material *pMaterial);

  FEAPI FeExpect<void, Error> CreateGeometry(uint32 id,
                                             uint32 vertexCount,
                                             const math::Vertex3D *pVertices,
                                             uint32 indexCount,
                                             const uint32 *pIndices);
  FEAPI FeExpect<void, Error> DestroyGeometry(uint32 id);

private:
  FeExpect<void, Error> MakeBackends();

private:
  std::array<FePtr<IRendererBackend>, scMaxBackends> _pBackends;
  RendererState _rendererState;

  memory::MemoryManager &_memoryManager;
  ApplicationState *_pAppState;
  string _applicationName;
  platform::FileSystem &_filesystem;
};

} // namespace flatearth::renderer

#endif // _FLATEARTH_ENGINE_RENDERER_FRONTEND_HPP

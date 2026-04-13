#ifndef _FLATEARHT_ENGINE_RENDERER_INTERFACE_HPP
#define _FLATEARHT_ENGINE_RENDERER_INTERFACE_HPP

#include "Core/ApplicationConfig.hpp"
#include "Defines.hpp"
#include "Math/MathTypes.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::renderer {

enum class BackendType {
  Vulkan = 0,
  OpenGL,
  DirectX,
  MaxBackends,
};

static constexpr int32 scMaxBackends = static_cast<int32>(BackendType::MaxBackends);

class IRendererBackend {
public:
  virtual FeExpect<bool, Error> Initialize(ApplicationState *appState) = 0;
  virtual FeExpect<bool, Error> OnResize(uint32 width, uint32 height) = 0;
  virtual FeExpect<bool, Error> BeginFrame(float32 deltaTime) = 0;
  virtual FeExpect<bool, Error> EndFrame(float32 deltaTime) = 0;
  virtual FeExpect<void, Error> UpdateGlobalState(math::Mat4D projection,
                                                  math::Mat4D view,
                                                  math::Vec3D viewPosition,
                                                  int32 mode) = 0;
  virtual FeExpect<void, Error> CreateTexture(const string &name,
                                              bool autoRelease,
                                              int32 width,
                                              int32 height,
                                              int32 channelCount,
                                              const uint8 *pPixels,
                                              bool hasTransparency,
                                              resources::Texture *pTexture) = 0;
  virtual FeExpect<void, Error> DestroyTexture(resources::Texture *pTexture) = 0;

  virtual FeExpect<void, Error> CreateGeometry(uint32 id,
                                               uint32 vertexCount,
                                               const math::Vertex3D *pVertices,
                                               uint32 indexCount,
                                               const uint32 *pIndices) = 0;
  virtual FeExpect<void, Error> DestroyGeometry(uint32 id) = 0;
  virtual void DrawGeometry(uint32 id, math::Mat4D model) = 0;

protected:
  virtual ~IRendererBackend() = default;
};

} // namespace flatearth::renderer

#endif // _FLATEARHT_ENGINE_RENDERER_INTERFACE_HPP

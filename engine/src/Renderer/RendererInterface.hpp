#ifndef _FLATEARHT_ENGINE_RENDERER_INTERFACE_HPP
#define _FLATEARHT_ENGINE_RENDERER_INTERFACE_HPP

#include "Core/ApplicationConfig.hpp"
#include "Defines.hpp"

namespace flatearth::renderer {

enum class BackendType {
  Vulkan = 0,
  OpenGL,
  DirectX,
  MaxBackends,
};

static constexpr int32 scMaxBackends = static_cast<int32>(BackendType::MaxBackends);

struct RenderPacket {
  float32 deltaTime;
};

class IRendererBackend {
public:
  virtual FeExpect<bool, Error> Initialize(ApplicationState* appState) = 0;
  virtual FeExpect<bool, Error> OnResize(uint32 width, uint32 height) = 0;
  virtual FeExpect<bool, Error> BeginFrame(float32 deltaTime) = 0;
  virtual FeExpect<bool, Error> EndFrame(float32 deltaTime) = 0;
  virtual FeExpect<bool, Error> DrawFrame(const RenderPacket& renderPacket) = 0;
  virtual FeExpect<void, Error> UpdateGlobalState(math::Mat4D projection,
                                                  math::Mat4D view,
                                                  math::Vec3D viewPosition,
                                                  int32 mode) = 0;

protected:
  virtual ~IRendererBackend() = default;
};

} // namespace flatearth::renderer

#endif // _FLATEARHT_ENGINE_RENDERER_INTERFACE_HPP

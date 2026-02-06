#include "Renderer/RendererFrontend.hpp"

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Math/Matrix4D.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererInterface.hpp"
#include "Renderer/Vulkan/VulkanBackend.hpp"

namespace flatearth::renderer {

FrontendRenderer::FrontendRenderer(ApplicationState *appState,
                                   memory::MemoryManager &memManager,
                                   platform::FileSystem &fs)
    : _applicationName(appState->appConfig.name), _memoryManager(memManager), _pAppState(appState),
      _filesystem(fs) {
}

FrontendRenderer::~FrontendRenderer() {
  FLOG_INFO("frontend renderer exited gracefully");
}

FeExpect<bool, Error> FrontendRenderer::Initialize() {
  auto backendsRes = MakeBackends();
  if (!backendsRes.has_value()) {
    FLOG_ERROR("failed to scaffold renderer backends: {}", backendsRes.error().message);
    return FeErr{backendsRes.error()};
  }

  // TODO: make this selection smart once OpenGL is integrated
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  if (_pBackends[vulkanIndex] == nullptr) {
    FLOG_FATAL("no valid backend found");
    return FeErr{Error("no backend was set in frontend renderer", ErrorType::NoBackendRenderer)};
  }

  _rendererState.pActiveBackend = _pBackends[vulkanIndex].get();
  auto backendInitRes = _rendererState.pActiveBackend->Initialize(_pAppState);
  if (!backendInitRes.has_value()) {
    FLOG_ERROR("failed to initialize backend renderer");
    return FeErr{backendInitRes.error()};
  }

  // Set up renderer state values
  float32 aspect = static_cast<float32>(_pAppState->width) / _pAppState->height;
  _rendererState.projection = math::Mat4D::Orthographic(
      -aspect, aspect, -1.0f, 1.0f, _rendererState.nearClip, _rendererState.farClip);
  _rendererState.view = math::Mat4D::Identity();

  FLOG_INFO("frontend renderer successfully initialized");
  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::BeginFrame(float32 deltaTime) {
  if (_rendererState.pActiveBackend == nullptr) {
    FLOG_WARN("no active backends");
    return FeFalse;
  }

  auto res = _rendererState.pActiveBackend->BeginFrame(deltaTime);
  if (!res.has_value()) {
    FLOG_ERROR("backend renderer failed to begin frame");
    return FeErr{res.error()};
  }

  return res.value();
}

FeExpect<bool, Error> FrontendRenderer::EndFrame(float32 deltaTime) {
  if (_rendererState.pActiveBackend == nullptr) {
    FLOG_WARN("no active backends");
    return FeFalse;
  }

  auto res = _rendererState.pActiveBackend->EndFrame(deltaTime);
  if (!res.has_value()) {
    FLOG_ERROR("backend renderer failed to end frame");
    return FeErr{res.error()};
  }

  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::DrawFrame(RenderPacket *pRenderPacket) {
  if (pRenderPacket == nullptr) {
    FLOG_WARN("nullptr renderpacket passed");
    return FeFalse;
  }

  auto beginRes = BeginFrame(pRenderPacket->deltaTime);
  if (!beginRes.has_value()) {
    FLOG_ERROR("failed to begin frame");
    return FeErr{beginRes.error()};
  }

  if (!beginRes.value()) {
    // Skips frame rendering (usually when resizing)
    return FeTrue;
  }


  static float32 angle = 0.0f;
  // radians per second (tweak speed)
  angle += pRenderPacket->deltaTime * 1.5f;

  math::Mat4D model = math::Mat4D::RotationZ(angle) * math::Mat4D::Translation(0.0f, 0.0f, 0.0f);
  _rendererState.pActiveBackend->UpdateObject(model);
  auto updateRes = _rendererState.pActiveBackend->UpdateGlobalState(
      _rendererState.projection, _rendererState.view, math::Vec3D::Zero(), 0);
  if (!updateRes.has_value()) {
    FLOG_ERROR("failed to update global state on frontend renderer");
    return FeErr{updateRes.error()};
  }

  auto endRes = EndFrame(pRenderPacket->deltaTime);
  if (!endRes.has_value()) {
    FLOG_ERROR("failed to end frame");
    return FeErr{endRes.error()};
  }

  return FeTrue;
}

FeExpect<void, Error> FrontendRenderer::OnResize(uint32 width, uint32 height) {
  if (_rendererState.pActiveBackend == nullptr) {
    FLOG_WARN("no active backends");
    return {};
  }

  float32 aspect = static_cast<float32>(width) / height;
  _rendererState.projection = math::Mat4D::Orthographic(
      aspect, -aspect, -1.0f, 1.0f, _rendererState.nearClip, _rendererState.farClip);
  auto res = _rendererState.pActiveBackend->OnResize(width, height);
  if (!res.has_value()) {
    FLOG_ERROR("backend renderer failed to resize");
    return FeErr{res.error()};
  }

  return {};
}

void FrontendRenderer::SetView(const math::Mat4D &view) {
  _rendererState.view = view;
}

FeExpect<void, Error> FrontendRenderer::MakeBackends() {
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  _pBackends[vulkanIndex] = _memoryManager.Allocate<IRendererBackend, vulkan::VulkanBackend>(
      memory::Tag::Renderer, _memoryManager, _filesystem);
  return {};
}

} // namespace flatearth::renderer

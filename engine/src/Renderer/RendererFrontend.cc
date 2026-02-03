#include "Renderer/RendererFrontend.hpp"

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererInterface.hpp"
#include "Renderer/Vulkan/VulkanBackend.hpp"

namespace flatearth::renderer {

FrontendRenderer::FrontendRenderer(ApplicationState* appState,
                                   memory::MemoryManager& memManager,
                                   platform::FileSystem& fs)
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

  _pActiveBackend = _pBackends[vulkanIndex].get();
  auto backendInitRes = _pActiveBackend->Initialize(_pAppState);
  if (!backendInitRes.has_value()) {
    FLOG_ERROR("failed to initialize backend renderer");
    return FeErr{backendInitRes.error()};
  }

  FLOG_INFO("frontend renderer successfully initialized");
  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::BeginFrame(float32 deltaTime) {
  if (_pActiveBackend == nullptr) {
    FLOG_WARN("no active backends");
    return FeFalse;
  }

  auto res = _pActiveBackend->BeginFrame(deltaTime);
  if (!res.has_value()) {
    FLOG_ERROR("backend renderer failed to begin frame");
    return FeErr{res.error()};
  }

  return res.value();
}

FeExpect<bool, Error> FrontendRenderer::EndFrame(float32 deltaTime) {
  if (_pActiveBackend == nullptr) {
    FLOG_WARN("no active backends");
    return FeFalse;
  }

  auto res = _pActiveBackend->EndFrame(deltaTime);
  if (!res.has_value()) {
    FLOG_ERROR("backend renderer failed to end frame");
    return FeErr{res.error()};
  }

  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::DrawFrame(RenderPacket* pRenderPacket) {
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

  auto updateRes = _pActiveBackend->UpdateGlobalState(
      math::Mat4D::Identity(), math::Mat4D::Identity(), math::Vec3D::Zero(), 0);
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
  if (_pActiveBackend == nullptr) {
    FLOG_WARN("no active backends");
    return {};
  }

  auto res = _pActiveBackend->OnResize(width, height);
  if (!res.has_value()) {
    FLOG_ERROR("backend renderer failed to resize");
    return FeErr{res.error()};
  }

  return {};
}

FeExpect<void, Error> FrontendRenderer::MakeBackends() {
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  _pBackends[vulkanIndex] = _memoryManager.Allocate<IRendererBackend, vulkan::VulkanBackend>(
      memory::Tag::Renderer, _memoryManager, _filesystem);
  return {};
}

} // namespace flatearth::renderer

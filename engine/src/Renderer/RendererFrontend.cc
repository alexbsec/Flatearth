#include "Renderer/RendererFrontend.hpp"
#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBackend.hpp"

namespace flatearth::renderer {

FrontendRenderer::FrontendRenderer(ApplicationState *appState,
                                   memory::MemoryManager &memManager)
    : _applicationName(appState->appConfig.name), _memoryManager(memManager),
      _appState(appState) {}

FeExpect<bool, Error>
FrontendRenderer::Initialize() {
  auto backendsRes = MakeBackends();
  if (!backendsRes.has_value()) {
    FLOG_ERROR("failed to scaffold renderer backends: {}",
               backendsRes.error().message);
    return FeErr{backendsRes.error()};
  }

  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  if (_pBackends[vulkanIndex] == nullptr) {
    FLOG_FATAL("no valid backend found");
    return FeErr{Error("no backend was set in frontend renderer",
                       ErrorType::NoBackendRenderer)};
  }

  _pActiveBackend = _pBackends[vulkanIndex].get();
  auto backendInitRes = _pActiveBackend->Initialize(_appState);
  if (!backendInitRes.has_value()) {
    FLOG_ERROR("failed to initialize backend renderer");
    return FeErr{backendInitRes.error()};
  }

  FLOG_INFO("frontend renderer successfully initialized");
  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::BeginFrame(float32 deltaTime) {
  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::EndFrame(float32 deltaTime) {
  return FeTrue;
}

FeExpect<bool, Error> FrontendRenderer::DrawFrame(RenderPacket *pRenderPacket) {
  return FeTrue;
}

FeExpect<void, Error> OnResize(uint32 width, uint32 height) { return {}; }

FeExpect<void, Error> FrontendRenderer::MakeBackends() {
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  _pBackends[vulkanIndex] =
      _memoryManager.Allocate<IRendererBackend, vulkan::VulkanBackend>(
          memory::Tag::Renderer, _memoryManager);
  return {};
}

} // namespace flatearth::renderer

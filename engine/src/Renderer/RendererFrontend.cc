#include "Renderer/RendererFrontend.hpp"

#include "Algorithms/StableSort.hpp"
#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Math/Matrix4D.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererInterface.hpp"
#include "Renderer/Vulkan/VulkanBackend.hpp"
#include "Resources/ResourceTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer {

FrontendRenderer::FrontendRenderer(ApplicationState *appState,
                                   memory::MemoryManager &memManager,
                                   platform::FileSystem &fs)
    : _applicationName(appState->appConfig.name), _memoryManager(memManager), _pAppState(appState),
      _filesystem(fs), _meshCache(memManager, *this), _materialCache(memManager, *this) {
}

FrontendRenderer::~FrontendRenderer() {
  FLOG_INFO("frontend renderer exited gracefully");
}

void FrontendRenderer::Shutdown() {
  _meshCache.Shutdown();
  _materialCache.Shutdown();
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

  auto updateRes = _rendererState.pActiveBackend->UpdateGlobalState(
      _rendererState.projection, pRenderPacket->view, math::Vec3D::Zero(), 0);
  if (!updateRes.has_value()) {
    FLOG_ERROR("failed to update global state on frontend renderer");
    return FeErr{updateRes.error()};
  }

  algorithms::StableSort(
      _memoryManager, pRenderPacket->objects, [](const RenderObject &a, const RenderObject &b) {
        if (a.layer != b.layer) {
          return a.layer < b.layer;
        }

        if (a.pMaterial != b.pMaterial) {
          return a.pMaterial < b.pMaterial;
        }

        return a.geometryId < b.geometryId;
      });

  for (uint32 i = 0; i < pRenderPacket->objects.Length(); i++) {
    const RenderObject &object = pRenderPacket->objects[i];
    const PushConstantData data{object.model, object.uvOffset, object.uvScale};
    _rendererState.pActiveBackend->DrawGeometry(object.geometryId, data, object.pMaterial);
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

FeExpect<void, Error> FrontendRenderer::CreateTexture(const string &name,
                                                      bool autoRelease,
                                                      int32 width,
                                                      int32 height,
                                                      int32 channelCount,
                                                      const uint8 *pPixels,
                                                      bool hasTransparency,
                                                      resources::Texture *pTexture,
                                                      resources::TextureFilter filter) {
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  return _pBackends[vulkanIndex]->CreateTexture(
      name, autoRelease, width, height, channelCount, pPixels, hasTransparency, pTexture, filter);
}

FeExpect<void, Error> FrontendRenderer::CreateGeometry(uint32 id,
                                                       uint32 vertexCount,
                                                       const math::Vertex3D *pVertices,
                                                       uint32 indexCount,
                                                       const uint32 *pIndices) {
  return _rendererState.pActiveBackend->CreateGeometry(
      id, vertexCount, pVertices, indexCount, pIndices);
}

FeExpect<void, Error> FrontendRenderer::DestroyGeometry(uint32 id) {
  return _rendererState.pActiveBackend->DestroyGeometry(id);
}

FeExpect<void, Error> FrontendRenderer::DestroyTexture(resources::Texture *pTexture) {
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  return _pBackends[vulkanIndex]->DestroyTexture(pTexture);
}

FeExpect<void, Error> FrontendRenderer::CreateMaterial(resources::Material *pMaterial,
                                                       const resources::Texture *pTexture) {
  return _rendererState.pActiveBackend->CreateMaterial(pMaterial, pTexture);
}

FeExpect<void, Error> FrontendRenderer::DestroyMaterial(resources::Material *pMaterial) {
  return _rendererState.pActiveBackend->DestroyMaterial(pMaterial);
}

FeExpect<resources::MeshHandle, Error> FrontendRenderer::AcquireMesh(resources::MeshShape shape) {
  return _meshCache.AcquireMesh(shape);
}

void FrontendRenderer::ReleaseMesh(resources::MeshHandle handle) {
  _meshCache.Release(handle);
}

resources::Mesh *FrontendRenderer::GetMesh(resources::MeshHandle handle) {
  return _meshCache.Get(handle);
}

FeExpect<resources::MaterialHandle, Error>
FrontendRenderer::AcquireMaterial(const string &name, resources::Texture *pTexture) {
  return _materialCache.AcquireMaterial(name, pTexture);
}

void FrontendRenderer::ReleaseMaterial(resources::MaterialHandle handle) {
  _materialCache.Release(handle);
}

resources::Material *FrontendRenderer::GetMaterial(resources::MaterialHandle handle) {
  return _materialCache.Get(handle);
}

FeExpect<void, Error> FrontendRenderer::MakeBackends() {
  uint32 vulkanIndex = static_cast<uint32>(BackendType::Vulkan);
  _pBackends[vulkanIndex] = _memoryManager.Allocate<IRendererBackend, vulkan::VulkanBackend>(
      memory::Tag::Renderer, _memoryManager, _filesystem);
  return {};
}

} // namespace flatearth::renderer

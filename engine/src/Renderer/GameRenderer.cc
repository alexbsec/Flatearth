#include "GameRenderer.hpp"

#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Scene/Components/Camera2D.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/Transform2D.hpp"
#include "UI/Components/UIAnchor.hpp"

namespace flatearth::renderer {

GameRenderer::GameRenderer(EngineState *pEngState,
                           memory::MemoryManager &memManager,
                           ecs::Registry &reg,
                           platform::FileSystem &fs)
    : _memoryManager(memManager), _frontendRenderer(pEngState, memManager, fs), _registry(reg) {
}

FeExpect<void, Error> GameRenderer::Initialize() {
  return _frontendRenderer.Initialize();
}

FeExpect<bool, Error> GameRenderer::Draw(float32 deltaTime) {
  using namespace scene;
  using namespace ecs;
  using namespace ui;

  math::Mat4D view = math::Mat4D::Identity();
  auto cameraView = _registry.ViewOf<Transform2D, Camera2D>();
  for (auto [entity, transform, cam] : cameraView) {
    // grabs first active camera
    view = math::Mat4D::Scale(cam.zoom, cam.zoom, 1.0f) *
           math::Mat4D::Translation(-transform.worldX, -transform.worldY, 0.0f) *
           math::Mat4D::RotationZ(-transform.worldRotation);
    break;
  }

  RenderPacket packet(_memoryManager);
  packet.deltaTime = deltaTime;
  packet.view = view;

  View<Transform2D, Sprite> gameObjectView = _registry.ViewOf<Transform2D, Sprite>();
  for (auto [entity, transform, sprite] : gameObjectView) {
    resources::Mesh *pMesh = _frontendRenderer.GetMesh(sprite.meshHandle);
    resources::Material *pMat = _frontendRenderer.GetMaterial(sprite.matHandle);
    if (pMesh == nullptr || pMat == nullptr) {
      continue;
    }

    math::Mat4D model = math::Mat4D::Translation(transform.worldX, transform.worldY, 0.0f) *
                        math::Mat4D::RotationZ(transform.worldRotation) *
                        math::Mat4D::Scale(transform.worldScaleX, transform.worldScaleY, 1.0f);
    RenderObject object{
        .geometryId = pMesh->id,
        .model = model,
        .uvOffset = sprite.uvOffset,
        .uvScale = sprite.uvScale,
        .layer = sprite.layer,
        .pMaterial = pMat,
    };
    packet.objects.Push(object);
  }

  View<UIAnchor, Sprite> uiObjectView = _registry.ViewOf<UIAnchor, Sprite>();
  for (auto [entity, uiAnchor, sprite] : uiObjectView) {
    resources::Mesh *pMesh = _frontendRenderer.GetMesh(sprite.meshHandle);
    resources::Material *pMat = _frontendRenderer.GetMaterial(sprite.matHandle);
    if (pMesh == nullptr || pMat == nullptr) {
      continue;
    }

    float32 ndcX = uiAnchor.normalizedX * 2 - 1, ndcY = uiAnchor.normalizedY * 2 - 1;
    math::Mat4D model = math::Mat4D::Translation(ndcX, ndcY, 0.0f) *
                        math::Mat4D::RotationZ(uiAnchor.rotation) *
                        math::Mat4D::Scale(uiAnchor.scaleX, uiAnchor.scaleY, 1.0f);

    RenderObject object{
        .geometryId = pMesh->id,
        .model = model,
        .uvOffset = sprite.uvOffset,
        .uvScale = sprite.uvScale,
        .layer = RenderLayer::UI,
        .pMaterial = pMat,
        .tint = {math::Vec3D{1.0f, 1.0f, 1.0f}, 1.0f},
        .useTexture = 1.0f,
    };
    packet.uiObjects.Push(object);
  }

  _lastDrawCallCount = packet.objects.Length() + packet.uiObjects.Length();
  return _frontendRenderer.DrawFrame(&packet);
}

void GameRenderer::Flush() {
  _frontendRenderer.Flush();
}

void GameRenderer::BeginImGuiFrame() {
  _frontendRenderer.BeginImGuiFrame();
}

FrontendRenderer &GameRenderer::FrontendReference() {
  return _frontendRenderer;
}

void GameRenderer::Shutdown() {
  _frontendRenderer.Shutdown();
}

} // namespace flatearth::renderer

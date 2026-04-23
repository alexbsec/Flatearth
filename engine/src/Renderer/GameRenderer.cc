#include "GameRenderer.hpp"

#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Scene/Components/Camera2D.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/Transform2D.hpp"

namespace flatearth::renderer {

GameRenderer::GameRenderer(ApplicationState *appState,
                           memory::MemoryManager &memManager,
                           ecs::Registry &reg,
                           platform::FileSystem &fs)
    : _memoryManager(memManager), _frontendRenderer(appState, memManager, fs), _registry(reg) {
}

FeExpect<bool, Error> GameRenderer::Initialize() {
  return _frontendRenderer.Initialize();
}

FeExpect<bool, Error> GameRenderer::Draw(float32 deltaTime) {
  using namespace scene;
  using namespace ecs;

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

  View<Transform2D, Sprite> spriteView = _registry.ViewOf<Transform2D, Sprite>();
  for (auto [entity, transform, sprite] : spriteView) {
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

  return _frontendRenderer.DrawFrame(&packet);
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

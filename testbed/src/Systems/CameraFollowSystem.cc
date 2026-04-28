#include "CameraFollowSystem.hpp"
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>

#include "../Components/Tags.hpp"

namespace flatearth::testbed {

CameraFollowSystem::CameraFollowSystem(EngineContext &ctx,
                                       const string &sceneName)
  : _ctx(ctx), _sceneName(sceneName) {}

void CameraFollowSystem::Initialize(ecs::Registry &registry) {
  ecs::EntityId cameraId = registry.Create();
  const scene::SceneOwnership ownership{_sceneName};
  registry.Insert(cameraId, scene::Transform2D{});
  registry.Insert(cameraId, scene::Camera2D{.zoom = 0.2f});
  registry.Insert(cameraId, ownership);

  auto res = _ctx.core.KVarsRegistry().Register("camera.zoom", "Camera zoom scale", 0.2f);
  if (!res.has_value()) {
    FLOG_ERROR("failed to register camera zoom kvar");
  }
}

void CameraFollowSystem::Update(ecs::Registry &registry, float32) {
  auto &kvars = _ctx.core.KVarsRegistry();

  float32 playerX = 0.0f, playerY = 0.0f;
  bool hasPlayer = false;
  for (auto [id, ptag, xform] : registry.ViewOf<PlayerTag, scene::Transform2D>()) {
    playerX = xform.x;
    playerY = xform.y;
    hasPlayer = true;
  }
  if (hasPlayer) {
    for (auto [id, xform, cam] : registry.ViewOf<scene::Transform2D, scene::Camera2D>()) {
      cam.zoom = kvars.Get<float32>("camera.zoom").value_or(0.2f);
      xform.x = playerX;
      xform.y = playerY;
      xform.dirty = FeTrue;
    }
  }
}

}

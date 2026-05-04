#include "CameraFollowSystem.hpp"

#include "../Components/Tags.hpp"

#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

CameraFollowSystem::CameraFollowSystem(EngineContext &ctx, scene::SceneId sceneId)
    : _ctx(ctx), _sceneId(sceneId) {
}

void CameraFollowSystem::Initialize(ecs::Registry &registry) {
  registry.Spawn()
      .With(scene::Transform2D{})
      .With(scene::Camera2D{.zoom = 0.2f})
      .OwnedBy(_sceneId)
      .Commit();

  _ctx.core.KVarsRegistry()
      .Register("camera.zoom", "Camera zoom scale", 0.2f)
      .or_log_error("failed to register camera zoom kvar");
}

void CameraFollowSystem::Update(ecs::Registry &registry, float32) {
  auto &kvars = _ctx.core.KVarsRegistry();

  float32 playerX = 0.0f, playerY = 0.0f;
  bool hasPlayer = FeFalse;
  for (auto [id, ptag, xform] : registry.ViewOf<PlayerTag, scene::Transform2D>()) {
    playerX = xform.x;
    playerY = xform.y;
    hasPlayer = FeTrue;
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

} // namespace flatearth::testbed

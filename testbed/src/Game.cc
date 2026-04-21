#include "Game.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Transform2D.hpp>

namespace flatearth::testbed {

GameState GameTest::_state = GameState{};

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  gameInstance->windowStartWidth = 1280;
  gameInstance->windowStartHeight = 720;
  gameInstance->windowStartPosX = 100;
  gameInstance->windowStartPosY = 100;
  gameInstance->gameName = "Testbed";
  return FeTrue;
}

bool GameTest::GameLoad(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;

  // Camera entity
  _state.cameraEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.cameraEntity, scene::Transform2D{});
  ctx.registry.Insert(_state.cameraEntity, scene::Camera2D{.zoom = 0.5f});

  // Quad sprite
  auto quadRes = ctx.assetManager.LoadSprite("assets/textures/texture.jpg",
                                             resources::MeshShape::Quad);
  if (!quadRes.has_value()) {
    FLOG_ERROR("failed to load quad sprite");
    return FeFalse;
  }
  _state.quadSprite = quadRes.value();
  _state.quadEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.quadEntity, scene::Transform2D{});
  ctx.registry.Insert(_state.quadEntity, _state.quadSprite);

  // Circle sprite
  auto circleRes = ctx.assetManager.LoadSprite("assets/textures/texture.jpg",
                                               resources::MeshShape::Circle);
  if (!circleRes.has_value()) {
    FLOG_ERROR("failed to load circle sprite");
    return FeFalse;
  }
  _state.circleSprite = circleRes.value();
  _state.circleEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.circleEntity, scene::Transform2D{0.0f, 1.2f});
  ctx.registry.Insert(_state.circleEntity, _state.circleSprite);

  // Capsule sprite
  auto capsuleRes = ctx.assetManager.LoadSprite("assets/textures/rugtexture.jpg",
                                                resources::MeshShape::Capsule2D);
  if (!capsuleRes.has_value()) {
    FLOG_ERROR("failed to load capsule sprite");
    return FeFalse;
  }
  _state.capsuleSprite = capsuleRes.value();
  _state.capsuleEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.capsuleEntity, scene::Transform2D{-1.2f, 0.0f});
  ctx.registry.Insert(_state.capsuleEntity, _state.capsuleSprite);

  return FeTrue;
}

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  if (!gameInstance->pCtx) return FeFalse;
  EngineContext &ctx = *gameInstance->pCtx;
  input::InputManager &input = ctx.inputManager;

  const float32 dy = 3.0f * deltaTime;
  const float32 dx = 3.0f * deltaTime;
  const float32 zoomFactor = 1.0f + 1.5f * deltaTime;

  auto &camTransform = ctx.registry.Get<scene::Transform2D>(_state.cameraEntity);
  auto &camComp = ctx.registry.Get<scene::Camera2D>(_state.cameraEntity);
  if (input.IsKeyDown(input::Keys::KEY_W)) { camTransform.y += dy; camTransform.dirty = FeTrue; }
  if (input.IsKeyDown(input::Keys::KEY_S)) { camTransform.y -= dy; camTransform.dirty = FeTrue; }
  if (input.IsKeyDown(input::Keys::KEY_A)) { camTransform.x += dx; camTransform.dirty = FeTrue; }
  if (input.IsKeyDown(input::Keys::KEY_D)) { camTransform.x -= dx; camTransform.dirty = FeTrue; }
  if (input.IsKeyDown(input::Keys::KEY_Z)) { camComp.zoom *= zoomFactor; }
  if (input.IsKeyDown(input::Keys::KEY_X)) { camComp.zoom /= zoomFactor; }

  _state.angle  += deltaTime * 1.5f;
  _state.angle2 -= deltaTime * 2.0f;
  _state.angle3 += deltaTime * 1.0f;

  ctx.registry.Get<scene::Transform2D>(_state.quadEntity).rotation    = _state.angle;
  ctx.registry.Get<scene::Transform2D>(_state.circleEntity).rotation  = _state.angle2;
  ctx.registry.Get<scene::Transform2D>(_state.capsuleEntity).rotation = _state.angle3;

  return FeTrue;
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  if (!gameInstance->pCtx) return;
  EngineContext &ctx = *gameInstance->pCtx;
  ctx.assetManager.ReleaseSprite(_state.quadSprite);
  ctx.assetManager.ReleaseSprite(_state.circleSprite);
  ctx.assetManager.ReleaseSprite(_state.capsuleSprite);
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

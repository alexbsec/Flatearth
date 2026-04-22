#include "Game.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Sprite.hpp>
#include <Scene/Components/SpriteAnimator.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

GameState GameTest::_state = GameState{};

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  gameInstance->windowStartWidth  = 1280;
  gameInstance->windowStartHeight = 720;
  gameInstance->windowStartPosX   = 100;
  gameInstance->windowStartPosY   = 100;
  gameInstance->gameName          = "TopDown";

  EngineContext &ctx = *gameInstance->pCtx;
  ctx.assets.prefab.Register<PlayerPrefab>([&ctx](ecs::EntityId id, ecs::Registry &reg) {
    auto walkTexRes = ctx.assets.manager.LoadTexture("assets/textures/Human_Walk.png");
    if (!walkTexRes.has_value()) return;

    auto playerSpriteRes = ctx.assets.manager.SpriteFromTexture(
        walkTexRes.value(), "player_walk", resources::MeshShape::Quad);
    if (!playerSpriteRes.has_value()) return;

    constexpr float32 uvW = 32.0f / 128.0f;
    constexpr float32 uvH = 32.0f / 128.0f;

    scene::SpriteAnimator animator{};
    animator.frameCount = 4;
    animator.loop       = FeTrue;
    animator.playing    = FeTrue;
    for (uint8 col = 0; col < 4; ++col) {
      animator.frames[col] = {
        .uvOffset = {col * uvW, 1.0f * uvH},
        .uvScale  = {uvW, -uvH},
        .duration = 0.2f,
      };
    }

    scene::Sprite playerSprite = playerSpriteRes.value();
    playerSprite.uvOffset      = animator.frames[0].uvOffset;
    playerSprite.uvScale       = animator.frames[0].uvScale;
    playerSprite.layer         = renderer::RenderLayer::Entities;

    reg.Insert(id, scene::Transform2D{0.0f, 0.0f});
    reg.Insert(id, playerSprite);
    reg.Insert(id, animator);
  });

  return FeTrue;
}

bool GameTest::GameLoad(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;

  auto sceneRes = ctx.sceneManager.Load("level1");
  if (!sceneRes.has_value()) {
    FLOG_ERROR("failed to create scene");
    return FeFalse;
  }
  scene::Scene *pScene = sceneRes.value();

  // Camera — zoomed out to fit 10x10 tile map (1 tile = 1 world unit)
  _state.cameraEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.cameraEntity, scene::Transform2D{});
  ctx.registry.Insert(_state.cameraEntity, scene::Camera2D{.zoom = 0.2f});
  pScene->roots.push_back(_state.cameraEntity);
  pScene->allEntities.push_back(_state.cameraEntity);

  // Tilemap
  auto tmRes = ctx.assets.tilemap.Load("assets/tiles/LevelEntrance.tmx", pScene);
  if (!tmRes.has_value()) {
    FLOG_ERROR("failed to load tilemap: {}", tmRes.error().message);
    return FeFalse;
  }
  _state.tilemapRoot = tmRes.value();

  // ── Player ───────────────────────────────────────────────────────────────
  auto playerRes = ctx.assets.prefab.Spawn<PlayerPrefab>(ctx.registry);
  if (!playerRes.has_value()) {
    FLOG_ERROR("failed to spawn player: {}", playerRes.error().message);
    return FeFalse;
  }
  _state.playerEntity = playerRes.value();
  pScene->allEntities.push_back(_state.playerEntity);

  return FeTrue;
}

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  EngineContext &ctx = *gameInstance->pCtx;

  if (_state.playerEntity == UINT32_MAX) return FeTrue;

  auto &playerXform = ctx.registry.Get<scene::Transform2D>(_state.playerEntity);
  auto &cameraXform = ctx.registry.Get<scene::Transform2D>(_state.cameraEntity);
  auto &anim        = ctx.registry.Get<scene::SpriteAnimator>(_state.playerEntity);
  auto &sprite      = ctx.registry.Get<scene::Sprite>(_state.playerEntity);

  constexpr float32 speed = 1.5f;
  float32 dx = 0.0f, dy = 0.0f;

  auto &input = ctx.core.input;
  if (input.IsKeyDown(input::KEY_W) || input.IsKeyDown(input::KEY_UP))    dy += 1.0f;
  if (input.IsKeyDown(input::KEY_S) || input.IsKeyDown(input::KEY_DOWN))  dy -= 1.0f;
  if (input.IsKeyDown(input::KEY_A) || input.IsKeyDown(input::KEY_LEFT))  dx -= 1.0f;
  if (input.IsKeyDown(input::KEY_D) || input.IsKeyDown(input::KEY_RIGHT)) dx += 1.0f;

  // Walk.png row order: 0=down, 1=left, 2=right, 3=up
  uint8 dirRow = 0;
  if      (dy < 0.0f) dirRow = 0;
  else if (dy > 0.0f) dirRow = 3;
  else if (dx < 0.0f) dirRow = 1;
  else if (dx > 0.0f) dirRow = 2;

  constexpr float32 uvW = 32.0f / 128.0f;
  constexpr float32 uvH = 32.0f / 128.0f;

  bool moving = (dx != 0.0f || dy != 0.0f);

  // Rebuild all animation frames for the new direction
  for (uint8 col = 0; col < 4; ++col) {
    anim.frames[col].uvOffset = {col * uvW, (dirRow + 1) * uvH};
    anim.frames[col].uvScale  = {uvW, -uvH};
  }

  // Push the current frame UV directly to the sprite so direction changes are instant
  anim.playing = moving;
  if (!moving) {
    anim.currentFrame = 0;
    anim.elapsed      = 0.0f;
  }
  sprite.uvOffset = anim.frames[anim.currentFrame].uvOffset;
  sprite.uvScale  = anim.frames[anim.currentFrame].uvScale;

  if (input.IsKeyDown(input::KEY_F) && !input.WasKeyDown(input::KEY_F))
    sprite.flipX = !sprite.flipX;
  if (input.IsKeyDown(input::KEY_G) && !input.WasKeyDown(input::KEY_G))
    sprite.flipY = !sprite.flipY;

  sprite.dirty    = FeTrue;

  playerXform.x += dx * speed * deltaTime;
  playerXform.y += dy * speed * deltaTime;
  playerXform.dirty = FeTrue;

  cameraXform.x     = playerXform.x;
  cameraXform.y     = playerXform.y;
  cameraXform.dirty = FeTrue;

  return FeTrue;
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  if (!gameInstance->pCtx) return;
  EngineContext &ctx = *gameInstance->pCtx;
  ctx.sceneManager.Unload("level1");
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

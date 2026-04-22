#include "Game.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Scene/Components/Camera2D.hpp>
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
  auto tmRes = ctx.tilemapManager.Load("assets/tiles/LevelEntrance.tmx", pScene);
  if (!tmRes.has_value()) {
    FLOG_ERROR("failed to load tilemap: {}", tmRes.error().message);
    return FeFalse;
  }
  _state.tilemapRoot = tmRes.value();

  return FeTrue;
}

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  (void)gameInstance;
  (void)deltaTime;
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

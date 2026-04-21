#include "Game.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Math/FeMath.hpp>
#include <Physics/BoxCollider.hpp>
#include <Physics/CircleCollider.hpp>
#include <Physics/RigidBody.hpp>
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

// World dimensions (orthographic): x in [-aspect, aspect], y in [-1, 1]
static constexpr float32 cAspect = 1280.0f / 720.0f;

// Platform
static constexpr float32 cPlatformY = -0.75f;
static constexpr float32 cPlatformHalfW = 0.25f;
static constexpr float32 cPlatformHalfH = 0.03f;
static constexpr float32 cPlatformSpeed = 2.5f;
static constexpr float32 cPlatformScaleX = cPlatformHalfW * 2.0f;
static constexpr float32 cPlatformScaleY = cPlatformHalfH * 2.0f;

// Ball
static constexpr float32 cBallRadius = 0.05f;
static constexpr float32 cBallScale = cBallRadius * 2.0f;
static constexpr float32 cBallStartX = 0.0f;
static constexpr float32 cBallStartY = 0.3f;
static constexpr float32 cBallInitVx = 0.5f;
static constexpr float32 cBallInitVy = -1.5f;

// Walls
static constexpr float32 cWallThick = 0.05f;

// Ball falls below this y → reset
static constexpr float32 cDeathY = -1.1f;

GameState GameTest::_state = GameState{};

// ── helpers ───────────────────────────────────────────────────────────────────

void GameTest::ResetBall(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;
  auto &ballT = ctx.registry.Get<scene::Transform2D>(_state.ballEntity);
  auto &ballB = ctx.registry.Get<physics::RigidBody>(_state.ballEntity);

  ballT.x = cBallStartX;
  ballT.y = cBallStartY;
  ballT.dirty = FeTrue;
  ballB.velocity = math::Vec2D{cBallInitVx, cBallInitVy};
  ballB.teleport = FeTrue;
}

// ── lifecycle ─────────────────────────────────────────────────────────────────

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  gameInstance->windowStartWidth = 1280;
  gameInstance->windowStartHeight = 720;
  gameInstance->windowStartPosX = 100;
  gameInstance->windowStartPosY = 100;
  gameInstance->gameName = "KeepItUp";
  return FeTrue;
}

bool GameTest::GameLoad(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;

  auto sceneRes = ctx.sceneManager.Load("keepitup");
  if (!sceneRes.has_value()) {
    FLOG_ERROR("failed to create keepitup scene");
    return FeFalse;
  }
  scene::Scene *pScene = sceneRes.value();

  // ── camera ───────────────────────────────────────────────────────────────
  _state.cameraEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.cameraEntity, scene::Transform2D{});
  ctx.registry.Insert(_state.cameraEntity, scene::Camera2D{});
  pScene->roots.push_back(_state.cameraEntity);
  pScene->allEntities.push_back(_state.cameraEntity);

  // ── sprites ───────────────────────────────────────────────────────────────
  auto platRes =
      ctx.assetManager.LoadSprite("assets/textures/texture.jpg", resources::MeshShape::Quad);
  if (!platRes.has_value()) {
    FLOG_ERROR("failed to load platform sprite");
    return FeFalse;
  }
  _state.platformSprite = platRes.value();

  auto ballRes =
      ctx.assetManager.LoadSprite("assets/textures/rugtexture.jpg", resources::MeshShape::Circle);
  if (!ballRes.has_value()) {
    FLOG_ERROR("failed to load ball sprite");
    return FeFalse;
  }
  _state.ballSprite = ballRes.value();

  // ── platform (kinematic) ──────────────────────────────────────────────────
  _state.platformEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.platformEntity,
                      scene::Transform2D{0.0f, cPlatformY, 0.0f, cPlatformScaleX, cPlatformScaleY});
  ctx.registry.Insert(
      _state.platformEntity,
      physics::RigidBody{.type = physics::BodyType::Kinematic, .fixedRotation = FeTrue});
  ctx.registry.Insert(
      _state.platformEntity,
      physics::BoxCollider{.halfWidth = cPlatformHalfW, .halfHeight = cPlatformHalfH});
  ctx.registry.Insert(_state.platformEntity, _state.platformSprite);
  pScene->roots.push_back(_state.platformEntity);
  pScene->allEntities.push_back(_state.platformEntity);

  // ── ball (dynamic) ────────────────────────────────────────────────────────
  _state.ballEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.ballEntity,
                      scene::Transform2D{cBallStartX, cBallStartY, 0.0f, cBallScale, cBallScale});
  ctx.registry.Insert(_state.ballEntity,
                      physics::RigidBody{.type = physics::BodyType::Dynamic,
                                         .friction = 0.1f,
                                         .restitution = 0.7f,
                                         .fixedRotation = FeTrue,
                                         .velocity = math::Vec2D{cBallInitVx, cBallInitVy}});
  ctx.registry.Insert(_state.ballEntity, physics::CircleCollider{.radius = cBallRadius});
  ctx.registry.Insert(_state.ballEntity, _state.ballSprite);
  pScene->roots.push_back(_state.ballEntity);
  pScene->allEntities.push_back(_state.ballEntity);

  // ── static walls ─────────────────────────────────────────────────────────
  auto MakeWall = [&](float32 x, float32 y, float32 hw, float32 hh) {
    auto e = ctx.registry.Create();
    ctx.registry.Insert(e, scene::Transform2D{x, y});
    ctx.registry.Insert(e, physics::RigidBody{.type = physics::BodyType::Static});
    ctx.registry.Insert(e, physics::BoxCollider{.halfWidth = hw, .halfHeight = hh});
    pScene->allEntities.push_back(e);
  };

  // Left wall — inner edge at x = -cAspect
  MakeWall(-(cAspect + cWallThick), 0.0f, cWallThick, 1.1f);
  // Right wall — inner edge at x = +cAspect
  MakeWall((cAspect + cWallThick), 0.0f, cWallThick, 1.1f);
  // Top wall — inner edge at y = +1
  MakeWall(0.0f, 1.0f + cWallThick, cAspect + cWallThick, cWallThick);

  return FeTrue;
}

// ── update ────────────────────────────────────────────────────────────────────

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  if (!gameInstance->pCtx)
    return FeFalse;

  EngineContext &ctx = *gameInstance->pCtx;
  input::InputManager &input = ctx.inputManager;

  auto &platT = ctx.registry.Get<scene::Transform2D>(_state.platformEntity);
  auto &platB = ctx.registry.Get<physics::RigidBody>(_state.platformEntity);
  auto &ballT = ctx.registry.Get<scene::Transform2D>(_state.ballEntity);
  auto &ballB = ctx.registry.Get<physics::RigidBody>(_state.ballEntity);

  // ── platform movement ─────────────────────────────────────────────────────
  float32 vx = 0.0f;
  if (input.IsKeyDown(input::Keys::KEY_LEFT) || input.IsKeyDown(input::Keys::KEY_A))
    vx = -cPlatformSpeed;
  if (input.IsKeyDown(input::Keys::KEY_RIGHT) || input.IsKeyDown(input::Keys::KEY_D))
    vx = cPlatformSpeed;

  // Clamp: stop at screen edges
  if (platT.x - cPlatformHalfW <= -cAspect && vx < 0.0f)
    vx = 0.0f;
  if (platT.x + cPlatformHalfW >= cAspect && vx > 0.0f)
    vx = 0.0f;

  platB.velocity = math::Vec2D{vx, 0.0f};

  // ── reset ball if it falls off the bottom ─────────────────────────────────
  if (ballT.y < cDeathY) {
    FLOG_INFO("ball fell — resetting");
    ResetBall(gameInstance);
  }

  return FeTrue;
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  if (!gameInstance->pCtx)
    return;
  EngineContext &ctx = *gameInstance->pCtx;
  ctx.assetManager.ReleaseSprite(_state.platformSprite);
  ctx.assetManager.ReleaseSprite(_state.ballSprite);
  ctx.sceneManager.Unload("keepitup");
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

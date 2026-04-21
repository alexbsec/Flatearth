#include "Game.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Math/FeMath.hpp>
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Transform2D.hpp>

namespace flatearth::testbed {

// ── field dimensions (orthographic: x in [-aspect,aspect], y in [-1,1]) ─────
static constexpr float32 cAspect       = 1280.0f / 720.0f;
static constexpr float32 cPaddleX      = 1.5f;
static constexpr float32 cPaddleScaleX = 0.08f;
static constexpr float32 cPaddleScaleY = 0.35f;
static constexpr float32 cPaddleHalfW  = cPaddleScaleX * 0.5f;
static constexpr float32 cPaddleHalfH  = cPaddleScaleY * 0.5f;
static constexpr float32 cBallScale    = 0.08f;
static constexpr float32 cBallHalf     = cBallScale * 0.5f;
static constexpr float32 cWallY        = 1.0f;
static constexpr float32 cPlayerSpeed  = 2.5f;
static constexpr float32 cAISpeed      = 2.2f;
static constexpr float32 cBallInitVx   = 1.5f;
static constexpr float32 cBallInitVy   = 0.8f;
static constexpr float32 cSpeedupRate  = 1.05f;

GameState GameTest::_state = GameState{};

// ── helpers ──────────────────────────────────────────────────────────────────

static inline float32 Clamp(float32 v, float32 lo, float32 hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

static inline bool AABBOverlap(float32 ax, float32 ay, float32 ahw, float32 ahh,
                                float32 bx, float32 by, float32 bhw, float32 bhh) {
  return math::Abs(ax - bx) < ahw + bhw && math::Abs(ay - by) < ahh + bhh;
}

void GameTest::ResetBall(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;
  auto &t = ctx.registry.Get<scene::Transform2D>(_state.ballEntity);
  t.x = 0.0f;
  t.y = 0.0f;
  t.dirty = FeTrue;

  _state.serveRight = !_state.serveRight;
  _state.ballVx = _state.serveRight ? cBallInitVx : -cBallInitVx;
  _state.ballVy = cBallInitVy;
}

// ── lifecycle ─────────────────────────────────────────────────────────────────

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  gameInstance->windowStartWidth  = 1280;
  gameInstance->windowStartHeight = 720;
  gameInstance->windowStartPosX   = 100;
  gameInstance->windowStartPosY   = 100;
  gameInstance->gameName          = "Pong";
  return FeTrue;
}

bool GameTest::GameLoad(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;

  // Camera — static, centered
  _state.cameraEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.cameraEntity, scene::Transform2D{});
  ctx.registry.Insert(_state.cameraEntity, scene::Camera2D{});

  // Paddle sprite (shared by both paddles)
  auto paddleRes = ctx.assetManager.LoadSprite("assets/textures/texture.jpg",
                                               resources::MeshShape::Quad);
  if (!paddleRes.has_value()) {
    FLOG_ERROR("failed to load paddle sprite");
    return FeFalse;
  }
  _state.paddleSprite = paddleRes.value();

  // Player paddle — left side
  _state.playerEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.playerEntity,
      scene::Transform2D{-cPaddleX, 0.0f, 0.0f, cPaddleScaleX, cPaddleScaleY});
  ctx.registry.Insert(_state.playerEntity, _state.paddleSprite);

  // AI paddle — right side (same sprite, different entity)
  _state.aiEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.aiEntity,
      scene::Transform2D{cPaddleX, 0.0f, 0.0f, cPaddleScaleX, cPaddleScaleY});
  ctx.registry.Insert(_state.aiEntity, _state.paddleSprite);

  // Ball sprite
  auto ballRes = ctx.assetManager.LoadSprite("assets/textures/rugtexture.jpg",
                                             resources::MeshShape::Circle);
  if (!ballRes.has_value()) {
    FLOG_ERROR("failed to load ball sprite");
    return FeFalse;
  }
  _state.ballSprite = ballRes.value();
  _state.ballEntity = ctx.registry.Create();
  ctx.registry.Insert(_state.ballEntity,
      scene::Transform2D{0.0f, 0.0f, 0.0f, cBallScale, cBallScale});
  ctx.registry.Insert(_state.ballEntity, _state.ballSprite);

  return FeTrue;
}

// ── update ────────────────────────────────────────────────────────────────────

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  if (!gameInstance->pCtx) return FeFalse;
  EngineContext &ctx = *gameInstance->pCtx;
  input::InputManager &input = ctx.inputManager;

  auto &player = ctx.registry.Get<scene::Transform2D>(_state.playerEntity);
  auto &ai     = ctx.registry.Get<scene::Transform2D>(_state.aiEntity);
  auto &ball   = ctx.registry.Get<scene::Transform2D>(_state.ballEntity);

  // ── player input ──────────────────────────────────────────────────────────
  if (input.IsKeyDown(input::Keys::KEY_W)) {
    player.y = Clamp(player.y + cPlayerSpeed * deltaTime,
                     -(cWallY - cPaddleHalfH), cWallY - cPaddleHalfH);
    player.dirty = FeTrue;
  }
  if (input.IsKeyDown(input::Keys::KEY_S)) {
    player.y = Clamp(player.y - cPlayerSpeed * deltaTime,
                     -(cWallY - cPaddleHalfH), cWallY - cPaddleHalfH);
    player.dirty = FeTrue;
  }

  // ── AI paddle tracks ball ─────────────────────────────────────────────────
  float32 aiDiff = ball.y - ai.y;
  float32 aiMove = Clamp(aiDiff, -cAISpeed * deltaTime, cAISpeed * deltaTime);
  ai.y = Clamp(ai.y + aiMove, -(cWallY - cPaddleHalfH), cWallY - cPaddleHalfH);
  ai.dirty = FeTrue;

  // ── move ball ─────────────────────────────────────────────────────────────
  ball.x += _state.ballVx * deltaTime;
  ball.y += _state.ballVy * deltaTime;
  ball.dirty = FeTrue;

  // ── wall bounce (top / bottom) ────────────────────────────────────────────
  if (ball.y + cBallHalf >= cWallY) {
    ball.y = cWallY - cBallHalf;
    _state.ballVy = -math::Abs(_state.ballVy);
  } else if (ball.y - cBallHalf <= -cWallY) {
    ball.y = -cWallY + cBallHalf;
    _state.ballVy = math::Abs(_state.ballVy);
  }

  // ── paddle collision ──────────────────────────────────────────────────────
  auto PaddleHit = [&](scene::Transform2D &paddle) {
    if (!AABBOverlap(ball.x, ball.y, cBallHalf, cBallHalf,
                     paddle.x, paddle.y, cPaddleHalfW, cPaddleHalfH)) return;

    float32 relHit = (ball.y - paddle.y) / cPaddleHalfH;
    _state.ballVx  = -_state.ballVx * cSpeedupRate;
    _state.ballVy  = Clamp(_state.ballVy + relHit * 1.5f, -3.0f, 3.0f);

    // push ball out of paddle to prevent tunnelling
    ball.x = paddle.x + ((_state.ballVx > 0) ? 1.0f : -1.0f) * (cPaddleHalfW + cBallHalf);
  };

  if (_state.ballVx < 0) PaddleHit(player);
  else                    PaddleHit(ai);

  // ── out of bounds → reset ─────────────────────────────────────────────────
  if (ball.x > cAspect + cBallHalf || ball.x < -(cAspect + cBallHalf)) {
    FLOG_INFO("point scored — resetting ball");
    ResetBall(gameInstance);
  }

  return FeTrue;
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  if (!gameInstance->pCtx) return;
  EngineContext &ctx = *gameInstance->pCtx;
  ctx.assetManager.ReleaseSprite(_state.paddleSprite);
  ctx.assetManager.ReleaseSprite(_state.ballSprite);
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

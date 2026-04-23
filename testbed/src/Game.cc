#include "Game.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Scene/Components/ParticleEmitter.hpp>
#include <Core/FeMemory.hpp>
#include <imgui.h>
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

  auto particleTexRes = ctx.assets.manager.LoadTexture("assets/textures/Human_Walk.png");
  if (particleTexRes.has_value()) {
    auto particleSprRes = ctx.assets.manager.SpriteFromTexture(
        particleTexRes.value(), "particle_mat", resources::MeshShape::Quad);
    if (particleSprRes.has_value()) {
      scene::Sprite &ps = particleSprRes.value();

      scene::ParticleEmitter emitter{};
      emitter.texHandle   = ps.texHandle;
      emitter.matHandle   = ps.matHandle;
      emitter.meshHandle  = ps.meshHandle;
      emitter.velocityMin = {-0.3f, 0.2f};
      emitter.velocityMax = { 0.3f, 0.8f};
      emitter.lifetimeMin = 0.3f;
      emitter.lifetimeMax = 0.8f;
      emitter.sizeStart   = 0.04f;
      emitter.sizeEnd     = 0.0f;
      emitter.spawnRate   = 15.0f;

      _state.particleEntity = ctx.registry.Create();
      ctx.registry.Insert(_state.particleEntity, scene::Transform2D{});
      ctx.registry.Insert(_state.particleEntity, emitter);
      pScene->allEntities.push_back(_state.particleEntity);
    }
  }

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

  if (_state.particleEntity != UINT32_MAX) {
    auto &emitterXform = ctx.registry.Get<scene::Transform2D>(_state.particleEntity);
    emitterXform.x     = playerXform.x;
    emitterXform.y     = playerXform.y;
    emitterXform.dirty = FeTrue;
  }

  playerXform.x += dx * speed * deltaTime;
  playerXform.y += dy * speed * deltaTime;
  playerXform.dirty = FeTrue;

  cameraXform.x     = playerXform.x;
  cameraXform.y     = playerXform.y;
  cameraXform.dirty = FeTrue;

  return FeTrue;
}

// TODO: remove, just here for testing
void GameTest::GameImGui(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;

  ImGui::Begin("Debug");

  ImGui::Text("FPS: %.1f", 1.0f / ImGui::GetIO().DeltaTime);

  if (ImGui::CollapsingHeader("ECS", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Entities alive: %u", ctx.registry.AliveCount());
    if (_state.playerEntity != UINT32_MAX) {
      auto &xform = ctx.registry.Get<scene::Transform2D>(_state.playerEntity);
      ImGui::Text("Player pos: (%.2f, %.2f)", xform.x, xform.y);
    }
  }

  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto camView = ctx.registry.ViewOf<scene::Transform2D, scene::Camera2D>();
    for (auto [e, xform, cam] : camView) {
      ImGui::Text("World pos: (%.2f, %.2f)", xform.worldX, xform.worldY);
      ImGui::Text("Zoom: %.3f", cam.zoom);
      break;
    }
  }

  if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto emitterView = ctx.registry.ViewOf<scene::ParticleEmitter, scene::Transform2D>();
    int emitterIdx = 0;
    for (auto [e, emitter, xform] : emitterView) {
      uint32 activeCount = 0;
      for (uint32 i = 0; i < scene::cMaxParticles; ++i)
        activeCount += emitter.active[i] ? 1 : 0;
      ImGui::Text("Emitter %d: %u / %u active", emitterIdx, activeCount, scene::cMaxParticles);
      ImGui::Text("  pos: (%.2f, %.2f)  rate: %.1f/s", xform.worldX, xform.worldY, emitter.spawnRate);
      ++emitterIdx;
    }
    if (emitterIdx == 0) ImGui::TextDisabled("no emitters");
  }

  if (ImGui::CollapsingHeader("Memory")) {
    const auto &stats = ctx.core.memory.GetStats();
    double totalKB = stats.memoryBlock.totalAllocated / 1024.0;
    double totalMB = totalKB / 1024.0;
    ImGui::Text("Total: %.2f KB (%.2f MB)", totalKB, totalMB);
    ImGui::Text("Active allocs: %llu", static_cast<unsigned long long>(stats.allocCount));
    ImGui::Separator();
    for (uint64 i = 0; i < memory::MaxTags; ++i) {
      uint64 bytes = stats.memoryBlock.taggedAllocations[i];
      if (bytes == 0) continue;
      ImGui::Text("  [%-16s] %.2f KB", memory::TagName(static_cast<memory::Tag>(i)), bytes / 1024.0);
    }
  }

  ImGui::End();
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  if (!gameInstance->pCtx) return;
  EngineContext &ctx = *gameInstance->pCtx;

  if (_state.playerEntity != UINT32_MAX) {
    auto &sprite = ctx.registry.Get<scene::Sprite>(_state.playerEntity);
    ctx.assets.manager.ReleaseSprite(sprite);
  }

  if (_state.particleEntity != UINT32_MAX) {
    auto &emitter = ctx.registry.Get<scene::ParticleEmitter>(_state.particleEntity);
    scene::Sprite base{};
    base.texHandle  = emitter.texHandle;
    base.matHandle  = emitter.matHandle;
    base.meshHandle = emitter.meshHandle;
    ctx.assets.manager.ReleaseSprite(base);
  }

  ctx.assets.tilemap.Unload("assets/tiles/LevelEntrance.tmx");
  ctx.sceneManager.Unload("level1");
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

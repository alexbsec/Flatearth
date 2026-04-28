#include "PlayerSystem.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Scene/Components/ParticleEmitter.hpp>
#include <Scene/Components/Sprite.hpp>
#include <Scene/Components/SpriteAnimator.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>
#include "../Components/Tags.hpp"

namespace flatearth::testbed {

static constexpr float32 kUVW = 32.0f / 128.0f;
static constexpr float32 kUVH = 32.0f / 128.0f;

PlayerSystem::PlayerSystem(EngineContext &ctx, string sceneName)
    : _ctx(ctx), _sceneName(std::move(sceneName)) {}

void PlayerSystem::Initialize(ecs::Registry &) {
  auto kvarRes =
      _ctx.core.KVarsRegistry().Register("player.speed", "Player movement speed (units/s)", 1.5f);
  if (!kvarRes.has_value()) {
    FLOG_ERROR("failed to register kvar for player speed");
  }

  _ctx.assets.Prefab().Register<PlayerPrefab>([&](ecs::EntityId id, ecs::Registry &reg) {
    auto walkTexRes = _ctx.assets.Manager().LoadTexture("assets/textures/Human_Walk.png");
    if (!walkTexRes.has_value())
      return;

    auto playerSpriteRes = _ctx.assets.Manager().SpriteFromTexture(
        walkTexRes.value(), "player_walk", resources::MeshShape::Quad);
    if (!playerSpriteRes.has_value())
      return;

    constexpr float32 uvW = 32.0f / 128.0f;
    constexpr float32 uvH = 32.0f / 128.0f;

    scene::SpriteAnimator animator{};
    animator.frameCount = 4;
    animator.loop = FeTrue;
    animator.playing = FeTrue;
    for (uint8 col = 0; col < 4; ++col) {
      animator.frames[col] = {
          .uvOffset = {col * uvW, 1.0f * uvH},
          .uvScale = {uvW, -uvH},
          .duration = 0.2f,
      };
    }

    scene::Sprite playerSprite = playerSpriteRes.value();
    playerSprite.uvOffset = animator.frames[0].uvOffset;
    playerSprite.uvScale = animator.frames[0].uvScale;
    playerSprite.layer = renderer::RenderLayer::Entities;

    reg.Insert(id, scene::Transform2D{0.0f, 0.0f});
    reg.Insert(id, playerSprite);
    reg.Insert(id, animator);
    reg.Insert(id, PlayerTag{});
    reg.Insert(id, PlayerMovementState{});
    // SceneOwnership already inserted by Spawn()
  });

  auto playerRes = _ctx.assets.Prefab().Spawn<PlayerPrefab>(
      _ctx.project.Registry(), scene::SceneOwnership{_sceneName});
  if (!playerRes.has_value()) {
    FLOG_ERROR("failed to spawn player: {}", playerRes.error().message);
    return;
  }

  auto particleTexRes = _ctx.assets.Manager().LoadTexture("assets/textures/Human_Walk.png");
  if (particleTexRes.has_value()) {
    auto particleSprRes = _ctx.assets.Manager().SpriteFromTexture(
        particleTexRes.value(), "particle_mat", resources::MeshShape::Quad);
    if (particleSprRes.has_value()) {
      scene::Sprite &ps = particleSprRes.value();

      scene::ParticleEmitter emitter{};
      emitter.texHandle = ps.texHandle;
      emitter.matHandle = ps.matHandle;
      emitter.meshHandle = ps.meshHandle;
      emitter.velocityMin = {-0.3f, 0.2f};
      emitter.velocityMax = {0.3f, 0.8f};
      emitter.lifetimeMin = 0.3f;
      emitter.lifetimeMax = 0.8f;
      emitter.sizeStart = 0.04f;
      emitter.sizeEnd = 0.0f;
      emitter.spawnRate = 15.0f;

      ecs::EntityId particleId = _ctx.project.Registry().Create();
      _ctx.project.Registry().Insert(particleId, scene::Transform2D{});
      _ctx.project.Registry().Insert(particleId, emitter);
      _ctx.project.Registry().Insert(particleId, ParticleTag{});
      _ctx.project.Registry().Insert(particleId, scene::SceneOwnership{_sceneName});
    }
  }
}

void PlayerSystem::Update(ecs::Registry &, float32 deltaTime) {
  auto &reg   = _ctx.project.Registry();
  auto &input = _ctx.core.Input();
  auto &kvars = _ctx.core.KVarsRegistry();
  float32 speed = kvars.Get<float32>("player.speed").value_or(1.5f);

  for (auto [id, ptag, xform, sprite, anim, mv] :
       reg.ViewOf<PlayerTag, scene::Transform2D, scene::Sprite,
                  scene::SpriteAnimator, PlayerMovementState>()) {
    float32 dx = 0.0f, dy = 0.0f;

    if (input.IsKeyDown(input::KEY_W) || input.IsKeyDown(input::KEY_UP))
      dy += 1.0f;
    if (input.IsKeyDown(input::KEY_S) || input.IsKeyDown(input::KEY_DOWN))
      dy -= 1.0f;
    if (input.IsKeyDown(input::KEY_A) || input.IsKeyDown(input::KEY_LEFT))
      dx -= 1.0f;
    if (input.IsKeyDown(input::KEY_D) || input.IsKeyDown(input::KEY_RIGHT))
      dx += 1.0f;

    bool moving = (dx != 0.0f || dy != 0.0f);

    if (dy > 0.0f) mv.vertRow = 3;
    if (dy < 0.0f) mv.vertRow = 0;
    if (dx < 0.0f) mv.horzRow = 1;
    if (dx > 0.0f) mv.horzRow = 2;

    uint8 dirRow;
    if (dx != 0.0f && dy != 0.0f) {
      if (dx < 0.0f && dy > 0.0f)      dirRow = 3;
      else if (dx < 0.0f && dy < 0.0f) dirRow = 1;
      else if (dx > 0.0f && dy < 0.0f) dirRow = 2;
      else                              dirRow = 2;
    } else if (dx != 0.0f) {
      dirRow = mv.vertRow;
    } else if (dy != 0.0f) {
      dirRow = mv.horzRow;
    } else {
      dirRow = mv.lastDirRow;
    }

    if (moving)
      mv.lastDirRow = dirRow;

    for (uint8 col = 0; col < 4; ++col) {
      anim.frames[col].uvOffset = {col * kUVW, (dirRow + 1) * kUVH};
      anim.frames[col].uvScale  = {kUVW, -kUVH};
    }

    anim.playing = moving;
    if (!moving) {
      anim.currentFrame = 0;
      anim.elapsed = 0.0f;
    }
    sprite.uvOffset = anim.frames[anim.currentFrame].uvOffset;
    sprite.uvScale  = anim.frames[anim.currentFrame].uvScale;
    sprite.dirty = FeTrue;

    xform.x += dx * speed * deltaTime;
    xform.y += dy * speed * deltaTime;
    xform.dirty = FeTrue;

    for (auto [pid, pttag, emitter, pt] :
         reg.ViewOf<ParticleTag, scene::ParticleEmitter, scene::Transform2D>()) {
      emitter.spawnRate = kvars.Get<float32>("particle.spawnRate").value_or(15.0f);
      pt.x = xform.x;
      pt.y = xform.y;
      pt.dirty = FeTrue;
    }
  }
}

} // namespace flatearth::testbed

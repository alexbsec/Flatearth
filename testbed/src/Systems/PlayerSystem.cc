#include "PlayerSystem.hpp"

#include "../Components/Tags.hpp"

#include <Core/EngineContext.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Scene/Components/ParticleEmitter.hpp>
#include <Scene/Components/Sprite.hpp>
#include <Scene/Components/SpriteAnimator.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>
#include <execution>

namespace flatearth::testbed {

static constexpr float32 kUVW = 32.0f / 128.0f;
static constexpr float32 kUVH = 32.0f / 128.0f;

PlayerSystem::PlayerSystem(EngineContext &ctx, scene::SceneId sceneId)
    : _ctx(ctx), _sceneId(sceneId) {
}

void PlayerSystem::Initialize(ecs::Registry &) {
  _ctx.core.KVarsRegistry()
      .Register("player.speed", "Player movement speed (units/s)", 1.5f)
      .or_log_error("failed to register kvar for player speed");

  // Register one walk clip per direction row (rows 1-4 in the spritesheet)
  for (uint8 row = 0; row < 4; ++row) {
    auto clipRes =
        _ctx.assets.Animations()
            .NewClip(std::format("player_walk_{}", row), "assets/textures/Human_Walk.png")
            .or_error("failed to create NewClip");
    if (clipRes.errored()) {
      return;
    }

    scene::AnimationClip clip = clipRes.value();
    clip.frameCount = 4;
    clip.loop = FeTrue;
    for (uint8 col = 0; col < 4; ++col) {
      clip.frames[col] = {
          .uvOffset = {col * kUVW, (row + 1) * kUVH},
          .uvScale = {kUVW, -kUVH},
          .duration = 0.2f,
      };
    }
    _ctx.assets.Animations().RegisterClip(clip);
  }

  _ctx.assets.Prefab().Register<PlayerPrefab>([&](ecs::EntityBuilder &b) {
    auto walkTexRes = _ctx.assets.Manager().LoadTexture("assets/textures/Human_Walk.png");
    if (walkTexRes.errored())
      return;

    auto playerSpriteRes = _ctx.assets.Manager().SpriteFromTexture(
        walkTexRes.value(), "player_walk", resources::MeshShape::Quad);
    if (playerSpriteRes.errored())
      return;

    scene::SpriteAnimator animator{};
    animator.currentClip.Set("player_walk_1").or_log_error("failed to set initial clip");
    animator.playing = FeTrue;

    const scene::AnimationClip *clip = _ctx.assets.Animations().Get("player_walk_1");
    scene::Sprite playerSprite = playerSpriteRes.value();
    if (clip && clip->frameCount > 0) {
      playerSprite.uvOffset = clip->frames[0].uvOffset;
      playerSprite.uvScale = clip->frames[0].uvScale;
    }
    playerSprite.layer = renderer::RenderLayer::Entities;

    b.With(scene::Transform2D{0.0f, 0.0f})
        .With(playerSprite)
        .With(animator)
        .With(PlayerTag{})
        .With(PlayerMovementState{});
  });

  auto playerRes = _ctx.assets.Prefab()
                       .Spawn<PlayerPrefab>(_ctx.project.Registry(), _sceneId)
                       .or_error("failed to spawn player prefab");
  if (playerRes.errored()) {
    return;
  }

  auto particleTexRes = _ctx.assets.Manager()
                            .LoadTexture("assets/textures/Human_Walk.png")
                            .or_error("failed to load particle texture");
  if (particleTexRes.errored()) {
    return;
  }

  auto particleSprRes =
      _ctx.assets.Manager()
          .SpriteFromTexture(particleTexRes.value(), "particle_mat", resources::MeshShape::Quad)
          .or_error("failed to create particle sprite from texture");
  if (particleSprRes.errored()) {
    return;
  }

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

  _ctx.project.Registry()
      .Spawn()
      .With(scene::Transform2D{})
      .With(emitter)
      .With(ParticleTag{})
      .OwnedBy(_sceneId)
      .Commit();
}

void PlayerSystem::Update(ecs::Registry &, float32 deltaTime) {
  auto &reg = _ctx.project.Registry();
  auto &input = _ctx.core.Input();
  auto &kvars = _ctx.core.KVarsRegistry();
  float32 speed = kvars.Get<float32>("player.speed").value_or(1.5f);

  for (auto [id, ptag, xform, sprite, anim, mv] : reg.ViewOf<PlayerTag,
                                                             scene::Transform2D,
                                                             scene::Sprite,
                                                             scene::SpriteAnimator,
                                                             PlayerMovementState>()) {
    float32 dx = 0.0f, dy = 0.0f;

    if (input.IsKeyDown(input::KEY_W) || input.IsKeyDown(input::KEY_UP)) {
      dy += 1.0f;
    }
    if (input.IsKeyDown(input::KEY_S) || input.IsKeyDown(input::KEY_DOWN)) {
      dy -= 1.0f;
    }
    if (input.IsKeyDown(input::KEY_A) || input.IsKeyDown(input::KEY_LEFT)) {
      dx -= 1.0f;
    }
    if (input.IsKeyDown(input::KEY_D) || input.IsKeyDown(input::KEY_RIGHT)) {
      dx += 1.0f;
    }

    bool moving = (dx != 0.0f || dy != 0.0f);

    if (dy > 0.0f) {
      mv.vertRow = 3;
    }
    if (dy < 0.0f) {
      mv.vertRow = 0;
    }
    if (dx < 0.0f) {
      mv.horzRow = 1;
    }
    if (dx > 0.0f) {
      mv.horzRow = 2;
    }

    uint8 dirRow;
    if (dx != 0.0f && dy != 0.0f) {
      if (dx < 0.0f && dy > 0.0f) {
        dirRow = 3;
      } else if (dx < 0.0f && dy < 0.0f) {
        dirRow = 1;
      } else if (dx > 0.0f && dy < 0.0f) {
        dirRow = 2;
      } else {
        dirRow = 2;
      }
    } else if (dx != 0.0f) {
      dirRow = mv.vertRow;
    } else if (dy != 0.0f) {
      dirRow = mv.horzRow;
    } else {
      dirRow = mv.lastDirRow;
    }

    if (moving) {
      mv.lastDirRow = dirRow;
    }

    anim.currentClip.Set(std::format("player_walk_{}", dirRow))
        .or_log_error("failed to set walk clip");
    anim.playing = moving;

    if (!moving) {
      anim.currentFrame = 0;
      anim.elapsed = 0.0f;
      const scene::AnimationClip *clip = _ctx.assets.Animations().Get(anim.currentClip.View());
      if (clip && clip->frameCount > 0) {
        sprite.uvOffset = clip->frames[0].uvOffset;
        sprite.uvScale = clip->frames[0].uvScale;
        sprite.dirty = FeTrue;
      }
    }

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

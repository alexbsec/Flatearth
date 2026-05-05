#include "SpriteSystem.hpp"

#include "Math/FeMath.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::scene::systems {
using namespace scene;

SpriteSystem::SpriteSystem(assets::AnimationRegistry &registry) : _animRegistry(registry) {
}

void SpriteSystem::Update(ecs::Registry &registry, float32 deltaTime) {
  ecs::View<Sprite, SpriteAnimator> animView = registry.ViewOf<Sprite, SpriteAnimator>();
  for (auto &&[entity, sprite, animator] : animView) {
    if (!animator.playing || animator.currentClip.View().empty()) {
      continue;
    }
    UpdateAnimator(animator, sprite, deltaTime);
  }

  ecs::View<Sprite> spriteView = registry.ViewOf<Sprite>();
  for (auto &&[entity, sprite] : spriteView) {
    if (!sprite.dirty) {
      continue;
    }
    UpdateSprite(sprite);
  }
}

void SpriteSystem::UpdateAnimator(SpriteAnimator &animator, Sprite &sprite, float32 deltaTime) {
  const AnimationClip *clip = _animRegistry.Get(animator.currentClip.View());
  if (!clip || clip->frameCount == 0) {
    return;
  }

  animator.elapsed += deltaTime;
  if (animator.elapsed >= clip->frames[animator.currentFrame].duration) {
    animator.elapsed = 0.0f;
    if (++animator.currentFrame >= clip->frameCount) {
      animator.currentFrame = clip->loop ? 0 : clip->frameCount - 1;
      if (!clip->loop) {
        animator.playing = FeFalse;
      }
    }
  }

  const AnimationFrame &frame = clip->frames[animator.currentFrame];
  sprite.uvOffset = frame.uvOffset;
  sprite.uvScale = frame.uvScale;
  sprite.dirty = FeTrue;
}

void SpriteSystem::UpdateSprite(Sprite &sprite) {
  if (sprite.flipX) {
    sprite.uvScale = math::Vec2D{-math::Abs32(sprite.uvScale.x()), sprite.uvScale.y()};
  } else {
    sprite.uvScale = math::Vec2D{math::Abs32(sprite.uvScale.x()), sprite.uvScale.y()};
  }
  if (sprite.flipY) {
    sprite.uvScale = math::Vec2D{sprite.uvScale.x(), -math::Abs32(sprite.uvScale.y())};
  } else {
    sprite.uvScale = math::Vec2D{sprite.uvScale.x(), math::Abs32(sprite.uvScale.y())};
  }
  sprite.dirty = FeFalse;
}

} // namespace flatearth::scene::systems

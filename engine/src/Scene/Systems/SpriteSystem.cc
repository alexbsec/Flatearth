#include "SpriteSystem.hpp"

#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::systems {
using namespace scene;

void SpriteSystem::Update(ecs::Registry &registry, float32 deltaTime) {
  ecs::View<Sprite, SpriteAnimator> animView = registry.ViewOf<Sprite, SpriteAnimator>();
  for (auto &&[entity, sprite, animator] : animView) {
    if (!animator.playing || animator.frameCount == 0)
      continue;
    UpdateAnimator(animator, sprite, deltaTime);
  }

  ecs::View<Sprite> spriteView = registry.ViewOf<Sprite>();
  for (auto &&[entity, sprite] : spriteView) {
    if (!sprite.dirty)
      continue;
    UpdateSprite(sprite);
  }
}

void SpriteSystem::UpdateAnimator(SpriteAnimator &animator, Sprite &sprite, float32 deltaTime) {
  animator.elapsed += deltaTime;
  if (animator.elapsed >= animator.frames[animator.currentFrame].duration) {
    animator.elapsed = 0.0f;
    animator.currentFrame++;

    if (animator.currentFrame >= animator.frameCount) {
      animator.currentFrame = animator.loop ? 0 : animator.frameCount - 1;
      if (!animator.loop) {
        animator.playing = FeFalse;
      }
    }

    const AnimationFrame &frame = animator.frames[animator.currentFrame];
    sprite.uvOffset = frame.uvOffset;
    sprite.uvScale = frame.uvScale;
    sprite.dirty = FeTrue;
  }
}

void SpriteSystem::UpdateSprite(Sprite &sprite) {
  if (sprite.flipX) {
    sprite.uvScale = sprite.uvScale * math::Vec2D{-1, 1};
  }
  if (sprite.flipY) {
    sprite.uvScale = sprite.uvScale * math::Vec2D{1, -1};
  }
}

} // namespace flatearth::systems

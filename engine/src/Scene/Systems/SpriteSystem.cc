#include "SpriteSystem.hpp"

#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::systems {
using namespace scene;

void SpriteSystem::Update(ecs::Registry &registry, float32 deltaTime) {
  ecs::View<Sprite, SpriteAnimator> view = registry.ViewOf<Sprite, SpriteAnimator>();

  for (auto &&[entity, sprite, animator] : view) {
    if (!animator.playing || animator.frameCount == 0) {
      continue;
    }

    UpdateAnimator(animator, sprite, deltaTime);
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

} // namespace flatearth::systems

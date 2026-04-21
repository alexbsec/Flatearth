#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_ANIMATOR_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_ANIMATOR_HPP

#include "Math/Vector2D.hpp"
namespace flatearth::scene {

constexpr uint32 cMaxAnimationFrames = 16;

struct AnimationFrame {
  math::Vec2D uvOffset{0.0f, 0.0f};
  math::Vec2D uvScale{1.0f, 1.0f};
  float32 duration{0.1f};
};

struct SpriteAnimator {
  AnimationFrame frames[cMaxAnimationFrames]{};
  uint8 frameCount{0};
  uint8 currentFrame{0};
  float32 elapsed{0.0f};
  bool loop{FeTrue};
  bool playing{FeFalse};
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_ANIMATOR_HPP

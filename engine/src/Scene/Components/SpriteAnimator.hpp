#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_ANIMATOR_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_ANIMATOR_HPP

#include "Math/Vector2D.hpp"
#include "Core/FeString.hpp"

namespace flatearth::scene {

constexpr uint32 cMaxAnimationFrames = 16;
constexpr uint32 cMaxClipNameLength = 256;
constexpr uint32 cMaxSpritesheetPathLength = 256;

struct AnimationFrame {
  math::Vec2D uvOffset{0.0f, 0.0f};
  math::Vec2D uvScale{1.0f, 1.0f};
  float32 duration{0.1f};
};

struct AnimationClip {
  FeString<cMaxClipNameLength> name{};
  FeString<cMaxSpritesheetPathLength> spritesheetPath{};
  AnimationFrame frames[cMaxAnimationFrames]{};
  uint8 frameCount{0};
  bool loop{FeFalse};
};

struct SpriteAnimator {
  FeString<cMaxClipNameLength> currentClip{};
  uint8 currentFrame{0};
  float32 elapsed{0.0f};
  bool playing{FeFalse};
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_ANIMATOR_HPP

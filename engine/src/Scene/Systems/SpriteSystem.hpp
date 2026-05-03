#ifndef _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP

#include "Assets/AnimationRegistry.hpp"
#include "ECS/SystemInterface.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::scene::systems {

class SpriteSystem : public ecs::ISystem {
public:
  explicit SpriteSystem(assets::AnimationRegistry &registry);
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  void UpdateAnimator(scene::SpriteAnimator &animator, scene::Sprite &sprite, float32 deltaTime);
  void UpdateSprite(scene::Sprite &sprite);

private:
  assets::AnimationRegistry &_animRegistry;
};

} // namespace flatearth::scene::systems

#endif // _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP

#ifndef _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP

#include "ECS/SystemInterface.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::systems {

class SpriteSystem : public ecs::ISystem {
public:
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  void UpdateAnimator(scene::SpriteAnimator &animator, scene::Sprite &sprite, float32 deltaTime);
};

} // namespace flatearth::systems

#endif // _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP

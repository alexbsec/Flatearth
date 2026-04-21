#ifndef _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP

#include "ECS/SystemInterface.hpp"

namespace flatearth::systems {

class SpriteSystem : public ecs::ISystem {
public:
  void Update(ecs::Registry &registry, float32 deltaTime) override;
};

} // namespace flatearth::systems

#endif // _FLATEARTH_ENGINE_SCENE_SYSTEMS_SPRITE_SYSTEM_HPP

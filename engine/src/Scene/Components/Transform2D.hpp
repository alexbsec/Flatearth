#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_TRANSFORM_2D_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_TRANSFORM_2D_HPP

#include "Defines.hpp"
#include "ECS/ECSTypes.hpp"

namespace flatearth::scene {

constexpr ecs::EntityId cNullEntity = UINT32_MAX;

struct Transform2D {
  float32 x{0.0f};
  float32 y{0.0f};
  float32 rotation{0.0f};
  float32 scaleX{1.0f};
  float32 scaleY{1.0f};

  // Runtime, not reflected
  ecs::EntityId parent{cNullEntity};
  bool dirty{FeTrue};
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_TRANSFORM_2D_HPP

#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_CHILDREN_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_CHILDREN_HPP

#include "Defines.hpp"
#include "ECS/ECSTypes.hpp"
namespace flatearth::scene {

struct Children {
  static constexpr uint8 scMaxChildren = 8;
  ecs::EntityId ids[scMaxChildren]{};
  uint8 count{0};

  bool Add(ecs::EntityId id);
  bool Remove(ecs::EntityId id);
  bool Has(ecs::EntityId id) const;
};

}

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_CHILDREN_HPP

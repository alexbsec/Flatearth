#ifndef _FLATEARTH_ENGINE_SCENE_SCENE_HPP
#define _FLATEARTH_ENGINE_SCENE_SCENE_HPP

#include "Defines.hpp"
#include "ECS/ECSTypes.hpp"

#include <vector>

namespace flatearth::scene {

struct Scene {
  string name;
  string sourcePath;
  std::vector<ecs::EntityId> roots;
  std::vector<ecs::EntityId> allEntities;
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_SCENE_HPP

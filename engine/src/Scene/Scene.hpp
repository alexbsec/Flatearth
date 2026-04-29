#ifndef _FLATEARTH_ENGINE_SCENE_SCENE_HPP
#define _FLATEARTH_ENGINE_SCENE_SCENE_HPP

#include "Defines.hpp"

namespace flatearth::scene {

using SceneId = uint16;
constexpr SceneId cNullScene = 0;

struct SceneOwnership {
  SceneId sceneId{cNullScene};

  SceneOwnership() = default;
  explicit SceneOwnership(SceneId id) : sceneId(id) {}
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_SCENE_HPP

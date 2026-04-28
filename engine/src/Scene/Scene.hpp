#ifndef _FLATEARTH_ENGINE_SCENE_SCENE_HPP
#define _FLATEARTH_ENGINE_SCENE_SCENE_HPP

#include "Defines.hpp"

#include <cstring>

namespace flatearth::scene {

constexpr uint32 cSceneNameMax = 64;

struct SceneOwnership {
  char sceneName[cSceneNameMax]{};

  SceneOwnership() = default;
  explicit SceneOwnership(stringv name) {
    std::strncpy(sceneName, name.data(), cSceneNameMax - 1);
  }
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_SCENE_HPP

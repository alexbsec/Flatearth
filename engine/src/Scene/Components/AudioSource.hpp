#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_AUDIO_SOURCE_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_AUDIO_SOURCE_HPP

#include "Defines.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::scene {

constexpr uint32 cAudioPathMax = 256;

struct AudioSource {
  char path[cAudioPathMax]{};
  char loadedPath[cAudioPathMax]{};
  resources::SoundHandle handle{resources::cInvalidSoundHandle};
  bool loop{FeFalse};
  bool playing{FeFalse};
  float32 volume{1.0f};
  bool dirty{FeTrue};
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_AUDIO_SOURCE_HPP

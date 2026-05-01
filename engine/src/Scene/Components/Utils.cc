#include "Utils.hpp"

#include "Core/Logger.hpp"
#include "Scene/Components/AudioSource.hpp"

#include <cstring>

namespace flatearth::scene {

void SetAudioPath(AudioSource &source, const string &path) {
  if (path.length() >= scene::cAudioPathMax) {
    FLOG_ERROR("audio path length cannot exceed '{}' characters. '{}' got {} characters",
               scene::cAudioPathMax,
               path,
               path.length());
    return;
  }

  std::strncpy(source.path, path.c_str(), scene::cAudioPathMax - 1);
}


} // namespace flatearth::scene

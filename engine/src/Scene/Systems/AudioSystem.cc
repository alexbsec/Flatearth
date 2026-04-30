#include "AudioSystem.hpp"

#include "Audio/AudioManager.hpp"
#include "Core/Logger.hpp"
#include "Scene/Components/AudioSource.hpp"

#include <cstring>

namespace flatearth::systems {

using namespace ecs;
using scene::AudioSource;

AudioSystem::AudioSystem(audio::AudioManager &am) : _audio(am) {
}

void AudioSystem::Update(Registry &registry, float32 delatTime) {
  View<AudioSource> view = registry.ViewOf<AudioSource>();

  for (auto &&[entity, audioSrc] : view) {
    bool hasHandle = audioSrc.handle != audio::cInvalidSound;

    const bool pathChanged = hasHandle && std::strcmp(audioSrc.loadedPath, audioSrc.path) != 0;
    const bool hasPath = audioSrc.path[0] != '\0';
    const bool isValidSource = hasPath || pathChanged;

    if (!isValidSource) {
      FLOG_WARN("invalid AudioSource: no path and handle was defined for this source");
      continue;
    }

    if (pathChanged) {
      _audio.Stop(audioSrc.handle);
      _audio.ReleaseSound(audioSrc.handle);
      audioSrc.handle = audio::cInvalidSound;
      continue; // will re-acquire next frame via first if
    }

    if (!hasHandle) {
      auto res = _audio.AcquireSound(audioSrc.path);
      if (res.errored()) {
        FLOG_ERROR("failed to acquire sound: {}", audioSrc.path);
        continue;
      }
      audioSrc.handle = res.value();
      std::strncpy(audioSrc.loadedPath, audioSrc.path, scene::cAudioPathMax - 1);

      audioSrc.dirty = FeTrue;
      hasHandle = FeTrue;
    }

    if (!audioSrc.dirty) {
      continue;
    }

    _audio.SetVolume(audioSrc.handle, audioSrc.volume);
    if (audioSrc.playing) {
      _audio.Play(audioSrc.handle, audioSrc.loop);
    } else {
      _audio.Stop(audioSrc.handle);
    }

    audioSrc.dirty = FeFalse;
  }
}

} // namespace flatearth::systems

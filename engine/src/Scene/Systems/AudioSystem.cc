#include "AudioSystem.hpp"

#include <cstring>

#include "Audio/AudioManager.hpp"
#include "Core/Logger.hpp"
#include "Scene/Components/AudioSource.hpp"

namespace flatearth::systems {

using namespace ecs;
using scene::AudioSource;

AudioSystem::AudioSystem(audio::AudioManager &am) : _audio(am) {
}

void AudioSystem::Update(Registry &registry, float32 delatTime) {
  View<AudioSource> view = registry.ViewOf<AudioSource>();

  for (auto &&[entity, audioSrc] : view) {
    if (audioSrc.handle == audio::cInvalidSound && audioSrc.path[0] != '\0') {
      auto res = _audio.AcquireSound(audioSrc.path);
      if (!res.has_value()) {
        FLOG_ERROR("failed to acquire sound: {}", audioSrc.path);
        continue;
      }
      audioSrc.handle = res.value();
      std::strncpy(audioSrc.loadedPath, audioSrc.path, scene::cAudioPathMax - 1);
      audioSrc.dirty = FeTrue;
    }

    if (audioSrc.handle != audio::cInvalidSound && std::strcmp(audioSrc.loadedPath, audioSrc.path) != 0) {
      _audio.Stop(audioSrc.handle);
      _audio.ReleaseSound(audioSrc.handle);
      audioSrc.handle = audio::cInvalidSound;
      continue; // will re-acquire next frame via first if
    }

    if (audioSrc.dirty && audioSrc.handle != audio::cInvalidSound) {
      _audio.SetVolume(audioSrc.handle, audioSrc.volume);
      if (audioSrc.playing) {
        _audio.Play(audioSrc.handle, audioSrc.loop);
      } else {
        _audio.Stop(audioSrc.handle);
      }

      audioSrc.dirty = FeFalse;
    }
  }
}

} // namespace flatearth::systems

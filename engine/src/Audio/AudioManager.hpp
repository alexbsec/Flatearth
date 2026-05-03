#ifndef _FLATEARTH_ENGINE_AUDIO_AUDIO_MANAGER_HPP
#define _FLATEARTH_ENGINE_AUDIO_AUDIO_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include "Resources/AudioCache.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Vendor/miniaudio.h"

namespace flatearth::audio {

constexpr resources::SoundHandle cInvalidSound = resources::cInvalidSoundHandle;

class AudioManager {
public:
  explicit AudioManager(memory::MemoryManager &);

  FeExpect<void, Error> Initialize();
  void Shutdown();

  FeExpect<resources::SoundHandle, Error> AcquireSound(const string &path);
  void ReleaseSound(resources::SoundHandle handle);

  void Play(resources::SoundHandle handle, bool loop = FeFalse);
  void Pause(resources::SoundHandle handle);
  void Stop(resources::SoundHandle handle);

  void SetVolume(resources::SoundHandle handle, float32 volume);
  void SetMasterVolume(float32 volume);

private:
  memory::MemoryManager &_memoryManager;
  ma_engine _miniaudio{};
  resources::AudioCache _soundCache;
  bool _initialized{FeFalse};
};

} // namespace flatearth::audio

#endif // _FLATEARTH_ENGINE_AUDIO_AUDIO_MANAGER_HPP

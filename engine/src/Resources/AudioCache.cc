#include "AudioCache.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Vendor/miniaudio.h"

namespace flatearth::resources {

AudioCache::AudioCache(memory::MemoryManager &memManager, ma_engine &miniaudio)
    : ResourceCache(memManager), _miniaudio(miniaudio) {
}

FeExpect<SoundHandle, Error> AudioCache::AcquireSound(const string &name) {
  return this->ProtectedAcquire(name);
}

FeExpect<void, Error> AudioCache::Create(Audio *pAudio, uint32, const string &name) {
  if (pAudio == nullptr) {
    return FeErr{Error("cannot create on nullptr Audio", ErrorType::NullptrException)};
  }

  pAudio->name = name;
  pAudio->pSound = static_cast<ma_sound *>(
      _memoryManager.RawAlloc(sizeof(ma_sound), alignof(ma_sound), memory::Tag::Audio));

  if (pAudio->pSound == nullptr) {
    return FeErr{Error("failed to allocate ma_sound", ErrorType::NullptrException)};
  }

  if (ma_sound_init_from_file(&_miniaudio, name.c_str(), 0, nullptr, nullptr, pAudio->pSound) !=
      MA_SUCCESS) {
    _memoryManager.RawFree(pAudio->pSound, sizeof(ma_sound), memory::Tag::Audio);
    pAudio->pSound = nullptr;
    FLOG_ERROR("failed to load sound: {}", name);
    return FeErr{Error("failed to load sound file", ErrorType::SystemError)};
  }

  return {};
}

FeExpect<void, Error> AudioCache::Destroy(Audio *pAudio) {
  if (pAudio == nullptr || pAudio->pSound == nullptr) {
    return FeErr{Error("cannot destroy nullptr Audio", ErrorType::NullptrException)};
  }

  ma_sound_uninit(pAudio->pSound);
  _memoryManager.RawFree(pAudio->pSound, sizeof(ma_sound), memory::Tag::Audio);
  pAudio->pSound = nullptr;
  return {};
}

} // namespace flatearth::resources

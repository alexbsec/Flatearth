#include "AudioManager.hpp"

#include "Core/Logger.hpp"

namespace flatearth::audio {

AudioManager::AudioManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager), _soundCache(memManager, _miniaudio) {}

FeExpect<void, Error> AudioManager::Initialize() {
  if (ma_engine_init(nullptr, &_miniaudio) != MA_SUCCESS) {
    FLOG_ERROR("failed to initialize miniaudio engine");
    return FeErr{Error("ma_engine_init failed", ErrorType::SystemError)};
  }
  _initialized = FeTrue;
  return {};
}

void AudioManager::Shutdown() {
  if (!_initialized) return;
  _soundCache.Shutdown();
  ma_engine_uninit(&_miniaudio);
  _initialized = FeFalse;
}

FeExpect<resources::SoundHandle, Error> AudioManager::AcquireSound(const string &path) {
  return _soundCache.AcquireSound(path);
}

void AudioManager::ReleaseSound(resources::SoundHandle handle) {
  _soundCache.Release(handle);
}

void AudioManager::Play(resources::SoundHandle handle, bool loop) {
  resources::Audio *pAudio = _soundCache.Get(handle);
  if (pAudio == nullptr || pAudio->pSound == nullptr) return;
  ma_sound_set_looping(pAudio->pSound, loop ? MA_TRUE : MA_FALSE);
  ma_sound_start(pAudio->pSound);
}

void AudioManager::Pause(resources::SoundHandle handle) {
  resources::Audio *pAudio = _soundCache.Get(handle);
  if (pAudio == nullptr || pAudio->pSound == nullptr) return;
  ma_sound_stop(pAudio->pSound);
}

void AudioManager::Stop(resources::SoundHandle handle) {
  resources::Audio *pAudio = _soundCache.Get(handle);
  if (pAudio == nullptr || pAudio->pSound == nullptr) return;
  ma_sound_stop(pAudio->pSound);
  ma_sound_seek_to_pcm_frame(pAudio->pSound, 0);
}

void AudioManager::SetVolume(resources::SoundHandle handle, float32 volume) {
  resources::Audio *pAudio = _soundCache.Get(handle);
  if (pAudio == nullptr || pAudio->pSound == nullptr) return;
  ma_sound_set_volume(pAudio->pSound, volume);
}

void AudioManager::SetMasterVolume(float32 volume) {
  ma_engine_set_volume(&_miniaudio, volume);
}

} // namespace flatearth::audio

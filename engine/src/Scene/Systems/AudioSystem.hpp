#ifndef _FLATEARTH_ENGINE_SCENE_SYSTEMS_AUDIO_SYSTEM_HPP
#define _FLATEARTH_ENGINE_SCENE_SYSTEMS_AUDIO_SYSTEM_HPP

#include "Audio/AudioManager.hpp"
#include "ECS/Registry.hpp"
#include "ECS/SystemInterface.hpp"

namespace flatearth::scene::systems {

class AudioSystem : public ecs::ISystem {
public:
  explicit AudioSystem(audio::AudioManager &am);
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  audio::AudioManager &_audio;
};

} // namespace flatearth::scene::systems

#endif // _FLATEARTH_ENGINE_SCENE_SYSTEMS_AUDIO_SYSTEM_HPP

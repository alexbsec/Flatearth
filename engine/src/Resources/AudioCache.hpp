#ifndef _FLATEARTH_ENGINE_RESOURCES_AUDIO_CACHE_HPP
#define _FLATEARTH_ENGINE_RESOURCES_AUDIO_CACHE_HPP

#include "Resources/ResourceCache.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Vendor/miniaudio.h"

namespace flatearth::resources {

class AudioCache : public ResourceCache<Audio> {
public:
  explicit AudioCache(memory::MemoryManager &, ma_engine &);

  FeExpect<SoundHandle, Error> AcquireSound(const string &name);

protected:
  FeExpect<void, Error> Create(Audio *, uint32 handle, const string &name) override;
  FeExpect<void, Error> Destroy(Audio *) override;

private:
  ma_engine &_miniaudio;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_AUDIO_CACHE_HPP

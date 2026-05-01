#ifndef _FLATEARTH_ENGINE_ASSETS_ANIMATION_REGISTRY_HPP
#define _FLATEARTH_ENGINE_ASSETS_ANIMATION_REGISTRY_HPP

#include "Assets/AssetManager.hpp"
#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::assets {

class AnimationRegistry {
public:
  explicit AnimationRegistry(memory::MemoryManager &, AssetManager &);

  FEAPI FeExpect<void, Error> AddClip(stringv name, const scene::SheetClip &);
  FEAPI void UnregisterClip(stringv name);
  FEAPI void UnregisterAll(stringv spritesheetPath);
  FEAPI void Clear();

  FEAPI const scene::AnimationClip *Get(stringv name) const;
  FEAPI bool Has(stringv name) const;

private:
  FeExpect<scene::AnimationClip, Error> NewClip(stringv name, stringv spritesheetPath);
  void RegisterClip(scene::AnimationClip clip);

private:
  containers::HashMap<string, scene::AnimationClip> _clipsMap;
  AssetManager &_assetManager;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_ANIMATION_REGISTRY_HPP

#include "AnimationRegistry.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"

namespace flatearth::assets {

AnimationRegistry::AnimationRegistry(memory::MemoryManager &mm) : _clipsMap(mm) {
}

FeExpect<scene::AnimationClip, Error> AnimationRegistry::NewClip(stringv name,
                                                                 stringv spritesheetPath) {
  scene::AnimationClip out{};
  auto setRes = out.name.Set(name).or_error("failed to set name to clip");
  if (setRes.errored()) {
    return FeErr{setRes.error()};
  }

  setRes = out.spritesheetPath.Set(spritesheetPath);
  if (setRes.errored()) {
    return FeErr{setRes.error()};
  }

  return std::move(out);
}

void AnimationRegistry::RegisterClip(scene::AnimationClip clip) {
  if (_clipsMap.Has(clip.name.String())) {
    return;
  }

  _clipsMap.Insert(clip.name.String(), clip);
}

void AnimationRegistry::UnregisterClip(stringv name) {
  const string nameStr = string(name);
  if (!_clipsMap.Has(nameStr)) {
    return;
  }

  if (!_clipsMap.Erase(nameStr)) {
    FLOG_WARN("failed to delete clip '{}'", nameStr);
  }
}

void AnimationRegistry::UnregisterAll(stringv spritesheetPath) {
  std::vector<string> toErase;
  _clipsMap.ForEach([&](const string &key, const scene::AnimationClip &clip) {
    if (clip.spritesheetPath.View() == spritesheetPath) {
      toErase.push_back(key);
    }
  });
  for (const auto &key : toErase) {
    _clipsMap.Erase(key);
  }
}

void AnimationRegistry::Clear() {
  _clipsMap.Clear();
}

const scene::AnimationClip *AnimationRegistry::Get(stringv name) const {
  return _clipsMap.Retrieve(string(name));
}

bool AnimationRegistry::Has(stringv name) const {
  return _clipsMap.Has(string(name));
}

} // namespace flatearth::assets

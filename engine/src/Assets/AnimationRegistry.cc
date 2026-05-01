#include "AnimationRegistry.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Scene/Components/SpriteAnimator.hpp"

namespace flatearth::assets {

AnimationRegistry::AnimationRegistry(memory::MemoryManager &mm, AssetManager &am)
    : _clipsMap(mm), _assetManager(am) {
}

FeExpect<void, Error> AnimationRegistry::AddClip(stringv name, const scene::SheetClip &sheetClip) {
  auto texRes = _assetManager.LoadTexture(string(sheetClip.sheet))
                    .or_error("failed to load texture for animation clip");
  if (texRes.errored()) {
    return FeErr{texRes.error()};
  }

  resources::Texture *tex = _assetManager.GetTexture(texRes.value());
  if (tex == nullptr) {
    return FeErr{Error("failed to get texture for sheet clip", ErrorType::NullptrException)};
  }

  float32 sheetW = static_cast<float32>(tex->width);
  float32 sheetH = static_cast<float32>(tex->height);
  float32 tileW = static_cast<float32>(sheetClip.tileWidth);
  float32 tileH = static_cast<float32>(sheetClip.tileHeight);

  auto clipRes =
      NewClip(name, sheetClip.sheet).or_error("failed to create new clip from sheet clip");
  if (clipRes.errored()) {
    return FeErr{clipRes.error()};
  }

  scene::AnimationClip clip = clipRes.value();
  clip.loop = sheetClip.loop;
  clip.frameCount = static_cast<uint8>(sheetClip.colEnd - sheetClip.colStart + 1);

  for (uint8 i = 0; i < clip.frameCount && i < scene::cMaxAnimationFrames; i++) {
    uint32 col = sheetClip.colStart + i;
    clip.frames[i] = scene::AnimationFrame{
        .uvOffset = math::Vec2D{col * tileW / sheetW, sheetClip.row * tileH / sheetH},
        .uvScale = math::Vec2D{tileW / sheetW, -tileH / sheetH},
        .duration = sheetClip.duration,
    };
  }

  RegisterClip(clip);
  return {};
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

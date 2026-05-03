#include "AssetManager.hpp"

#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Scene/Components/Sprite.hpp"

namespace flatearth::assets {

AssetManager::AssetManager(memory::MemoryManager &memManager, platform::FileSystem &fs)
    : _textureCache(memManager, fs), _fontCache(memManager, fs), _memoryManager(memManager),
      _fs(fs) {
}

void AssetManager::Initialize(renderer::FrontendRenderer *pRenderer) {
  if (pRenderer == nullptr) {
    return;
  }

  _textureCache.Initialize(pRenderer);
  _fontCache.Initialize(pRenderer);
  _pRenderer = pRenderer;
}

FeExpect<resources::TextureHandle, Error>
AssetManager::LoadTexture(const string &path, resources::TextureFilter filter) {
  return _textureCache.AcquireTexture(string(path), filter);
}

void AssetManager::ReleaseTexture(resources::TextureHandle handle) {
  _textureCache.Release(handle);
}

resources::Texture *AssetManager::GetTexture(resources::TextureHandle handle) {
  return _textureCache.Get(handle);
}

FeExpect<scene::Sprite, Error> AssetManager::LoadSprite(stringv path,
                                                        resources::MeshShape shape,
                                                        resources::TextureFilter filter) {
  auto loadRes = LoadTexture(string(path), filter);
  if (loadRes.errored()) {
    return FeErr{loadRes.error()};
  }

  resources::TextureHandle handle = loadRes.value();
  scene::Sprite sprite{};
  sprite.texHandle = handle;

  resources::Texture *pTexture = GetTexture(handle);
  if (pTexture == nullptr) {
    return FeErr{Error("cannot create sprite with nullptr texture", ErrorType::NullptrException)};
  }

  auto acMatRes = _pRenderer->AcquireMaterial(string(path), pTexture);
  if (acMatRes.errored()) {
    return FeErr{acMatRes.error()};
  }
  sprite.matHandle = acMatRes.value();

  auto acMeshRes = _pRenderer->AcquireMesh(shape);
  if (acMeshRes.errored()) {
    return FeErr{acMeshRes.error()};
  }
  sprite.meshHandle = acMeshRes.value();

  return std::move(sprite);
}

FeExpect<scene::Sprite, Error> AssetManager::SpriteFromTexture(resources::TextureHandle texHandle,
                                                               stringv name,
                                                               resources::MeshShape shape) {
  resources::Texture *pTexture = GetTexture(texHandle);
  if (pTexture == nullptr)
    return FeErr{Error("SpriteFromTexture: invalid texture handle", ErrorType::NullptrException)};

  scene::Sprite sprite{};
  sprite.texHandle = texHandle;

  auto acMatRes = _pRenderer->AcquireMaterial(string(name), pTexture);
  if (acMatRes.errored())
    return FeErr{acMatRes.error()};
  sprite.matHandle = acMatRes.value();

  auto acMeshRes = _pRenderer->AcquireMesh(shape);
  if (acMeshRes.errored())
    return FeErr{acMeshRes.error()};
  sprite.meshHandle = acMeshRes.value();

  return sprite;
}

void AssetManager::ReleaseSprite(scene::Sprite &sprite) {
  _pRenderer->ReleaseMaterial(sprite.matHandle);
  _pRenderer->ReleaseMesh(sprite.meshHandle);
  ReleaseTexture(sprite.texHandle);
  sprite = scene::Sprite{};
}

FeExpect<resources::FontHandle, Error> AssetManager::LoadFont(const string &path,
                                                              const string &ttfPath,
                                                              float32 fontSize,
                                                              int32 atlasSize) {
  return _fontCache.AcquireFont(path, ttfPath, fontSize, atlasSize);
}

void AssetManager::ReleaseFont(resources::FontHandle handle) {
  return _fontCache.Release(handle);
}

resources::FontAtlas *AssetManager::GetFontAtlas(resources::FontHandle handle) {
  return _fontCache.Get(handle);
}

void AssetManager::Shutdown() {
  _textureCache.Shutdown();
  _fontCache.Shutdown();
}

bool AssetManager::Initialized() const {
  return _pRenderer != nullptr;
}

} // namespace flatearth::assets

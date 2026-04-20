#include "MaterialCache.hpp"

#include "Core/Logger.hpp"

namespace flatearth::resources {

MaterialCache::MaterialCache(memory::MemoryManager &memManager,
                             renderer::FrontendRenderer &renderer,
                             TextureCache &textureCache)
    : ResourceCache(memManager), _renderer(renderer), _textureCache(textureCache) {}

FeExpect<MaterialHandle, Error> MaterialCache::AcquireMaterial(const string &name,
                                                               TextureHandle texHandle) {
  _pendingTexHandle = texHandle;
  return this->ProtectedAcquire(name);
}

FeExpect<void, Error> MaterialCache::Create(Material *pMaterial,
                                            uint32 handle,
                                            const string &name) {
  Texture *pTexture = _textureCache.Get(_pendingTexHandle);
  if (pTexture == nullptr) {
    FLOG_ERROR("failed to resolve texture handle {} for material '{}'", _pendingTexHandle, name);
    return FeErr{Error("texture handle could not be resolved", ErrorType::NullptrException)};
  }

  pMaterial->id = handle;
  pMaterial->name = name;
  pMaterial->texHandle = _pendingTexHandle;

  return _renderer.CreateMaterial(pMaterial, pTexture);
}

FeExpect<void, Error> MaterialCache::Destroy(Material *pMaterial) {
  return _renderer.DestroyMaterial(pMaterial);
}

} // namespace flatearth::resources

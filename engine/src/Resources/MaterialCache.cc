#include "MaterialCache.hpp"

#include "Core/Logger.hpp"
#include "Renderer/RendererFrontend.hpp"

namespace flatearth::resources {

MaterialCache::MaterialCache(memory::MemoryManager &memManager,
                             renderer::FrontendRenderer &renderer)
    : ResourceCache(memManager), _renderer(renderer) {}

FeExpect<MaterialHandle, Error> MaterialCache::AcquireMaterial(const string &name,
                                                               Texture *pTexture) {
  _pendingTexture = pTexture;
  return this->ProtectedAcquire(name);
}

FeExpect<void, Error> MaterialCache::Create(Material *pMaterial,
                                            uint32 handle,
                                            const string &name) {
  if (_pendingTexture == nullptr) {
    FLOG_ERROR("nullptr texture passed for material '{}'", name);
    return FeErr{Error("texture pointer is null", ErrorType::NullptrException)};
  }

  pMaterial->id = handle;
  pMaterial->name = name;
  pMaterial->texHandle = _pendingTexture->id;

  return _renderer.CreateMaterial(pMaterial, _pendingTexture);
}

FeExpect<void, Error> MaterialCache::Destroy(Material *pMaterial) {
  return _renderer.DestroyMaterial(pMaterial);
}

} // namespace flatearth::resources

#include "MaterialSystem.hpp"

#include "Core/Logger.hpp"

namespace flatearth::resources {

MaterialSystem::MaterialSystem(memory::MemoryManager &memManager,
                               renderer::FrontendRenderer &renderer,
                               TextureSystem &textureSystem)
    : ResourceSystem(memManager), _renderer(renderer), _textureSystem(textureSystem) {}

FeExpect<MaterialHandle, Error> MaterialSystem::AcquireMaterial(const string &name,
                                                                TextureHandle texHandle) {
  _pendingTexHandle = texHandle;
  return this->ProtectedAcquire(name);
}

FeExpect<void, Error> MaterialSystem::Create(Material *pMaterial,
                                             uint32 handle,
                                             const string &name) {
  Texture *pTexture = _textureSystem.Get(_pendingTexHandle);
  if (pTexture == nullptr) {
    FLOG_ERROR("failed to resolve texture handle {} for material '{}'", _pendingTexHandle, name);
    return FeErr{Error("texture handle could not be resolved", ErrorType::NullptrException)};
  }

  pMaterial->id = handle;
  pMaterial->name = name;
  pMaterial->texHandle = _pendingTexHandle;

  return _renderer.CreateMaterial(pMaterial, pTexture);
}

FeExpect<void, Error> MaterialSystem::Destroy(Material *pMaterial) {
  return _renderer.DestroyMaterial(pMaterial);
}

} // namespace flatearth::resources

#include "MaterialSystem.hpp"

#include "Core/Logger.hpp"

namespace flatearth::resources {

MaterialSystem::MaterialSystem(memory::MemoryManager &memManager,
                               renderer::FrontendRenderer &renderer,
                               TextureSystem &textureSystem)
    : _memoryManager(memManager), _frontendRenderer(renderer), _textureSystem(textureSystem),
      _nameHandleMap(memManager), _handleEntryMap(memManager), _activeHandles(memManager) {}

FeExpect<MaterialHandle, Error> MaterialSystem::AcquireMaterial(const string &name,
                                                                TextureHandle texHandle) {
  MaterialHandle *pHandle = _nameHandleMap.Retrieve(name);
  if (pHandle != nullptr) {
    MaterialEntry *pEntry = _handleEntryMap.Retrieve(*pHandle);
    if (pEntry == nullptr) {
      FLOG_ERROR("handle {} exists for a nullptr material entry", *pHandle);
      return FeErr{Error("handle exists but represents no material entry", ErrorType::LogicalError)};
    }
    pEntry->refCount++;
    return *pHandle;
  }

  Texture *pTexture = _textureSystem.GetTexture(texHandle);
  if (pTexture == nullptr) {
    FLOG_ERROR("failed to resolve texture handle {} for material '{}'", texHandle, name);
    return FeErr{Error("texture handle could not be resolved", ErrorType::NullptrException)};
  }

  Material material{};
  material.id = _nextHandle;
  material.name = name;
  material.texHandle = texHandle;

  auto createRes = _frontendRenderer.CreateMaterial(&material, pTexture);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create material '{}'", name);
    return FeErr{createRes.error()};
  }

  auto insertRes = _nameHandleMap.Insert(name, _nextHandle);
  if (!insertRes.has_value()) {
    FLOG_ERROR("failed to cache name to handle for material '{}'", name);
    return FeErr{insertRes.error()};
  }

  MaterialEntry entry{};
  entry.material = material;
  entry.refCount = 1;
  insertRes = _handleEntryMap.Insert(_nextHandle, entry);
  if (!insertRes.has_value()) {
    bool _ = _nameHandleMap.Erase(name);
    FLOG_ERROR("failed to cache handle to material entry for '{}'", name);
    return FeErr{insertRes.error()};
  }

  _activeHandles.Push(_nextHandle);
  return _nextHandle++;
}

void MaterialSystem::ReleaseMaterial(MaterialHandle handle) {
  MaterialEntry *pEntry = _handleEntryMap.Retrieve(handle);
  if (pEntry == nullptr) {
    FLOG_WARN("no material entry found for handle {}", handle);
    return;
  }

  if (pEntry->refCount >= 1) {
    pEntry->refCount--;
  }

  if (pEntry->refCount != 0) {
    return;
  }

  auto destroyRes = _frontendRenderer.DestroyMaterial(&pEntry->material);
  if (!destroyRes.has_value()) {
    FLOG_FATAL("failed to destroy material on GPU — resource leaked, handle will be discarded");
  }

  bool erased = _nameHandleMap.Erase(pEntry->material.name);
  if (!erased) {
    FLOG_ERROR("failed to erase material from name map");
  }

  erased = _handleEntryMap.Erase(handle);
  if (!erased) {
    FLOG_ERROR("failed to erase material entry from handle map");
  }
}

Material *MaterialSystem::GetMaterial(MaterialHandle handle) {
  MaterialEntry *pEntry = _handleEntryMap.Retrieve(handle);
  if (pEntry == nullptr) {
    FLOG_WARN("no material entry found for handle {}", handle);
    return nullptr;
  }
  return &pEntry->material;
}

void MaterialSystem::Shutdown() {
  for (uint64 i = 0; i < _activeHandles.Length(); i++) {
    MaterialHandle handle = _activeHandles[i];
    MaterialEntry *pEntry = _handleEntryMap.Retrieve(handle);
    if (pEntry == nullptr) {
      continue; // already properly released
    }

    if (pEntry->refCount > 0) {
      FLOG_WARN("MaterialSystem::Shutdown — material '{}' still has {} reference(s), force destroying",
                pEntry->material.name, pEntry->refCount);
    }

    auto destroyRes = _frontendRenderer.DestroyMaterial(&pEntry->material);
    if (!destroyRes.has_value()) {
      FLOG_FATAL("MaterialSystem::Shutdown — failed to destroy material '{}'",
                 pEntry->material.name);
    }

    _nameHandleMap.Erase(pEntry->material.name);
    _handleEntryMap.Erase(handle);
  }

  _activeHandles.Clear();
}

} // namespace flatearth::resources

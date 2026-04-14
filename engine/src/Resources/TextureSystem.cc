#include "TextureSystem.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "Vendor/stb_image.h"

namespace flatearth::resources {

TextureSystem::TextureSystem(memory::MemoryManager &memManager,
                             renderer::FrontendRenderer &renderer,
                             platform::FileSystem &fs)
    : _memoryManager(memManager), _frontendRenderer(renderer), _filesystem(fs),
      _pathHandleMap(memManager), _handleEntryMap(memManager) {
}

FeExpect<TextureHandle, Error> TextureSystem::AcquireTexture(const string &path) {
  TextureHandle *pHandle = _pathHandleMap.Retrieve(path);
  if (pHandle != nullptr) {
    TextureEntry *pEntry = _handleEntryMap.Retrieve(*pHandle);
    if (pEntry == nullptr) {
      FLOG_ERROR("handle {} exists for a nullptr entry", *pHandle);
      return FeErr{
          Error("something unexpected happened: handle exists but represents no texture entry!",
                ErrorType::LogicalError)};
    }

    pEntry->refCount++;
    return *pHandle;
  }

  Texture texture{};
  auto loadRes = LoadTexture(path, &texture);
  if (!loadRes.has_value()) {
    FLOG_ERROR("failed to load texture at {}", path);
    return FeErr{loadRes.error()};
  }

  auto insertRes = _pathHandleMap.Insert(path, _nextHandle);
  if (!insertRes.has_value()) {
    FLOG_ERROR("failed to cache path to handle");
    return FeErr{insertRes.error()};
  }

  TextureEntry entry{};
  entry.path = path;
  entry.texture = texture;
  entry.refCount = 1;
  insertRes = _handleEntryMap.Insert(_nextHandle, entry);
  if (!insertRes.has_value()) {
    // Erase so that we dont have logical error
    bool _ = _pathHandleMap.Erase(path);
    FLOG_ERROR("failed to cache handle to texture entry");
    return FeErr{insertRes.error()};
  }

  return _nextHandle++;
}

void TextureSystem::ReleaseTexture(TextureHandle handle) {
  TextureEntry *pEntry = _handleEntryMap.Retrieve(handle);
  if (pEntry == nullptr) {
    FLOG_WARN("entry does not exist in current handle entry map");
    return;
  }

  // Sanity check so that we dont overflow
  if (pEntry->refCount >= 1) {
    pEntry->refCount--;
  }

  if (pEntry->refCount != 0) {
    return;
  }

  // NOTE: if DestroyTexture fails here, the entry is still erased from both
  // maps below, meaning the handle is permanently gone with no retry path.
  // This is an accepted tradeoff: a GPU-level destroy failure is unrecoverable.
  // Revisit with a zombie/retry mechanism when a proper error recovery layer exists.
  auto destroyRes = _frontendRenderer.DestroyTexture(&pEntry->texture);
  if (!destroyRes.has_value()) {
    FLOG_FATAL("failed to destroy texture on GPU — resource leaked, handle will be discarded");
  }

  bool erased = _pathHandleMap.Erase(pEntry->path);
  if (!erased) {
    FLOG_ERROR("failed to erase handle from path map");
  }

  erased = _handleEntryMap.Erase(handle);
  if (!erased) {
    FLOG_ERROR("failed to erase texture entry from handle map");
  }
}

Texture *TextureSystem::GetTexture(TextureHandle handle) {
  TextureEntry *pEntry = _handleEntryMap.Retrieve(handle);
  if (pEntry == nullptr) {
    FLOG_WARN("no entry found for handle {}", handle);
    return nullptr;
  }

  return &pEntry->texture;
}

FeExpect<void, Error> TextureSystem::LoadTexture(const string &path, Texture *pTexture) {
  if (!_filesystem.Exists(path)) {
    FLOG_ERROR("texture file not found: {}", path);
    return FeErr{Error("texture file not found", ErrorType::FileOpenError)};
  }

  auto handleRes = _filesystem.OpenFile(path, platform::FileMode::Read, FeTrue);
  if (!handleRes.has_value()) {
    FLOG_ERROR("failed to open texture file: {}", path);
    return FeErr{handleRes.error()};
  }

  platform::FileHandle handle = handleRes.value();
  uint64 fileSize = _filesystem.SizeOfFile(path);

  containers::DArray<uint8> rawBytes(_memoryManager);
  rawBytes.Resize(fileSize);
  auto readRes = _filesystem.ReadFromFile(
      handle, std::span<std::byte>(reinterpret_cast<std::byte *>(rawBytes.Data()), fileSize));
  auto closeRes = _filesystem.CloseFile(handle);
  if (!closeRes.has_value()) {
    FLOG_ERROR("failed to close texture file: {}", path);
    return FeErr{closeRes.error()};
  }

  if (!readRes.has_value()) {
    FLOG_ERROR("failed to read texture file: {}", path);
    return FeErr{readRes.error()};
  }

  int32 width, height, channels;
  uint8 *pPixels = stbi_load_from_memory(
      rawBytes.Data(), static_cast<int32>(fileSize), &width, &height, &channels, STBI_rgb_alpha);

  if (pPixels == nullptr) {
    FLOG_ERROR("stb failed to decode texture: {}", path);
    return FeErr{Error("stb image decode failed", ErrorType::Unknown)};
  }

  bool hasTransparency = channels == 4;
  auto texRes = _frontendRenderer.CreateTexture(
      path, false, width, height, 4, pPixels, hasTransparency, pTexture);

  stbi_image_free(pPixels);

  if (!texRes.has_value()) {
    FLOG_ERROR("failed to create texture from file: {}", path);
    return FeErr{texRes.error()};
  }

  return {};
}

} // namespace flatearth::resources

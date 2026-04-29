#include "TextureCache.hpp"

#include "Core/Logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "Vendor/stb_image.h"

namespace flatearth::resources {

TextureCache::TextureCache(memory::MemoryManager &memManager, platform::FileSystem &fs)
    : ResourceCache(memManager), _fs(fs) {
}

void TextureCache::Initialize(renderer::FrontendRenderer *pRenderer) {
  if (pRenderer == nullptr) {
    return;
  }

  _pRenderer = pRenderer;
}

FeExpect<TextureHandle, Error> TextureCache::AcquireTexture(const string &path,
                                                            TextureFilter filter) {
  _filterToUse = filter;
  return this->ProtectedAcquire(path);
}

bool TextureCache::Initialized() const {
  return _pRenderer != nullptr;
}

FeExpect<void, Error>
TextureCache::Create(Texture *pTexture, uint32 /*handle*/, const string &path) {
  if (!_fs.Exists(path)) {
    FLOG_ERROR("texture file not found: {}", path);
    return FeErr{Error("texture file not found", ErrorType::FileOpenError)};
  }

  auto handleRes = _fs.OpenFile(path, platform::FileMode::Read, FeTrue).or_error("failed to open texture file");
  if (handleRes.errored()) {
    return FeErr{handleRes.error()};
  }

  platform::FileHandle fileHandle = handleRes.value();
  uint64 fileSize = _fs.SizeOfFile(path);

  containers::DArray<uint8> rawBytes(_memoryManager);
  rawBytes.Resize(fileSize);

  auto readRes = _fs.ReadFromFile(
      fileHandle, std::span<std::byte>(reinterpret_cast<std::byte *>(rawBytes.Data()), fileSize));

  if (auto closeRes = _fs.CloseFile(fileHandle); closeRes.errored()) {
    FLOG_ERROR("failed to close texture file: {}", path);
    return FeErr{closeRes.error()};
  }

  if (readRes.errored()) {
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
  auto texRes = _pRenderer->CreateTexture(
      path, false, width, height, 4, pPixels, hasTransparency, pTexture, _filterToUse);

  stbi_image_free(pPixels);

  if (texRes.errored()) {
    FLOG_ERROR("failed to create texture from file: {}", path);
    return FeErr{texRes.error()};
  }

  return {};
}

FeExpect<void, Error> TextureCache::Destroy(Texture *pTexture) {
  return _pRenderer->DestroyTexture(pTexture);
}

} // namespace flatearth::resources

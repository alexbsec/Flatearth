#include "TextureLoader.hpp"

#include "Core/Logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "Vendor/stb_image.h"

#include <vector>

namespace flatearth::resources {

FeExpect<void, Error> LoadTexture(const string &path,
                                  platform::FileSystem &filesystem,
                                  renderer::FrontendRenderer &renderer,
                                  Texture *pTexture) {
  if (!filesystem.Exists(path)) {
    FLOG_ERROR("texture file not found: {}", path);
    return FeErr{Error("texture file not found", ErrorType::FileOpenError)};
  }

  auto handleRes = filesystem.OpenFile(path, platform::FileMode::Read, FeTrue);
  if (!handleRes.has_value()) {
    FLOG_ERROR("failed to open texture file: {}", path);
    return FeErr{handleRes.error()};
  }

  platform::FileHandle handle = handleRes.value();
  uint64 fileSize = filesystem.SizeOfFile(path);

  std::vector<uint8> rawBytes(fileSize);
  auto readRes = filesystem.ReadFromFile(
      handle, std::span<std::byte>(reinterpret_cast<std::byte *>(rawBytes.data()), fileSize));
  auto closeRes = filesystem.CloseFile(handle);
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
      rawBytes.data(), static_cast<int32>(fileSize), &width, &height, &channels, STBI_rgb_alpha);

  if (pPixels == nullptr) {
    FLOG_ERROR("stb failed to decode texture: {}", path);
    return FeErr{Error("stb image decode failed", ErrorType::Unknown)};
  }

  bool hasTransparency = channels == 4;
  auto texRes =
      renderer.CreateTexture(path, false, width, height, 4, pPixels, hasTransparency, pTexture);

  stbi_image_free(pPixels);

  if (!texRes.has_value()) {
    FLOG_ERROR("failed to create texture from file: {}", path);
    return FeErr{texRes.error()};
  }

  return {};
}

} // namespace flatearth::resources

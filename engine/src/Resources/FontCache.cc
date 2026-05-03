#include "FontCache.hpp"

#include "Core/Logger.hpp"
#include "Platform/Filesystem.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "Vendor/stb_truetype.h"

namespace flatearth::resources {

static constexpr int32 cFirstChar = 32;
static constexpr int32 cCharCount = 96;

FontCache::FontCache(memory::MemoryManager &mm, platform::FileSystem &fs)
    : ResourceCache(mm), _filesystem(fs) {
}

void FontCache::Initialize(renderer::FrontendRenderer *pRenderer) {
  _pRenderer = pRenderer;
}

bool FontCache::Initialized() const {
  return _pRenderer != nullptr;
}

FeExpect<FontHandle, Error> FontCache::AcquireFont(const string &name,
                                                   const string &ttfPath,
                                                   float32 fontSize,
                                                   int32 atlasSize) {
  _pendingTtfPath = ttfPath;
  _pendingFontSize = fontSize;
  _pendingAtlasSize = atlasSize;
  return this->ProtectedAcquire(name);
}

FeExpect<void, Error> FontCache::Create(FontAtlas *pAtlas, uint32 handle, const string &name) {
  if (_pRenderer == nullptr) {
    FLOG_ERROR("FontCache::Create called before Initialize");
    return FeErr{Error("renderer not set", ErrorType::NullptrException)};
  }

  if (!_filesystem.Exists(_pendingTtfPath)) {
    FLOG_ERROR("font file not found: {}", _pendingTtfPath);
    return FeErr{Error("font file not found", ErrorType::FileOpenError)};
  }

  auto fileHandleRes = _filesystem.OpenFile(_pendingTtfPath, platform::FileMode::Read, FeTrue)
                           .or_error("failed to open font file");
  if (fileHandleRes.errored()) {
    return FeErr{fileHandleRes.error()};
  }

  platform::FileHandle fileHandle = fileHandleRes.value();
  uint64 fileSize = _filesystem.SizeOfFile(_pendingTtfPath);

  containers::DArray<uint8> ttfBytes(_memoryManager);
  ttfBytes.Resize(fileSize);

  auto readRes = _filesystem.ReadFromFile(
      fileHandle, std::span<std::byte>(reinterpret_cast<std::byte *>(ttfBytes.Data()), fileSize));

  if (auto closeRes = _filesystem.CloseFile(fileHandle); closeRes.errored()) {
    FLOG_ERROR("failed to close font file: {}", _pendingTtfPath);
    return FeErr{closeRes.error()};
  }

  if (readRes.errored()) {
    FLOG_ERROR("failed to read font file: {}", _pendingTtfPath);
    return FeErr{readRes.error()};
  }

  // Rasterize all printable ASCII glyphs into a single-channel bitmap
  stbtt_bakedchar bakedChars[cCharCount];
  containers::DArray<uint8> atlasBitmap(_memoryManager);
  atlasBitmap.Resize(_pendingAtlasSize * _pendingAtlasSize);

  int32 result = stbtt_BakeFontBitmap(ttfBytes.Data(),
                                      0,
                                      _pendingFontSize,
                                      atlasBitmap.Data(),
                                      _pendingAtlasSize,
                                      _pendingAtlasSize,
                                      cFirstChar,
                                      cCharCount,
                                      bakedChars);

  if (result <= 0) {
    FLOG_ERROR(
        "stb_truetype failed to bake atlas for font '{}' — try a larger atlas or smaller font size",
        name);
    return FeErr{Error("font atlas bake failed", ErrorType::Unknown)};
  }

  // Expand single-channel to RGBA: white with alpha from the glyph mask
  containers::DArray<uint8> rgbaPixels(_memoryManager);
  rgbaPixels.Resize(_pendingAtlasSize * _pendingAtlasSize * 4);
  for (int32 i = 0; i < _pendingAtlasSize * _pendingAtlasSize; i++) {
    rgbaPixels[i * 4 + 0] = 255;
    rgbaPixels[i * 4 + 1] = 255;
    rgbaPixels[i * 4 + 2] = 255;
    rgbaPixels[i * 4 + 3] = atlasBitmap[i];
  }

  auto texRes = _pRenderer->CreateTexture(name,
                                          false,
                                          _pendingAtlasSize,
                                          _pendingAtlasSize,
                                          4,
                                          rgbaPixels.Data(),
                                          FeTrue,
                                          &pAtlas->texture,
                                          TextureFilter::Linear);
  if (texRes.errored()) {
    FLOG_ERROR("failed to upload font atlas texture for '{}'", name);
    return FeErr{texRes.error()};
  }

  auto matRes = _pRenderer->AcquireMaterial(name, &pAtlas->texture);
  if (matRes.errored()) {
    _pRenderer->DestroyTexture(&pAtlas->texture);
    FLOG_ERROR("failed to create font material for '{}'", name);
    return FeErr{matRes.error()};
  }

  auto meshRes = _pRenderer->AcquireMesh(MeshShape::Quad);
  if (meshRes.errored()) {
    _pRenderer->ReleaseMaterial(pAtlas->matHandle);
    _pRenderer->DestroyTexture(&pAtlas->texture);
    FLOG_ERROR("failed to acquire quad mesh for font '{}'", name);
    return FeErr{meshRes.error()};
  }

  pAtlas->id = handle;
  pAtlas->name = name;
  pAtlas->matHandle = matRes.value();
  pAtlas->quadMeshHandle = meshRes.value();

  // Line height from font vertical metrics
  stbtt_fontinfo fontInfo;
  stbtt_InitFont(&fontInfo, ttfBytes.Data(), 0);
  float32 scale = stbtt_ScaleForPixelHeight(&fontInfo, _pendingFontSize);
  int32 ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
  pAtlas->lineHeight = (ascent - descent + lineGap) * scale;

  // Fill per-glyph data indexed by (char - cFirstChar)
  for (int32 i = 0; i < cCharCount; i++) {
    const stbtt_bakedchar &bc = bakedChars[i];
    GlyphInfo &glyph = pAtlas->glyphs[i];
    glyph.uvOffset = {bc.x0 / static_cast<float32>(_pendingAtlasSize),
                      bc.y0 / static_cast<float32>(_pendingAtlasSize)};
    glyph.uvSize = {(bc.x1 - bc.x0) / static_cast<float32>(_pendingAtlasSize),
                    (bc.y1 - bc.y0) / static_cast<float32>(_pendingAtlasSize)};
    glyph.advance = bc.xadvance;
    glyph.bearingX = bc.xoff;
    glyph.bearingY = bc.yoff;
    glyph.width = static_cast<float32>(bc.x1 - bc.x0);
    glyph.height = static_cast<float32>(bc.y1 - bc.y0);
  }

  return {};
}

FeExpect<void, Error> FontCache::Destroy(FontAtlas *pAtlas) {
  _pRenderer->ReleaseMesh(pAtlas->quadMeshHandle);
  _pRenderer->ReleaseMaterial(pAtlas->matHandle);
  return _pRenderer->DestroyTexture(&pAtlas->texture);
}

} // namespace flatearth::resources

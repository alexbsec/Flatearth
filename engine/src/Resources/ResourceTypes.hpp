#ifndef _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP
#define _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP

#include "Containers/DArray.hpp"
#include "Containers/HashMap.hpp"
#include "Defines.hpp"
#include "Math/Vector2D.hpp"
#include "Vendor/miniaudio.h"

namespace flatearth::resources {

#define DEFINE_HANDLE(Name) \
  using Name = uint32;      \
  constexpr Name cInvalid##Name = std::numeric_limits<uint32>::max();

// DEFINED HANDLES:
DEFINE_HANDLE(TextureHandle);
DEFINE_HANDLE(MaterialHandle);
DEFINE_HANDLE(MeshHandle);
DEFINE_HANDLE(SoundHandle);
DEFINE_HANDLE(FontHandle);

enum class MeshShape : uint16 {
  Quad,
  Circle,
  Capsule2D,
};

enum class TextureFilter {
  Nearest,
  Linear,
};

struct Mesh {
  uint32 id{0};
  string name{};
  uint32 vertexCount{0};
  uint32 indexCount{0};
  void *pInternalData{nullptr};
};

struct Material {
  uint32 id{0};
  string name{};
  TextureHandle texHandle{cInvalidTextureHandle};
  void *pInternalData{nullptr};
};

struct Texture {
  uint32 id{0};
  uint32 width{0};
  uint32 height{0};
  uint32 generation{0};
  uint32 channelCount{0};
  bool hasTransparency{0};
  TextureFilter filter{TextureFilter::Nearest};
  void *pInternalData{nullptr};
};

struct Audio {
  string name{};
  SoundHandle sndHandle{cInvalidSoundHandle};
  ma_sound *pSound{nullptr};
};

struct GlyphInfo {
  math::Vec2D uvOffset{0.0f, 0.0f};
  math::Vec2D uvSize{1.0f, 1.0f};
  float32 advance{0.0f};
  float32 bearingX{0.0f};
  float32 bearingY{0.0f};
  float32 width{1.0f};
  float32 height{1.0f};
};

struct FontAtlas {
  uint32 id{0};
  string name{};
  float32 lineHeight{1.0f};
  MaterialHandle matHandle{cInvalidMaterialHandle};
  MeshHandle quadMeshHandle{cInvalidMeshHandle};
  Texture texture{};
  std::array<GlyphInfo, 96> glyphs;
};

}; // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP

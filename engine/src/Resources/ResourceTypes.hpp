#ifndef _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP
#define _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP

#include "Defines.hpp"
#include "Vendor/miniaudio.h"

namespace flatearth::resources {

using TextureHandle = uint32;
using MaterialHandle = uint32;
using MeshHandle = uint32;
using SoundHandle = uint32;

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
  uint32 id;
  string name;
  uint32 vertexCount;
  uint32 indexCount;
  void *pInternalData;
};

struct Material {
  uint32 id;
  string name;
  TextureHandle texHandle;
  void *pInternalData;
};

struct Texture {
  uint32 id;
  uint32 width;
  uint32 height;
  uint32 generation;
  uint32 channelCount;
  bool hasTransparency;
  TextureFilter filter;
  void *pInternalData;
};

struct Audio {
  string name;
  SoundHandle sndHandle;
  ma_sound *pSound{nullptr};
};

}; // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP

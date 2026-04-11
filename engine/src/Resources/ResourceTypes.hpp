#ifndef _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP
#define _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP

#include "Defines.hpp"

namespace flatearth::resources {

struct Texture {
  uint32 id;
  uint32 width;
  uint32 height;
  uint32 generation;
  uint32 channelCount;
  bool hasTransparency;
  void *pInternalData;
};

};

#endif // _FLATEARTH_ENGINE_RESOURCES_RESOURCE_TYPES_HPP

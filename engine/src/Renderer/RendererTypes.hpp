#ifndef _FLATEARHT_ENGINE_RENDERER_TYPES_HPP
#define _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

#include "Containers/DArray.hpp"
#include "Math/Matrix4D.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::renderer {

struct RenderObject {
  uint32 geometryId;
  math::Mat4D model;
  resources::Material *pMaterial{nullptr};
};

struct RenderPacket {
  float32 deltaTime;
  math::Mat4D view;
  containers::DArray<RenderObject> objects;

  RenderPacket(memory::MemoryManager &memManager)
    : objects(memManager) {}
};

struct GlobalUniformObject {
  math::Mat4D projection, view;
  // These below are so that GlobalUniformObject always
  // has a size of 256 bytes
  math::Mat4D reservedSpace1, reservedSpace2;
};

} // namespace flatearth::renderer

#endif // _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

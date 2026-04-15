#ifndef _FLATEARHT_ENGINE_RENDERER_TYPES_HPP
#define _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

#include "Containers/DArray.hpp"
#include "Math/Matrix4D.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::renderer {

enum class RenderLayer : uint32 {
  Background = 0,
  Tiles = 1,
  Entities = 2,
  Effects = 3,
  UI = 4,
};

struct RenderObject {
  uint32 geometryId;
  math::Mat4D model;
  math::Vec2D uvOffset;
  math::Vec2D uvScale;
  RenderLayer layer{RenderLayer::Background};
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

struct PushConstantData {
  math::Mat4D model;
  math::Vec2D uvOffset;
  math::Vec2D uvScale;
};

} // namespace flatearth::renderer

#endif // _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

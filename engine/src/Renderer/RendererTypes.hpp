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

struct Tint {
  math::Vec3D rgb{};
  float32 alpha{1.0f};
};

struct RenderObject {
  uint32 geometryId;
  math::Mat4D model;
  math::Vec2D uvOffset;
  math::Vec2D uvScale;
  RenderLayer layer{RenderLayer::Background};
  resources::Material *pMaterial{nullptr};
  Tint tint{};
  float32 useTexture{0.0f};
};

struct RenderPacket {
  float32 deltaTime{0.0f};
  math::Mat4D view{};
  containers::DArray<RenderObject> objects;
  containers::DArray<RenderObject> uiObjects;

  RenderPacket(memory::MemoryManager &memManager) : objects(memManager), uiObjects(memManager) {}
};

struct GlobalUniformObject {
  math::Mat4D projection, view;
  // These below are so that GlobalUniformObject always
  // has a size of 256 bytes
  math::Mat4D reservedSpace1, reservedSpace2;
};

struct UIPushConstantData {
  math::Mat4D model{};
  math::Vec2D uvOffset{};
  math::Vec2D uvScale{};
  Tint tint{};
  float32 useTexture{0.0f};
};

struct PushConstantData {
  math::Mat4D model;
  math::Vec2D uvOffset;
  math::Vec2D uvScale;
};

using ShaderHandle = uint32;

enum class ShaderInterpreter {
  Vulkan,
  OpenGL,
};

struct Shader {
  ShaderInterpreter interpreter{ShaderInterpreter::Vulkan};
  virtual ~Shader() = default;
};


} // namespace flatearth::renderer

#endif // _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

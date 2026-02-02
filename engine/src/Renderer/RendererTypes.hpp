#ifndef _FLATEARHT_ENGINE_RENDERER_TYPES_HPP
#define _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

#include "Math/Matrix4D.hpp"

namespace flatearth::renderer {

struct GlobalUniformObject {
  math::Mat4D projection, view;
  // These below are so that GlobalUniformObject always
  // has a size of 256 bytes
  math::Mat4D reservedSpace1, reservedSpace2;
};

} // namespace flatearth::renderer

#endif // _FLATEARHT_ENGINE_RENDERER_TYPES_HPP

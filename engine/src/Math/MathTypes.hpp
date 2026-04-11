#ifndef _FLATEARTH_ENGINE_MATH_MATH_TYPES_HPP
#define _FLATEARTH_ENGINE_MATH_MATH_TYPES_HPP

#include "Vector3D.hpp"

namespace flatearth::math {

enum class MatrixLayout : uint8 { RowMajor, ColumnMajor };

constexpr MatrixLayout CPUMatrixLayout() {
  return MatrixLayout::RowMajor;
}

constexpr MatrixLayout GPUMatrixLayout() {
  return MatrixLayout::ColumnMajor;
}

struct Vertex3D {
  Vec3D position;
  Vec2D uv;
};

} // namespace flatearth::math

#endif // _FLATEARTH_ENGINE_MATH_MATH_TYPES_HPP

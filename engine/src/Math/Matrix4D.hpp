#ifndef _FLATEARTH_ENGINE_MATH_MATRIX4D_HPP_
#define _FLATEARTH_ENGINE_MATH_MATRIX4D_HPP_

#include "Defines.hpp"
#include "Math/FeMath.hpp"
#include "Math/Vector3D.hpp"

namespace flatearth::math {

class Mat4D {
public:
  // Identity
  constexpr Mat4D() noexcept
      : _x00(1), _x01(0), _x02(0), _x03(0), _x10(0), _x11(1), _x12(0), _x13(0), _x20(0), _x21(0),
        _x22(1), _x23(0), _x30(0), _x31(0), _x32(0), _x33(1) {}

  constexpr Mat4D(const Mat4D&) noexcept = default;
  constexpr Mat4D& operator=(const Mat4D&) noexcept = default;

  /*==============================
    Static factories
  ==============================*/

  FEINLINE constexpr Mat4D Identity() { return Mat4D(); }

  FEINLINE constexpr Mat4D Translation(float32 tx, float32 ty, float32 tz) {
    Mat4D m;
    m._x03 = tx;
    m._x13 = ty;
    m._x23 = tz;
    return m;
  }

  FEINLINE constexpr Mat4D Scale(float32 sx, float32 sy, float32 sz) {
    Mat4D m;
    m._x00 = sx;
    m._x11 = sy;
    m._x22 = sz;
    return m;
  }

  FEINLINE Mat4D RotationX(float32 rad) {
    Mat4D m;
    float32 c = Cos(rad);
    float32 s = Sin(rad);

    m._x11 = c;
    m._x12 = -s;
    m._x21 = s;
    m._x22 = c;
    return m;
  }

  FEINLINE Mat4D RotationY(float32 rad) {
    Mat4D m;
    float32 c = Cos(rad);
    float32 s = Sin(rad);

    m._x00 = c;
    m._x02 = s;
    m._x20 = -s;
    m._x22 = c;
    return m;
  }

  FEINLINE Mat4D RotationZ(float32 rad) {
    Mat4D m;
    float32 c = Cos(rad);
    float32 s = Sin(rad);

    m._x00 = c;
    m._x01 = -s;
    m._x10 = s;
    m._x11 = c;
    return m;
  }

  FEINLINE Mat4D TRS(Vec3D translation = Vec3D::Zero(),
                     Vec3D scale = Vec3D::One(),
                     Vec3D rotationRad = Vec3D::Zero()) {

    Mat4D t = Translation(translation.x(), translation.y(), translation.z());
    Mat4D rx = RotationX(rotationRad.x());
    Mat4D ry = RotationY(rotationRad.y());
    Mat4D rz = RotationZ(rotationRad.z());
    Mat4D s = Scale(scale.x(), scale.y(), scale.z());

    return t * rz * ry * rx * s;
  }

  /*==============================
    Operators
  ==============================*/

  inline constexpr Mat4D operator*(const Mat4D& o) const noexcept {
    Mat4D result;

    result._x00 = _x00 * o._x00 + _x01 * o._x10 + _x02 * o._x20 + _x03 * o._x30;
    result._x01 = _x00 * o._x01 + _x01 * o._x11 + _x02 * o._x21 + _x03 * o._x31;
    result._x02 = _x00 * o._x02 + _x01 * o._x12 + _x02 * o._x22 + _x03 * o._x32;
    result._x03 = _x00 * o._x03 + _x01 * o._x13 + _x02 * o._x23 + _x03 * o._x33;

    result._x10 = _x10 * o._x00 + _x11 * o._x10 + _x12 * o._x20 + _x13 * o._x30;
    result._x11 = _x10 * o._x01 + _x11 * o._x11 + _x12 * o._x21 + _x13 * o._x31;
    result._x12 = _x10 * o._x02 + _x11 * o._x12 + _x12 * o._x22 + _x13 * o._x32;
    result._x13 = _x10 * o._x03 + _x11 * o._x13 + _x12 * o._x23 + _x13 * o._x33;

    result._x20 = _x20 * o._x00 + _x21 * o._x10 + _x22 * o._x20 + _x23 * o._x30;
    result._x21 = _x20 * o._x01 + _x21 * o._x11 + _x22 * o._x21 + _x23 * o._x31;
    result._x22 = _x20 * o._x02 + _x21 * o._x12 + _x22 * o._x22 + _x23 * o._x32;
    result._x23 = _x20 * o._x03 + _x21 * o._x13 + _x22 * o._x23 + _x23 * o._x33;

    result._x30 = _x30 * o._x00 + _x31 * o._x10 + _x32 * o._x20 + _x33 * o._x30;
    result._x31 = _x30 * o._x01 + _x31 * o._x11 + _x32 * o._x21 + _x33 * o._x31;
    result._x32 = _x30 * o._x02 + _x31 * o._x12 + _x32 * o._x22 + _x33 * o._x32;
    result._x33 = _x30 * o._x03 + _x31 * o._x13 + _x32 * o._x23 + _x33 * o._x33;

    return result;
  }

  inline constexpr Mat4D& operator*=(const Mat4D& o) noexcept {
    *this = *this * o;
    return *this;
  }

  /*==============================
    Transform
  ==============================*/

  inline Vec3D TransformPoint(const Vec3D& p) const noexcept {
    float32 x = _x00 * p.x() + _x01 * p.y() + _x02 * p.z() + _x03;
    float32 y = _x10 * p.x() + _x11 * p.y() + _x12 * p.z() + _x13;
    float32 z = _x20 * p.x() + _x21 * p.y() + _x22 * p.z() + _x23;
    return Vec3D(x, y, z);
  }

  inline Vec3D TransformVector(const Vec3D& v) const noexcept {
    float32 x = _x00 * v.x() + _x01 * v.y() + _x02 * v.z();
    float32 y = _x10 * v.x() + _x11 * v.y() + _x12 * v.z();
    float32 z = _x20 * v.x() + _x21 * v.y() + _x22 * v.z();
    return Vec3D(x, y, z);
  }

private:
  float32 _x00, _x01, _x02, _x03, _x10, _x11, _x12, _x13, _x20, _x21, _x22, _x23, _x30, _x31, _x32,
      _x33;
};

} // namespace flatearth::math

#endif // _FLATEARTH_ENGINE_MATH_MATRIX4D_HPP_

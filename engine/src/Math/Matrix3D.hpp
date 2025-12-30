#ifndef _FLATEARTH_ENGINE_MATH_MATRIX3D_HPP_
#define _FLATEARTH_ENGINE_MATH_MATRIX3D_HPP_

#include "Defines.hpp"
#include "Error.hpp"
#include "Math/FeMath.hpp"
#include "Math/Vector2D.hpp"

namespace flatearth::math {

class Mat3D {
public:
  constexpr Mat3D() noexcept
      : _x00(1.0f), _x01(0.0f), _x02(0.0f), _x10(0.0f), _x11(1.0f), _x12(0.0f),
        _x20(0.0f), _x21(0.0f), _x22(1.0f) {}

  constexpr Mat3D(const Mat3D &other) noexcept
      : _x00(other._x00), _x01(other._x01), _x02(other._x02), _x10(other._x10),
        _x11(other._x11), _x12(other._x12), _x20(other._x20), _x21(other._x21),
        _x22(other._x22) {}

  // Static factories (do not depend on instance)
  FEINLINE constexpr Mat3D Identity() { return Mat3D(); }

  FEINLINE constexpr Mat3D Translation(float32 tx, float32 ty) {
    Mat3D mat;
    mat._x02 = tx;
    mat._x12 = ty;
    return mat;
  }

  FEINLINE constexpr Mat3D Scale(float32 sx, float32 sy) {
    Mat3D mat;
    mat._x00 = sx;
    mat._x11 = sy;
    return mat;
  }

  FEINLINE Mat3D Rotation(float32 angleRad) {
    Mat3D mat;
    float32 c = Cos(angleRad);
    float32 s = Sin(angleRad);
    mat._x00 = c;
    mat._x01 = -s;
    mat._x10 = s;
    mat._x11 = c;
    return mat;
  }

  FEINLINE Mat3D TRS(Vec2D translation = Vec2D::Zero(),
                     Vec2D scale = Vec2D::One(),
                     float32 rotationRad = 0.0f) {
    Mat3D t = Translation(translation.x(), translation.y());
    Mat3D r = Rotation(rotationRad);
    Mat3D s = Scale(scale.x(), scale.y());
    return t * r * s;
  }

  constexpr Mat3D &operator=(const Mat3D &other) noexcept = default;

  inline constexpr Mat3D operator*(const Mat3D &other) const noexcept {
    Mat3D result;
    result._x00 = _x00 * other._x00 + _x01 * other._x10 + _x02 * other._x20;
    result._x01 = _x00 * other._x01 + _x01 * other._x11 + _x02 * other._x21;
    result._x02 = _x00 * other._x02 + _x01 * other._x12 + _x02 * other._x22;

    result._x10 = _x10 * other._x00 + _x11 * other._x10 + _x12 * other._x20;
    result._x11 = _x10 * other._x01 + _x11 * other._x11 + _x12 * other._x21;
    result._x12 = _x10 * other._x02 + _x11 * other._x12 + _x12 * other._x22;

    result._x20 = _x20 * other._x00 + _x21 * other._x10 + _x22 * other._x20;
    result._x21 = _x20 * other._x01 + _x21 * other._x11 + _x22 * other._x21;
    result._x22 = _x20 * other._x02 + _x21 * other._x12 + _x22 * other._x22;

    return result;
  }

  inline constexpr Mat3D &operator*=(const Mat3D &other) noexcept {
    *this = *this * other;
    return *this;
  }

  inline constexpr Mat3D Transpose() const noexcept {
    Mat3D result;
    result._x00 = _x00;
    result._x01 = _x10;
    result._x02 = _x20;

    result._x10 = _x01;
    result._x11 = _x11;
    result._x12 = _x21;

    result._x20 = _x02;
    result._x21 = _x12;
    result._x22 = _x22;
    return result;
  }

  inline FeExpect<Mat3D, Error>
  Inverse(float32 epsilon = FE_F32EPS) const noexcept {
    Mat3D result;
    float32 det = Determinant();

    if (Abs(det) <= epsilon) {
      return FeErr{Error("matrix is not invertible", ErrorType::MathError)};
    }

    float32 invDet = 1.0f / det;

    result._x00 = (_x11 * _x22 - _x12 * _x21) * invDet;
    result._x01 = (_x02 * _x21 - _x01 * _x22) * invDet;
    result._x02 = (_x01 * _x12 - _x02 * _x11) * invDet;

    result._x10 = (_x12 * _x20 - _x10 * _x22) * invDet;
    result._x11 = (_x00 * _x22 - _x02 * _x20) * invDet;
    result._x12 = (_x02 * _x10 - _x00 * _x12) * invDet;

    result._x20 = (_x10 * _x21 - _x11 * _x20) * invDet;
    result._x21 = (_x01 * _x20 - _x00 * _x21) * invDet;
    result._x22 = (_x00 * _x11 - _x01 * _x10) * invDet;

    return result;
  }

  inline float32 Determinant() const noexcept {
    return _x00 * (_x11 * _x22 - _x12 * _x21) -
           _x01 * (_x10 * _x22 - _x12 * _x20) +
           _x02 * (_x10 * _x21 - _x11 * _x20);
  }

  inline Vec2D TransformPoint(const Vec2D &point) const noexcept {
    float32 x = _x00 * point.x() + _x01 * point.y() + _x02;
    float32 y = _x10 * point.x() + _x11 * point.y() + _x12;
    return Vec2D(x, y);
  }

  inline Vec2D TransformVector(const Vec2D &vector) const noexcept {
    float32 x = _x00 * vector.x() + _x01 * vector.y();
    float32 y = _x10 * vector.x() + _x11 * vector.y();
    return Vec2D(x, y);
  }

  inline constexpr bool Equals(const Mat3D &other,
                               float32 epsilon = FE_F32EPS) const noexcept {
    return (Abs(_x00 - other._x00) <= epsilon) &&
           (Abs(_x01 - other._x01) <= epsilon) &&
           (Abs(_x02 - other._x02) <= epsilon) &&

           (Abs(_x10 - other._x10) <= epsilon) &&
           (Abs(_x11 - other._x11) <= epsilon) &&
           (Abs(_x12 - other._x12) <= epsilon) &&

           (Abs(_x20 - other._x20) <= epsilon) &&
           (Abs(_x21 - other._x21) <= epsilon) &&
           (Abs(_x22 - other._x22) <= epsilon);
  }

private:
  float32 _x00, _x01, _x02;
  float32 _x10, _x11, _x12;
  float32 _x20, _x21, _x22;
};

} // namespace flatearth::math

#endif // _FLATEARTH_ENGINE_MATH_MATRIX3D_HPP_

#ifndef _FLATEARTH_ENGINE_MATH_VECTOR3D_HPP_
#define _FLATEARTH_ENGINE_MATH_VECTOR3D_HPP_

#include "Defines.hpp"
#include "Error.hpp"
#include "Math/FeMath.hpp"
#include "Vector2D.hpp"

namespace flatearth::math {

class Vec3D {
public:
  inline constexpr Vec3D() : _x(0.0f), _y(0.0f), _z(0.0f) {}
  inline constexpr Vec3D(float32 x, float32 y, float32 z) : _x(x), _y(y), _z(z) {}
  inline Vec3D(const Vec3D &other) noexcept : _x(other._x), _y(other._y), _z(other._z) {}

  // Static factories (do not depend on instance)
  FEINLINE constexpr Vec3D Zero() { return Vec3D(0.0f, 0.0f, 0.0f); }
  FEINLINE constexpr Vec3D One() { return Vec3D(1.0f, 1.0f, 1.0f); }
  FEINLINE constexpr Vec3D Up() { return Vec3D(0.0f, 1.0f, 0.0f); }
  FEINLINE constexpr Vec3D Right() { return Vec3D(1.0f, 0.0f, 0.0f); }
  FEINLINE constexpr Vec3D Forward() { return Vec3D(0.0f, 0.0f, 1.0f); }
  FEINLINE constexpr Vec3D Down() { return Vec3D(0.0f, -1.0f, 0.0f); }
  FEINLINE constexpr Vec3D Left() { return Vec3D(-1.0f, 0.0f, 0.0f); }
  FEINLINE constexpr Vec3D Backward() { return Vec3D(0.0f, 0.0f, -1.0f); }

  inline constexpr Vec3D operator+(const Vec3D &other) const noexcept {
    return Vec3D(_x + other._x, _y + other._y, _z + other._z);
  }

  inline constexpr Vec3D operator-(const Vec3D &other) const noexcept {
    return Vec3D(_x - other._x, _y - other._y, _z - other._z);
  }

  inline constexpr Vec3D operator*(float32 scalar) const noexcept {
    return Vec3D(_x * scalar, _y * scalar, _z * scalar);
  }

  inline constexpr Vec3D operator*(const Vec3D &other) const noexcept {
    return Vec3D(_x * other._x, _y * other._y, _z * other._z);
  }

  inline const FeExpect<Vec3D, Error> operator/(float32 scalar) const {
    if (scalar == 0.0f) {
      return FeErr{Error("scalar cannot be zero on divide operator", ErrorType::DivisionByZero)};
    }
    return Vec3D(_x / scalar, _y / scalar, _z / scalar);
  }

  inline constexpr Vec3D &operator+=(const Vec3D &other) noexcept {
    _x += other._x;
    _y += other._y;
    _z += other._z;
    return *this;
  }

  inline constexpr Vec3D &operator-=(const Vec3D &other) noexcept {
    _x -= other._x;
    _y -= other._y;
    _z -= other._z;
    return *this;
  }

  inline constexpr Vec3D &operator*=(float32 scalar) noexcept {
    _x *= scalar;
    _y *= scalar;
    _z *= scalar;
    return *this;
  }

  inline FeExpect<Vec3D, Error> Normalize() noexcept {
    if (_x == 0.0f && _y == 0.0f && _z == 0.0f) {
      return FeErr{Error("cannot normalize a zero-length vector", ErrorType::DivisionByZero)};
    }
    float32 length = Length();
    return Vec3D(_x / length, _y / length, _z / length);
  }

  inline float32 Length() const noexcept {
    return static_cast<float32>(Sqrt(_x * _x + _y * _y + _z * _z));
  }

  inline constexpr float32 LengthSqrd() const noexcept { return _x * _x + _y * _y + _z * _z; }

  inline bool Equals(const Vec3D &other, float64 epsilon = FE_F64EPS) const noexcept {
    return (Abs(_x - other._x) < epsilon) && (Abs(_y - other._y) < epsilon) &&
           (Abs(_z - other._z) < epsilon);
  }

  inline constexpr float32 Dot(const Vec3D &other) const noexcept {
    return _x * other._x + _y * other._y + _z * other._z;
  }

  inline constexpr Vec3D Curl(const Vec3D &other) const noexcept {
    return Vec3D(_y * other._z - _z * other._y,
                 _z * other._x - _x * other._z,
                 _x * other._y - _y * other._x);
  }

  inline Vec2D XProjection() const noexcept { return Vec2D(_x, _z); }

  inline Vec2D YProjection() const noexcept { return Vec2D(_y, _z); }

  inline Vec2D ZProjection() const noexcept { return Vec2D(_x, _y); }

  inline constexpr float32 x() const noexcept { return _x; }
  inline constexpr float32 y() const noexcept { return _y; }
  inline constexpr float32 z() const noexcept { return _z; }

private:
  float32 _x, _y, _z;
};

} // namespace flatearth::math

#endif // _FLATEARTH_ENGINE_MATH_VECTOR3D_HPP_

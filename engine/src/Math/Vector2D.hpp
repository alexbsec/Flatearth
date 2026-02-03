#ifndef _FLATEARTH_ENGINE_MATH_VECTOR2D_HPP_
#define _FLATEARTH_ENGINE_MATH_VECTOR2D_HPP_

#include "Error.hpp"
#include "Math/FeMath.hpp"

namespace flatearth::math {

class Vec2D {
public:
  inline constexpr Vec2D() : _x(0.0f), _y(0.0f) {}
  inline constexpr Vec2D(float32 x, float32 y) : _x(x), _y(y) {}
  inline Vec2D(const Vec2D &other) noexcept : _x(other._x), _y(other._y) {}

  // Static factories (do not depend on instance)
  FEINLINE constexpr Vec2D Zero() { return Vec2D(0.0f, 0.0f); }
  FEINLINE constexpr Vec2D One() { return Vec2D(1.0f, 1.0f); }
  FEINLINE constexpr Vec2D Up() { return Vec2D(0.0f, 1.0f); }
  FEINLINE constexpr Vec2D Right() { return Vec2D(1.0f, 0.0f); }
  FEINLINE constexpr Vec2D Down() { return Vec2D(0.0f, -1.0f); }
  FEINLINE constexpr Vec2D Left() { return Vec2D(-1.0f, 0.0f); }

  inline constexpr Vec2D operator+(const Vec2D &other) const noexcept {
    return Vec2D(_x + other._x, _y + other._y);
  }

  inline constexpr Vec2D operator-(const Vec2D &other) const noexcept {
    return Vec2D(_x - other._x, _y - other._y);
  }

  inline constexpr Vec2D operator*(float32 scalar) const noexcept {
    return Vec2D(_x * scalar, _y * scalar);
  }

  inline constexpr Vec2D operator*(const Vec2D &other) const noexcept {
    return Vec2D(_x * other._x, _y * other._y);
  }

  inline const FeExpect<Vec2D, Error> operator/(float32 scalar) const {
    if (scalar == 0.0f) {
      return FeErr{Error("scalar cannot be zero on divide operator", ErrorType::DivisionByZero)};
    }
    return Vec2D(_x / scalar, _y / scalar);
  }

  inline constexpr Vec2D &operator+=(const Vec2D &other) noexcept {
    _x += other._x;
    _y += other._y;
    return *this;
  }

  inline constexpr Vec2D &operator-=(const Vec2D &other) noexcept {
    _x -= other._x;
    _y -= other._y;
    return *this;
  }

  inline constexpr Vec2D &operator*=(float32 scalar) noexcept {
    _x *= scalar;
    _y *= scalar;
    return *this;
  }

  inline FeExpect<Vec2D, Error> Normalize() noexcept {
    if (_x == 0.0f && _y == 0.0f) {
      return FeErr{Error("cannot normalize a zero-length vector", ErrorType::DivisionByZero)};
    }

    const float32 cLength = Length();
    _x /= cLength;
    _y /= cLength;
    return *this;
  }

  inline float32 Length() const noexcept { return static_cast<float32>(Sqrt(_x * _x + _y * _y)); }

  inline constexpr float32 LengthSqrd() const noexcept { return _x * _x + _y * _y; }

  inline constexpr float32 Dot(const Vec2D &other) const noexcept {
    return _x * other._x + _y * other._y;
  }

  inline bool Equals(const Vec2D &other, float64 epsilon = FE_F64EPS) const noexcept {
    return (Abs(_x - other._x) <= epsilon) && (Abs(_y - other._y) <= epsilon);
  }

  inline constexpr float32 x() const noexcept { return _x; }
  inline constexpr float32 y() const noexcept { return _y; }

private:
  float32 _x, _y;
};

} // namespace flatearth::math

#endif // _FLATEARTH_ENGINE_MATH_VECTOR2D_HPP_

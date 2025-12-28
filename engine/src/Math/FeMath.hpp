#ifndef _FLATEARTH_ENGINE_MATH_FEMATH_HPP
#define _FLATEARTH_ENGINE_MATH_FEMATH_HPP

#include "Defines.hpp"
#include <cmath>
namespace flatearth::math {

inline float64 Sqrt(float64 value) {
  return std::sqrtf(value);
}

inline float64 Sin(float64 radians) {
  return std::sinf(radians);
}

inline float64 Cos(float64 radians) {
  return std::cosf(radians);
}

inline float64 Tan(float64 radians) {
  return std::tanf(radians);
}

inline float64 Arcsin(float64 value) {
  return std::asinf(value);
}

inline float64 Arccos(float64 value) {
  return std::acosf(value);
}

inline float64 Arctan(float64 value) {
  return std::atanf(value);
}

inline float64 Sec(float64 radians) {
  return 1.0f / Cos(radians);
}

inline float64 Csc(float64 radians) {
  return 1.0f / Sin(radians);
}

inline float64 Cot(float64 radians) {
  return 1.0f / Tan(radians);
}

inline float64 Abs(float64 value) {
  return std::abs(value);
}

} // namespace flatearth::math

#endif // _FLATEARTH_ENGINE_MATH_FEMATH_HPP

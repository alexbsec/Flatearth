#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_FIELD_TYPE_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_FIELD_TYPE_HPP

#include "Defines.hpp"

namespace flatearth::ecs::reflect {

enum class FieldType : uint8 {
  Null,
  Bool,
  Int8,
  Int16,
  Int32,
  Int64,
  Uint8,
  Uint16,
  Uint32,
  Uint64,
  Float32,
  Float64,
  Vec2D,
  String,
  CString, // fixed-size char array; arraySize holds the buffer capacity
};

constexpr uint8 FieldSize(FieldType t) {
  switch (t) {
    default:
      return 0;
    case FieldType::Bool:
      return 1;
    case FieldType::Int8:
      return 1;
    case FieldType::Int16:
      return 2;
    case FieldType::Int32:
      return 4;
    case FieldType::Int64:
      return 8;
    case FieldType::Uint8:
      return 1;
    case FieldType::Uint16:
      return 2;
    case FieldType::Uint32:
      return 4;
    case FieldType::Uint64:
      return 8;
    case FieldType::Float32:
      return 4;
    case FieldType::Float64:
      return 8;
    case FieldType::Vec2D:
      return 8;
    case FieldType::String:
      return 0; // variable length
    case FieldType::CString:
      return 0; // variable; arraySize in FieldDescriptor holds the capacity
  }
}

constexpr const char *FieldTypeName(FieldType t) {
  switch (t) {
    case FieldType::Null:
      return "null";
    case FieldType::Bool:
      return "bool";
    case FieldType::Int8:
      return "int8";
    case FieldType::Int16:
      return "int16";
    case FieldType::Int32:
      return "int32";
    case FieldType::Int64:
      return "int64";
    case FieldType::Uint8:
      return "uint8";
    case FieldType::Uint16:
      return "uint16";
    case FieldType::Uint32:
      return "uint32";
    case FieldType::Uint64:
      return "uint64";
    case FieldType::Float32:
      return "float32";
    case FieldType::Float64:
      return "float64";
    case FieldType::Vec2D:
      return "vec2d";
    case FieldType::String:
      return "string";
    case FieldType::CString:
      return "cstring";
  }
  return "unknown";
}

} // namespace flatearth::ecs::reflect

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_FIELD_TYPE_HPP

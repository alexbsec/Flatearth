#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_REFLECT_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_REFLECT_HPP

#include "TypeRegistry.hpp"

// ── public API ───────────────────────────────────────────────────────────────
// Usage (outside the struct, at file scope):
//
//   REFLECT_COMPONENT(Transform)
//       FIELD(x,        FieldType::Float32)
//       FIELD(y,        FieldType::Float32)
//       FIELD(rotation, FieldType::Float32)
//   END_REFLECT

#define _FE_CONCAT2(a, b) a##b
#define _FE_CONCAT(a, b)  _FE_CONCAT2(a, b)
#define _FE_UNIQUE(base)  _FE_CONCAT(base, __COUNTER__)

#define REFLECT_COMPONENT(Type)                       \
  namespace {                                         \
  struct _Reflector_##Type {                          \
    _Reflector_##Type() {                             \
      using T = Type;                                 \
      ::flatearth::ecs::reflect::TypeDescriptor desc; \
      desc.name = #Type;                              \
      desc.fields = {

#define FIELD(name, ftype) {#name, static_cast<uint32>(offsetof(T, name)), ftype},

#define END_REFLECT                                                        \
  }                                                                        \
  ;                                                                        \
  ::flatearth::ecs::reflect::TypeRegistry::Register<T>(std::move(desc));  \
  }                                                                        \
  } _FE_UNIQUE(_reflector_instance_);                                       \
  }

#endif

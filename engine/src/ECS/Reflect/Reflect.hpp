#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_REFLECT_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_REFLECT_HPP

#include "Authority.hpp"
#include "TypeRegistry.hpp"

// ── public API ───────────────────────────────────────────────────────────────
// Usage (outside the struct, at file scope):
//
//   REFLECT_COMPONENT(Transform)
//       FIELD(x,        FieldType::Float32)
//       FIELD(y,        FieldType::Float32)
//       FIELD(rotation, FieldType::Float32)
//   END_REFLECT
//
//   REFLECT_COMPONENT(NetPos, Authority::Server)
//       FIELD(x, FieldType::Float32)
//   END_REFLECT

#define _FE_CONCAT2(a, b) a##b
#define _FE_CONCAT(a, b) _FE_CONCAT2(a, b)
#define _FE_UNIQUE(base) _FE_CONCAT(base, __COUNTER__)

#define _REFLECT_COMPONENT_IMPL(Type, Auth)           \
  namespace {                                         \
  struct _Reflector_##Type {                          \
    _Reflector_##Type() {                             \
      using T = Type;                                 \
      ::flatearth::ecs::reflect::TypeDescriptor desc; \
      desc.name = #Type;                              \
      desc.authority = (Auth);                        \
      desc.fields = {

#define _FE_EXPAND(x) x
#define _FE_REFLECT_GET3(_1, _2, NAME, ...) NAME
#define _REFLECT_COMPONENT_2(Type, Auth) _REFLECT_COMPONENT_IMPL(Type, Auth)
#define _REFLECT_COMPONENT_1(Type) \
  _REFLECT_COMPONENT_IMPL(Type, ::flatearth::ecs::reflect::Authority::Local)

#define REFLECT_COMPONENT(...) \
  _FE_EXPAND(_FE_REFLECT_GET3(__VA_ARGS__, _REFLECT_COMPONENT_2, _REFLECT_COMPONENT_1))(__VA_ARGS__)

#define FIELD(name, ftype) {#name, static_cast<uint32>(offsetof(T, name)), ftype},

#define END_REFLECT                                                      \
  }                                                                      \
  ;                                                                      \
  ::flatearth::ecs::reflect::TypeRegistry::Register<T>(std::move(desc)); \
  }                                                                      \
  }                                                                      \
  _FE_UNIQUE(_reflector_instance_);                                      \
  }

#endif

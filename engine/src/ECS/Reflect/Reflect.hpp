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

#define _REFLECT_COMPONENT_IMPL(Type, Auth, ...)      \
  namespace {                                         \
  struct _Reflector_##Type {                          \
    _Reflector_##Type() {                             \
      using T = Type;                                 \
      ::flatearth::ecs::reflect::TypeDescriptor desc; \
      desc.name = #Type;                              \
      desc.authority = (Auth);                        \
      desc.fields = {

// __VA_OPT__ (C++20) avoids MSVC __VA_ARGS__ comma-counting bug.
// If Auth arg is present it wins; otherwise defaults to Local.
#define REFLECT_COMPONENT(Type, ...) \
  _REFLECT_COMPONENT_IMPL(Type, __VA_OPT__(__VA_ARGS__,) ::flatearth::ecs::reflect::Authority::Local)

#define FIELD(name, ftype) {#name, static_cast<uint32>(offsetof(T, name)), ftype},
#define FIELD_CSTR(name, maxlen)                  \
  {#name,                                         \
   static_cast<uint32>(offsetof(T, name)),        \
   ::flatearth::ecs::reflect::FieldType::CString, \
   (maxlen)},

#define END_REFLECT                                                      \
  }                                                                      \
  ;                                                                      \
  ::flatearth::ecs::reflect::TypeRegistry::Register<T>(std::move(desc)); \
  }                                                                      \
  }                                                                      \
  _FE_UNIQUE(_reflector_instance_);                                      \
  }

#endif

#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_SERIALIZE_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_SERIALIZE_HPP

#include "ECS/Reflect/TypeRegistry.hpp"

#include <cstddef>
#include <vector>

namespace flatearth::ecs::reflect {

class Serialize {
public:
  template <typename T>
  static string ToJson(const T &component) {
    const TypeDescriptor *desc = TypeRegistry::Get<T>();
    if (!desc) return "{}";
    return ToJsonRaw(&component, *desc);
  }

  template <typename T>
  static bool FromJson(T &component, stringv json) {
    const TypeDescriptor *desc = TypeRegistry::Get<T>();
    if (!desc) return false;
    return FromJsonRaw(&component, *desc, json);
  }

  template <typename T>
  static std::vector<std::byte> ToBinary(const T &component) {
    const TypeDescriptor *desc = TypeRegistry::Get<T>();
    if (!desc) return {};
    return ToBinaryRaw(&component, *desc);
  }

  template <typename T>
  static bool FromBinary(T &component, const std::vector<std::byte> &data) {
    const TypeDescriptor *desc = TypeRegistry::Get<T>();
    if (!desc) return false;
    return FromBinaryRaw(&component, *desc, data);
  }

private:
  FEAPI static string ToJsonRaw(const void *component, const TypeDescriptor &desc);
  FEAPI static bool FromJsonRaw(void *component, const TypeDescriptor &desc, stringv json);
  FEAPI static std::vector<std::byte> ToBinaryRaw(const void *component, const TypeDescriptor &desc);
  FEAPI static bool FromBinaryRaw(void *component, const TypeDescriptor &desc,
                                  const std::vector<std::byte> &data);
};

} // namespace flatearth::ecs::reflect

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_SERIALIZE_HPP

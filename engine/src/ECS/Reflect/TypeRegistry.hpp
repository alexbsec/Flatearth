#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_REGISTRY_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_REGISTRY_HPP

#include "Defines.hpp"
#include "ECS/ComponentPool.hpp"
#include "ECS/Reflect/TypeDescriptor.hpp"
#include <unordered_map>

namespace flatearth::ecs::reflect {

class TypeRegistry {
public:
  template <typename T>
  static void Register(TypeDescriptor desc) {
    desc.typeId = ComponentTypeId<T>::Value();
    Self()._byId[desc.typeId] = std::move(desc);
    Self()._byName[Self()._byId[desc.typeId].name] = desc.typeId;
  }

  template <typename T>
  static const TypeDescriptor *Get() {
    uint32 id = ComponentTypeId<T>::Value();
    auto it = Self()._byId.find(id);
    return it != Self()._byId.end() ? &it->second : nullptr;
  }

  template <typename T>
  static const TypeDescriptor *Get(stringv name) {
    auto it = Self()._byName.find(name);
    if (it == Self()._byName.end()) {
      return nullptr;
    }
    auto it2 = Self()._byId.find(it->second);
    return it2 != Self()._byId.end() ? &it2->second : nullptr;
  }

public:
  FEINLINE TypeRegistry &Self() {
    static TypeRegistry instance;
    return instance;
  }

private:
  TypeRegistry() = default;

private:
  std::unordered_map<uint32, TypeDescriptor> _byId;
  std::unordered_map<stringv, uint32> _byName;
};

}

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_REGISTRY_HPP

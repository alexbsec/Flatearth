#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP

#include "Authority.hpp"
#include "FieldType.hpp"

#include <vector>

namespace flatearth::ecs::reflect {

struct FieldDescriptor {
  stringv name{""};
  uint32 offset{0};
  FieldType type{FieldType::Null};
  uint32 arraySize{0}; // used by CString to store the buffer capacity
};

struct TypeDescriptor {
  stringv name{""};
  uint32 typeId{0};
  Authority authority{Authority::Local};
  std::vector<FieldDescriptor> fields{};
};

} // namespace flatearth::ecs::reflect

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP

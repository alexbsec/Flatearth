#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP

#include "FieldType.hpp"

#include <vector>

namespace flatearth::ecs::reflect {

struct FieldDescriptor {
  stringv name;
  uint32 offset;
  FieldType type;
};

struct TypeDescriptor {
  stringv name;
  uint32 typeId;
  std::vector<FieldDescriptor> fields;
};

} // namespace flatearth::ecs::reflect

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP

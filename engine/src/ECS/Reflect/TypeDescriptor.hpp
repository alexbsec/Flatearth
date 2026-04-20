#ifndef _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP
#define _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP

#include "FieldType.hpp"
#include "Authority.hpp"

#include <vector>

namespace flatearth::ecs::reflect {

struct FieldDescriptor {
  stringv name{""};
  uint32 offset{0};
  FieldType type{FieldType::Null};
};

struct TypeDescriptor {
  stringv name{""};
  uint32 typeId{0};
  Authority authority{Authority::Local};
  std::vector<FieldDescriptor> fields{};
};

} // namespace flatearth::ecs::reflect

#endif // _FLATEARTH_ENGINE_ECS_REFLECT_TYPE_DESCRIPTOR_HPP

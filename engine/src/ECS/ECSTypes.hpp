#ifndef _FLATEARTH_ENGINE_ECS_ECS_TYPES_HPP
#define _FLATEARTH_ENGINE_ECS_ECS_TYPES_HPP

#include "Defines.hpp"

namespace flatearth::ecs {

using EntityId = uint32;

inline constexpr uint16 IdIndex(EntityId id) {
  return id & 0xffff;
}

inline constexpr uint16 IdGeneration(EntityId id) {
  return (id >> 16) & 0Xffff;
}

inline constexpr uint32 NewEntityId(uint16 index, uint16 generation) {
  return (static_cast<uint32>(generation) << 16) | index;
}

}

#endif // _FLATEARTH_ENGINE_ECS_ECS_TYPES_HPP

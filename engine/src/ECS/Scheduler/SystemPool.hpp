#ifndef _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_POOL_HPP
#define _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_POOL_HPP

#include "Defines.hpp"

namespace flatearth::ecs {

struct SystemTypeIdCounter {
  inline static uint32 sCounter{0};
};

template <typename T>
struct SystemTypeId {
  static uint32 Value() {
    static uint32 id = SystemTypeIdCounter::sCounter++;
    return id;
  }
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_POOL_HPP

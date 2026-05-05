#ifndef _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_POOL_HPP
#define _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_POOL_HPP

#include "Defines.hpp"

#include <typeindex>

namespace flatearth::ecs {

// Counter-based IDs have separate static state per DLL/EXE module on Windows,
// causing ID collisions when engine and game systems are registered together.
// Use type_index::hash_code() instead — stable and cross-module on MSVC/GCC/Clang.
template <typename T>
struct SystemTypeId {
  static uint32 Value() {
    static const uint32 id = static_cast<uint32>(std::type_index(typeid(T)).hash_code());
    return id;
  }
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_POOL_HPP

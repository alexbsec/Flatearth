#ifndef _FLATEARTH_ENGINE_ECS_COMPONENT_POOL_HPP
#define _FLATEARTH_ENGINE_ECS_COMPONENT_POOL_HPP

#include "Containers/SparseSet.hpp"
#include "Core/FeMemory.hpp"

#include <typeindex>

namespace flatearth::ecs {

using ISparseSetBase = containers::ISparseSetBase;

template <typename T>
class SparseSetHolder final : public ISparseSetBase {
public:
  explicit SparseSetHolder(memory::MemoryManager &memManager, uint32 maxEntities)
      : sparseSet(memManager, maxEntities) {}

  void Remove(uint32 entityIndex) override { sparseSet.Remove(entityIndex); }
  uint32 *Entities() override { return sparseSet.Entities(); }
  uint64 Length() const override { return sparseSet.Length(); }

public:
  containers::SparseSet<T> sparseSet;
};

// Counter-based IDs have separate static state per DLL/EXE module.
// Use type_index::hash_code() instead — stable, cross-module, collision-free
// in practice (MSVC/GCC/Clang all hash the mangled type name).
template <typename T>
struct ComponentTypeId {
  static uint32 Value() {
    static const uint32 id = static_cast<uint32>(std::type_index(typeid(T)).hash_code());
    return id;
  }
};

template <typename T>
struct PrefabTypeId {
  static uint32 Value() {
    static const uint32 id = static_cast<uint32>(std::type_index(typeid(T)).hash_code());
    return id;
  }
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_COMPONENT_POOL_HPP

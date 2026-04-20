#ifndef _FLATEARTH_ENGINE_ECS_COMPONENT_POOL_HPP
#define _FLATEARTH_ENGINE_ECS_COMPONENT_POOL_HPP

#include "Containers/SparseSet.hpp"
#include "Core/FeMemory.hpp"

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

struct ComponentTypeIdCounter {
  inline static uint32 sCounter{0};
};

template <typename T>
struct ComponentTypeId {
  static uint32 Value() {
    static uint32 id = ComponentTypeIdCounter::sCounter++;
    return id;
  }
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_COMPONENT_POOL_HPP

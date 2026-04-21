#ifndef _FLATEARTH_ENGINE_ECS_ENTITY_MANAGER_HPP
#define _FLATEARTH_ENGINE_ECS_ENTITY_MANAGER_HPP

#include "Containers/DArray.hpp"
#include "Core/FeMemory.hpp"
#include "ECSTypes.hpp"

namespace flatearth::ecs {

constexpr const uint32 cMaxEntities = 1024;

class EntityManager {
public:
  explicit EntityManager(memory::MemoryManager &memManager);
  EntityId Create();
  void Destroy(EntityId id);
  bool IsAlive(EntityId id);
  uint32 AliveCount() const;

private:
  memory::MemoryManager &_memoryManager;

  std::array<uint16, cMaxEntities> _generations;
  containers::DArray<uint16> _freeList;
  uint32 _aliveCount{0};
};

}; // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_ENTITY_MANAGER_HPP

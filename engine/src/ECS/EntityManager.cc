#include "EntityManager.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "ECS/ECSTypes.hpp"

namespace flatearth::ecs {

EntityManager::EntityManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager), _generations{}, _freeList(memManager) {
  _freeList.Reserve(cMaxEntities);
  for (uint16 i = 1; i < cMaxEntities; i++) {
    _freeList.Push(i);
  }
}

EntityId EntityManager::Create() {
  if (_freeList.Empty()) {
    FLOG_FATAL("max entity count ({}) reached for EntityManager", cMaxEntities);
    // 0 is reserved as null sentinel
    return 0;
  }

  const uint32 indexPos = _freeList.Length() - 1;
  uint16 index = _freeList[indexPos];
  _freeList.Pop();
  _aliveCount++;
  return NewEntityId(index, _generations[index]);
}

void EntityManager::Destroy(EntityId id) {
  if (!IsAlive(id)) {
    FLOG_WARN("destroy called on stale or null EntityId");
    return;
  }

  uint16 index = IdIndex(id);
  _generations[index]++;
  _freeList.Push(index);
  _aliveCount--;
}

bool EntityManager::IsAlive(EntityId id) {
  if (id == 0) {
    return FeFalse;
  }

  uint16 index = IdIndex(id);
  return IdGeneration(id) == _generations[index];
}

uint32 EntityManager::AliveCount() const {
  return _aliveCount;
}

} // namespace flatearth::ecs

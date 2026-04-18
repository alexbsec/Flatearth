#include "Registry.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"

namespace flatearth::ecs {

Registry::Registry(memory::MemoryManager &memManager)
    : _memoryManager(memManager), _entityManager(memManager), _poolsMap(memManager) {}

EntityId Registry::Create() {
  return _entityManager.Create();
}

void Registry::Destroy(EntityId id) {
  if (!_entityManager.IsAlive(id)) {
    FLOG_WARN("destroy called on stale or null EntityId");
    return;
  }

  _poolsMap.ForEach([id](uint32, FePtr<ISparseSetBase> &pPool) {
    pPool->Remove(id);
  });
  _entityManager.Destroy(id);
  FLOG_DEBUG("entity '{}' destroyed", id);
}

} // namespace flatearth::ecs

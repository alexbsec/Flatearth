#include "EntityBuilder.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"

namespace flatearth::ecs {

EntityBuilder::EntityBuilder(memory::MemoryManager &mm, Registry &reg)
  : _registry(reg), _memoryManager(mm), _inserts(mm) {}

EntityBuilder::~EntityBuilder() {
  if (_commited) {
    return;
  }

  FLOG_WARN("EntityBuilder destroyed without Commit() - entity was never created");
  for (uint64 i = 0; i < _inserts.Length(); i++) {
    DeferredInsert &entry = _inserts[i];
    _memoryManager.RawFree(entry.pData, entry.size, memory::Tag::Entity);
  }
}

EntityId EntityBuilder::Commit() {
  EntityId id = _registry.Create();
  CarveEntity(id);
  _commited = FeTrue;
  return id;
}

void EntityBuilder::CarveEntity(EntityId id) {
  for (uint64 i = 0; i < _inserts.Length(); i++) {
    DeferredInsert &entry = _inserts[i];
    entry.fn(_registry, id, entry.pData);
    _memoryManager.RawFree(entry.pData, entry.size, memory::Tag::Entity);
  }
}

}

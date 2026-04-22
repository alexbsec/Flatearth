#include "PrefabManager.hpp"
#include "Core/FeMemory.hpp"

namespace flatearth::assets {

PrefabManager::PrefabManager(memory::MemoryManager &memManager)
  : _prefabsMap(memManager) {}

}

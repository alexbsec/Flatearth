#include "FeMemroy.hpp"
#include "Logger.hpp"


namespace flatearth::memory {
  
MemoryManager::MemoryManager() {
  _memoryState.allocCount = 0;
  _memoryState.memoryBlock = MemoryBlock{
    .totalAllocated = 0,
  };
}

void *MemoryManager::RawAlloc(uint64 size, Tag tag) {
  void *block = std::malloc(size);

  auto tagIndex = static_cast<uint64>(tag);
  if (block == nullptr) {
    FLOG_ERROR("failed to allocate size {} for tag {}",
               size, tagIndex);
    return nullptr;
  }

  _memoryState.memoryBlock.totalAllocated += size;
  _memoryState.memoryBlock.taggedAllocations[tagIndex] += size;
  _memoryState.allocCount++;
  return block;
}

void MemoryManager::RawFree(void *block, uint64 size, Tag tag) {
  if (block == nullptr) {
    return;
  }

  auto tagIndex = static_cast<uint64>(tag);

  _memoryState.memoryBlock.totalAllocated -= size;
  _memoryState.memoryBlock.taggedAllocations[tagIndex] -= size;
  _memoryState.allocCount--;

  std::free(block);
  return;
}

}

#include "FeMemroy.hpp"
#include "Logger.hpp"


namespace flatearth::memory {
  
MemoryManager::MemoryManager() {
  _memoryState.allocCount = 0;
  _memoryState.memoryBlock.totalAllocated = 0;
  _memoryState.memoryBlock.taggedAllocations.fill(0);
}

void *MemoryManager::RawAlloc(uint64 size, uint64 alignment, Tag tag) {
  void *block = nullptr;
#if defined(_MSC_VER)
  block = _aligned_malloc(size, alignment);
#else
  if (posix_memalign(&block, alignment, size) != 0) {
    block = nullptr;
  }
#endif

  if (!block) {
    FLOG_ERROR("failed to allocate aligned size {} (align {}) for tag {}",
               size, alignment, static_cast<uint64>(tag));
    return nullptr;
  }

  _memoryState.memoryBlock.totalAllocated += size;
  _memoryState.memoryBlock.taggedAllocations[static_cast<uint64>(tag)] += size;
  _memoryState.allocCount++;

  return block;
}

void MemoryManager::RawFree(void *block, uint64 size, Tag tag) {
  if (!block) return;

  auto tagIndex = static_cast<uint64>(tag);

#ifdef FE_DEBUG
  assert(_memoryState.memoryBlock.totalAllocated >= size);
  assert(_memoryState.memoryBlock.taggedAllocations[tagIndex] >= size);
  assert(_memoryState.allocCount > 0);
#endif

  _memoryState.memoryBlock.totalAllocated -= size;
  _memoryState.memoryBlock.taggedAllocations[tagIndex] -= size;
  _memoryState.allocCount--;

#if defined(_MSC_VER)
  _aligned_free(block);
#else
  std::free(block);
#endif
}

}

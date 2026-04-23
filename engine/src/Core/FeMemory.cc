#include "FeMemory.hpp"

#include "Core/Logger.hpp"

#include <cstring>

namespace flatearth::memory {

static inline uint64 NormalizeAlignment(uint64 alignment) {
  const uint64 minAlign = static_cast<uint64>(sizeof(void *)); // 8 on x64
  if (alignment < minAlign)
    alignment = minAlign;

  // ensure power of two
  if ((alignment & (alignment - 1)) != 0) {
    uint64 p = 1;
    while (p < alignment)
      p <<= 1;
    alignment = p;
  }
  return alignment;
}


struct MemFmt {
  uint64 bytes;
  double kb;
  double mb;
};

inline MemFmt FormatBytes(uint64 bytes) {
  return {
      bytes,
      bytes / 1024.0,
      bytes / (1024.0 * 1024.0),
  };
}

MemoryManager::MemoryManager() {
  _memoryState.allocCount = 0;
  _memoryState.memoryBlock.totalAllocated = 0;
  _memoryState.memoryBlock.taggedAllocations.fill(0);
}

MemoryManager::~MemoryManager() {
  FLOG_INFO("memory manager exited gracefully");
}

void *MemoryManager::RawAlloc(uint64 size, uint64 alignment, Tag tag) {
  void *block = nullptr;
  alignment = NormalizeAlignment(alignment);

#if defined(_MSC_VER)
  block = _aligned_malloc(size, alignment);
#else
  const int rc = posix_memalign(&block, static_cast<size_t>(alignment), static_cast<size_t>(size));
  if (rc != 0) {
    block = nullptr;
    FLOG_ERROR("posix_memalign failed: rc={} ({}) size={} align={} tag={}",
               rc,
               std::strerror(rc),
               size,
               alignment,
               static_cast<uint64>(tag));
  }
#endif

  if (!block) {
#if defined(_MSC_VER)
    FLOG_ERROR("failed to allocate aligned size {} (align {}) for tag {}",
               size,
               alignment,
               static_cast<uint64>(tag));
#endif
    return nullptr;
  }

  _memoryState.memoryBlock.totalAllocated += size;
  _memoryState.memoryBlock.taggedAllocations[static_cast<uint64>(tag)] += size;
  _memoryState.allocCount++;
  return block;
}

void MemoryManager::RawFree(void *block, uint64 size, Tag tag) {
  if (!block)
    return;

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

void MemoryManager::FZeroMemory(void *block, uint64 size) {
  if (block == nullptr || size == 0) {
    return;
  }

  std::memset(block, 0, size);
}

void MemoryManager::FCopyMemory(void *dst, const void *src, uint64 size) {
  if (src == nullptr || dst == nullptr || size == 0) {
    return;
  }

  std::memcpy(dst, src, size);
}

void MemoryManager::MemoryUsage() {
  std::println("========== Memory Usage ==========");

  auto total = FormatBytes(_memoryState.memoryBlock.totalAllocated);

  std::println(
      "Total Allocated : {} bytes ({:.2f} KB | {:.2f} MB)", total.bytes, total.kb, total.mb);

  std::println("Active Allocs   : {}", _memoryState.allocCount);

  for (uint64 i = 0; i < MaxTags; i++) {
    uint64 bytes = _memoryState.memoryBlock.taggedAllocations[i];
    if (bytes == 0) {
      continue; // skip empty tags
    }

    auto fmt = FormatBytes(bytes);
    Tag tag = static_cast<Tag>(i);

    std::println(
        "  [{}] {} bytes ({:.2f} KB | {:.2f} MB)", TagName(tag), fmt.bytes, fmt.kb, fmt.mb);
  }

  std::println("==================================");
}

const char *TagName(flatearth::memory::Tag tag) {
  using flatearth::memory::Tag;
  switch (tag) {
    case Tag::Unknown:
      return "Unknown";
    case Tag::Array:
      return "Array";
    case Tag::DArray:
      return "DArray";
    case Tag::Dictionary:
      return "Dictionary";
    case Tag::Queue:
      return "Queue";
    case Tag::RingQueue:
      return "RingQueue";
    case Tag::BST:
      return "BST";
    case Tag::HashSet:
      return "HashSet";
    case Tag::Application:
      return "Application";
    case Tag::Job:
      return "Job";
    case Tag::Texture:
      return "Texture";
    case Tag::MaterialInstance:
      return "MaterialInstance";
    case Tag::Renderer:
      return "Renderer";
    case Tag::Game:
      return "Game";
    case Tag::Platform:
      return "Platform";
    case Tag::Transform:
      return "Transform";
    case Tag::Entity:
      return "Entity";
    case Tag::EntityNode:
      return "EntityNode";
    case Tag::Scene:
      return "Scene";
    case Tag::HashMap:
      return "HashMap";
    default:
      return "Invalid";
  }
}

} // namespace flatearth::memory

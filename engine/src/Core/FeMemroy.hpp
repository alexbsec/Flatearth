#ifndef _FLATEARTH_ENGINE_CORE_FE_MEMORY_HPP
#define _FLATEARTH_ENGINE_CORE_FE_MEMORY_HPP

#include "Defines.hpp"
#include <array>
#include <functional>
#include <memory>

namespace flatearth::memory {

enum class Tag : uint64 {
  Unknown,
  Array,
  DArray,
  Dictionary,
  RingQueue,
  BST,
  Application,
  Job,
  Texture,
  MaterialInstance,
  Renderer,
  Game,
  Platform,
  Transform,
  Entity,
  EntityNode,
  Scene,
  MaxTags,
};

constexpr uint64 MaxTags = static_cast<uint64>(Tag::MaxTags);

struct MemoryBlock {
  uint64 totalAllocated;
  std::array<uint64, MaxTags> taggedAllocations;
};

struct SystemState {
  MemoryBlock memoryBlock;
  uint64 allocCount;
};

class MemoryManager {
public:
  MemoryManager();

  void *RawAlloc(uint64 size, Tag tag);
  void RawFree(void *block, uint64 size, Tag tag);

  // template API
public:
  template <typename T, typename... Args>
  inline FePtr<T> Allocate(Tag tag, Args &&...args) {
    void *raw = RawAlloc(sizeof(T), tag);

    if (raw == nullptr) {
      return std::move(FePtr<T>(nullptr, nullptr));
    }

    T *object = new (raw) T(std::forward<Args>(args)...);
    auto deleter = std::function<void(T *)>([this, tag](T *ptr) {
      if (ptr == nullptr) {
        // nothing to de-allocated
        return;
      }
      ptr->~T();
      RawFree(ptr, sizeof(T), tag);
    });

    return std::move(FePtr<T>(object, deleter));
  }

private:
  SystemState _memoryState{};
};

} // namespace flatearth::memory

#endif // _FLATEARTH_ENGINE_CORE_FE_MEMORY_HPP

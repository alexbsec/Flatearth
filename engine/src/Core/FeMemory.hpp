#ifndef _FLATEARTH_ENGINE_CORE_FE_MEMORY_HPP
#define _FLATEARTH_ENGINE_CORE_FE_MEMORY_HPP

#include "Defines.hpp"

#include <array>
#include <functional>

namespace flatearth::memory {

enum class Tag : uint64 {
  Unknown,
  Array,
  DArray,
  Dictionary,
  Queue,
  RingQueue,
  BST,
  HashSet,
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
  Lo,
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
  ~MemoryManager();

  void *RawAlloc(uint64 size, uint64 alignment, Tag tag);
  void RawFree(void *block, uint64 size, Tag tag);
  void FZeroMemory(void *block, uint64 size);
  void CopyMemory(void *dst, const void *src, uint64 size);

  template <typename T, typename... Args>
  inline FePtr<T> Allocate(Tag tag, Args &&...args) {
    void *raw = RawAlloc(sizeof(T), alignof(T), tag);

    if (raw == nullptr) {
      return FePtr<T>(nullptr, nullptr);
    }

    T *object = new (raw) T(std::forward<Args>(args)...);

    auto deleter = std::function<void(T *)>([this, tag](T *ptr) {
      if (!ptr)
        return;
      ptr->~T();
      RawFree(ptr, sizeof(T), tag);
    });

    return FePtr<T>(object, deleter);
  }

  template <typename Base, typename Derived, typename... Args>
  inline FePtr<Base> Allocate(Tag tag, Args &&...args) {
    FePtr<Derived> concrete = Allocate<Derived>(tag, args...);
    auto baseDeleter = [deleter = concrete.get_deleter()](Base *ptr) {
      deleter(FeCast<Derived>(ptr));
    };

    return FePtr<Base>(FeCast<Base>(concrete.release()), std::move(baseDeleter));
  }

  void MemoryUsage();

private:
  SystemState _memoryState{};
};

} // namespace flatearth::memory

#endif // _FLATEARTH_ENGINE_CORE_FE_MEMORY_HPP

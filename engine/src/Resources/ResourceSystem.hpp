#ifndef _FLATEARTH_ENGINE_RESOURCES_RESOURCE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_RESOURCES_RESOURCE_SYSTEM_HPP

#include "Containers/DArray.hpp"
#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Error.hpp"

namespace flatearth::resources {

template <typename TResource>
class ResourceSystem {
public:
  void Release(uint32 handle) {
    Entry *pEntry = _handleEntryMap.Retrieve(handle);
    if (pEntry == nullptr) {
      FLOG_WARN("no entry found for handle {}", handle);
      return;
    }

    if (pEntry->refCount >= 1) {
      pEntry->refCount--;
    }

    if (pEntry->refCount != 0) {
      return;
    }

    if (auto res = Destroy(&pEntry->resource); !res.has_value()) {
      FLOG_FATAL("failed to destroy resource '{}' — GPU resource leaked", pEntry->name);
    }

    _nameHandleMap.Erase(pEntry->name);
    _handleEntryMap.Erase(handle);
  }

  TResource *Get(uint32 handle) {
    Entry *pEntry = _handleEntryMap.Retrieve(handle);
    if (pEntry == nullptr) {
      FLOG_WARN("no entry found for handle {}", handle);
      return nullptr;
    }
    return &pEntry->resource;
  }

  void Shutdown() {
    for (uint64 i = 0; i < _activeHandles.Length(); i++) {
      uint32 handle = _activeHandles[i];
      Entry *pEntry = _handleEntryMap.Retrieve(handle);
      if (pEntry == nullptr) {
        continue;
      }

      if (pEntry->refCount > 0) {
        FLOG_WARN("ResourceSystem::Shutdown — '{}' still has {} reference(s), force destroying",
                  pEntry->name,
                  pEntry->refCount);
      }

      if (auto res = Destroy(&pEntry->resource); !res.has_value()) {
        FLOG_FATAL("ResourceSystem::Shutdown — failed to destroy '{}'", pEntry->name);
      }

      _nameHandleMap.Erase(pEntry->name);
      _handleEntryMap.Erase(handle);
    }

    _activeHandles.Clear();
  }

protected:
  explicit ResourceSystem(memory::MemoryManager &memoryManager)
      : _memoryManager(memoryManager), _nameHandleMap(memoryManager),
        _handleEntryMap(memoryManager), _activeHandles(memoryManager) {}

  virtual ~ResourceSystem() = default;

  // Called by each derived system's own public Acquire method
  FeExpect<uint32, Error> ProtectedAcquire(const string &name) {
    uint32 *pHandle = _nameHandleMap.Retrieve(name);
    if (pHandle != nullptr) {
      Entry *pEntry = _handleEntryMap.Retrieve(*pHandle);
      if (pEntry == nullptr) {
        FLOG_ERROR("handle {} exists for a nullptr entry", *pHandle);
        return FeErr{Error("handle exists but represents no entry", ErrorType::LogicalError)};
      }
      pEntry->refCount++;
      return *pHandle;
    }

    Entry entry{};
    entry.name = name;
    entry.refCount = 1;

    if (auto res = Create(&entry.resource, _nextHandle, name); !res.has_value()) {
      FLOG_ERROR("failed to create resource '{}'", name);
      return FeErr{res.error()};
    }

    if (auto res = _nameHandleMap.Insert(name, _nextHandle); !res.has_value()) {
      auto _ = Destroy(&entry.resource);
      FLOG_ERROR("failed to cache name->handle for '{}'", name);
      return FeErr{res.error()};
    }

    if (auto res = _handleEntryMap.Insert(_nextHandle, entry); !res.has_value()) {
      _nameHandleMap.Erase(name);
      auto _ = Destroy(&entry.resource);
      FLOG_ERROR("failed to cache handle->entry for '{}'", name);
      return FeErr{res.error()};
    }

    _activeHandles.Push(_nextHandle);
    return _nextHandle++;
  }

protected:
  virtual FeExpect<void, Error> Create(TResource *pResource, uint32 handle, const string &name) = 0;
  virtual FeExpect<void, Error> Destroy(TResource *pResource) = 0;

protected:
  struct Entry {
    string name{};
    TResource resource{};
    uint32 refCount{0};
  };

  memory::MemoryManager &_memoryManager;

private:
  containers::HashMap<string, uint32> _nameHandleMap;
  containers::HashMap<uint32, Entry> _handleEntryMap;
  containers::DArray<uint32> _activeHandles;
  uint32 _nextHandle{0};
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_RESOURCE_SYSTEM_HPP

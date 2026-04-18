#ifndef _FLATEARTH_ENGINE_ECS_REGISTRY_HPP
#define _FLATEARTH_ENGINE_ECS_REGISTRY_HPP

#include "Containers/HashMap.hpp"
#include "Containers/SparseSet.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/EntityManager.hpp"

namespace flatearth::ecs {

class ISparseSetBase {
public:
  virtual ~ISparseSetBase() = default;
  virtual void Remove(uint32 entityIndex) = 0;
};

template <typename T>
class SparseSetHolder final : public ISparseSetBase {
public:
  explicit SparseSetHolder(memory::MemoryManager &memManager, uint32 maxEntities)
      : sparseSet(memManager, maxEntities) {}

  void Remove(uint32 entityIndex) override { sparseSet.Remove(entityIndex); };

public:
  containers::SparseSet<T> sparseSet;
};

struct ComponentTypeIdCounter {
  inline static uint32 sCounter{0};
};

template <typename T>
struct ComponentTypeId {
  static uint32 Value() {
    static uint32 id = ComponentTypeIdCounter::sCounter++;
    return id;
  }
};

class Registry {
public:
  explicit Registry(memory::MemoryManager &memManager);

  EntityId Create();
  void Destroy(EntityId id);

  template <typename T>
  void Insert(EntityId id, const T &component) {
    if (!_entityManager.IsAlive(id)) {
      return;
    }

    containers::SparseSet<T> *pSet = GetPool<T>();
    pSet->Insert(IdIndex(id), component);
  }

  template <typename T>
  void Remove(EntityId id) {
    if (!_entityManager.IsAlive(id)) {
      return;
    }

    containers::SparseSet<T> *pSet = GetPool<T>();
    pSet->Remove(id);
  }

  template <typename T>
  T &Get(EntityId id) {
    containers::SparseSet<T> *pSet = GetPool<T>();
    return pSet->Get(id);
  }

  template <typename T>
  containers::SparseSet<T> *GetPool() {
    uint32 typeId = ComponentTypeId<T>::Value();
    FePtr<ISparseSetBase> *ppBase = _poolsMap.Retrieve(typeId);
    if (ppBase != nullptr) {
      return &static_cast<SparseSetHolder<T> *>(ppBase->get())->sparseSet;
    }

    FePtr<SparseSetHolder<T>> pHolder = _memoryManager.Allocate<ISparseSetBase, SparseSetHolder<T>>(
        memory::Tag::Entity, _memoryManager, cMaxEntities);

    containers::SparseSet<T> *pSet = &static_cast<SparseSetHolder<T> *>(pHolder.get())->sparseSet;
    auto _ = _poolsMap.Insert(typeId, std::move(pHolder));
    return pSet;
  }

  template <typename T>
  bool Has(EntityId id) const {
    uint32 typeId = ComponentTypeId<T>::Value();
    const FePtr<ISparseSetBase> *ppBase = _poolsMap.Retrieve(typeId);
    if (ppBase == nullptr) return false;
    return static_cast<const SparseSetHolder<T> *>(ppBase->get())->sparseSet.Has(id);
  }

private:
  memory::MemoryManager &_memoryManager;
  EntityManager _entityManager;
  containers::HashMap<uint32, FePtr<ISparseSetBase>> _poolsMap;
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_REGISTRY_HPP

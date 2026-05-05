#ifndef _FLATEARTH_ENGINE_ECS_REGISTRY_HPP
#define _FLATEARTH_ENGINE_ECS_REGISTRY_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/ComponentPool.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/View.hpp"

namespace flatearth::ecs {

class EntityBuilder;

class Registry {
public:
  FEAPI explicit Registry(memory::MemoryManager &memManager);

  FEAPI EntityId Create();
  void Destroy(EntityId id);
  FEAPI EntityBuilder Spawn();

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
    pSet->Remove(IdIndex(id));
  }

  template <typename T>
  T &Get(EntityId id) {
    containers::SparseSet<T> *pSet = GetPool<T>();
    return pSet->Get(IdIndex(id));
  }

  template <typename T>
  T *TryGet(EntityId id) {
    if (!Has<T>(id)) {
      return nullptr;
    }
    return &Get<T>(id);
  }

  template <typename T>
  containers::SparseSet<T> *GetPool() {
    uint32 typeId = ComponentTypeId<T>::Value();
    FePtr<ISparseSetBase> *ppBase = _poolsMap.Retrieve(typeId);
    if (ppBase != nullptr) {
      return &static_cast<SparseSetHolder<T> *>(ppBase->get())->sparseSet;
    }

    FePtr<ISparseSetBase> pHolder = _memoryManager.Allocate<ISparseSetBase, SparseSetHolder<T>>(
        memory::Tag::Entity, _memoryManager, cMaxEntities);

    containers::SparseSet<T> *pSet = &static_cast<SparseSetHolder<T> *>(pHolder.get())->sparseSet;
    auto _ = _poolsMap.Insert(typeId, std::move(pHolder));
    return pSet;
  }

  template <typename T>
  bool Has(EntityId id) const {
    uint32 typeId = ComponentTypeId<T>::Value();
    const FePtr<ISparseSetBase> *ppBase = _poolsMap.Retrieve(typeId);
    if (ppBase == nullptr)
      return false;
    return static_cast<const SparseSetHolder<T> *>(ppBase->get())->sparseSet.Has(IdIndex(id));
  }

  template <typename... Ts>
  View<Ts...> ViewOf() {
    return View<Ts...>(GetPool<Ts>()...);
  }

  uint32 AliveCount() const { return _entityManager.AliveCount(); }

private:
  memory::MemoryManager &_memoryManager;
  EntityManager _entityManager;
  containers::HashMap<uint32, FePtr<ISparseSetBase>> _poolsMap;
};

} // namespace flatearth::ecs

// Deferred include: EntityBuilder uses Registry, Registry returns EntityBuilder from Spawn().
// Registry must be fully defined before EntityBuilder.hpp is processed.
#include "ECS/EntityBuilder.hpp"

#endif // _FLATEARTH_ENGINE_ECS_REGISTRY_HPP

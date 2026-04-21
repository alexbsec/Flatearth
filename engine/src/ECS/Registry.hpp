#ifndef _FLATEARTH_ENGINE_ECS_REGISTRY_HPP
#define _FLATEARTH_ENGINE_ECS_REGISTRY_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/ComponentPool.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/View.hpp"

namespace flatearth::ecs {

class Registry {
public:
  FEAPI explicit Registry(memory::MemoryManager &memManager);

  FEAPI EntityId Create();
  void Destroy(EntityId id);

  template <typename T>
  FEAPI void Insert(EntityId id, const T &component) {
    if (!_entityManager.IsAlive(id)) {
      return;
    }

    containers::SparseSet<T> *pSet = GetPool<T>();
    pSet->Insert(IdIndex(id), component);
  }

  template <typename T>
  FEAPI void Remove(EntityId id) {
    if (!_entityManager.IsAlive(id)) {
      return;
    }

    containers::SparseSet<T> *pSet = GetPool<T>();
    pSet->Remove(id);
  }

  template <typename T>
  FEAPI T &Get(EntityId id) {
    containers::SparseSet<T> *pSet = GetPool<T>();
    return pSet->Get(id);
  }

  template <typename T>
  FEAPI containers::SparseSet<T> *GetPool() {
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
  FEAPI bool Has(EntityId id) const {
    uint32 typeId = ComponentTypeId<T>::Value();
    const FePtr<ISparseSetBase> *ppBase = _poolsMap.Retrieve(typeId);
    if (ppBase == nullptr) return false;
    return static_cast<const SparseSetHolder<T> *>(ppBase->get())->sparseSet.Has(id);
  }

  template <typename ...Ts>
  FEAPI View<Ts...> ViewOf() {
    return View<Ts...>(GetPool<Ts>()...);
  }

private:
  memory::MemoryManager &_memoryManager;
  EntityManager _entityManager;
  containers::HashMap<uint32, FePtr<ISparseSetBase>> _poolsMap;
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_REGISTRY_HPP

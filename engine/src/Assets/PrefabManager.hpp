#ifndef _FLATEARTH_ENGINE_ASSETS_PREFAB_MANAGER_HPP
#define _FLATEARTH_ENGINE_ASSETS_PREFAB_MANAGER_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/ComponentPool.hpp"
#include "ECS/Registry.hpp"

namespace flatearth::assets {

class PrefabManager {
public:
  using RegisterFn = std::function<void(ecs::EntityId, ecs::Registry &)>;

public:
  explicit PrefabManager(memory::MemoryManager &);

public:
  template <typename Tag>
  FEAPI void Register(RegisterFn fn) {
    _prefabsMap.Insert(ecs::PrefabTypeId<Tag>::Value(), std::move(fn));
  }

  template <typename Tag>
  FEAPI FeExpect<ecs::EntityId, Error> Spawn(ecs::Registry &reg) const {
    const auto *pFn = _prefabsMap.Retrieve(ecs::PrefabTypeId<Tag>::Value());
    if (pFn == nullptr) {
      return FeErr{Error("cannot spawn unknown entity prefab", ErrorType::NullptrException)};
    }

    ecs::EntityId id = reg.Create();
    (*pFn)(id, reg);
    return id;
  }

private:
  containers::HashMap<uint32, RegisterFn> _prefabsMap;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_PREFAB_MANAGER_HPP

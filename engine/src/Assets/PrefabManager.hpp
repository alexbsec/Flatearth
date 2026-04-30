#ifndef _FLATEARTH_ENGINE_ASSETS_PREFAB_MANAGER_HPP
#define _FLATEARTH_ENGINE_ASSETS_PREFAB_MANAGER_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/EntityBuilder.hpp"
#include "ECS/Registry.hpp"
#include <Scene/Scene.hpp>

namespace flatearth::assets {

class PrefabManager {
public:
  using SetupFn = std::function<void(ecs::EntityBuilder &)>;

public:
  explicit PrefabManager(memory::MemoryManager &);

public:
  template <typename Tag>
  FEAPI void Register(SetupFn fn) {
    _prefabsMap.Insert(ecs::PrefabTypeId<Tag>::Value(), std::move(fn));
  }

  template <typename Tag>
  FEAPI FeExpect<ecs::EntityId, Error> Spawn(ecs::Registry &reg, scene::SceneId sceneId) const {
    const auto *pFn = _prefabsMap.Retrieve(ecs::PrefabTypeId<Tag>::Value());
    if (pFn == nullptr) {
      return FeErr{Error("cannot spawn unknown entity prefab", ErrorType::NullptrException)};
    }

    ecs::EntityBuilder builder = reg.Spawn();
    (*pFn)(builder);
    return builder.OwnedBy(sceneId).Commit();
  }

private:
  containers::HashMap<uint32, SetupFn> _prefabsMap;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_PREFAB_MANAGER_HPP

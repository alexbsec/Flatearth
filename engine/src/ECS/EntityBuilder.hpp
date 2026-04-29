#ifndef _FLATEARTH_ENGINE_ECS_ENTITY_BUILDER_HPP
#define _FLATEARTH_ENGINE_ECS_ENTITY_BUILDER_HPP

#include "Containers/DArray.hpp"
#include "Core/FeMemory.hpp"
#include "ECS/ECSTypes.hpp"
#include "Scene/Scene.hpp"
#include "ECS/Registry.hpp"

namespace flatearth::ecs {

class EntityBuilder {
public:
  explicit EntityBuilder(memory::MemoryManager &, Registry &);
  ~EntityBuilder();

  EntityId Commit();

  template <typename T>
  EntityBuilder &With(T component) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "EntityBuilder::With<T> - T must be trivially copyable");

    void *pData = _memoryManager.RawAlloc(sizeof(T), alignof(T), memory::Tag::Entity);
    _memoryManager.FCopyMemory(pData, &component, sizeof(T));

    _inserts.Push(DeferredInsert{
      [](Registry &reg, EntityId id, const void *data) {
        reg.Insert(id, *static_cast<const T *>(data));
      },
      pData,
      sizeof(T),
    });

    return *this;
  }

  EntityBuilder &OwnedBy(scene::SceneId id) { return With(scene::SceneOwnership{id}); }

private:
  using InsertFn = void (*)(Registry &, EntityId, const void *);

  struct DeferredInsert {
    InsertFn fn{};
    void *pData{nullptr};
    uint64 size{0};
  };

private:
  void CarveEntity(EntityId id);

private:
  Registry &_registry;
  memory::MemoryManager &_memoryManager;
  containers::DArray<DeferredInsert> _inserts;
  bool _commited{FeFalse};
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_ENTITY_BUILDER_HPP

#include "Scene/Systems/TransformSystem.hpp"

#include "Scene/Components/Children.hpp"
#include "Scene/Components/Transform2D.hpp"

namespace flatearth::systems {

using scene::Children;
using scene::Transform2D;

TransformSystem::TransformSystem(memory::MemoryManager &memManager) : _memoryManager(memManager) {
}

void TransformSystem::Update(ecs::Registry &registry, float32 deltaTime) {
  ecs::View<Transform2D> view = registry.ViewOf<Transform2D>();

  for (auto [entity, transform] : view) {
    if (!transform.dirty) {
      continue;
    }

    if (!registry.Has<Children>(entity)) {
      continue;
    }

    Children &children = registry.Get<Children>(entity);
    for (uint8 i = 0; i < children.count; i++) {
      ecs::EntityId child = children.ids[i];
      if (registry.Has<Transform2D>(child)) {
        registry.Get<Transform2D>(child).dirty = FeTrue;
      }
    }
  }
}

} // namespace flatearth::systems

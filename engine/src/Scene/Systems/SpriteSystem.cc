#include "SpriteSystem.hpp"
#include "Scene/Components/Children.hpp"
#include "Scene/Components/Sprite.hpp"

namespace flatearth::systems {

void SpriteSystem::Update(ecs::Registry &registry, float32 deltaTime) {
  using namespace scene;
  ecs::View<Sprite> view = registry.ViewOf<Sprite>();

  for (auto [entity, sprite] : view) {
    if (!sprite.dirty) {
      continue;
    }

    if (!registry.Has<Children>(entity)) {
      continue;
    }

    Children &children = registry.Get<Children>(entity);
    for (uint8 i = 0; i < children.count; i++) {
      ecs::EntityId child = children.ids[i];
      if (registry.Has<Sprite>(child)) {
        registry.Get<Sprite>(child).dirty = FeTrue;
      }
    }
  }
}

}

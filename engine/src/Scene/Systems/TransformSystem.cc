#include "Scene/Systems/TransformSystem.hpp"

#include "Containers/HashSet.hpp"
#include "Core/Logger.hpp"
#include "ECS/ECSTypes.hpp"
#include "Math/FeMath.hpp"
#include "Scene/Components/Children.hpp"
#include "Scene/Components/Transform2D.hpp"

namespace flatearth::systems {

using ecs::cNullEntity;
using scene::Children;
using scene::Transform2D;

TransformSystem::TransformSystem(memory::MemoryManager &memManager) : _memoryManager(memManager) {
}

void TransformSystem::Update(ecs::Registry &registry, float32) {
  ecs::View<Transform2D> view = registry.ViewOf<Transform2D>();

  containers::HashSet<ecs::EntityId> matusalemSet(_memoryManager);
  for (auto [entity, transform] : view) {
    if (!transform.dirty) {
      continue;
    }

    if (transform.parent != cNullEntity) {
      ecs::EntityId antecessorId = PropagateTransformBackwards(registry, transform.parent);
      if (matusalemSet.Has(antecessorId)) {
        continue;
      }

      auto res = matusalemSet.Insert(antecessorId);
      if (!res.has_value()) {
        FLOG_ERROR("failed to insert entity '{}' into antecessor's (matusalem) map", antecessorId);
      }
      continue;
    }

    auto _ = matusalemSet.Insert(entity);
  }

  matusalemSet.ForEach([&](const ecs::EntityId &id) {
    if (!registry.Has<Transform2D>(id)) {
      return;
    }

    auto &transform = registry.Get<Transform2D>(id);
    PropagateTransformForward(registry, id, transform, nullptr);
  });
}

ecs::EntityId TransformSystem::PropagateTransformBackwards(ecs::Registry &registry,
                                                           ecs::EntityId parent) {
  if (!registry.Has<Transform2D>(parent)) {
    FLOG_ERROR("parent of transform has no Transform2D component");
    return cNullEntity;
  }

  Transform2D &parentT = registry.Get<Transform2D>(parent);
  parentT.dirty = FeTrue;
  if (parentT.parent != cNullEntity) {
    return PropagateTransformBackwards(registry, parentT.parent);
  }

  return parent;
}

void TransformSystem::PropagateTransformForward(ecs::Registry &registry,
                                                ecs::EntityId entity,
                                                Transform2D &transform,
                                                const Transform2D *pParentTransform) {
  if (pParentTransform != nullptr) {
    float32 cos = math::Cos(pParentTransform->worldRotation);
    float32 sin = math::Sin(pParentTransform->worldRotation);

    transform.worldX = pParentTransform->worldX +
                       pParentTransform->worldScaleX * (cos * transform.x - sin * transform.y);
    transform.worldY = pParentTransform->worldY +
                       pParentTransform->worldScaleY * (sin * transform.x + cos * transform.y);
    transform.worldRotation = pParentTransform->worldRotation + transform.rotation;
    transform.worldScaleX = pParentTransform->worldScaleX * transform.scaleX;
    transform.worldScaleY = pParentTransform->worldScaleY * transform.scaleY;
  } else {
    transform.worldX = transform.x;
    transform.worldY = transform.y;
    transform.worldRotation = transform.rotation;
    transform.worldScaleX = transform.scaleX;
    transform.worldScaleY = transform.scaleY;
  }

  transform.dirty = FeFalse;

  if (!registry.Has<Children>(entity)) {
    return;
  }

  Children &children = registry.Get<Children>(entity);
  for (uint8 i = 0; i < children.count; i++) {
    ecs::EntityId childId = children.ids[i];
    if (!registry.Has<Transform2D>(childId)) {
      continue;
    }

    Transform2D &child = registry.Get<Transform2D>(childId);
    PropagateTransformForward(registry, childId, child, &transform);
  }
}

} // namespace flatearth::systems

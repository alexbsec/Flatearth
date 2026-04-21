#ifndef _FLATEARTH_ENGINE_SCENE_SYSTEMS_TRANSFORM_SYSTEM_HPP
#define _FLATEARTH_ENGINE_SCENE_SYSTEMS_TRANSFORM_SYSTEM_HPP

#include "Core/FeMemory.hpp"
#include "ECS/SystemInterface.hpp"

namespace flatearth::systems {

class TransformSystem : public ecs::ISystem {
public:
  explicit TransformSystem(memory::MemoryManager &memManager);
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::systems

#endif // _FLATEARTH_ENGINE_SCENE_SYSTEMS_TRANSFORM_SYSTEM_HPP

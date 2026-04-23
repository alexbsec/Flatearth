#ifndef _FLATEARTH_ENGINE_SCENE_SYSTEMS_PARTICLE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_SCENE_SYSTEMS_PARTICLE_SYSTEM_HPP

#include "ECS/Registry.hpp"
#include "ECS/SystemInterface.hpp"
#include "Scene/Components/ParticleEmitter.hpp"
#include "Scene/Components/Transform2D.hpp"

namespace flatearth::systems {

class ParticleSystem : public ecs::ISystem {
public:
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  void InitPool(ecs::Registry &reg, scene::ParticleEmitter &emitter,
                float32 deltaTime);
  void SpawnParticles(ecs::Registry &reg, scene::ParticleEmitter &emitter,
                      const scene::Transform2D &origin, float32 deltaTime);
  void UpdateParticles(ecs::Registry &reg, scene::ParticleEmitter &emitter,
                       float32 deltaTime);

private:
  static float32 RandRange(float32 lo, float32 hi);
};

}

#endif // _FLATEARTH_ENGINE_SCENE_SYSTEMS_PARTICLE_SYSTEM_HPP

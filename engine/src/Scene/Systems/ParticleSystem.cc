#include "ParticleSystem.hpp"

#include "Scene/Components/ParticleEmitter.hpp"
#include "Scene/Components/Sprite.hpp"

#include <cstdlib>

namespace flatearth::systems {

using namespace ecs;
using namespace scene;

void ParticleSystem::Update(Registry &reg, float32 deltaTime) {
  View<ParticleEmitter, Transform2D> view = reg.ViewOf<ParticleEmitter, Transform2D>();

  for (auto &&[entity, emitter, transform] : view) {
    if (!emitter.initialized) {
      InitPool(reg, emitter, deltaTime);
    }

    SpawnParticles(reg, emitter, transform, deltaTime);
    UpdateParticles(reg, emitter, deltaTime);
  }
}

void ParticleSystem::InitPool(Registry &reg, ParticleEmitter &emitter, float32 deltaTime) {
  for (uint32 i = 0; i < cMaxParticles; i++) {
    EntityId id = reg.Create();

    Transform2D xform{};
    xform.scaleX = xform.scaleY = 0.0f;

    Sprite sprite{};
    sprite.texHandle = emitter.texHandle;
    sprite.matHandle = emitter.matHandle;
    sprite.meshHandle = emitter.meshHandle;
    sprite.layer = emitter.layer;
    sprite.uvOffset = {0.0f, 0.0f};
    sprite.uvScale = {1.0f, 1.0f};

    reg.Insert(id, xform);
    reg.Insert(id, sprite);

    emitter.pool[i] = id;
    emitter.active[i] = FeFalse;
  }

  emitter.initialized = FeTrue;
}

void ParticleSystem::SpawnParticles(Registry &reg,
                                    ParticleEmitter &emitter,
                                    const Transform2D &origin,
                                    float32 deltaTime) {
  emitter.spawnAccum += emitter.spawnRate * deltaTime;
  uint32 toSpawn = static_cast<uint32>(emitter.spawnAccum);
  emitter.spawnAccum -= static_cast<float32>(toSpawn);

  uint32 spawned = 0;
  for (uint32 i = 0; i < cMaxParticles && spawned < toSpawn; i++) {
    if (emitter.active[i]) {
      continue;
    }

    emitter.active[i] = FeTrue;
    emitter.elapsed[i] = 0.0f;
    emitter.lifetime[i] = RandRange(emitter.lifetimeMin, emitter.lifetimeMax);
    emitter.velX[i] = RandRange(emitter.velocityMin.x(), emitter.velocityMax.x());
    emitter.velY[i] = RandRange(emitter.velocityMin.y(), emitter.velocityMax.y());

    auto &xform = reg.Get<Transform2D>(emitter.pool[i]);
    xform.x = origin.worldX;
    xform.y = origin.worldY;
    xform.scaleX = emitter.sizeStart;
    xform.scaleY = emitter.sizeStart;
    xform.dirty = FeTrue;
    spawned++;
  }
}

void ParticleSystem::UpdateParticles(Registry &reg, ParticleEmitter &emitter, float32 deltaTime) {
  for (uint32 i = 0; i < cMaxParticles; i++) {
    if (!emitter.active[i]) {
      continue;
    }

    emitter.elapsed[i] += deltaTime;
    if (emitter.elapsed[i] >= emitter.lifetime[i]) {
      emitter.active[i] = FeFalse;
      auto &xform = reg.Get<Transform2D>(emitter.pool[i]);
      xform.scaleX = 0.0f;
      xform.scaleY = 0.0f;
      xform.dirty = FeTrue;
      continue;
    }

    float32 t = emitter.elapsed[i] / emitter.lifetime[i];
    float32 size = emitter.sizeStart + (emitter.sizeEnd - emitter.sizeStart) * t;

    auto &xform = reg.Get<Transform2D>(emitter.pool[i]);
    xform.x += emitter.velX[i] * deltaTime;
    xform.y += emitter.velY[i] * deltaTime;
    xform.scaleX = size;
    xform.scaleY = size;
    xform.dirty = FeTrue;
  }
}

float32 ParticleSystem::RandRange(float32 lo, float32 hi) {
  return lo + (hi - lo) * (static_cast<float32>(std::rand()) / static_cast<float32>(RAND_MAX));
}

} // namespace flatearth::systems

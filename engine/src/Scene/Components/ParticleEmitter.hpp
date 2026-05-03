#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_PARTICLE_EMITTER_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_PARTICLE_EMITTER_HPP

#include "ECS/ECSTypes.hpp"
#include "Math/Vector2D.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

#include <array>

namespace flatearth::scene {

constexpr uint32 cMaxParticles = 128;

struct ParticleEmitter {
  math::Vec2D velocityMin{-1.0f, -1.0f};
  math::Vec2D velocityMax{1.0f, 1.0f};
  float32 lifetimeMin{0.5f};
  float32 lifetimeMax{0.5f};
  float32 sizeStart{0.15f};
  float32 sizeEnd{0.0f};
  float32 spawnRate{10.0f};
  renderer::RenderLayer layer{renderer::RenderLayer::Effects};

  // Runtime: not serializable
  std::array<ecs::EntityId, cMaxParticles> pool{};
  std::array<float32, cMaxParticles> velX{};
  std::array<float32, cMaxParticles> velY{};
  std::array<float32, cMaxParticles> lifetime{};
  std::array<float32, cMaxParticles> elapsed{};
  std::array<bool, cMaxParticles> active{};
  float32 spawnAccum{0.0f};
  bool initialized{FeFalse};
  resources::TextureHandle texHandle{0};
  resources::MaterialHandle matHandle{0};
  resources::MeshHandle meshHandle{0};
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_PARTICLE_EMITTER_HPP

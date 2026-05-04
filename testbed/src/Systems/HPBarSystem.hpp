#pragma once

#include <Core/EngineContext.hpp>
#include <ECS/ECSTypes.hpp>
#include <ECS/SystemInterface.hpp>
#include <Resources/ResourceTypes.hpp>
#include <Scene/Scene.hpp>

#include "../Director.hpp"

namespace flatearth::testbed {

class HPBarSystem : public ecs::ISystem {
public:
  explicit HPBarSystem(EngineContext &ctx, Orchestrator &orchestrator, scene::SceneId sceneId);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  void SpawnEntities(ecs::Registry &registry);

private:
  EngineContext &_ctx;
  Orchestrator &_orchestrator;
  scene::SceneId _sceneId;
  resources::FontHandle _fontHandle{resources::cInvalidFontHandle};
  resources::MeshHandle _bgMeshHandle{};
  resources::MaterialHandle _bgMatHandle{};
  resources::MeshHandle _fillMeshHandle{};
  resources::MaterialHandle _fillMatHandle{};
  ecs::EntityId _bgBarEntity{ecs::cNullEntity};
  ecs::EntityId _fillBarEntity{ecs::cNullEntity};
  ecs::EntityId _textEntity{ecs::cNullEntity};
  bool _spawned{FeFalse};
};

} // namespace flatearth::testbed

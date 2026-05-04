#pragma once

#include <Core/EngineContext.hpp>
#include <ECS/SystemInterface.hpp>
#include <Scene/Scene.hpp>

#include "../Director.hpp"

namespace flatearth::testbed {

struct PlayerPrefab {};

class PlayerSystem : public ecs::ISystem {
public:
  explicit PlayerSystem(EngineContext &ctx, Orchestrator &orchestrator, scene::SceneId sceneId);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  void SpawnEntities();

private:
  EngineContext &_ctx;
  Orchestrator &_orchestrator;
  scene::SceneId _sceneId;
  bool _spawned{FeFalse};
};

} // namespace flatearth::testbed

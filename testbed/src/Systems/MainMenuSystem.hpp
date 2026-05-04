#pragma once

#include <Core/EngineContext.hpp>
#include <ECS/SystemInterface.hpp>
#include <Scene/Scene.hpp>

#include "../Director.hpp"

namespace flatearth::testbed {

class MainMenuSystem : public ecs::ISystem {
public:
  explicit MainMenuSystem(EngineContext &ctx, Orchestrator &orchestrator, scene::SceneId sceneId);
  void Initialize(ecs::Registry &registry) override;
  void Update(ecs::Registry &, float32) override {}

private:
  EngineContext &_ctx;
  Orchestrator &_orchestrator;
  scene::SceneId _sceneId;
};

} // namespace flatearth::testbed

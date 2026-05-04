#ifndef _FLATEARTH_TESTBED_SYSTEMS_WORLD_SYSTEM_HPP
#define _FLATEARTH_TESTBED_SYSTEMS_WORLD_SYSTEM_HPP

#include <Core/EngineContext.hpp>
#include <ECS/SystemInterface.hpp>
#include <Scene/Scene.hpp>

#include "../Director.hpp"

namespace flatearth::testbed {

class WorldSystem : public ecs::ISystem {
public:
  explicit WorldSystem(EngineContext &ctx, Orchestrator &orchestrator, scene::SceneId sceneId);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  void SpawnEntities(ecs::Registry &registry);

private:
  EngineContext &_ctx;
  Orchestrator &_orchestrator;
  scene::SceneId _sceneId{scene::cNullScene};
  bool _spawned{FeFalse};
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTBED_SYSTEMS_WORLD_SYSTEM_HPP

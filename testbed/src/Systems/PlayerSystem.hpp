#pragma once

#include <Core/EngineContext.hpp>
#include <ECS/SystemInterface.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

struct PlayerPrefab {};

class PlayerSystem : public ecs::ISystem {
public:
  explicit PlayerSystem(EngineContext &ctx, scene::SceneId sceneId);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  EngineContext &_ctx;
  scene::SceneId _sceneId;
};


} // namespace flatearth::testbed

#pragma once

#include <Core/EngineContext.hpp>
#include <ECS/SystemInterface.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

struct PlayerPrefab {};

class PlayerSystem : public ecs::ISystem {
public:
  explicit PlayerSystem(EngineContext &ctx, string sceneName);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  EngineContext &_ctx;
  string _sceneName;
};


} // namespace flatearth::testbed

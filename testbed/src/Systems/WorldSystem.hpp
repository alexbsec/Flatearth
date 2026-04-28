#ifndef _FLATEARTH_TESTBED_SYSTEMS_WORLD_SYSTEM_HPP
#define _FLATEARTH_TESTBED_SYSTEMS_WORLD_SYSTEM_HPP

#include <Core/EngineContext.hpp>
#include <ECS/SystemInterface.hpp>

namespace flatearth::testbed {

class WorldSystem : public ecs::ISystem {
public:
  explicit WorldSystem(EngineContext& ctx, const string &sceneName);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  EngineContext &_ctx;
  string _sceneName{};
};

}

#endif // _FLATEARTH_TESTBED_SYSTEMS_WORLD_SYSTEM_HPP

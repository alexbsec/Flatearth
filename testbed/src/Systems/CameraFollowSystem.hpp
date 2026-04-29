#ifndef _FLATEARTH_TESTBED_SYSTEMS_CAMERA_FOLLOW_SYSTEM_HPP
#define _FLATEARTH_TESTBED_SYSTEMS_CAMERA_FOLLOW_SYSTEM_HPP

#include <Core/EngineContext.hpp>
#include <ECS/Registry.hpp>
#include <ECS/SystemInterface.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

class CameraFollowSystem : public ecs::ISystem {
public:
  explicit CameraFollowSystem(EngineContext &ctx, scene::SceneId sceneId);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  EngineContext &_ctx;
  scene::SceneId _sceneId;
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTBED_SYSTEMS_CAMERA_FOLLOW_SYSTEM_HPP

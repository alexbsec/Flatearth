#pragma once

#include <Core/EngineContext.hpp>
#include <ECS/ECSTypes.hpp>
#include <ECS/SystemInterface.hpp>
#include <Resources/ResourceTypes.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

class HPBarSystem : public ecs::ISystem {
public:
  explicit HPBarSystem(EngineContext &ctx, scene::SceneId sceneId);
  void Initialize(ecs::Registry &) override;
  void Update(ecs::Registry &, float32) override;

private:
  EngineContext &_ctx;
  scene::SceneId _sceneId;
  resources::FontHandle _fontHandle{resources::cInvalidFontHandle};
  ecs::EntityId _bgBarEntity{ecs::cNullEntity};
  ecs::EntityId _fillBarEntity{ecs::cNullEntity};
  ecs::EntityId _textEntity{ecs::cNullEntity};
};

} // namespace flatearth::testbed

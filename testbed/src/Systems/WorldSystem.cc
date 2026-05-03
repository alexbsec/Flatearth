#include "WorldSystem.hpp"

#include "../Components/Tags.hpp"

#include <Assets/TilemapManager.hpp>
#include <Core/Logger.hpp>
#include <Scene/Components/AudioSource.hpp>
#include <Scene/Components/Utils.hpp>
#include <Scene/Scene.hpp>
#include <UI/Components/UIAnchor.hpp>
#include <UI/Components/UIText.hpp>

namespace flatearth::testbed {

WorldSystem::WorldSystem(EngineContext &ctx, scene::SceneId sceneId)
    : _ctx(ctx), _sceneId(sceneId) {
}

void WorldSystem::Initialize(ecs::Registry &registry) {
  _ctx.core.KVarsRegistry()
      .Register("particle.spawnRate", "Particle emitter spawn rate (particles/s)", 15.0f)
      .or_log_error("failed to register particle spawn rate kvar");

  _ctx.core.KVarsRegistry()
      .Register("bgm.volume", "BGM volume (0.0 - 2.0)", 1.0f)
      .or_log_error("failed to register bgm volume kvar");

  _ctx.assets.Tilemap()
      .Load("assets/tiles/LevelEntrance.tmx", _sceneId)
      .or_log_error("failed to load tilemap");

  // in Initialize():
  auto fontHandle = _ctx.assets.Manager()
                        .LoadFont("robotika", "assets/fonts/stocky.ttf", 120.0f, 2048)
                        .or_fatal("failed to load font");

  auto textEntity =
      registry.Spawn()
          .With(ui::UIAnchor{
              .normalizedX = 0.02f, .normalizedY = 0.5f, .scaleX = 0.1f, .scaleY = 0.1f})
          .OwnedBy(_sceneId)
          .Commit();

  ui::UIText uiText{};
  uiText.handle = fontHandle;
  uiText.color = {{1.0f, 1.0f, 0.0f}, 1.0f};
  uiText.text.Set("Hello Flatearth!");
  registry.Insert<ui::UIText>(textEntity, uiText);

  scene::AudioSource bgm{};
  scene::SetAudioPath(bgm, "assets/bgm/95.mp3");
  bgm.loop = FeTrue;
  bgm.playing = FeTrue;
  registry.Spawn().With(bgm).With(WorldTag{}).OwnedBy(_sceneId).Commit();
}

void WorldSystem::Update(ecs::Registry &registry, float32) {
  auto &kvars = _ctx.core.KVarsRegistry();
  ecs::View<scene::AudioSource, WorldTag> view = registry.ViewOf<scene::AudioSource, WorldTag>();
  for (auto &&[id, audioSrc, worldTag] : view) {
    float32 vol = kvars.Get<float32>("bgm.volume").value_or(1.0f);
    if (audioSrc.volume != vol) {
      LOG_INFO("audio volume changed to {}", vol);
      audioSrc.volume = vol;
      audioSrc.dirty = FeTrue;
    }
  }
}
} // namespace flatearth::testbed

#include "WorldSystem.hpp"

#include "../Components/Tags.hpp"

#include <Core/Logger.hpp>
#include <Scene/Components/AudioSource.hpp>
#include <Scene/Components/Utils.hpp>
#include <Scene/Scene.hpp>
#include <cstring>

#include <Assets/TilemapManager.hpp>

namespace flatearth::testbed {

WorldSystem::WorldSystem(EngineContext &ctx, const string &sceneName)
    : _ctx(ctx), _sceneName(sceneName) {
}

void WorldSystem::Initialize(ecs::Registry &registry) {
  auto res = _ctx.core.KVarsRegistry().Register(
      "particle.spawnRate", "Particle emitter spawn rate (particles/s)", 15.0f);
  if (!res.has_value()) {
    FLOG_ERROR("failed to register particle spawn rate kvar");
  }

  res = _ctx.core.KVarsRegistry().Register("bgm.volume", "BGM volume (0.0 - 2.0)", 1.0f);
  if (!res.has_value()) {
    FLOG_ERROR("failed to register bgm volume kvar");
  }

  auto tilemapRes = _ctx.assets.Tilemap().Load("assets/tiles/LevelEntrance.tmx", _sceneName);
  if (!tilemapRes.has_value()) {
    FLOG_ERROR("failed to load tilemap: {}", tilemapRes.error().message);
  }

  const scene::SceneOwnership ownership{_sceneName};
  ecs::EntityId audioId = registry.Create();
  scene::AudioSource bgm{};
  scene::SetAudioPath(bgm, "assets/bgm/95.mp3");
  bgm.loop = FeTrue;
  bgm.playing = FeTrue;
  registry.Insert(audioId, bgm);
  registry.Insert(audioId, WorldTag{});
  registry.Insert(audioId, ownership);

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

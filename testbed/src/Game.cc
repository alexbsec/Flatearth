#include "Game.hpp"

#include "Components/Tags.hpp"
#include "Systems/CameraFollowSystem.hpp"
#include "Systems/PlayerSystem.hpp"
#include "Systems/WorldSystem.hpp"

#include <Core/EngineContext.hpp>
#include <Core/FeMemory.hpp>
#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Scene/Components/AudioSource.hpp>
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/ParticleEmitter.hpp>
#include <Scene/Components/Sprite.hpp>
#include <Scene/Components/SpriteAnimator.hpp>
#include <Scene/Components/Transform2D.hpp>
#include <Scene/Scene.hpp>
#include <Scene/Systems/AudioSystem.hpp>
#include <Scene/Systems/TransformSystem.hpp>
#include <cstring>
#include <imgui.h>
#include <vector>

namespace flatearth::testbed {

GameState GameTest::_state = GameState{};
FePtr<Orchestrator> GameTest::_pOrchestrator{};

bool GameTest::GameLoad(flatearth::Game *gameInstance) {
  Setup(gameInstance);
  return FeTrue;
}

void GameTest::GameRegisterSystems(flatearth::Game *gameInstance, ecs::SystemScheduler &scheduler) {
  auto playerRes = scheduler.Register<PlayerSystem>(*gameInstance->pCtx, string("level1"));
  if (!playerRes.has_value()) {
    FLOG_FATAL("failed to register PlayerSystem: {}", playerRes.error().message);
    std::exit(1);
  }
  playerRes.value().Before<systems::TransformSystem>();

  auto cameraRes = scheduler.Register<CameraFollowSystem>(*gameInstance->pCtx, string("level1"));
  if (!cameraRes.has_value()) {
    FLOG_FATAL("failed to register CameraFollowSystem: {}", cameraRes.error().message);
    std::exit(1);
  }
  cameraRes.value().After<PlayerSystem>().Before<systems::TransformSystem>();

  auto worldRes = scheduler.Register<WorldSystem>(*gameInstance->pCtx, string("level1"));
  if (!worldRes.has_value()) {
    FLOG_FATAL("failed to register WorldSystem: {}", worldRes.error().message);
    std::exit(1);
  }
  worldRes.value().Before<systems::AudioSystem>();
}

// TODO: remove, just here for testing
void GameTest::GameImGui(flatearth::Game *gameInstance) {
  EngineContext &ctx = *gameInstance->pCtx;

  ImGui::Begin("Debug");

  ImGui::Text("FPS: %.1f  |  Draw calls: %u", 1.0f / ImGui::GetIO().DeltaTime, ctx.drawCallCount);

  if (ImGui::CollapsingHeader("ECS", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Entities alive: %u", ctx.project.Registry().AliveCount());
    for (auto [id, ptag, xform] : ctx.project.Registry().ViewOf<PlayerTag, scene::Transform2D>()) {
      ImGui::Text("Player pos: (%.2f, %.2f)", xform.x, xform.y);
    }
  }

  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto camView = ctx.project.Registry().ViewOf<scene::Transform2D, scene::Camera2D>();
    for (auto [e, xform, cam] : camView) {
      ImGui::Text("World pos: (%.2f, %.2f)", xform.worldX, xform.worldY);
      ImGui::Text("Zoom: %.3f", cam.zoom);
      break;
    }
  }

  if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto emitterView = ctx.project.Registry().ViewOf<scene::ParticleEmitter, scene::Transform2D>();
    int emitterIdx = 0;
    for (auto [e, emitter, xform] : emitterView) {
      uint32 activeCount = 0;
      for (uint32 i = 0; i < scene::cMaxParticles; ++i) {
        activeCount += emitter.active[i] ? 1 : 0;
      }
      ImGui::Text("Emitter %d: %u / %u active", emitterIdx, activeCount, scene::cMaxParticles);
      ImGui::Text(
          "  pos: (%.2f, %.2f)  rate: %.1f/s", xform.worldX, xform.worldY, emitter.spawnRate);
      ++emitterIdx;
    }
    if (emitterIdx == 0) {
      ImGui::TextDisabled("no emitters");
    }
  }

  if (ImGui::CollapsingHeader("KVars")) {
    ctx.core.KVarsRegistry().ForEach([](const string &key, flatearth::KVar &kvar) {
      if (std::holds_alternative<float32>(kvar.value)) {
        float v = std::get<float32>(kvar.value);
        if (ImGui::SliderFloat(key.c_str(), &v, 0.0f, 2.0f)) {
          kvar.value = v;
        }
      } else if (std::holds_alternative<bool>(kvar.value)) {
        bool v = std::get<bool>(kvar.value);
        if (ImGui::Checkbox(key.c_str(), &v)) {
          kvar.value = v;
        }
      } else if (std::holds_alternative<int32>(kvar.value)) {
        int v = std::get<int32>(kvar.value);
        if (ImGui::InputInt(key.c_str(), &v)) {
          kvar.value = static_cast<int32>(v);
        }
      } else if (std::holds_alternative<string>(kvar.value)) {
        ImGui::TextDisabled("%s: %s", key.c_str(), std::get<string>(kvar.value).c_str());
      }
      if (ImGui::IsItemHovered() && !kvar.description.empty()) {
        ImGui::SetTooltip("%s", kvar.description.c_str());
      }
    });
  }

  if (ImGui::CollapsingHeader("Memory")) {
    const auto &stats = ctx.core.Memory().GetStats();
    double totalKB = stats.memoryBlock.totalAllocated / 1024.0;
    double totalMB = totalKB / 1024.0;
    ImGui::Text("Total: %.2f KB (%.2f MB)", totalKB, totalMB);
    ImGui::Text("Active allocs: %llu", static_cast<unsigned long long>(stats.allocCount));
    ImGui::Separator();
    for (uint64 i = 0; i < memory::MaxTags; ++i) {
      uint64 bytes = stats.memoryBlock.taggedAllocations[i];
      if (bytes == 0) {
        continue;
      }
      ImGui::Text(
          "  [%-16s] %.2f KB", memory::TagName(static_cast<memory::Tag>(i)), bytes / 1024.0);
    }
  }

  ImGui::End();
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  _pOrchestrator.reset();

  if (!gameInstance->pCtx) {
    return;
  }
  EngineContext &ctx = *gameInstance->pCtx;

  auto &reg = ctx.project.Registry();
  for (auto [id, ptag, sprite] : reg.ViewOf<PlayerTag, scene::Sprite>()) {
    ctx.assets.Manager().ReleaseSprite(sprite);
  }

  for (auto [id, pttag, emitter] : reg.ViewOf<ParticleTag, scene::ParticleEmitter>()) {
    scene::Sprite base{};
    base.texHandle = emitter.texHandle;
    base.matHandle = emitter.matHandle;
    base.meshHandle = emitter.meshHandle;
    ctx.assets.Manager().ReleaseSprite(base);
  }

  ctx.assets.Tilemap().Unload("assets/tiles/LevelEntrance.tmx");

  std::vector<ecs::EntityId> toDestroy;
  for (auto [id, ownership] : reg.ViewOf<scene::SceneOwnership>()) {
    if (std::strncmp(ownership.sceneName, "level1", scene::cSceneNameMax) == 0) {
      toDestroy.push_back(id);
    }
  }
  for (auto id : toDestroy) {
    reg.Destroy(id);
  }
}

void GameTest::Setup(flatearth::Game *gameInstance) {
  gameInstance->windowStartWidth = 1280;
  gameInstance->windowStartHeight = 720;
  gameInstance->windowStartPosX = 100;
  gameInstance->windowStartPosY = 100;
  gameInstance->gameName = "TopDown";
  _pOrchestrator = gameInstance->pCtx->core.Memory().Allocate<Orchestrator>(memory::Tag::Game,
                                                                            *gameInstance->pCtx);
}

} // namespace flatearth::testbed

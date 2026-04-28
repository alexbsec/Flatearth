#include "Director.hpp"

#include <Core/Logger.hpp>
#include <ECS/Registry.hpp>
#include <Scene/Scene.hpp>

#include <cstring>
#include <vector>

namespace flatearth::testbed {

static const string sceneName_Playing    = "level1";
static const string sceneName_MainMenu   = "main_menu";
static const string sceneName_GameOver   = "game_over";
static const string sceneName_GamePaused = "game_paused";

static string PhaseToSceneName(GamePhase phase) {
  switch (phase) {
    case GamePhase::Playing:  return sceneName_Playing;
    case GamePhase::MainMenu: return sceneName_MainMenu;
    case GamePhase::GameOver: return sceneName_GameOver;
    case GamePhase::Paused:   return sceneName_GamePaused;
  }
  return {};
}

Orchestrator::Orchestrator(EngineContext &ctx) : _ctx(ctx) {}

void Orchestrator::RequestTransition(GamePhase phase) {
  _pendingPhase = phase;
}

void Orchestrator::Update(float32) {
  if (_pendingPhase == _currentPhase) {
    return;
  }
  ChangeScene();
}

void Orchestrator::ChangeScene() {
  if (!_currentSceneName.empty()) {
    auto &reg = _ctx.project.Registry();
    std::vector<ecs::EntityId> toDestroy;
    for (auto [id, ownership] : reg.ViewOf<scene::SceneOwnership>()) {
      if (std::strncmp(ownership.sceneName, _currentSceneName.c_str(), scene::cSceneNameMax) == 0)
        toDestroy.push_back(id);
    }
    for (auto id : toDestroy)
      reg.Destroy(id);
  }

  _currentSceneName = PhaseToSceneName(_pendingPhase);
  _currentPhase = _pendingPhase;
}

} // namespace flatearth::testbed

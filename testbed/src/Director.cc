#include "Director.hpp"

#include <Core/Logger.hpp>

namespace flatearth::testbed {

Orchestrator::Orchestrator(EngineContext &ctx) : _ctx(ctx) {
  _sceneIds[static_cast<int>(GamePhase::MainMenu)] = ctx.project.RegisterScene("main_menu");
  _sceneIds[static_cast<int>(GamePhase::Playing)] = ctx.project.RegisterScene("level1");
  _sceneIds[static_cast<int>(GamePhase::Paused)] = ctx.project.RegisterScene("game_paused");
  _sceneIds[static_cast<int>(GamePhase::GameOver)] = ctx.project.RegisterScene("game_over");
}

scene::SceneId Orchestrator::PhaseToSceneId(GamePhase phase) const {
  return _sceneIds[static_cast<int>(phase)];
}

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
  if (_currentSceneId != scene::cNullScene) {
    _ctx.project.DestroyScene(_currentSceneId);
  }

  _currentSceneId = PhaseToSceneId(_pendingPhase);
  _currentPhase = _pendingPhase;
}

} // namespace flatearth::testbed

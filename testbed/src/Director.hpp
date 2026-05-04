#ifndef _FLATEARTH_TESTBED_DIRECTOR_HPP
#define _FLATEARTH_TESTBED_DIRECTOR_HPP

#include <Core/EngineContext.hpp>
#include <Scene/Scene.hpp>

namespace flatearth::testbed {

enum class GamePhase {
  MainMenu,
  Playing,
  Paused,
  GameOver,
};

class Orchestrator {
public:
  explicit Orchestrator(EngineContext &ctx);
  void RequestTransition(GamePhase);
  void Update(float32);
  GamePhase CurrentPhase() const { return _currentPhase; }

private:
  void ChangeScene();
  scene::SceneId PhaseToSceneId(GamePhase phase) const;

private:
  EngineContext &_ctx;
  GamePhase _currentPhase{GamePhase::MainMenu};
  GamePhase _pendingPhase{GamePhase::MainMenu};
  scene::SceneId _currentSceneId{scene::cNullScene};
  scene::SceneId _sceneIds[4]{};
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTBED_DIRECTOR_HPP

#ifndef _FLATEARTH_TESTBED_DIRECTOR_HPP
#define _FLATEARTH_TESTBED_DIRECTOR_HPP

#include <Core/EngineContext.hpp>

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

private:
  void ChangeScene();

private:
  EngineContext &_ctx;
  GamePhase _currentPhase{GamePhase::MainMenu};
  GamePhase _pendingPhase{GamePhase::MainMenu};
  string _currentSceneName{};
};

}

#endif // _FLATEARTH_TESTBED_DIRECTOR_HPP

#ifndef _FLATEARTH_TESTSUITE_GAME_HPP
#define _FLATEARTH_TESTSUITE_GAME_HPP

#include "Director.hpp"

#include <Defines.hpp>
#include <ECS/ECSTypes.hpp>
#include <ECS/Scheduler/SystemScheduler.hpp>
#include <GameTypes.hpp>

namespace flatearth::testbed {

struct GameState {};

class GameTest {
public:
  static bool GameLoad(flatearth::Game *gameInstance);
  static void GameUnload(flatearth::Game *gameInstance);
  static void GameRegisterSystems(flatearth::Game *gameInstance, ecs::SystemScheduler &scheduler);
  static void GameImGui(flatearth::Game *gameInstance);

private:
  static void Setup(flatearth::Game *gameInstance);

private:
  static GameState _state;
  static FePtr<Orchestrator> _pOrchestrator;
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTSUITE_GAME_HPP

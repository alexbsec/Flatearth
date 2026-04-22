#ifndef _FLATEARTH_TESTSUITE_GAME_HPP
#define _FLATEARTH_TESTSUITE_GAME_HPP

#include <Defines.hpp>
#include <ECS/ECSTypes.hpp>
#include <GameTypes.hpp>

namespace flatearth::testbed {

struct GameState {
  ecs::EntityId cameraEntity{UINT32_MAX};
  ecs::EntityId tilemapRoot{UINT32_MAX};
};

class GameTest {
public:
  static bool GameInitialize(flatearth::Game *gameInstance);
  static bool GameLoad(flatearth::Game *gameInstance);
  static void GameUnload(flatearth::Game *gameInstance);
  static bool GameUpdate(flatearth::Game *gameInstance, float32 deltaTime);
  static bool GameOnResize(flatearth::Game *gameInstance, uint32 width, uint32 height);

private:
  static GameState _state;
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTSUITE_GAME_HPP

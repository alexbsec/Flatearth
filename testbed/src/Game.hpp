#ifndef _FLATEARTH_TESTSUITE_GAME_HPP
#define _FLATEARTH_TESTSUITE_GAME_HPP

#include <Defines.hpp>
#include <GameTypes.hpp>

namespace flatearth::testbed {

struct GameState {
  float32 deltaTime;
};

class GameTest {
public:
  static bool GameInitialize(flatearth::Game *gameInstance);
  static bool GameUpdate(flatearth::Game *gameInstance, float32 deltaTime);
  static bool GameRender(flatearth::Game *gameInstance, float32 deltaTime);
  static bool GameOnResize(flatearth::Game *gameInstance, uint32 width, uint32 height);
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTSUITE_GAME_HPP

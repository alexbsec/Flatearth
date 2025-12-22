#include "Game.hpp"

namespace flatearth::testbed {

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  // FDEBUG("GameTest::GameInitialize() called");
  gameInstance->windowStartWidth = 1280;
  gameInstance->windowStartHeight = 980;
  gameInstance->windowStartPosX = 100;
  gameInstance->windowStartPosY = 100;
  gameInstance->gameName = "Testbed";
  return FeTrue;
}

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  return FeTrue;
}

bool GameTest::GameRender(flatearth::Game *gameInstance, float32 deltaTime) {
  return FeTrue;
}

bool GameTest::GameOnResize(flatearth::Game *gameInstance, uint32 width,
                            uint32 height) {
  return FeTrue;
}

} // namespace testsuite

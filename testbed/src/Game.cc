#include "Game.hpp"

namespace flatearth {
namespace testsuite {

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  // FDEBUG("GameTest::GameInitialize() called");
  return FeTrue;
}

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  return FeTrue;
}

bool GameTest::GameRender(flatearth::Game *gameInstance, float32 deltaTime) {
  return FeTrue;
}

void GameTest::GameOnResize(flatearth::Game *gameInstance, uint32 width,
                            uint32 height) {}

} // namespace testsuite
} // namespace flatearth

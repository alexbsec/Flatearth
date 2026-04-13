#include "Game.hpp"
#include "GameTypes.hpp"

#include <Core/Application.hpp>
#include <Core/Logger.hpp>
#include <Entrypoint.hpp>
#include <cstdlib>

bool CreateGame(flatearth::Game *pGame) {
  using namespace flatearth::testbed;
  pGame->Initialize = GameTest::GameInitialize;
  pGame->Load = GameTest::GameLoad;
  pGame->Update = GameTest::GameUpdate;
  pGame->Render = GameTest::GameRender;
  pGame->OnResize = GameTest::GameOnResize;
  return true;
}

int main(void) {
  flatearth::Game gameInstance;

  if (!CreateGame(&gameInstance)) {
    LOG_FATAL("could not create game");
    return EXIT_FAILURE;
  }

  flatearth::Engine engine(&gameInstance);
  if (auto res = engine.Initialize(); !res.has_value()) {
    LOG_ERROR("engine failed to initialize");
    return EXIT_FAILURE;
  }

  if (auto res = engine.Start(); !res.has_value()) {
    LOG_ERROR("engine failed to start");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

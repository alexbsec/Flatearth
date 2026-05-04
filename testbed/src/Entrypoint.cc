#include "Game.hpp"
#include "GameTypes.hpp"

#include <Core/Application.hpp>
#include <Core/Logger.hpp>
#include <Entrypoint.hpp>
#include <cstdlib>

bool CreateGame(flatearth::Game *pGame) {
  using namespace flatearth::testbed;
  pGame->Load = GameTest::GameLoad;
  pGame->Unload = GameTest::GameUnload;
  pGame->RegisterSystems = GameTest::GameRegisterSystems;
  pGame->OnImGui = GameTest::GameImGui;
  return FeTrue;
}

int main(void) {
  flatearth::Game gameInstance;

  if (!CreateGame(&gameInstance)) {
    LOG_FATAL("could not create game");
    return EXIT_FAILURE;
  }

  flatearth::Application app;
  if (auto res = app.Initialize(gameInstance); res.errored()) {
    LOG_ERROR("engine failed to initialize");
    return EXIT_FAILURE;
  }

  if (auto res = app.Start(); res.errored()) {
    LOG_ERROR("engine failed to start");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

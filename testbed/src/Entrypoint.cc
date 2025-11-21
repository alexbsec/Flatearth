#include "GameTypes.hpp"
#include <Entrypoint.hpp>
#include <Core/Logger.hpp>
#include <Core/Application.hpp>
#include <cstdlib>

bool CreateGame(flatearth::Game *outGame) {
  return true;
}

int main(void) {
  flatearth::Game gameInstance;

  if (!CreateGame(&gameInstance)) {
    LOG_FATAL("could not create game");
    return EXIT_FAILURE;
  }

  flatearth::Engine engine(gameInstance); 
  if (!engine.Initialize()) {
    LOG_ERROR("engine failed to initialize");
    return EXIT_FAILURE;
  }

  if (!engine.Start()) {
    LOG_ERROR("engine failed to start");
    return EXIT_FAILURE;
  }

  LOG_INFO("engine started");
 
  return EXIT_SUCCESS;
}

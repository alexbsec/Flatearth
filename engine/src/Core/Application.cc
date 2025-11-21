#include "Application.hpp"
#include "Defines.hpp"

namespace flatearth {

Engine::Engine(Game &game) : _appState(game) {}

bool Engine::Initialize() {
  return FeTrue;
}

bool Engine::Start() {
  return FeTrue;
}

}

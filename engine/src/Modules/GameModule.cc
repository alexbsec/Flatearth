#include "GameModule.hpp"

namespace flatearth::modules {

FeExpect<bool, Error> Project::Initialize(float32 gravityX, float32 gravityY) {
  if (_initialized) {
    return FeFalse;
  }

  _world.Initialize(gravityX, gravityY);
  _initialized = FeTrue;
  return FeTrue;
}

void Project::Shutdown() {
  if (!_initialized) {
    return;
  }

  _world.Shutdown();
  _initialized = FeFalse;
}

}

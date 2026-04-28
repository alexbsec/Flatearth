#include "CoreModule.hpp"

namespace flatearth::modules {

FeExpect<bool, Error> Core::Initialize() {
  if (_initialized) {
    return FeFalse;
  }

  auto initRes = _audio.Initialize();
  if (!initRes.has_value()) {
    return FeErr{initRes.error()};
  }

  _initialized = FeTrue;
  return FeTrue;
}

void Core::Shutdown() {
  if (!_initialized) {
    return;
  }

  _audio.Shutdown();
  _initialized = FeFalse;
}

}

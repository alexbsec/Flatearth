#include "CoreModule.hpp"

namespace flatearth::modules {

FeExpect<void, Error> Core::Initialize() {
  if (_initialized) {
    return {};
  }

  auto initRes = _audio.Initialize();
  if (initRes.errored()) {
    return FeErr{initRes.error()};
  }

  _initialized = FeTrue;
  return {};
}

void Core::Shutdown() {
  if (!_initialized) {
    return;
  }

  _audio.Shutdown();
  _initialized = FeFalse;
}

} // namespace flatearth::modules

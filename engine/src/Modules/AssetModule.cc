#include "AssetModule.hpp"

#include "Renderer/RendererFrontend.hpp"

namespace flatearth::modules {

FeExpect<bool, Error> Assets::Initialize(renderer::FrontendRenderer *pRenderer) {
  if (_initialized) {
    return FeFalse;
  }

  if (pRenderer == nullptr) {
    return FeErr{Error("cannot initialize assets module with nullptr renderer",
                       ErrorType::NullptrException)};
  }

  _manager.Initialize(pRenderer);
  _initialized = FeTrue;
  return FeTrue;
}

void Assets::Shutdown() {
  if (!_initialized) {
    return;
  }

  _manager.Shutdown();
  _initialized = FeFalse;
}

} // namespace flatearth::modules

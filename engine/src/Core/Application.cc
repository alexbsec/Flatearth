#include "Application.hpp"

#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Error.hpp"

namespace flatearth {

Application::Application() : _engine(_memoryManager) {}

FeExpect<void, Error> Application::Initialize(Game &game, const ApplicationConfig &config) {
  if (_initialized) return {};
  FILE_LOGGING(FeTrue);
  _appConfig = config;
  auto res = _engine.Initialize(game, _appConfig);
  if (!res.has_value()) {
    FLOG_FATAL("engine failed to initialize");
    return FeErr{res.error()};
  }
  _initialized = true;
  return {};
}

FeExpect<void, Error> Application::Start() {
  return _engine.Start();
}

void Application::Shutdown() {
  _initialized = false;
}

Engine &Application::GetEngine() {
  return _engine;
}

} // namespace flatearth

#include "Application.hpp"

#include "Core/Logger.hpp"
#include "Defines.hpp"

namespace flatearth {

Application::Application()
    : _engine(_memoryManager.Allocate<Engine>(memory::Tag::Application, _memoryManager)) {
}

FeExpect<void, Error> Application::Initialize(Game &game, const ApplicationConfig &config) {
  if (_initialized)
    return {};
  FILE_LOGGING(FeTrue);
  _appConfig = config;
  FE_RIPPLE(_engine->Initialize(game, _appConfig).or_error("engine failed to initialize"));
  _initialized = true;
  return {};
}

FeExpect<void, Error> Application::Start() {
  return _engine->Start();
}

void Application::Shutdown() {
  _initialized = false;
}

Engine &Application::GetEngine() {
  return *_engine;
}

} // namespace flatearth

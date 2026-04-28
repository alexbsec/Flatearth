#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/Engine.hpp"
#include "Core/FeMemory.hpp"
#include "GameTypes.hpp"

namespace flatearth {

class Application {
public:
  FEAPI Application();

  FEAPI FeExpect<void, Error> Initialize(Game &game, const ApplicationConfig &config = {});
  FEAPI FeExpect<void, Error> Start();
  FEAPI void Shutdown();
  FEAPI Engine &GetEngine();

private:
  ApplicationConfig _appConfig{};
  memory::MemoryManager _memoryManager;
  Engine _engine;
  bool _initialized{false};
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

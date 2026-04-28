#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_HPP

#include "Core/CoreTypes.hpp"
#include "Core/EngineConsole.hpp"
#include "Core/EngineContext.hpp"
#include "Core/Event.hpp"
#include "ECS/Scheduler/SystemScheduler.hpp"
#include "GameTypes.hpp"
#include "Modules/AssetModule.hpp"
#include "Modules/CoreModule.hpp"
#include "Modules/GameModule.hpp"
#include "Renderer/GameRenderer.hpp"

namespace flatearth {

class Engine {
public:
  FEAPI explicit Engine(memory::MemoryManager &);
  FEAPI ~Engine();

  FEAPI FeExpect<void, Error> Initialize(Game &game, ApplicationConfig &config);
  FEAPI FeExpect<void, Error> Start();

private:
  FeExpect<void, Error> RegisterSystems();
  FeExpect<void, Error> CheckGamePrerequisites();

private:
  memory::MemoryManager &_memoryManager;

  EngineState _state;

  platform::FileSystem _filesystem;
  event::EventManager _eventManager;
  ecs::Registry _registry;
  ecs::SystemScheduler _scheduler;

  modules::Project _gameModule;
  modules::Assets _assetsModule;
  modules::Core _coreModule;

  EngineContext _ctx;

  renderer::GameRenderer _renderer;
  DevConsole _devConsole;

  FeSharedPtr<platform::Platform> _pPlatform;
  FePtr<event::IEventListener> _engineListener;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_HPP

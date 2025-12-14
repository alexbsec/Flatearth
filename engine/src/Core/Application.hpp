#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "Core/FeMemroy.hpp"
#include "Defines.hpp"
#include "ApplicationConfig.hpp"
#include "Platform/Platform.hpp"

namespace flatearth {

class Engine {
public:
  FEAPI Engine(Game &game);
  FEAPI FeExpect<void, Error> Initialize();
  FEAPI FeExpect<void, Error> Start();

private:
  ApplicationState _appState;
  FePtr<platform::Platform> _pPlatform;  
  memory::MemoryManager _memoryManager;
};

}

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

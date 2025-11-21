#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "Defines.hpp"
#include "ApplicationConfig.hpp"

namespace flatearth {

class Engine {
public:
  FEAPI Engine(Game &game);
  FEAPI bool Initialize();
  FEAPI bool Start();

private:
  ApplicationState _appState;
};

}

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

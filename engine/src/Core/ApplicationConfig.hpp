#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP

#include "Defines.hpp"
#include "GameTypes.hpp"

namespace flatearth {

struct ApplicationConfig {
  // Window specs
  int16 windowStartPosX, windowStartPosY, windowStartWidth, windowStartHeight;

  // Application name
  string name;
};

struct ApplicationState {
  Game &gameInstance; 
  bool isRunning;
  bool isSuspended;
  int16 width;
  int16 height;
  float64 lastTime;

  ApplicationState(Game &game) : gameInstance(game) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP

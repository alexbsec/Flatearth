#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP

#include "Defines.hpp"

namespace flatearth {

struct Game;

struct ApplicationConfig {
  // Window specs
  int32 windowStartPosX{0}, windowStartPosY{0}, windowStartWidth{1280}, windowStartHeight{920};

  // Application name
  string name;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP

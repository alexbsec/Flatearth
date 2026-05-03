#ifndef _FLATEARTH_ENGINE_CORE_CORE_TYPES_HPP
#define _FLATEARTH_ENGINE_CORE_CORE_TYPES_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/Clock.hpp"
#include "Platform/Platform.hpp"

namespace flatearth {

struct EngineState {
  Game *pGameInstance;
  bool isRunning;
  bool isSuspended;
  int32 width;
  int32 height;
  float64 lastTime;
  platform::PlatformState *platformState;
  ApplicationConfig *pAppConfig;
  clock::Clock clock;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_CORE_TYPES_HPP

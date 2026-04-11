#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP

#include "Core/Clock.hpp"
#include "Defines.hpp"
#include "GameTypes.hpp"
#include "Platform/Platform.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth {

struct ApplicationConfig {
  // Window specs
  int32 windowStartPosX, windowStartPosY, windowStartWidth, windowStartHeight;

  // Application name
  string name;
};

struct ApplicationState {
  Game *pGameInstance;
  bool isRunning;
  bool isSuspended;
  int32 width;
  int32 height;
  float64 lastTime;
  platform::PlatformState *platformState;
  ApplicationConfig appConfig;
  clock::Clock clock;

  // TODO: remove, only here to store the test texture
  resources::Texture testTexture;

  ApplicationState(Game *game) : pGameInstance(game) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_CONFIG_HPP

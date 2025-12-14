#include "Application.hpp"
#include "Core/FeMemroy.hpp"
#include "Defines.hpp"

namespace flatearth {

Engine::Engine(Game &game) : _appState(game) {
  _appState.appConfig.windowStartPosX = game.windowStartPosX;
  _appState.appConfig.windowStartPosY = game.windowStartPosY;
  _appState.appConfig.windowStartWidth = game.windowStartWidth;
  _appState.appConfig.windowStartHeight = game.windowStartHeight;
}

FeExpect<void, Error> Engine::Initialize() {
  _pPlatform = _memoryManager.Allocate<platform::Platform>(
      memory::Tag::Platform, _appState.gameInstance.gameName,
      _appState.appConfig.windowStartPosX, _appState.appConfig.windowStartPosY,
      _appState.appConfig.windowStartWidth,
      _appState.appConfig.windowStartHeight, _memoryManager);



  _appState.isRunning = FeTrue;
  _appState.isSuspended = FeFalse;
  _appState.platformState = _pPlatform->State();
  return {};
}

FeExpect<void, Error> Engine::Start() { return {}; }

} // namespace flatearth

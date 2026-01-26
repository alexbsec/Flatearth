#include "Application.hpp"
#include "Core/Clock.hpp"
#include "Core/EngineListener.hpp"
#include "Core/Event.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Error.hpp"

namespace flatearth {

Engine::Engine(Game &game)
    : _appState(game), _eventManager(_memoryManager),
      _inputManager(_eventManager),
      _frontendRenderer(&_appState, _memoryManager, _filesystem),
      _filesystem(_memoryManager) {
  _engineListener =
      _memoryManager.Allocate<event::IEventListener, EngineListener>(
          memory::Tag::Application, _eventManager, _appState,
          _frontendRenderer);
}

Engine::~Engine() { FLOG_INFO("engine shutdown gracefully"); }

FeExpect<void, Error> Engine::Initialize() {
  FILE_LOGGING(FeTrue);
  if (auto checkRes = CheckGamePrerequisites(); !checkRes.has_value()) {
    FLOG_ERROR("game has undefined callbacks: {}", checkRes.error().message);
    return FeErr{checkRes.error()};
  }

  // initialize game
  if (!_appState.gameInstance.Initialize(&_appState.gameInstance)) {
    FLOG_FATAL("game failed to initialize");
    return FeErr{Error("game Initialize() returned, cannot initialize it",
                       ErrorType::GameInitializeError)};
  }

  _appState.appConfig.windowStartPosX = _appState.gameInstance.windowStartPosX;
  _appState.appConfig.windowStartPosY = _appState.gameInstance.windowStartPosY;
  _appState.appConfig.windowStartWidth =
      _appState.gameInstance.windowStartWidth;
  _appState.appConfig.windowStartHeight =
      _appState.gameInstance.windowStartHeight;

  _pPlatform = _memoryManager.Allocate<platform::Platform>(
      memory::Tag::Platform, _appState.gameInstance.gameName,
      _appState.appConfig.windowStartPosX, _appState.appConfig.windowStartPosY,
      _appState.appConfig.windowStartWidth,
      _appState.appConfig.windowStartHeight, _memoryManager, _inputManager,
      _eventManager);

  auto platInitRes = _pPlatform->Initialize();
  if (!platInitRes.has_value()) {
    FLOG_ERROR("engine failed to initialize platform");
    return FeErr{platInitRes.error()};
  }
  _appState.platformState = _pPlatform->State();

  FeExpect<void, Error> listenerInitRes = _engineListener->Initialize();
  if (!listenerInitRes.has_value()) {
    FLOG_ERROR("engine listener failed to initialize");
    return FeErr{listenerInitRes.error()};
  }

  FeExpect<bool, Error> frontendInitRes = _frontendRenderer.Initialize();
  if (!frontendInitRes.has_value()) {
    FLOG_ERROR("frontend renderer failed to initialize: {}",
               frontendInitRes.error().message);
    return FeErr{frontendInitRes.error()};
  }

  _appState.isRunning = FeTrue;
  _appState.isSuspended = FeFalse;
  _appState.platformState = _pPlatform->State();

  FLOG_INFO("engine successfully initialized");
  return {};
}

FeExpect<void, Error> Engine::Start() {
  _appState.clock.Start();
  _appState.clock.Update();
  _appState.lastTime = _appState.clock.Elapsed();
  float64 runnigTime = 0.0;
  uint8 frameCount = 0;
  float64 targetFrameSeconds = 1.0 / 60;

  while (_appState.isRunning) {
    auto pollRes = _pPlatform->PollEvents();
    if (!pollRes.has_value()) {
      FLOG_ERROR("engine failed to poll events from platform {}",
                 pollRes.error().message);
      return FeErr{pollRes.error()};
    }

    if (!pollRes.value()) {
      FLOG_DEBUG("closing window was requested");
      break;
    }

    if (_appState.isSuspended) {
      continue;
    }

    _appState.clock.Update();
    float64 currentTime = _appState.clock.Elapsed();
    float64 deltaTime = currentTime - _appState.lastTime;
    float64 frameStartTime = clock::GetAbsoluteTime();

    // Update game
    if (!_appState.gameInstance.Update(&_appState.gameInstance, deltaTime)) {
      FLOG_FATAL("game update failed, shutting down application");
      break;
    }

    // Hardcoded just to make it up and running
    // TODO: remove
    renderer::RenderPacket packet;
    packet.deltaTime = deltaTime;
    auto drawRes = _frontendRenderer.DrawFrame(&packet);
    if (!drawRes.has_value()) {
      FLOG_ERROR("frontend renderer failed to draw frame: {}",
                 drawRes.error().message);
      return FeErr{drawRes.error()};
    }

    float64 frameEndTime = clock::GetAbsoluteTime();
    float64 frameElapsed = frameEndTime - frameStartTime;
    runnigTime += frameElapsed;
    float64 remainingSeconds = targetFrameSeconds - frameElapsed;

    if (remainingSeconds > 0.0) {
      float64 remainingMs = remainingSeconds * 1000.0;
      // hardcoded due to debbuging purposes
      bool limitFrames = FeFalse;
      if (remainingMs > 0.0 && limitFrames) {
        // sleep
      }

      frameCount++;
    }

    _inputManager.Update(deltaTime);
    _appState.lastTime = currentTime;
  }

  _appState.isRunning = FeFalse;
  return {};
}

FeExpect<void, Error> Engine::CheckGamePrerequisites() {
  if (_appState.gameInstance.Initialize == nullptr) {
    return FeErr{Error("game instance Initialize() function is not defined",
                       ErrorType::GameInitializeUndefined)};
  }

  if (_appState.gameInstance.Update == nullptr) {
    return FeErr{Error("game instance Update() function is not defined",
                       ErrorType::GameUpdateUndefined)};
  }

  if (_appState.gameInstance.OnResize == nullptr) {
    return FeErr{Error("game instance OnResize() function is not defined",
                       ErrorType::GameResizeUndefined)};
  }

  return {};
}

} // namespace flatearth

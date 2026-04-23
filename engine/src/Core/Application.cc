#include "Application.hpp"

#include <imgui.h>

#include "Core/Clock.hpp"
#include "Core/EngineListener.hpp"
#include "Core/Event.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Scene/Systems/ParticleSystem.hpp"
#include "Scene/Systems/SpriteSystem.hpp"
#include "Scene/Systems/TransformSystem.hpp"

namespace flatearth {

Engine::Engine(Game *pGame)
    : _appState(pGame), _eventManager(_memoryManager), _inputManager(_eventManager),
      _renderer(&_appState, _memoryManager, _registry, _filesystem), _filesystem(_memoryManager),
      _assetManager(_memoryManager, _filesystem), _tilemapManager(_memoryManager, _assetManager, _registry),
      _prefabManager(_memoryManager), _scheduler(_memoryManager),
      _registry(_memoryManager), _sceneManager(_memoryManager, _registry), _world(_memoryManager),
      _ctx(_memoryManager,
           _assetManager,
           _tilemapManager,
           _prefabManager,
           _inputManager,
           _registry,
           _sceneManager,
           _world) {
  _engineListener = _memoryManager.Allocate<event::IEventListener, EngineListener>(
      memory::Tag::Application, _eventManager, _renderer, _appState);
}

Engine::~Engine() {
  _renderer.Flush();
  if (_appState.pGameInstance->Unload) {
    _appState.pGameInstance->Unload(_appState.pGameInstance);
  }
  _renderer.Shutdown();
  _assetManager.Shutdown();
  _world.Shutdown();
  FLOG_INFO("engine shutdown gracefully");
}

FeExpect<void, Error> Engine::Initialize() {
  FILE_LOGGING(FeTrue);
  if (auto checkRes = CheckGamePrerequisites(); !checkRes.has_value()) {
    FLOG_ERROR("game has undefined callbacks: {}", checkRes.error().message);
    return FeErr{checkRes.error()};
  }

  _appState.pGameInstance->pCtx = &_ctx;

  // initialize game
  if (!_appState.pGameInstance->Initialize(_appState.pGameInstance)) {
    FLOG_FATAL("game failed to initialize");
    return FeErr{
        Error("game Initialize() returned, cannot initialize it", ErrorType::GameInitializeError)};
  }

  _appState.appConfig.windowStartPosX = _appState.pGameInstance->windowStartPosX;
  _appState.appConfig.windowStartPosY = _appState.pGameInstance->windowStartPosY;
  _appState.appConfig.windowStartWidth = _appState.pGameInstance->windowStartWidth;
  _appState.appConfig.windowStartHeight = _appState.pGameInstance->windowStartHeight;

  _pPlatform = _memoryManager.Allocate<platform::Platform>(memory::Tag::Platform,
                                                           _appState.pGameInstance->gameName,
                                                           _appState.appConfig.windowStartPosX,
                                                           _appState.appConfig.windowStartPosY,
                                                           _appState.appConfig.windowStartWidth,
                                                           _appState.appConfig.windowStartHeight,
                                                           _memoryManager,
                                                           _inputManager,
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

  FeExpect<bool, Error> renderInitRes = _renderer.Initialize();
  if (!renderInitRes.has_value()) {
    FLOG_ERROR("game renderer failed to initialize: {}", renderInitRes.error().message);
    return FeErr{renderInitRes.error()};
  }

  _assetManager.Initialize(&_renderer.FrontendReference());

  _appState.isRunning = FeTrue;
  _appState.isSuspended = FeFalse;
  _appState.platformState = _pPlatform->State();

  if (_appState.pGameInstance->Load) {
    if (!_appState.pGameInstance->Load(_appState.pGameInstance)) {
      FLOG_FATAL("game failed to load resources");
      return FeErr{Error("game Load() failed", ErrorType::GameInitializeError)};
    }
  }

  _world.Initialize();
  FeExpect<void, Error> registerRes = RegisterSystems();
  if (!registerRes.has_value()) {
    FLOG_ERROR("failed to register engine systems");
    return FeErr{registerRes.error()};
  }

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
      FLOG_ERROR("engine failed to poll events from platform {}", pollRes.error().message);
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
    if (!_appState.pGameInstance->Update(_appState.pGameInstance, deltaTime)) {
      FLOG_FATAL("game update failed, shutting down application");
      break;
    }

    _scheduler.Update(_registry, deltaTime);

    _renderer.BeginImGuiFrame();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(_appState.width),
                            static_cast<float>(_appState.height));
    io.DeltaTime   = static_cast<float>(deltaTime);
    int32 mx, my;
    _inputManager.GetMousePosition(mx, my);
    io.AddMousePosEvent(static_cast<float>(mx), static_cast<float>(my));
    io.AddMouseButtonEvent(0, _inputManager.IsButtonDown(input::Button::Left));
    io.AddMouseButtonEvent(1, _inputManager.IsButtonDown(input::Button::Right));
    io.AddMouseButtonEvent(2, _inputManager.IsButtonDown(input::Button::Middle));
    ImGui::NewFrame();
    if (_appState.pGameInstance->OnImGui) {
      _appState.pGameInstance->OnImGui(_appState.pGameInstance);
    }
    ImGui::Render();

    auto drawRes = _renderer.Draw(deltaTime);
    if (!drawRes.has_value()) {
      FLOG_ERROR("game renderer failed to draw frame: {}", drawRes.error().message);
      return FeErr{drawRes.error()};
    }
    _ctx.drawCallCount = _renderer.LastDrawCallCount();

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

FeExpect<void, Error> Engine::RegisterSystems() {
  _scheduler.Prune();

  auto registerRes = _scheduler.Register<systems::TransformSystem>(_memoryManager);
  if (!registerRes.has_value()) {
    FLOG_ERROR("could not register TransformSystem into scheduler");
    return FeErr{registerRes.error()};
  }

  auto spriteRes = _scheduler.Register<systems::SpriteSystem>();
  if (!spriteRes.has_value()) {
    FLOG_ERROR("could not register SpriteSystem into scheduler");
    return FeErr{spriteRes.error()};
  }

  auto physicsRes = _scheduler.Register<physics::PhysicsSystem>(_world);
  if (!physicsRes.has_value()) {
    FLOG_ERROR("could not register PhysicsSystem into scheduler");
    return FeErr{physicsRes.error()};
  }

  auto particleRes = _scheduler.Register<systems::ParticleSystem>();
  if (!particleRes.has_value()) {
    FLOG_ERROR("could not register ParticleSystem into scheduler");
    return FeErr{particleRes.error()};
  }

  auto buildRes = _scheduler.Build();
  if (!buildRes.has_value()) {
    FLOG_ERROR("failed to build scheduler");
    return FeErr{buildRes.error()};
  }

  return {};
}

FeExpect<void, Error> Engine::CheckGamePrerequisites() {
  if (_appState.pGameInstance->Initialize == nullptr) {
    return FeErr{Error("game instance Initialize() function is not defined",
                       ErrorType::GameInitializeUndefined)};
  }

  if (_appState.pGameInstance->Update == nullptr) {
    return FeErr{
        Error("game instance Update() function is not defined", ErrorType::GameUpdateUndefined)};
  }

  if (_appState.pGameInstance->OnResize == nullptr) {
    return FeErr{
        Error("game instance OnResize() function is not defined", ErrorType::GameResizeUndefined)};
  }

  return {};
}

} // namespace flatearth

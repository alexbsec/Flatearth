#include "Engine.hpp"

#include "Core/Clock.hpp"
#include "Core/EngineListener.hpp"
#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include "Physics/PhysicsSystem.hpp"
#include "Platform/Platform.hpp"
#include "Scene/Systems/AudioSystem.hpp"
#include "Scene/Systems/ParticleSystem.hpp"
#include "Scene/Systems/SpriteSystem.hpp"
#include "Scene/Systems/TransformSystem.hpp"

#include <imgui.h>

namespace flatearth {

Engine::Engine(memory::MemoryManager &mm)
    : _memoryManager(mm), _filesystem(mm), _eventManager(mm), _registry(mm), _scheduler(mm),
      _gameModule(mm, _registry), _assetsModule(mm, _filesystem, _registry),
      _coreModule(mm, _eventManager), _ctx(_coreModule, _assetsModule, _gameModule),
      _renderer(&_state, mm, _registry, _filesystem, _assetsModule.Manager()),
      _devConsole(_coreModule.Input()) {
  _engineListener = _memoryManager.Allocate<event::IEventListener, EngineListener>(
      memory::Tag::Application, _eventManager, _renderer, _state);
}

Engine::~Engine() {
  _renderer.Flush();
  if (_state.pGameInstance != nullptr && _state.pGameInstance->Unload) {
    _state.pGameInstance->Unload(_state.pGameInstance);
  }
  _renderer.Shutdown();
  _assetsModule.Shutdown();
  _coreModule.Shutdown();
  _gameModule.Shutdown();
  FLOG_INFO("engine shutdown gracefully");
}

FeExpect<void, Error> Engine::Initialize(Game &game, ApplicationConfig &config) {
  _state.pGameInstance = &game;
  _state.pAppConfig = &config;
  _state.pGameInstance->pCtx = &_ctx;

  if (_state.pGameInstance->Initialize != nullptr &&
      !_state.pGameInstance->Initialize(_state.pGameInstance)) {
    FLOG_FATAL("game failed to initialize");
    return FeErr{Error("game Initialize() returned false", ErrorType::GameInitializeError)};
  }

  // game.Initialize() sets preferred window dimensions
  config.windowStartPosX = _state.pGameInstance->windowStartPosX;
  config.windowStartPosY = _state.pGameInstance->windowStartPosY;
  config.windowStartWidth = _state.pGameInstance->windowStartWidth;
  config.windowStartHeight = _state.pGameInstance->windowStartHeight;

  _pPlatform = _memoryManager.AllocateShared<platform::Platform>(memory::Tag::Platform,
                                                                 config.name,
                                                                 config.windowStartPosX,
                                                                 config.windowStartPosY,
                                                                 config.windowStartWidth,
                                                                 config.windowStartHeight,
                                                                 _memoryManager,
                                                                 _coreModule.Input(),
                                                                 _eventManager);

  auto platInitRes = _pPlatform->Initialize().or_error("failed to initialize OS platform");
  if (platInitRes.errored()) {
    return FeErr{platInitRes.error()};
  }
  _state.platformState = _pPlatform->State();

  auto listenerRes =
      _engineListener->Initialize().or_error("failed to initialize engine event listener");
  if (listenerRes.errored()) {
    return FeErr{listenerRes.error()};
  }

  auto rendererInit = _renderer.Initialize().or_error("failed to initialize game renderer");
  if (rendererInit.errored()) {
    return FeErr{rendererInit.error()};
  }

  auto coreInit = _coreModule.Initialize().or_error("failed to initialize core module");
  if (coreInit.errored()) {
    return FeErr{coreInit.error()};
  }

  auto assetsInit = _assetsModule.Initialize(&_renderer.FrontendReference())
                        .or_error("failed to initialize assets module");
  if (assetsInit.errored()) {
    return FeErr{assetsInit.error()};
  }

  auto gameInit = _gameModule.Initialize().or_error("failed to initialize game project module");
  if (gameInit.errored()) {
    return FeErr{gameInit.error()};
  }

  _state.isRunning = FeTrue;
  _state.isSuspended = FeFalse;
  if (_state.pGameInstance->Load != nullptr && !_state.pGameInstance->Load(_state.pGameInstance)) {
    FLOG_FATAL("game failed to load resources");
    return FeErr{Error("game Load() failed", ErrorType::GameInitializeError)};
  }

  auto systemRes = RegisterSystems().or_error("failed to register engine systems");
  if (systemRes.errored()) {
    return FeErr{systemRes.error()};
  }
  _scheduler.BootSystems(_registry);

  FLOG_INFO("engine successfully initialized");
  return {};
}

FeExpect<void, Error> Engine::Start() {
  _state.clock.Start();
  _state.clock.Update();
  _state.lastTime = _state.clock.Elapsed();

  float64 runningTime = 0.0;
  uint8 frameCount = 0;
  float64 targetFrameSeconds = 1.0 / 60;

  while (_state.isRunning) {
    auto pollRes = _pPlatform->PollEvents().or_error("engine failed to poll events");
    if (pollRes.errored()) {
      return FeErr{pollRes.error()};
    }
    if (!pollRes.value()) {
      FLOG_DEBUG("closing window was requested");
      break;
    }

    if (_state.isSuspended) {
      continue;
    }

    _state.clock.Update();
    float64 currentTime = _state.clock.Elapsed();
    float64 deltaTime = currentTime - _state.lastTime;
    float64 frameStart = clock::GetAbsoluteTime();

    if (!_devConsole.IsOpen()) {
      if (_state.pGameInstance->Update != nullptr &&
          !_state.pGameInstance->Update(_state.pGameInstance, deltaTime)) {
        FLOG_FATAL("game update failed, shutting down");
        break;
      }
      _scheduler.Update(_registry, deltaTime);
    }

    _renderer.BeginImGuiFrame();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(_state.width), static_cast<float>(_state.height));
    io.DeltaTime = static_cast<float>(deltaTime);
    int32 mx, my;
    _coreModule.Input().GetMousePosition(mx, my);
    io.AddMousePosEvent(static_cast<float>(mx), static_cast<float>(my));
    io.AddMouseButtonEvent(0, _coreModule.Input().IsButtonDown(input::Button::Left));
    io.AddMouseButtonEvent(1, _coreModule.Input().IsButtonDown(input::Button::Right));
    io.AddMouseButtonEvent(2, _coreModule.Input().IsButtonDown(input::Button::Middle));
    ImGui::NewFrame();
    if (_state.pGameInstance->OnImGui != nullptr) {
      _state.pGameInstance->OnImGui(_state.pGameInstance);
    }
    _devConsole.Draw(_coreModule.KVarsRegistry());
    ImGui::Render();

    auto drawRes = _renderer.Draw(deltaTime).or_error("renderer failed to draw frame");
    if (drawRes.errored()) {
      return FeErr{drawRes.error()};
    }
    _ctx.drawCallCount = _renderer.LastDrawCallCount();

    float64 frameEnd = clock::GetAbsoluteTime();
    float64 frameElapsed = frameEnd - frameStart;
    runningTime += frameElapsed;
    float64 remaining = targetFrameSeconds - frameElapsed;

    if (remaining > 0.0) {
      float64 remainingMs = remaining * 1000.0;
      bool limitFrames = FeFalse;
      if (remainingMs > 0.0 && limitFrames) {
        // sleep
      }
      frameCount++;
    }

    _coreModule.Input().Update(deltaTime);
    _state.lastTime = currentTime;
  }

  _state.isRunning = FeFalse;
  return {};
}

FeExpect<void, Error> Engine::RegisterSystems() {
  _scheduler.Prune();

  auto transformRes = _scheduler.Register<scene::systems::TransformSystem>(_memoryManager);
  if (transformRes.errored()) {
    FLOG_ERROR("could not register TransformSystem");
    return FeErr{transformRes.error()};
  }

  auto spriteRes = _scheduler.Register<scene::systems::SpriteSystem>(_assetsModule.Animations());
  if (spriteRes.errored()) {
    FLOG_ERROR("could not register SpriteSystem");
    return FeErr{spriteRes.error()};
  }

  auto physicsRes = _scheduler.Register<physics::PhysicsSystem>(_gameModule.World());
  if (physicsRes.errored()) {
    FLOG_ERROR("could not register PhysicsSystem");
    return FeErr{physicsRes.error()};
  }
  physicsRes.value().Before<scene::systems::TransformSystem>();

  auto particleRes = _scheduler.Register<scene::systems::ParticleSystem>();
  if (particleRes.errored()) {
    FLOG_ERROR("could not register ParticleSystem");
    return FeErr{particleRes.error()};
  }

  auto audioRes = _scheduler.Register<scene::systems::AudioSystem>(_coreModule.Audio());
  if (audioRes.errored()) {
    FLOG_ERROR("could not register AudioSystem");
    return FeErr{audioRes.error()};
  }

  if (_state.pGameInstance->RegisterSystems != nullptr) {
    _state.pGameInstance->RegisterSystems(_state.pGameInstance, _scheduler);
  }

  auto buildRes = _scheduler.Build();
  if (buildRes.errored()) {
    FLOG_ERROR("failed to build scheduler");
    return FeErr{buildRes.error()};
  }

  return {};
}

FeExpect<void, Error> Engine::CheckGamePrerequisites() {
  // Add any prerequisite if needed
  return {};
}

} // namespace flatearth

#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/Event.hpp"
#include "Core/Input.hpp"
#include "Platform/Platform.hpp"
#include "Renderer/RendererFrontend.hpp"

namespace flatearth {

class Engine {
public:
  FEAPI Engine(Game &game);
  FEAPI FeExpect<void, Error> Initialize();
  FEAPI FeExpect<void, Error> Start();

private:
  FeExpect<void, Error> CheckGamePrerequisites();

private:
  ApplicationState _appState;
  FePtr<platform::Platform> _pPlatform;
  memory::MemoryManager _memoryManager;
  event::EventManager _eventManager;
  input::InputManager _inputManager;
  renderer::FrontendRenderer _frontendRenderer;
  FePtr<event::IEventListener> _engineListener;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

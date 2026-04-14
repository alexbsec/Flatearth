#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/Event.hpp"
#include "Core/Input.hpp"
#include "GameTypes.hpp"
#include "Platform/Filesystem.hpp"
#include "Platform/Platform.hpp"
#include "Resources/MaterialSystem.hpp"
#include "Resources/TextureSystem.hpp"
#include "Renderer/RendererFrontend.hpp"

namespace flatearth {

class Engine {
public:
  FEAPI Engine(Game *pGame);
  FEAPI ~Engine();
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
  platform::FileSystem _filesystem;
  renderer::FrontendRenderer _frontendRenderer;
  resources::TextureSystem _textureSystem;
  resources::MaterialSystem _materialSystem;
  FePtr<event::IEventListener> _engineListener;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

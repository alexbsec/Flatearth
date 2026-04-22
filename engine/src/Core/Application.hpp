#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "Assets/AssetManager.hpp"
#include "Assets/PrefabManager.hpp"
#include "Assets/TilemapManager.hpp"
#include "Core/ApplicationConfig.hpp"
#include "Core/EngineContext.hpp"
#include "Core/Event.hpp"
#include "Core/Input.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Scheduler/SystemScheduler.hpp"
#include "GameTypes.hpp"
#include "Physics/FlatearthWorld.hpp"
#include "Platform/Filesystem.hpp"
#include "Platform/Platform.hpp"
#include "Renderer/GameRenderer.hpp"
#include "Scene/SceneManager.hpp"

namespace flatearth {

class Engine {
public:
  FEAPI Engine(Game *pGame);
  FEAPI ~Engine();
  FEAPI FeExpect<void, Error> Initialize();
  FEAPI FeExpect<void, Error> Start();

private:
  FeExpect<void, Error> RegisterSystems();
  FeExpect<void, Error> CheckGamePrerequisites();

private:
  ApplicationState _appState;
  FePtr<platform::Platform> _pPlatform;
  memory::MemoryManager _memoryManager;
  event::EventManager _eventManager;
  input::InputManager _inputManager;
  platform::FileSystem _filesystem;
  assets::AssetManager   _assetManager;
  assets::TilemapManager _tilemapManager;
  assets::PrefabManager _prefabManager;
  ecs::SystemScheduler   _scheduler;
  ecs::Registry          _registry;
  renderer::GameRenderer _renderer;
  scene::SceneManager    _sceneManager;
  physics::FlatearthWorld _world;
  FePtr<event::IEventListener> _engineListener;
  EngineContext _ctx;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

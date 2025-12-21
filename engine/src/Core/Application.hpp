#ifndef _FLATEARTH_ENGINE_CORE_APPLICATION_HPP
#define _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

#include "ApplicationConfig.hpp"
#include "Core/Event.hpp"
#include "Core/FeMemroy.hpp"
#include "Core/Input.hpp"
#include "Defines.hpp"
#include "Platform/Platform.hpp"

namespace flatearth {

class EngineListener final : public event::IEventListener {
public:
  explicit EngineListener(event::EventManager &eventManager,
                          ApplicationState &appState);

  ~EngineListener();
  // Inherited overrrides
  FeExpect<void, Error> Initialize() override;
  FeExpect<bool, Error> OnResize(const event::EventDispatchContext &ctx,
                                 const event::EventContext &eventCtx) override;
  FeExpect<bool, Error> OnKey(const event::EventDispatchContext &ctx,
                              const event::EventContext &eventCtx) override;
  FeExpect<bool, Error> OnEvent(const event::EventDispatchContext &ctx,
                                const event::EventContext &eventCtx) override;
  FeExpect<bool, Error> OnButton(const event::EventDispatchContext &ctx,
                                 const event::EventContext &eventCtx) override;
  FeExpect<bool, Error>
  OnMouseMove(const event::EventDispatchContext &ctx,
              const event::EventContext &eventCtx) override;

private:
  FeExpect<void, Error> WireEvents();
  FeExpect<bool, Error> OnKeyPress(const event::EventDispatchContext &ctx,
                                   const event::EventContext &eventCtx);
  FeExpect<bool, Error> OnKeyRelease(const event::EventDispatchContext &ctx,
                                     const event::EventContext &eventCtx);

  FeExpect<bool, Error> OnButtonPress(const event::EventDispatchContext &ctx,
                                      const event::EventContext &eventCtx);
  FeExpect<bool, Error> OnButtonRelease(const event::EventDispatchContext &ctx,
                                        const event::EventContext &eventCtx);

private:
  ApplicationState &_appState;
  event::EventManager &_eventManager;
};

class Engine {
public:
  FEAPI Engine(Game &game);
  FEAPI FeExpect<void, Error> Initialize();
  FEAPI FeExpect<void, Error> Start();

private:
  ApplicationState _appState;
  FePtr<platform::Platform> _pPlatform;
  memory::MemoryManager _memoryManager;
  event::EventManager _eventManager;
  input::InputManager _inputManager;
  FePtr<event::IEventListener> _engineListener;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_APPLICATION_HPP

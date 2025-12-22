#ifndef _FLATEARTH_ENGINE_ENGINE_LISTENER_HPP
#define _FLATEARTH_ENGINE_ENGINE_LISTENER_HPP

#include "ApplicationConfig.hpp"
#include "Core/Event.hpp"

namespace flatearth {


class EngineListener final : public event::ListenerAdapter {
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

}

#endif // _FLATEARTH_ENGINE_ENGINE_LISTENER_HPP

#include "Event.hpp"

#include "Core/Logger.hpp"

#include <cassert>

namespace flatearth::event {

EventManager::EventManager(memory::MemoryManager &memManager) : _memoryManager(memManager) {
}

EventManager::~EventManager() {
}

FeExpect<uint64, Error> EventManager::RegisterEvent(SystemEventCode code,
                                                    IEventListener *listener) {
  using containers::DArray;

  // on registration, whoever registered must guarantee ownership of listener
  // and its lifetime (dangling pointer possibility)
  if (listener == nullptr) {
    // defensiveness
    return FeErr{
        Error("cannot register event with null IEventListener", ErrorType::NullptrException)};
  }

  uint8 numberCode = ToUnderlying(code);
  if (_state.registered[numberCode].pEvents == nullptr) {
    _state.registered[numberCode].pEvents =
        _memoryManager.Allocate<DArray<RegisteredEvent>>(memory::Tag::DArray, _memoryManager);
  }

  DArray<RegisteredEvent> &events = *_state.registered[numberCode].pEvents;
  uint64 registeredCount = events.Length();
  for (uint64 i = 0; i < registeredCount; i++) {
    if (events[i].listener == listener) {
      FLOG_WARN("listener for event {} is already registered", events[i].id);
      return FeFalse;
    }
  }

  uint64 eventId = _currentId++;
  RegisteredEvent newEvent{
      .id = eventId,
      .listener = listener,
  };

  _state.registered[numberCode].pEvents->Push(newEvent);
  return eventId;
}

FeExpect<bool, Error> EventManager::UnregisterEvent(SystemEventCode code,
                                                    IEventListener *listener) {
  using containers::DArray;
  if (listener == nullptr) {
    // defensiveness
    return FeErr{
        Error("cannot unregister event with null IEventListener", ErrorType::NullptrException)};
  }

  uint8 numberCode = ToUnderlying(code);
  if (_state.registered[numberCode].pEvents == nullptr) {
    return FeFalse;
  }

  DArray<RegisteredEvent> &events = *_state.registered[numberCode].pEvents;
  uint64 count = events.Length();
  for (uint64 i = 0; i < count; i++) {
    RegisteredEvent &eventRef = events[i];
    if (eventRef.listener != listener) {
      continue;
    }

    auto result = events.PopAt(i);
    if (result.errored()) {
      FLOG_ERROR("failed to pop event from DArray at index {}", i);
      return FeErr{result.error()};
    }
    return FeTrue;
  }

  return FeFalse;
}

FeExpect<bool, Error>
EventManager::FireEvent(SystemEventCode code, void *sender, const EventContext &eventCtx) {
  using containers::DArray;
  uint8 numberCode = ToUnderlying(code);
  DArray<RegisteredEvent> *pEvents = _state.registered[numberCode].pEvents.get();
  if (pEvents == nullptr) {
    return FeFalse;
  }

  for (uint64 i = 0; i < pEvents->Length(); i++) {
    RegisteredEvent &event = (*pEvents)[i];

    EventDispatchContext dispatchCtx{
        .code = code,
        .sender = sender,
        .listener = event.listener,
    };

    FeExpect<bool, Error> res = DecideAndDispatch(code, event.listener, dispatchCtx, eventCtx);

    if (res.errored()) {
      return FeErr{res.error()};
    }

    if (res.value()) {
      return FeTrue;
    }
  }

  return FeFalse;
}

FeExpect<bool, Error>
EventManager::Broadcast(SystemEventCode code, void *sender, const EventContext &eventCtx) {
  using containers::DArray;
  uint8 numberCode = ToUnderlying(code);
  DArray<RegisteredEvent> *pEvents = _state.registered[numberCode].pEvents.get();
  if (pEvents == nullptr) {
    return FeFalse;
  }

  uint64 consumeCount = 0;
  uint64 ignoreCount = 0;
  uint64 errCount = 0;
  for (uint64 i = 0; i < pEvents->Length(); i++) {
    RegisteredEvent &event = (*pEvents)[i];

    EventDispatchContext dispatchCtx{
        .code = code,
        .sender = sender,
        .listener = event.listener,
    };

    FeExpect<bool, Error> res = DecideAndDispatch(code, event.listener, dispatchCtx, eventCtx);

    if (res.errored()) {
      FLOG_ERROR("callback for event {} return error: {}", event.id, res.error().message);
      errCount++;
      continue;
    }

    if (res.value()) {
      consumeCount++;
    } else {
      ignoreCount++;
    }
  }

  FLOG_INFO("broadcast finished: {} consumed, {} ignored, {} errored",
            consumeCount,
            ignoreCount,
            errCount);

  return consumeCount > 0;
}

uint64 EventManager::CountEvents(SystemEventCode code) const {
  uint8 numberCode = ToUnderlying(code);
  return _state.registered[numberCode].pEvents == nullptr
             ? 0
             : _state.registered[numberCode].pEvents->Length();
}

FeExpect<bool, Error> EventManager::DecideAndDispatch(SystemEventCode code,
                                                      IEventListener *listener,
                                                      const EventDispatchContext &ctx,
                                                      const EventContext &eventCtx) {

  FeExpect<bool, Error> res;
  switch (code) {
    case SystemEventCode::WindowResized:
      res = listener->OnResize(ctx, eventCtx);
      break;
    case SystemEventCode::KeyPressed:
    case SystemEventCode::KeyReleased:
      res = listener->OnKey(ctx, eventCtx);
      break;
    case SystemEventCode::ApplicationQuit:
      res = listener->OnEvent(ctx, eventCtx);
      break;
    case SystemEventCode::ButtonPressed:
    case SystemEventCode::ButtonReleased:
      res = listener->OnButton(ctx, eventCtx);
      break;
    case SystemEventCode::MouseMoved:
      res = listener->OnMouseMove(ctx, eventCtx);
      break;
    default:
      FLOG_ERROR("event code {} is not considered", ToUnderlying(code));
      return FeErr{Error("unbinded event code", ErrorType::Unknown)};
  }

  return res;
}

} // namespace flatearth::event

#include "Event.hpp"
#include "Core/Logger.hpp"
#include <format>

namespace flatearth::event {

EventManager::EventManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

EventManager::~EventManager() {}

FeExpect<uint64, Error> EventManager::RegisterEvent(SystemEventCode code,
                                                    void *listener,
                                                    EventCallbackFn callback) {
  uint8 numberCode = ToUnderlying(code);
  if (_state.registered[numberCode].events == nullptr) {
    _state.registered[numberCode].events =
        _memoryManager.Allocate<containers::DArray<RegisteredEvent>>(
            memory::Tag::DArray);
  }

  uint64 registeredCount = _state.registered[numberCode].events->Length();
  uint64 eventId = _currentId++;
  RegisteredEvent newEvent{
      .id = eventId,
      .listener = listener,
      .callback = callback,
  };

  _state.registered[numberCode].events->Push(newEvent);
  return eventId;
}

FeExpect<bool, Error>
EventManager::UnregisterEvent(SystemEventCode code, uint64 eventId,
                              void *listener, EventCallbackFn /*callback*/) {
  uint8 numberCode = ToUnderlying(code);

  if (_state.registered[numberCode].events == nullptr) {
    return FeFalse;
  }

  auto &arr = *_state.registered[numberCode].events;
  uint64 count = arr.Length();

  for (uint64 i = 0; i < count; i++) {
    RegisteredEvent &eventRef = arr[i];
    if (eventRef.id != eventId) {
      continue;
    }

    auto result = arr.PopAt(i);
    if (!result.has_value()) {
      FLOG_ERROR("failed to pop event from DArray at index {}", i);
      return FeErr{result.error()};
    }
    return FeTrue;
  }

  return FeFalse;
}

FeExpect<bool, Error> EventManager::FireEvent(SystemEventCode code,
                                              void *sender,
                                              EventContext eventCtx) {
  uint8 numberCode = ToUnderlying(code);

  if (_state.registered[numberCode].events == nullptr) {
    return FeFalse;
  }

  auto &arr = *_state.registered[numberCode].events;
  uint64 count = arr.Length();
  bool handled = false;

  for (uint64 i = 0; i < count; i++) {
    RegisteredEvent &eventRef = arr[i];
    bool result = eventRef.callback(code, sender, eventRef.listener, eventCtx);

    // Convention: true = event consumed
    if (result) {
      handled = true;
    }
  }

  return handled ? FeTrue : FeFalse;
}

} // namespace flatearth::event

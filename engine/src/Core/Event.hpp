#ifndef _FLATEARTH_ENGINE_CORE_EVENT_HPP
#define _FLATEARTH_ENGINE_CORE_EVENT_HPP

#include "Containers/DArray.hpp"
#include "Core/FeMemroy.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include <cstddef>
#include <cstring>
namespace flatearth::event {

enum class SystemEventCode : uint16 {
  ApplicationQuit = 0x01,
  KeyPressed = 0x02,
  KeyReleased = 0x03,
  ButtonPressed = 0x04,
  ButtonReleased = 0x05,
  MouseMoved = 0x06,
  MouseWheel = 0x07,
  WindowResized = 0x08,
};

inline constexpr uint16 ToUnderlying(SystemEventCode code) {
  return static_cast<uint16>(code);
}

struct EventPayload {
  alignas(16) std::byte storage[16] = {};
};

enum class EventLayout : uint8 {
  Null,
  Int64x2,
  Uint64x2,
  Float64x2,
  Int32x4,
  Uint32x4,
  Float32x4,
  Int16x8,
  Uint16x8,
  Int8x16,
  Uint8x16,
  Charx16,
  MaxLayouts,
};

constexpr uint16 cMaxEvents = 256;

struct EventContext {
  EventLayout layout = EventLayout::Null;
  EventPayload payload{};

  template <typename T> inline void Set(EventLayout l, const T &val) {
    layout = l;
    static_assert(sizeof(T) <= 16);
    std::memcpy(payload.storage, &val, sizeof(T));
  }

  template <typename T> inline T Get() const {
    static_assert(sizeof(T) <= 16);
    T out{};
    std::memcpy(&out, payload.storage, sizeof(T));
    return out;
  }
};

using EventCallbackFn = std::function<bool(
    SystemEventCode code, void *sender, void *listener, const EventContext &)>;

struct RegisteredEvent {
  uint64 id;
  void *listener;
  EventCallbackFn callback;
};

struct EventCodeEntry {
  FePtr<containers::DArray<RegisteredEvent>> events;
};

struct EventSystemState {
  std::array<EventCodeEntry, cMaxEvents> registered;
};

class EventManager {
public:
  FEAPI explicit EventManager(memory::MemoryManager &memManager);
  FEAPI ~EventManager();

  FEAPI FeExpect<uint64, Error>
  RegisterEvent(SystemEventCode code, void *listener, EventCallbackFn callback);

  FEAPI FeExpect<bool, Error> UnregisterEvent(SystemEventCode code,
                                              uint64 eventId,
                                              void *listener,
                                              EventCallbackFn callback);

  FEAPI FeExpect<bool, Error> FireEvent(SystemEventCode code, void *sender,
                                        EventContext eventCtx);

  uint64 CountEvents(SystemEventCode code) const;

private:
  EventSystemState _state;
  memory::MemoryManager &_memoryManager;

  uint64 _currentId{0};
};

} // namespace flatearth::event

#endif // _FLATEARTH_ENGINE_CORE_EVENT_HPP

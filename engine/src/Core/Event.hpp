#ifndef _FLATEARTH_ENGINE_CORE_EVENT_HPP
#define _FLATEARTH_ENGINE_CORE_EVENT_HPP

#include "Containers/DArray.hpp"
#include "Core/FeMemory.hpp"
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

struct EventDispatchContext {
  SystemEventCode code;
  void *sender, *listener;
};

class IEventListener {
public:
  virtual FeExpect<void, Error> Initialize() = 0;

  virtual FeExpect<bool, Error> OnResize(const EventDispatchContext &ctx,
                                         const EventContext &eventCtx) = 0;
  virtual FeExpect<bool, Error> OnKey(const EventDispatchContext &ctx,
                                      const EventContext &eventCtx) = 0;
  virtual FeExpect<bool, Error> OnEvent(const EventDispatchContext &ctx,
                                        const EventContext &eventCtx) = 0;

  virtual FeExpect<bool, Error> OnButton(const EventDispatchContext &ctx,
                                         const EventContext &eventCtx) = 0;

  virtual FeExpect<bool, Error> OnMouseMove(const EventDispatchContext &ctx,
                                            const EventContext &eventCtx) = 0;

protected:
  virtual ~IEventListener() = default;
};

class ListenerAdapter : public IEventListener {
public:
  FeExpect<void, Error> Initialize() override {
    return {};
  }

  FeExpect<bool, Error> OnResize(const EventDispatchContext &ctx,
                                 const EventContext &eventCtx) override {
    return FeFalse; 
  }

  FeExpect<bool, Error> OnKey(const EventDispatchContext &ctx,
                              const EventContext &eventCtx) override {
    return FeFalse;
  }

  FeExpect<bool, Error> OnEvent(const EventDispatchContext &ctx,
                                const EventContext &eventCtx) override {
    return FeFalse;
  }

  FeExpect<bool, Error> OnButton(const EventDispatchContext &ctx,
                                 const EventContext &eventCtx) override {
    return FeFalse;
  }

  FeExpect<bool, Error> OnMouseMove(const EventDispatchContext &ctx,
                                    const EventContext &eventCtx) override {
    return FeFalse;
  }
};

struct RegisteredEvent {
  uint64 id;
  IEventListener *listener;
};

struct EventCodeEntry {
  FePtr<containers::DArray<RegisteredEvent>> pEvents;
};

struct EventSystemState {
  std::array<EventCodeEntry, cMaxEvents> registered;
};

// EventManager is NOT thread-safe.
// All calls must occur on the owning thread.
class EventManager {
public:
  FEAPI explicit EventManager(memory::MemoryManager &memManager);
  FEAPI ~EventManager();

  /**
   * Registers a callback to listen for events with the specified code.
   * Duplicate listener/callback pairs will not be re-registered.
   *
   * @param code The event code to listen for.
   * @param listener A pointer to the listener instance (optional, can be
   * nullptr).
   * @param callback The callback function to invoke when the event is fired.
   * @returns True if the event was successfully registered, false otherwise,
   * return Error struct on error
   */
  FEAPI FeExpect<uint64, Error> RegisterEvent(SystemEventCode code,
                                              IEventListener *listener);

  /**
   * Unregisters a callback for the specified event code.
   * If no matching registration is found, this function does nothing.
   *
   * @param code The event code to unregister from.
   * @param listener A pointer to the listener instance (optional, can be
   * nullptr).
   * @param callback The callback function to be unregistered.
   * @returns True if the event was successfully unregistered, false otherwise,
   * returns Error struct on error.
   */
  FEAPI FeExpect<bool, Error> UnregisterEvent(SystemEventCode code,
                                              IEventListener *listener);

  /**
   * Fires an event to all listeners registered for the specified code.
   * If a callback returns true, the event is considered handled, and no further
   * callbacks are invoked.
   *
   * @param code The event code to fire.
   * @param sender A pointer to the sender instance (optional, can be nullptr).
   * @param context The event context data to pass to listeners.
   * @returns True if the event was handled by any listener, false otherwise,
   * returns Error struct on error.
   */
  FEAPI FeExpect<bool, Error> FireEvent(SystemEventCode code, void *sender,
                                        const EventContext &eventCtx);

  /**
   * Broadcasts an event to all listeners registered for the specified event
   * code.
   *
   * Unlike FireEvent, this function always invokes every registered listener,
   * regardless of whether individual listeners consume the event.
   *
   * Listeners are invoked sequentially in registration order. Each listener
   * receives a temporary dispatch context describing the event source and
   * target.
   *
   * A listener may:
   *  - return `true`  to indicate the event was consumed;
   *  - return `false` to indicate the event was ignored;
   *  - return an error, which is logged and does not interrupt the broadcast.
   *
   * @param code    The event code to broadcast.
   * @param sender  Pointer to the event sender (optional, may be nullptr).
   * @param eventCtx Immutable event payload passed to listeners.
   *
   * @returns
   *  - true  : if at least one listener consumed the event.
   *  - false : if no listener consumed the event, including when no listeners
   *            are registered for the specified code.
   *
   * @note Errors returned by individual listeners do not abort the broadcast.
   * @note Modifying event registrations during broadcast is undefined behavior.
   */
  FEAPI FeExpect<bool, Error> Broadcast(SystemEventCode code, void *sender,
                                        const EventContext &eventCtx);

  uint64 CountEvents(SystemEventCode code) const;

private:
  FeExpect<bool, Error> DecideAndDispatch(SystemEventCode code,
                                          IEventListener *listener,
                                          const EventDispatchContext &ctx,
                                          const EventContext &eventCtx);

private:
  EventSystemState _state;
  memory::MemoryManager &_memoryManager;
  uint64 _currentId{0};
};

// 64-bit (2 elements = 16 bytes)
using Int64x2 = std::array<int64, 2>;
using Uint64x2 = std::array<uint64, 2>;
using Float64x2 = std::array<float64, 2>;

// 32-bit (4 elements = 16 bytes)
using Int32x4 = std::array<int32, 4>;
using Uint32x4 = std::array<uint32, 4>;
using Float32x4 = std::array<float32, 4>;

// 16-bit (8 elements = 16 bytes)
using Int16x8 = std::array<int16, 8>;
using Uint16x8 = std::array<uint16, 8>;

// 8-bit (16 elements = 16 bytes)
using Int8x16 = std::array<int8, 16>;
using Uint8x16 = std::array<uint8, 16>;
using Charx16 = std::array<char, 16>;

} // namespace flatearth::event

#endif // _FLATEARTH_ENGINE_CORE_EVENT_HPP

#include "Input.hpp"

#include "Core/Event.hpp"

namespace flatearth::input {

InputManager::InputManager(event::EventManager &eventManager) : _eventManager(eventManager) {
}

void InputManager::Update(float64 deltaTime) {
  _state.keyboardPrevious = _state.keyboardCurrent;
  _state.mousePrevious = _state.mouseCurrent;
}

FeExpect<bool, Error> InputManager::ProcessKey(Keys key, bool pressed) {
  using namespace event;
  if (_state.keyboardCurrent.keys[key] == pressed) {
    return FeFalse;
  }

  _state.keyboardCurrent.keys[key] = pressed;
  EventContext eventCtx;
  eventCtx.Set(EventLayout::Uint16x8, static_cast<uint16>(key));
  SystemEventCode code = pressed ? SystemEventCode::KeyPressed : SystemEventCode::KeyReleased;
  return _eventManager.FireEvent(code, nullptr, eventCtx);
}

FeExpect<bool, Error> InputManager::ProcessButton(Button button, bool pressed) {
  using namespace event;
  uint16 numberButton = static_cast<uint16>(button);
  if (_state.mouseCurrent.buttons[numberButton] == pressed) {
    return FeFalse;
  }

  _state.mouseCurrent.buttons[numberButton] = pressed;
  EventContext eventCtx;
  eventCtx.Set(EventLayout::Uint16x8, numberButton);
  SystemEventCode code = pressed ? SystemEventCode::ButtonPressed : SystemEventCode::ButtonReleased;

  return _eventManager.FireEvent(code, nullptr, eventCtx);
}

FeExpect<bool, Error> InputManager::ProcessMouseMove(int16 x, int16 y) {
  using namespace event;
  if (_state.mouseCurrent.x == x && _state.mouseCurrent.y == y) {
    return FeFalse;
  }

  _state.mouseCurrent.x = x;
  _state.mouseCurrent.y = y;

  Int16x8 mousePos{
      x,
      y,
      0,
      0,
  };

  EventContext eventCtx;
  eventCtx.Set(EventLayout::Int16x8, mousePos);
  SystemEventCode code = SystemEventCode::MouseMoved;
  return _eventManager.FireEvent(code, nullptr, eventCtx);
}

} // namespace flatearth::input

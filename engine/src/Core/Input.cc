#include "Input.hpp"

#include "Core/Event.hpp"

namespace flatearth::input {

uint16 KeyIndex(Keys key) noexcept {
  return static_cast<uint16>(key);
}

uint16 ButtonIndex(Button button) noexcept {
  return static_cast<uint16>(button);
}

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

bool InputManager::IsKeyDown(Keys key) {
  return _state.keyboardCurrent.keys[KeyIndex(key)];
}

bool InputManager::IsKeyUp(Keys key) {
  return !_state.keyboardCurrent.keys[KeyIndex(key)];
}

bool InputManager::WasKeyDown(Keys key) {
  return _state.keyboardPrevious.keys[KeyIndex(key)];
}

bool InputManager::WasKeyUp(Keys key) {
  return !_state.keyboardPrevious.keys[KeyIndex(key)];
}

bool InputManager::IsButtonDown(Button button) {
  return _state.mouseCurrent.buttons[ButtonIndex(button)];
}

bool InputManager::IsButtonUp(Button button) {
  return !_state.mouseCurrent.buttons[ButtonIndex(button)];
}

bool InputManager::WasButtonDown(Button button) {
  return _state.mousePrevious.buttons[ButtonIndex(button)];
}

bool InputManager::WasButtonUp(Button button) {
  return !_state.mousePrevious.buttons[ButtonIndex(button)];
}


} // namespace flatearth::input

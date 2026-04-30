#include "EngineListener.hpp"

#include "Core/Logger.hpp"
#include "Renderer/GameRenderer.hpp"
#include "GameTypes.hpp"

namespace flatearth {

constexpr stringv cUngracefulShutdown = "engine did not shutdown gracefully";

const char *KeyToString(input::Keys key);
constexpr string RegisterFailedMessageFor(stringv system);

EngineListener::EngineListener(event::EventManager &eventManager,
                               renderer::GameRenderer &renderer,
                               EngineState &engState)
    : _eventManager(eventManager), _engState(engState), _renderer(renderer) {
}

EngineListener::~EngineListener() {
  if (!_allInitialized) {
    FLOG_INFO("engine listener successfully shutdown");
    return;
  }

  using event::SystemEventCode;
  auto appQuitUnregisterRes = _eventManager.UnregisterEvent(SystemEventCode::ApplicationQuit, this)
      .or_error(cUngracefulShutdown);
  if (appQuitUnregisterRes.errored() || !appQuitUnregisterRes.value()) {
    return;
  }

  auto keyPressUnregisterRes = _eventManager.UnregisterEvent(SystemEventCode::KeyPressed, this)
      .or_error(cUngracefulShutdown);
  if (keyPressUnregisterRes.errored() || !keyPressUnregisterRes.value()) {
    return;
  }

  auto keyReleaseUnregisterRes = _eventManager.UnregisterEvent(SystemEventCode::KeyReleased, this)
      .or_error(cUngracefulShutdown);
  if (keyReleaseUnregisterRes.errored() || !keyReleaseUnregisterRes.value()) {
    return;
  }

  auto resizeUnregisterRes = _eventManager.UnregisterEvent(SystemEventCode::WindowResized, this)
      .or_error(cUngracefulShutdown);
  if (resizeUnregisterRes.errored() || !resizeUnregisterRes.value()) {
    return;
  }

  _eventManager.UnregisterEvent(SystemEventCode::ButtonPressed, this)
      .or_log_error(cUngracefulShutdown);

  _eventManager.UnregisterEvent(SystemEventCode::ButtonReleased, this)
      .or_log_error(cUngracefulShutdown);

  _eventManager.UnregisterEvent(SystemEventCode::MouseMoved, this)
      .or_log_error(cUngracefulShutdown);

  _allInitialized = FeFalse;
  FLOG_INFO("engine listener successfully shutdown");
}

FeExpect<void, Error> EngineListener::Initialize() {
  return WireEvents();
}

FeExpect<bool, Error> EngineListener::OnResize(const event::EventDispatchContext &ctx,
                                               const event::EventContext &eventCtx) {
  if (ctx.code != event::SystemEventCode::WindowResized) {
    return FeFalse;
  }

  auto resizeEvent = eventCtx.Get<event::Uint32x4>();
  uint32 width = resizeEvent[0];
  uint32 height = resizeEvent[1];

  if (width == _engState.width && height == _engState.height) {
    return FeFalse;
  }

  _engState.width = width;
  _engState.height = height;

  FLOG_DEBUG("window resizing: {}x{}", width, height);

  if (width == 0 || height == 0) {
    FLOG_INFO("window minimized, suspending application");
    _engState.isSuspended = FeTrue;
    return FeTrue;
  }

  if (_engState.isSuspended) {
    FLOG_INFO("window restored, resuming application.");
    _engState.isSuspended = FeFalse;
  }

  if (_engState.pGameInstance->OnResize != nullptr &&
      !_engState.pGameInstance->OnResize(_engState.pGameInstance, width, height)) {
    return FeErr{Error("game failed to resize", ErrorType::GameResizeError)};
  }

  auto res = _renderer.FrontendReference().OnResize(width, height);
  if (res.errored()) {
    FLOG_ERROR("renderer failed to resize to {}x{}", width, height);
    return FeFalse;
  }

  return FeTrue;
}

FeExpect<bool, Error> EngineListener::OnKey(const event::EventDispatchContext &ctx,
                                            const event::EventContext &eventCtx) {
  using event::SystemEventCode;
  if (ctx.code != SystemEventCode::KeyPressed && ctx.code != SystemEventCode::KeyReleased) {
    return FeFalse;
  }

  return ctx.code == SystemEventCode::KeyPressed ? OnKeyPress(ctx, eventCtx)
                                                 : OnKeyRelease(ctx, eventCtx);
}

FeExpect<bool, Error> EngineListener::OnEvent(const event::EventDispatchContext &ctx,
                                              const event::EventContext &eventCtx) {
  if (ctx.code == event::SystemEventCode::ApplicationQuit) {
    _engState.isRunning = FeFalse;
    return FeTrue;
  }

  if (ctx.code != event::SystemEventCode::FileLoaded) {
    return FeFalse;
  }

  return FeTrue;
}

FeExpect<bool, Error> EngineListener::OnButton(const event::EventDispatchContext &ctx,
                                               const event::EventContext &eventCtx) {
  using event::SystemEventCode;

  if (ctx.code != SystemEventCode::ButtonPressed && ctx.code != SystemEventCode::ButtonReleased) {
    return FeFalse;
  }

  return ctx.code == SystemEventCode::ButtonPressed ? OnButtonPress(ctx, eventCtx)
                                                    : OnButtonRelease(ctx, eventCtx);
}

FeExpect<bool, Error> EngineListener::OnMouseMove(const event::EventDispatchContext &ctx,
                                                  const event::EventContext &eventCtx) {
  using event::SystemEventCode;
  if (ctx.code != SystemEventCode::MouseMoved) {
    return FeFalse;
  }

  event::Int16x8 mousePos = eventCtx.Get<event::Int16x8>();
  return FeTrue;
}

FeExpect<void, Error> EngineListener::WireEvents() {
  using event::SystemEventCode;

  auto resizeRegisterRes = _eventManager.RegisterEvent(SystemEventCode::WindowResized, this)
      .or_error("failed to register resize event");
  if (resizeRegisterRes.errored()) {
    return FeErr{resizeRegisterRes.error()};
  }

  auto keyPressRegisterRes = _eventManager.RegisterEvent(SystemEventCode::KeyPressed, this)
      .or_error("failed to register key press event");
  if (keyPressRegisterRes.errored()) {
    return FeErr{keyPressRegisterRes.error()};
  }

  auto keyReleasedRegisterRes = _eventManager.RegisterEvent(SystemEventCode::KeyReleased, this)
      .or_error("failed to register key release event");
  if (keyReleasedRegisterRes.errored()) {
    return FeErr{keyReleasedRegisterRes.error()};
  }

  auto appQuitRegisterRes = _eventManager.RegisterEvent(SystemEventCode::ApplicationQuit, this)
      .or_error("failed to register application quit event");
  if (appQuitRegisterRes.errored()) {
    return FeErr{appQuitRegisterRes.error()};
  }

  auto buttonPressRegisterRes = _eventManager.RegisterEvent(SystemEventCode::ButtonPressed, this)
      .or_error("failed to register mouse button press event");
  if (buttonPressRegisterRes.errored()) {
    return FeErr{buttonPressRegisterRes.error()};
  }

  auto buttonReleaseRegisterRes = _eventManager.RegisterEvent(SystemEventCode::ButtonReleased, this)
      .or_error("failed to register mouse button release event");
  if (buttonReleaseRegisterRes.errored()) {
    return FeErr{buttonReleaseRegisterRes.error()};
  }

  auto mouseMoveRegisterRes = _eventManager.RegisterEvent(SystemEventCode::MouseMoved, this)
      .or_error("failed to register mouse move event");
  if (mouseMoveRegisterRes.errored()) {
    return FeErr{mouseMoveRegisterRes.error()};
  }

  _allInitialized = FeTrue;
  FLOG_INFO("all engine events registered and ready to dispatch");
  return {};
}

FeExpect<bool, Error> EngineListener::OnKeyPress(const event::EventDispatchContext &ctx,
                                                 const event::EventContext &eventCtx) {
  using event::SystemEventCode;
  using event::Uint16x8;
  Uint16x8 keyContext = eventCtx.Get<Uint16x8>();
  input::Keys keyCode = static_cast<input::Keys>(keyContext[0]);
  if (keyCode == input::Keys::KEY_ESCAPE) {
    return _eventManager.FireEvent(SystemEventCode::ApplicationQuit, this, eventCtx);
  }

  return FeTrue;
}

FeExpect<bool, Error> EngineListener::OnKeyRelease(const event::EventDispatchContext &ctx,
                                                   const event::EventContext &eventCtx) {
  using event::SystemEventCode;
  using event::Uint16x8;

  Uint16x8 keyContext = eventCtx.Get<Uint16x8>();
  input::Keys keyCode = static_cast<input::Keys>(keyContext[0]);
  if (keyCode == input::Keys::KEY_ESCAPE) {
    return _eventManager.FireEvent(SystemEventCode::ApplicationQuit, this, eventCtx);
  }

  return FeTrue;
}

FeExpect<bool, Error> EngineListener::OnButtonPress(const event::EventDispatchContext &ctx,
                                                    const event::EventContext &eventCtx) {
  using event::SystemEventCode;
  using event::Uint16x8;

  Uint16x8 buttonContext = eventCtx.Get<Uint16x8>();
  // NOTE to get button: auto button = static_cast<input::Button>(buttonContext[0]);

  return FeTrue;
}

FeExpect<bool, Error> EngineListener::OnButtonRelease(const event::EventDispatchContext &ctx,
                                                      const event::EventContext &eventCtx) {
  using event::SystemEventCode;
  using event::Uint16x8;

  Uint16x8 buttonContext = eventCtx.Get<Uint16x8>();
  // NOTE to cast mouse click: auto button = static_cast<input::Button>(buttonContext[0]);

  return FeTrue;
}

const char *KeyToString(input::Keys key) {
  using namespace input;

  switch (key) {
    case KEY_0:
      return "0";
    case KEY_1:
      return "1";
    case KEY_2:
      return "2";
    case KEY_3:
      return "3";
    case KEY_4:
      return "4";
    case KEY_5:
      return "5";
    case KEY_6:
      return "6";
    case KEY_7:
      return "7";
    case KEY_8:
      return "8";
    case KEY_9:
      return "9";
    case KEY_BACKSPACE:
      return "Backspace";
    case KEY_ENTER:
      return "Enter";
    case KEY_TAB:
      return "Tab";
    case KEY_SHIFT:
      return "Shift";
    case KEY_CONTROL:
      return "Control";
    case KEY_PAUSE:
      return "Pause";
    case KEY_CAPITAL:
      return "Caps Lock";
    case KEY_ESCAPE:
      return "Escape";
    case KEY_CONVERT:
      return "Convert";
    case KEY_NONCONVERT:
      return "Nonconvert";
    case KEY_ACCEPT:
      return "Accept";
    case KEY_MODECHANGE:
      return "Mode Change";
    case KEY_SPACE:
      return "Space";
    case KEY_PRIOR:
      return "Page Up";
    case KEY_NEXT:
      return "Page Down";
    case KEY_END:
      return "End";
    case KEY_HOME:
      return "Home";
    case KEY_LEFT:
      return "Left Arrow";
    case KEY_UP:
      return "Up Arrow";
    case KEY_RIGHT:
      return "Right Arrow";
    case KEY_DOWN:
      return "Down Arrow";
    case KEY_SELECT:
      return "Select";
    case KEY_PRINT:
      return "Print";
    case KEY_EXECUTE:
      return "Execute";
    case KEY_SNAPSHOT:
      return "Print Screen";
    case KEY_INSERT:
      return "Insert";
    case KEY_DELETE:
      return "Delete";
    case KEY_HELP:
      return "Help";
    case KEY_A:
      return "A";
    case KEY_B:
      return "B";
    case KEY_C:
      return "C";
    case KEY_D:
      return "D";
    case KEY_E:
      return "E";
    case KEY_F:
      return "F";
    case KEY_G:
      return "G";
    case KEY_H:
      return "H";
    case KEY_I:
      return "I";
    case KEY_J:
      return "J";
    case KEY_K:
      return "K";
    case KEY_L:
      return "L";
    case KEY_M:
      return "M";
    case KEY_N:
      return "N";
    case KEY_O:
      return "O";
    case KEY_P:
      return "P";
    case KEY_Q:
      return "Q";
    case KEY_R:
      return "R";
    case KEY_S:
      return "S";
    case KEY_T:
      return "T";
    case KEY_U:
      return "U";
    case KEY_V:
      return "V";
    case KEY_W:
      return "W";
    case KEY_X:
      return "X";
    case KEY_Y:
      return "Y";
    case KEY_Z:
      return "Z";
    case KEY_LWIN:
      return "Left Win";
    case KEY_RWIN:
      return "Right Win";
    case KEY_APPS:
      return "Apps";
    case KEY_SLEEP:
      return "Sleep";
    case KEY_NUMPAD0:
      return "Numpad 0";
    case KEY_NUMPAD1:
      return "Numpad 1";
    case KEY_NUMPAD2:
      return "Numpad 2";
    case KEY_NUMPAD3:
      return "Numpad 3";
    case KEY_NUMPAD4:
      return "Numpad 4";
    case KEY_NUMPAD5:
      return "Numpad 5";
    case KEY_NUMPAD6:
      return "Numpad 6";
    case KEY_NUMPAD7:
      return "Numpad 7";
    case KEY_NUMPAD8:
      return "Numpad 8";
    case KEY_NUMPAD9:
      return "Numpad 9";
    case KEY_MULTIPLY:
      return "Numpad *";
    case KEY_ADD:
      return "Numpad +";
    case KEY_SEPARATOR:
      return "Separator";
    case KEY_SUBTRACT:
      return "Numpad -";
    case KEY_DECIMAL:
      return "Numpad .";
    case KEY_DIVIDE:
      return "Numpad /";
    case KEY_F1:
      return "F1";
    case KEY_F2:
      return "F2";
    case KEY_F3:
      return "F3";
    case KEY_F4:
      return "F4";
    case KEY_F5:
      return "F5";
    case KEY_F6:
      return "F6";
    case KEY_F7:
      return "F7";
    case KEY_F8:
      return "F8";
    case KEY_F9:
      return "F9";
    case KEY_F10:
      return "F10";
    case KEY_F11:
      return "F11";
    case KEY_F12:
      return "F12";
    case KEY_F13:
      return "F13";
    case KEY_F14:
      return "F14";
    case KEY_F15:
      return "F15";
    case KEY_F16:
      return "F16";
    case KEY_F17:
      return "F17";
    case KEY_F18:
      return "F18";
    case KEY_F19:
      return "F19";
    case KEY_F20:
      return "F20";
    case KEY_F21:
      return "F21";
    case KEY_F22:
      return "F22";
    case KEY_F23:
      return "F23";
    case KEY_F24:
      return "F24";
    case KEY_NUMLOCK:
      return "Num Lock";
    case KEY_SCROLL:
      return "Scroll Lock";
    case KEY_NUMPAD_EQUAL:
      return "Numpad =";
    case KEY_LSHIFT:
      return "Left Shift";
    case KEY_RSHIFT:
      return "Right Shift";
    case KEY_LCONTROL:
      return "Left Control";
    case KEY_RCONTROL:
      return "Right Control";
    case KEY_LMENU:
      return "Left Alt";
    case KEY_RMENU:
      return "Right Alt";
    case KEY_SEMICOLON:
      return "Semicolon";
    case KEY_PLUS:
      return "Plus";
    case KEY_COMMA:
      return "Comma";
    case KEY_MINUS:
      return "Minus";
    case KEY_PERIOD:
      return "Period";
    case KEY_SLASH:
      return "Slash";
    case KEY_GRAVE:
      return "Grave Accent";
    case KEY_NULL:
      return "Null";
    case KEYS_MAX_KEYS:
      return "MaxKeys";
    default:
      return "Unknown Key";
  }
}

constexpr string RegisterFailedMessageFor(stringv system) {
  return "failed to register " + string(system) + " event";
}

} // namespace flatearth

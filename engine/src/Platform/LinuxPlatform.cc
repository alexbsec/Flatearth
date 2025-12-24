#include "Core/Event.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Platform.hpp"

#if FEPLATFORM_LINUX

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/XKBlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>
#else
#include <unistd.h>
#endif

namespace flatearth::platform {

struct InternalState {
  Display *display;
  xcb_connection_t *connection;
  xcb_window_t window;
  xcb_screen_t *screen;
  xcb_atom_t wmProtocols;
  xcb_atom_t wmDeleteWin;
  VkSurfaceKHR surface;

  ~InternalState() {
    if (display != nullptr) {
      XAutoRepeatOn(display);
      XCloseDisplay(display);
      display = nullptr;
    }

    connection = nullptr;
  }
};

void GetRequiredExtNames(containers::DArray<const char *> *namesDArray) {
  namesDArray->Push("VK_KHR_xcb_surface");
}

FeExpect<void, Error> CreateVulkanSurface(PlatformState *platState,
                                          renderer::vulkan::Context &ctx) {
  if (platState == nullptr) {
    FLOG_ERROR("platState is nullptr, aborting");
    return FeErr{Error("cannot create Vulkan surface on nullptr platformState",
                       ErrorType::NullptrException)};
  }

  auto &pInternalState = platState->internalState;
  VkXcbSurfaceCreateInfoKHR createInfo = {
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
  createInfo.connection = pInternalState->connection;
  createInfo.window = pInternalState->window;

  VkResult res = vkCreateXcbSurfaceKHR(
      ctx.instance, &createInfo, ctx.pAllocator, &pInternalState->surface);
  if (res != VK_SUCCESS) {
    FLOG_FATAL("Vulkan surface failed to create");
    return FeErr{Error("vkCreateXcbSurfaceKHR did not succeeded",
                       ErrorType::PlatformCreateSurface)};
  }

  ctx.surface = pInternalState->surface;
  return {};
}

input::Keys TranslateKeySymbol(uint32 keySym);

const string cNullInternalStateError = "platform internal state is nullptr";
const string cXCBConnectionError = "XCB failed to connect";
const string cXCBFlushError = "XCB failed to flush";

Platform::Platform(const string &applicationName, int32 x, int32 y, int32 width,
                   int32 height, memory::MemoryManager &memManager,
                   input::InputManager &inputManager,
                   event::EventManager &eventManager)
    : _xPos(x), _yPos(y), _width(width), _height(height),
      _applicationName(applicationName), _memoryManager(memManager),
      _inputManager(inputManager), _eventManager(eventManager) {
  _platState.internalState =
      _memoryManager.Allocate<InternalState>(memory::Tag::Platform);
}

Platform::~Platform() {}

FeExpect<void, Error> Platform::Initialize() {
  if (_platState.internalState == nullptr) {
    return FeErr<Error>(cNullInternalStateError);
  }

  auto &pInternalState = _platState.internalState;

  pInternalState->display = XOpenDisplay(nullptr);
  if (!pInternalState->display) {
    return FeErr<Error>("XOpenDisplay failed");
  }
  XAutoRepeatOff(pInternalState->display);

  pInternalState->connection = XGetXCBConnection(pInternalState->display);
  XSetEventQueueOwner(pInternalState->display, XCBOwnsEventQueue);

  if (xcb_connection_has_error(pInternalState->connection)) {
    FLOG_ERROR("failed to connect to X server via XCB");
    return FeErr<Error>(cXCBConnectionError);
  }

  const struct xcb_setup_t *setup = xcb_get_setup(pInternalState->connection);

  xcb_screen_iterator_t screenIterator = xcb_setup_roots_iterator(setup);
  int32 screenP = 0;
  for (int32 s = screenP; s > 0; s--) {
    xcb_screen_next(&screenIterator);
  }

  // assign the screens
  pInternalState->screen = screenIterator.data;
  pInternalState->window = xcb_generate_id(pInternalState->connection);

  uint32 eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

  uint32 eventValues =
      XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
      XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
      XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
      XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

  uint32 valueList[] = {pInternalState->screen->black_pixel, eventValues};

  // create window
  xcb_void_cookie_t cookie = xcb_create_window(
      pInternalState->connection, XCB_COPY_FROM_PARENT, pInternalState->window,
      pInternalState->screen->root, _xPos, _yPos, _width, _height, 0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, pInternalState->screen->root_visual,
      eventMask, valueList);

  // change title
  xcb_change_property(pInternalState->connection, XCB_PROP_MODE_REPLACE,
                      pInternalState->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                      8, _applicationName.length(), _applicationName.c_str());

  // tell the server to notify when the window manager attempts to destroy the
  // window
  const string cDeleteStr = "WM_DELETE_WINDOW";
  const string cProtocolsString = "WM_PROTOCOLS";

  xcb_intern_atom_cookie_t wmDeleteCookie = xcb_intern_atom(
      pInternalState->connection, 0, cDeleteStr.length(), cDeleteStr.c_str());

  xcb_intern_atom_cookie_t wmProtocolsCookie =
      xcb_intern_atom(pInternalState->connection, 0, cProtocolsString.length(),
                      cProtocolsString.c_str());

  xcb_intern_atom_reply_t *wmDeleteReply = xcb_intern_atom_reply(
      pInternalState->connection, wmDeleteCookie, nullptr);

  xcb_intern_atom_reply_t *wmProtocolsReply = xcb_intern_atom_reply(
      pInternalState->connection, wmProtocolsCookie, nullptr);

  pInternalState->wmDeleteWin = wmDeleteReply->atom;
  pInternalState->wmProtocols = wmProtocolsReply->atom;

  xcb_change_property(pInternalState->connection, XCB_PROP_MODE_REPLACE,
                      pInternalState->window, pInternalState->wmProtocols, 4,
                      32, 1, &pInternalState->wmDeleteWin);

  xcb_map_window(pInternalState->connection, pInternalState->window);

  int32 streamResult = xcb_flush(pInternalState->connection);
  if (streamResult <= 0) {
    FLOG_FATAL("an error occurred when flushing the stream: %d", streamResult);
    return FeErr<Error>(cXCBFlushError);
  }

  FLOG_INFO("platform initialized successfully");
  return {};
}

FeExpect<bool, Error> Platform::PollEvents() {
  if (_platState.internalState == nullptr) {
    LOG_ERROR(cNullInternalStateError);
    return FeErr<Error>(cNullInternalStateError);
  }

  auto &pInternalState = _platState.internalState;
  xcb_generic_event_t *event;
  xcb_client_message_event_t *cm;

  bool quitFlag = FeFalse;
  while ((event = xcb_poll_for_event(pInternalState->connection)) != nullptr) {
    if (event == nullptr) {
      break;
    }

    switch (event->response_type & 0x7f) {
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE: {
      xcb_key_press_event_t *keyEvent = (xcb_key_press_event_t *)event;
      bool pressed = event->response_type == XCB_KEY_PRESS;
      xcb_keycode_t code = keyEvent->detail;
      int32 level = (keyEvent->state & XCB_MOD_MASK_SHIFT) ? 1 : 0;
      KeySym keySymbol =
          XkbKeycodeToKeysym(pInternalState->display, (KeyCode)code, 0, level);
      input::Keys key = TranslateKeySymbol((uint32)keySymbol);
      auto processRes = _inputManager.ProcessKey(key, pressed);
      if (!processRes.has_value()) {
        FLOG_ERROR("input manager failed to process key {} event",
                   (uint32)keySymbol);
        break;
      }

      if (!processRes.value()) {
        FLOG_WARN("event for key {} was not fired for some reason",
                  (uint32)keySymbol);
      }
    } break;
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE: {
      xcb_button_press_event_t *buttonEvent = (xcb_button_press_event_t *)event;
      bool pressed = event->response_type == XCB_BUTTON_PRESS;
      input::Button button = input::Button::MaxButtons;
      switch (buttonEvent->detail) {
      case XCB_BUTTON_INDEX_1:
        button = input::Button::Left;
        break;
      case XCB_BUTTON_INDEX_2:
        button = input::Button::Middle;
        break;
      case XCB_BUTTON_INDEX_3:
        button = input::Button::Right;
        break;
      }

      if (button == input::Button::MaxButtons) {
        break;
      }

      auto buttonRes = _inputManager.ProcessButton(button, pressed);
      if (!buttonRes.has_value()) {
        FLOG_ERROR("input manager failed to process mouse click");
        break;
      }

      if (!buttonRes.value()) {
        FLOG_WARN("button event was not fired for some reason");
      }
    } break;
    case XCB_MOTION_NOTIFY: {
      xcb_motion_notify_event_t *moveEvent = (xcb_motion_notify_event_t *)event;
      auto moveRes = _inputManager.ProcessMouseMove(moveEvent->event_x,
                                                    moveEvent->event_y);
      if (!moveRes.has_value()) {
        FLOG_ERROR("input manager failed to process mouse move");
        break;
      }

      if (!moveRes.value()) {
        FLOG_WARN("mouse move event was not fired for some reason");
      }
    } break;
    case XCB_CONFIGURE_NOTIFY: {
      xcb_configure_notify_event_t *configureEvent =
          (xcb_configure_notify_event_t *)event;
      event::EventContext eventCtx{};
      event::Uint32x4 resizePayload{
          configureEvent->width,
          configureEvent->height,
      };
      eventCtx.Set(event::EventLayout::Uint32x4, resizePayload);
      auto res = _eventManager.FireEvent(event::SystemEventCode::WindowResized,
                                         nullptr, eventCtx);
      if (!res.has_value()) {
        FLOG_ERROR("event manager failed to fire resize event: {}",
                   res.error().message);
      }
    } break;
    case XCB_CLIENT_MESSAGE: {
      auto *cm = (xcb_client_message_event_t *)event;
      if (cm->data.data32[0] == pInternalState->wmDeleteWin) {
        quitFlag = FeTrue;
      }
    } break;
    default:
      break;
    }

    free(event);
  }

  return !quitFlag;
}

PlatformState *Platform::State() { return &_platState; }

input::Keys TranslateKeySymbol(uint32 keySymbol) {
  switch (keySymbol) {
  case XK_0:
    return input::KEY_0;
  case XK_1:
    return input::KEY_1;
  case XK_2:
    return input::KEY_2;
  case XK_3:
    return input::KEY_3;
  case XK_4:
    return input::KEY_4;
  case XK_5:
    return input::KEY_5;
  case XK_6:
    return input::KEY_6;
  case XK_7:
    return input::KEY_7;
  case XK_8:
    return input::KEY_8;
  case XK_9:
    return input::KEY_9;
  case XK_BackSpace:
    return input::KEY_BACKSPACE;
  case XK_Return:
    return input::KEY_ENTER;
  case XK_Tab:
    return input::KEY_TAB;
    // case XK_Shift: return input::KEY_SHIFT;
    // case XK_Control: return input::KEY_CONTROL;
  case XK_Pause:
    return input::KEY_PAUSE;
  case XK_Caps_Lock:
    return input::KEY_CAPITAL;
  case XK_Escape:
    return input::KEY_ESCAPE;
    // Not supported
    // case : return input::KEY_CONVERT;
    // case : return input::KEY_NONCONVERT;
    // case : return input::KEY_ACCEPT;
  case XK_Mode_switch:
    return input::KEY_MODECHANGE;
  case XK_space:
    return input::KEY_SPACE;
  case XK_Prior:
    return input::KEY_PRIOR;
  case XK_Next:
    return input::KEY_NEXT;
  case XK_End:
    return input::KEY_END;
  case XK_Home:
    return input::KEY_HOME;
  case XK_Left:
    return input::KEY_LEFT;
  case XK_Up:
    return input::KEY_UP;
  case XK_Right:
    return input::KEY_RIGHT;
  case XK_Down:
    return input::KEY_DOWN;
  case XK_Select:
    return input::KEY_SELECT;
  case XK_Print:
    return input::KEY_PRINT;
  case XK_Execute:
    return input::KEY_EXECUTE;
  // case XK_snapshot: return input::KEY_SNAPSHOT; // not supported
  case XK_Insert:
    return input::KEY_INSERT;
  case XK_Delete:
    return input::KEY_DELETE;
  case XK_Help:
    return input::KEY_HELP;
  case XK_Meta_L:
    return input::KEY_LWIN; // TODO: not sure this is right
  case XK_Meta_R:
    return input::KEY_RWIN;
    // case XK_apps: return input::KEY_APPS; // not supported
    // case XK_sleep: return input::KEY_SLEEP; //not supported
  case XK_KP_0:
    return input::KEY_NUMPAD0;
  case XK_KP_1:
    return input::KEY_NUMPAD1;
  case XK_KP_2:
    return input::KEY_NUMPAD2;
  case XK_KP_3:
    return input::KEY_NUMPAD3;
  case XK_KP_4:
    return input::KEY_NUMPAD4;
  case XK_KP_5:
    return input::KEY_NUMPAD5;
  case XK_KP_6:
    return input::KEY_NUMPAD6;
  case XK_KP_7:
    return input::KEY_NUMPAD7;
  case XK_KP_8:
    return input::KEY_NUMPAD8;
  case XK_KP_9:
    return input::KEY_NUMPAD9;
  case XK_multiply:
    return input::KEY_MULTIPLY;
  case XK_KP_Add:
    return input::KEY_ADD;
  case XK_KP_Separator:
    return input::KEY_SEPARATOR;
  case XK_KP_Subtract:
    return input::KEY_SUBTRACT;
  case XK_KP_Decimal:
    return input::KEY_DECIMAL;
  case XK_KP_Divide:
    return input::KEY_DIVIDE;
  case XK_F1:
    return input::KEY_F1;
  case XK_F2:
    return input::KEY_F2;
  case XK_F3:
    return input::KEY_F3;
  case XK_F4:
    return input::KEY_F4;
  case XK_F5:
    return input::KEY_F5;
  case XK_F6:
    return input::KEY_F6;
  case XK_F7:
    return input::KEY_F7;
  case XK_F8:
    return input::KEY_F8;
  case XK_F9:
    return input::KEY_F9;
  case XK_F10:
    return input::KEY_F10;
  case XK_F11:
    return input::KEY_F11;
  case XK_F12:
    return input::KEY_F12;
  case XK_F13:
    return input::KEY_F13;
  case XK_F14:
    return input::KEY_F14;
  case XK_F15:
    return input::KEY_F15;
  case XK_F16:
    return input::KEY_F16;
  case XK_F17:
    return input::KEY_F17;
  case XK_F18:
    return input::KEY_F18;
  case XK_F19:
    return input::KEY_F19;
  case XK_F20:
    return input::KEY_F20;
  case XK_F21:
    return input::KEY_F21;
  case XK_F22:
    return input::KEY_F22;
  case XK_F23:
    return input::KEY_F23;
  case XK_F24:
    return input::KEY_F24;
  case XK_Num_Lock:
    return input::KEY_NUMLOCK;
  case XK_Scroll_Lock:
    return input::KEY_SCROLL;
  case XK_KP_Equal:
    return input::KEY_NUMPAD_EQUAL;
  case XK_Shift_L:
    return input::KEY_LSHIFT;
  case XK_Shift_R:
    return input::KEY_RSHIFT;
  case XK_Control_L:
    return input::KEY_LCONTROL;
  case XK_Control_R:
    return input::KEY_RCONTROL;
  // case XK_Menu: return input::KEY_LMENU;
  case XK_Menu:
    return input::KEY_RMENU;
  case XK_Alt_L:
    return input::KEY_LMENU;
  case XK_Alt_R:
    return input::KEY_RMENU;
  case XK_semicolon:
    return input::KEY_SEMICOLON;
  case XK_plus:
    return input::KEY_PLUS;
  case XK_comma:
    return input::KEY_COMMA;
  case XK_minus:
    return input::KEY_MINUS;
  case XK_period:
    return input::KEY_PERIOD;
  case XK_slash:
    return input::KEY_SLASH;
  case XK_grave:
    return input::KEY_GRAVE;
  case XK_a:
  case XK_A:
    return input::KEY_A;
  case XK_b:
  case XK_B:
    return input::KEY_B;
  case XK_c:
  case XK_C:
    return input::KEY_C;
  case XK_d:
  case XK_D:
    return input::KEY_D;
  case XK_e:
  case XK_E:
    return input::KEY_E;
  case XK_f:
  case XK_F:
    return input::KEY_F;
  case XK_g:
  case XK_G:
    return input::KEY_G;
  case XK_h:
  case XK_H:
    return input::KEY_H;
  case XK_i:
  case XK_I:
    return input::KEY_I;
  case XK_j:
  case XK_J:
    return input::KEY_J;
  case XK_k:
  case XK_K:
    return input::KEY_K;
  case XK_l:
  case XK_L:
    return input::KEY_L;
  case XK_m:
  case XK_M:
    return input::KEY_M;
  case XK_n:
  case XK_N:
    return input::KEY_N;
  case XK_o:
  case XK_O:
    return input::KEY_O;
  case XK_p:
  case XK_P:
    return input::KEY_P;
  case XK_q:
  case XK_Q:
    return input::KEY_Q;
  case XK_r:
  case XK_R:
    return input::KEY_R;
  case XK_s:
  case XK_S:
    return input::KEY_S;
  case XK_t:
  case XK_T:
    return input::KEY_T;
  case XK_u:
  case XK_U:
    return input::KEY_U;
  case XK_v:
  case XK_V:
    return input::KEY_V;
  case XK_w:
  case XK_W:
    return input::KEY_W;
  case XK_x:
  case XK_X:
    return input::KEY_X;
  case XK_y:
  case XK_Y:
    return input::KEY_Y;
  case XK_z:
  case XK_Z:
    return input::KEY_Z;
  default:
    return input::KEY_NULL;
  }
}

} // namespace flatearth::platform

#endif

#include "Core/Logger.hpp"
#include "Platform/Platform.hpp"

#if FEPLATFORM_WINDOWS

#include <cstring>
#include <stdexcept>
#include <windows.h>
#include <windowsx.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

static float64 sClockFrequency;
static LARGE_INTEGER sStartTime;

namespace flatearth::platform {

static const char* scSurface = "VK_KHR_win32_surface";

const string cNullInternalStateError = "platform internal state is nullptr";

struct InternalState {
	HINSTANCE hInstance;
	HWND hwnd;
  VkSurfaceKHR surface;
};

void GetRequiredExtNames(containers::DArray<const char*>* namesDArray) {
  namesDArray->Push(scSurface);
}

FeExpect<void, Error> CreateVulkanSurface(PlatformState* platState,
  renderer::vulkan::Context &context) {
  auto& pInternalState = platState->internalState;

  VkWin32SurfaceCreateInfoKHR createInfo = {
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
  createInfo.hinstance = pInternalState->hInstance;
  createInfo.hwnd = pInternalState->hwnd;

  VkResult result = vkCreateWin32SurfaceKHR(
    context.instance, &createInfo, context.pAllocator, &pInternalState->surface);
  if (result != VK_SUCCESS) {
    FLOG_FATAL("Vulkan surface failed to be created");
    return FeErr{ Error("failed to create Vulkan surface", ErrorType::RendererVulkanError) };
  }

  context.surface = pInternalState->surface;
  return {};
}

Platform::Platform(const string& applicationName, int32 x, int32 y, int32 width, int32 height, memory::MemoryManager& memManager, input::InputManager& inputManager, event::EventManager &eventManager)
  : _xPos(x), _yPos(y), _width(width), _height(height),
  _applicationName(applicationName), _memoryManager(memManager),
  _inputManager(inputManager), _eventManager(eventManager) {
  _platState.internalState =
    _memoryManager.Allocate<InternalState>(memory::Tag::Platform);
}

Platform::~Platform() {}

FeExpect<void, Error> Platform::Initialize() {
  if (_platState.internalState == nullptr) {
    return FeErr{ Error(cNullInternalStateError, ErrorType::NullptrException) };
  }

  auto& pInternalState = _platState.internalState;
  pInternalState->hInstance = GetModuleHandleA(0);
  HICON icon = LoadIcon(pInternalState->hInstance, IDI_APPLICATION);
  WNDCLASSA wca;
  std::memset(&wca, 0, sizeof(wca));
  wca.style = CS_DBLCLKS;
  wca.lpfnWndProc = Win32ProcessMessage;
  wca.cbClsExtra = 0;
  wca.cbWndExtra = 0;
  wca.hInstance = pInternalState->hInstance;
  wca.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wca.hbrBackground = nullptr;
  wca.lpszClassName = "flatearth_window_class";

  if (!RegisterClass(&wca)) {
    MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
    FLOG_FATAL("window registration failed");
    return FeErr{ Error("windows platform failed to register") };
  }

  _xPos = CW_USEDEFAULT;
  _yPos = CW_USEDEFAULT;

  uint32 clientWidth = _width;
  uint32 clientHeight = _height;

  int windowX = CW_USEDEFAULT;
  int windowY = CW_USEDEFAULT;
  uint32 windowWidth = clientWidth;
  uint32 windowHeight = clientHeight;

  uint32 windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_VISIBLE;
  uint32 windowExStyle = WS_EX_APPWINDOW;

  windowStyle |= WS_MAXIMIZEBOX;
  windowStyle |= WS_MINIMIZEBOX;
  windowStyle |= WS_THICKFRAME;

  RECT borderRect = { 0, 0, 0, 0 };
  AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);
  
  windowWidth += borderRect.right - borderRect.left;
  windowHeight += borderRect.bottom - borderRect.top;

  HWND handle = CreateWindowExA(windowExStyle, "flatearth_window_class",
    _applicationName.c_str(), windowStyle, windowX,
    windowY, windowWidth, windowHeight, 0, 0,
    pInternalState->hInstance, this);

  // Check handle
  if (handle == 0) {
    MessageBoxA(0, "Window creation failed", "Error",
      MB_ICONEXCLAMATION | MB_OK);
    FLOG_FATAL("Platform: window creation failed");
    return FeErr{ Error("failed to create window") };
  }

  pInternalState->hwnd = handle;

  bool shouldActivate = FeTrue;
  int32 showWindowCommandFlags = shouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
  ShowWindow(pInternalState->hwnd, showWindowCommandFlags);
  UpdateWindow(pInternalState->hwnd);

  RECT wr{}, cr{};
  GetWindowRect(pInternalState->hwnd, &wr);

  FLOG_INFO("HWND={} visible={} iconic={} zoomed={} rect=({}, {}) {}x{}",
    (void*)pInternalState->hwnd,
    (int)IsWindowVisible(pInternalState->hwnd),
    (int)IsIconic(pInternalState->hwnd),
    (int)IsZoomed(pInternalState->hwnd),
    wr.left, wr.top,
    (wr.right - wr.left), (wr.bottom - wr.top)
  );

  GetClientRect(pInternalState->hwnd, &cr);
  FLOG_INFO("client rect: {}x{}",
    (wr.right - wr.left), (wr.bottom - wr.top),
    (cr.right - cr.left), (cr.bottom - cr.top)
  );

  HMONITOR mon = MonitorFromWindow(pInternalState->hwnd, MONITOR_DEFAULTTONULL);
  FLOG_INFO("monitor_from_window={}", (void*)mon);

  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  sClockFrequency = 1.0 / static_cast<float64>(freq.QuadPart);
  QueryPerformanceCounter(&sStartTime);
  FLOG_INFO("windows platform successfully initialized: {}x{}", _width, _height);
  return {};
}

FeExpect<bool, Error> Platform::PollEvents() {
  MSG message;
  while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
    if (message.message == WM_QUIT) {
      return FeFalse;
    }
    TranslateMessage(&message);
    DispatchMessageA(&message);
  }
  return FeTrue;
}

PlatformState* Platform::State() {
  return &_platState;
}

LRESULT CALLBACK Platform::Win32ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  Platform* platform = nullptr;

  if (msg == WM_NCCREATE) {
    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
    platform = static_cast<Platform *>(cs->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)platform);
  }
  else {
    platform = reinterpret_cast<Platform *>(
      GetWindowLongPtr(hwnd, GWLP_USERDATA)
      );
  }

  if (platform != nullptr) {
    return platform->ProcessMessage(hwnd, msg, wParam, lParam);
  }

  return DefWindowProcA(hwnd, msg, wParam, lParam);
}


LRESULT Platform::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  using namespace event;
  switch (msg) {
  case WM_ERASEBKGND:
    // Notify the OS that erasing will be handled by the application to prevent
    // flicker
    return 1;

  case WM_CLOSE: {
    DestroyWindow(hwnd);
    return 0;
  }

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_SIZE: {
    if (wParam == SIZE_MINIMIZED) {
      return 0;
    }

    uint32 width = LOWORD(lParam);
    uint32 height = HIWORD(lParam);

    if (width == 0 || height == 0) {
      return 0;
    }

    EventContext eventCtx{};
    Uint16x8 resizePayload{ width, height, 0u, 0u };

    eventCtx.Set<Uint16x8>(EventLayout::Uint16x8, resizePayload);
    _eventManager.FireEvent(
      SystemEventCode::WindowResized,
      nullptr,
      eventCtx
    );

  } break;
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP: {
    bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    input::Keys key =
      static_cast<input::Keys>((uint16)wParam);

    if (wParam == VK_MENU) {
      // Alt has been pressed
      if (GetKeyState(VK_RMENU) & 0x8000) {
        key = input::KEY_RMENU;
      }
      else if (GetKeyState(VK_LMENU) & 0x8000) {
        key = input::KEY_LMENU;
      }
    }
    else if (wParam == VK_SHIFT) {
      if (GetKeyState(VK_RSHIFT) & 0x8000) {
        key = input::KEY_RSHIFT;
      }
      else if (GetKeyState(VK_LSHIFT) & 0x8000) {
        key = input::KEY_LSHIFT;
      }
    }

    _inputManager.ProcessKey(key, pressed);
  } break;

  case WM_MOUSEMOVE: {
    int32 xPos = GET_X_LPARAM(lParam);
    int32 yPos = GET_Y_LPARAM(lParam);
    _inputManager.ProcessMouseMove(xPos, yPos);
  } break;

  case WM_MOUSEHWHEEL: {
    int32 zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (zDelta != 0) {
      zDelta = (zDelta < 0) ? -1 : 1;
      // TODO: _inputManager.ProcessMouseWheel(zDelta);
    }
  } break;

  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP: {
    bool pressed = (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN ||
      msg == WM_RBUTTONDOWN);
    input::Button button = input::Button::MaxButtons;
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      button = input::Button::Left;
      break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      button = input::Button::Middle;
      break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      button = input::Button::Right;
      break;
    }

    if (button != input::Button::MaxButtons) {
      _inputManager.ProcessButton(button, pressed);
    }
  } break;
  }

  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

}

#endif
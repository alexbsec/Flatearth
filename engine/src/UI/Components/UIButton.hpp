#ifndef _FLATEARTH_ENGINE_UI_COMPONENTS_UI_BUTTON_HPP
#define _FLATEARTH_ENGINE_UI_COMPONENTS_UI_BUTTON_HPP

#include "Defines.hpp"
#include "Renderer/RendererTypes.hpp"

namespace flatearth::ui {

enum class UIButtonState : uint8 {
  Normal,
  Hovered,
  Pressed,
  Disabled,
};

struct UIButton {
  UIButtonState state{UIButtonState::Normal};
  bool justClicked{FeFalse};

  // NOTE: These must be trivially copyable, thats why
  // raw function ptrs are used
  void (*pFn_OnClick)(void *){nullptr};
  void *onClickUserData{nullptr};

  void (*pFn_OnHover)(void *){nullptr};
  void *onHoverUserData{nullptr};

  renderer::Tint normalTint{{0.75f, 0.75f, 0.75f}, 1.0f};
  renderer::Tint hoverTint{{1.0f, 1.0f, 1.0f}, 1.0f};
  renderer::Tint pressedTint{{0.5f, 0.5f, 0.5f}, 1.0f};
  renderer::Tint disabledTint{{0.3f, 0.3f, 0.3f}, 1.0f};
};

} // namespace flatearth::ui

#endif // _FLATEARTH_ENGINE_UI_COMPONENTS_UI_BUTTON_HPP

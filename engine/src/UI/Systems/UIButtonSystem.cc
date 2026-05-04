#include "UIButtonSystem.hpp"

#include "Core/CoreTypes.hpp"
#include "UI/Components/UIAnchor.hpp"
#include "UI/Components/UIButton.hpp"
#include "UI/Components/UIStyle.hpp"

namespace flatearth::ui::systems {

UIButtonSystem::UIButtonSystem(input::InputManager &input, EngineState *pEngState)
    : _input(input), _pEngState(pEngState) {
}

void UIButtonSystem::Update(ecs::Registry &registry, float32) {
  if (_pEngState == nullptr || _pEngState->width == 0 || _pEngState->height == 0) {
    return;
  }

  int32 mouseX, mouseY;
  _input.GetMousePosition(mouseX, mouseY);

  float32 ndcX = 2.0f * (static_cast<float32>(mouseX) / _pEngState->width) - 1.0f;
  float32 ndcY = 1.0f - 2.0f * (static_cast<float32>(mouseY) / _pEngState->height);

  bool isDown = _input.IsButtonDown(input::Button::Left);
  bool wasDown = _input.WasButtonDown(input::Button::Left);
  bool justReleased = !isDown && wasDown;

  for (auto [entity, anchor, btn] : registry.ViewOf<UIAnchor, UIButton>()) {
    btn.justClicked = FeFalse;

    float32 anchorNdcX = anchor.normalizedX * 2.0f - 1.0f;
    float32 anchorNdcY = anchor.normalizedY * 2.0f - 1.0f;
    float32 aspect = static_cast<float32>(_pEngState->width) / static_cast<float32>(_pEngState->height);
    float32 hitHalfX = anchor.aspectCorrectX ? anchor.scaleX / aspect * 0.5f : anchor.scaleX * 0.5f;
    bool inBounds = math::Abs32(ndcX - anchorNdcX) <= hitHalfX &&
                    math::Abs32(ndcY - anchorNdcY) <= anchor.scaleY * 0.5f;

    UIButtonState prev = btn.state;

    if (btn.state == UIButtonState::Disabled) {
      // Disabled state is set externally — system does not clear it
    } else if (!inBounds) {
      btn.state = UIButtonState::Normal;
    } else if (isDown) {
      btn.state = UIButtonState::Pressed;
    } else {
      if (prev == UIButtonState::Pressed && justReleased) {
        btn.justClicked = FeTrue;
        if (btn.pFn_OnClick) {
          btn.pFn_OnClick(btn.onClickUserData);
        }
      }
      btn.state = UIButtonState::Hovered;
      if (prev != UIButtonState::Hovered && btn.pFn_OnHover) {
        btn.pFn_OnHover(btn.onHoverUserData);
      }
    }

    // Sync tint to UIStyle if present
    UIStyle *pStyle = registry.TryGet<UIStyle>(entity);
    if (pStyle != nullptr) {
      switch (btn.state) {
        case UIButtonState::Hovered:
          pStyle->tint = btn.hoverTint;
          break;
        case UIButtonState::Pressed:
          pStyle->tint = btn.pressedTint;
          break;
        case UIButtonState::Disabled:
          pStyle->tint = btn.disabledTint;
          break;
        default:
          pStyle->tint = btn.normalTint;
          break;
      }
    }
  }
}

} // namespace flatearth::ui::systems

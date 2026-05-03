#ifndef _FLATEARTH_ENGINE_UI_COMPONENTS_UI_ANCHOR_HPP
#define _FLATEARTH_ENGINE_UI_COMPONENTS_UI_ANCHOR_HPP

#include "Defines.hpp"

namespace flatearth::ui {

struct UIAnchor {
  float32 normalizedX{0.0f};
  float32 normalizedY{0.0f};
  float32 rotation{0.0f};
  float32 scaleX{1.0f};
  float32 scaleY{1.0f};
};

} // namespace flatearth::ui

#endif // _FLATEARTH_ENGINE_UI_COMPONENTS_UI_ANCHOR_HPP

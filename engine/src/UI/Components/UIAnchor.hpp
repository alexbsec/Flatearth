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
  // When true, scaleX is in height-normalized units (same as scaleY and UIText),
  // and the renderer divides it by aspect ratio before drawing. Use this for any
  // element whose width should be consistent with adjacent UIText labels.
  bool aspectCorrectX{FeFalse};
};

} // namespace flatearth::ui

#endif // _FLATEARTH_ENGINE_UI_COMPONENTS_UI_ANCHOR_HPP

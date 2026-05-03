#ifndef _FLATEARTH_ENGINE_UI_COMPONENTS_UI_TEXT_HPP
#define _FLATEARTH_ENGINE_UI_COMPONENTS_UI_TEXT_HPP

#include "Core/FeString.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::ui {

// TODO: make this runtime configurable
constexpr uint32 cMaxTextLength = 512;

struct UIText {
  resources::FontHandle handle{resources::cInvalidFontHandle};
  FeString<cMaxTextLength> text{};
  renderer::Tint color{{1.0f, 1.0f, 1.0f}, 1.0f};
};

} // namespace flatearth::ui

#endif // _FLATEARTH_ENGINE_UI_COMPONENTS_UI_TEXT_HPP

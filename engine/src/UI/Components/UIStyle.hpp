#ifndef _FLATEARTH_ENGINE_UI_COMPONENTS_UI_STYLE_HPP
#define _FLATEARTH_ENGINE_UI_COMPONENTS_UI_STYLE_HPP

#include "Math/Vector2D.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::ui {

struct UIStyle {
  resources::MeshHandle meshHandle{resources::cInvalidMeshHandle};
  resources::MaterialHandle matHandle{resources::cInvalidMaterialHandle};
  math::Vec2D uvOffset{0.0f, 0.0f};
  math::Vec2D uvScale{1.0f, 1.0f};
  renderer::Tint tint{{1.0f, 1.0f, 1.0f}, 1.0f};
  float32 useTexture{1.0f};
};

} // namespace flatearth::ui

#endif // _FLATEARTH_ENGINE_UI_COMPONENTS_UI_STYLE_HPP

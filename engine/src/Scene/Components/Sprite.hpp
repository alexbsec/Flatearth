#ifndef _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_HPP
#define _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_HPP

#include "Defines.hpp"
#include "Math/Vector2D.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::scene {

struct Sprite {
public:
  renderer::RenderLayer layer{renderer::RenderLayer::Entities};
  resources::MeshShape meshShape{resources::MeshShape::Quad};
  math::Vec2D uvOffset{0.0f, 0.0f};
  math::Vec2D uvScale{1.0f, 1.0f};
  bool flipX{FeFalse}, flipY{FeFalse};

  // Runtime only, not reflected
  bool dirty{FeTrue};
  resources::TextureHandle texHandle{0};
  resources::MaterialHandle matHandle{0};
  resources::MeshHandle meshHandle{0};
};

} // namespace flatearth::scene

#endif // _FLATEARTH_ENGINE_SCENE_COMPONENTS_SPRITE_HPP

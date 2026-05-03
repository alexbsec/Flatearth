#ifndef _FLATEARTH_ENGINE_RESOURCES_TILESET_HPP
#define _FLATEARTH_ENGINE_RESOURCES_TILESET_HPP

#include "Math/Vector2D.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::resources {

struct TilesetTile {
  uint32 id{0};
  math::Vec2D uvOffset{0.0f, 0.0f};
  math::Vec2D uvScale{1.0f, 1.0f};
};

struct Tileset {
  string name;
  string imagePath;
  TextureHandle texture{0};
  uint32 tileWidth{0};
  uint32 tileHeight{0};
  uint32 imageWidth{0};
  uint32 imageHeight{0};
  uint32 tileCount{0};
  uint32 columns{0};
  std::vector<TilesetTile> tiles;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_TILESET_HPP

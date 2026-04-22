#ifndef _FLATEARTH_ENGINE_RESOURCES_TILEMAP_HPP
#define _FLATEARTH_ENGINE_RESOURCES_TILEMAP_HPP

#include "Defines.hpp"
#include "Resources/Tileset.hpp"

#include <vector>

namespace flatearth::resources {

struct TilemapLayer {
  string              name;
  uint32              width{0};
  uint32              height{0};
  std::vector<uint32> tiles;  // row-major, global tile ID, 0 = empty
};

struct TilemapObject {
  string  name;       // prefab name to spawn
  float32 x{0.0f};
  float32 y{0.0f};
};

struct Tilemap {
  uint32                     tileWidth{0};
  uint32                     tileHeight{0};
  uint32                     mapWidth{0};
  uint32                     mapHeight{0};
  uint32                     firstGid{1};
  Tileset                    tileset;
  std::vector<TilemapLayer>  renderLayers;
  TilemapLayer               collisionLayer;
  std::vector<TilemapObject> objects;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_TILEMAP_HPP

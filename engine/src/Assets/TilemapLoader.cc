#include "Assets/TilemapLoader.hpp"

#include "Assets/AssetManager.hpp"
#include "Assets/TilesetLoader.hpp"
#include "Core/Logger.hpp"
#include "tinyxml2.h"

#include <filesystem>

namespace flatearth::assets {

namespace {

// Parses a CSV tile data string into a vector of global tile IDs
// Only CSV encoding is supported. Configure Tiled to export as CSV.
std::vector<uint32> ParseCSV(const char *text) {
  std::vector<uint32> tiles;
  if (!text)
    return tiles;

  const char *p = text;
  while (*p) {
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
      ++p;
    if (*p == '\0')
      break;
    char *end;
    tiles.push_back(static_cast<uint32>(std::strtoul(p, &end, 10)));
    p = end;
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
      ++p;
    if (*p == ',')
      ++p;
  }
  return tiles;
}

} // namespace

TilemapLoader::TilemapLoader(AssetManager &assetManager) : _assetManager(assetManager) {
}

FeExpect<resources::Tilemap, Error> TilemapLoader::Load(stringv tmxPath) {
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(tmxPath.data()) != tinyxml2::XML_SUCCESS) {
    FLOG_ERROR("TilemapLoader: failed to parse '{}'", tmxPath);
    return FeErr{Error("failed to parse tmx file", ErrorType::FileReadError)};
  }

  auto *mapElem = doc.FirstChildElement("map");
  if (!mapElem) {
    return FeErr{Error("tmx missing <map> root element", ErrorType::FileReadError)};
  }

  resources::Tilemap tm;
  tm.tileWidth = mapElem->UnsignedAttribute("tilewidth");
  tm.tileHeight = mapElem->UnsignedAttribute("tileheight");
  tm.mapWidth = mapElem->UnsignedAttribute("width");
  tm.mapHeight = mapElem->UnsignedAttribute("height");

  auto *tsElem = mapElem->FirstChildElement("tileset");
  if (!tsElem) {
    return FeErr{Error("tmx missing <tileset> element", ErrorType::FileReadError)};
  }

  tm.firstGid = tsElem->UnsignedAttribute("firstgid");

  namespace fs = std::filesystem;
  string tsxPath =
      (fs::path(tmxPath).parent_path() / tsElem->Attribute("source")).lexically_normal().string();

  TilesetLoader tsLoader{_assetManager};
  auto tsRes = tsLoader.Load(tsxPath).or_error("TilemapLoader: failed to load tileset");
  if (tsRes.errored()) {
    return FeErr{tsRes.error()};
  }
  tm.tileset = std::move(tsRes.value());

  for (auto *layerElem = mapElem->FirstChildElement("layer"); layerElem;
       layerElem = layerElem->NextSiblingElement("layer")) {

    resources::TilemapLayer layer;
    layer.name = layerElem->Attribute("name") ? layerElem->Attribute("name") : "";
    layer.width = layerElem->UnsignedAttribute("width");
    layer.height = layerElem->UnsignedAttribute("height");

    auto *dataElem = layerElem->FirstChildElement("data");
    if (dataElem) {
      layer.tiles = ParseCSV(dataElem->GetText());
    }

    if (layer.name == "Collision") {
      tm.collisionLayer = std::move(layer);
    }
    else {
      tm.renderLayers.push_back(std::move(layer));
    }
  }

  for (auto *objGroup = mapElem->FirstChildElement("objectgroup"); objGroup;
       objGroup = objGroup->NextSiblingElement("objectgroup")) {

    for (auto *obj = objGroup->FirstChildElement("object"); obj;
         obj = obj->NextSiblingElement("object")) {

      resources::TilemapObject o;
      o.name = obj->Attribute("name") ? obj->Attribute("name") : "";
      o.x = obj->FloatAttribute("x");
      o.y = obj->FloatAttribute("y");
      tm.objects.push_back(o);
    }
  }

  FLOG_INFO("TilemapLoader: loaded '{}' — {}x{} tiles, {} render layers, {} objects",
            tmxPath,
            tm.mapWidth,
            tm.mapHeight,
            tm.renderLayers.size(),
            tm.objects.size());
  return tm;
}

} // namespace flatearth::assets

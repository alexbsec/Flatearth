#include "Assets/TilemapManager.hpp"

#include "Assets/AssetManager.hpp"
#include "Assets/TilemapLoader.hpp"
#include "Core/Logger.hpp"
#include "ECS/Registry.hpp"
#include "Physics/BoxCollider.hpp"
#include "Physics/RigidBody.hpp"
#include "Resources/Tilemap.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/Transform2D.hpp"
#include "Scene/Scene.hpp"

namespace flatearth::assets {

TilemapManager::TilemapManager(AssetManager &assetManager, ecs::Registry &registry)
    : _assetManager(assetManager), _registry(registry) {}

FeExpect<ecs::EntityId, Error> TilemapManager::Load(stringv tmxPath, scene::Scene *scene) {
  TilemapLoader loader{_assetManager};
  auto tmRes = loader.Load(tmxPath);
  if (!tmRes.has_value())
    return FeErr{tmRes.error()};

  resources::Tilemap tm = std::move(tmRes.value());

  // base sprite shared by all tiles (same texture, quad mesh, UV overridden per tile)
  auto baseSpriteRes = _assetManager.SpriteFromTexture(tm.tileset.texture,
                                                       tm.tileset.name,
                                                       resources::MeshShape::Quad);
  if (!baseSpriteRes.has_value())
    return FeErr{baseSpriteRes.error()};

  scene::Sprite baseSprite = baseSpriteRes.value();

  // Coordinate convention: 1 tile = 1 world unit, Y-up, origin at map center.
  float32 halfW = tm.mapWidth  * 0.5f;
  float32 halfH = tm.mapHeight * 0.5f;

  // ── render layers → sprite entities ──────────────────────────────────────
  for (auto &layer : tm.renderLayers) {
    for (uint32 row = 0; row < layer.height; ++row) {
      for (uint32 col = 0; col < layer.width; ++col) {
        uint32 globalId = layer.tiles[row * layer.width + col];
        if (globalId == 0) continue;  // empty tile

        uint32 localId = globalId - tm.firstGid;
        if (localId >= tm.tileset.tileCount) continue;

        const auto &tile = tm.tileset.tiles[localId];

        float32 wx = col - halfW + 0.5f;
        float32 wy = halfH - row - 0.5f;

        auto e = _registry.Create();

        scene::Sprite tileSprite   = baseSprite;
        tileSprite.uvOffset        = tile.uvOffset;
        tileSprite.uvScale         = tile.uvScale;
        tileSprite.layer           = renderer::RenderLayer::Background;

        _registry.Insert(e, scene::Transform2D{wx, wy, 0.0f, 1.0f, 1.0f});
        _registry.Insert(e, tileSprite);
        scene->allEntities.push_back(e);
      }
    }
  }

  // ── collision layer → static Box2D bodies ────────────────────────────────
  auto &coll = tm.collisionLayer;
  for (uint32 row = 0; row < coll.height; ++row) {
    for (uint32 col = 0; col < coll.width; ++col) {
      if (coll.tiles[row * coll.width + col] == 0) continue;

      float32 wx = col - halfW + 0.5f;
      float32 wy = row - halfH + 0.5f;

      auto e = _registry.Create();
      _registry.Insert(e, scene::Transform2D{wx, wy});
      _registry.Insert(e, physics::RigidBody{.type = physics::BodyType::Static});
      _registry.Insert(e, physics::BoxCollider{.halfWidth = 0.5f, .halfHeight = 0.5f});
      scene->allEntities.push_back(e);
    }
  }

  uint32 renderLayerCount = static_cast<uint32>(tm.renderLayers.size());
  uint32 collTileCount    = static_cast<uint32>(coll.tiles.size());
  uint32 objectCount      = static_cast<uint32>(tm.objects.size());

  // ── root tilemap entity ───────────────────────────────────────────────────
  // Tilemap data is not stored as a component (contains std::vector — not trivially copyable).
  // TilemapManager owns the data; the root entity is just a transform anchor.
  auto root = _registry.Create();
  _registry.Insert(root, scene::Transform2D{});
  scene->roots.push_back(root);
  scene->allEntities.push_back(root);

  FLOG_INFO("TilemapManager: loaded '{}' — {} render layers, {} collision tiles, {} objects",
            tmxPath, renderLayerCount, collTileCount, objectCount);
  return root;
}

} // namespace flatearth::assets

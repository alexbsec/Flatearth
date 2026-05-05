#include "GameRenderer.hpp"

#include "Core/Logger.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Scene/Components/Camera2D.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Components/Transform2D.hpp"
#include "UI/Components/UIAnchor.hpp"
#include "UI/Components/UIStyle.hpp"
#include "UI/Components/UIText.hpp"

namespace flatearth::renderer {

using namespace scene;
using namespace ecs;
using namespace ui;
using namespace assets;

namespace packer {
void PackGameObjects(RenderPacket &, FrontendRenderer &, Registry &);
void PackUIObjects(RenderPacket &, FrontendRenderer &, Registry &);
void PackUISprites(RenderPacket &, FrontendRenderer &, Registry &);
void PackUIStyles(RenderPacket &, FrontendRenderer &, Registry &);
void PackUIText(RenderPacket &, AssetManager &, FrontendRenderer &, Registry &);
} // namespace packer

GameRenderer::GameRenderer(EngineState *pEngState,
                           memory::MemoryManager &memManager,
                           ecs::Registry &reg,
                           platform::FileSystem &fs,
                           assets::AssetManager &am)
    : _memoryManager(memManager), _frontendRenderer(pEngState, memManager, fs), _registry(reg),
      _assetManager(am) {
}

FeExpect<void, Error> GameRenderer::Initialize() {
  return _frontendRenderer.Initialize();
}

FeExpect<bool, Error> GameRenderer::Draw(float32 deltaTime) {
  math::Mat4D view = math::Mat4D::Identity();
  auto cameraView = _registry.ViewOf<Transform2D, Camera2D>();
  for (auto [entity, transform, cam] : cameraView) {
    // grabs first active camera
    view = math::Mat4D::Scale(cam.zoom, cam.zoom, 1.0f) *
           math::Mat4D::Translation(-transform.worldX, -transform.worldY, 0.0f) *
           math::Mat4D::RotationZ(-transform.worldRotation);
    break;
  }

  RenderPacket packet(_memoryManager);
  packet.deltaTime = deltaTime;
  packet.view = view;

  // Fill the packet with relevant data
  PackIt(packet);

  _lastDrawCallCount = packet.objects.Length() + packet.uiObjects.Length();
  return _frontendRenderer.DrawFrame(&packet);
}

void GameRenderer::Flush() {
  _frontendRenderer.Flush();
}

void GameRenderer::BeginImGuiFrame() {
  _frontendRenderer.BeginImGuiFrame();
}

FrontendRenderer &GameRenderer::FrontendReference() {
  return _frontendRenderer;
}

void GameRenderer::Shutdown() {
  _frontendRenderer.Shutdown();
}

void GameRenderer::PackIt(RenderPacket &packet) {
  packer::PackGameObjects(packet, _frontendRenderer, _registry);
  packer::PackUIObjects(packet, _frontendRenderer, _registry);
  packer::PackUIText(packet, _assetManager, _frontendRenderer, _registry);
}

namespace packer {

void PackGameObjects(RenderPacket &packet, FrontendRenderer &fr, Registry &reg) {
  View<Transform2D, Sprite> gameObjectView = reg.ViewOf<Transform2D, Sprite>();
  for (auto [entity, transform, sprite] : gameObjectView) {
    resources::Mesh *pMesh = fr.GetMesh(sprite.meshHandle);
    resources::Material *pMat = fr.GetMaterial(sprite.matHandle);
    if (pMesh == nullptr || pMat == nullptr) {
      continue;
    }

    math::Mat4D model = math::Mat4D::Translation(transform.worldX, transform.worldY, 0.0f) *
                        math::Mat4D::RotationZ(transform.worldRotation) *
                        math::Mat4D::Scale(transform.worldScaleX, transform.worldScaleY, 1.0f);
#if FEPLATFORM_WINDOWS
    model *= math::Mat4D::Scale(1.0f, -1.0f, 1.0f);
#endif
    RenderObject object{
        .geometryId = pMesh->id,
        .model = model,
        .uvOffset = sprite.uvOffset,
        .uvScale = sprite.uvScale,
        .layer = sprite.layer,
        .pMaterial = pMat,
    };
    packet.objects.Push(object);
  }
}

void PackUIObjects(RenderPacket &packet, FrontendRenderer &fr, Registry &reg) {
  // Warn about entities with both visual components — only one should be used
  View<UIAnchor, UIStyle, Sprite> conflictView = reg.ViewOf<UIAnchor, UIStyle, Sprite>();
  for (auto [entity, anchor, style, sprite] : conflictView) {
    FLOG_WARN("Entity {} has both UIStyle and Sprite — only UIStyle will be rendered", entity);
  }

  PackUISprites(packet, fr, reg);
  PackUIStyles(packet, fr, reg);
}

void PackUISprites(RenderPacket &packet, FrontendRenderer &fr, Registry &reg) {
  float32 aspect = fr.AspectRatio();
  View<UIAnchor, Sprite> uiSpriteView = reg.ViewOf<UIAnchor, Sprite>();
  for (auto [entity, uiAnchor, sprite] : uiSpriteView) {
    if (reg.TryGet<UIStyle>(entity)) {
      continue;
    }

    resources::Mesh *pMesh = fr.GetMesh(sprite.meshHandle);
    resources::Material *pMat = fr.GetMaterial(sprite.matHandle);
    if (pMesh == nullptr || pMat == nullptr) {
      continue;
    }

    float32 ndcX = uiAnchor.normalizedX * 2 - 1, ndcY = uiAnchor.normalizedY * 2 - 1;
    float32 drawScaleX = uiAnchor.aspectCorrectX ? uiAnchor.scaleX / aspect : uiAnchor.scaleX;
    math::Mat4D model = math::Mat4D::Translation(ndcX, ndcY, 0.0f) *
                        math::Mat4D::RotationZ(uiAnchor.rotation) *
                        math::Mat4D::Scale(drawScaleX, uiAnchor.scaleY, 1.0f);

    RenderObject object{
        .geometryId = pMesh->id,
        .model = model,
        .uvOffset = sprite.uvOffset,
        .uvScale = sprite.uvScale,
        .layer = RenderLayer::UI,
        .pMaterial = pMat,
        .tint = {math::Vec3D{1.0f, 1.0f, 1.0f}, 1.0f},
        .useTexture = 1.0f,
    };
    packet.uiObjects.Push(object);
  }
}

void PackUIStyles(RenderPacket &packet, FrontendRenderer &fr, Registry &reg) {
  float32 aspect = fr.AspectRatio();
  View<UIAnchor, UIStyle> uiStyleView = reg.ViewOf<UIAnchor, UIStyle>();
  for (auto [entity, anchor, style] : uiStyleView) {
    resources::Mesh *pMesh = fr.GetMesh(style.meshHandle);
    resources::Material *pMat = fr.GetMaterial(style.matHandle);
    if (pMesh == nullptr || pMat == nullptr) {
      continue;
    }

    float32 ndcX = anchor.normalizedX * 2 - 1, ndcY = anchor.normalizedY * 2 - 1;
    float32 drawScaleX = anchor.aspectCorrectX ? anchor.scaleX / aspect : anchor.scaleX;
    // quadAspect: physical width / physical height in screen pixels.
    // aspectCorrectX divides scaleX by aspect, which cancels when multiplied by the pixel ratio.
    float32 quadAspect = anchor.aspectCorrectX
                             ? anchor.scaleX / anchor.scaleY
                             : (anchor.scaleX / anchor.scaleY) * aspect;
    math::Mat4D model = math::Mat4D::Translation(ndcX, ndcY, 0.0f) *
                        math::Mat4D::RotationZ(anchor.rotation) *
                        math::Mat4D::Scale(drawScaleX, anchor.scaleY, 1.0f);

    RenderObject object{
        .geometryId = pMesh->id,
        .model = model,
        .uvOffset = style.uvOffset,
        .uvScale = style.uvScale,
        .layer = RenderLayer::UI,
        .pMaterial = pMat,
        .tint = style.tint,
        .useTexture = style.useTexture,
        .cornerRadius = style.cornerRadius,
        .quadAspect = quadAspect,
    };
    packet.uiObjects.Push(object);
  }
}

void PackUIText(RenderPacket &packet, AssetManager &am, FrontendRenderer &fr, Registry &reg) {
  View<UIAnchor, UIText> uiTextView = reg.ViewOf<UIAnchor, UIText>();
  for (auto [entity, anchor, uiText] : uiTextView) {
    if (uiText.handle == resources::cInvalidFontHandle) {
      continue;
    }

    resources::FontAtlas *pAtlas = am.GetFontAtlas(uiText.handle);
    if (pAtlas == nullptr) {
      continue;
    }

    resources::Mesh *pMesh = fr.GetMesh(pAtlas->quadMeshHandle);
    resources::Material *pMat = fr.GetMaterial(pAtlas->matHandle);
    if (pMesh == nullptr || pMat == nullptr) {
      continue;
    }

    float32 ndcX = anchor.normalizedX * 2.0f - 1.0f;
    float32 ndcY = anchor.normalizedY * 2.0f - 1.0f;
    float32 lineH = pAtlas->lineHeight;
    float32 aspect = fr.AspectRatio();

    stringv text = uiText.text.View();

    // Pre-pass: measure advance and vertical extent for centering
    float32 totalAdvance = 0.0f;
    float32 maxAbove = 0.0f; // px above baseline  (-bearingY, bearingY is negative for above)
    float32 maxBelow = 0.0f; // px below baseline  (bearingY + height, positive = below)
    for (char c : text) {
      if (c < 32 || c > 126) {
        continue;
      }
      const resources::GlyphInfo &g = pAtlas->glyphs[c - 32];
      totalAdvance += g.advance;
      if (-g.bearingY > maxAbove) { maxAbove = -g.bearingY; }
      if (g.bearingY + g.height > maxBelow) { maxBelow = g.bearingY + g.height; }
    }
    float32 cursorX = -totalAdvance * 0.5f;
    // Shift baseline so the visual text block is centered on ndcY.
    // (maxAbove - maxBelow)/2 is the distance from midpoint to baseline.
    float32 ndcYAdj = ndcY - (maxAbove - maxBelow) * 0.5f / lineH * anchor.scaleY;

    for (char c : text) {
      if (c < 32 || c > 126) {
        continue;
      }

      const resources::GlyphInfo &glyph = pAtlas->glyphs[c - 32];

      // X is divided by aspect so glyphs are square in pixel space
      float32 glyphW = (glyph.width / lineH) * anchor.scaleY / aspect;
      float32 glyphH = (glyph.height / lineH) * anchor.scaleY;

      // cursorX is in pixels — normalize with lineH alongside bearing/size
      float32 centerX = ndcX + ((cursorX + glyph.bearingX + glyph.width * 0.5f) / lineH) * anchor.scaleY / aspect;
      float32 centerY = ndcYAdj - (glyph.bearingY + glyph.height * 0.5f) / lineH * anchor.scaleY;

      math::Mat4D model = math::Mat4D::Translation(centerX, centerY, 0.0f) *
                          math::Mat4D::Scale(glyphW, glyphH, 1.0f);

      // flip UV Y to correct atlas orientation
      math::Vec2D uvOffset = {glyph.uvOffset.x(), glyph.uvOffset.y() + glyph.uvSize.y()};
      math::Vec2D uvScale  = {glyph.uvSize.x(), -glyph.uvSize.y()};

      RenderObject object{
          .geometryId = pMesh->id,
          .model = model,
          .uvOffset = uvOffset,
          .uvScale = uvScale,
          .layer = RenderLayer::UI,
          .pMaterial = pMat,
          .tint = uiText.color,
          .useTexture = 1.0f,
      };
      packet.uiObjects.Push(object);

      cursorX += glyph.advance; // pixels
    }
  }
}

} // namespace packer

} // namespace flatearth::renderer

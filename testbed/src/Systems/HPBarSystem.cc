#include "HPBarSystem.hpp"

#include "../Components/Tags.hpp"

#include <Core/Logger.hpp>
#include <UI/Components/UIAnchor.hpp>
#include <UI/Components/UIStyle.hpp>
#include <UI/Components/UIText.hpp>

#include <algorithm>
#include <format>

namespace flatearth::testbed {

// Bar layout constants (NDC units, Y+ up)
static constexpr float32 cBarLeftNdc    = -0.95f;
static constexpr float32 cBarMaxWidth   = 0.25f;   // NDC width at full health
static constexpr float32 cBarHeight     = 0.045f;
static constexpr float32 cBarNormY      = 0.93f;   // near top-left

static constexpr float32 cBgNormX = (cBarLeftNdc + cBarMaxWidth * 0.5f + 1.0f) / 2.0f;

HPBarSystem::HPBarSystem(EngineContext &ctx, scene::SceneId sceneId)
    : _ctx(ctx), _sceneId(sceneId) {
}

void HPBarSystem::Initialize(ecs::Registry &registry) {
  auto fontRes = _ctx.assets.Manager()
                     .LoadFont("hud_font", "assets/fonts/stocky.ttf", 36.0f, 512)
                     .or_error("HPBarSystem: failed to load hud font");
  if (!fontRes.errored()) {
    _fontHandle = fontRes.value();
  }

  // Use any already-cached texture as backing for the solid-color quads
  // (useTexture=0 so the actual texture content is irrelevant)
  auto texRes = _ctx.assets.Manager().LoadTexture("assets/textures/Human_Walk.png");
  if (texRes.errored()) {
    FLOG_WARN("HPBarSystem: cannot load backing texture, bars will not render");
    return;
  }

  auto bgSprRes = _ctx.assets.Manager().SpriteFromTexture(texRes.value(), "hp_bar_bg");
  if (bgSprRes.errored()) {
    return;
  }
  auto fillSprRes = _ctx.assets.Manager().SpriteFromTexture(texRes.value(), "hp_bar_fill");
  if (fillSprRes.errored()) {
    return;
  }

  const auto &bgSpr   = bgSprRes.value();
  const auto &fillSpr = fillSprRes.value();

  // Background bar — dark, full width, spawned first so it draws underneath
  ui::UIAnchor bgAnchor{};
  bgAnchor.normalizedX = cBgNormX;
  bgAnchor.normalizedY = cBarNormY;
  bgAnchor.scaleX      = cBarMaxWidth;
  bgAnchor.scaleY      = cBarHeight;

  ui::UIStyle bgStyle{};
  bgStyle.meshHandle  = bgSpr.meshHandle;
  bgStyle.matHandle   = bgSpr.matHandle;
  bgStyle.tint        = {{0.1f, 0.1f, 0.1f}, 0.85f};
  bgStyle.useTexture  = 0.0f;

  _bgBarEntity = registry.Spawn()
                     .With(bgAnchor)
                     .With(bgStyle)
                     .OwnedBy(_sceneId)
                     .Commit();

  // Fill bar — red, starts full, spawned after bg so it draws on top
  ui::UIAnchor fillAnchor{};
  fillAnchor.normalizedX = cBgNormX;
  fillAnchor.normalizedY = cBarNormY;
  fillAnchor.scaleX      = cBarMaxWidth;
  fillAnchor.scaleY      = cBarHeight * 0.7f;

  ui::UIStyle fillStyle{};
  fillStyle.meshHandle = fillSpr.meshHandle;
  fillStyle.matHandle  = fillSpr.matHandle;
  fillStyle.tint       = {{0.85f, 0.15f, 0.1f}, 1.0f};
  fillStyle.useTexture = 0.0f;

  _fillBarEntity = registry.Spawn()
                       .With(fillAnchor)
                       .With(fillStyle)
                       .OwnedBy(_sceneId)
                       .Commit();

  // HP label above the bar
  if (_fontHandle != resources::cInvalidFontHandle) {
    ui::UIAnchor textAnchor{};
    textAnchor.normalizedX = cBgNormX;
    textAnchor.normalizedY = cBarNormY + 0.04f;
    textAnchor.scaleX      = 0.05f;
    textAnchor.scaleY      = 0.05f;

    ui::UIText uiText{};
    uiText.handle = _fontHandle;
    uiText.color  = {{1.0f, 1.0f, 1.0f}, 1.0f};
    uiText.text.Set("100/100");

    _textEntity = registry.Spawn().With(textAnchor).OwnedBy(_sceneId).Commit();
    registry.Insert<ui::UIText>(_textEntity, uiText);
  }
}

void HPBarSystem::Update(ecs::Registry &registry, float32) {
  Health *pHealth = nullptr;
  for (auto [id, ptag, health] : registry.ViewOf<PlayerTag, Health>()) {
    pHealth = &health;
    break;
  }
  if (pHealth == nullptr) {
    return;
  }

  float32 ratio = std::clamp(pHealth->current / pHealth->max, 0.0f, 1.0f);

  if (_fillBarEntity != ecs::cNullEntity) {
    ui::UIAnchor *pFill = registry.TryGet<ui::UIAnchor>(_fillBarEntity);
    if (pFill) {
      float32 fillWidth       = ratio * cBarMaxWidth;
      pFill->scaleX           = fillWidth;
      pFill->normalizedX      = (cBarLeftNdc + fillWidth * 0.5f + 1.0f) / 2.0f;
    }
  }

  if (_textEntity != ecs::cNullEntity) {
    ui::UIText *pText = registry.TryGet<ui::UIText>(_textEntity);
    if (pText) {
      pText->text.Set(std::format("{}/{}", static_cast<int32>(pHealth->current),
                                  static_cast<int32>(pHealth->max)));
    }
  }
}

} // namespace flatearth::testbed

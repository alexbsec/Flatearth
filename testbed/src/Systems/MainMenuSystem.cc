#include "MainMenuSystem.hpp"

#include <Core/Logger.hpp>
#include <Resources/ResourceTypes.hpp>
#include <UI/Components/UIAnchor.hpp>
#include <UI/Components/UIButton.hpp>
#include <UI/Components/UIStyle.hpp>
#include <UI/Components/UIText.hpp>

namespace flatearth::testbed {

static void OnStartGame(void *ud) {
  static_cast<Orchestrator *>(ud)->RequestTransition(GamePhase::Playing);
}

static void OnQuitGame(void *ud) {
  static_cast<EngineContext *>(ud)->core.RequestQuit();
}

MainMenuSystem::MainMenuSystem(EngineContext &ctx, Orchestrator &orchestrator,
                               scene::SceneId sceneId)
    : _ctx(ctx), _orchestrator(orchestrator), _sceneId(sceneId) {
}

void MainMenuSystem::Initialize(ecs::Registry &registry) {
  auto fontRes = _ctx.assets.Manager()
                     .LoadFont("menu_font", "assets/fonts/stocky.ttf", 64.0f, 1024)
                     .or_error("MainMenuSystem: failed to load menu font");
  if (fontRes.errored()) {
    return;
  }
  resources::FontHandle fontHandle = fontRes.value();

  auto titleFontRes = _ctx.assets.Manager()
                          .LoadFont("menu_title_font", "assets/fonts/stocky.ttf", 120.0f, 2048)
                          .or_error("MainMenuSystem: failed to load title font");
  if (!titleFontRes.errored()) {
    ui::UIAnchor titleAnchor{};
    titleAnchor.normalizedX = 0.5f;
    titleAnchor.normalizedY = 0.75f;
    titleAnchor.scaleX      = 0.5f;
    titleAnchor.scaleY      = 0.13f;

    ui::UIText titleText{};
    titleText.handle = titleFontRes.value();
    titleText.color  = {{1.0f, 1.0f, 1.0f}, 1.0f};
    titleText.text.Set("Flatearth Testbed");

    auto titleEntity = registry.Spawn().With(titleAnchor).OwnedBy(_sceneId).Commit();
    registry.Insert<ui::UIText>(titleEntity, titleText);
  }

  auto texRes = _ctx.assets.Manager().LoadTexture("assets/textures/Human_Walk.png");
  if (texRes.errored()) {
    FLOG_WARN("MainMenuSystem: cannot load backing texture");
    return;
  }

  auto btnSprRes =
      _ctx.assets.Manager().SpriteFromTexture(texRes.value(), "menu_btn_mat");
  if (btnSprRes.errored()) {
    return;
  }
  resources::MeshHandle btnMesh = btnSprRes.value().meshHandle;
  resources::MaterialHandle btnMat = btnSprRes.value().matHandle;

  // Layout: two centered buttons stacked vertically
  struct BtnDef {
    const char *label;
    float32 normY;
    void (*callback)(void *);
    void *userData;
  };

  BtnDef defs[2] = {
      {"Start Game", 0.55f, OnStartGame, &_orchestrator},
      {"Quit Game",  0.40f, OnQuitGame,  &_ctx},
  };

  constexpr float32 cBtnNormX  = 0.5f;
  // scaleX is in height-normalized units (aspectCorrectX=true) so
  // the button width is consistent relative to text at any aspect ratio.
  // Text "Start Game" at scaleY=0.055 occupies ~0.35 height-units wide;
  // 0.50 gives a comfortable margin on all screens.
  constexpr float32 cBtnScaleX = 0.50f;
  constexpr float32 cBtnScaleY = 0.08f;
  constexpr float32 cLblScaleY = 0.055f;

  for (auto &def : defs) {
    ui::UIAnchor btnAnchor{};
    btnAnchor.normalizedX    = cBtnNormX;
    btnAnchor.normalizedY    = def.normY;
    btnAnchor.scaleX         = cBtnScaleX;
    btnAnchor.scaleY         = cBtnScaleY;
    btnAnchor.aspectCorrectX = FeTrue;

    ui::UIStyle btnStyle{};
    btnStyle.meshHandle = btnMesh;
    btnStyle.matHandle  = btnMat;
    btnStyle.tint       = {{0.75f, 0.75f, 0.75f}, 1.0f};
    btnStyle.useTexture = 0.0f;
    btnStyle.cornerRadius = 0.5f;

    ui::UIButton btn{};
    btn.pFn_OnClick    = def.callback;
    btn.onClickUserData = def.userData;

    registry.Spawn()
        .With(btnAnchor)
        .With(btnStyle)
        .With(btn)
        .OwnedBy(_sceneId)
        .Commit();

    // Label entity — same anchor position, smaller scale
    ui::UIAnchor lblAnchor{};
    lblAnchor.normalizedX = cBtnNormX;
    lblAnchor.normalizedY = def.normY;
    lblAnchor.scaleX      = cBtnScaleX;
    lblAnchor.scaleY      = cLblScaleY;

    ui::UIText lblText{};
    lblText.handle = fontHandle;
    lblText.color  = {{0.05f, 0.05f, 0.05f}, 1.0f};
    lblText.text.Set(def.label);

    auto lblEntity = registry.Spawn().With(lblAnchor).OwnedBy(_sceneId).Commit();
    registry.Insert<ui::UIText>(lblEntity, lblText);
  }
}

} // namespace flatearth::testbed

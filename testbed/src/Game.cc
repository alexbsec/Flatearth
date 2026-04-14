#include "Game.hpp"

#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Math/FeMath.hpp>
#include <Renderer/RendererFrontend.hpp>
#include <Resources/MaterialSystem.hpp>
#include <Resources/TextureSystem.hpp>
#include <Renderer/RendererTypes.hpp>

namespace flatearth::testbed {

GameState GameTest::_state = GameState{};

static void RecalculateCameraView(GameState &state) {
  if (!state.cameraViewDirty)
    return;

  state.view = math::Mat4D::Translation(-state.cameraPosition.x(), -state.cameraPosition.y(), 0.0f);

  state.cameraViewDirty = FeFalse;
}

bool GameTest::GameInitialize(flatearth::Game *gameInstance) {
  gameInstance->windowStartWidth = 1280;
  gameInstance->windowStartHeight = 720;
  gameInstance->windowStartPosX = 100;
  gameInstance->windowStartPosY = 100;
  gameInstance->gameName = "Testbed";

  _state.cameraPosition = math::Vec3D(0.0f, 0.0f, -5.0f);
  _state.cameraEuler = math::Vec3D::Zero();
  _state.view = math::Mat4D::Identity();
  _state.cameraViewDirty = FeTrue;
  return FeTrue;
}

bool GameTest::GameLoad(flatearth::Game *gameInstance) {
  std::array<math::Vertex3D, 4> verts{};
  verts[0].position = math::Vec3D(-0.5f, -0.5f, 0.0f);
  verts[1].position = math::Vec3D(0.5f, 0.5f, 0.0f);
  verts[2].position = math::Vec3D(-0.5f, 0.5f, 0.0f);
  verts[3].position = math::Vec3D(0.5f, -0.5f, 0.0f);

  verts[0].uv = math::Vec2D::Zero();
  verts[1].uv = math::Vec2D::Right();
  verts[2].uv = math::Vec2D::One();
  verts[3].uv = math::Vec2D::Up();

  std::array<uint32, 6> indices = {0, 2, 1, 0, 3, 1};

  auto geomRes = gameInstance->pRenderer->CreateGeometry(
      _state.quadGeometry, 4, verts.data(), 6, indices.data());
  if (!geomRes.has_value()) {
    FLOG_ERROR("failed to create quad geometry");
    return FeFalse;
  }

  geomRes = gameInstance->pRenderer->CreateGeometry(
      _state.quad2Geometry, 4, verts.data(), 6, indices.data());
  if (!geomRes.has_value()) {
    FLOG_ERROR("failed to create second quad geometry");
    return FeFalse;
  }

  auto texRes = gameInstance->pTextureSystem->AcquireTexture("assets/textures/texture.jpg");
  if (!texRes.has_value()) {
    FLOG_ERROR("failed to load texture");
    return FeFalse;
  }

  _state.texHandle = texRes.value();

  auto matRes = gameInstance->pMaterialSystem->AcquireMaterial("quad_material", _state.texHandle);
  if (!matRes.has_value()) {
    FLOG_ERROR("failed to acquire material");
    return FeFalse;
  }

  _state.matHandle = matRes.value();
  _state.pMaterial = gameInstance->pMaterialSystem->GetMaterial(_state.matHandle);

  auto tex2Res = gameInstance->pTextureSystem->AcquireTexture("assets/textures/rugtexture.jpg");
  if (!tex2Res.has_value()) {
    FLOG_ERROR("failed to load rug texture");
    return FeFalse;
  }

  _state.tex2Handle = tex2Res.value();

  auto mat2Res = gameInstance->pMaterialSystem->AcquireMaterial("rug_material", _state.tex2Handle);
  if (!mat2Res.has_value()) {
    FLOG_ERROR("failed to acquire rug material");
    return FeFalse;
  }

  _state.mat2Handle = mat2Res.value();
  _state.pMaterial2 = gameInstance->pMaterialSystem->GetMaterial(_state.mat2Handle);
  return FeTrue;
}

bool GameTest::GameUpdate(flatearth::Game *gameInstance, float32 deltaTime) {
  using namespace input;
  if (!gameInstance->pInputManager)
    return FeFalse;

  const float32 dy = 3.0f * deltaTime;
  const float32 dx = 3.0f * deltaTime;

  if (gameInstance->pInputManager->IsKeyDown(Keys::KEY_W)) {
    _state.cameraPosition = _state.cameraPosition + math::Vec3D(0.0f, +dy, 0.0f);
    _state.cameraViewDirty = FeTrue;
  }

  if (gameInstance->pInputManager->IsKeyDown(Keys::KEY_S)) {
    _state.cameraPosition = _state.cameraPosition + math::Vec3D(0.0f, -dy, 0.0f);
    _state.cameraViewDirty = FeTrue;
  }

  if (gameInstance->pInputManager->IsKeyDown(Keys::KEY_A)) {
    _state.cameraPosition = _state.cameraPosition + math::Vec3D(+dx, 0.0f, 0.0f);
    _state.cameraViewDirty = FeTrue;
  }

  if (gameInstance->pInputManager->IsKeyDown(Keys::KEY_D)) {
    _state.cameraPosition = _state.cameraPosition + math::Vec3D(-dx, 0.0f, 0.0f);
    _state.cameraViewDirty = FeTrue;
  }

  return FeTrue;
}

bool GameTest::GameRender(flatearth::Game *, renderer::RenderPacket &packet) {
  RecalculateCameraView(_state);
  packet.view = _state.view;

  _state.angle += packet.deltaTime * 1.5f;
  math::Mat4D model = math::Mat4D::RotationZ(_state.angle);
  packet.objects.Push({_state.quadGeometry, model, _state.pMaterial});

  _state.angle2 -= packet.deltaTime * 2.0f;
  math::Mat4D model2 =
      math::Mat4D::Translation(0.0f, 1.2f, 0.0f) * math::Mat4D::RotationZ(_state.angle2);
  packet.objects.Push({_state.quad2Geometry, model2, _state.pMaterial2});

  return FeTrue;
}

void GameTest::GameUnload(flatearth::Game *gameInstance) {
  if (gameInstance->pMaterialSystem) {
    gameInstance->pMaterialSystem->ReleaseMaterial(_state.matHandle);
    gameInstance->pMaterialSystem->ReleaseMaterial(_state.mat2Handle);
  }
  if (gameInstance->pTextureSystem) {
    gameInstance->pTextureSystem->ReleaseTexture(_state.texHandle);
    gameInstance->pTextureSystem->ReleaseTexture(_state.tex2Handle);
  }
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

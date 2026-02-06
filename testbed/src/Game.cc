#include "Game.hpp"

#include <Core/Input.hpp>
#include <Core/Logger.hpp>
#include <Defines.hpp>
#include <Math/FeMath.hpp>
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
    _state.cameraPosition = _state.cameraPosition + math::Vec3D(-dx, 0.0f, 0.0f);
    _state.cameraViewDirty = FeTrue;
  }

  if (gameInstance->pInputManager->IsKeyDown(Keys::KEY_D)) {
    _state.cameraPosition = _state.cameraPosition + math::Vec3D(dx, 0.0f, 0.0f);
    _state.cameraViewDirty = FeTrue;
  }

  return FeTrue;
}

bool GameTest::GameRender(flatearth::Game *, renderer::RenderPacket &packet) {
  RecalculateCameraView(_state);
  packet.view = _state.view;
  return FeTrue;
}

bool GameTest::GameOnResize(flatearth::Game *, uint32, uint32) {
  return FeTrue;
}

} // namespace flatearth::testbed

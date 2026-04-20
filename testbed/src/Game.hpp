#ifndef _FLATEARTH_TESTSUITE_GAME_HPP
#define _FLATEARTH_TESTSUITE_GAME_HPP

#include <Defines.hpp>
#include <GameTypes.hpp>
#include <Math/Matrix4D.hpp>
#include <Renderer/RendererTypes.hpp>
#include <Resources/MeshCache.hpp>
#include <Resources/ResourceTypes.hpp>

namespace flatearth::testbed {

struct GameState {
  float32 deltaTime;
  math::Mat4D view;
  math::Vec3D cameraPosition, cameraEuler;
  bool cameraViewDirty{false};

  resources::TextureHandle texHandle{0};
  resources::MaterialHandle matHandle{0};
  resources::Material *pMaterial{nullptr};

  resources::TextureHandle tex2Handle{0};
  resources::MaterialHandle mat2Handle{0};
  resources::Material *pMaterial2{nullptr};

  resources::MeshHandle quadMesh{0};
  resources::MeshHandle circleMesh{0};
  resources::MeshHandle capsuleMesh{0};
  float32 angle{0.0f};
  float32 angle2{0.0f};
  float32 angle3{0.0f};
};

class GameTest {
public:
  static bool GameInitialize(flatearth::Game *gameInstance);
  static bool GameLoad(flatearth::Game *gameInstance);
  static void GameUnload(flatearth::Game *gameInstance);
  static bool GameUpdate(flatearth::Game *gameInstance, float32 deltaTime);
  static bool GameRender(flatearth::Game *gameInstance, renderer::RenderPacket &packet);
  static bool GameOnResize(flatearth::Game *gameInstance, uint32 width, uint32 height);

private:
  static GameState _state;
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTSUITE_GAME_HPP

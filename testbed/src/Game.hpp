#ifndef _FLATEARTH_TESTSUITE_GAME_HPP
#define _FLATEARTH_TESTSUITE_GAME_HPP

#include <Defines.hpp>
#include <ECS/ECSTypes.hpp>
#include <GameTypes.hpp>
#include <Scene/Components/Camera2D.hpp>
#include <Scene/Components/Sprite.hpp>
#include <Scene/Components/Transform2D.hpp>

namespace flatearth::testbed {

struct GameState {
  ecs::EntityId cameraEntity{UINT32_MAX};
  ecs::EntityId quadEntity{UINT32_MAX};
  ecs::EntityId circleEntity{UINT32_MAX};
  ecs::EntityId capsuleEntity{UINT32_MAX};
  scene::Sprite quadSprite{};
  scene::Sprite circleSprite{};
  scene::Sprite capsuleSprite{};
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
  static bool GameOnResize(flatearth::Game *gameInstance, uint32 width, uint32 height);

private:
  static GameState _state;
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTSUITE_GAME_HPP

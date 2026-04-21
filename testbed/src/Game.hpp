#ifndef _FLATEARTH_TESTSUITE_GAME_HPP
#define _FLATEARTH_TESTSUITE_GAME_HPP

#include <Defines.hpp>
#include <ECS/ECSTypes.hpp>
#include <GameTypes.hpp>
#include <Scene/Components/Sprite.hpp>
#include <Scene/Components/Transform2D.hpp>

namespace flatearth::testbed {

struct GameState {
  ecs::EntityId cameraEntity{UINT32_MAX};
  ecs::EntityId playerEntity{UINT32_MAX};
  ecs::EntityId aiEntity{UINT32_MAX};
  ecs::EntityId ballEntity{UINT32_MAX};

  scene::Sprite paddleSprite{};
  scene::Sprite ballSprite{};

  float32 ballVx{1.5f};
  float32 ballVy{0.8f};
  bool    serveRight{true};
};

class GameTest {
public:
  static bool GameInitialize(flatearth::Game *gameInstance);
  static bool GameLoad(flatearth::Game *gameInstance);
  static void GameUnload(flatearth::Game *gameInstance);
  static bool GameUpdate(flatearth::Game *gameInstance, float32 deltaTime);
  static bool GameOnResize(flatearth::Game *gameInstance, uint32 width, uint32 height);

private:
  static void ResetBall(flatearth::Game *gameInstance);
  static GameState _state;
};

} // namespace flatearth::testbed

#endif // _FLATEARTH_TESTSUITE_GAME_HPP

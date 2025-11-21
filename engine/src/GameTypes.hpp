#ifndef _FLATEARTH_ENGINE_GAME_TYPES_HPP
#define _FLATEARTH_ENGINE_GAME_TYPES_HPP

#include "Defines.hpp"
#include <functional>

namespace flatearth {

struct Game {
  std::function<bool(struct Game *gameInstance)> Initialize;
  std::function<bool(struct Game *gameInstance, float32 deltaTime)> Update;
  std::function<bool(struct Game *gameInstance, float32 deltaTime)> Render;
  std::function<bool(struct Game *gameInstance, uint32 width, uint32 height)>
      OnResize;

  Game()
      : Initialize(nullptr), Update(nullptr), Render(nullptr),
        OnResize(nullptr) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_GAME_TYPES_HPP

#ifndef _FLATEARTH_ENGINE_GAME_TYPES_HPP
#define _FLATEARTH_ENGINE_GAME_TYPES_HPP

#include "Defines.hpp"

#include <functional>

namespace flatearth {
struct EngineContext;
}

namespace flatearth {

static constexpr int16 scDefaultStartWidth = 1080;
static constexpr int16 scDefaultStartHeight = 960;
const string cGameName = "Flatearth Engine Demo";

struct Game {
public:
  std::function<bool(struct Game *gameInstance)> Initialize;
  std::function<bool(struct Game *gameInstance)> Load;
  std::function<void(struct Game *gameInstance)> Unload;
  std::function<bool(struct Game *gameInstance, float32 deltaTime)> Update;
  std::function<bool(struct Game *gameInstance, uint32 width, uint32 height)> OnResize;

  explicit Game()
      : Initialize(nullptr), Load(nullptr), Unload(nullptr), Update(nullptr), OnResize(nullptr),
        gameName(cGameName), windowStartWidth(scDefaultStartWidth),
        windowStartHeight(scDefaultStartHeight) {}

  Game(const string &name)
      : Initialize(nullptr), Load(nullptr), Unload(nullptr), Update(nullptr), OnResize(nullptr),
        gameName(name), windowStartWidth(scDefaultStartWidth),
        windowStartHeight(scDefaultStartHeight) {}

public:
  string gameName;
  int32 windowStartPosX, windowStartPosY, windowStartWidth, windowStartHeight;
  EngineContext *pCtx{nullptr};
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_GAME_TYPES_HPP

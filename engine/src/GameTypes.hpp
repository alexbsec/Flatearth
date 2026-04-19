#ifndef _FLATEARTH_ENGINE_GAME_TYPES_HPP
#define _FLATEARTH_ENGINE_GAME_TYPES_HPP

#include "Core/Input.hpp"
#include "Defines.hpp"
#include "Renderer/RendererTypes.hpp"

#include <functional>

// TODO: remove this once EngineContext exists
namespace flatearth::resources {
class TextureSystem;
class MaterialSystem;
class MeshSystem;
} // namespace flatearth::resources

namespace flatearth::renderer {
class FrontendRenderer;
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
  std::function<bool(struct Game *gameInstance, renderer::RenderPacket &packet)> Render;
  std::function<bool(struct Game *gameInstance, uint32 width, uint32 height)> OnResize;

  Game()
      : Initialize(nullptr), Load(nullptr), Unload(nullptr), Update(nullptr), Render(nullptr),
        OnResize(nullptr), gameName(cGameName), windowStartWidth(scDefaultStartWidth),
        windowStartHeight(scDefaultStartHeight) {}

  Game(const string &name)
      : Initialize(nullptr), Load(nullptr), Unload(nullptr), Update(nullptr), Render(nullptr),
        OnResize(nullptr), gameName(name), windowStartWidth(scDefaultStartWidth),
        windowStartHeight(scDefaultStartHeight) {}

public:
  string gameName;
  int32 windowStartPosX, windowStartPosY, windowStartWidth, windowStartHeight;

  // TODO: these are only here so that we can get it up and running. REMOVE
  // LATER
  input::InputManager *pInputManager{nullptr};
  resources::TextureSystem *pTextureSystem{nullptr};
  resources::MaterialSystem *pMaterialSystem{nullptr};
  resources::MeshSystem *pMeshSystem{nullptr};
  renderer::FrontendRenderer *pRenderer{nullptr};
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_GAME_TYPES_HPP

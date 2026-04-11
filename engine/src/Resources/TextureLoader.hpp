#ifndef _FLATEARTH_ENGINE_RESOURCES_TEXTURE_LOADER_HPP
#define _FLATEARTH_ENGINE_RESOURCES_TEXTURE_LOADER_HPP

#include "Defines.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"

namespace flatearth::resources {

FeExpect<void, Error> LoadTexture(const string &path,
                                  platform::FileSystem &filesystem,
                                  renderer::FrontendRenderer &renderer,
                                  Texture *pTexture);

};

#endif // _FLATEARTH_ENGINE_RESOURCES_TEXTURE_LOADER_HPP

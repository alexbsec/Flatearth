#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_RENDERPASS_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_RENDERPASS_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

class RenderpassManager {
public:
  explicit RenderpassManager(memory::MemoryManager &memManager);
  ~RenderpassManager();

  FeExpect<void, Error> CreateRenderpass(Context &ctx, Renderpass *pRenderpass,
                                         float32 x, float32 y, float32 width,
                                         float32 height, float32 r, float32 g,
                                         float32 b, float32 a, float32 depth,
                                         uint32 stencil);

  FeExpect<void, Error> DestroyRenderpass(Context &ctx,
                                          Renderpass *pRenderpass);

  void BeginRenderpass(Context &ctx, CommandBuffer *pCmdBuffer,
                       Renderpass *pRenderpass, VkFramebuffer frameBuffer);

  void EndRenderpass(Context &ctx, CommandBuffer *pCmdBuffer,
                     Renderpass *pRenderpass);

private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_RENDERPASS_MANAGER_HPP

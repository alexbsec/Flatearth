#include "VulkanRenderpassManager.hpp"

namespace flatearth::renderer::vulkan {

RenderpassManager::RenderpassManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

RenderpassManager::~RenderpassManager() {
  FLOG_INFO("renderpass exited gracefully");
}

FeExpect<void, Error> RenderpassManager::CreateRenderpass(
    Context &ctx, Renderpass *pRenderpass, float32 x, float32 y, float32 width,
    float32 height, float32 r, float32 g, float32 b, float32 a, float32 depth,
    uint32 stencil) {

  return {};
}

FeExpect<void, Error>
RenderpassManager::DestroyRenderpass(Context &ctx, Renderpass *pRenderpass) {
  return {};
}

void RenderpassManager::BeginRenderpass(Context &ctx, CommandBuffer *pCmdBuffer,
                                        Renderpass *pRenderpass,
                                        VkFramebuffer frameBuffer) {}

void RenderpassManager::EndRenderpass(Context &ctx, CommandBuffer *pCmdBuffer,
                                      Renderpass *pRenderpass) {}

} // namespace flatearth::renderer::vulkan

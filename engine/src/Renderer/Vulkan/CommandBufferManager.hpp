#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

class CommandBufferManager {
public:
  explicit CommandBufferManager(memory::MemoryManager &memManager);
  ~CommandBufferManager();

  FeExpect<void, Error> AllocateBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                       VkCommandPool pool, bool isPrimary);

  FeExpect<void, Error> FreeBuffer(Context &ctx, CommandBuffer *pCmdBuffer);

  void BeginBuffer(Context &ctx, CommandBuffer &cmdBuffer, bool isSingleUse,
                   bool isRenderpassContinue, bool isSimultaneousUse);

  void EndBuffer(Context &ctx, CommandBuffer &cmdBuffer);

  void ResetBuffer(Context &ctx, CommandBuffer *pCmdBuffer);


private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP

#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

class CommandBufferManager {
public:
  explicit CommandBufferManager(memory::MemoryManager &memManager);
  ~CommandBufferManager();

  FeExpect<void, Error> CreateBuffers(Context &ctx);
  FeExpect<void, Error> DestroyBuffers(Context &ctx);
  void BeginBuffer(Context &ctx, CommandBuffer &cmdBuffer, bool isSingleUse,
                   bool isRenderpassContinue, bool isSimultaneousUse);
  void EndBuffer(Context &ctx, CommandBuffer &cmdBuffer);
  void ResetBuffer(Context &ctx, CommandBuffer &cmdBuffer);

  template <class Fn>
  inline FeExpect<void, Error> ImmediateSubmit(Context &ctx, VkCommandPool pool,
                                               VkQueue queue, VkFence fence,
                                               Fn &&recordFn) {
    // Allocate a temporary primary command buffer from the given pool
    CommandBuffer tmp{};
    {
      auto allocRes = AllocateBuffer(ctx, &tmp, pool, /*isPrimary=*/FeTrue);
      if (!allocRes.has_value()) {
        FLOG_ERROR("ImmediateSubmit: failed to allocate temp command buffer");
        return FeErr{allocRes.error()};
      }
    }

    // Begin one-time submit recording
    BeginBuffer(ctx, tmp,
                /*isSingleUse=*/FeTrue,
                /*isRenderpassContinue=*/FeFalse,
                /*isSimultaneousUse=*/FeFalse);

    // Record commands
    // recordFn is expected to call vkCmd* commands using tmp.handle.
    recordFn(tmp.handle);

    // End recording
    EndBuffer(ctx, tmp);

    if (auto res = VkCheck(vkResetFences(ctx.device.logicalDevice, 1, &fence));
        !res.has_value()) {
      FLOG_ERROR("ImmediateSubmit: failed to reset fence");
      auto _ = FreeBuffer(ctx, &tmp, pool);
      return FeErr{res.error()};
    }

    // Submit
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tmp.handle;

    if (auto res = VkCheck(vkQueueSubmit(queue, 1, &submitInfo, fence));
        !res.has_value()) {
      FLOG_ERROR("ImmediateSubmit: vkQueueSubmit failed");
      auto _ = FreeBuffer(ctx, &tmp, pool);
      return FeErr{res.error()};
    }

    // Wait for completion (so resources used by recordFn are safe to
    // reuse/free)
    if (auto res = VkCheck(vkWaitForFences(ctx.device.logicalDevice, 1, &fence,
                                           VK_TRUE, UINT64_MAX));
        !res.has_value()) {
      FLOG_ERROR("ImmediateSubmit: vkWaitForFences failed");
      auto _ = FreeBuffer(ctx, &tmp, pool);
      return FeErr{res.error()};
    }

    // Free the temporary command buffer
    {
      auto freeRes = FreeBuffer(ctx, &tmp, pool);
      if (!freeRes.has_value()) {
        FLOG_ERROR("ImmediateSubmit: failed to free temp command buffer");
        return FeErr{freeRes.error()};
      }
    }

    return {};
  }

private:
  FeExpect<void, Error> AllocateBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                       VkCommandPool pool, bool isPrimary);
  FeExpect<void, Error> FreeBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                   VkCommandPool pool);

private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP

#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "VulkanTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

class CommandBufferManager {
public:
  explicit CommandBufferManager(memory::MemoryManager &memManager);
  ~CommandBufferManager();

  FeExpect<void, Error> CreateBuffers(Context &ctx);
  FeExpect<void, Error> DestroyBuffers(Context &ctx);
  void BeginBuffer(Context &ctx,
                   CommandBuffer &cmdBuffer,
                   bool isSingleUse,
                   bool isRenderpassContinue,
                   bool isSimultaneousUse);
  void EndBuffer(Context &ctx, CommandBuffer &cmdBuffer);
  void ResetBuffer(Context &ctx, CommandBuffer &cmdBuffer);

  template <class Fn>
  inline FeExpect<void, Error>
  ImmediateSubmit(Context &ctx, VkCommandPool pool, VkQueue queue, VkFence fence, Fn &&recordFn) {

    CommandBuffer tmp{};
    {
      auto allocRes = AllocateBuffer(ctx, &tmp, pool, /*isPrimary=*/FeTrue);
      if (!allocRes.has_value()) {
        FLOG_ERROR("failed to allocate temp command buffer");
        return FeErr{allocRes.error()};
      }
    }

    BeginBuffer(ctx, tmp, FeTrue, FeFalse, FeFalse);

    recordFn(tmp);

    EndBuffer(ctx, tmp);

    // Reset fence only if it's valid
    if (fence != VK_NULL_HANDLE) {
      if (auto res = VkCheck(vkResetFences(ctx.device.logicalDevice, 1, &fence));
          !res.has_value()) {
        FLOG_ERROR("failed to reset fence");
        auto _ = FreeBuffer(ctx, &tmp, pool);
        return FeErr{res.error()};
      }
    }

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tmp.handle;

    if (auto res = VkCheck(vkQueueSubmit(queue, 1, &submitInfo, fence)); !res.has_value()) {
      FLOG_ERROR("vkQueueSubmit failed");
      auto _ = FreeBuffer(ctx, &tmp, pool);
      return FeErr{res.error()};
    }

    // Wait for completion so it's safe to free temp resources
    if (fence != VK_NULL_HANDLE) {
      if (auto res =
              VkCheck(vkWaitForFences(ctx.device.logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));
          !res.has_value()) {
        FLOG_ERROR("vkWaitForFences failed");
        auto _ = FreeBuffer(ctx, &tmp, pool);
        return FeErr{res.error()};
      }
    } else {
      if (auto res = VkCheck(vkQueueWaitIdle(queue)); !res.has_value()) {
        FLOG_ERROR("vkQueueWaitIdle failed");
        auto _ = FreeBuffer(ctx, &tmp, pool);
        return FeErr{res.error()};
      }
    }

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
  FeExpect<void, Error>
  AllocateBuffer(Context &ctx, CommandBuffer *pCmdBuffer, VkCommandPool pool, bool isPrimary);
  FeExpect<void, Error> FreeBuffer(Context &ctx, CommandBuffer *pCmdBuffer, VkCommandPool pool);

private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_COMMAND_BUFFER_MANAGER_HPP

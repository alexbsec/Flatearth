#include "VulkanCommandBufferManager.hpp"
#include "Core/Logger.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

CommandBufferManager::CommandBufferManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

CommandBufferManager::~CommandBufferManager() {
  FLOG_INFO("command buffer exitted successfully");
}

FeExpect<void, Error> CommandBufferManager::CreateBuffers(Context &ctx) {
  if (ctx.graphicsCommandBuffer.Empty()) {
    ctx.graphicsCommandBuffer.Reserve(ctx.swapchain.imageCount);
    for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
      _memoryManager.FZeroMemory(&ctx.graphicsCommandBuffer[i],
                                sizeof(CommandBuffer));
    }
  }

  for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
    if (ctx.graphicsCommandBuffer[i].state != CmdBufferState::NotAllocated) {
      auto freeRes = FreeBuffer(ctx, &ctx.graphicsCommandBuffer[i],
                                ctx.device.graphicsCommandPool);
      if (!freeRes.has_value()) {
        FLOG_ERROR("failed to free buffer at index {}", i);
        return FeErr{freeRes.error()};
      }
    }
    _memoryManager.FZeroMemory(&ctx.graphicsCommandBuffer[i],
                              sizeof(CommandBuffer));
    auto allocRes = AllocateBuffer(ctx, &ctx.graphicsCommandBuffer[i],
                                   ctx.device.graphicsCommandPool, FeTrue);
    if (!allocRes.has_value()) {
      FLOG_ERROR("failed to allocate buffer at inidex {}", i);
      return FeErr{allocRes.error()};
    }
  }

  return {};
}

FeExpect<void, Error> CommandBufferManager::DestroyBuffers(Context &ctx) {
  if (ctx.graphicsCommandBuffer.Empty()) {
    FLOG_DEBUG("no buffer to destroy");
    return {};
  }

  for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
    if (ctx.graphicsCommandBuffer[i].handle == nullptr) {
      continue;
    }

    auto freeRes = FreeBuffer(ctx, &ctx.graphicsCommandBuffer[i],
                              ctx.device.graphicsCommandPool);
    if (!freeRes.has_value()) {
      FLOG_ERROR("failed to free buffer at index {}", i);
      return FeErr{freeRes.error()};
    }
  }

  return {};
}

FeExpect<void, Error>
CommandBufferManager::AllocateBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                     VkCommandPool pool, bool isPrimary) {
  // just to make sure
  _memoryManager.FZeroMemory(pCmdBuffer, sizeof(CommandBuffer));

  VkCommandBufferAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = pool;
  allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                              : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandBufferCount = 1;
  allocInfo.pNext = nullptr;

  pCmdBuffer->state = CmdBufferState::NotAllocated;
  if (auto res = VkCheck(vkAllocateCommandBuffers(
          ctx.device.logicalDevice, &allocInfo, &pCmdBuffer->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to allocate command buffers");
    return FeErr{res.error()};
  }
  pCmdBuffer->state = CmdBufferState::Ready;
  return {};
}

FeExpect<void, Error>
CommandBufferManager::FreeBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                 VkCommandPool pool) {
  constexpr uint32 cCmdBufferCount = 1;
  vkFreeCommandBuffers(ctx.device.logicalDevice, pool, cCmdBufferCount,
                       &pCmdBuffer->handle);
  return {};
}

void CommandBufferManager::BeginBuffer(Context &ctx, CommandBuffer &cmdBuffer,
                                       bool isSingleUse,
                                       bool isRenderpassContinue,
                                       bool isSimultaneousUse) {
  VkCommandBufferBeginInfo beginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = 0;
  if (isSingleUse) {
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  }

  if (isRenderpassContinue) {
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  }

  if (isSimultaneousUse) {
    beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  }

  if (auto res = VkCheck(vkBeginCommandBuffer(cmdBuffer.handle, &beginInfo));
      !res.has_value()) {
    FLOG_ERROR("failed to begin command buffer");
    return;
  }
  cmdBuffer.state = CmdBufferState::Recording;
}

void CommandBufferManager::EndBuffer(Context &ctx, CommandBuffer &cmdBuffer) {
  if (auto res = VkCheck(vkEndCommandBuffer(cmdBuffer.handle));
      !res.has_value()) {
    FLOG_ERROR("failed to end command buffer");
    return;
  }
  cmdBuffer.state = CmdBufferState::RecordingEnded;
}

void CommandBufferManager::ResetBuffer(Context &ctx, CommandBuffer &cmdBuffer) {
  cmdBuffer.state = CmdBufferState::Ready;
}

} // namespace flatearth::renderer::vulkan

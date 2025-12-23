#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_SWAPCHAIN_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_SWAPCHAIN_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
namespace flatearth::renderer::vulkan {

FeExpect<void, Error> QuerySwapchainSupport(VkPhysicalDevice device,
                                            VkSurfaceKHR surface,
                                            SwapchainSupportInfo &outInfo,
                                            memory::MemoryManager &memManager);

class SwapchainManager {
public:
  explicit SwapchainManager(memory::MemoryManager &memManager);

  FeExpect<void, Error> CreateSwapchain(Context &ctx, Swapchain *pSwapchain,
                                        uint32 width, uint32 height);
  FeExpect<void, Error> RecreateSwapchain(Context &ctx, Swapchain *pSwapchain,
                                          uint32 width, uint32 height);
  FeExpect<void, Error> DestroySwapchain(Context &ctx, Swapchain *pSwapchain);

  FeExpect<void, Error> AcquireNextImage(Context &ctx, uint64 timeoutNs,
                                         VkSemaphore imageAvailableSemaphore,
                                         VkFence fence, uint32 *outImageIndex);

  FeExpect<void, Error> PresentSwapchain(Context &ctx, VkQueue graphicsQueue,
                                         VkQueue presentQueue,
                                         VkSemaphore renderCompleteSemaphore,
                                         uint32 presentImageIndex);

private:
  FeExpect<void, Error> CreateLogic(Context &ctx, Swapchain *pSwapchain,
                                    uint32 width, uint32 height);
  FeExpect<void, Error> DestroyLogic(Context &ctx, Swapchain *pSwapchain);

private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_SWAPCHAIN_MANAGER_HPP

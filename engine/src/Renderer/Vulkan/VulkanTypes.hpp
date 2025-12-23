#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_TYPES_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_TYPES_HPP

#include "Containers/DArray.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

inline FeExpect<void, Error> VkCheck(VkResult result) {
  if (result != VK_SUCCESS) {
    FLOG_ERROR("Vulkan call failed: {}", (int)result);
    return FeErr{Error("Vulkan call failed", ErrorType::RendererVulkanError)};
  }
  return {};
}

struct SwapchainSupportInfo {
  VkSurfaceCapabilitiesKHR capabilities;
  uint32 formatCount;
  VkSurfaceFormatKHR *pFormats;
  uint32 presentModeCount;
  VkPresentModeKHR *pPresentMode;
};

struct Device {
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
  SwapchainSupportInfo swapchainSupportInfo;
  int32 graphicsQueueIndex, presentQueueIndex, transferQueueIndex;
  VkCommandPool graphicsCommandPool;
  VkFormat depthFormat;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkQueue transferQueue;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memoryProperties;
};

struct Image {
  VkImage handle;
  VkDeviceMemory deviceMemory;
  VkImageView view;
  uint32 width, height;
};

enum class RenderpassState {
  Ready,
  Recording,
  InRenderpass,
  RecordingEnded,
  Submitted,
  NotAllocated,
};

struct Renderpass {
  VkRenderPass handle;
  float32 x, y, width, height;
  float32 r, g, b, a;
  float32 depth;
  uint32 stencil;
  RenderpassState state;
};

struct FrameBuffer {
  VkFramebuffer handle;
  uint32 attachmentCount;
  VkImageView *pAttachments;
  Renderpass *pRenderpass;
};

struct Swapchain {
  VkSwapchainKHR handle;
  VkSurfaceFormatKHR imageFormat;
  uint8 maxFrames;
  VkImage *pImages;
  VkImageView *pViews;
  Image depthAttachment;
  containers::DArray<FrameBuffer> framebuffers;

  explicit Swapchain(memory::MemoryManager &memManager)
      : framebuffers(memManager) {}
};

enum class CmdBufferState {
  Ready,
  Recording,
  InRenderpass,
  RecordingEnded,
  Submitted,
  NotAllocated,
};

struct CommandBuffer {
  VkCommandBuffer handle;
  CmdBufferState state;
};

struct Fence {
  VkFence handle;
  bool isSignaled;
};

struct ShaderStage {
  VkShaderModule handle;
  VkShaderModuleCreateInfo shaderModuleCreateInfo;
  VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
};

struct Context {
  uint32 framebufferWidth, framebufferHeight;
  uint64 framebufferSizeGeneration;
  uint64 framebufferSizeLastGeneration;
  uint32 currentFrame;
  uint32 imageIndex;
  VkInstance instance;
  VkAllocationCallbacks *pAllocator;
  VkSurfaceKHR surface;
  Device device;
  Swapchain swapchain;
  Renderpass mainRenderpass;
  containers::DArray<CommandBuffer> graphicsCommandBuffer;
  containers::DArray<VkSemaphore> imageAvailableSemaphores;
  containers::DArray<VkSemaphore> queueCompleteSemaphores;

  uint32 inFlightFenceCount;
  containers::DArray<Fence> inFlightFences;
  containers::DArray<Fence *> imagesInFlight;

  bool recreatingSwapchain;

  explicit Context(memory::MemoryManager &memManager)
      : swapchain(memManager), graphicsCommandBuffer(memManager),
        imageAvailableSemaphores(memManager),
        queueCompleteSemaphores(memManager), inFlightFences(memManager),
        imagesInFlight(memManager) {}

  int32 FindMemoryIndex(uint32 typeFilter, uint32 propertyFlags) {
    VkPhysicalDeviceMemoryProperties memoryProps;
    vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProps);
    for (uint32 i = 0; i < memoryProps.memoryTypeCount; i++) {
      if (typeFilter & (1 << i) &&
          memoryProps.memoryTypes[i].propertyFlags & propertyFlags) {
        return i;
      }
    }

    FLOG_WARN("unable to find memory type");
    return -1;
  }

#if defined(_DEBUG)
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_TYPES_HPP

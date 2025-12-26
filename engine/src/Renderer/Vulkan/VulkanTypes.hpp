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

inline bool VkResultIsSuccess(VkResult result) {
  // From:
  // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
  switch (result) {
    // Success Codes
  default:
  case VK_SUCCESS:
  case VK_NOT_READY:
  case VK_TIMEOUT:
  case VK_EVENT_SET:
  case VK_EVENT_RESET:
  case VK_INCOMPLETE:
  case VK_SUBOPTIMAL_KHR:
  case VK_THREAD_IDLE_KHR:
  case VK_THREAD_DONE_KHR:
  case VK_OPERATION_DEFERRED_KHR:
  case VK_OPERATION_NOT_DEFERRED_KHR:
  case VK_PIPELINE_COMPILE_REQUIRED_EXT:
    return FeTrue;

  // Error codes
  case VK_ERROR_OUT_OF_HOST_MEMORY:
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
  case VK_ERROR_INITIALIZATION_FAILED:
  case VK_ERROR_DEVICE_LOST:
  case VK_ERROR_MEMORY_MAP_FAILED:
  case VK_ERROR_LAYER_NOT_PRESENT:
  case VK_ERROR_EXTENSION_NOT_PRESENT:
  case VK_ERROR_FEATURE_NOT_PRESENT:
  case VK_ERROR_INCOMPATIBLE_DRIVER:
  case VK_ERROR_TOO_MANY_OBJECTS:
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
  case VK_ERROR_FRAGMENTED_POOL:
  case VK_ERROR_SURFACE_LOST_KHR:
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
  case VK_ERROR_OUT_OF_DATE_KHR:
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
  case VK_ERROR_INVALID_SHADER_NV:
  case VK_ERROR_OUT_OF_POOL_MEMORY:
  case VK_ERROR_INVALID_EXTERNAL_HANDLE:
  case VK_ERROR_FRAGMENTATION:
  case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
  // NOTE: Same as above
  // case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
  case VK_ERROR_UNKNOWN:
    return FeFalse;
  }
}

struct SwapchainSupportInfo {
  VkSurfaceCapabilitiesKHR capabilities{};
  uint32 formatCount{0};
  VkSurfaceFormatKHR *pFormats{nullptr};
  uint32 formatsCapacity{0};

  uint32 presentModeCount{0};
  VkPresentModeKHR *pPresentMode{nullptr};
  uint32 presentModesCapacity{0};
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
  VkImageView *pAttachments{nullptr};
  Renderpass *pRenderpass{nullptr};
};

struct Swapchain {
  VkSwapchainKHR handle;
  VkSurfaceFormatKHR imageFormat;
  uint8 maxFrames{3};
  VkImage *pImages{nullptr};
  VkImageView *pViews{nullptr};
  Image depthAttachment;
  uint32 imageCount;
  uint32 viewsCapacity;
  uint32 imagesCapacity;
  uint32 widthExtent{0}, heightExtent{0};
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
  VkCommandBuffer handle{nullptr};
  CmdBufferState state{CmdBufferState::NotAllocated};
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
  uint64 framebufferSizeGeneration{0};
  uint64 framebufferSizeLastGeneration{0};
  uint32 currentFrame{0};
  uint32 imageIndex{0};
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

  bool recreatingSwapchain{false};

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

FeExpect<void, Error> DetectDeviceDepthFormat(Device &device);

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_TYPES_HPP

#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_DEVICE_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_DEVICE_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

struct PhysicalDeviceRequirements {
  bool graphics;
  bool present;
  bool compute;
  bool transfer;
  containers::DArray<const char *> deviceExtNames;
  bool samplerAnisotropy;
  bool discreteGPU;

  explicit PhysicalDeviceRequirements(memory::MemoryManager &memManager) 
    : deviceExtNames(memManager) {}
};

struct PhysicalDeviceQueueFamilyInfo {
  uint32 graphicsFamilyIndex;
  uint32 presentFamilyIndex;
  uint32 computeFamilyIndex;
  uint32 transferFamilyIndex;
};

class DeviceManager {
public:
  explicit DeviceManager(memory::MemoryManager &memManager);
  FeExpect<void, Error> CreateDevice(Context &ctx);
  FeExpect<void, Error> DestroyDevice(Context &ctx);
  FeExpect<void, Error> DetectDeviceDepthFormat(Device &device);

private:
  bool SelectPhysicalDevice(Context &ctx);
  bool PhysicalDeviceMeetsRequirements(
      VkPhysicalDevice device, VkSurfaceKHR surface,
      const VkPhysicalDeviceProperties *properties,
      const VkPhysicalDeviceFeatures *features,
      const PhysicalDeviceRequirements *requirements,
      PhysicalDeviceQueueFamilyInfo *outQueueInfo,
      SwapchainSupportInfo *outSwapchainInfo);

private:
  memory::MemoryManager &_memoryManager;
};

}

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_DEVICE_MANAGER_HPP

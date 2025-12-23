#include "VulkanDeviceManager.hpp"
#include "VulkanSwapchainManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "VulkanTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

static constexpr uint32 scMaxQueueTypes = 4;


DeviceManager::DeviceManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

FeExpect<void, Error> DeviceManager::CreateDevice(Context &ctx) {
  if (!SelectPhysicalDevice(ctx)) {
    return FeErr{Error("failed to select physical device",
                       ErrorType::RendererSelectPhysicalDevice)};
  }

  FLOG_INFO("creating logical device");
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(ctx.device.physicalDevice,
                                           &queueFamilyCount, nullptr);
  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(ctx.device.physicalDevice,
                                           &queueFamilyCount, queueFamilies);

  uint32 indices[scMaxQueueTypes];
  uint32 indexCount = 0;

  auto AddUniqueQueueIndex = [&](int32 queueIndex) -> void {
    bool exists = FeFalse;
    for (uint32 i = 0; i < indexCount; i++) {
      if (indices[i] == queueIndex) {
        exists = FeTrue;
        break;
      }
    }
    if (!exists && queueIndex < queueFamilyCount) {
      indices[indexCount++] = queueIndex;
    }
  };

  AddUniqueQueueIndex(ctx.device.graphicsQueueIndex);
  AddUniqueQueueIndex(ctx.device.presentQueueIndex);
  AddUniqueQueueIndex(ctx.device.transferQueueIndex);

  VkDeviceQueueCreateInfo queueCreateInfo[indexCount];
  for (uint32 i = 0; i < indexCount; i++) {
    queueCreateInfo[i].sType = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo[i].queueFamilyIndex = indices[i];

    uint32 maxQueues = queueFamilies[indices[i]].queueCount;
    queueCreateInfo[i].queueCount =
        (indices[i] == ctx.device.graphicsQueueIndex && maxQueues >= 2) ? 2 : 1;

    queueCreateInfo[i].flags = 0;
    queueCreateInfo[i].pNext = nullptr;
    float32 queuePriority = 1.0f;
    queueCreateInfo[i].pQueuePriorities = &queuePriority;
  }

  // TODO: make this not hardcoded
  VkPhysicalDeviceFeatures deviceFeats{};
  deviceFeats.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceCreateInfo.queueCreateInfoCount = indexCount;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
  deviceCreateInfo.pEnabledFeatures = &deviceFeats;
  deviceCreateInfo.enabledExtensionCount = 1;
  const char *extNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  deviceCreateInfo.ppEnabledExtensionNames = &extNames;
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = nullptr;

  if (auto res =
          VkCheck(vkCreateDevice(ctx.device.physicalDevice, &deviceCreateInfo,
                                 ctx.pAllocator, &ctx.device.logicalDevice));
      !res.has_value()) {
    FLOG_FATAL("failed to create logical device");
    return FeErr{res.error()};
  }

  FLOG_INFO("logical device created");

  VkCommandPoolCreateInfo poolCreateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolCreateInfo.queueFamilyIndex = ctx.device.graphicsQueueIndex;
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (auto res = VkCheck(vkCreateCommandPool(ctx.device.logicalDevice,
                                             &poolCreateInfo, ctx.pAllocator,
                                             &ctx.device.graphicsCommandPool));
      !res.has_value()) {
    FLOG_FATAL("failed to create command pool");
    return FeErr{res.error()};
  }

  FLOG_INFO("graphics command pool created");
  return {};
}

FeExpect<void, Error> DeviceManager::DestroyDevice(Context &ctx) {
  // If we have a device, we must ensure nothing is in-flight before teardown.
  if (ctx.device.logicalDevice != VK_NULL_HANDLE) {
    // REQUIRED: make sure all queues are done before destroying pools/device.
    vkDeviceWaitIdle(ctx.device.logicalDevice);

    // Handles are Vulkan handles, not pointers.
    ctx.device.graphicsQueue = VK_NULL_HANDLE;
    ctx.device.presentQueue = VK_NULL_HANDLE;
    ctx.device.transferQueue = VK_NULL_HANDLE;

    FLOG_DEBUG("destroying command pools");
    if (ctx.device.graphicsCommandPool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(ctx.device.logicalDevice,
                           ctx.device.graphicsCommandPool, ctx.pAllocator);
      ctx.device.graphicsCommandPool = VK_NULL_HANDLE;
    }

    FLOG_DEBUG("destroying logical device");
    vkDestroyDevice(ctx.device.logicalDevice, ctx.pAllocator);
    ctx.device.logicalDevice = VK_NULL_HANDLE;
    FLOG_DEBUG("logical device destroyed");
  }

  // Release cached swapchain support arrays (CPU-side allocations).
  FLOG_DEBUG("releasing physical device resources");

  if (ctx.device.swapchainSupportInfo.pFormats != nullptr) {
    _memoryManager.RawFree(ctx.device.swapchainSupportInfo.pFormats,
                           sizeof(VkSurfaceFormatKHR) *
                               ctx.device.swapchainSupportInfo.formatCount,
                           memory::Tag::Renderer);
    ctx.device.swapchainSupportInfo.pFormats = nullptr;
    ctx.device.swapchainSupportInfo.formatCount = 0;
  }

  if (ctx.device.swapchainSupportInfo.pPresentMode != nullptr) {
    _memoryManager.RawFree(ctx.device.swapchainSupportInfo.pPresentMode,
                           sizeof(VkPresentModeKHR) *
                               ctx.device.swapchainSupportInfo.presentModeCount,
                           memory::Tag::Renderer);
    ctx.device.swapchainSupportInfo.pPresentMode = nullptr;
    ctx.device.swapchainSupportInfo.presentModeCount = 0;
  }

  ctx.device.graphicsQueueIndex = -1;
  ctx.device.presentQueueIndex = -1;
  ctx.device.transferQueueIndex = -1;

  return {};
}

bool DeviceManager::SelectPhysicalDevice(Context &ctx) {
  uint32 physicalDeviceCount = 0;
  if (auto res = VkCheck(vkEnumeratePhysicalDevices(
          ctx.instance, &physicalDeviceCount, nullptr));
      !res.has_value()) {
    FLOG_FATAL("failed to enumerate physical devices: {}", res.error().message);
    return FeFalse;
  }

  if (physicalDeviceCount == 0) {
    FLOG_FATAL("no device which support Vulkan were found");
    return FeFalse;
  }

  VkPhysicalDevice physicalDevices[physicalDeviceCount];
  if (auto res = VkCheck(vkEnumeratePhysicalDevices(
          ctx.instance, &physicalDeviceCount, physicalDevices));
      !res.has_value()) {
    FLOG_FATAL("failed to store enumerated physical devices: {}",
               res.error().message);
    return FeFalse;
  }

  for (uint32 i = 0; i < physicalDeviceCount; i++) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures(physicalDevices[i], &feats);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memProps);

    PhysicalDeviceRequirements requirements(_memoryManager);
    requirements.graphics = FeTrue;
    requirements.present = FeTrue;
    requirements.transfer = FeTrue;
    requirements.compute = FeFalse;
    requirements.samplerAnisotropy = FeTrue;
    requirements.discreteGPU = FeFalse;
    requirements.deviceExtNames.Push(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    PhysicalDeviceQueueFamilyInfo queueInfo;
    bool result = PhysicalDeviceMeetsRequirements(
        physicalDevices[i], ctx.surface, &props, &feats, &requirements,
        &queueInfo, &ctx.device.swapchainSupportInfo);
    if (!result) {
      FLOG_ERROR("no physical devices were found which meet the requirements");
      return FeFalse;
    }

    FLOG_INFO("selected device: {}", props.deviceName);
    switch (props.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
      FLOG_INFO("GPU type is unknown");
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      FLOG_INFO("GPU type is integrated");
      break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      FLOG_INFO("GPU type is discrete");
      break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      FLOG_INFO("GPU type is virtual");
      break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      FLOG_INFO("GPU type is CPU");
      break;
    default:
      FLOG_WARN("GPU type could not be identified");
      break;
    }

    FLOG_INFO("GPU driver version: {}.{}.{}",
              VK_VERSION_MAJOR(props.driverVersion),
              VK_VERSION_MINOR(props.driverVersion),
              VK_VERSION_PATCH(props.driverVersion));

    FLOG_INFO(
        "Vulkan API version: {}.{}.{}", VK_VERSION_MAJOR(props.apiVersion),
        VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));

    for (uint32 j = 0; j < memProps.memoryHeapCount; j++) {
      float32 memorySizeGiba = (((float32)memProps.memoryHeaps[j].size) /
                                1024.0f / 1024.0f / 1024.0f);

      if (memProps.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
        FLOG_INFO("Local GPU memory: {:.2f} GiB", memorySizeGiba);
      } else {
        FLOG_INFO("Shared System memory: {:.2f} GiB", memorySizeGiba);
      }
    }

    ctx.device.physicalDevice = physicalDevices[i];
    ctx.device.graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
    ctx.device.presentQueueIndex = queueInfo.presentFamilyIndex;
    ctx.device.transferQueueIndex = queueInfo.transferFamilyIndex;

    ctx.device.properties = props;
    ctx.device.features = feats;
    ctx.device.memoryProperties = memProps;
    break;
  }

  return FeTrue;
}

bool DeviceManager::PhysicalDeviceMeetsRequirements(
    VkPhysicalDevice device, VkSurfaceKHR surface,
    const VkPhysicalDeviceProperties *properties,
    const VkPhysicalDeviceFeatures *features,
    const PhysicalDeviceRequirements *requirements,
    PhysicalDeviceQueueFamilyInfo *outQueueInfo,
    SwapchainSupportInfo *outSwapchainInfo) {

  // Reset outputs
  outQueueInfo->graphicsFamilyIndex = -1;
  outQueueInfo->computeFamilyIndex = -1;
  outQueueInfo->presentFamilyIndex = -1;
  outQueueInfo->transferFamilyIndex = -1;

  // --- GPU type requirement --------------------------------------------------
  if (requirements->discreteGPU &&
      properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    FLOG_INFO("device is not a discrete GPU, and one is required. Skipping.");
    return FeFalse;
  }

  // --- Queue family discovery ------------------------------------------------
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  VkQueueFamilyProperties queueFamilies[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies);

  FLOG_INFO("Graphics | Present | Compute | Transfer | Name");

  uint8 minTransferScore = 255;

  for (uint32 i = 0; i < queueFamilyCount; ++i) {
    uint8 score = 0;

    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      outQueueInfo->graphicsFamilyIndex = i;
      ++score;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      outQueueInfo->computeFamilyIndex = i;
      ++score;
    }

    if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
        score <= minTransferScore) {
      minTransferScore = score;
      outQueueInfo->transferFamilyIndex = i;
    }

    VkBool32 supportsPresent = VK_FALSE;
    auto res = VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                                            &supportsPresent));

    if (!res.has_value()) {
      FLOG_FATAL("failed to get physical device surface support: {}",
                 res.error().message);
      return FeFalse;
    }

    if (supportsPresent) {
      outQueueInfo->presentFamilyIndex = i;
    }
  }

  FLOG_INFO("       {} |       {} |       {} |        {} | {}",
            outQueueInfo->graphicsFamilyIndex, outQueueInfo->presentFamilyIndex,
            outQueueInfo->computeFamilyIndex, outQueueInfo->transferFamilyIndex,
            properties->deviceName);

  // --- Queue requirements ----------------------------------------------------
  auto require = [](bool needed, int index) { return !needed || index != -1; };

  if (!require(requirements->graphics, outQueueInfo->graphicsFamilyIndex) ||
      !require(requirements->present, outQueueInfo->presentFamilyIndex) ||
      !require(requirements->compute, outQueueInfo->computeFamilyIndex) ||
      !require(requirements->transfer, outQueueInfo->transferFamilyIndex)) {
    return FeFalse;
  }

  FLOG_INFO("Device meets all queue requirements");
  FLOG_TRACE("Graphics Family Index: {}", outQueueInfo->graphicsFamilyIndex);
  FLOG_TRACE("Presentation Family Index: {}", outQueueInfo->presentFamilyIndex);
  FLOG_TRACE("Compute Family Index: {}", outQueueInfo->computeFamilyIndex);
  FLOG_TRACE("Transfer Family Index: {}", outQueueInfo->transferFamilyIndex);

  // --- Swapchain support -----------------------------------------------------
  auto swapRes =
      QuerySwapchainSupport(device, surface, *outSwapchainInfo, _memoryManager);

  FLOG_TRACE("swapchain queried");

  if (!swapRes.has_value()) {
    FLOG_ERROR("failed to query swapchain support: {}",
               swapRes.error().message);
    return FeFalse;
  }

  if (outSwapchainInfo->formatCount < 1 ||
      outSwapchainInfo->presentModeCount < 1) {
    FLOG_INFO("Required swapchain support not present, skipping device.");
    return FeFalse;
  }

  // --- Device extensions -----------------------------------------------------
  if (requirements->deviceExtNames.Empty()) {
    return FeTrue; 
  }

  uint32 extCount = 0;
  if (auto res = VkCheck(vkEnumerateDeviceExtensionProperties(
          device, nullptr, &extCount, nullptr));
      !res.has_value()) {
    FLOG_ERROR("failed to get device extension property count");
    return FeFalse;
  }

  if (extCount == 0) {
    // Nenhuma extensão exposta — quem exige swapchain vai falhar depois
    return FeTrue;
  }

  uint32 capacity = extCount;
  VkExtensionProperties *exts = nullptr;

  for (;;) {
    exts = FeCast<VkExtensionProperties>(_memoryManager.RawAlloc(
        sizeof(VkExtensionProperties) * capacity,
        alignof(VkExtensionProperties), memory::Tag::Renderer));

    uint32 written = capacity;
    VkResult r =
        vkEnumerateDeviceExtensionProperties(device, nullptr, &written, exts);

    if (r == VK_SUCCESS) {
      break;
    }

    _memoryManager.RawFree(FeCast<std::byte>(exts),
                           sizeof(VkExtensionProperties) * capacity,
                           memory::Tag::Renderer);

    if (r != VK_INCOMPLETE || written == 0) {
      FLOG_ERROR("vkEnumerateDeviceExtensionProperties failed");
      return FeFalse;
    }

    capacity = written; // retry com novo tamanho
  }

  // --- Feature requirements -------------------------------------------------
  if (requirements->samplerAnisotropy && !features->samplerAnisotropy) {
    FLOG_INFO("Device does not support sampler anisotropy, skipping.");
    return FeFalse;
  }

  return FeTrue;
}

} // namespace flatearth::renderer::vulkan

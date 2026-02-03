#include "VulkanDeviceManager.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "VulkanSwapchainManager.hpp"
#include "VulkanTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

static constexpr uint32 scMaxQueueTypes = 4;

FeExpect<void, Error> DetectDeviceDepthFormat(Device& device) {
  const uint64 candidateCount = 3;
  VkFormat candidates[candidateCount] = {
      VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

  uint32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  for (uint64 i = 0; i < candidateCount; i++) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device.physicalDevice, candidates[i], &props);

    if ((props.linearTilingFeatures & flags) == flags) {
      device.depthFormat = candidates[i];
      return {};
    } else if ((props.optimalTilingFeatures & flags) == flags) {
      device.depthFormat = candidates[i];
      return {};
    }
  }

  return FeErr{Error("no candidate was selected for the device depth format",
                     ErrorType::RendererVulkanError)};
}

DeviceManager::DeviceManager(memory::MemoryManager& memManager) : _memoryManager(memManager) {
}

FeExpect<void, Error> DeviceManager::CreateDevice(Context& ctx) {
  if (!SelectPhysicalDevice(ctx)) {
    return FeErr{
        Error("failed to select physical device", ErrorType::RendererSelectPhysicalDevice)};
  }

  FLOG_INFO("creating logical device");
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(ctx.device.physicalDevice, &queueFamilyCount, nullptr);
  containers::DArray<VkQueueFamilyProperties> queueFamilies(_memoryManager);
  queueFamilies.Reserve(queueFamilyCount);
  for (uint32 i = 0; i < queueFamilyCount; i++) {
    queueFamilies.Push(VkQueueFamilyProperties{});
  }
  vkGetPhysicalDeviceQueueFamilyProperties(
      ctx.device.physicalDevice, &queueFamilyCount, queueFamilies.Data());

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

  containers::DArray<VkDeviceQueueCreateInfo> queueCreateInfo(_memoryManager);
  queueCreateInfo.Reserve(indexCount);
  for (uint32 i = 0; i < indexCount; i++) {
    queueCreateInfo.Push(VkDeviceQueueCreateInfo{});
  }

  float32 queuePrio1[1] = {1.0f};
  float32 queuePrio2[2] = {1.0f, 1.0f};
  for (uint32 i = 0; i < indexCount; i++) {
    queueCreateInfo[i].sType = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo[i].queueFamilyIndex = indices[i];

    uint32 maxQueues = queueFamilies[indices[i]].queueCount;
    queueCreateInfo[i].queueCount =
        (indices[i] == ctx.device.graphicsQueueIndex && maxQueues >= 2) ? 2 : 1;

    queueCreateInfo[i].flags = 0;
    queueCreateInfo[i].pNext = nullptr;
    float32 queuePriority = 1.0f;
    queueCreateInfo[i].pQueuePriorities =
        (queueCreateInfo[i].queueCount == 2) ? queuePrio2 : queuePrio1;
  }

  // TODO: make this not hardcoded
  VkPhysicalDeviceFeatures deviceFeats{};
  deviceFeats.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceCreateInfo.queueCreateInfoCount = indexCount;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfo.Data();
  deviceCreateInfo.pEnabledFeatures = &deviceFeats;
  deviceCreateInfo.enabledExtensionCount = 1;
  const char* extNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  deviceCreateInfo.ppEnabledExtensionNames = &extNames;
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = nullptr;

  if (auto res = VkCheck(vkCreateDevice(
          ctx.device.physicalDevice, &deviceCreateInfo, ctx.pAllocator, &ctx.device.logicalDevice));
      !res.has_value()) {
    FLOG_FATAL("failed to create logical device");
    return FeErr{res.error()};
  }

  FLOG_INFO("logical device created");

  vkGetDeviceQueue(
      ctx.device.logicalDevice, ctx.device.graphicsQueueIndex, 0, &ctx.device.graphicsQueue);
  vkGetDeviceQueue(
      ctx.device.logicalDevice, ctx.device.presentQueueIndex, 0, &ctx.device.presentQueue);

  FLOG_DEBUG("creating graphics command pool");
  VkCommandPoolCreateInfo poolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolCreateInfo.queueFamilyIndex = ctx.device.graphicsQueueIndex;
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (auto res = VkCheck(vkCreateCommandPool(ctx.device.logicalDevice,
                                             &poolCreateInfo,
                                             ctx.pAllocator,
                                             &ctx.device.graphicsCommandPool));
      !res.has_value()) {
    FLOG_FATAL("failed to create command pool");
    return FeErr{res.error()};
  }

  FLOG_INFO("graphics command pool created");
  return {};
}

FeExpect<void, Error> DeviceManager::DestroyDevice(Context& ctx) {
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
      vkDestroyCommandPool(
          ctx.device.logicalDevice, ctx.device.graphicsCommandPool, ctx.pAllocator);
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
                           sizeof(VkSurfaceFormatKHR) * ctx.device.swapchainSupportInfo.formatCount,
                           memory::Tag::Renderer);
    ctx.device.swapchainSupportInfo.pFormats = nullptr;
    ctx.device.swapchainSupportInfo.formatCount = 0;
  }

  if (ctx.device.swapchainSupportInfo.pPresentMode != nullptr) {
    _memoryManager.RawFree(
        ctx.device.swapchainSupportInfo.pPresentMode,
        sizeof(VkPresentModeKHR) * ctx.device.swapchainSupportInfo.presentModeCount,
        memory::Tag::Renderer);
    ctx.device.swapchainSupportInfo.pPresentMode = nullptr;
    ctx.device.swapchainSupportInfo.presentModeCount = 0;
  }

  ctx.device.graphicsQueueIndex = -1;
  ctx.device.presentQueueIndex = -1;
  ctx.device.transferQueueIndex = -1;

  return {};
}

bool DeviceManager::SelectPhysicalDevice(Context& ctx) {
  uint32 physicalDeviceCount = 0;
  if (auto res = VkCheck(vkEnumeratePhysicalDevices(ctx.instance, &physicalDeviceCount, nullptr));
      !res.has_value()) {
    FLOG_FATAL("failed to enumerate physical devices: {}", res.error().message);
    return FeFalse;
  }

  if (physicalDeviceCount == 0) {
    FLOG_FATAL("no device which support Vulkan were found");
    return FeFalse;
  }

  containers::DArray<VkPhysicalDevice> physicalDevices(_memoryManager);
  physicalDevices.Reserve(physicalDeviceCount);
  for (uint32 i = 0; i < physicalDeviceCount; i++) {
    physicalDevices.Push(VkPhysicalDevice{});
  }

  if (auto res = VkCheck(
          vkEnumeratePhysicalDevices(ctx.instance, &physicalDeviceCount, physicalDevices.Data()));
      !res.has_value()) {
    FLOG_FATAL("failed to store enumerated physical devices: {}", res.error().message);
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
    bool result = PhysicalDeviceMeetsRequirements(physicalDevices[i],
                                                  ctx.surface,
                                                  &props,
                                                  &feats,
                                                  &requirements,
                                                  &queueInfo,
                                                  &ctx.device.swapchainSupportInfo);
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

    FLOG_INFO("Vulkan API version: {}.{}.{}",
              VK_VERSION_MAJOR(props.apiVersion),
              VK_VERSION_MINOR(props.apiVersion),
              VK_VERSION_PATCH(props.apiVersion));

    for (uint32 j = 0; j < memProps.memoryHeapCount; j++) {
      float32 memorySizeGiba =
          (((float32)memProps.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);

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

bool DeviceManager::PhysicalDeviceMeetsRequirements(VkPhysicalDevice device,
                                                    VkSurfaceKHR surface,
                                                    const VkPhysicalDeviceProperties* properties,
                                                    const VkPhysicalDeviceFeatures* features,
                                                    const PhysicalDeviceRequirements* requirements,
                                                    PhysicalDeviceQueueFamilyInfo* outQueueInfo,
                                                    SwapchainSupportInfo* outSwapchainInfo) {

  // Reset outputs
  outQueueInfo->graphicsFamilyIndex = -1;
  outQueueInfo->computeFamilyIndex = -1;
  outQueueInfo->presentFamilyIndex = -1;
  outQueueInfo->transferFamilyIndex = -1;

  // --- GPU type requirement --------------------------------------------------
  if (requirements->discreteGPU && properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    FLOG_INFO("device is not a discrete GPU, and one is required. Skipping.");
    return FeFalse;
  }

  // --- Queue family discovery ------------------------------------------------
  uint32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  if (queueFamilyCount == 0) {
    FLOG_INFO("device exposes 0 queue families, skipping.");
    return FeFalse;
  }

  VkQueueFamilyProperties* queueFamilies = FeCast<VkQueueFamilyProperties>(
      _memoryManager.RawAlloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount,
                              alignof(VkQueueFamilyProperties),
                              memory::Tag::Renderer));

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

  int32 graphicsFirst = -1;
  int32 presentFirst = -1;

  int32 graphicsPresent = -1;

  int32 computeAny = -1;
  int32 computeNoGraphics = -1;

  int32 transferAny = -1;
  int32 transferDedicated = -1; // transfer && !graphics && !compute

  for (uint32 i = 0; i < queueFamilyCount; ++i) {
    const VkQueueFlags flags = queueFamilies[i].queueFlags;

    const bool hasGraphics = (flags & VK_QUEUE_GRAPHICS_BIT) != 0;
    const bool hasCompute = (flags & VK_QUEUE_COMPUTE_BIT) != 0;
    const bool hasTransfer = (flags & VK_QUEUE_TRANSFER_BIT) != 0;

    VkBool32 supportsPresent = VK_FALSE;
    auto presRes =
        VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
    if (!presRes.has_value()) {
      FLOG_FATAL("failed to get physical device surface support: {}", presRes.error().message);
      _memoryManager.RawFree(FeCast<std::byte>(queueFamilies),
                             sizeof(VkQueueFamilyProperties) * queueFamilyCount,
                             memory::Tag::Renderer);
      return FeFalse;
    }

    // Prefer a single family that supports both graphics + present
    if (hasGraphics && supportsPresent && graphicsPresent == -1) {
      graphicsPresent = (int32)i;
    }

    if (hasGraphics && graphicsFirst == -1) {
      graphicsFirst = (int32)i;
    }

    if (supportsPresent && presentFirst == -1) {
      presentFirst = (int32)i;
    }

    // Compute: prefer compute-only (no graphics)
    if (hasCompute) {
      if (computeAny == -1)
        computeAny = (int32)i;
      if (!hasGraphics && computeNoGraphics == -1)
        computeNoGraphics = (int32)i;
    }

    // Transfer: prefer transfer-only (no graphics/compute)
    if (hasTransfer) {
      if (transferAny == -1)
        transferAny = (int32)i;
      if (!hasGraphics && !hasCompute && transferDedicated == -1) {
        transferDedicated = (int32)i;
      }
    }
  }

  // Final picks
  if (graphicsPresent != -1) {
    outQueueInfo->graphicsFamilyIndex = graphicsPresent;
    outQueueInfo->presentFamilyIndex = graphicsPresent;
  } else {
    outQueueInfo->graphicsFamilyIndex = graphicsFirst;
    outQueueInfo->presentFamilyIndex = presentFirst;
  }

  outQueueInfo->computeFamilyIndex = (computeNoGraphics != -1) ? computeNoGraphics : computeAny;

  // Transfer: prefer dedicated; else any transfer; else leave -1 (requirement check will fail if
  // needed)
  outQueueInfo->transferFamilyIndex = (transferDedicated != -1) ? transferDedicated : transferAny;

  FLOG_INFO("Graphics | Present | Compute | Transfer | Name");
  FLOG_INFO("       {} |       {} |       {} |        {} | {}",
            outQueueInfo->graphicsFamilyIndex,
            outQueueInfo->presentFamilyIndex,
            outQueueInfo->computeFamilyIndex,
            outQueueInfo->transferFamilyIndex,
            properties->deviceName);

  // Done with queueFamilies buffer
  _memoryManager.RawFree(FeCast<std::byte>(queueFamilies),
                         sizeof(VkQueueFamilyProperties) * queueFamilyCount,
                         memory::Tag::Renderer);

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
  auto swapRes = QuerySwapchainSupport(device, surface, *outSwapchainInfo, _memoryManager);

  FLOG_TRACE("swapchain queried");

  if (!swapRes.has_value()) {
    FLOG_ERROR("failed to query swapchain support: {}", swapRes.error().message);
    return FeFalse;
  }

  if (outSwapchainInfo->formatCount < 1 || outSwapchainInfo->presentModeCount < 1) {
    FLOG_INFO("Required swapchain support not present, skipping device.");
    return FeFalse;
  }

  // --- Device extensions (ACTUALLY validate) --------------------------------
  if (!requirements->deviceExtNames.Empty()) {
    uint32 extCount = 0;
    if (auto res =
            VkCheck(vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr));
        !res.has_value()) {
      FLOG_ERROR("failed to get device extension property count");
      return FeFalse;
    }

    if (extCount == 0) {
      FLOG_INFO("device exposes 0 extensions but extensions are required.");
      return FeFalse;
    }

    VkExtensionProperties* exts = FeCast<VkExtensionProperties>(
        _memoryManager.RawAlloc(sizeof(VkExtensionProperties) * extCount,
                                alignof(VkExtensionProperties),
                                memory::Tag::Renderer));

    uint32 written = extCount;
    VkResult r = vkEnumerateDeviceExtensionProperties(device, nullptr, &written, exts);
    if (r != VK_SUCCESS) {
      _memoryManager.RawFree(
          FeCast<std::byte>(exts), sizeof(VkExtensionProperties) * extCount, memory::Tag::Renderer);
      FLOG_ERROR("vkEnumerateDeviceExtensionProperties failed");
      return FeFalse;
    }

    // Check required extension names exist
    for (uint32 req = 0; req < requirements->deviceExtNames.Length(); ++req) {
      const char* needed = requirements->deviceExtNames[req];
      bool found = FeFalse;

      for (uint32 i = 0; i < written; ++i) {
        if (std::strcmp(needed, exts[i].extensionName) == 0) {
          found = FeTrue;
          break;
        }
      }

      if (!found) {
        FLOG_INFO("missing required device extension: {}", needed);
        _memoryManager.RawFree(FeCast<std::byte>(exts),
                               sizeof(VkExtensionProperties) * extCount,
                               memory::Tag::Renderer);
        return FeFalse;
      }
    }

    _memoryManager.RawFree(
        FeCast<std::byte>(exts), sizeof(VkExtensionProperties) * extCount, memory::Tag::Renderer);
  }

  // --- Feature requirements --------------------------------------------------
  if (requirements->samplerAnisotropy && !features->samplerAnisotropy) {
    FLOG_INFO("Device does not support sampler anisotropy, skipping.");
    return FeFalse;
  }

  return FeTrue;
}

} // namespace flatearth::renderer::vulkan

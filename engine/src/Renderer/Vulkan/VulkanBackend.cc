#include "VulkanBackend.hpp"
#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Platform/Platform.hpp"

namespace flatearth::renderer::vulkan {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, uint32 messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData);

VulkanBackend::VulkanBackend(memory::MemoryManager &memManager)
    : _memoryManager(memManager), _deviceManager(memManager),
      _swapchainManager(memManager, _imageManager),
      _renderpassManager(memManager), _cmdBufferManager(memManager),
      _ctx(memManager) {}

VulkanBackend::~VulkanBackend() {
  auto destroyRes = _cmdBufferManager.DestroyBuffers(_ctx);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}",
               destroyRes.error().message);
    return;
  }

  destroyRes =
      _renderpassManager.DestroyRenderpass(_ctx, &_ctx.mainRenderpass);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}",
               destroyRes.error().message);
    return;
  }

  destroyRes = _swapchainManager.DestroySwapchain(_ctx, &_ctx.swapchain);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}",
               destroyRes.error().message);
    return;
  }

  destroyRes = _deviceManager.DestroyDevice(_ctx);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}",
               destroyRes.error().message);
    return;
  }

  FLOG_INFO("Vulkan backend exited gracefully");
}

FeExpect<bool, Error> VulkanBackend::Initialize(ApplicationState *appState) {
  // TODO: custom allocator
  _ctx.pAllocator = nullptr;
  _cachedFrameBufferWidth = appState->width;
  _cachedFrameBufferHeight = appState->height;
  _ctx.framebufferWidth =
      (_cachedFrameBufferWidth != 0) ? _cachedFrameBufferWidth : 946;
  _ctx.framebufferHeight =
      (_cachedFrameBufferHeight != 0) ? _cachedFrameBufferHeight : 507;

  _cachedFrameBufferWidth = 0;
  _cachedFrameBufferHeight = 0;

  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_2;
  appInfo.pApplicationName = appState->appConfig.name.c_str();
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Flatearth Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;

  containers::DArray<const char *> requiredExtensions(_memoryManager);
  requiredExtensions.Push(VK_KHR_SURFACE_EXTENSION_NAME);
  platform::GetRequiredExtNames(&requiredExtensions);

#if defined(_DEBUG)
  requiredExtensions.Push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  FLOG_DEBUG("Required extensions:");
  uint32 len = requiredExtensions.Length();
  for (uint32 i = 0; i < len; i++) {
    FLOG_DEBUG(requiredExtensions[i]);
  }
#endif

  createInfo.enabledExtensionCount = requiredExtensions.Length();
  createInfo.ppEnabledExtensionNames = requiredExtensions.Data();

  containers::DArray<const char *> requiredValidationLayerNames(_memoryManager);
  uint32 requiredValidationLayerCount = 0;

#if defined(_DEBUG)
  FLOG_INFO("validation layers enabled. Enumarating...");

  requiredValidationLayerNames.Push("VK_LAYER_KHRONOS_validation");
  requiredValidationLayerCount = requiredValidationLayerNames.Length();

  // Obtain list of available layers
  uint32 availableLayerCount = 0;
  if (auto res = VkCheck(
          vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
      !res.has_value()) {
    FLOG_ERROR("failed to enumerate instance layer porperty count");
    return FeErr{res.error()};
  }

  containers::DArray<VkLayerProperties> availableLayers(_memoryManager);
  availableLayers.Reserve(availableLayerCount);
  if (auto res = VkCheck(vkEnumerateInstanceLayerProperties(
          &availableLayerCount, availableLayers.Data()));
      !res.has_value()) {
    FLOG_ERROR("failed to enumerate instance layer properties");
    return FeErr{res.error()};
  }

  // Verify if all layers are available
  for (uint32 i = 0; i < requiredValidationLayerCount; i++) {
    FLOG_INFO("Searching for layer: {}...", requiredValidationLayerNames[i]);
    bool found = FeFalse;
    for (uint32 j = 0; j < availableLayerCount; j++) {
      string reqValLayerStr(requiredValidationLayerNames[i]);
      string availableLayerStr(availableLayers[j].layerName);
      if (reqValLayerStr == availableLayerStr) {
        found = FeTrue;
        FLOG_INFO("Found!");
        break;
      }
    }

    if (!found) {
      FLOG_FATAL("Required validation layer is missing: {}",
                 requiredValidationLayerNames[i]);
      return FeFalse;
    }
  }

  FLOG_INFO("All required validation layers were found");
#endif

  createInfo.enabledLayerCount = requiredValidationLayerCount;
  createInfo.ppEnabledLayerNames = requiredValidationLayerNames.Data();

  if (auto res = VkCheck(
          vkCreateInstance(&createInfo, _ctx.pAllocator, &_ctx.instance));
      !res.has_value()) {
    FLOG_ERROR("failed to create Vulkan instance");
    return FeErr{res.error()};
  }

#if defined(_DEBUG)
  FLOG_DEBUG("Creating Vulkan debugger...");
  uint32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};

  debugCreateInfo.messageSeverity = logSeverity;
  debugCreateInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debugCreateInfo.pfnUserCallback = DebugCallback;

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          _ctx.instance, "vkCreateDebugUtilsMessengerEXT");
  if (auto res = VkCheck(func(_ctx.instance, &debugCreateInfo, _ctx.pAllocator,
                              &_ctx.debugMessenger));
      !res.has_value()) {
    FLOG_ERROR("failed to get instance proc address");
    return FeErr{res.error()};
  }
  FLOG_DEBUG("Vulkan debugger created");
#endif

  FLOG_DEBUG("creating Vulkan surface");
  if (auto res = platform::CreateVulkanSurface(appState->platformState, _ctx);
      !res.has_value()) {
    FLOG_ERROR("failed to create Vulkan surface");
    return FeErr{res.error()};
  }
  FLOG_DEBUG("Vulkan surface created");

  auto deviceRes = _deviceManager.CreateDevice(_ctx);
  if (!deviceRes.has_value()) {
    FLOG_ERROR("failed to create device");
    return FeErr{deviceRes.error()};
  }

  FLOG_DEBUG("creating swapchain");
  auto swapRes = _swapchainManager.CreateSwapchain(
      _ctx, &_ctx.swapchain, _ctx.framebufferWidth, _ctx.framebufferHeight);
  if (!swapRes.has_value()) {
    FLOG_ERROR("failed to create swapchain");
    return FeErr{swapRes.error()};
  }
  FLOG_INFO("swapchain created successfully");

  FLOG_DEBUG("creating renderpass");
  auto rpassRes = _renderpassManager.CreateRenderpass(
      _ctx, &_ctx.mainRenderpass, 0, 0, _ctx.framebufferWidth,
      _ctx.framebufferHeight, 1.0f, 1.0f, 0.3f, 1.0f, 1.0f, 0);
  if (!rpassRes.has_value()) {
    FLOG_ERROR("failed to create renderpass");
    return FeErr{rpassRes.error()};
  }
  FLOG_INFO("renderpass created successfully");

  FLOG_DEBUG("regenerating frame buffers");
  _ctx.swapchain.framebuffers.Reserve(_ctx.swapchain.imageCount);
  auto frameBufRes = _swapchainManager.RegenerateFrameBuffer(
      _ctx, &_ctx.swapchain, &_ctx.mainRenderpass);
  if (!frameBufRes.has_value()) {
    FLOG_ERROR("failed regenerating frame buffers");
    return FeErr{frameBufRes.error()};
  }
  FLOG_INFO("frame buffers regenerated successfully");

  FLOG_DEBUG("creating command buffers");
  auto cmdBufRes = _cmdBufferManager.CreateBuffers(_ctx);
  if (!cmdBufRes.has_value()) {
    FLOG_ERROR("failed to create command buffers");
    return FeErr{cmdBufRes.error()};
  }
  FLOG_INFO("command buffers created successfully");

  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::OnResize(uint32 width, uint32 height) {
  _cachedFrameBufferWidth = width;
  _cachedFrameBufferHeight = height;
  _ctx.framebufferSizeGeneration++;
  FLOG_INFO("w/h/gen: {}/{}/{}", width, height, _ctx.framebufferSizeGeneration);
  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::BeginFrame(float32 deltaTime) {
  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::EndFrame(float32 deltaTime) {
  return FeTrue;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, uint32 messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData) {
  switch (messageSeverity) {
  default:
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    FLOG_ERROR(callbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    FLOG_WARN(callbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    FLOG_INFO(callbackData->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    FLOG_TRACE(callbackData->pMessage);
    break;
  }

  // This is a must
  return VK_FALSE;
}

} // namespace flatearth::renderer::vulkan

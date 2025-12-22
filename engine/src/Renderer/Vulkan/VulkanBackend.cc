#include "VulkanBackend.hpp"
#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Platform/Platform.hpp"

namespace flatearth::renderer::vulkan {

VulkanBackend::VulkanBackend(memory::MemoryManager &memManager)
    : _memoryManager(memManager), _deviceManager(memManager), _ctx(memManager) {
}

VulkanBackend::~VulkanBackend() {
  auto destroyRes = _deviceManager.DestroyDevice(_ctx);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}",
               destroyRes.error().message);
    return;
  }

  FLOG_INFO("Vulkan backend exitted gracefully");
}

FeExpect<bool, Error> VulkanBackend::Initialize(ApplicationState *appState) {
  // TODO: custom allocator
  _ctx.pAllocator = nullptr;
  _cachedFrameBufferWidth = appState->width;
  _cachedFrameBufferHeight = appState->height;
  _ctx.framebufferWidth =
      (_cachedFrameBufferWidth != 0) ? _cachedFrameBufferWidth : 1280;
  _ctx.framebufferHeight =
      (_cachedFrameBufferHeight != 0) ? _cachedFrameBufferHeight : 920;

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

#if defined(FE_DEBUG)
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

  createInfo.enabledLayerCount = requiredValidationLayerCount;
  createInfo.ppEnabledLayerNames = requiredValidationLayerNames.Data();

  if (auto res = VkCheck(
          vkCreateInstance(&createInfo, _ctx.pAllocator, &_ctx.instance));
      !res.has_value()) {
    FLOG_ERROR("failed to create Vulkan instance");
    return FeErr{res.error()};
  }

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

  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::OnResize(uint32 width, uint32 height) {
  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::BeginFrame(float32 deltaTime) {
  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::EndFrame(float32 deltaTime) {
  return FeTrue;
}

} // namespace flatearth::renderer::vulkan

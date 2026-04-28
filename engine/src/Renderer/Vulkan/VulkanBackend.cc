#include "VulkanBackend.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Math/MathTypes.hpp"
#include "Platform/Platform.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include "Renderer/Vulkan/VulkanUtils.hpp"
#include "Resources/ResourceTypes.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              uint32 messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
              void *userData);

VulkanBackend::VulkanBackend(memory::MemoryManager &memManager, platform::FileSystem &fs)
    : _memoryManager(memManager), _deviceManager(memManager),
      _swapchainManager(memManager, _imageManager), _renderpassManager(memManager),
      _cmdBufferManager(memManager), _bufferManager(_cmdBufferManager),
      _vulkanShader(memManager, _bufferManager), _ctx(memManager, fs), _geometries(memManager) {
}

VulkanBackend::~VulkanBackend() {
  vkDeviceWaitIdle(_ctx.device.logicalDevice);
  ShutdownImGui();

  _bufferManager.DestroyVulkanBuffer(_ctx, &_ctx.objectVertexBuffer);
  _bufferManager.DestroyVulkanBuffer(_ctx, &_ctx.objectIndexBuffer);
  _vulkanShader.DestroyObjectShader(_ctx, &_ctx.objectShader);

  for (uint32 i = 0; i < _ctx.swapchain.imageCount; i++) {
    if (_ctx.queueCompleteSemaphores[i] != nullptr) {
      vkDestroySemaphore(
          _ctx.device.logicalDevice, _ctx.queueCompleteSemaphores[i], _ctx.pAllocator);

      _ctx.queueCompleteSemaphores[i] = nullptr;
    }
  }

  for (uint32 i = 0; i < _ctx.swapchain.maxFrames; i++) {
    if (_ctx.imageAvailableSemaphores[i] != nullptr) {
      vkDestroySemaphore(
          _ctx.device.logicalDevice, _ctx.imageAvailableSemaphores[i], _ctx.pAllocator);

      _ctx.imageAvailableSemaphores[i] = nullptr;
    }

    auto destroyRes = DestroyFence(&_ctx.inFlightFences[i]);
    if (!destroyRes.has_value()) {
      FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}", destroyRes.error().message);
      return;
    }
  }

  auto destroyRes = _cmdBufferManager.DestroyBuffers(_ctx);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}", destroyRes.error().message);
    return;
  }

  destroyRes = _renderpassManager.DestroyRenderpass(_ctx, &_ctx.mainRenderpass);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}", destroyRes.error().message);
    return;
  }

  destroyRes = _swapchainManager.DestroySwapchain(_ctx, &_ctx.swapchain);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}", destroyRes.error().message);
    return;
  }

  destroyRes = _deviceManager.DestroyDevice(_ctx);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("Vulkan backend did not shutdown gracefully: {}", destroyRes.error().message);
    return;
  }

  FLOG_INFO("Vulkan backend exited gracefully");
}

FeExpect<bool, Error> VulkanBackend::Initialize(EngineState *pEngState) {
  // TODO: custom allocator
  _ctx.pAllocator = nullptr;
  _cachedFrameBufferWidth = pEngState->width;
  _cachedFrameBufferHeight = pEngState->height;
  _ctx.framebufferWidth = (_cachedFrameBufferWidth != 0) ? _cachedFrameBufferWidth : 946;
  _ctx.framebufferHeight = (_cachedFrameBufferHeight != 0) ? _cachedFrameBufferHeight : 507;

  _cachedFrameBufferWidth = 0;
  _cachedFrameBufferHeight = 0;

  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.apiVersion = VK_API_VERSION_1_2;
  appInfo.pApplicationName = pEngState->pAppConfig->name.c_str();
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
  if (auto res = VkCheck(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
      !res.has_value()) {
    FLOG_ERROR("failed to enumerate instance layer porperty count");
    return FeErr{res.error()};
  }

  containers::DArray<VkLayerProperties> availableLayers(_memoryManager);
  availableLayers.Reserve(availableLayerCount);
  if (auto res =
          VkCheck(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.Data()));
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
      FLOG_FATAL("Required validation layer is missing: {}", requiredValidationLayerNames[i]);
      return FeFalse;
    }
  }

  FLOG_INFO("All required validation layers were found");
#endif

  createInfo.enabledLayerCount = requiredValidationLayerCount;
  createInfo.ppEnabledLayerNames = requiredValidationLayerNames.Data();

  if (auto res = VkCheck(vkCreateInstance(&createInfo, _ctx.pAllocator, &_ctx.instance));
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
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  debugCreateInfo.pfnUserCallback = DebugCallback;

  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_ctx.instance,
                                                                "vkCreateDebugUtilsMessengerEXT");
  if (auto res =
          VkCheck(func(_ctx.instance, &debugCreateInfo, _ctx.pAllocator, &_ctx.debugMessenger));
      !res.has_value()) {
    FLOG_ERROR("failed to get instance proc address");
    return FeErr{res.error()};
  }
  FLOG_DEBUG("Vulkan debugger created");
#endif

  FLOG_DEBUG("creating Vulkan surface");
  if (auto res = platform::CreateVulkanSurface(pEngState->platformState, _ctx); !res.has_value()) {
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
  auto rpassRes = _renderpassManager.CreateRenderpass(_ctx,
                                                      &_ctx.mainRenderpass,
                                                      0,
                                                      0,
                                                      _ctx.framebufferWidth,
                                                      _ctx.framebufferHeight,
                                                      0.08f,
                                                      0.08f,
                                                      0.10f,
                                                      1.0f,
                                                      1.0f,
                                                      0);
  if (!rpassRes.has_value()) {
    FLOG_ERROR("failed to create renderpass");
    return FeErr{rpassRes.error()};
  }
  FLOG_INFO("renderpass created successfully");

  FLOG_DEBUG("regenerating frame buffers");
  _ctx.swapchain.framebuffers.Reserve(_ctx.swapchain.imageCount);
  for (uint32 i = 0; i < _ctx.swapchain.imageCount; i++) {
    _ctx.swapchain.framebuffers.Push(FrameBuffer{});
  }

  auto frameBufRes =
      _swapchainManager.RegenerateFrameBuffer(_ctx, &_ctx.swapchain, &_ctx.mainRenderpass);
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

  FLOG_DEBUG("creating sync objects & fences");
  _ctx.imageAvailableSemaphores.Reserve(_ctx.swapchain.maxFrames);
  _ctx.inFlightFences.Reserve(_ctx.swapchain.maxFrames);
  for (uint32 i = 0; i < _ctx.swapchain.maxFrames; i++) {
    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;
    if (auto res = VkCheck(vkCreateSemaphore(_ctx.device.logicalDevice,
                                             &semaphoreInfo,
                                             _ctx.pAllocator,
                                             &_ctx.imageAvailableSemaphores[i]));
        !res.has_value()) {
      FLOG_ERROR("failed to create image available semaphore");
      return FeErr{res.error()};
    }

    auto fenceRes = CreateFence(&_ctx.inFlightFences[i], FeTrue);
    if (!fenceRes.has_value()) {
      FLOG_ERROR("failed to create in-flight fence");
      return FeErr{fenceRes.error()};
    }
  }

  _ctx.queueCompleteSemaphores.Reserve(_ctx.swapchain.imageCount);
  for (uint32 i = 0; i < _ctx.swapchain.imageCount; i++) {
    VkSemaphoreCreateInfo semaphoreInfo = {

        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;
    if (auto res = VkCheck(vkCreateSemaphore(_ctx.device.logicalDevice,
                                             &semaphoreInfo,
                                             _ctx.pAllocator,
                                             &_ctx.queueCompleteSemaphores[i]));
        !res.has_value()) {
      FLOG_ERROR("failed to create queue complete semaphore");
      return FeErr{res.error()};
    }
  }

  // At this point no in flight fences exist, so clear the array. These
  // are stored in pointers because initial state must be nullptr and will
  // be nullptr when not in use. Actual fences are not owned by this array
  _ctx.imagesInFlight.Reserve(_ctx.swapchain.imageCount);
  for (uint32 i = 0; i < _ctx.swapchain.imageCount; i++) {
    _memoryManager.FZeroMemory(&_ctx.imagesInFlight[i], sizeof(Fence *));
    _ctx.imagesInFlight[i] = nullptr;
  }

  // create builtin shaders
  auto shaderRes = _vulkanShader.CreateObjectShader(_ctx, &_ctx.objectShader);
  if (!shaderRes.has_value()) {
    FLOG_ERROR("failed to create object shader");
    return FeErr{shaderRes.error()};
  }

  // create vulkan buffer
  auto createBufferRes = CreateBuffers();
  if (!createBufferRes.has_value()) {
    FLOG_ERROR("vulkan buffers creation failed");
    return FeErr{createBufferRes.error()};
  }

  InitImGui();

  FLOG_INFO("sync objects & fences created successfully");
  FLOG_INFO("Vulkan backend initialized successfully");
  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::OnResize(uint32 width, uint32 height) {
  if (width == 0 || height == 0) {
    FLOG_WARN("attempted to resize framebuffer to zero dimension");
    return FeFalse;
  }

  _cachedFrameBufferWidth = width;
  _cachedFrameBufferHeight = height;
  _ctx.framebufferSizeGeneration++;
  FLOG_INFO("w/h/gen: {}/{}/{}", width, height, _ctx.framebufferSizeGeneration);
  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::BeginFrame(float32 deltaTime) {
  Device &device = _ctx.device;

  // Reset per-frame draw state
  _cpLastBoundMaterial = nullptr;
  _lastBoundGeometry = UINT32_MAX;
  _frameStatebound = FeFalse;

  // check if recreating swapchain is happening
  if (_ctx.recreatingSwapchain) {
    VkResult res = vkDeviceWaitIdle(device.logicalDevice);
    if (!VkResultIsSuccess(res)) {
      FLOG_ERROR("device lost while waiting for idle");
      return FeErr{Error("device lost while waiting for idle", ErrorType::RendererVulkanError)};
    }

    FLOG_INFO("recreating swapchain on begin frame");
    return FeFalse;
  }

  if (_ctx.framebufferSizeGeneration != _ctx.framebufferSizeLastGeneration) {
    FLOG_INFO("framebuffer size generation changed, recreating swapchain");
    VkResult res = vkDeviceWaitIdle(device.logicalDevice);
    if (!VkResultIsSuccess(res)) {
      FLOG_ERROR("device lost while waiting for idle");
      return FeErr{Error("device lost while waiting for idle", ErrorType::RendererVulkanError)};
    }

    FLOG_INFO("new framebuffer size: {}/{}", _cachedFrameBufferWidth, _cachedFrameBufferHeight);
    auto swapRes = _swapchainManager.RecreateSwapchain(
        _ctx, _cachedFrameBufferWidth, _cachedFrameBufferHeight);
    if (!swapRes.has_value()) {
      FLOG_ERROR("failed to recreate swapchain");
      return FeErr{swapRes.error()};
    }

    auto cmdBufRes = _cmdBufferManager.CreateBuffers(_ctx);
    if (!cmdBufRes.has_value()) {
      FLOG_ERROR("failed to recreate command buffers");
      return FeErr{cmdBufRes.error()};
    }

    return FeFalse;
  }

  auto awaitRes = AwaitFence(&_ctx.inFlightFences[_ctx.currentFrame], UINT64_MAX);
  if (!awaitRes.has_value()) {
    FLOG_ERROR("failed to await in-flight fence");
    return FeErr{awaitRes.error()};
  }

  constexpr VkFence cFence = 0;
  auto acquireRes =
      _swapchainManager.AcquireNextImage(_ctx,
                                         _ctx.swapchain,
                                         UINT64_MAX,
                                         _ctx.imageAvailableSemaphores[_ctx.currentFrame],
                                         cFence,
                                         &_ctx.imageIndex);
  if (!acquireRes.has_value()) {
    FLOG_ERROR("failed to acquire next image from swapchain");
    return FeErr{acquireRes.error()};
  }

  if (!acquireRes.value()) {
    return FeFalse;
  }

  CommandBuffer &cmdBuffer = _ctx.graphicsCommandBuffer[_ctx.imageIndex];
  _cmdBufferManager.ResetBuffer(_ctx, cmdBuffer);
  _cmdBufferManager.BeginBuffer(_ctx, cmdBuffer, FeFalse, FeFalse, FeFalse);

  _renderpassManager.BeginRenderpass(
      _ctx, &cmdBuffer, &_ctx.mainRenderpass, _ctx.swapchain.framebuffers[_ctx.imageIndex].handle);

  return FeTrue;
}

FeExpect<bool, Error> VulkanBackend::EndFrame(float32 deltaTime) {
  Device &device = _ctx.device;
  CommandBuffer &cmdBuffer = _ctx.graphicsCommandBuffer[_ctx.imageIndex];

  _renderpassManager.EndRenderpass(_ctx, &cmdBuffer, &_ctx.mainRenderpass);
  _cmdBufferManager.EndBuffer(_ctx, cmdBuffer);

  if (_ctx.imagesInFlight[_ctx.imageIndex] != VK_NULL_HANDLE) {
    auto awaitRes = AwaitFence(_ctx.imagesInFlight[_ctx.imageIndex], UINT64_MAX);
    if (!awaitRes.has_value()) {
      FLOG_ERROR("failed to await image in-flight fence");
      return FeErr{awaitRes.error()};
    }
  }

  _ctx.imagesInFlight[_ctx.imageIndex] = &_ctx.inFlightFences[_ctx.currentFrame];

  auto resetRes = ResetFence(&_ctx.inFlightFences[_ctx.currentFrame]);
  if (!resetRes.has_value()) {
    FLOG_ERROR("failed to reset in-flight fence");
    return FeErr{resetRes.error()};
  }

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer.handle;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &_ctx.imageAvailableSemaphores[_ctx.currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &_ctx.queueCompleteSemaphores[_ctx.imageIndex];

  constexpr uint32 cFlagCount = 1;
  constexpr uint32 cSubmitCount = 1;

  // Each semaphore waits on the corresponding pipeline stage to complete 1:1
  // ratio. VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent
  // color attachment writes from executing until the semaphore signals
  VkPipelineStageFlags waitStages[cFlagCount] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.pWaitDstStageMask = waitStages;

  VkResult result = vkQueueSubmit(device.graphicsQueue,
                                  cSubmitCount,
                                  &submitInfo,
                                  _ctx.inFlightFences[_ctx.currentFrame].handle);
  if (auto res = VkCheck(result); !res.has_value()) {
    FLOG_ERROR("failed to submit to graphics queue");
    return FeErr{res.error()};
  }

  cmdBuffer.state = CmdBufferState::Submitted;

  auto presentRes =
      _swapchainManager.PresentSwapchain(_ctx,
                                         _ctx.swapchain,
                                         device.graphicsQueue,
                                         device.presentQueue,
                                         _ctx.queueCompleteSemaphores[_ctx.imageIndex],
                                         _ctx.imageIndex);
  if (!presentRes.has_value()) {
    FLOG_ERROR("failed to present swapchain image");
    return FeErr{presentRes.error()};
  }

  return FeTrue;
}

FeExpect<void, Error> VulkanBackend::UpdateGlobalState(math::Mat4D projection,
                                                       math::Mat4D view,
                                                       math::Vec3D viewPosition,
                                                       int32 mode) {
  EnsureGPUMatrixLayout(projection, view);
  CommandBuffer &cmdBuffer = _ctx.graphicsCommandBuffer[_ctx.imageIndex];
  _vulkanShader.UseShader(_ctx, _ctx.objectShader);

  _ctx.objectShader.globalUBO.projection = projection;
  _ctx.objectShader.globalUBO.view = view;
  // TODO: other ubo properties

  auto updateRes = _vulkanShader.UpdateGlobalState(_ctx, _ctx.objectShader);
  if (!updateRes.has_value()) {
    FLOG_ERROR("failed to update global state");
    return FeErr{updateRes.error()};
  }

  return {};
}

FeExpect<void, Error> VulkanBackend::CreateTexture(const string &name,
                                                   bool autoRelease,
                                                   int32 width,
                                                   int32 height,
                                                   int32 channelCount,
                                                   const uint8 *pPixels,
                                                   bool hasTransparency,
                                                   resources::Texture *pTexture,
                                                   resources::TextureFilter filter) {
  if (pTexture == nullptr) {
    FLOG_ERROR("cannot create a texture on a nullptr");
    return FeErr{Error("failed to create texture in vulkan backend", ErrorType::NullptrException)};
  }

  pTexture->width = width;
  pTexture->height = height;
  pTexture->channelCount = channelCount;
  pTexture->generation = 0;
  pTexture->filter = filter;

  void *pData =
      _memoryManager.RawAlloc(sizeof(TextureData), alignof(TextureData), memory::Tag::Texture);
  pTexture->pInternalData = pData;
  TextureData *pTextureData = reinterpret_cast<TextureData *>(pData);

  VkDeviceSize imageSize = width * height * channelCount;

  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags memFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VulkanBuffer staging;
  auto createRes =
      _bufferManager.CreateVulkanBuffer(_ctx, imageSize, usage, memFlags, FeTrue, &staging);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create vulkan buffer for texture");
    return FeErr{Error("texture creation failed", ErrorType::RendererVulkanError)};
  }

  auto loadRes = _bufferManager.LoadData(_ctx, staging, 0, imageSize, 0, pPixels);
  if (!loadRes.has_value()) {
    FLOG_ERROR("failed to load data for texture");
    return FeErr{Error("load texture failed", ErrorType::RendererVulkanError)};
  }

  // NOTE: assume 8 bits per channel
  VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
  createRes = _imageManager.CreateImage(
      _ctx,
      pTextureData->image,
      VK_IMAGE_TYPE_2D,
      width,
      height,
      imageFormat,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      FeTrue,
      VK_IMAGE_ASPECT_COLOR_BIT,
      filter);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create image for texture");
    return FeErr{Error("texture failed to create image", ErrorType::RendererVulkanError)};
  }

  VkCommandPool pool = _ctx.device.graphicsCommandPool;
  VkQueue queue = _ctx.device.graphicsQueue;

  auto callback = [&](CommandBuffer buffer) {
    return TextureSubmitCallback(buffer, imageFormat, pTextureData, staging);
  };

  VkFence tempFence;
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  vkCreateFence(_ctx.device.logicalDevice, &fenceInfo, _ctx.pAllocator, &tempFence);

  auto submitRes = _cmdBufferManager.ImmediateSubmit(_ctx, pool, queue, tempFence, callback);
  vkDestroyFence(_ctx.device.logicalDevice, tempFence, _ctx.pAllocator);
  _bufferManager.DestroyVulkanBuffer(_ctx, &staging);
  if (!submitRes.has_value()) {
    FLOG_ERROR("failed to submit command buffer for texture creation");
    return FeErr{Error("texture creation failed during submit", ErrorType::RendererVulkanError)};
  }

  VkSamplerCreateInfo samplerInfo = SamplerInfoByFilter(filter);
  VkResult result = vkCreateSampler(
      _ctx.device.logicalDevice, &samplerInfo, _ctx.pAllocator, &pTextureData->sampler);
  if (!VkResultIsSuccess(result)) {
    FLOG_ERROR("failed to create sampler for texture");
    return FeErr{
        Error("texture creation failed during sampler creation", ErrorType::RendererVulkanError)};
  }

  pTexture->hasTransparency = hasTransparency;
  pTexture->generation++;
  return {};
}

FeExpect<void, Error> VulkanBackend::DestroyTexture(resources::Texture *pTexture) {
  if (pTexture == nullptr) {
    return {};
  }

  TextureData *pTextureData = reinterpret_cast<TextureData *>(pTexture->pInternalData);
  if (pTextureData == nullptr) {
    FLOG_WARN("attempted to destroy texture with nullptr internal data");
    return {};
  }

  auto destroyRes = _imageManager.DestroyImage(_ctx, pTextureData->image);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("failed to destroy image for texture");
    return FeErr{Error("failed to destroy texture image", ErrorType::RendererVulkanError)};
  }

  _memoryManager.FZeroMemory(&pTextureData->image, sizeof(Image));

  vkDestroySampler(_ctx.device.logicalDevice, pTextureData->sampler, _ctx.pAllocator);
  pTextureData->sampler = VK_NULL_HANDLE;

  _memoryManager.RawFree(pTexture->pInternalData, sizeof(TextureData), memory::Tag::Texture);
  _memoryManager.FZeroMemory(pTexture, sizeof(resources::Texture));
  return {};
}

FeExpect<void, Error> VulkanBackend::CreateGeometry(uint32 id,
                                                    uint32 vertexCount,
                                                    const math::Vertex3D *pVertices,
                                                    uint32 indexCount,
                                                    const uint32 *pIndices) {
  VkFence tempFence;
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  vkCreateFence(_ctx.device.logicalDevice, &fenceInfo, _ctx.pAllocator, &tempFence);

  uint64 vertexOffset = _ctx.geometryVertexOffset;
  uint64 indexOffset = _ctx.geometryIndexOffset;
  uint64 vertexSize = sizeof(math::Vertex3D) * vertexCount;
  uint64 indexSize = sizeof(uint32) * indexCount;

  auto uploadRes = UploadDataRange(_ctx.device.graphicsCommandPool,
                                   tempFence,
                                   _ctx.device.graphicsQueue,
                                   vertexOffset,
                                   vertexSize,
                                   _ctx.objectVertexBuffer,
                                   pVertices);
  if (!uploadRes.has_value()) {
    vkDestroyFence(_ctx.device.logicalDevice, tempFence, _ctx.pAllocator);
    FLOG_ERROR("failed to upload data range in test");
    return FeErr{uploadRes.error()};
  }

  uploadRes = UploadDataRange(_ctx.device.graphicsCommandPool,
                              tempFence,
                              _ctx.device.graphicsQueue,
                              indexOffset,
                              indexSize,
                              _ctx.objectIndexBuffer,
                              pIndices);

  vkDestroyFence(_ctx.device.logicalDevice, tempFence, _ctx.pAllocator);
  if (!uploadRes.has_value()) {
    FLOG_ERROR("failed to upload index data");
    return FeErr{uploadRes.error()};
  }

  GeometryData geom{};
  geom.vertexBufferOffset = vertexOffset;
  geom.indexBufferOffset = indexOffset;
  geom.indexCount = indexCount;
  auto insertRes = _geometries.Insert(id, geom);
  if (!insertRes.has_value()) {
    FLOG_ERROR("failed to insert geometry into hashmap");
    return FeErr{insertRes.error()};
  }

  _ctx.geometryIndexOffset += indexSize;
  _ctx.geometryVertexOffset += vertexSize;
  return {};
}

FeExpect<void, Error> VulkanBackend::DestroyGeometry(uint32 id) {
  return {};
}

void VulkanBackend::DrawGeometry(uint32 id,
                                 const PushConstantData &data,
                                 const resources::Material *pMaterial) {
  CommandBuffer &cmdBuffer = _ctx.graphicsCommandBuffer[_ctx.imageIndex];
  PushConstantData gpuData = data;
  gpuData.model = data.model.ToGPUMatrix();

  // Push constants
  _vulkanShader.UpdateObject(_ctx, _ctx.objectShader, gpuData);

  // Pipeline + set0 + viewport/scissor
  if (!_frameStatebound) {
    _vulkanShader.UseShader(_ctx, _ctx.objectShader);

    VkDescriptorSet set0 = _ctx.objectShader.globalDescriptorSets[_ctx.imageIndex];
    vkCmdBindDescriptorSets(cmdBuffer.handle,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _ctx.objectShader.pipeline.layout,
                            0,
                            1,
                            &set0,
                            0,
                            nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float32>(_ctx.framebufferHeight);
    viewport.width = static_cast<float32>(_ctx.framebufferWidth);
    viewport.height = -viewport.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {_ctx.framebufferWidth, _ctx.framebufferHeight};

    vkCmdSetViewport(cmdBuffer.handle, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer.handle, 0, 1, &scissor);

    _frameStatebound = FeTrue;
  }

  // Material descriptor set
  if (pMaterial != _cpLastBoundMaterial) {
    if (pMaterial == nullptr) {
      FLOG_ERROR("DrawGeometry: pMaterial is nullptr — set 1 will not be bound");
    } else if (pMaterial->pInternalData == nullptr) {
      FLOG_ERROR("DrawGeometry: pMaterial->pInternalData is nullptr — set 1 will not be bound");
    } else {
      const MaterialData *pMatData = FeCast<MaterialData>(pMaterial->pInternalData);
      vkCmdBindDescriptorSets(cmdBuffer.handle,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              _ctx.objectShader.pipeline.layout,
                              1,
                              1,
                              &pMatData->descriptorSet,
                              0,
                              nullptr);
      _cpLastBoundMaterial = pMaterial;
    }
  }

  const GeometryData *pGeom = _geometries.Retrieve(id);
  if (pGeom == nullptr) {
    FLOG_WARN("DrawGeometry: unknown geometry id {}", id);
    return;
  }

  // Vertex + index buffers: only on geometry change
  if (id != _lastBoundGeometry) {
    VkDeviceSize offset = pGeom->vertexBufferOffset;
    vkCmdBindVertexBuffers(cmdBuffer.handle, 0, 1, &_ctx.objectVertexBuffer.handle, &offset);
    vkCmdBindIndexBuffer(cmdBuffer.handle,
                         _ctx.objectIndexBuffer.handle,
                         pGeom->indexBufferOffset,
                         VK_INDEX_TYPE_UINT32);
    _lastBoundGeometry = id;
  }

  vkCmdDrawIndexed(cmdBuffer.handle, pGeom->indexCount, 1, 0, 0, 0);
}

FeExpect<void, Error> VulkanBackend::CreateMaterial(resources::Material *pMaterial,
                                                    const resources::Texture *pTexture) {
  if (pTexture == nullptr) {
    FLOG_WARN("cannot create material on nullptr texture");
    return {};
  }

  const TextureData *pData = FeCast<TextureData>(pTexture->pInternalData);
  MaterialData *pMatData = FeCast<MaterialData>(
      _memoryManager.RawAlloc(sizeof(MaterialData), alignof(MaterialData), memory::Tag::Renderer));
  pMaterial->pInternalData = pMatData;

  VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = _ctx.objectShader.textureDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &_ctx.objectShader.textureDescriptorSetLayout;
  if (auto res = VkCheck(vkAllocateDescriptorSets(
          _ctx.device.logicalDevice, &allocInfo, &pMatData->descriptorSet));
      !res.has_value()) {
    FLOG_ERROR("failed to allocate descriptor sets for material");
    return FeErr{res.error()};
  }

  VkDescriptorImageInfo imageInfo{};
  imageInfo.sampler = pData->sampler;
  imageInfo.imageView = pData->image.view;
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = pMatData->descriptorSet;
  write.dstBinding = 0;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.descriptorCount = 1;
  write.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(_ctx.device.logicalDevice, 1, &write, 0, nullptr);
  return {};
}

FeExpect<void, Error> VulkanBackend::DestroyMaterial(resources::Material *pMaterial) {
  if (pMaterial == nullptr || pMaterial->pInternalData == nullptr) {
    return {};
  }

  MaterialData *pMatData = FeCast<MaterialData>(pMaterial->pInternalData);
  if (auto res = VkCheck(vkFreeDescriptorSets(_ctx.device.logicalDevice,
                                              _ctx.objectShader.textureDescriptorPool,
                                              1,
                                              &pMatData->descriptorSet));
      !res.has_value()) {
    FLOG_ERROR("failed to free descriptor set for material");
    return FeErr{res.error()};
  }

  _memoryManager.RawFree(pMatData, sizeof(MaterialData), memory::Tag::Renderer);
  pMaterial->pInternalData = nullptr;
  return {};
}

void VulkanBackend::Flush() {
  vkDeviceWaitIdle(_ctx.device.logicalDevice);
}

void VulkanBackend::BeginImGuiFrame() {
  ImGui_ImplVulkan_NewFrame();
}

void VulkanBackend::DrawImGui() {
  CommandBuffer &cmd = _ctx.graphicsCommandBuffer[_ctx.imageIndex];
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd.handle);
}

// PRIVATE MEMBERS

FeExpect<void, Error> VulkanBackend::CreateFence(Fence *pFence, bool signaled) {
  if (pFence == nullptr) {
    FLOG_ERROR("cannot create fence on a nullptr fence");
    return FeErr{Error("attempt to create a fence on a nullptr", ErrorType::NullptrException)};
  }

  pFence->isSignaled = signaled;
  VkFenceCreateInfo createInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  if (pFence->isSignaled) {
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  if (auto res = VkCheck(
          vkCreateFence(_ctx.device.logicalDevice, &createInfo, _ctx.pAllocator, &pFence->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create vulkan fence");
    return FeErr{res.error()};
  }
  return {};
}

FeExpect<void, Error> VulkanBackend::DestroyFence(Fence *pFence) {
  if (pFence == nullptr) {
    FLOG_ERROR("cannot destroy fence on a nullptr fence");
    return FeErr{Error("attempt to destroy a fence on a nullptr", ErrorType::NullptrException)};
  }

  if (pFence->handle != VK_NULL_HANDLE) {
    vkDestroyFence(_ctx.device.logicalDevice, pFence->handle, _ctx.pAllocator);
    pFence->handle = VK_NULL_HANDLE;
  }

  pFence->isSignaled = false;
  return {};
}

FeExpect<bool, Error> VulkanBackend::AwaitFence(Fence *pFence, uint64 timeoutNs) {
  if (pFence == nullptr) {
    FLOG_ERROR("cannot await fence on a nullptr fence");
    return FeErr{Error("attempt to await a fence on a nullptr", ErrorType::NullptrException)};
  }

  constexpr uint32 cFenceCount = 1;
  constexpr bool cAwaitAll = FeTrue;
  VkResult result = vkWaitForFences(
      _ctx.device.logicalDevice, cFenceCount, &pFence->handle, cAwaitAll, timeoutNs);

  switch (result) {
    case VK_SUCCESS:
      pFence->isSignaled = FeTrue;
      return FeTrue;
    case VK_TIMEOUT:
      FLOG_WARN("fence await timed out");
      break;
    case VK_ERROR_DEVICE_LOST:
      FLOG_ERROR("device lost while awaiting fence");
      return FeErr{Error("device lost while awaiting fence", ErrorType::RendererVulkanError)};
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      FLOG_ERROR("out of host memory while awaiting fence");
      return FeErr{
          Error("out of host memory while awaiting fence", ErrorType::RendererVulkanError)};
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      FLOG_ERROR("out of device memory while awaiting fence");
      return FeErr{
          Error("out of device memory while awaiting fence", ErrorType::RendererVulkanError)};
    default:
      FLOG_ERROR("unmapped error while awaiting fence: {}", static_cast<uint32>(result));
      return FeErr{Error("unmapped error while awaiting fence")};
  }

  return FeFalse;
}

FeExpect<void, Error> VulkanBackend::ResetFence(Fence *pFence) {
  if (pFence == nullptr) {
    FLOG_ERROR("cannot reset fence on a nullptr fence");
    return FeErr{Error("attempt to reset a fence on a nullptr", ErrorType::NullptrException)};
  }

  constexpr uint32 cFenceCount = 1;
  if (auto res = VkCheck(vkResetFences(_ctx.device.logicalDevice, cFenceCount, &pFence->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to reset vulkan fence");
    return FeErr{res.error()};
  }

  pFence->isSignaled = FeFalse;
  return {};
}

FeExpect<void, Error> VulkanBackend::CreateBuffers() {
  VkMemoryPropertyFlagBits memoryPropertyFlag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  const uint64 cVertexBufferSize = sizeof(math::Vertex3D) * 1024 * 1024;
  auto createRes = _bufferManager.CreateVulkanBuffer(
      _ctx, cVertexBufferSize, usageFlags, memoryPropertyFlag, FeTrue, &_ctx.objectVertexBuffer);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create vulkan vertex buffer");
    return FeErr{createRes.error()};
  }

  _ctx.geometryVertexOffset = 0;

  usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  const uint64 cIndexBufferSize = sizeof(uint32) * 1024 * 1024;
  createRes = _bufferManager.CreateVulkanBuffer(
      _ctx, cIndexBufferSize, usageFlags, memoryPropertyFlag, FeTrue, &_ctx.objectIndexBuffer);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create vulkan index buffer");
    return FeErr{createRes.error()};
  }

  _ctx.geometryIndexOffset = 0;
  return {};
}

FeExpect<void, Error> VulkanBackend::UploadDataRange(VkCommandPool pool,
                                                     VkFence fence,
                                                     VkQueue queue,
                                                     uint64 offset,
                                                     uint64 size,
                                                     VulkanBuffer &buffer,
                                                     const void *pData) {
  VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VkMemoryPropertyFlags memoryFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  VulkanBuffer staging;

  auto createRes =
      _bufferManager.CreateVulkanBuffer(_ctx, size, usageFlags, memoryFlags, FeTrue, &staging);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create vulkan buffer");
    return FeErr{createRes.error()};
  }

  auto loadRes = _bufferManager.LoadData(_ctx, staging, 0, size, 0, pData);
  if (!loadRes.has_value()) {
    FLOG_ERROR("failed to load data from vulkan buffer");
    return FeErr{loadRes.error()};
  }

  auto copyRes = _bufferManager.CopyBufferTo(
      _ctx, pool, fence, queue, staging.handle, 0, buffer.handle, offset, size);
  if (!copyRes.has_value()) {
    FLOG_ERROR("failed to copy buffer on upload");
    return FeErr{copyRes.error()};
  }

  _bufferManager.DestroyVulkanBuffer(_ctx, &staging);
  return {};
}

void VulkanBackend::TextureSubmitCallback(CommandBuffer cmd,
                                          VkFormat imageFormat,
                                          TextureData *pTextureData,
                                          const VulkanBuffer &vkBuffer) {
  auto transitionRes = _imageManager.TransitionImageLayout(_ctx,
                                                           cmd,
                                                           pTextureData->image,
                                                           imageFormat,
                                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  if (!transitionRes.has_value()) {
    FLOG_ERROR("failed to transition image layout for texture upload");
    return;
  }

  const uint32 width = pTextureData->image.width;
  const uint32 height = pTextureData->image.height;

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  _imageManager.CopyFromBuffer(_ctx, pTextureData->image, cmd, vkBuffer.handle);

  transitionRes = _imageManager.TransitionImageLayout(_ctx,
                                                      cmd,
                                                      pTextureData->image,
                                                      imageFormat,
                                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  if (!transitionRes.has_value()) {
    FLOG_ERROR("failed to transition image layout for texture upload");
    return;
  }
}

void VulkanBackend::InitImGui() {
  VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100};
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 100;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  vkCreateDescriptorPool(_ctx.device.logicalDevice, &poolInfo, nullptr, &_imguiDescriptorPool);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = _ctx.instance;
  initInfo.PhysicalDevice = _ctx.device.physicalDevice;
  initInfo.Device = _ctx.device.logicalDevice;
  initInfo.QueueFamily = static_cast<uint32>(_ctx.device.graphicsQueueIndex);
  initInfo.Queue = _ctx.device.graphicsQueue;
  initInfo.DescriptorPool = _imguiDescriptorPool;
  initInfo.RenderPass = _ctx.mainRenderpass.handle;
  initInfo.Subpass = 0;
  initInfo.MinImageCount = _ctx.swapchain.maxFrames;
  initInfo.ImageCount = _ctx.swapchain.imageCount;
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.CheckVkResultFn = [](VkResult err) {
    if (err != VK_SUCCESS) {
      FLOG_ERROR("ImGui Vulkan Error: {}", static_cast<int32>(err));
    }
  };
  ImGui_ImplVulkan_Init(&initInfo);
  FLOG_INFO("ImGui initialized with Vulkan Implementation");
}

void VulkanBackend::ShutdownImGui() {
  if (_imguiDescriptorPool == VK_NULL_HANDLE) {
    return;
  }

  ImGui_ImplVulkan_Shutdown();
  ImGui::DestroyContext();
  vkDestroyDescriptorPool(_ctx.device.logicalDevice, _imguiDescriptorPool, nullptr);
  _imguiDescriptorPool = VK_NULL_HANDLE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              uint32 messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
              void *userData) {
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

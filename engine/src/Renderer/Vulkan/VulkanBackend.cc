#include "VulkanBackend.hpp"

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "Math/MathTypes.hpp"
#include "Platform/Platform.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

void EnsureGPUMatrixLayout(math::Mat4D &inProj, math::Mat4D &inView);

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              uint32 messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
              void *userData);

VulkanBackend::VulkanBackend(memory::MemoryManager &memManager, platform::FileSystem &fs)
    : _memoryManager(memManager), _deviceManager(memManager),
      _swapchainManager(memManager, _imageManager), _renderpassManager(memManager),
      _cmdBufferManager(memManager), _bufferManager(_cmdBufferManager),
      _vulkanShader(memManager, _bufferManager), _ctx(memManager, fs) {
}

VulkanBackend::~VulkanBackend() {
  vkDeviceWaitIdle(_ctx.device.logicalDevice);

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

FeExpect<bool, Error> VulkanBackend::Initialize(ApplicationState *appState) {
  // TODO: custom allocator
  _ctx.pAllocator = nullptr;
  _cachedFrameBufferWidth = appState->width;
  _cachedFrameBufferHeight = appState->height;
  _ctx.framebufferWidth = (_cachedFrameBufferWidth != 0) ? _cachedFrameBufferWidth : 946;
  _ctx.framebufferHeight = (_cachedFrameBufferHeight != 0) ? _cachedFrameBufferHeight : 507;

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
  if (auto res = platform::CreateVulkanSurface(appState->platformState, _ctx); !res.has_value()) {
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
                                                      1.0f,
                                                      1.0f,
                                                      1.0f,
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

  // TEMPORARY TEST CODEBack
  const uint32 cVertCount = 4;
  std::array<math::Vertex3D, cVertCount> vertices;
  _memoryManager.FZeroMemory(vertices.data(), sizeof(math::Vertex3D) * cVertCount);

  vertices[0].position = math::Vec3D(0.0f, -0.5f, 0.0f);
  vertices[1].position = math::Vec3D(0.5f, 0.5f, 0.0f);
  vertices[2].position = math::Vec3D(0.0f, 0.5f, 0.0f);
  vertices[3].position = math::Vec3D(0.5f, -0.5f, 0.0f);

  const uint32 cIndexCount = 6;
  std::array<uint32, cIndexCount> indicies = {0, 2, 1, 0, 3, 1};

  VkFence tempFence;
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  vkCreateFence(_ctx.device.logicalDevice, &fenceInfo, _ctx.pAllocator, &tempFence);

  auto uploadRes = UploadDataRange(_ctx.device.graphicsCommandPool,
                                   tempFence,
                                   _ctx.device.graphicsQueue,
                                   0,
                                   sizeof(math::Vertex3D) * cVertCount,
                                   _ctx.objectVertexBuffer,
                                   vertices.data());
  if (!uploadRes.has_value()) {
    FLOG_ERROR("failed to upload data range in test");
    return FeErr{uploadRes.error()};
  }

  uploadRes = UploadDataRange(_ctx.device.graphicsCommandPool,
                              tempFence,
                              _ctx.device.graphicsQueue,
                              0,
                              sizeof(uint32) * cIndexCount,
                              _ctx.objectIndexBuffer,
                              indicies.data());

  vkDestroyFence(_ctx.device.logicalDevice, tempFence, _ctx.pAllocator);
  if (!uploadRes.has_value()) {
    FLOG_ERROR("failed to upload index data");
    return FeErr{uploadRes.error()};
  }

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

  // Prepare viewport and scissor
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = static_cast<float32>(_ctx.framebufferHeight);
  viewport.width = static_cast<float32>(_ctx.framebufferWidth);
  viewport.height = -viewport.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = {_ctx.framebufferWidth, _ctx.framebufferHeight};

  constexpr uint32 cFirstViewport = 0;
  constexpr uint32 cViewportCount = 1;
  constexpr uint32 cFirstScissor = 0;
  constexpr uint32 cScissorCount = 1;

  _renderpassManager.BeginRenderpass(
      _ctx, &cmdBuffer, &_ctx.mainRenderpass, _ctx.swapchain.framebuffers[_ctx.imageIndex].handle);

  _vulkanShader.UseShader(_ctx, _ctx.objectShader);

  {
    VkDescriptorSet set0 = _ctx.objectShader.globalDescriptorSets[_ctx.imageIndex];
    vkCmdBindDescriptorSets(cmdBuffer.handle,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _ctx.objectShader.pipeline.layout,
                            0,
                            1,
                            &set0,
                            0,
                            nullptr);
  }

  // TODO: remove (for tests)
  std::array<VkDeviceSize, 1> offsets = {0};

  vkCmdSetViewport(cmdBuffer.handle, cFirstViewport, cViewportCount, &viewport);
  vkCmdSetScissor(cmdBuffer.handle, cFirstScissor, cScissorCount, &scissor);

  vkCmdBindVertexBuffers(cmdBuffer.handle,
                         0,
                         1,
                         &_ctx.objectVertexBuffer.handle,
                         static_cast<VkDeviceSize *>(offsets.data()));

  vkCmdBindIndexBuffer(cmdBuffer.handle, _ctx.objectIndexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(cmdBuffer.handle, 6, 1, 0, 0, 0);
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

FeExpect<bool, Error> VulkanBackend::DrawFrame(const RenderPacket &renderPacket) {
  if (_ctx.recreatingSwapchain) {
    return FeFalse;
  }

  (void)renderPacket;

  CommandBuffer &cmdBuffer = _ctx.graphicsCommandBuffer[_ctx.imageIndex];

  _vulkanShader.UseShader(_ctx, _ctx.objectShader);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmdBuffer.handle, 0, 1, &_ctx.objectVertexBuffer.handle, &offset);

  vkCmdBindIndexBuffer(cmdBuffer.handle, _ctx.objectIndexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);

  // Draw quad (6 indices)
  vkCmdDrawIndexed(cmdBuffer.handle, 6, 1, 0, 0, 0);
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
                                                     void *pData) {
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

void VulkanBackend::UpdateObject(math::Mat4D model) {
  _vulkanShader.UpdateObject(_ctx, _ctx.objectShader, model);
}

void EnsureGPUMatrixLayout(math::Mat4D &inProj, math::Mat4D &inView) {
  inProj = inProj.ToGPUMatrix();
  inView = inView.ToGPUMatrix();
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

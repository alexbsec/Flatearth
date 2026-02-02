#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/Shaders/ObjectShader.hpp"
#include "Renderer/Vulkan/VulkanDeviceManager.hpp"
#include "Renderer/Vulkan/VulkanImager.hpp"
#include "Renderer/Vulkan/VulkanRenderpassManager.hpp"
#include "Renderer/Vulkan/VulkanSwapchainManager.hpp"
#include "Renderer/Vulkan/VulkanCommandBufferManager.hpp"

namespace flatearth::renderer::vulkan {

class VulkanBackend : public IRendererBackend {
public:
  explicit VulkanBackend(memory::MemoryManager &memManager, platform::FileSystem &fs);
  ~VulkanBackend();

  FeExpect<bool, Error> Initialize(ApplicationState *appState) override;
  FeExpect<bool, Error> OnResize(uint32 width, uint32 height) override;
  FeExpect<bool, Error> BeginFrame(float32 deltaTime) override;
  FeExpect<bool, Error> EndFrame(float32 deltaTime) override;
  FeExpect<bool, Error> DrawFrame(const RenderPacket &renderPacket) override;

private:
  FeExpect<void, Error> CreateFence(Fence *pFence, bool signaled);
  FeExpect<void, Error> DestroyFence(Fence *pFence);
  FeExpect<bool, Error> AwaitFence(Fence *pFence, uint64 timeoutNs);
  FeExpect<void, Error> ResetFence(Fence *pFence);
  FeExpect<void, Error> CreateBuffers();
  FeExpect<void, Error> UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, uint64 offset, uint64 size, VulkanBuffer &buffer, void *pData);


private:
  memory::MemoryManager &_memoryManager;
  DeviceManager _deviceManager;
  SwapchainManager _swapchainManager;
  ImageManager _imageManager;
  RenderpassManager _renderpassManager;
  CommandBufferManager _cmdBufferManager;
  BufferManager _bufferManager;
  shaders::VulkanShader _vulkanShader;
    // No-op (not an error)
  Context _ctx;

  uint32 _cachedFrameBufferWidth{0}, _cachedFrameBufferHeight{0};
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP

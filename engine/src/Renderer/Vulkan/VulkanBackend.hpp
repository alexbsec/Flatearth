#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP

#include "Core/ApplicationConfig.hpp"
#include "Core/FeMemory.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanDeviceManager.hpp"
#include "Renderer/Vulkan/VulkanImager.hpp"
#include "Renderer/Vulkan/VulkanRenderpassManager.hpp"
#include "Renderer/Vulkan/VulkanSwapchainManager.hpp"
#include "Renderer/Vulkan/VulkanCommandBufferManager.hpp"

namespace flatearth::renderer::vulkan {

class VulkanBackend : public IRendererBackend {
public:
  explicit VulkanBackend(memory::MemoryManager &memManager);
  ~VulkanBackend();

  FeExpect<bool, Error> Initialize(ApplicationState *appState) override;
  FeExpect<bool, Error> OnResize(uint32 width, uint32 height) override;
  FeExpect<bool, Error> BeginFrame(float32 deltaTime) override;
  FeExpect<bool, Error> EndFrame(float32 deltaTime) override;

private:
  memory::MemoryManager &_memoryManager;
  DeviceManager _deviceManager;
  SwapchainManager _swapchainManager;
  ImageManager _imageManager;
  RenderpassManager _renderpassManager;
  CommandBufferManager _cmdBufferManager;
  Context _ctx;

  uint32 _cachedFrameBufferWidth{0}, _cachedFrameBufferHeight{0};
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP

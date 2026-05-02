#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererInterface.hpp"
#include "Renderer/ShaderRegistry.hpp"
#include "Renderer/Vulkan/Shaders/VulkanShader.hpp"
#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/VulkanCommandBufferManager.hpp"
#include "Renderer/Vulkan/VulkanDeviceManager.hpp"
#include "Renderer/Vulkan/VulkanImager.hpp"
#include "Renderer/Vulkan/VulkanRenderpassManager.hpp"
#include "Renderer/Vulkan/VulkanSwapchainManager.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::renderer::vulkan {

class VulkanBackend : public IRendererBackend {
public:
  explicit VulkanBackend(memory::MemoryManager &memManager, platform::FileSystem &fs);
  ~VulkanBackend();

  FeExpect<bool, Error> Initialize(EngineState *pEngState) override;
  FeExpect<bool, Error> OnResize(uint32 width, uint32 height) override;
  FeExpect<bool, Error> BeginFrame(float32 deltaTime) override;
  FeExpect<bool, Error> EndFrame(float32 deltaTime) override;
  FeExpect<void, Error> UpdateGlobalState(math::Mat4D projection,
                                          math::Mat4D view,
                                          math::Vec3D viewPosition,
                                          int32 mode,
                                          stringv shaderName = "Builtin.ObjectShader") override;

  FeExpect<void, Error> CreateTexture(const string &name,
                                      bool autoRelease,
                                      int32 width,
                                      int32 height,
                                      int32 channelCount,
                                      const uint8 *pPixels,
                                      bool hasTransparency,
                                      resources::Texture *pTexture,
                                      resources::TextureFilter filter) override;
  FeExpect<void, Error> DestroyTexture(resources::Texture *pTexture) override;
  FeExpect<void, Error> CreateGeometry(uint32 id,
                                       uint32 vertexCount,
                                       const math::Vertex3D *pVertices,
                                       uint32 indexCount,
                                       const uint32 *pIndices) override;
  FeExpect<void, Error> DestroyGeometry(uint32 id) override;
  void DrawGeometry(uint32 id,
                    stringv shadeName,
                    const void *pPushData,
                    uint32 pushSize,
                    const resources::Material *pMaterial) override;

  FeExpect<void, Error> CreateMaterial(resources::Material *pMaterial,
                                       const resources::Texture *pTexture) override;
  FeExpect<void, Error> DestroyMaterial(resources::Material *pMaterial) override;

  void Flush() override;
  void BeginImGuiFrame() override;
  void DrawImGui() override;

private:
  FeExpect<void, Error> CreateFence(Fence *pFence, bool signaled);
  FeExpect<void, Error> DestroyFence(Fence *pFence);
  FeExpect<bool, Error> AwaitFence(Fence *pFence, uint64 timeoutNs);
  FeExpect<void, Error> ResetFence(Fence *pFence);
  FeExpect<void, Error> CreateBuffers();
  FeExpect<void, Error> UploadDataRange(VkCommandPool pool,
                                        VkFence fence,
                                        VkQueue queue,
                                        uint64 offset,
                                        uint64 size,
                                        VulkanBuffer &buffer,
                                        const void *pData);

  void InitImGui();
  void ShutdownImGui();

  void TextureSubmitCallback(CommandBuffer cmd,
                             VkFormat imageFormat,
                             TextureData *pTextureData,
                             const VulkanBuffer &vkBuffer);

private:
  memory::MemoryManager &_memoryManager;
  DeviceManager _deviceManager;
  SwapchainManager _swapchainManager;
  ImageManager _imageManager;
  RenderpassManager _renderpassManager;
  CommandBufferManager _cmdBufferManager;
  BufferManager _bufferManager;
  shaders::VulkanShader _vulkanShader;
  ShaderRegistry _shaderRegistry;
  Context _ctx;

  containers::HashMap<uint32, GeometryData> _geometries;

  VkDescriptorPool _imguiDescriptorPool{VK_NULL_HANDLE};

  const resources::Material *_cpLastBoundMaterial{nullptr};
  const ObjectShader *_cpLastBoundShader{nullptr};
  uint32 _lastBoundGeometry{UINT32_MAX};
  uint32 _cachedFrameBufferWidth{0}, _cachedFrameBufferHeight{0};
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_BACKEND_HPP

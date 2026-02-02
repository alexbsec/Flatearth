#include "ObjectShader.hpp"
#include "Math/Vector3D.hpp"
#include "Renderer/RendererTypes.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include "Renderer/Vulkan/VulkanUtils.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan::shaders {

VulkanShader::VulkanShader(memory::MemoryManager &memManager,
                           BufferManager &bufferManager)
    : _memoryManager(memManager), _bufferManager(bufferManager) {}

VulkanShader::~VulkanShader() {
  FLOG_INFO("vulkan shader successfully destroyed");
}

FeExpect<bool, Error>
VulkanShader::CreateObjectShader(Context &ctx, ObjectShader *pObjShader) {
  if (pObjShader == nullptr) {
    FLOG_ERROR("cannot create shader object on nullptr object shader");
    return FeErr{
        Error("object shader is nullptr", ErrorType::NullptrException)};
  }

  std::array<string, cObjectShaderStageCount> cStageTypeStrs{
      "vert",
      "frag",
  };

  constexpr std::array<VkShaderStageFlagBits, cObjectShaderStageCount>
      cStageTypes{
          VK_SHADER_STAGE_VERTEX_BIT,
          VK_SHADER_STAGE_FRAGMENT_BIT,
      };

  const string cBuiltinShaderName = "Builtin.ObjectShader";
  for (uint32 i = 0; i < cObjectShaderStageCount; i++) {
    auto createRes =
        CreateShaderModule(ctx, cBuiltinShaderName, cStageTypeStrs[i],
                           cStageTypes[i], i, pObjShader->shaderStages.data());
    if (!createRes.has_value()) {
      FLOG_ERROR("failed to create shader module at index {}", i);
      return FeErr{createRes.error()};
    }
    FLOG_INFO("{} object shader created", cStageTypeStrs[i]);
  }

  // -----------------------------
  // Global descriptor set layout
  // -----------------------------
  VkDescriptorSetLayoutBinding globalUBOLayoutBinding{};
  globalUBOLayoutBinding.binding = 0;
  globalUBOLayoutBinding.descriptorCount = 1;
  globalUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  globalUBOLayoutBinding.pImmutableSamplers = nullptr;
  globalUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutCreateInfo.bindingCount = 1;
  layoutCreateInfo.pBindings = &globalUBOLayoutBinding;

  if (auto res = VkCheck(vkCreateDescriptorSetLayout(
          ctx.device.logicalDevice, &layoutCreateInfo, ctx.pAllocator,
          &pObjShader->globalDescriptorSetLayout));
      !res.has_value()) {
    FLOG_ERROR("failed to create descriptor set layout");
    return FeErr{res.error()};
  }

  // -----------------------------
  // Descriptor pool
  // -----------------------------
  VkDescriptorPoolSize globalPoolSize{};
  globalPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  globalPoolSize.descriptorCount = ctx.swapchain.imageCount; // 1 UBO per set

  VkDescriptorPoolCreateInfo poolInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &globalPoolSize;
  poolInfo.maxSets = ctx.swapchain.imageCount;

  if (auto res = VkCheck(vkCreateDescriptorPool(
          ctx.device.logicalDevice, &poolInfo, ctx.pAllocator,
          &pObjShader->globalDescriptorPool));
      !res.has_value()) {
    FLOG_ERROR("failed to create descriptor pool");
    return FeErr{res.error()};
  }

  // -----------------------------
  // Pipeline creation
  // -----------------------------
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float32>(ctx.framebufferWidth);
  viewport.height = static_cast<float32>(ctx.framebufferHeight);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = ctx.framebufferWidth;
  scissor.extent.height = ctx.framebufferHeight;

  uint32 offset = 0;
  const int32 attributeCount = 1;
  std::array<VkVertexInputAttributeDescription, attributeCount>
      attrDescription{};
  std::array<VkFormat, attributeCount> formats = {VK_FORMAT_R32G32B32_SFLOAT};
  std::array<uint32, attributeCount> sizes = {sizeof(math::Vec3D)};

  for (uint32 i = 0; i < static_cast<uint32>(attributeCount); i++) {
    attrDescription[i].binding = 0;
    attrDescription[i].location = i;
    attrDescription[i].format = formats[i];
    attrDescription[i].offset = offset;
    offset += sizes[i];
  }

  // Descriptor set layouts for pipeline layout
  const uint64 cLayoutCount = 1;
  std::array<VkDescriptorSetLayout, cLayoutCount> layouts = {
      pObjShader->globalDescriptorSetLayout,
  };

  // Shader stages
  std::array<VkPipelineShaderStageCreateInfo, cObjectShaderStageCount>
      stageInfos{};
  _memoryManager.FZeroMemory(stageInfos.data(), sizeof(stageInfos));
  for (uint32 i = 0; i < cObjectShaderStageCount; i++) {
    stageInfos[i] = pObjShader->shaderStages[i].shaderStageCreateInfo;
  }

  auto pipelineRes = _pipelineManager.CreateGraphicsPipeline(
      ctx, &ctx.mainRenderpass, attributeCount, attrDescription.data(),
      cLayoutCount, layouts.data(), cObjectShaderStageCount, stageInfos.data(),
      viewport, scissor, FeFalse, &pObjShader->pipeline);
  if (!pipelineRes.has_value()) {
    FLOG_ERROR("failed to create graphics pipeline");
    return FeErr{pipelineRes.error()};
  }

  // -----------------------------
  // Global uniform buffer (host visible)
  // -----------------------------
  VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  uint32 memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  auto createBufferRes = _bufferManager.CreateVulkanBuffer(
      ctx, sizeof(GlobalUniformObject), usage, memFlags, FeTrue,
      &pObjShader->globalUniformBuffer);
  if (!createBufferRes.has_value()) {
    FLOG_ERROR("failed to create vulkan buffer for GlobalUniformObject");
    return FeErr{createBufferRes.error()};
  }

  // -----------------------------
  // Allocate global descriptor sets (ONE PER SWAPCHAIN IMAGE)
  // -----------------------------
  const uint32 count = ctx.swapchain.imageCount;

  // Resize DArray<VkDescriptorSet>
  pObjShader->globalDescriptorSets.Resize(count);

  // Build a layouts array (count copies)
  containers::DArray<VkDescriptorSetLayout> globalLayouts(_memoryManager);
  globalLayouts.Resize(count);
  for (uint32 i = 0; i < count; ++i) {
    globalLayouts[i] = pObjShader->globalDescriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = pObjShader->globalDescriptorPool;
  allocInfo.descriptorSetCount = count;
  allocInfo.pSetLayouts = globalLayouts.Data();

  if (auto res = VkCheck(
          vkAllocateDescriptorSets(ctx.device.logicalDevice, &allocInfo,
                                   pObjShader->globalDescriptorSets.Data()));
      !res.has_value()) {
    FLOG_ERROR("failed to allocate descriptor sets");
    return FeErr{res.error()};
  }

  containers::DArray<VkDescriptorBufferInfo> infos(_memoryManager);
  containers::DArray<VkWriteDescriptorSet> writes(_memoryManager);
  infos.Resize(count);
  writes.Resize(count);

  for (uint32 i = 0; i < count; ++i) {
    infos[i] = {};
    infos[i].buffer = pObjShader->globalUniformBuffer.handle;
    infos[i].offset = 0;
    infos[i].range = sizeof(GlobalUniformObject);

    writes[i] = {};
    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = pObjShader->globalDescriptorSets[i];
    writes[i].dstBinding = 0;
    writes[i].dstArrayElement = 0;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[i].descriptorCount = 1;
    writes[i].pBufferInfo = &infos[i];
  }

  vkUpdateDescriptorSets(ctx.device.logicalDevice, count, writes.Data(), 0,
                         nullptr);

  return FeTrue;
}

void VulkanShader::DestroyObjectShader(Context &ctx, ObjectShader *pObjShader) {
  if (!pObjShader) return;

  VkDevice dev = ctx.device.logicalDevice;

  vkDeviceWaitIdle(dev);

  if (pObjShader->globalDescriptorPool) {
    vkDestroyDescriptorPool(dev, pObjShader->globalDescriptorPool, ctx.pAllocator);
    pObjShader->globalDescriptorPool = VK_NULL_HANDLE;
  }

  if (pObjShader->globalDescriptorSetLayout) {
    vkDestroyDescriptorSetLayout(dev, pObjShader->globalDescriptorSetLayout, ctx.pAllocator);
    pObjShader->globalDescriptorSetLayout = VK_NULL_HANDLE;
  }

  _bufferManager.DestroyVulkanBuffer(ctx, &pObjShader->globalUniformBuffer);

  _pipelineManager.DestroyGraphicsPipeline(ctx, &pObjShader->pipeline);

  for (uint32 i = 0; i < cObjectShaderStageCount; i++) {
    if (pObjShader->shaderStages[i].handle) {
      vkDestroyShaderModule(dev, pObjShader->shaderStages[i].handle, ctx.pAllocator);
      pObjShader->shaderStages[i].handle = VK_NULL_HANDLE;
    }
  }
}

void VulkanShader::UseShader(Context &ctx, ObjectShader &objShader) {
  uint32 imageIndex = ctx.imageIndex;
  VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  _pipelineManager.BindPipeline(ctx.graphicsCommandBuffer[imageIndex],
                                bindPoint, objShader.pipeline);
}


FeExpect<void, Error>
VulkanShader::UpdateGlobalState(Context &ctx, ObjectShader &objShader) {
  constexpr uint32 cRange  = sizeof(GlobalUniformObject);
  constexpr uint64 cOffset = 0;

  auto loadRes = _bufferManager.LoadData(ctx,
                                        objShader.globalUniformBuffer,
                                        cOffset,
                                        cRange,
                                        0,
                                        &objShader.globalUBO);
  if (!loadRes.has_value()) {
    FLOG_ERROR("failed to load data while updating global state");
    return FeErr{loadRes.error()};
  }

  return {};
}


} // namespace flatearth::renderer::vulkan::shaders

#include "VulkanRenderpassManager.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

RenderpassManager::RenderpassManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

RenderpassManager::~RenderpassManager() {
  FLOG_INFO("renderpass exited gracefully");
}

FeExpect<void, Error> RenderpassManager::CreateRenderpass(
    Context &ctx, Renderpass *pRenderpass, float32 x, float32 y, float32 width,
    float32 height, float32 r, float32 g, float32 b, float32 a, float32 depth,
    uint32 stencil) {

  pRenderpass->x = x;
  pRenderpass->y = y;
  pRenderpass->width = width;
  pRenderpass->height = height;
  pRenderpass->r = r;
  pRenderpass->g = g;
  pRenderpass->b = b;
  pRenderpass->a = a;
  pRenderpass->depth = depth;
  pRenderpass->stencil = stencil;

  // Main subpass struct
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

  /***** TODO: MAKE ALL OF THIS CONFIGURABLE *****/
  uint32 attachmentDescriptionCount = 2;
  containers::DArray<VkAttachmentDescription> attachmentDescriptions(_memoryManager);
  attachmentDescriptions.Reserve(attachmentDescriptionCount);
  for (uint32 i = 0; i < attachmentDescriptionCount; i++) {
    attachmentDescriptions.Push(VkAttachmentDescription{});
  }

  // Color attachment
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = ctx.swapchain.imageFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachmentDescriptions[0] = colorAttachment;

  VkAttachmentReference colorAttachmentRef;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachmentRef.attachment = 0;

  // Subpass populate
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  // Depth attachment
  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = ctx.device.depthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  attachmentDescriptions[1] = depthAttachment;

  VkAttachmentReference depthAttachmentRef;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttachmentRef.attachment = 1;

  // TODO: other attachments

  // Subpass populate
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  // Other attachments (explicitly null and 0 because not impl yet)
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.pResolveAttachments = nullptr;
  subpass.pPreserveAttachments = nullptr;
  subpass.preserveAttachmentCount = 0;

  // Render pass dependencies
  VkSubpassDependency dependency;
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependency.dependencyFlags = 0;

  // Render pass creation
  VkRenderPassCreateInfo renderPassCreateInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassCreateInfo.attachmentCount = attachmentDescriptionCount;
  renderPassCreateInfo.pAttachments = attachmentDescriptions.Data();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies = &dependency;
  renderPassCreateInfo.pNext = nullptr;
  renderPassCreateInfo.flags = 0;
  /***********************************************/

  if (auto res = VkCheck(
          vkCreateRenderPass(ctx.device.logicalDevice, &renderPassCreateInfo,
                             ctx.pAllocator, &pRenderpass->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create renderpass");
    return FeErr{res.error()};
  }

  return {};
}

FeExpect<void, Error>
RenderpassManager::DestroyRenderpass(Context &ctx, Renderpass *pRenderpass) {

  if (pRenderpass != nullptr && pRenderpass->handle != nullptr) {
    vkDestroyRenderPass(ctx.device.logicalDevice, pRenderpass->handle,
                        ctx.pAllocator);
    pRenderpass->handle = nullptr;
  }

  return {};
}

void RenderpassManager::BeginRenderpass(Context &ctx, CommandBuffer *pCmdBuffer,
                                        Renderpass *pRenderpass,
                                        VkFramebuffer frameBuffer) {

  VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = pRenderpass->handle;
  beginInfo.framebuffer = frameBuffer;
  beginInfo.renderArea.offset.x = pRenderpass->x;
  beginInfo.renderArea.offset.y = pRenderpass->y;
  beginInfo.renderArea.extent.width = pRenderpass->width;
  beginInfo.renderArea.extent.height = pRenderpass->height;

  /** TODO: Make these configurable **/
  const uint32 clearValCount = 2;
  VkClearValue clearVals[clearValCount] = {};
  clearVals[0].color.float32[0] = pRenderpass->r;
  clearVals[0].color.float32[1] = pRenderpass->g;
  clearVals[0].color.float32[2] = pRenderpass->b;
  clearVals[0].color.float32[3] = pRenderpass->a;
  clearVals[1].depthStencil.depth = pRenderpass->depth;
  clearVals[1].depthStencil.stencil = pRenderpass->stencil;

  beginInfo.clearValueCount = clearValCount;
  beginInfo.pClearValues = clearVals;

  vkCmdBeginRenderPass(pCmdBuffer->handle, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  pCmdBuffer->state = CmdBufferState::InRenderpass;
}

void RenderpassManager::EndRenderpass(Context &ctx, CommandBuffer *pCmdBuffer,
                                      Renderpass *pRenderpass) {
  vkCmdEndRenderPass(pCmdBuffer->handle);
  pCmdBuffer->state = CmdBufferState::Recording;
}

} // namespace flatearth::renderer::vulkan

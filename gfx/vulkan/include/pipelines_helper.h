#ifndef  BKP_PIPELINES_HELPER_H_
#define  BKP_PIPELINES_HELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vk_pipeline.h"

/*********************************************************************
 * Defines
*********************************************************************/

/*********************************************************************
 * Type def & enum
*********************************************************************/

/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

BKP_EXPORTED VkPipelineInputAssemblyStateCreateInfo bkpDefaultPipelineInputAssembly(VkPrimitiveTopology topology);
BKP_EXPORTED VkViewport bkpDefaultViewport(float x, float y, float w, float h, float minDepth, float maxDepth);
BKP_EXPORTED VkRect2D bkpDefaultRect2D(VkOffset2D offset, VkExtent2D dim);
BKP_EXPORTED VkPipelineViewportStateCreateInfo bkpDefaultPipelineViewport(const VkViewport * viewports , uint32_t vCount, const VkRect2D * scissors, uint32_t sCount);
BKP_EXPORTED VkPipelineRasterizationStateCreateInfo bkpDefaultPipelineRasterization(VkPolygonMode poly, VkCullModeFlags cull, VkFrontFace face, VkPipelineRasterizationStateCreateFlags flags);
BKP_EXPORTED VkPipelineMultisampleStateCreateInfo bkpDefaultPipelineMultisample(VkBool32 sampleShadingEnabled, VkSampleCountFlagBits samples, float minSampleShading);
BKP_EXPORTED VkPipelineColorBlendAttachmentState bkpDefaultPipelineColorBlendAttachement();
BKP_EXPORTED VkPipelineColorBlendStateCreateInfo bkpDefaultPipelineColorBlend(const VkPipelineColorBlendAttachmentState * attachments, uint32_t count);
BKP_EXPORTED VkPipelineDepthStencilStateCreateInfo bkpDefaultPipelineDepthStenscil(VkBool32 testEnabled, VkBool32 writeEnabled, VkCompareOp op);
BKP_EXPORTED VkPipelineDynamicStateCreateInfo bkpDefaultPipelineDynamicState(const VkDynamicState * dstates, uint32_t count, VkPipelineDynamicStateCreateFlags flags);
BKP_EXPORTED VkPushConstantRange bkpDefaultPipelinePushConstant(VkShaderStageFlags stage, uint32_t offset, uint32_t size);
BKP_EXPORTED VkPipelineLayoutCreateInfo bkpDefaultPipelineLayout(VkDescriptorSetLayout * layouts, uint32_t count);
BKP_EXPORTED VkDescriptorPoolCreateInfo bkpDefaultDescriptorPool(VkDescriptorPoolSize * poolSizes, uint32_t poolSizeCount, uint32_t maxSet);
/**
 * @brief Zero-initialise `pplInfo` and fill it with sensible defaults.
 *
 * **Zeroes `*pplInfo` entirely first** — any values you set before calling
 * this function will be lost.  Always set `pplInfo->shaderProgram` and
 * `pplInfo->colorAttachmentFormat` **after** this call.
 *
 * Defaults applied:
 * - Topology: `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`
 * - Culling: back-face, counter-clockwise front face
 * - Depth test + write enabled, compare op `LESS_OR_EQUAL`
 * - Single color attachment with no blending
 * - Dynamic state **disabled** (viewport and scissor are static)
 * - No tessellation, no multisampling (1 sample)
 *
 * **Note**: `dimension` is currently unused — viewport and scissor are
 * not set by this function.  Use @ref bkpUpdateWindowViewportScissor
 * (or @ref bkpCmdSetViewport / @ref bkpCmdSetScissor) at draw time.
 *
 * @param pplInfo   Output struct to initialise.
 * @param dimension Reserved for future use; pass the window extent for forward compatibility.
 */
BKP_EXPORTED void bkpCreatePipelineMinimalInfo(BkpPipelineGraphicInfo * pplInfo, VkExtent2D dimension);
BKP_EXPORTED VkPipelineDynamicStateCreateInfo bkpDefaultDynamicStates(VkDynamicState * st, size_t count);

#ifdef __cplusplus
}
#endif

#endif


#ifndef BKP_VK_CMD_H
#define BKP_VK_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vk_types.h"

/*********************************************************************
 * Functions
 *********************************************************************/

/* dynamic rendering */
BKP_EXPORTED void bkpCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo * info);
BKP_EXPORTED void bkpCmdEndRendering(VkCommandBuffer cmd);

/* viewport / scissor */
BKP_EXPORTED void bkpCmdSetViewport(VkCommandBuffer cmd, const VkViewport * vp);
BKP_EXPORTED void bkpCmdSetScissor(VkCommandBuffer cmd, const VkRect2D * sc);

/* pipeline & descriptor binding */

/** @brief Bind a graphics pipeline (`VK_PIPELINE_BIND_POINT_GRAPHICS`). */
BKP_EXPORTED void bkpCmdBindPipeline(VkCommandBuffer cmd, VkPipeline pipeline);
/** @brief Bind descriptor sets to the graphics bind point starting at `firstSet`. */
BKP_EXPORTED void bkpCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineLayout layout, uint32_t firstSet, uint32_t count, const VkDescriptorSet * sets);

/* vertex / index buffers */
/** @brief Bind a vertex buffer at the given binding slot with `offset` bytes into the buffer. */
BKP_EXPORTED void bkpCmdBindVertexBuffer(VkCommandBuffer cmd, uint32_t binding, VkBuffer buffer, VkDeviceSize offset);
BKP_EXPORTED void bkpCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

/* push constants */
BKP_EXPORTED void bkpCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void * data);

/* draw */

/**
 * @brief Draw `vertexCount` vertices with `instanceCount = 1` and `firstInstance = 0`.
 * @param firstVertex  First vertex index in the bound vertex buffer (or `gl_VertexIndex`
 *                     base when no vertex buffer is bound).
 */
BKP_EXPORTED void bkpCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t firstVertex);

/** @brief Draw indexed with `instanceCount = 1` and `firstInstance = 0`. */
BKP_EXPORTED void bkpCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset);

/* memory barrier (no image) */
BKP_EXPORTED void bkpCmdMemoryBarrier(VkCommandBuffer cmd,
                                       VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                       VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

/**
 * @brief Submit one or more image memory barriers via the Vulkan sync2 API.
 *
 * Emits a single `vkCmdPipelineBarrier2` call covering all `count` entries.
 * Each @ref BkpImageBarrierInfo specifies the image, old/new layouts, stage
 * and access masks, aspect, mip range, and layer range.  Both queue family
 * indices are set to `VK_QUEUE_FAMILY_IGNORED` (no ownership transfer).
 *
 * @param barriers  Array of barrier descriptors (must not be NULL when count > 0).
 * @param count     Number of barriers in the array.
 */
BKP_EXPORTED void bkpCmdBarrierImages(VkCommandBuffer cmd,
                                       const BkpImageBarrierInfo * barriers,
                                       uint32_t count);

#ifdef __cplusplus
}
#endif

#endif

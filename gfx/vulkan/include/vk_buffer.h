#ifndef BKP_VULKAN_BUFFER_H
#define BKP_VULKAN_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vk_types.h"

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

BKP_EXPORTED uint32_t bkpLookUpMemoryType(VkPhysicalDevice gpu, uint32_t typeFilter, VkMemoryPropertyFlags properties);
BKP_EXPORTED void bkpCreateVkBufferMemory(BkpGpuAdapter adapter, VkBufferCreateInfo info, VkBuffer * buffer, VkDeviceMemory * bufferMemory, VkMemoryPropertyFlags properties);
BKP_EXPORTED void bkpCreateMemoryForImage(BkpGpuAdapter adapter, VkDeviceSize allocSize, VkDeviceMemory * bufferMemory, VkMemoryRequirements * memReq, VkMemoryPropertyFlags properties);
BKP_EXPORTED void bkpCreateVkBuffer(BkpGpuAdapter adapter, VkBufferCreateInfo info, VkBuffer * buffer);
BKP_EXPORTED void bkpCopyBuffer(BkpGpuAdapter adapter, VkBuffer dst, VkDeviceSize dstOffset, VkBuffer src, VkDeviceSize srcOffset, size_t size);

/**
 * @brief Allocate `cmd->count` command buffers from `cmd->commandPool`.
 *
 * The following fields must be set before calling:
 * - `cmd->commandPool` — pool to allocate from (e.g. `&adp->commandPoolGraphic`).
 * - `cmd->count`       — number of command buffers (typically `maxFrameInFlight`).
 * - `cmd->level`       — `VK_COMMAND_BUFFER_LEVEL_PRIMARY` for direct submission.
 *
 * `cmd->resetable` is set from `cmd->commandPool->resetable`, overriding any value
 * you set on the struct.  Set `cmd->commandPool->resetable = BKP_TRUE` beforehand
 * if you need per-buffer reset via @ref bkpResetCommandBuffer.
 *
 * Free with @ref bkpDestroyCommandBuffers.
 */
BKP_EXPORTED void bkpAllocateCmdBuffer(BkpGpuAdapter adapter, BkpCommandBuffer * cmdBuff);

/**
 * @brief Free all command buffers and release the backing array.
 * @param commandBuffer Must have been filled by @ref bkpAllocateCmdBuffer.
 */
BKP_EXPORTED void bkpDestroyCommandBuffers(BkpGpuAdapter adapter, BkpCommandBuffer * commandBuffer);

/** @brief Allocate a single one-time-submit command buffer, begin recording immediately.
 *         Submit and free with @ref bkpEndCmdBufferUniqUsage. */
BKP_EXPORTED void bkpBeginCmdBufferUniqUsage(BkpGpuAdapter adapter, BkpCommandBuffer * cmd, EGpuQueue type);
/** @brief End, submit, wait for completion, and free a one-time command buffer. */
BKP_EXPORTED void bkpEndCmdBufferUniqUsage(BkpGpuAdapter adapter, BkpCommandBuffer * commandBuffer);
BKP_EXPORTED void bkpFlushCmdBuffer(BkpGpuAdapter adapter, BkpCommandBuffer * commandBuffer);

/**
 * @brief Reset then begin recording command buffer at `index`.
 *
 * Calls @ref bkpResetCommandBuffer internally — do not call both.
 *
 * @param index  Frame index, typically `adapter->frameInfo.currentFrame`.
 * @param flag   Usage flags, e.g. `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`
 *               or 0 for a reusable buffer.
 */
BKP_EXPORTED void bkpBeginCommandBuffer(BkpCommandBuffer * cmd, uint32_t index, VkCommandBufferUsageFlags flag);

/**
 * @brief End recording of command buffer at `index`.
 * @param index  Frame index, typically `adapter->frameInfo.currentFrame`.
 */
BKP_EXPORTED void bkpEndCommandBuffer(BkpCommandBuffer * cmd, uint32_t index);

/**
 * @brief Reset command buffer at `index` to the initial state.
 *
 * No-op if `cmd->resetable == BKP_FALSE`.  Uses
 * `VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT`.
 * Already called internally by @ref bkpBeginCommandBuffer.
 */
BKP_EXPORTED void bkpResetCommandBuffer(BkpCommandBuffer * cmd, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif


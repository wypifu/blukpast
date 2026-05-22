#ifndef BKP_VULKAN_INFO_H
#define BKP_VULKAN_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vulkan.h"

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

BKP_EXPORTED const char * bkpVkShaderStage2String(VkShaderStageFlagBits type);
BKP_EXPORTED const char * bkpVkDescriptorType2String(VkDescriptorType type);
BKP_EXPORTED const char * bkpQueue2String(VkQueueFlagBits flag);

BKP_EXPORTED uint32_t getMaxUniformBufferRange(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxStorageBufferRange(BkpGpuAdapter adapter);

BKP_EXPORTED uint32_t getMaxDescriptorSetSamplers(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxDescriptorSetSampledImages(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxDescriptorSetUniformBuffers(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxDescriptorSetUniformBuffersDynamic(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxDescriptorSetStorageBuffers(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxDescriptorSetStorageBuffersDynamic(BkpGpuAdapter adapter);
BKP_EXPORTED uint32_t getMaxDescriptorSetInputAttachments(BkpGpuAdapter adapter);

#ifdef __cplusplus
}
#endif

#endif


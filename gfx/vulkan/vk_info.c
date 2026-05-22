/*Copyright*/
/*
 * AUTHOR: lilington
 * addr	: lilington@yahoo.fr
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "include/vk_info.h"

/**************************************************************************
*	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Union and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/

/**************************************************************************
*	Classes
**************************************************************************/

/***************************************************************************
*	Prototypes
**************************************************************************/

/**************************************************************************
*	Main
**************************************************************************/




/**************************************************************************
*	Implementations
**************************************************************************/

/*_________________________________________________________________________________*/
const char * bkpVkShaderStage2String(VkShaderStageFlagBits type)
{
	switch(type)
	{
		case VK_SHADER_STAGE_VERTEX_BIT : return "VK_SHADER_STAGE_VERTEX_BIT";
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : return "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT";
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT : return "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT";
		case VK_SHADER_STAGE_GEOMETRY_BIT : return "VK_SHADER_STAGE_GEOMETRY_BIT";
		case VK_SHADER_STAGE_FRAGMENT_BIT : return "VK_SHADER_STAGE_FRAGMENT_BIT";
		case VK_SHADER_STAGE_COMPUTE_BIT : return "VK_SHADER_STAGE_COMPUTE_BIT";
		case VK_SHADER_STAGE_TASK_BIT_NV : return "VK_SHADER_STAGE_TASK_BIT_NV";
		case VK_SHADER_STAGE_MESH_BIT_NV : return "VK_SHADER_STAGE_MESH_BIT_NV";
		case VK_SHADER_STAGE_RAYGEN_BIT_KHR : return "VK_SHADER_STAGE_RAYGEN_BIT_KHR";
		case VK_SHADER_STAGE_ANY_HIT_BIT_KHR : return "VK_SHADER_STAGE_ANY_HIT_BIT_KHR";
		case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR : return "VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR";
		case VK_SHADER_STAGE_MISS_BIT_KHR : return "VK_SHADER_STAGE_MISS_BIT_KHR";
		case VK_SHADER_STAGE_INTERSECTION_BIT_KHR : return "VK_SHADER_STAGE_INTERSECTION_BIT_KHR";
		case VK_SHADER_STAGE_CALLABLE_BIT_KHR : return "VK_SHADER_STAGE_CALLABLE_BIT_KHR";
		case VK_SHADER_STAGE_ALL_GRAPHICS : return "VK_SHADER_STAGE_ALL_GRAPHICS";
		case VK_SHADER_STAGE_ALL: return "VK_SHADER_STAGE_ALL";
		case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI : return "VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI";
		case VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI : return "VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI";
	}

	return "VK_SHADER_STAGE_????";
}

/*_________________________________________________________________________________*/
const char * bkpVkDescriptorType2String(VkDescriptorType type)
{
	switch(type)
	{
		case VK_DESCRIPTOR_TYPE_SAMPLER : return "VK_DESCRIPTOR_TYPE_SAMPLER";
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT : return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
		case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK : return "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK";
		case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV : return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV";
		case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE : return "VK_DESCRIPTOR_TYPE_MUTABLE_VALVE";
		case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM : return "VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM";
		case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM : return "VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM";
		case VK_DESCRIPTOR_TYPE_MAX_ENUM : return "VK_DESCRIPTOR_TYPE_MAX_ENUM";
		default: return "VK_DESCRIPTOR_TYPE_????";
	}
}

/*_________________________________________________________________________________*/
const char * bkpQueue2String(VkQueueFlagBits flag)
{
	switch(flag)
	{
			case VK_QUEUE_GRAPHICS_BIT:
				return "FAMILY_GRAPHIC";
			case VK_QUEUE_COMPUTE_BIT:
				return "FAMILY_COMPUTE";
			case VK_QUEUE_TRANSFER_BIT:
				return "FAMILY_TRANSFERT";
			case VK_QUEUE_SPARSE_BINDING_BIT:
				return "FAMILY_SPARSE_BINDING";
			case VK_QUEUE_PROTECTED_BIT:
				return "FAMILY_PROTECTED";
			case VK_QUEUE_VIDEO_DECODE_BIT_KHR:
				return "FAMILY_VIDEO_DECODE";
			case VK_QUEUE_VIDEO_ENCODE_BIT_KHR:
				return "FAMILY_VIDEO_ENCODE";
			case VK_QUEUE_OPTICAL_FLOW_BIT_NV:
				return "FAMILY_OPTICAL_FLOW";
			case VK_QUEUE_FLAG_BITS_MAX_ENUM:
				return "QUEUE_MAX_ENUM";
			default: return "Unknown";
	}
}

/*_________________________________________________________________________________*/
uint32_t getMaxUniformBufferRange(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxUniformBufferRange;
}

/*_________________________________________________________________________________*/
uint32_t getMaxStorageBufferRange(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxStorageBufferRange;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetSamplers(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetSamplers;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetSampledImages(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetSampledImages;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetUniformBuffers(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetUniformBuffers;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetUniformBuffersDynamic(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetStorageBuffers(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetStorageBuffers;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetStorageBuffersDynamic(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetStorageBuffersDynamic;
}

/*_________________________________________________________________________________*/
uint32_t getMaxDescriptorSetInputAttachments(BkpGpuAdapter adapter)
{
	return adapter->deviceProperties.limits.maxDescriptorSetInputAttachments;
}

/*_________________________________________________________________________________*/

#ifdef __cplusplus
}
#endif

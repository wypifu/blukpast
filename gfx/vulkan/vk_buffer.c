/*Copyright*/
/*
 * AUTHOR: lilington
 * addr	: lilington@yahoo.fr
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "include/vk_buffer.h"
#include <system/include/bkp_log.h>

/**************************************************************************
*	Defines & Maro
**************************************************************************/

/**************************************************************************
*	Structs, Enum, Union and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/
static const char * bkpColor = "\x1b[0; 34m";
static const char * bkpTAG = "GFX";

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

/*____________________________________________________________________________________*/
uint32_t bkpLookUpMemoryType(VkPhysicalDevice gpu, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(gpu, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	LOGC(eFATAL, bkpTAG, bkpColor, "Unable to find suitable memory type");

	return 0;
}

void bkpCreateVkBuffer(BkpGpuAdapter adapter, VkBufferCreateInfo info, VkBuffer * buffer)
{
	if(vkCreateBuffer(adapter->device, &info, adapter->allocator, buffer) != VK_SUCCESS)
	{
		LOGC(eDEBUG, bkpTAG, bkpColor, "Unable to create VkBuffer!");
	}
}

/*____________________________________________________________________________________*/
void bkpCreateVkBufferMemory(BkpGpuAdapter adapter, VkBufferCreateInfo info, VkBuffer * buffer, VkDeviceMemory * bufferMemory, VkMemoryPropertyFlags properties)
{
	bkpCreateVkBuffer(adapter, info, buffer);

	VkMemoryRequirements memReq = {};
	vkGetBufferMemoryRequirements(adapter->device, *buffer, &memReq);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = bkpLookUpMemoryType(adapter->gpu, memReq.memoryTypeBits, properties);

	VkMemoryAllocateFlagsInfoKHR allocFlagsinfo = {};
	if((info.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		allocFlagsinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsinfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		allocInfo.pNext = &allocFlagsinfo;
	}

	if(vkAllocateMemory(adapter->device, &allocInfo, adapter->allocator, bufferMemory) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to allocate VkMemoryBuffer!");
	}

	++adapter->allocCount;

	vkBindBufferMemory(adapter->device, *buffer, *bufferMemory, 0);
}

/*__________________________________________________________________________ */
void bkpCreateMemoryForImage(BkpGpuAdapter adapter, VkDeviceSize allocSize, VkDeviceMemory * bufferMemory, VkMemoryRequirements * memReq, VkMemoryPropertyFlags properties)
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = allocSize;
	allocInfo.memoryTypeIndex = bkpLookUpMemoryType(adapter->gpu, memReq->memoryTypeBits, properties);
	if(vkAllocateMemory(adapter->device, &allocInfo, adapter->allocator, bufferMemory) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to allocate image memory!");
	}

	++adapter->allocCount;
}

/*__________________________________________________________________________ */
void bkpCreateVkImageMemory(BkpGpuAdapter adapter, VkImageCreateInfo * imageInfo, VkImage * image, VkDeviceMemory * imageMemory, VkMemoryPropertyFlags properties)
{
	if(vkCreateImage(adapter->device, imageInfo, adapter->allocator, image) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to create image!");
	}

	VkMemoryRequirements memReq = {};
	vkGetImageMemoryRequirements(adapter->device, *image, &memReq);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex   = bkpLookUpMemoryType(adapter->gpu, memReq.memoryTypeBits, properties);

	if(vkAllocateMemory(adapter->device, &allocInfo, adapter->allocator, imageMemory) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to allocate image memory!");
	}

	vkBindImageMemory(adapter->device, *image, *imageMemory, 0);
	++adapter->allocCount;
}


/*____________________________________________________________________________________*/
void bkpCopyBuffer(BkpGpuAdapter adapter, VkBuffer dst, VkDeviceSize dstOffset, VkBuffer src, VkDeviceSize srcOffset, size_t size)
{
	BkpCommandBuffer cmd;
	bkpBeginCmdBufferUniqUsage(adapter, &cmd, eQFAMILY_TRANSFERT);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size		 = size;
	vkCmdCopyBuffer(cmd.cmds[0], src, dst, 1, &copyRegion);

	bkpEndCmdBufferUniqUsage(adapter, &cmd);
}

/*____________________________________________________________________________________*/
void bkpAllocateCmdBuffer(BkpGpuAdapter adapter, BkpCommandBuffer * cmdBuff)
{
	cmdBuff->cmds = (BkpArray(VkCommandBuffer)) bkpArrayCreateFrom(VkCommandBuffer, adapter->memoryGroupId);
	bkpArrayResize(&cmdBuff->cmds, cmdBuff->count);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool                 = cmdBuff->commandPool->commandPool ;
	allocInfo.level                       = cmdBuff->level;
	allocInfo.commandBufferCount          = cmdBuff->count;
	cmdBuff->resetable					  = cmdBuff->commandPool->resetable;

	if(vkAllocateCommandBuffers(adapter->device, &allocInfo, cmdBuff->cmds) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create command buffers!\n");
	}

	cmdBuff->state = eSTATE_READY;
}

/*____________________________________________________________________________________*/
void bkpDestroyCommandBuffers(BkpGpuAdapter adapter, BkpCommandBuffer * commandBuffer)
{
	vkFreeCommandBuffers(adapter->device, commandBuffer->commandPool->commandPool, (uint32_t)bkpArraySize(commandBuffer->cmds), &commandBuffer->cmds[0]);
	bkpArrayDestroy(&commandBuffer->cmds);
}

/*____________________________________________________________________________________*/
void bkpBeginCmdBufferUniqUsage(BkpGpuAdapter adapter, BkpCommandBuffer * cmd, EGpuQueue type)
{
	cmd->level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd->count       = 1;

	if(type == eQFAMILY_GRAPHIC)
	{
		cmd->commandPool    = &adapter->commandPoolGraphic;
	}
	else if(type == eQFAMILY_TRANSFERT)
	{
		cmd->commandPool    = &adapter->commandPoolTransfer;
	}

	bkpAllocateCmdBuffer(adapter, cmd);
	bkpBeginCommandBuffer(cmd, 0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

/*____________________________________________________________________________________*/
void bkpEndCmdBufferUniqUsage(BkpGpuAdapter adapter, BkpCommandBuffer * commandBuffer)
{
	bkpEndCommandBuffer(commandBuffer, 0);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &commandBuffer->cmds[0];

  VkFenceCreateInfo fenceInfo = (VkFenceCreateInfo){};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FLAGS_NONE;
  VkFence fence;
	if(vkCreateFence(adapter->device, &fenceInfo, adapter->allocator, &fence) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create Fence!\n");
	}

	vkQueueSubmit(commandBuffer->commandPool->pQueue->queue[0], 1, &info, fence);
  vkWaitForFences(adapter->device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
  vkDestroyFence(adapter->device, fence, NULL);
	bkpDestroyCommandBuffers(adapter, commandBuffer);
}


/*____________________________________________________________________________________*/
void bkpFlushCmdBuffer(BkpGpuAdapter adapter, BkpCommandBuffer * commandBuffer)
{
	bkpEndCommandBuffer(commandBuffer, 0);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &commandBuffer->cmds[0];

  VkFenceCreateInfo fenceInfo = (VkFenceCreateInfo){};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FLAGS_NONE;
  VkFence fence;
	if(vkCreateFence(adapter->device, &fenceInfo, adapter->allocator, &fence) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create Fence!\n");
	}

	vkQueueSubmit(commandBuffer->commandPool->pQueue->queue[0], 1, &info, fence);
  vkWaitForFences(adapter->device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
  vkDestroyFence(adapter->device, fence, NULL);
}


/*____________________________________________________________________________________*/
void bkpBeginCommandBuffer(BkpCommandBuffer * cmd, uint32_t index, VkCommandBufferUsageFlags flag)
{
	VkCommandBufferBeginInfo bg = {};
	bg.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bg.flags = flag;
	bg.pInheritanceInfo = NULL;

	bkpResetCommandBuffer(cmd, index);

	if(vkBeginCommandBuffer(cmd->cmds[index], &bg))
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "unable to begin comandBuffer");
	}

	cmd->state = eSTATE_RECORDING;
}

/*____________________________________________________________________________________*/
void bkpEndCommandBuffer(BkpCommandBuffer * cmd, uint32_t index)
{
	if(vkEndCommandBuffer(cmd->cmds[index]) != VK_SUCCESS)
	{
		LOG(eFATAL, "Application", "unable to end recording comandBuffer");
	}

	cmd->state = eSTATE_RECORDED;
}

/*____________________________________________________________________________________*/
void bkpResetCommandBuffer(BkpCommandBuffer * cmd, uint32_t index)
{
	if(cmd->resetable)
	{
		if(vkResetCommandBuffer(cmd->cmds[index], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT))
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "unable to reset comandBuffer");
		}
		cmd->state = eSTATE_READY;
	}
}

/*________________________________________________________________________________*/

/*________________________________________________________________________________*/


#ifdef __cplusplus
}
#endif

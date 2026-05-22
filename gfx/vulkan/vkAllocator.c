#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include <system/include/bkp_log.h>
#include <system/include/bkp_thread.h>
#include <system/include/bkp_stack.h>
#include <system/include/bkp_array.h>
#include <system/include/bkp_list.h>
#include <system/include/bkp_allocator.h>
#include "include/vkAllocator.h"

#define EVALUTATION_OPTIMAL_FACTOR 10

	//1 = bold ;38;5 start 256 forground  58m colornumber
//static const char * bkpColor = "\x1b[1;38;5;58m";
static const char * bkpColor = "\x1b[1;38;2;125;70;0m";
static const char * bkpTAG = "GFX::vma";

struct SBlock;
struct SChunkGroup;

/*____________________________________________________________________________________*/
typedef struct
{
	BkpMutex mutex;
	struct SBlock * blocksReverse;
	struct SBlock * blocks;
	struct SBlock * firstFree;
	struct SChunkGroup * pChunkgroup;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
	VkDeviceSize allocatedSize;
	VkDeviceSize size;
	uint32_t blockCount;
	void * mappedData;
	BkpBool mapped;
	size_t mappedCount;
}SChunk;

/*____________________________________________________________________________________*/
typedef struct
{
	void * mappedData;
}SMappedInfo;

/*____________________________________________________________________________________*/
struct BkpBuffer_
{
	SChunk * chunk;
	VkImage image;
	void * mappedData;

	VkBufferUsageFlags usage;
	EBufferType type;
	VkDeviceSize offset;
	VkDeviceSize size;
	VkDeviceSize alignment;
	BkpBool mapped;
	BkpBool gpuOnly;

	/* custom allocator fields — valid only when chunk == NULL */
	VkBuffer       directBuffer;
	VkDeviceMemory directMemory;
	void *         customAlloc;
};

/*____________________________________________________________________________________*/
struct SChunkGroup
{
	BkpMutex mutex;
	BkpList(SChunk *) l_freeChunks;
	BkpArray(SChunk *)  a_chunks;
	VkDeviceSize alignment;
};

/*____________________________________________________________________________________*/
typedef struct
{
	union
	{
		VkBufferUsageFlags usage;
		VkDeviceSize alignment;
	};
	struct SChunkGroup groups[eBUFFER_COUNT];
}SChunkGroupArray;

/*____________________________________________________________________________________*/
struct SBlock
{
	struct SBlock* previous;
	struct SBlock* next;
	struct SBlock* previous_free;
	struct SBlock* next_free;
	BkpBool used;
	struct BkpBuffer_  payload;
};

/*____________________________________________________________________________________*/
struct BkpVkMemoryAllocator_
{
	BkpMutex mutex;
	BkpMutex stagMutex;
	BkpStack(BkpBuffer) staggings;
	BkpArray(SChunkGroupArray *) a_chunkGroups;
	BkpArray(SChunkGroupArray *) a_chunkImageGroups;
	VkDeviceSize chunkSize;
	VkDeviceSize chunkImageSize;

	VkAllocationCallbacks * allocator;
	size_t allocWarning;
	uint16_t availableStagging;
};

/***************************************************************************
 *  Prototypes
 **************************************************************************/

static SChunk * newChunk(BkpGpuAdapter adapter, VkDeviceSize * allocSize, VkDeviceSize size);
static struct SBlock * newBlock(VkDeviceSize size, size_t grpId);
static void allocateBuffer_(BkpGpuAdapter adapter, struct SChunkGroup * grp, BkpBuffer * dstBuffer, BkpBufferInfo * info);
static void allocateBufferImage_(BkpGpuAdapter adapter, struct SChunkGroup * grp, BkpBuffer * dstBuffer, BkpBufferInfo * info, VkMemoryRequirements * memReq);
static SChunk * getChunk(BkpGpuAdapter adapter, struct SChunkGroup * grp, VkDeviceSize size, BkpBufferInfo * info, VkMemoryRequirements * memReq);
static void createChunk(BkpGpuAdapter adapter, SChunk * chunk, VkDeviceSize allocSize, BkpBufferInfo * info);
static void createChunkImage(BkpGpuAdapter adapter, SChunk * chunk, VkDeviceSize allocSize, BkpBufferInfo * info, VkMemoryRequirements * memReq);
static void destroyMemoryAllocator(BkpGpuAdapter adapter);     //cleanup vkbuffers and memory only set to unused
static void uploadDataStagging(BkpGpuAdapter adapter, struct SChunkGroup * grp, BkpBuffer buffer, void * data, size_t offset, size_t data_size);
static void mapChunk(BkpGpuAdapter adapter, SChunk * chk, VkMemoryPropertyFlags propFlags);
static void unMapChunk(BkpGpuAdapter adapter, SChunk * chk);
static void defragmentChunk(BkpGpuAdapter adapter, SChunk * chunk, VkBufferUsageFlags usage);
static void deframent(BkpGpuAdapter adapter, SChunk* chunk, VkBuffer tmp);
static BkpBuffer getAvailableStagging(BkpGpuAdapter adapter, BkpVkMemoryAllocator vma);
static void freeBlock(BkpGpuAdapter adapter, struct SChunkGroup * chunkGroup, BkpBuffer sB);

static void moveBufferDataLeft(BkpGpuAdapter adapter, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkBuffer srcBuffer, VkDeviceSize srcOffset, VkDeviceSize size);
static void addToFreeList(SChunk * chunk, struct SBlock * block);
static void removeFromFreeList(SChunk * chunk, struct SBlock *block);
static void print_Chunk(SChunk * chunk);

/**************************************************************************
 *  Implementations
 **************************************************************************/

/*____________________________________________________________________________________*/
void bkpStartGpuMemoryAllocator(BkpGpuAdapter adapter, BkpAllocatorGPUInfo * info)
{
	BkpVkMemoryAllocator vma_ = bkpAllocFrom(sizeof(struct BkpVkMemoryAllocator_), adapter->memoryGroupId);
	uint32_t maxAllocCount    = adapter->deviceProperties.limits.maxMemoryAllocationCount;
	vma_->allocWarning        = (size_t)(maxAllocCount * adapter->warningAllocPersent);

	if(info->chunkSize > adapter->maxMemoryAllocationSize)
	{
		info->chunkSize = adapter->maxMemoryAllocationSize;
	}
	if(info->chunkImageSize > adapter->maxMemoryAllocationSize)
	{
		info->chunkImageSize = adapter->maxMemoryAllocationSize;
	}

	vma_->staggings =  bkpStackCreateFrom(BkpBuffer, adapter->memoryGroupId);
	vma_->a_chunkGroups = bkpArrayCreateFrom(SChunkGroupArray *, adapter->memoryGroupId);
	vma_->a_chunkImageGroups = bkpArrayCreateFrom(SChunkGroupArray *, adapter->memoryGroupId);
	vma_->chunkSize         = info->chunkSize;
	vma_->availableStagging = info->maxStaggingBuffers;
	bkpCreateMutex(&vma_->mutex);
	bkpCreateMutex(&vma_->stagMutex);

	adapter->vma_ = vma_;
	LOGC(eDEBUG, bkpTAG, bkpColor, "Memory Allocator Started: maxAllocCount %u, maxAllocSize %lu, warningAlloc %u, alloc %u",
			maxAllocCount, adapter->maxMemoryAllocationSize, adapter->vma_->allocWarning, adapter->allocCount);
}

/*____________________________________________________________________________________*/
void bkpShutdownGpuMemoryAllocator(BkpGpuAdapter adapter)
{
	if(adapter->vma_)
	{
		bkpDestroyMutex(&adapter->vma_->mutex);
		bkpDestroyMutex(&adapter->vma_->stagMutex);
		destroyMemoryAllocator(adapter);
		bkpFree(adapter->vma_);
		adapter->vma_ = NULL;
	}

	LOGC(eDEBUG, bkpTAG, bkpColor, "Memory Allocator Shutdown");
}

/*____________________________________________________________________________________*/
VkDeviceSize bkpGetAlignmentVk(BkpGpuAdapter adapter, VkBufferUsageFlags usage)
{
	VkDeviceSize alig = 16; /* minimum: satisfies float/vec3/vec4 and __m128 requirements */

	if((usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	{
		VkDeviceSize uboAlign = adapter->deviceProperties.limits.minUniformBufferOffsetAlignment;
		if(uboAlign > alig) alig = uboAlign;
	}

	if((usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
	{
		VkDeviceSize ssboAlign = adapter->deviceProperties.limits.minStorageBufferOffsetAlignment;
		if(ssboAlign > alig) alig = ssboAlign;
	}

	return alig;
}

/*____________________________________________________________________________________*/
VkDeviceMemory bkpGetDeviceMemory(BkpGpuAdapter adapter, BkpBuffer buff)
{
	(void)adapter;
	return buff->chunk ? buff->chunk->bufferMemory : buff->directMemory;
}

/*____________________________________________________________________________________*/
VkDeviceSize bkpAlignSizeVk(VkDeviceSize size, VkDeviceSize alignment)
{
	VkDeviceSize div = size / alignment;
	if(div * alignment < size)
	{
		return (div + 1) * alignment;
	}

	return size;
}

/*____________________________________________________________________________________*/
struct SChunkGroup * getChunkGroup(BkpGpuAdapter adapter, VkBufferUsageFlags usage, uint8_t type)
{
	//LOGC(eDEBUG, bkpTAG, bkpColor, "\t\t********* Allocationg Buffer: %lu (%u)", usage, type);
	size_t grpSize = bkpArraySize(adapter->vma_->a_chunkGroups);
	for(size_t i = 0; i < grpSize; ++i)
	{
		if(adapter->vma_->a_chunkGroups[i]->usage == usage)
		{
			return  &adapter->vma_->a_chunkGroups[i]->groups[type];
		}
	}

	SChunkGroupArray * arr = bkpAllocFrom(sizeof(SChunkGroupArray), adapter->memoryGroupId);
	arr->usage = usage;
	for(size_t i = 0; i < (uint8_t)eBUFFER_COUNT; ++i)
	{
		arr->groups[i].alignment = 0;
		bkpCreateMutex(&arr->groups[i].mutex);
		arr->groups[i].alignment = bkpGetAlignmentVk(adapter, usage);
		arr->groups[i].a_chunks = bkpArrayCreateFrom(SChunk*, adapter->memoryGroupId);
		arr->groups[i].l_freeChunks = (SChunk **) bkpListCreateFrom(SChunk *, adapter->memoryGroupId);
	}
	bkpArrayPush(&adapter->vma_->a_chunkGroups, arr);

	return &arr->groups[type];
}

/*____________________________________________________________________________________*/
struct SChunkGroup * getChunkImageGroup(BkpGpuAdapter adapter, VkDeviceSize alignment, uint8_t type)
{
	//LOGC(eDEBUG, bkpTAG, bkpColor, "\t\t ++++++++++++++++Allocationg imageBuffer align : %lu (%u)", alignment, type);
	size_t grpSize = bkpArraySize(adapter->vma_->a_chunkImageGroups);
	for(size_t i = 0; i < grpSize; ++i)
	{
		if(adapter->vma_->a_chunkImageGroups[i]->alignment == alignment)
		{
			return  &adapter->vma_->a_chunkImageGroups[i]->groups[type];
		}
	}

	SChunkGroupArray * arr = bkpAllocFrom(sizeof(SChunkGroupArray), adapter->memoryGroupId);
	arr->alignment = alignment;
	for(size_t i = 0; i < (uint8_t)eBUFFER_COUNT; ++i)
	{
		bkpCreateMutex(&arr->groups[i].mutex);
		arr->groups[i].alignment = alignment;
		arr->groups[i].a_chunks = bkpArrayCreateFrom(SChunk*, adapter->memoryGroupId);
		arr->groups[i].l_freeChunks = (SChunk **) bkpListCreateFrom(SChunk *, adapter->memoryGroupId);
	}
	bkpArrayPush(&adapter->vma_->a_chunkImageGroups, arr);

	return &arr->groups[type];
}


/*____________________________________________________________________________________*/
static void allocBuffer_bkp(BkpGpuAdapter adapter, BkpBuffer * dstBuff, BkpBufferInfo * info)
{
	BkpVkMemoryAllocator vma = adapter->vma_;
	if(!vma)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Memory Allocator not initialized");
	}

	struct SChunkGroup * chunkGroup = getChunkGroup(adapter, info->usage, (uint8_t)info->type);

	info->isImage = BKP_FALSE;
	info->size = bkpAlignSizeVk(info->size, chunkGroup->alignment);

	allocateBuffer_(adapter, chunkGroup, dstBuff, info);
}

/*____________________________________________________________________________________*/
static void allocImage_bkp(BkpGpuAdapter adapter, VkImageCreateInfo * imageInfo, BkpBuffer * dstBuffer, BkpBufferInfo * info)
{
	BkpVkMemoryAllocator vma = adapter->vma_;
	if(!vma)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Memory Allocator not initialized");
	}

	VkImage image = VK_NULL_HANDLE;
	imageInfo->sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	if(vkCreateImage(adapter->device, imageInfo, adapter->allocator, &image) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to create image!");
	}

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(adapter->device, image, &memReq);

	memReq.size = bkpAlignSizeVk(memReq.size, memReq.alignment);
	info->isImage = BKP_TRUE;
	info->size = memReq.size;

	struct SChunkGroup * chunkGroup = getChunkImageGroup(adapter, memReq.alignment, (uint8_t)info->type);

	BkpBuffer tmpBuffer;
	allocateBufferImage_(adapter, chunkGroup, &tmpBuffer, info, &memReq);

	vkBindImageMemory(adapter->device, image, tmpBuffer->chunk->bufferMemory, tmpBuffer->offset);
	tmpBuffer->image = image;
	tmpBuffer->alignment = memReq.alignment;
	*dstBuffer = tmpBuffer;
}

/*____________________________________________________________________________________*/
static void allocBuffer_none(BkpGpuAdapter adapter, BkpBuffer * dstBuff, BkpBufferInfo * info)
{
	(void)adapter; (void)info;
	LOGC(eWARNING, bkpTAG, bkpColor, "VMA_NONE: buffer allocation is disabled");
	*dstBuff = NULL;
}

static void allocImage_none(BkpGpuAdapter adapter, VkImageCreateInfo * imageInfo, BkpBuffer * dstBuffer, BkpBufferInfo * info)
{
	(void)adapter; (void)imageInfo; (void)info;
	LOGC(eWARNING, bkpTAG, bkpColor, "VMA_NONE: image allocation is disabled");
	*dstBuffer = NULL;
}

static void freeBuffer_none(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	(void)adapter;
	*destBuffer = NULL;
}

static void freeImage_none(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	(void)adapter;
	*destBuffer = NULL;
}

static void mapBuffer_none(BkpGpuAdapter adapter, BkpBuffer buff)   { (void)adapter; (void)buff; }
static void unmapBuffer_none(BkpGpuAdapter adapter, BkpBuffer buff) { (void)adapter; (void)buff; }

static void uploadData_none(BkpGpuAdapter adapter, BkpBuffer buffer, void * data, size_t offset, size_t data_size)
{
	(void)adapter; (void)buffer; (void)data; (void)offset; (void)data_size;
	LOGC(eWARNING, bkpTAG, bkpColor, "VMA_NONE: uploadData is disabled");
}

/*____________________________________________________________________________________*/
static void allocBuffer_custom(BkpGpuAdapter adapter, BkpBuffer * dstBuff, BkpBufferInfo * info)
{
	VkBufferCreateInfo ci = {0};
	ci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.size                  = info->size;
	ci.usage                 = info->usage;
	ci.sharingMode           = info->qCount > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	ci.queueFamilyIndexCount = info->qCount;
	ci.pQueueFamilyIndices   = info->queueIndices;

	VkBuffer buf = VK_NULL_HANDLE;
	VkDeviceMemory mem = VK_NULL_HANDLE;
	void * alloc = NULL;
	if(!adapter->vmaCallbacks.allocBuffer(adapter->vmaCallbacks.userData,
	                                       adapter->device, adapter->gpu,
	                                       &ci, info->type, &buf, &mem, &alloc))
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Custom VMA: allocBuffer callback failed");
		*dstBuff = NULL;
		return;
	}

	BkpBuffer b = (BkpBuffer)bkpAllocFrom(sizeof(struct BkpBuffer_), adapter->memoryGroupId);
	memset(b, 0, sizeof(struct BkpBuffer_));
	b->directBuffer = buf;
	b->directMemory = mem;
	b->customAlloc  = alloc;
	b->usage   = info->usage;
	b->type    = info->type;
	b->size    = info->size;
	b->gpuOnly = (info->type == eBUFFER_GPU) ? BKP_TRUE : BKP_FALSE;
	*dstBuff = b;
}

static void allocImage_custom(BkpGpuAdapter adapter, VkImageCreateInfo * imageInfo, BkpBuffer * dstBuffer, BkpBufferInfo * info)
{
	imageInfo->sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	VkImage img = VK_NULL_HANDLE;
	VkDeviceMemory mem = VK_NULL_HANDLE;
	void * alloc = NULL;
	if(!adapter->vmaCallbacks.allocImage(adapter->vmaCallbacks.userData,
	                                      adapter->device, adapter->gpu,
	                                      imageInfo, info->type, &img, &mem, &alloc))
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Custom VMA: allocImage callback failed");
		*dstBuffer = NULL;
		return;
	}

	BkpBuffer b = (BkpBuffer)bkpAllocFrom(sizeof(struct BkpBuffer_), adapter->memoryGroupId);
	memset(b, 0, sizeof(struct BkpBuffer_));
	b->image        = img;
	b->directMemory = mem;
	b->customAlloc  = alloc;
	b->type    = info->type;
	b->size    = info->size;
	b->gpuOnly = BKP_TRUE;
	*dstBuffer = b;
}

static void freeBuffer_custom(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	BkpBuffer b = *destBuffer;
	*destBuffer = NULL;
	adapter->vmaCallbacks.freeBuffer(adapter->vmaCallbacks.userData,
	                                  b->directBuffer, b->directMemory, b->customAlloc);
	bkpFree(b);
}

static void freeImage_custom(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	BkpBuffer b = *destBuffer;
	*destBuffer = NULL;
	adapter->vmaCallbacks.freeImage(adapter->vmaCallbacks.userData,
	                                 b->image, b->directMemory, b->customAlloc);
	bkpFree(b);
}

static void mapBuffer_custom(BkpGpuAdapter adapter, BkpBuffer buff)
{
	if(buff->mapped || buff->type == eBUFFER_GPU) return;
	buff->mappedData = adapter->vmaCallbacks.mapMemory(adapter->vmaCallbacks.userData, buff->customAlloc);
	buff->mapped = BKP_TRUE;
}

static void unmapBuffer_custom(BkpGpuAdapter adapter, BkpBuffer buff)
{
	if(!buff->mapped) return;
	adapter->vmaCallbacks.unmapMemory(adapter->vmaCallbacks.userData, buff->customAlloc);
	buff->mappedData = NULL;
	buff->mapped = BKP_FALSE;
}

static void uploadData_custom(BkpGpuAdapter adapter, BkpBuffer buffer, void * data, size_t offset, size_t data_size)
{
	(void)adapter;
	if(buffer->type == eBUFFER_CPU_GPU && buffer->mappedData)
	{
		int8_t * p = (int8_t *)buffer->mappedData + offset;
		memcpy(p, data, data_size);
	}
	else
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "Custom VMA: uploadData only supported for mapped CPU_GPU buffers");
	}
}

/*____________________________________________________________________________________*/
static BkpVmaVtable gVtableNone   = { allocBuffer_none,   allocImage_none,   freeBuffer_none,   freeImage_none,   mapBuffer_none,   unmapBuffer_none,   uploadData_none   };
static BkpVmaVtable gVtableCustom = { allocBuffer_custom, allocImage_custom, freeBuffer_custom, freeImage_custom, mapBuffer_custom, unmapBuffer_custom, uploadData_custom };

/*____________________________________________________________________________________*/
/* gVtableBkp is defined later (after the static bkp free/map functions) */
static void freeBuffer_bkp(BkpGpuAdapter adapter, BkpBuffer * destBuffer);
static void freeImage_bkp (BkpGpuAdapter adapter, BkpBuffer * destBuffer);
static void mapBuffer_bkp (BkpGpuAdapter adapter, BkpBuffer buff);
static void unmapBuffer_bkp(BkpGpuAdapter adapter, BkpBuffer buff);
static void uploadData_bkp (BkpGpuAdapter adapter, BkpBuffer buffer, void * data, size_t offset, size_t data_size);

static BkpVmaVtable gVtableBkp = { allocBuffer_bkp, allocImage_bkp, freeBuffer_bkp, freeImage_bkp, mapBuffer_bkp, unmapBuffer_bkp, uploadData_bkp };

/*____________________________________________________________________________________*/
void bkpSetupVmaVtable(BkpGpuAdapter adapter, EBkpVmaMode mode, BkpVmaCallbacks * callbacks)
{
	if(callbacks) adapter->vmaCallbacks = *callbacks;
	switch(mode)
	{
		case eVMA_CUSTOM: adapter->vmaVtable = &gVtableCustom; break;
		case eVMA_NONE:   adapter->vmaVtable = &gVtableNone;   break;
		default:          adapter->vmaVtable = &gVtableBkp;    break;
	}
}

/*____________________________________________________________________________________*/
void bkpAllocateBuffer(BkpGpuAdapter adapter, BkpBuffer * dstBuff, BkpBufferInfo * info)
{
	adapter->vmaVtable->allocBuffer(adapter, dstBuff, info);
}

/*____________________________________________________________________________________*/
void bkpAllocateBufferImage(BkpGpuAdapter adapter, VkImageCreateInfo * imageInfo, BkpBuffer * dstBuffer, BkpBufferInfo * info)
{
	adapter->vmaVtable->allocImage(adapter, imageInfo, dstBuffer, info);
}

/*____________________________________________________________________________________*/
static struct SBlock * newBlock(VkDeviceSize size, size_t grpId)
{
	struct SBlock * p = (struct SBlock *)bkpAllocFrom(sizeof(struct SBlock), grpId);
	p->next = p->previous = p->previous_free = p->next_free =  NULL;
	p->used = BKP_FALSE;
	BkpBuffer pBuff = (BkpBuffer)((uint8_t *)p + offsetof(struct SBlock, payload));
	pBuff->offset = 0;
	pBuff->size = size;
	pBuff->alignment = 0;
	pBuff->mapped = BKP_FALSE;
	pBuff->image = VK_NULL_HANDLE;

	return p;
}


/*____________________________________________________________________________________*/
static void allocateBuffer_(BkpGpuAdapter adapter, struct SChunkGroup * grp, BkpBuffer * dstBuffer, BkpBufferInfo * info)
{
	SChunk * chunk = getChunk(adapter, grp, info->size, info, NULL);

	bkpLockMutex(&chunk->mutex);
	struct SBlock * avail = chunk->firstFree;
	struct SBlock * block = NULL;

	while(block == NULL)
	{
		while(avail != NULL)
		{
			BkpBuffer pBuff = (BkpBuffer)((uint8_t *)avail + offsetof(struct SBlock, payload));
			if(pBuff->size < info->size)
			{
				avail = avail->next_free;
				continue;
			}

			if(pBuff->size > info->size)
			{
				++chunk->blockCount;
				block = newBlock(info->size, adapter->memoryGroupId);
				//addToFreeList(chunk, block);
				BkpBuffer qBuff = (BkpBuffer)((uint8_t *)block + offsetof(struct SBlock, payload));
				if(avail->previous) avail->previous->next = block;
				block->previous = avail->previous;
				block->next	= avail;
				avail->previous = block;
				block->used = BKP_TRUE;

				qBuff->type = info->type;
				qBuff->usage = info->usage;
				qBuff->chunk = chunk;
				qBuff->offset = pBuff->offset;
				qBuff->size = info->size;
				qBuff->mapped = BKP_FALSE;
				qBuff->gpuOnly = pBuff->gpuOnly;
				pBuff->size -= info->size;
				pBuff->offset += info->size;
				if(chunk->blocks == avail) chunk->blocks = block;
			}
			else if(pBuff->size == info->size)
			{
				block = avail;
				block->used = BKP_TRUE;
				pBuff->type   = info->type;
				pBuff->usage  = info->usage;
				pBuff->image  = VK_NULL_HANDLE;
				pBuff->mapped = BKP_FALSE;
				removeFromFreeList(chunk, block);
				if(chunk->firstFree == NULL)
				{
					void * it = bkpListFind(grp->l_freeChunks, chunk, bkpContainerFindPtr);
					bkpListRemove(&grp->l_freeChunks, it);
				}
			}
			break;
		}

		if(block == NULL)
		{
			LOGC(eDEBUG, bkpTAG, bkpColor, "Defragmentation...");
			print_Chunk(chunk);
			defragmentChunk(adapter, chunk, info->usage);
			print_Chunk(chunk);
			avail = chunk->firstFree;
		}
	}

	chunk->size -= info->size;
	bkpUnlockMutex(&chunk->mutex);

	*dstBuffer = (BkpBuffer)((uint8_t *)block + offsetof(struct SBlock, payload));
}

/*____________________________________________________________________________________*/
static void allocateBufferImage_(BkpGpuAdapter adapter, struct SChunkGroup * grp, BkpBuffer * dstBuffer, BkpBufferInfo * info, VkMemoryRequirements * memReq)
{
	SChunk * chunk = getChunk(adapter, grp, info->size, info, memReq);

	bkpLockMutex(&chunk->mutex);
	struct SBlock * avail = chunk->firstFree;
	struct SBlock * block = NULL;

	while(block == NULL)
	{
		while(avail != NULL)
		{
			BkpBuffer pBuff = (BkpBuffer)((uint8_t *)avail + offsetof(struct SBlock, payload));
			//LOG(eWARNING, "TESTER %%", "image size %u, buff size %u", info->size, pBuff->size);
			if(pBuff->size < info->size)
			{
				avail = avail->next_free;
				continue;
			}

			if(pBuff->size > info->size)
			{
				++chunk->blockCount;
				block = newBlock(info->size, adapter->memoryGroupId);
				BkpBuffer qBuff = (BkpBuffer)((uint8_t *)block + offsetof(struct SBlock, payload));
				if(avail->previous) avail->previous->next = block;
				block->previous = avail->previous;
				block->next	= avail;
				avail->previous = block;
				block->used = BKP_TRUE;

				qBuff->type = info->type;
				qBuff->usage = info->usage;
				qBuff->chunk = chunk;
				qBuff->offset = pBuff->offset;
				qBuff->gpuOnly = pBuff->gpuOnly;
				qBuff->size = info->size;
				pBuff->size -= info->size;
				pBuff->offset += info->size;
				if(chunk->blocks == avail) chunk->blocks = block;
			}
			else if(pBuff->size == info->size)
			{
				block = avail;
				block->used = BKP_TRUE;
				removeFromFreeList(chunk, block);
				if(chunk->firstFree == NULL)
				{
					void * it = bkpListFind(grp->l_freeChunks, chunk, bkpContainerFindPtr);
					bkpListRemove(&grp->l_freeChunks, it);
				}
			}
			break;
		}

		if(block == NULL)
		{
			VkDeviceSize size_;
			SChunk * chunk0 = newChunk(adapter, &size_, info->size);
			createChunkImage(adapter, chunk0, size_, info, memReq);
			avail = chunk0->firstFree;
			bkpLockMutex(&grp->mutex);
			bkpArrayPush(&grp->a_chunks, chunk0);
			bkpListAdd(&grp->l_freeChunks, chunk0);
			bkpUnlockMutex(&grp->mutex);
			chunk = chunk0;
		}
	}

	chunk->size -= info->size;
	bkpUnlockMutex(&chunk->mutex);

	*dstBuffer = (BkpBuffer)((uint8_t *)block + offsetof(struct SBlock, payload));
}

/*____________________________________________________________________________________*/
static void freeBuffer_bkp(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	BkpBuffer sB = *destBuffer;
	*destBuffer = NULL;
	struct SChunkGroup * chunkGroup = sB->chunk->pChunkgroup;
	freeBlock(adapter, chunkGroup, sB);
}

static void freeImage_bkp(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	BkpBuffer sB = *destBuffer;
	struct SChunkGroup * chunkGroup = sB->chunk->pChunkgroup;
	vkDestroyImage(adapter->device, sB->image, adapter->allocator);
	freeBlock(adapter, chunkGroup, sB);
	*destBuffer = NULL;
}

/*____________________________________________________________________________________*/
void bkpFreeBuffer(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	adapter->vmaVtable->freeBuffer(adapter, destBuffer);
}

/*____________________________________________________________________________________*/
void bkpFreeBufferImage(BkpGpuAdapter adapter, BkpBuffer * destBuffer)
{
	adapter->vmaVtable->freeImage(adapter, destBuffer);
}

/*____________________________________________________________________________________*/
static void freeBlock(BkpGpuAdapter adapter, struct SChunkGroup * chunkGroup, BkpBuffer sB)
{
	SChunk ** it0 = (SChunk **)bkpArrayFind(chunkGroup->a_chunks, sB->chunk, bkpContainerFindPtr);
	if(it0 == NULL)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "chunk not found will ignore");
		return;
	}

	SChunk * chk = *it0;
	struct SBlock * block = (struct SBlock *)((uint8_t *)sB - offsetof(struct SBlock, payload));

	if(!block->used) LOGC(eFATAL, bkpTAG, bkpColor, "*** Double free corruption ***");

	if(chk->size == 0)
	{
		bkpLockMutex(&chunkGroup->mutex);
		bkpListAdd(&chunkGroup->l_freeChunks, chk);
		bkpUnlockMutex(&chunkGroup->mutex);
	}

	bkpLockMutex(&chk->mutex);

	chk->size += sB->size;
	block->used = BKP_FALSE;

	if(sB->mapped)
	{
		bkpUnlockMutex(&chk->mutex);
		bkpUnmapBuffer(adapter, sB);
		bkpLockMutex(&chk->mutex);
	}
	uint8_t hasPrev = 0;

	if(block->previous)
	{
		if(block->previous->used == BKP_FALSE)
		{
			hasPrev = 1;
			struct SBlock * p = block->previous;
			p->next = block->next;

			if(block->next) block->next->previous = p;

			BkpBuffer pB = (BkpBuffer)((uint8_t *)p + offsetof(struct SBlock, payload));
			BkpBuffer qB = (BkpBuffer)((uint8_t *)block + offsetof(struct SBlock, payload));
			pB->size	+= qB->size;

			if(chk->blocks == block)
			{
				chk->blocks = p;
			}

			bkpFree(block);
			block = p;
			--chk->blockCount;
		}
	}

	if(hasPrev == 0)
	{
	   	addToFreeList(chk, block);
	}

	if(block->next)
	{
		if(block->next->used == BKP_FALSE)
		{
			struct SBlock * p = block->next;
			removeFromFreeList(chk, p);

			if(p->next) p->next->previous = block;
			block->next = p->next;

			BkpBuffer pB = (BkpBuffer)((uint8_t *)p + offsetof(struct SBlock, payload));
			BkpBuffer qB = (BkpBuffer)((uint8_t *)block + offsetof(struct SBlock, payload));
			qB->size	+= pB->size;

			if(chk->blocks == p)
			{
				chk->blocks = block;
			}
			if(chk->blocksReverse == p) chk->blocksReverse = block;

			bkpFree(p);
			--chk->blockCount;
		}
	}

	bkpUnlockMutex(&chk->mutex);
}

/*____________________________________________________________________________________*/
static SChunk * newChunk(BkpGpuAdapter adapter, VkDeviceSize * allocSize, VkDeviceSize size)
{
	SChunk * chunk = (SChunk *)bkpAllocFrom(sizeof(SChunk), adapter->memoryGroupId);

	VkDeviceSize size_ = size > adapter->vma_->chunkSize ? size : adapter->vma_->chunkSize;

	if(size_ > adapter->maxMemoryAllocationSize)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Trying to create chunk of size over GPU maxMemoryAllocationSize limit");
	}

	*allocSize = size_;

	return chunk;
}

/*____________________________________________________________________________________*/
static SChunk * getChunk(BkpGpuAdapter adapter, struct SChunkGroup * grp, VkDeviceSize size, BkpBufferInfo * info, VkMemoryRequirements * memReq)
{
	bkpLockMutex(&grp->mutex);
	for(SChunk ** iterator = (SChunk **)bkpListFirst(grp->l_freeChunks); iterator != NULL; iterator = (SChunk **)bkpListNext(iterator))
	{
		if(size <= (*iterator)->size)
		{
			bkpUnlockMutex(&grp->mutex);
			return *iterator;
		}
	}
	bkpUnlockMutex(&grp->mutex);

	VkDeviceSize size_;
	SChunk * chunk = newChunk(adapter, &size_, size);

	if(!info->isImage)
	{
		createChunk(adapter, chunk, size_, info);
	}
	else
	{
		createChunkImage(adapter, chunk, size_, info, memReq);
	}

	bkpLockMutex(&grp->mutex);
	bkpArrayPush(&grp->a_chunks, chunk);
	bkpListAdd(&grp->l_freeChunks, chunk);
	chunk->pChunkgroup = grp;
	bkpUnlockMutex(&grp->mutex);

	return chunk;
}

/*____________________________________________________________________________________*/
static void createChunk(BkpGpuAdapter adapter, SChunk * chunk, VkDeviceSize allocSize, BkpBufferInfo * info)
{
	VkMemoryPropertyFlags properties = 0;
	VkBufferUsageFlags vkUsage = info->usage;
	BkpBool gpuOnly = BKP_FALSE;

	if(info->customType.used == BKP_TRUE)
	{
		gpuOnly = info->customType.gpuOnly;
		properties = info->customType.properties;
	}
	else
	{
		if(info->type == eBUFFER_GPU)
		{
			properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			gpuOnly = BKP_TRUE;
		}
		else if((info->type == eBUFFER_CPU_GPU) || (info->type == eBUFFER_GPU_CPU))
		{
			properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
	}

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext                 = NULL;
	bufferInfo.flags                 = 0;
	bufferInfo.size                  = allocSize;
	bufferInfo.usage                 = vkUsage;
	if(info->qCount > 0)
	{
		bufferInfo.queueFamilyIndexCount = info->qCount;
		bufferInfo.pQueueFamilyIndices   = info->queueIndices;
		if(info->qCount > 1)
		{
			bufferInfo.sharingMode 			 = VK_SHARING_MODE_CONCURRENT;
		}
	}
	else
	{
		bufferInfo.queueFamilyIndexCount = 0;
		bufferInfo.pQueueFamilyIndices   = 0;
		bufferInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	}

	bkpCreateVkBufferMemory(adapter, bufferInfo, &chunk->buffer, &chunk->bufferMemory, properties);
	LOGC(eDEBUG, bkpTAG, bkpColor, "Buffer and Memory created %lu/%u, with %u queues", adapter->allocCount, adapter->deviceProperties.limits.maxMemoryAllocationCount, bufferInfo.queueFamilyIndexCount = info->qCount);
	chunk->allocatedSize = allocSize;
	chunk->size = allocSize;
	chunk->blocks = newBlock(allocSize, adapter->memoryGroupId);
	chunk->firstFree = chunk->blocksReverse = chunk->blocks;
	chunk->mapped = BKP_FALSE;
	chunk->blockCount = 1;
	chunk->mappedCount = 0;

	BkpBuffer buff = (BkpBuffer)((uint8_t *)chunk->blocks + offsetof(struct SBlock, payload));
	buff->offset = 0;
	buff->size = allocSize;
	buff->chunk = chunk;
	buff->type = info->type;
	buff->usage = info->usage;
	buff->gpuOnly = gpuOnly;

	bkpCreateMutex(&chunk->mutex);

	if(buff->gpuOnly == BKP_FALSE)
	{
		mapChunk(adapter, chunk, properties);
	}
}

/*____________________________________________________________________________________*/
static void createChunkImage(BkpGpuAdapter adapter, SChunk * chunk, VkDeviceSize allocSize, BkpBufferInfo * info, VkMemoryRequirements * memReq)
{
	VkMemoryPropertyFlags properties = 0;
	BkpBool gpuOnly = BKP_FALSE;

	if(info->customType.used == BKP_TRUE)
	{
		gpuOnly = info->customType.gpuOnly;
		properties = info->customType.properties;
	}
	else
	{
		if(info->type == eBUFFER_GPU)
		{
			properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			gpuOnly = BKP_TRUE;
		}
		else if((info->type == eBUFFER_CPU_GPU) || (info->type == eBUFFER_GPU_CPU))
		{
			properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
	}

	bkpCreateMemoryForImage(adapter, allocSize, &chunk->bufferMemory, memReq, properties);

	//LOGC(eERROR, bkpTAG, bkpColor, "Image Memory created %lu/%u", adapter->allocCount, adapter->deviceProperties.limits.maxMemoryAllocationCount);
	chunk->allocatedSize = allocSize;
	chunk->size = allocSize;
	chunk->blocks = newBlock(allocSize, adapter->memoryGroupId);
	chunk->firstFree = chunk->blocksReverse = chunk->blocks;
	chunk->mapped = BKP_FALSE;
	chunk->blockCount = 1;

	BkpBuffer buff = (BkpBuffer)((uint8_t *)chunk->blocks + offsetof(struct SBlock, payload));
	buff->offset = 0;
	buff->size = allocSize;
	buff->chunk = chunk;
	buff->type = info->type;
	buff->usage = info->usage;
	buff->gpuOnly = gpuOnly;

	bkpCreateMutex(&chunk->mutex);
}

/*____________________________________________________________________________________*/
void bkpClearGpuMemoryAllocator(BkpGpuAdapter adapter)
{
}

/*____________________________________________________________________________________*/
static void destroyMemoryAllocator(BkpGpuAdapter adapter)
{
	LOGC(eDEBUG, bkpTAG, bkpColor, "Destroying Vulkan memory allocator");
	BkpVkMemoryAllocator vma = adapter->vma_;
	{
		size_t s_stack = bkpStackSize(vma->staggings);
		LOGC(eDEBUG, bkpTAG, bkpColor, "\t* Destroying %lu stagging buffers", s_stack);
		while(s_stack > 0)
		{
			BkpBuffer st;
			bkpStackPop(&vma->staggings, &st);
			bkpFreeBuffer(adapter, &st);
			s_stack = bkpStackSize(vma->staggings);
		}
		bkpStackDestroy(&vma->staggings);

		size_t grpSize = bkpArraySize(adapter->vma_->a_chunkGroups);
		for(size_t i = 0; i < grpSize; ++i)
		{
			for(size_t j = 0; j < eBUFFER_COUNT; ++j)
			{
				SChunkGroupArray * arr = vma->a_chunkGroups[i];
				bkpListClear(&arr->groups[j].l_freeChunks);
				size_t chunkCount = bkpArraySize(arr->groups[j].a_chunks);
				for(size_t k = 0; k < chunkCount; ++k)
				{
					SChunk * ch = arr->groups[j].a_chunks[k];
					if(ch->allocatedSize != ch->size)
					{
						size_t n = 0;
						for(struct SBlock * p  = ch->blocksReverse; p != NULL ; p = p->previous )
						{
							if(p->used) ++n;
						}
						LOGC(eFATAL, bkpTAG, bkpColor, "Memory leak!!!!  bkp::gfx::BkpBuffer not Destroy remaining(%u)", n);
					}
					bkpFree(ch->blocksReverse);
					if(ch->mapped)
					{
						if(ch->mappedCount > 0)
						{
							LOGC(eERROR, bkpTAG, bkpColor, "Dangling pointers(%u) head, should unmap all buffers", ch->mappedCount);
						}
						unMapChunk(adapter, ch);
					}
					vkDestroyBuffer(adapter->device, ch->buffer, adapter->allocator);
					vkFreeMemory(adapter->device, ch->bufferMemory, adapter->allocator);
					bkpDestroyMutex(&ch->mutex);
					bkpFree(ch);
				}
				bkpArrayDestroy(&arr->groups[j].a_chunks);
				bkpListDestroy(&arr->groups[j].l_freeChunks);
				if(arr->groups[j].mutex.initialized)
				{
					bkpDestroyMutex(&arr->groups[j].mutex);
				}
			}
			bkpFree(vma->a_chunkGroups[i]);
		}
		bkpArrayDestroy(&vma->a_chunkGroups);
	}

	{
		size_t grpSize = bkpArraySize(adapter->vma_->a_chunkImageGroups);
		for(size_t i = 0; i < grpSize; ++i)
		{
			for(size_t j = 0; j < eBUFFER_COUNT; ++j)
			{
				SChunkGroupArray * arr = vma->a_chunkImageGroups[i];
				bkpListClear(&arr->groups[j].l_freeChunks);
				size_t chunkCount = bkpArraySize(arr->groups[j].a_chunks);
				for(size_t k = 0; k < chunkCount; ++k)
				{
					SChunk * ch = arr->groups[j].a_chunks[k];
					if(ch->allocatedSize != ch->size)
					{
						size_t n = 0;
						for(struct SBlock * p  = ch->blocksReverse; p != NULL ; p = p->previous )
						{
							if(p->used) ++n;
						}
						LOGC(eFATAL, bkpTAG, bkpColor, "Memory leak!!!!  bkp::gfx::BkpBufferImage not Destroy remaining(%u)", n);
					}
					bkpFree(ch->blocksReverse);
					if(ch->mapped)
					{
						if(ch->mappedCount > 0)
						{
							LOGC(eERROR, bkpTAG, bkpColor, "Dangling pointers(%u) head, should unmap all buffers", ch->mappedCount);
						}
						unMapChunk(adapter, ch);
					}
					vkFreeMemory(adapter->device, ch->bufferMemory, adapter->allocator);
					bkpDestroyMutex(&ch->mutex);
					bkpFree(ch);
				}
				bkpArrayDestroy(&arr->groups[j].a_chunks);
				bkpListDestroy(&arr->groups[j].l_freeChunks);

				if(arr->groups[j].mutex.initialized)
				{
					bkpDestroyMutex(&arr->groups[j].mutex);
				}
			}
			bkpFree(vma->a_chunkImageGroups[i]);
		}
		bkpArrayDestroy(&vma->a_chunkImageGroups);
	}
}

/*____________________________________________________________________________________*/
static void uploadData_bkp(BkpGpuAdapter adapter, BkpBuffer buffer, void * data, size_t offset, size_t data_size)
{
	if(buffer->image != VK_NULL_HANDLE)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "bkpUploadBufferData called on an image buffer — use bkpCreateTexture* instead");
		return;
	}
	if(buffer->gpuOnly == BKP_TRUE)
	{
		struct SChunkGroup * chunkGroup = buffer->chunk->pChunkgroup;
		uploadDataStagging(adapter, chunkGroup, buffer, data, offset, data_size);
	}
	else if(buffer->type == eBUFFER_CPU_GPU)
	{
		int8_t * p = (int8_t *)buffer->mappedData + offset;
		memcpy(p, data, data_size);
	}
}

void bkpUploadBufferData(BkpGpuAdapter adapter, BkpBuffer buffer, void * data, size_t offset, size_t data_size)
{
	adapter->vmaVtable->uploadData(adapter, buffer, data, offset, data_size);
}

/*____________________________________________________________________________________*/
static void uploadDataStagging(BkpGpuAdapter adapter, struct SChunkGroup * grp, BkpBuffer buffer, void * data, size_t offset, size_t data_size)
{
	BkpVkMemoryAllocator vma = adapter->vma_;
	SChunk ** it0 = (SChunk **)bkpArrayFind(grp->a_chunks, buffer->chunk, bkpContainerFindPtr);
	if(it0 == NULL)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Attempting to upload data on unfound chunk");
		return;
	}

	BkpBuffer stagging = getAvailableStagging(adapter, vma);
	if(stagging->size < data_size)
	{
		LOGC(eDEBUG, bkpTAG, bkpColor, "Stagging buffer update size");
		bkpFreeBuffer(adapter, &stagging);

		BkpBufferInfo info = {};
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		info.type = eBUFFER_CPU_GPU;
		info.isImage = BKP_FALSE;
		info.size = data_size;
		bkpAllocateBuffer(adapter, &stagging, &info);
		bkpMapBuffer(adapter, stagging);
	}

	SChunk * chk = *it0;

	//LOGC(eDEBUG, bkpTAG, bkpColor, "uploading %u at offset %u", data_size,buffer->offset + offset);
	bkpLockMutex(&chk->mutex);
	memcpy(stagging->mappedData, data, data_size);
	bkpCopyBuffer(adapter, chk->buffer, buffer->offset + offset, stagging->chunk->buffer, 0, data_size);
	bkpUnlockMutex(&chk->mutex);

	bkpLockMutex(&vma->stagMutex);
	bkpStackPush(&vma->staggings, stagging);
	bkpUnlockMutex(&vma->stagMutex);
}

/*____________________________________________________________________________________*/
static void mapChunk(BkpGpuAdapter adapter, SChunk * chk, VkMemoryPropertyFlags propFlags)
{
	bkpLockMutex(&chk->mutex);
	vkMapMemory(adapter->device, chk->bufferMemory, 0, chk->size, 0, &chk->mappedData);
	if((propFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange mrange = {};
		mrange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mrange.memory = chk->bufferMemory;
		mrange.offset = 0;
		mrange.size = chk->size;
		vkFlushMappedMemoryRanges(adapter->device, 1, &mrange);
	}
	chk->mappedCount = 0;
	chk->mapped = BKP_TRUE;
	bkpUnlockMutex(&chk->mutex);
}

/*____________________________________________________________________________________*/
static void unMapChunk(BkpGpuAdapter adapter, SChunk * chk)
{
	bkpLockMutex(&chk->mutex);
	vkUnmapMemory(adapter->device, chk->bufferMemory);
	chk->mapped = BKP_FALSE;
	bkpUnlockMutex(&chk->mutex);
}

/*____________________________________________________________________________________*/
static void mapBuffer_bkp(BkpGpuAdapter adapter, BkpBuffer buff)
{
	(void)adapter;
	if(buff->mapped || buff->type == eBUFFER_GPU) return;
	char * p = (char *)buff->chunk->mappedData + buff->offset;
	buff->mappedData = (void *)p;
	bkpLockMutex(&buff->chunk->mutex);
	++buff->chunk->mappedCount;
	bkpUnlockMutex(&buff->chunk->mutex);
	buff->mapped = BKP_TRUE;
}

static void unmapBuffer_bkp(BkpGpuAdapter adapter, BkpBuffer buff)
{
	(void)adapter;
	if(!buff->mapped) return;
	buff->mappedData = NULL;
	buff->mapped = BKP_FALSE;
	bkpLockMutex(&buff->chunk->mutex);
	--buff->chunk->mappedCount;
	bkpUnlockMutex(&buff->chunk->mutex);
}

/*____________________________________________________________________________________*/
void bkpMapBuffer(BkpGpuAdapter adapter, BkpBuffer buff)
{
	adapter->vmaVtable->mapBuffer(adapter, buff);
}

/*____________________________________________________________________________________*/
void bkpUnmapBuffer(BkpGpuAdapter adapter, BkpBuffer buff)
{
	adapter->vmaVtable->unmapBuffer(adapter, buff);
}

/*____________________________________________________________________________________*/
void bkpGetBuffer(BkpBuffer buffer, VkBuffer * b)
{
	*b = buffer->chunk ? buffer->chunk->buffer : buffer->directBuffer;
}

/*____________________________________________________________________________________*/
void bkpGetBufferImage(BkpBuffer buffer, VkImage* b)
{
	*b = buffer->image;
}

/*____________________________________________________________________________________*/
void bkpGetBufferOffset(BkpBuffer buffer, VkDeviceSize * s)
{
	*s = buffer->offset;
}

/*____________________________________________________________________________________*/
void bkpGetBufferSize(BkpBuffer buffer, VkDeviceSize * s)
{
	*s = buffer->size;
}

/*____________________________________________________________________________________*/
void bkpGenerateChunk(BkpGpuAdapter adapter, VkBufferUsageFlags bufferUsageFlagBits, EBufferType type, VkDeviceSize size)
{
}

/*____________________________________________________________________________________*/
static void defragmentChunk(BkpGpuAdapter adapter, SChunk * chunk, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext                 = NULL;
	bufferInfo.flags                 = 0;
	bufferInfo.size                  = chunk->allocatedSize;
	bufferInfo.usage                 = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;

	bufferInfo.queueFamilyIndexCount = 1;
    bufferInfo.pQueueFamilyIndices   = &adapter->transfertQueue.index;
	VkBuffer tmpBuffer;
	bkpCreateVkBuffer(adapter, bufferInfo, &tmpBuffer);
	if (adapter->allocCount > adapter->vma_->allocWarning)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "Memory allocation > %.2f!", WARNING_ALLOC_PERCENT * 100);
	}
	vkBindBufferMemory(adapter->device, tmpBuffer, chunk->bufferMemory, 0);
	deframent(adapter, chunk, tmpBuffer);
	vkDestroyBuffer(adapter->device, tmpBuffer, adapter->allocator);
}

/*____________________________________________________________________________________*/
static void deframent(BkpGpuAdapter adapter, SChunk* chunk, VkBuffer tmp)
{
	struct SBlock * pBlock = chunk->blocks;

	while(pBlock && pBlock->used) pBlock = pBlock->next;

	if(chunk->blocks == pBlock && !pBlock->used)
	{
		chunk->blocks = pBlock->next;
	}

	while(pBlock)
	{

		struct SBlock * qBlock = pBlock->next;
		if(!qBlock)
		{
			break;
		}

		while(qBlock->next && qBlock->next->used)
		{
			qBlock = qBlock->next;
		}


		struct SBlock * blk = pBlock->next;

		BkpBuffer buff0 = (BkpBuffer)((uint8_t *)pBlock + offsetof(struct SBlock, payload));
		BkpBuffer buff2 = (BkpBuffer)(((uint8_t *)blk) + offsetof(struct SBlock, payload));
		moveBufferDataLeft(adapter, chunk->buffer , buff0->offset, tmp, buff2->offset, buff2->size);
		buff2->offset = buff0->offset;

 		while(blk != qBlock)
		{
			blk = blk->next;
			BkpBuffer buff1 = (BkpBuffer)(((uint8_t *)blk->previous) + offsetof(struct SBlock, payload));
			VkDeviceSize offset = buff1->offset + buff1->size;
			buff2 = (BkpBuffer)((uint8_t *)blk + offsetof(struct SBlock, payload));
			moveBufferDataLeft(adapter, chunk->buffer , offset, tmp, buff2->offset, buff2->size);
			buff2->offset = offset;
		}

		buff2 = (BkpBuffer)((uint8_t *)qBlock + offsetof(struct SBlock, payload));
		buff0->offset = buff2->offset + buff2->size;
		pBlock->next->previous = pBlock->previous;
		if(pBlock->previous)
		{
			pBlock->previous->next = pBlock->next;
		}
		pBlock->previous = qBlock;
		pBlock->next = qBlock->next;
		qBlock->next = pBlock;
		if(pBlock->next)
		{
			struct SBlock * p = pBlock->next;
			if(p->used)
			{
				LOGC(eFATAL, bkpTAG, bkpColor, "Chunk corrupted supposed to be free\n");
			}
			buff2 = (BkpBuffer)((uint8_t *)p + offsetof(struct SBlock, payload));
			buff0->size += buff2->size;
			pBlock->next = p->next;
			if(p->next)
			{
				p->next->previous = pBlock;
			}
			removeFromFreeList(chunk, p);
			if(p == chunk->blocksReverse)
			{
				chunk->blocksReverse = pBlock;
			}
			bkpFree(p);
		}
	}
}

/*____________________________________________________________________________________*/
static void moveBufferDataLeft(BkpGpuAdapter adapter, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkBuffer srcBuffer, VkDeviceSize srcOffset, VkDeviceSize size)
{
	if(srcOffset < dstOffset + size)
	{
		VkDeviceSize s = srcOffset - dstOffset;
		BkpVkMemoryAllocator vma = adapter->vma_;
		BkpBuffer stagging = getAvailableStagging(adapter, vma);
		if(stagging->size > 0 && stagging->size >= EVALUTATION_OPTIMAL_FACTOR * s)
		{
			VkBuffer tmp = stagging->chunk->buffer;
			LOGC(eWARNING, bkpTAG, bkpColor, "Using stagging memeory for defrag");
			s = stagging->size;
			while(size)
			{
				bkpCopyBuffer(adapter, tmp, 0, srcBuffer, srcOffset, s);
				bkpCopyBuffer(adapter, dstBuffer, dstOffset, tmp, 0, s);
				dstOffset += s;
				srcOffset += s;
				size -= s;
				if(size < s)  s = size;
			}
		}
		else
		{
			LOGC(eWARNING, bkpTAG, bkpColor, "Using same memory memeory  multi for defrag");
			while(size)
			{
				bkpCopyBuffer(adapter, dstBuffer, dstOffset, srcBuffer, srcOffset, s);
				dstOffset += s;
				srcOffset += s;
				size -= s;
				if(size < s)  s = size;
			}
		}
		bkpLockMutex(&vma->stagMutex);
		bkpStackPush(&vma->staggings, stagging);
		bkpUnlockMutex(&vma->stagMutex);
	}
	else
	{
		LOGC(eDEBUG, bkpTAG, bkpColor, "Using same memory Chunk for defrag");
		bkpCopyBuffer(adapter, dstBuffer, dstOffset, srcBuffer, srcOffset, size);
	}
}

static BkpBuffer getAvailableStagging(BkpGpuAdapter adapter, BkpVkMemoryAllocator vma)
{
	while (1)
	{
		bkpLockMutex(&vma->stagMutex);
		size_t s = bkpStackSize(vma->staggings);
		uint32_t avail = vma->availableStagging;
		if (s > 0)
		{
			BkpBuffer buff;
			bkpStackPop(&vma->staggings, &buff);
			bkpUnlockMutex(&vma->stagMutex);
			//LOGC(eDEBUG, bkpTAG, bkpColor, "Getting stagging (%u) remains, can still create : %u", s - 1, avail);
			return buff;

		}

		if (avail > 0)
		{
			--vma->availableStagging;
		}
		bkpUnlockMutex(&vma->stagMutex);

		if (avail > 0)
		{
			BkpBuffer stagging;
			BkpBufferInfo info = {};
			info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			info.type = eBUFFER_CPU_GPU;
			info.size = SIZE_100_KIB;
			info.isImage = BKP_FALSE;
			bkpAllocateBuffer(adapter, &stagging, &info);
			bkpMapBuffer(adapter, stagging);

			return stagging;
		}
		//should wait a little bit;
	}

}

/*____________________________________________________________________________________*/
static void addToFreeList(SChunk * chunk, struct SBlock * block)
{
	block->previous_free = NULL;
	block->next_free = chunk->firstFree;
	if(chunk->firstFree != NULL)
	{
		chunk->firstFree->previous_free = block;
	}
	chunk->firstFree = block;
}

/*____________________________________________________________________________________*/
static void removeFromFreeList(SChunk * chunk, struct SBlock *block)
{
	struct SBlock * p = chunk->firstFree;
	while(p != NULL)
	{
	//	fprintf(stderr, "\t %p vs %p\n", p, block);
		if(p == block)
		{
			if(p->previous_free != NULL)
			{
				p->previous_free->next_free = p->next_free;
			}

			if(p->next_free != NULL)
			{
				p->next_free->previous_free = p->previous_free;
			}

			if(chunk->firstFree == block)
			{
				chunk->firstFree = block->next_free ? block->next_free : NULL;
			}

			block->previous_free = block->next_free = NULL;
			return;
		}
		p = p->next_free;
	}
}

/*____________________________________________________________________________________*/
static void print_Chunk(SChunk * chunk)
{
	fprintf(stderr, "Chunk :\n\tAllocated:%llu, size: %llu, firstfree :%p\n\tBlocks :\n", (unsigned long long)chunk->allocatedSize, (unsigned long long)chunk->size, chunk->firstFree);
	for(struct SBlock * p = chunk->blocks;  p != NULL; p = p->next)
	{
		BkpBuffer b = (BkpBuffer)((uint8_t *)p + offsetof(struct SBlock, payload));
		fprintf(stderr, "\t\t%p : used %d, size : %llu, offset : %llu\n", p, p->used, (unsigned long long)b->size, (unsigned long long)b->offset);
	}

	fprintf(stderr, "\n\tFrees :\n");
	for(struct SBlock * p = chunk->firstFree;  p != NULL; p = p->next_free)
	{
		fprintf(stderr, "\t\t%p <=> %p <=> %p\n", p->previous_free, p, p->next_free);
	}
}

/*____________________________________________________________________________________*/
VkDescriptorBufferInfo bkpMakeUboInfo(BkpBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
{
	VkBuffer vkbuf = buffer->chunk ? buffer->chunk->buffer : buffer->directBuffer;
	return  (VkDescriptorBufferInfo){vkbuf, buffer->offset + offset * size, size};
}

/*____________________________________________________________________________________*/
void bkpPrintBufferChunk(BkpBuffer buffer)
{
	print_Chunk(buffer->chunk);
}


#ifdef __cplusplus
}
#endif


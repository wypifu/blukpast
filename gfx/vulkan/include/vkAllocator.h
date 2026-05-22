#ifndef BKP_BUFFERS_H_
#define BKP_BUFFERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vk_types.h"
#include "vk_buffer.h"

/*********************************************************************
 * Defines
 *********************************************************************/

#define SIZE_1_KIB   1024
#define SIZE_10_KIB  10240
#define SIZE_100_KIB 102400
#define SIZE_1_MIB   1048576
#define SIZE_10_MIB  10485760
#define SIZE_100_MIB 104857600
#define SIZE_1_GIB   1073741824
#define BKP_VMA_CHUNK_DEFAULT_SIZE SIZE_100_MIB //100MB
#define WARNING_ALLOC_PERCENT 0.8f

/*********************************************************************
 * Type def & enum
 *********************************************************************/

/*********************************************************************
 * Macro
 *********************************************************************/

/*********************************************************************
 * Struct
 *********************************************************************/
 typedef struct
 {
	BkpBool used;
	BkpBool gpuOnly;
	VkMemoryPropertyFlags properties;
 }BkpBufferTypeInfoCustom;

typedef struct
{
	BkpBufferTypeInfoCustom customType;
	VkBufferUsageFlags usage;
	EBufferType type;
	uint32_t queueIndices[16];
	uint32_t qCount;
	size_t size;
	BkpBool isImage;
}BkpBufferInfo;

typedef struct BkpVmaVtable_
{
	void (*allocBuffer)(BkpGpuAdapter, BkpBuffer *, BkpBufferInfo *);
	void (*allocImage) (BkpGpuAdapter, VkImageCreateInfo *, BkpBuffer *, BkpBufferInfo *);
	void (*freeBuffer) (BkpGpuAdapter, BkpBuffer *);
	void (*freeImage)  (BkpGpuAdapter, BkpBuffer *);
	void (*mapBuffer)  (BkpGpuAdapter, BkpBuffer);
	void (*unmapBuffer)(BkpGpuAdapter, BkpBuffer);
	void (*uploadData) (BkpGpuAdapter, BkpBuffer, void *, size_t, size_t);
} BkpVmaVtable;

typedef struct
{
	VkDeviceSize chunkSize;
	VkDeviceSize chunkImageSize;
	uint16_t maxStaggingBuffers;
}BkpAllocatorGPUInfo;

/*********************************************************************
 * Global
 *********************************************************************/

/*********************************************************************
 * Struct
 *********************************************************************/

/*********************************************************************
 * Functions
 *********************************************************************/

BKP_EXPORTED void bkpSetupVmaVtable(BkpGpuAdapter adapter, EBkpVmaMode mode, BkpVmaCallbacks * callbacks);
BKP_EXPORTED void bkpStartGpuMemoryAllocator(BkpGpuAdapter adapter, BkpAllocatorGPUInfo * info);
BKP_EXPORTED void bkpShutdownGpuMemoryAllocator(BkpGpuAdapter adapter);
BKP_EXPORTED void bkpClearGpuMemoryAllocator(BkpGpuAdapter adapter);						//set chunks as free.
BKP_EXPORTED void bkpAllocateBuffer(BkpGpuAdapter adapter, BkpBuffer * destBuffer, BkpBufferInfo * info);
BKP_EXPORTED void bkpAllocateBufferImage(BkpGpuAdapter adapter, VkImageCreateInfo * imageInfo, BkpBuffer * bufferImage, BkpBufferInfo * info);
BKP_EXPORTED void bkpUploadBufferData(BkpGpuAdapter adapter, BkpBuffer buffer, void * data, size_t offset, size_t data_size);
BKP_EXPORTED void bkpMapBuffer(BkpGpuAdapter adapter, BkpBuffer buff);
BKP_EXPORTED void bkpUnmapBuffer(BkpGpuAdapter adapter, BkpBuffer buff);
BKP_EXPORTED void bkpGetBuffer(BkpBuffer buffer, VkBuffer * b);
BKP_EXPORTED void bkpGetBufferImage(BkpBuffer buffer, VkImage* b);
BKP_EXPORTED void bkpGetBufferOffset(BkpBuffer buffer, VkDeviceSize * s);
BKP_EXPORTED void bkpGetBufferSize(BkpBuffer buffer, VkDeviceSize * s);
BKP_EXPORTED VkDeviceSize bkpAlignSizeVk(VkDeviceSize size, VkDeviceSize alignment);
BKP_EXPORTED VkDeviceSize bkpGetAlignmentVk(BkpGpuAdapter adapter, VkBufferUsageFlags usage);
BKP_EXPORTED VkDeviceMemory bkpGetDeviceMemory(BkpGpuAdapter adapter, BkpBuffer buff);
BKP_EXPORTED //void bkpDefragmentChunk(BkpGpuAdapter adapter, BkpBuffer buffer);
BKP_EXPORTED void bkpFreeBuffer(BkpGpuAdapter adapter, BkpBuffer * destBuffer);
BKP_EXPORTED void bkpFreeBufferImage(BkpGpuAdapter adapter, BkpBuffer * destBuffer);
BKP_EXPORTED void bkpGenerateChunk(BkpGpuAdapter adapter, VkBufferUsageFlags bufferUsageFlagBits, EBufferType usage, VkDeviceSize size);
BKP_EXPORTED VkDescriptorBufferInfo bkpMakeUboInfo(BkpBuffer buffer, VkDeviceSize offset, VkDeviceSize size);
BKP_EXPORTED void bkpPrintBufferChunk(BkpBuffer buffer);


#ifdef __cplusplus
}
#endif

#endif


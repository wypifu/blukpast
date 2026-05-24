#ifndef VK_TYPES_H_
#define VK_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gfx/window_manager/include/windowing.h>
#ifdef __cplusplus
#include <system/include/bkp_array.h>
#else
#include <system/include/bkp_array.h>
#endif

#include <system/include/bkp_export.h>
#include <system/include/bkp_bool.h>

/*********************************************************************
 * Defines
*********************************************************************/
#define BKP_IMAGE_COUNT_DEFAULT 0
#define BKP_MAX_SAMPLER_COUNT 7   //  1 2 4 8 16 32 64 bits
#define BKP_MAX_QUEUE_COUNT 16

#define DEFAULT_BUFFER_SIZE 32
#define MAX_TEXTURE_RESOURCES 32
#define MAX_DESCRIPTOR_SET 1024
#define MAX_UBO_PER_SET 64
#define MAX_BUFFER_BINDING 64
#define MIPMAP_LOG 0
#define MAX_IMAGES_BUFFER 16  // max Image for anyframe buffer  expect swapchain one that will be defined by deviceCapability
//
#define DEFAULT_FENCE_TIMEOUT 100000000000
#define VK_FLAGS_NONE 0

/*********************************************************************
 * Type def & enum
 *********************************************************************/
typedef enum {eBKP_IMAGE_TYPE_PPM, eBKP_IMAGE_TYPE_PNG, eBKP_IMAGE_TYPE_COUNT} EImageType;

typedef enum {eQFAMILY_IGNORED, eQFAMILY_GRAPHIC, eQFAMILY_COMPUTE, eQFAMILY_TRANSFERT, eQFAMILY_PRESENT,
			  eQFAMILY_SPARSE_BINDING, eQFAMILY_PROTECTED, eQFAMILY_VIDEO_DECODE, eQFAMILY_VIDEO_ENCODE,
			  eQFAMILY_OPTICAL_FLOW, eQFAMILY_COUNT} EGpuQueue ;
typedef enum {eMIPMAP_NONE = 0, eMIPMAP_RUNTIME_GENERATE, eMIPMAP_USER_DEFINED, eMIPMAP_INFILE, eMIPMAP_COUNT} EMipMapType;
typedef enum {eSTATE_INVALID, eSTATE_READY, eSTATE_RECORDING, eSTATE_IN_PASS, eSTATE_RECORDED, eSTATE_SUBMITTED} ECmdBufferState;
typedef enum {eBUFFER_GPU, eBUFFER_CPU_GPU, eBUFFER_GPU_CPU, eBUFFER_COUNT} EBufferType ;
typedef enum {eGPUTYPE_OTHER, eGPUTYPE_INTEGRATED, eGPUTYPE_DISCRETE, eGPUTYPE_VIRTUAL, eGPUTYPE_CPU} BkpGpuDeviceType;
typedef enum {eVMA_BKP = 0, eVMA_CUSTOM = 1, eVMA_NONE = 2} EBkpVmaMode;

/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/
struct BkpBuffer_;
typedef struct BkpBuffer_ * BkpBuffer;
struct BkpBufferImage_;
typedef struct BkpBufferImage_ * BkpBufferImage;
struct BkpVkMemoryAllocator_;
typedef struct BkpVkMemoryAllocator_ * BkpVkMemoryAllocator;

/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

typedef struct
{
	void * buffer;
	size_t offset;
	size_t size;
}BkpMemInfo;

typedef struct
{
	BkpArray(VkSpecializationMapEntry) entriesMap;
	VkSpecializationInfo info;
	const void * pData;
} BkpSpecialization;

typedef struct
{
    /* user-facing: set by caller, not owned, compatible with arrays / vector.data() */
    const VkDescriptorSetLayoutBinding * bindings;
    uint32_t                             bindingCount;
    const VkDescriptorBindingFlags *     bindingFlags;    /* NULL if no variable/partially-bound bindings */
    uint32_t                             variableDescriptorMax; /* max for variable-count binding; 0 → default 1024 */
    VkDescriptorSetLayout                layout;
    /* internal: owned by bkp reflection — do not touch from user code */
    VkDescriptorSetLayoutBinding *       _info;
    VkDescriptorBindingFlags *           _bindingFlags;
} BkpDescriptorSetLayout;

typedef struct
{
    VkDeviceSize offset;
    VkDeviceSize range;
} BkpBufferBindings;

/*___________________________________________________________________*/
typedef struct
{
	BkpArray(void *) aMapped;
    void * data;
    size_t size;
} BkpData;

/*___________________________________________________________________*/
typedef struct
{
	VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize range;
} BkpUniformBufferInfo;

/*___________________________________________________________________*/
typedef struct
{
	size_t count;
	VkDeviceSize size;
	BkpBuffer buffer;
} BkpBufferGPU;

/*___________________________________________________________________*/
typedef struct
{
	VkImageLayout    imageLayout;
	VkImageView      * pImageView;
	VkSampler        * sampler;
} BkpDescriptorImage;

/*---------------------------------------------------------------------------*/
typedef struct
{
	VkFence fence;
	BkpBool signaled;
} BkpFence;

/*---------------------------------------------------------------------------*/
typedef struct
{
	EGpuQueue type;
	uint32_t index;
	float priority;
	VkQueue queue[BKP_MAX_QUEUE_COUNT];
	BkpBool isSet;
	VkQueueFamilyProperties properties;
} BkpGpuQueue;

/*---------------------------------------------------------------------------*/
typedef struct
{
	VkSurfaceCapabilitiesKHR capabilities;
	BkpArray(VkSurfaceFormatKHR) formats;
	BkpArray(VkPresentModeKHR) presentModes;
} BkpSwapChainSupportDetails;

/*---------------------------------------------------------------------------*/
typedef struct
{
	VkSwapchainKHR 		swapChain;
	BkpArray(VkImage)     images;
	BkpArray(VkImageView) imageViews;
	BkpArray(VkPresentModeKHR) presentModes;
	VkSwapchainCreateInfoKHR info;
	BkpSwapChainSupportDetails details;
	uint32_t   imageCount;
} BkpSwapChain;
/*---------------------------------------------------------------------------*/
typedef struct
{
	VkCommandPool 		commandPool;
	const void                  * pNext;
	VkCommandPoolCreateFlags    flags;
	BkpGpuQueue   * pQueue;
	BkpBool resetable;
} BkpCommandPool;

/*---------------------------------------------------------------------------*/
typedef struct
{
	BkpArray(VkCommandBuffer) cmds;

	const void *             pNext;
	BkpCommandPool * commandPool;
	VkCommandBufferLevel    level;
	uint32_t                count;
	BkpBool resetable;
	ECmdBufferState state;
} BkpCommandBuffer;

/*---------------------------------------------------------------------------*/
typedef struct
{
	VkAttachmentDescription info;
	const VkImageView *     views;
	uint32_t                viewCount;
} BkpAttachment;

/*---------------------------------------------------------------------------*/
typedef struct
{
	BkpArray(VkImage) images;
	BkpArray(VkImageView) imageViews;
	BkpArray(BkpBuffer) bufferImages;
	BkpArray(VkDeviceMemory) imageMemories;

	VkImageCreateInfo imageInfo;
	VkImageViewCreateInfo viewInfo;

	VkFilter 			 filter;
	VkFormatFeatureFlags features;
	VkMemoryPropertyFlags memoryPropertyFlags;
	VkSampler * sampler;
	VkDeviceSize alignment;
	EMipMapType mipType;
	EBufferType bufferType;
	BkpBool hasAlpha;
  VkImageLayout imageLayout;
} BkpImageResource;

typedef struct
{
    uint8_t color1[4];
    uint8_t color2[4];
    float begin;
    float end;
}BkpColorGradianInfo;

typedef struct
{
	VkDescriptorPoolCreateInfo info;
	VkDescriptorPool pool;
} BkpDescriptorPool;

typedef struct
{
	BkpDescriptorPool * descPool;
	BkpDescriptorSetLayout * descLayout;
	VkDescriptorSet * descriptorSets;
	uint32_t count;
} BkpDescriptorSet;

typedef struct
{
	VkDescriptorType type;
	union
	{
		VkDescriptorBufferInfo * ubos;
		VkDescriptorImageInfo * imgs;
	};
	uint32_t count;
} BkpDescriptorSetBinding;

typedef struct
{
	BkpDescriptorSetBinding setBindings[MAX_BUFFER_BINDING]; 
	size_t setBindingCount;
} BkpWriteDescriptorSetInfo;

/*___________________________________________________________________*/
typedef struct
{
  VkShaderStageFlagBits stage;
  uint32_t size;
  uint32_t offset;
  void * data;
}BkpPushConstant;

/*___________________________________________________________________*/
typedef struct
{
	VkVertexInputAttributeDescription * vertexAttributes;
	VkVertexInputBindingDescription * vertexBindings;
	size_t vertexAttributeCount;
	size_t vertexBindingCount;
} BkpVertexLayout;

/*---------------------------------------------------------------------------*/
typedef struct
{
	VkSampler sampler;
	VkSamplerCreateInfo info;
} BkpSampler;


/*---------------------------------------------------------------------------*/
typedef struct
{
	VkSubpassDependency * dependencies;
	VkSubpassDescription * subpasses;
	BkpAttachment * attachments;

	VkAttachmentReference * colorAttachmentRef;
	VkAttachmentReference * inputAttachmentRef;
	VkAttachmentReference * resolvAttachmentRef;
	VkAttachmentReference  depthAttachmentRef;
	uint32_t *  preserveAttachementRef;

	uint32_t targetCount;
	uint32_t frameBufferWidth, frameBufferHeight, frameBufferLayers;
	VkFramebufferCreateFlags frameBufferFlags;

	void * frameBufferNext;
	BkpImageResource * msaa;

	size_t dependencyCount;
	size_t subpassCount;
	size_t attachmentCount;
	size_t colorAttachmentRefCount;
	size_t inputAttachmentRefCount;
	size_t preserveAttachementRefCount;

	BkpBool hasDepth;
} BkpRenderPassInfo;

/*---------------------------------------------------------------------------*/
typedef struct
{
	BkpRenderPassInfo info;
	VkRenderPass renderPass;
	BkpArray(VkFramebuffer) frameBuffers;
	BkpArray(VkImageView) views;
	const VkClearValue * clearValues;
	uint32_t             clearValueCount;
	VkRenderPassBeginInfo beginInfo;
	VkSubpassContents passContents;
	VkOffset2D	offset;
	VkExtent2D	extent;

	VkRenderPassCreateFlags flags;
	void * pNext;
	BkpBool hasFrameBuffers;
} BkpRenderTarget;

/*---------------------------------------------------------------------------*/
typedef struct
{
	void * userData;
	BkpBool (*allocBuffer)(void * userData, VkDevice device, VkPhysicalDevice physDev,
	                        const VkBufferCreateInfo * info, EBufferType memType,
	                        VkBuffer * outBuffer, VkDeviceMemory * outMemory, void ** outAlloc);
	void    (*freeBuffer) (void * userData, VkBuffer buffer, VkDeviceMemory memory, void * alloc);
	void *  (*mapMemory)  (void * userData, void * alloc);
	void    (*unmapMemory)(void * userData, void * alloc);
	BkpBool (*allocImage) (void * userData, VkDevice device, VkPhysicalDevice physDev,
	                        const VkImageCreateInfo * info, EBufferType memType,
	                        VkImage * outImage, VkDeviceMemory * outMemory, void ** outAlloc);
	void    (*freeImage)  (void * userData, VkImage image, VkDeviceMemory memory, void * alloc);
} BkpVmaCallbacks;

/*---------------------------------------------------------------------------*/
/**
 * @brief Configuration passed to @ref bkpInitVulkanContext (and indirectly to @ref bkpInit).
 *
 * Zero-initialise with @c {0}, then override only the fields you need.
 * Any field left at 0 / @c BKP_FALSE / @c NULL falls back to a built-in default.
 * Use @ref bkpDefaultConfig to obtain a ready-to-use @ref BkpConfig (which embeds
 * this struct) with all defaults pre-filled.
 */
typedef struct
{
	const char * appName;     /**< Application name embedded in the Vulkan instance info.  NULL → "BKP Application". */
	BkpWindow  * win;         /**< Existing window to attach to.  NULL → bkp creates one sized @p winWidth × @p winHeight. */
	EWindowType  eWinType;    /**< Window-system backend (e.g. @c eWIN_GLFW).  Ignored when @p win is provided. */
	int          winWidth;    /**< Initial window width in pixels.   0 → 1920. */
	int          winHeight;   /**< Initial window height in pixels.  0 → 1080. */

	uint8_t maxFrameInFlight;   /**< Pipeline depth: 1 = no overlap, 2 = double-buffer (typical), 3 = triple.  0 → 2. */
	BkpBool forceIntegratedGPU; /**< @c BKP_TRUE: skip dedicated GPUs and select the integrated adapter. */
	BkpBool headless;           /**< @c BKP_TRUE: no window / no surface — off-screen rendering only. */
	BkpBool fullScreen;         /**< @c BKP_TRUE: open the window in fullscreen mode. */
	BkpBool printVulkanInfo;    /**< @c BKP_TRUE: log detailed device/extension info at startup. */

	EBkpVmaMode     vmaMode;      /**< Memory-allocator back-end (bkp built-in, Vulkan Memory Allocator, custom vtable). */
	BkpVmaCallbacks vmaCallbacks; /**< Custom allocator vtable; only used when @p vmaMode == @c eVMA_CUSTOM. */

	/**
	 * @brief Maximum staging-memory budget per texture-upload batch, in bytes  (mode A).
	 *
	 * @ref bkpUploadTextureBatch splits large uploads into sub-batches so that at most
	 * @p stagingBudgetBytes of CPU-visible staging memory is live simultaneously.
	 * This prevents OOM on APUs (shared VRAM) and caps peak RAM pressure on
	 * dedicated GPUs when loading models with many high-resolution textures.
	 *
	 * - @c 0 → built-in default (256 MiB).
	 * - Ignored when @p dynamicStagingBudget is @c BKP_TRUE.
	 *
	 * @note Set before calling @ref bkpInitVulkanContext (or @ref bkpInit).
	 */
	size_t stagingBudgetBytes;

	/**
	 * @brief Enable runtime-adaptive staging budget via @c VK_EXT_memory_budget  (mode C).
	 *
	 * When @c BKP_TRUE, bkp requests the @c VK_EXT_memory_budget device extension
	 * and, at each texture-upload call, queries the available CPU-visible heap to
	 * derive the per-batch budget (¼ of the currently free space).  This adapts
	 * automatically to real-time memory pressure and is the recommended mode for APUs
	 * or any configuration where VRAM is shared with system RAM.
	 *
	 * If @c VK_EXT_memory_budget is absent on the selected device, bkp logs a warning
	 * and silently falls back to the static @p stagingBudgetBytes budget (mode A).
	 *
	 * @note Must be set **before** @ref bkpInitVulkanContext (or @ref bkpInit) so
	 * that the extension is enabled during logical-device creation.
	 */
	BkpBool dynamicStagingBudget;

} BkpVulkanContextInfo;


typedef struct
{
	float deltaTime, lastTime;
	float ratio;
	uint32_t currentFrame,
			 imageAcquired,
			 imageCount,
			 frameCount;
	int winWidth;
	int winHeight;
	uint8_t maxFrameInFlight;
} BkpFrameInfo;

/*---------------------------------------------------------------------------*/
struct BkpVmaVtable_;

struct SAdapter
{
	BkpVkMemoryAllocator   vma_;
	struct BkpVmaVtable_ * vmaVtable;
	BkpVmaCallbacks        vmaCallbacks;
	VkPhysicalDevice gpu;
	VkDevice device;
	BkpArray(VkSemaphore)  semaphoreRenderReady;
	BkpArray(VkSemaphore)  semaphorePresentReady;
	BkpArray(BkpFence) 	   inFlightFences;
	BkpArray(BkpFence) 	   imagesInFlight;

	VkSurfaceKHR surface;
	VkDeviceSize maxMemoryAllocationSize;
	VkDeviceSize maxPerSetDescriptors;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemory;

	BkpSwapChain * swapChain;
	BkpGpuQueue graphicQueue;
	BkpGpuQueue computeQueue;
	BkpGpuQueue transfertQueue;
	BkpGpuQueue presentQueue;
	BkpCommandPool commandPoolTransfer, commandPoolGraphic;
	VkAllocationCallbacks * allocator;

	VkSampleCountFlagBits samplerCountBit[BKP_MAX_SAMPLER_COUNT];
	VkMemoryPropertyFlagBits uboVisibleLocal;
	BkpFrameInfo frameInfo;
	char szName[32];
	size_t memoryGroupId;
	size_t allocCount;
	uint32_t score;
	uint32_t imageCount;
	uint16_t sampleIndex;
	float warningAllocPersent;
	BkpBool frameBufferResized;
	BkpBool enabled;
	BkpBool isDiscret;
	BkpBool isRenderSyncInit;
} ;

typedef struct SAdapter * BkpGpuAdapter;

/*---------------------------------------------------------------------------*/
/** Input/output pair for requesting optional Vulkan device features.
 *  Set fields in `requested` to VK_TRUE before calling bkpActivateGpuAdapter.
 *  BKP writes the result into `activated` — VK_FALSE means the GPU does not
 *  support the feature; a warning is logged for each unsupported request. */
typedef struct
{
    VkPhysicalDeviceFeatures requested;
    VkPhysicalDeviceFeatures activated;
} BkpFeatureHint;

/*---------------------------------------------------------------------------*/
typedef struct
{
	size_t           index;
	BkpGpuDeviceType deviceType;
	BkpFeatureHint * hint;   /**< NULL = BKP defaults only (fillModeNonSolid, samplerAnisotropy) */
} BkpAdapterCriteria;

/*---------------------------------------------------------------------------*/
typedef struct
{
	BkpArray(VkPipelineStageFlags) waitStages;
	BkpArray(VkSemaphore) waitSemaphores;
	BkpArray(VkSemaphore) signalSemaphores;
}BkpWaitInfo;

/*---------------------------------------------------------------------------*/
typedef struct
{
	VkImage               image;
	VkImageLayout         oldLayout;
	VkImageLayout         newLayout;
	VkPipelineStageFlags2 srcStage;
	VkAccessFlags2        srcAccess;
	VkPipelineStageFlags2 dstStage;
	VkAccessFlags2        dstAccess;
	VkImageAspectFlags    aspect;
	uint32_t              baseMip;
	uint32_t              mipCount;
	uint32_t              baseLayer;
	uint32_t              layerCount;
} BkpImageBarrierInfo;

/*---------------------------------------------------------------------------*/
typedef struct
{
	BkpWindow * win;

	VkInstance instance;
	VkSurfaceKHR surface;
	BkpArray(VkExtensionProperties) vkExtprop;
	VkDebugUtilsMessengerEXT debugMessenger;
	BkpArray(struct SAdapter) adapters;
	BkpArray(const char *) szDeviceExt;
	BkpArray(const char *) vkExt;
	const char * appName;

	size_t defaultAdapterIndex;

	BkpVulkanContextInfo info;
	VkAllocationCallbacks * allocator;
	size_t memoryGroupId;
} BkpVulkanContext;

/*********************************************************************
 * Functions
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif


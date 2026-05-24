#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "include/vulkan.h"
#include "include/vk_cmd.h"
#include "include/vkAllocator.h"
#include "include/vk_info.h"

#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb/stb_image.h>
#include <system/include/bkp_allocator.h>
#include <system/include/bkp_log.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <thirdparty/stb/stb_image_write.h>

/**************************************************************************
 *  Defines & Maro
 **************************************************************************/

#ifdef DEBUG
static const BkpBool bValidation_ = BKP_TRUE;
#else
static const BkpBool bValidation_ = BKP_FALSE;
#endif

const BkpBool STAGGING = BKP_TRUE;
const BkpBool NO_STAGGING = BKP_FALSE;

#define MIN_ALLOC_GROUP_GFX 5
#define WINWIDTH 1024
#define WINHEIGHT 768
#define MAX_QUEUE 8
#define BOOL_TO_STRING(val) (val) ? "True" : "False"

static const char * gAppName = "Blukpast Application";

static const char * bkpColorVulkanWarning = "\x1b[2;38;2;240;140;0;48;2;44;44;20m";
static const char * bkpColorVulkanError = "\x1b[2;38;2;140;0;0;48;2;120;120;120m";
static const char * bkpColorVulkanCrash = "\x1b[2;38;2;160;160;160;48;2;110;10;10m";
static const char * bkpColor = "\x1b[0;38;5;75m";
static const char * bkpTAG = "GFX";

/**************************************************************************
 *  Structs, Enum, Unio and Typesdef
 **************************************************************************/

/**************************************************************************
 *  Globals
 **************************************************************************/
static size_t gVULKAN_MEMORY_PAGE_SIZE  = SIZE_1_MIB;
static size_t gVULKAN_ADAPTER_PAGE_SIZE = SIZE_100_MIB * 2.56f ;

/* ---- staging budget (read by vk_texture_batch.c) ---- */
static size_t  gSTAGING_BUDGET_BYTES = SIZE_1_MIB * 256; /* mode A default: 256 MiB */
static BkpBool gDYNAMIC_STAGING      = BKP_FALSE;        /* mode C: use VK_EXT_memory_budget */

static const char * gTszValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};
static const int32_t gValidationLayersCount = sizeof(gTszValidationLayers) / sizeof(gTszValidationLayers[0]);
static uint16_t gVersMajor = 0, gVersMinor = 3, gVersCorr = 1;
static uint16_t stc_vMinor = 0, stc_vMajor = 1;

void (*pBkpOnResize)(BkpGpuAdapter context) = NULL;

const char * gDeviceType[5] = {"Other", "Integrated", "Discrete", "Virtual", "CPU"};

/***************************************************************************
 *  Prototypes
 **************************************************************************/

static void createStaggingImageBuffer(BkpGpuAdapter adapter, stbi_uc ** bitmaps, size_t bitmapCount, VkDeviceSize layerSize, BkpBuffer * stagging, VkDeviceSize size);
static void createInstance(BkpVulkanContext * ctx);
static void setExtensionProperties(BkpVulkanContext * ctx);
static BkpBool checkValidationLayerSupport(int32_t vMajor, int32_t vMinor, size_t groupId);
static void getRequiredExtensions(BkpArray(const char *) *ptr, size_t memoryGroupId);
static void vulkan_LogExtension(BkpVulkanContext * ctx);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagBitsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void * pUserData);
static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT * createinfo);
static void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT * debugMessenger, VkAllocationCallbacks * allocator);
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT * debugMessenger, const VkAllocationCallbacks* pAllocator);
static void getSamplerList(BkpGpuAdapter adapter);
static void selectQueue(uint8_t family, BkpGpuQueue * queue, BkpGpuAdapter adapter, VkQueueFamilyProperties * qList, size_t listCount,size_t * vQ);

static void createSurface(BkpVulkanContext * context);

static void pickPhysicalDevice(BkpVulkanContext * ctx);
static uint32_t isDeviceSuitable(BkpVulkanContext * ctx, BkpGpuAdapter adapter, VkSurfaceKHR surface);
static BkpBool checkDeviceExtensionsSupported(BkpVulkanContext * ctx, VkPhysicalDevice gpu);
static BkpArray(char *) getRequiredDeviceExtensions(VkPhysicalDevice gpu, size_t memoryGroupId);

static void createLogicalDevice(BkpVulkanContext * ctx, BkpGpuAdapter adapter, BkpAdapterCriteria * criteria);
static void findQueueFamilies(BkpGpuAdapter adapter);

static void querySwapChainSupport(BkpGpuAdapter adapter, BkpSwapChain * sc, VkSurfaceKHR surface);
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR * list, size_t size);
static VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR * list, size_t listCount, const VkPresentModeKHR * prefered, size_t preferedCount);
static VkExtent2D chooseSwapExtent2D(const VkSurfaceCapabilitiesKHR capabilities);

static void createImageViews(BkpGpuAdapter adapter, BkpSwapChain * swapChain);
static VkImageView createImageView(VkDevice device, VkImage image, VkImageViewCreateInfo * info, VkAllocationCallbacks * allocator);

static VkFormat findDepthFormat(VkPhysicalDevice gpu, VkImageTiling tiling, VkFormatFeatureFlags features);
static uint8_t savePPM(const char * path, char * data, int width , int height, BkpBool colorSwizzle, VkDeviceSize size);
static uint8_t savePNG(const char * path, char * data, int width , int height, BkpBool colorSwizzle, VkDeviceSize size);
static uint8_t saveImageToFile(BkpGpuAdapter adp, VkImage srcImage, VkFormat format, int width, int height, VkImageLayout imageLayout, VkFormatFeatureFlags tiling, const char * path, VkImageAspectFlags aspect, EImageType fileType);


/**************************************************************************
 *  Implementations
 **************************************************************************/

/*____________________________________________________________________________________*/

/*____________________________________________________________________________________*/


void bkpSetVulkanMemoryPage(size_t pageSize)
{
	gVULKAN_MEMORY_PAGE_SIZE = pageSize;
}

/*____________________________________________________________________________________*/
void bkpSetVulkanAdapterMemoryPage(size_t pageSize)
{
	gVULKAN_ADAPTER_PAGE_SIZE = pageSize;
}

/*____________________________________________________________________________________*/
/**
 * @brief Resolve the effective staging-memory budget for the current upload.
 *
 * Internal function called by @c bkpUploadTextureBatch.
 * - Mode A (@c gDYNAMIC_STAGING == BKP_FALSE): returns the static @c gSTAGING_BUDGET_BYTES.
 * - Mode C (@c gDYNAMIC_STAGING == BKP_TRUE): queries @c VK_EXT_memory_budget and returns
 *   ¼ of the largest available CPU-visible heap.  Falls back to the static budget when
 *   the reported available space is 0.
 */
void bkpSetStagingBudget(size_t bytes)
{
	gSTAGING_BUDGET_BYTES = bytes ? bytes : SIZE_1_MIB * 256;
}

/*____________________________________________________________________________________*/
void bkpSetDynamicStagingBudget(BkpBool enabled)
{
	gDYNAMIC_STAGING = enabled;
}

/*____________________________________________________________________________________*/
size_t bkpResolveStagingBudget(BkpGpuAdapter adapter)
{
	if(!gDYNAMIC_STAGING)
	{
		LOGC(eDEBUG, bkpTAG, bkpColor,
		     "Staging budget: static %zu MiB", gSTAGING_BUDGET_BYTES / (1024u * 1024u));
		return gSTAGING_BUDGET_BYTES;
	}

	VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {0};
	budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

	VkPhysicalDeviceMemoryProperties2 props = {0};
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	props.pNext = &budget;

	vkGetPhysicalDeviceMemoryProperties2(adapter->gpu, &props);

	VkDeviceSize best = 0;
	for(uint32_t i = 0; i < props.memoryProperties.memoryHeapCount; ++i)
	{
		if(props.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			continue; /* skip device-local heaps; we want CPU-visible staging memory */
		VkDeviceSize avail = budget.heapBudget[i] > budget.heapUsage[i]
		                   ? budget.heapBudget[i] - budget.heapUsage[i]
		                   : 0;
		if(avail > best)
		{
			best = avail;
		}
	}

	if(best == 0)
	{
		LOGC(eWARNING, bkpTAG, bkpColor,
		     "Dynamic staging budget query returned 0 — falling back to %zu MiB static budget",
		     gSTAGING_BUDGET_BYTES / (1024u * 1024u));
		return gSTAGING_BUDGET_BYTES;
	}

	LOGC(eDEBUG, bkpTAG, bkpColor,
	     "Staging budget: dynamic best=%zu MiB → using %zu MiB (1/4)",
	     (size_t)(best / (1024u * 1024u)), (size_t)(best / 4 / (1024u * 1024u)));
	return (size_t)(best / 4);
}

/*____________________________________________________________________________________*/
BkpBool bkpInitVulkanContext(BkpVulkanContextInfo * info, BkpVulkanContext * ctx)
{
	if(bkpGetMemoryGroupCount() == 1)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Memory Allocator not initialized");
	}

	size_t avCount = bkpGetAvailableMemoryGroupCount();
	if(avCount < MIN_ALLOC_GROUP_GFX)
	{
		LOGC(eDEBUG, bkpTAG, bkpColor, "Bkp Graphics needs space for 5 groups %lu available", avCount);
	}

	BkpAllocationGroupInfo grp = {"bkpGFX", gVULKAN_MEMORY_PAGE_SIZE};
	ctx->memoryGroupId = bkpAddAllocatorGroup(&grp);

	ctx->info.headless = info->headless;

	if(info->appName == NULL)
	{
		info->appName = gAppName;
	}
	ctx->appName   = info->appName;


	if(info->winWidth == 0 && info->winHeight == 0)
	{
		info->winWidth  = WINWIDTH;
		info->winHeight = WINHEIGHT;
	}

	ctx->surface = VK_NULL_HANDLE;
	ctx->szDeviceExt = (const char **)bkpArrayCreateFrom(const char *, ctx->memoryGroupId);

    if(ctx->info.headless != BKP_TRUE)
	{
		// {"VK_KHR_swapchain"};
		bkpArrayPush(&ctx->szDeviceExt, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		ctx->win = info->win;
		if(info->win == NULL)
		{
			//ctx->win = createWindow(info->eWinType, EApi::eVULKAN);
			const char * appName = info->appName ? info->appName : gAppName;
			ctx->win = bkpCreateWindow(info->eWinType);
			ctx->win->window = bkpOpenWindow(info->winWidth, info->winHeight, appName, info->fullScreen);
			//ctx->win = NULL; //to create a seg fault
		}
	}

	createInstance(ctx);
	setupDebugMessenger(ctx->instance, &ctx->debugMessenger, ctx->allocator);

    if(ctx->info.headless != BKP_TRUE)
	{
		createSurface(ctx);
	}

	ctx->info = *info;

	if(info->stagingBudgetBytes)
	{
		gSTAGING_BUDGET_BYTES = info->stagingBudgetBytes;
	}
	if(info->dynamicStagingBudget)
	{
		gDYNAMIC_STAGING = BKP_TRUE;
	}

	pickPhysicalDevice(ctx);

	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
void bkpActivateGpuAdapter(BkpVulkanContext * ctx, BkpGpuAdapter * adapter_, BkpAdapterCriteria * criteria)
{
//	size_t adapterCount = bkpArraySize(ctx->adapters);

	BkpGpuAdapter adapter = &ctx->adapters[0];

	adapter->enabled = BKP_TRUE;
	adapter->frameInfo.lastTime = (float)glfwGetTime();
	adapter->frameInfo.frameCount = 0;
	adapter->frameInfo.currentFrame = 0;
	adapter->frameInfo.maxFrameInFlight = ctx->info.maxFrameInFlight;
	adapter->frameInfo.winWidth = ctx->info.winWidth;
	adapter->frameInfo.winHeight = ctx->info.winHeight;

	sprintf(adapter->szName, "adapter_%u", 0);
	BkpAllocationGroupInfo grp = {adapter->szName, (size_t)(gVULKAN_ADAPTER_PAGE_SIZE)};
	adapter->memoryGroupId = bkpAddAllocatorGroup(&grp);

	LOGC(eINFO, bkpTAG, bkpColor, "Selected - GPU %s (%u, %u) score : %u", adapter->deviceProperties.deviceName,
			VK_VERSION_MAJOR(adapter->deviceProperties.apiVersion),
			VK_VERSION_MINOR(adapter->deviceProperties.apiVersion),
			adapter->score);
	LOGC(eINFO, bkpTAG, bkpColor, "Memory both host_visible and local_bit: %s", BOOL_TO_STRING(adapter->uboVisibleLocal));

	LOGC(eINFO, bkpTAG, bkpColor, "SamplerBits:");
	for (uint16_t i = 0; i < BKP_MAX_SAMPLER_COUNT; ++i)
	{
		LOGC(eINFO, bkpTAG, bkpColor, "\t%u : %u", 1 << i, adapter->samplerCountBit[i]);
	}

	adapter->surface = ctx->surface;

	/* Mode C: try to enable VK_EXT_memory_budget for dynamic staging budget queries.
	   Must be done before createLogicalDevice so the extension is passed at device creation. */
	if(gDYNAMIC_STAGING)
	{
		BkpArray(char *) avail     = getRequiredDeviceExtensions(adapter->gpu, ctx->memoryGroupId);
		size_t           availCount = bkpArraySize(avail);
		BkpBool          found      = BKP_FALSE;

		for(size_t k = 0; k < availCount && !found; ++k)
		{
			if(strcmp(avail[k], VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0)
			{
				found = BKP_TRUE;
			}
		}
		bkpArrayDestroy(&avail);

		if(found)
		{
			bkpArrayPush(&ctx->szDeviceExt, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
			LOGC(eINFO, bkpTAG, bkpColor, "VK_EXT_memory_budget enabled — dynamic staging budget active");
		}
		else
		{
			LOGC(eWARNING, bkpTAG, bkpColor,
			     "VK_EXT_memory_budget not available — falling back to static staging budget (%zu MiB)",
			     gSTAGING_BUDGET_BYTES / (1024u * 1024u));
			gDYNAMIC_STAGING = BKP_FALSE;
		}
	}

	createLogicalDevice(ctx, adapter, criteria);

	bkpSetupVmaVtable(adapter, ctx->info.vmaMode, &ctx->info.vmaCallbacks);
	if(ctx->info.vmaMode == eVMA_BKP)
	{
		BkpAllocatorGPUInfo inf;
		inf.chunkSize = SIZE_100_MIB * 2;
		inf.chunkImageSize = SIZE_100_MIB * 2;
		inf.maxStaggingBuffers = 16;
		bkpStartGpuMemoryAllocator(adapter, &inf);
	}

	LOGC(eDEBUG, bkpTAG, bkpColor, "Creating transfer commandPool");
	adapter->commandPoolTransfer.pNext  = NULL;
	adapter->commandPoolTransfer.pQueue = &adapter->transfertQueue;
	adapter->commandPoolTransfer.flags  =  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	bkpCreateCmdPool(adapter, &adapter->commandPoolTransfer);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Creating graphic commandPool");
	adapter->commandPoolGraphic.pNext   = NULL;
	adapter->commandPoolGraphic.pQueue  = &adapter->graphicQueue;
	adapter->commandPoolGraphic.flags   =  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	bkpCreateCmdPool(adapter, &adapter->commandPoolGraphic);
	*adapter_ = adapter;

	LOGC(eINFO, bkpTAG, bkpColor, "Compute Limits: \n\tMaxComputeGroupCount (x = %u, y = %u, z = %u)\n"
		"\tMaxComputeGroupInvocations %u\n\tMaxComputeGroupSize (x = %u, y = %u, z = %u)",
		adapter->deviceProperties.limits.maxComputeWorkGroupCount[0],
		adapter->deviceProperties.limits.maxComputeWorkGroupCount[1],
		adapter->deviceProperties.limits.maxComputeWorkGroupCount[2],
		adapter->deviceProperties.limits.maxComputeWorkGroupInvocations,
		adapter->deviceProperties.limits.maxComputeWorkGroupSize[0],
		adapter->deviceProperties.limits.maxComputeWorkGroupSize[1],
		adapter->deviceProperties.limits.maxComputeWorkGroupSize[2]);
}

/*____________________________________________________________________________________*/
void bkpWaitDevice(BkpGpuAdapter adapter)
{
	vkDeviceWaitIdle(adapter->device);
}

/*____________________________________________________________________________________*/
void bkpClearGpuAdapter(BkpGpuAdapter adapter)
{
	vkDeviceWaitIdle(adapter->device);
	bkpDestroyCommandPool(adapter, &adapter->commandPoolTransfer);
	bkpDestroyCommandPool(adapter, &adapter->commandPoolGraphic);

    //if(ctx->info.noVMA != BKP_TRUE)
	bkpShutdownGpuMemoryAllocator(adapter);
	vkDestroyDevice(adapter->device, adapter->allocator);
}

/*____________________________________________________________________________________*/
void bkpClearContext(BkpVulkanContext * ctx)
{

	if(ctx->info.headless != BKP_TRUE)
	{
		vkDestroySurfaceKHR(ctx->instance, ctx->surface, ctx->allocator);
	}

	DestroyDebugUtilsMessengerEXT(ctx->instance, &ctx->debugMessenger, ctx->allocator);
	vkDestroyInstance(ctx->instance, ctx->allocator);
	bkpArrayDestroy(&ctx->szDeviceExt);
	bkpArrayDestroy(&ctx->vkExt);
	bkpArrayDestroy(&ctx->vkExtprop);
	bkpArrayDestroy(&ctx->adapters);

	LOGC(eDEBUG, bkpTAG, bkpColor, "exiting  context");
}

/*____________________________________________________________________________________*/
static void createInstance(BkpVulkanContext * ctx)
{
	VkApplicationInfo appInfo  = {};
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = ctx->appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "Blukpast";
	appInfo.engineVersion      = VK_MAKE_VERSION(gVersMajor, gVersMinor, gVersCorr);
	appInfo.apiVersion         = VK_API_VERSION_1_3;
	appInfo.pNext              = NULL;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo     = &appInfo;

	ctx->vkExtprop = (BkpArray(VkExtensionProperties))bkpArrayCreateFrom(VkExtensionProperties, ctx->memoryGroupId);
	setExtensionProperties(ctx);

	ctx->vkExt = bkpArrayCreateFrom(const char *, ctx->memoryGroupId);
	getRequiredExtensions(&ctx->vkExt, ctx->memoryGroupId);
	createInfo.enabledExtensionCount = (uint32_t)bkpArraySize(ctx->vkExt);
	createInfo.ppEnabledExtensionNames = (const char * const*)ctx->vkExt;
	vulkan_LogExtension(ctx);

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if(bValidation_)
	{
		if(BKP_FALSE == checkValidationLayerSupport(0, 0, ctx->memoryGroupId))
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Validation Layers Requested but not available!");
		}

		createInfo.enabledLayerCount = gValidationLayersCount;
		createInfo.ppEnabledLayerNames = gTszValidationLayers;

		populateDebugMessengerCreateInfo(&debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = NULL;
	}

	VkResult res = vkCreateInstance(&createInfo, ctx->allocator, &ctx->instance);
	if(res != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to create vkInstance : %s", vkResullt2String(res, 0));
	}

	LOGC(eDEBUG, bkpTAG, bkpColor, "Instance creation : %s", vkResullt2String(res, 0));
	return;
}

/*--------------------------------------------------------------------------------*/
static void setExtensionProperties(BkpVulkanContext * ctx)
{
	if(bkpArraySize(ctx->vkExtprop) > 0)
	{
		return;
	}

	uint32_t extPropCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extPropCount, NULL);
	bkpArrayResize(&ctx->vkExtprop, extPropCount);
	vkEnumerateInstanceExtensionProperties(NULL, &extPropCount, ctx->vkExtprop);

	return;
}

/*_________________________________________________________________________________*/
static BkpBool checkValidationLayerSupport(int32_t vMajor, int32_t vMinor, size_t groupId)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	BkpArray(VkLayerProperties) availablelayers = (BkpArray(VkLayerProperties)) bkpArrayCreateFrom(VkLayerProperties, groupId);
    bkpArrayResize(&availablelayers, layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availablelayers);

	BkpBool toReturn = BKP_TRUE;

	for(size_t i = 0; i < gValidationLayersCount; ++i)
	{
		size_t layerFound = 0;
		for(size_t j = 0; j < layerCount; ++j)
		{
			if(strcmp(gTszValidationLayers[i], availablelayers[j].layerName) == 0)
			{
				layerFound = 1;
				break;
			}
		}
		if(layerFound == 0)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "'%s' Not Found", gTszValidationLayers[i]);
			toReturn = BKP_FALSE;
			goto returnLine;
		}
	}

returnLine:
	bkpArrayDestroy(&availablelayers);
	return toReturn;
}

/*--------------------------------------------------------------------------------*/
static void getRequiredExtensions(BkpArray(const char *) *ptr, size_t memoryGroupId)
{
	uint32_t n_glfwExtension = 0;
	const char ** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&n_glfwExtension);
	bkpArrayReserve(ptr, n_glfwExtension);

	for(uint32_t i = 0; i < n_glfwExtension; ++i)
	{
		bkpArrayPush(ptr, glfwExtensions[i]);
    //fprintf(stderr, "%s \n", glfwExtensions[i]);
	}

	if(BKP_TRUE == bValidation_)
	{
		bkpArrayPush(ptr, "VK_EXT_debug_utils");
	}
}

/*--------------------------------------------------------------------------------*/
static void vulkan_LogExtension(BkpVulkanContext * ctx)
{
	size_t s = bkpArraySize(ctx->vkExtprop);
	if( s < 1)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "vkExtension Properties not set !");
		return;
	}

  if(ctx->info.printVulkanInfo== 1) 
  {
    LOGC(eINFO, bkpTAG, bkpColor, "Available Extensions Properties:");
    for(uint32_t i = 0; i < s; ++i)
    {
      LOGC(eINFO, bkpTAG, bkpColor, "\t#%u %s\t", i + 1, ctx->vkExtprop[i].extensionName);
    }
  }

	s = bkpArraySize(ctx->vkExt);
	if( s < 1)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "vkExtension not set !");
		return;
	}

  if(ctx->info.printVulkanInfo == 1) 
  {
    LOGC(eINFO, bkpTAG, bkpColor, "Available Extensions:");
    for(uint32_t i = 0; i < s; ++i)
    {
      LOGC(eINFO, bkpTAG, bkpColor, "\t#%u %s\t", i + 1, ctx->vkExt[i]);
    }
  }

	return;
}

/*--------------------------------------------------------------------------------*/
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagBitsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void * pUserData)
{
	if( messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		LOGC(eDEBUG, "VULKAN INFO", bkpColor, "%s", pCallbackData->pMessage);
	}
	else if(messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LOGC(eWARNING, "VULKAN WARNING", bkpColorVulkanWarning, "%s", pCallbackData->pMessage);
	}
	else if(messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LOGC(eERROR, "VULKAN ERROR", bkpColorVulkanError, "%s", pCallbackData->pMessage);
	}
	else
	{
		LOGC(ePRIORITY, "VULKAN Debug", bkpColorVulkanCrash,"%s", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

/*--------------------------------------------------------------------------------*/
static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT * createInfo)
{
	createInfo->sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo->pNext           = NULL;
	createInfo->flags           = 0;
	//createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo->messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo->pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugCallback;
	createInfo->pUserData       = NULL;

}

/*--------------------------------------------------------------------------------*/
static void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT * debugMessenger, VkAllocationCallbacks * allocator)
{
	if(BKP_FALSE == bValidation_) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(&createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, debugMessenger) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to set up debug messenger!");
	}

	return;
}

/*--------------------------------------------------------------------------------*/
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != NULL)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/*_________________________________________________________________________________*/
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT * debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	if(!bValidation_) return;

	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != NULL)
	{
		func(instance, *debugMessenger, pAllocator);
	}

	return;
}

/*_________________________________________________________________________________*/
static void getSamplerList(BkpGpuAdapter adapter)
{
	VkSampleCountFlags counts = adapter->deviceProperties.limits.framebufferColorSampleCounts & adapter->deviceProperties.limits.framebufferDepthSampleCounts;

	adapter->samplerCountBit[0] = VK_SAMPLE_COUNT_1_BIT;
	adapter->samplerCountBit[1] = (counts & VK_SAMPLE_COUNT_2_BIT)  ? VK_SAMPLE_COUNT_2_BIT :  VK_SAMPLE_COUNT_1_BIT;
	adapter->samplerCountBit[2] = (counts & VK_SAMPLE_COUNT_4_BIT)  ? VK_SAMPLE_COUNT_4_BIT :  VK_SAMPLE_COUNT_1_BIT;
	adapter->samplerCountBit[3] = (counts & VK_SAMPLE_COUNT_8_BIT)  ? VK_SAMPLE_COUNT_8_BIT :  VK_SAMPLE_COUNT_1_BIT;
	adapter->samplerCountBit[4] = (counts & VK_SAMPLE_COUNT_16_BIT) ? VK_SAMPLE_COUNT_16_BIT:  VK_SAMPLE_COUNT_1_BIT;
	adapter->samplerCountBit[5] = (counts & VK_SAMPLE_COUNT_32_BIT) ? VK_SAMPLE_COUNT_32_BIT:  VK_SAMPLE_COUNT_1_BIT;
	adapter->samplerCountBit[6] = (counts & VK_SAMPLE_COUNT_64_BIT) ? VK_SAMPLE_COUNT_64_BIT:  VK_SAMPLE_COUNT_1_BIT;
}


/*_________________________________________________________________________________*/
static void createSurface(BkpVulkanContext * ctx)
{
	//should test glfw, widows, XCB, wayland ...
	if(glfwCreateWindowSurface(ctx->instance, (GLFWwindow *) ctx->win->window, ctx->allocator, &ctx->surface) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed create window surface!");
	}
	return;
}

/*_________________________________________________________________________________*/
static void pickPhysicalDevice(BkpVulkanContext * ctx)
{
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(ctx->instance, &gpuCount, NULL);

	if(0 == gpuCount)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to find GPU with  support!");
	}

	BkpBool found = BKP_FALSE;
	VkPhysicalDevice * vkGPUs = (VkPhysicalDevice *)bkpArrayCreateFrom(VkPhysicalDevice, ctx->memoryGroupId);
	bkpArrayResize(&vkGPUs, gpuCount);
	vkEnumeratePhysicalDevices(ctx->instance, &gpuCount, vkGPUs);

	ctx->adapters = bkpArrayCreateFrom(struct SAdapter, ctx->memoryGroupId);
	bkpArrayReserve(&ctx->adapters, gpuCount);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Devices : %u", gpuCount);
	for(uint32_t i = 0; i < gpuCount; ++i)
	{
		struct SAdapter adapter = {};
		adapter.frameInfo = (BkpFrameInfo) {};
		adapter.gpu = vkGPUs[i];
		adapter.enabled = BKP_FALSE;
		adapter.isRenderSyncInit = BKP_FALSE;
		adapter.vma_ = NULL;
		adapter.score = isDeviceSuitable(ctx, &adapter, ctx->surface);
		size_t sAd = bkpArraySize(ctx->adapters);
		size_t indexToAdd = sAd;
		if(adapter.score > 0)
		{
			found = BKP_TRUE;
		}

		for(size_t j = 0; j < sAd; ++j)
		{
			if(ctx->adapters[j].score < adapter.score)
			{
				indexToAdd = j;
				break;
			}
		}
		bkpArrayInsertAt(&ctx->adapters, adapter, indexToAdd);
	}
	bkpArrayDestroy(&vkGPUs);

	size_t sAd = bkpArraySize(ctx->adapters);
	for(size_t j = 0; j < sAd; ++j)
	{
		LOGC(eINFO, bkpTAG, bkpColor, "#%u, GPU %s (%u, %u) score : %u", j, ctx->adapters[j].deviceProperties.deviceName,
				VK_VERSION_MAJOR(ctx->adapters[j].deviceProperties.apiVersion),
				VK_VERSION_MINOR(ctx->adapters[j].deviceProperties.apiVersion),
				ctx->adapters[j].score);
		LOGC(eDEBUG, bkpTAG, bkpColor, "\t\t\tscore : %u\n", ctx->adapters[j]);
	}

	if(found == BKP_FALSE)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to find a suitable GPU!");
	}
	return;
	/*
	uint32_t gpuCount = 0;
	vkEnumeratePhysicalDevices(ctx->instance, &gpuCount, NULL);

	if(0 == gpuCount)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to find GPU with  support!");
	}

	std::vector<VkPhysicalDevice> vkGPUs(gpuCount);
	vkEnumeratePhysicalDevices(ctx->instance, &gpuCount, vkGPUs.data());

	ctx->adapter = NULL;
	size_t indice = 0;
	uint32_t score = 0;

	LOGC(eDEBUG, bkpTAG, bkpColor, "Devices : %u", gpuCount);
	for(uint32_t i = 0; i < gpuCount; ++i)
	{
		struct SAdapter adapter(vkGPUs[i], BKP_FALSE);
		uint32_t tscore = isDeviceSuitable(ctx, &adapter, ctx->surface);

		ctx->adapters.push_back(adapter);

		LOGC(eDEBUG, bkpTAG, bkpColor, "\t\t\tscore : %u\n", tscore);

		if(tscore > score)
		{
			indice = i;
			score = tscore;
		}
	}

	if(score == 0)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to find a suitable GPU!");
	}

	//bkpActivateAdapter(ctx, indice);
	ctx->adapter = &ctx->adapters[indice];
	ctx->adapter->enabled = BKP_TRUE;
	ctx->defaultAdapterIndex = indice;
	ctx->currentFrame = 0;

	sprintf(ctx->adapter->szName, "adapter_%lu", indice);
	BkpAllocationGroupInfo grp = {ctx->adapter->szName, (size_t)(SIZE_100_MIB * 2.56f)};
	ctx->adapter->memoryGroupId = bkpAddAllocatorGroup(&grp);

	LOGC(eINFO, bkpTAG, bkpColor, "Selected - GPU %s (%u, %u) score : %u", ctx->adapter->deviceProperties.deviceName,
			VK_VERSION_MAJOR(ctx->adapter->deviceProperties.apiVersion),
			VK_VERSION_MINOR(ctx->adapter->deviceProperties.apiVersion),
			score);
	LOGC(eINFO, bkpTAG, bkpColor, "Memory both host_visible and local_bit: %s", BOOL_TO_STRING(ctx->adapter->uboVisibleLocal));

	LOGC(eINFO, bkpTAG, bkpColor, "SamplerBits:");
	for (uint16_t i = 0; i < BKP_MAX_SAMPLER_COUNT; ++i)
	{
		LOGC(eINFO, bkpTAG, bkpColor, "\t%u : %u", 1 << i, ctx->adapter->samplerCountBit[i]);
	}
	return;
	*/
}

/*____________________________________________________________________________________*/
static uint32_t isDeviceSuitable(BkpVulkanContext * ctx, BkpGpuAdapter adapter, VkSurfaceKHR surface)
{
	uint32_t score = 0;
  VkPhysicalDeviceMaintenance6Properties maint6Prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_PROPERTIES, NULL};
	VkPhysicalDeviceMaintenance3Properties maint3Prop = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES, &maint6Prop};
	VkPhysicalDeviceProperties2 propDev = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &maint3Prop};


	vkGetPhysicalDeviceProperties2(adapter->gpu, &propDev);
	vkGetPhysicalDeviceFeatures(adapter->gpu, &adapter->deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(adapter->gpu, &adapter->deviceMemory);

	adapter->deviceProperties        = propDev.properties;
	adapter->maxMemoryAllocationSize = maint3Prop.maxMemoryAllocationSize;
	adapter->maxPerSetDescriptors    = maint3Prop.maxPerSetDescriptors;
	adapter->frameBufferResized      = BKP_FALSE;
	adapter->allocCount              = 0;
	adapter->allocator               = ctx->allocator;
	adapter->warningAllocPersent     = WARNING_ALLOC_PERCENT;
	//LOGC(eFATAL, bkpTAG, bkpColor, "MaxCombine size : %u", maint6Prop.maxCombinedImageSamplerDescriptorCount);

	getSamplerList(adapter);

	LOGC(eINFO, bkpTAG, bkpColor, "MaxAlloc size : %u", adapter->maxMemoryAllocationSize);
	LOGC(eINFO, bkpTAG, bkpColor, "MaxPerSet : %u", adapter->maxPerSetDescriptors);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Device : %s", adapter->deviceProperties.deviceName);

	if(!adapter->deviceFeatures.geometryShader)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Geometry Shader not supported!");
		return 0;
	}

	if(!adapter->deviceFeatures.fillModeNonSolid)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Wired Mode not supported!");
		return 0;
	}

	if(VK_VERSION_MAJOR(adapter->deviceProperties.apiVersion) < stc_vMajor)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, " version not satisfied! current Major %u but required Major >= %u", VK_VERSION_MAJOR(adapter->deviceProperties.apiVersion),stc_vMajor);
		return 0;
	}
	else
	{
		score += 200 * VK_VERSION_MAJOR(adapter->deviceProperties.apiVersion);
	}

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(adapter->gpu, &queueFamilyCount, NULL);
	score += 10 * queueFamilyCount;

	if(VK_VERSION_MINOR(adapter->deviceProperties.apiVersion) < stc_vMinor)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, " version not satisfied! current %u.%u"
				" but required >= %u.%u", VK_VERSION_MAJOR(adapter->deviceProperties.apiVersion), VK_VERSION_MINOR(adapter->deviceProperties.apiVersion),
				stc_vMajor, stc_vMinor);
		return 0;
	}


	if(adapter->deviceProperties.deviceType < 5)
	{
		LOGC(eDEBUG, bkpTAG, bkpColor, "Device type : %s", gDeviceType[adapter->deviceProperties.deviceType]);
	}
	else
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Device type : Unknown");
		return 0;
	}

	if(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == adapter->deviceProperties.deviceType)
	{
		if(ctx->info.forceIntegratedGPU == BKP_TRUE)
		{
			return 0;
		}

		score += 9000000;
	}
	else
	{
		if(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == adapter->deviceProperties.deviceType)
		{
			score += 100000;
		}
		else if(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU == adapter->deviceProperties.deviceType)
		{
			score += 10000;
		}
		else if(VK_PHYSICAL_DEVICE_TYPE_CPU == adapter->deviceProperties.deviceType)
		{
			score += 1000;
		}
		else
		{
			score += 50;
		}
	}

	adapter->uboVisibleLocal = (VkMemoryPropertyFlagBits)(0);
	for(size_t i = 0; i < adapter->deviceMemory.memoryTypeCount; ++i)
	{
		if(((adapter->deviceMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
			((adapter->deviceMemory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0))
		{
			adapter->uboVisibleLocal = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			score += 10000;
			break;
		}

	}

	score += adapter->deviceProperties.limits.maxImageDimension2D;
	score += adapter->deviceProperties.limits.maxImageDimension3D;
	score += adapter->deviceProperties.limits.maxImageDimensionCube;


	if(BKP_FALSE == checkDeviceExtensionsSupported(ctx, adapter->gpu))
	{
		LOGC(eERROR, bkpTAG, bkpColor, "\nSome extension devices are not supported!\n");
		return 0;
	}

	return score;
}

/*____________________________________________________________________________________*/
static BkpBool checkDeviceExtensionsSupported(BkpVulkanContext * ctx, VkPhysicalDevice gpu)
{
	uint32_t n = 0;
	BkpArray(const char *) deviceExt = (BkpArray(const char *))getRequiredDeviceExtensions(gpu, ctx->memoryGroupId);
	size_t deviceExtCount = bkpArraySize(deviceExt);
	size_t ctxDeviceExtCount = bkpArraySize(ctx->szDeviceExt);

	for(size_t i = 0; i < ctxDeviceExtCount; ++i)
	{
		int in = 0;
		for(size_t j = 0; j < deviceExtCount; ++j)
		{
      if(ctx->info.printVulkanInfo == BKP_TRUE)
      {
        LOGC(eDEBUG, bkpTAG, bkpColor, "\t\t\t%s", deviceExt[j]);
      }
			if(strcmp(ctx->szDeviceExt[i], deviceExt[j]) == 0)
			{
				in = 1;
				++n;
				break;
			}
		}
		if(in == 0)
		{
			LOGC(eWARNING, bkpTAG, bkpColor, "\tExtension :'%s' not supported!", ctx->szDeviceExt[i]);
		}
	}
	bkpArrayDestroy(&deviceExt);

	LOGC(eDEBUG, bkpTAG, bkpColor, "\tRequiered device extension %d / %d", n, ctxDeviceExtCount);

	return n == ctxDeviceExtCount;
}

/*____________________________________________________________________________________*/
static BkpArray(char *) getRequiredDeviceExtensions(VkPhysicalDevice gpu, size_t memoryGroupId)
{
	uint32_t extensionCount = 0;

	VkExtensionProperties * extProp = (VkExtensionProperties *)bkpArrayCreateFrom(VkExtensionProperties, memoryGroupId);
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &extensionCount, NULL);
	bkpArrayResize(&extProp, extensionCount);
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &extensionCount, extProp);

	BkpArray(char *) exten = (BkpArray(char *))bkpArrayCreateFrom(char *, memoryGroupId);
	bkpArrayReserve(&exten, extensionCount);

	for(size_t i = 0; i < extensionCount; ++i)
	{
		bkpArrayPush(&exten, extProp[i].extensionName);
	}

	bkpArrayDestroy(&extProp);

	return exten;
}

/*____________________________________________________________________________________*/
/* Iterate VkPhysicalDeviceFeatures as a flat VkBool32 array.
 * For each requested field: enable it if the GPU supports it, warn otherwise.
 * `activated` may be NULL (used for BKP internal defaults). */
static void applyFeatureHints(BkpGpuAdapter adapter,
                               VkPhysicalDeviceFeatures * dst,
                               const VkPhysicalDeviceFeatures * requested,
                               VkPhysicalDeviceFeatures * activated)
{
	const VkBool32 * req = (const VkBool32 *)requested;
	const VkBool32 * sup = (const VkBool32 *)&adapter->deviceFeatures;
	VkBool32       * out = (VkBool32 *)dst;
	VkBool32       * act = activated ? (VkBool32 *)activated : NULL;
	size_t count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

	for(size_t i = 0; i < count; ++i)
	{
		if(!req[i]) continue;
		if(sup[i])
		{
			out[i] = VK_TRUE;
			if(act) act[i] = VK_TRUE;
		}
		else
		{
			if(act) act[i] = VK_FALSE;
			LOGC(eWARNING, bkpTAG, bkpColor,
			     "VkPhysicalDeviceFeatures[%zu] requested but not supported by this GPU — ignored", i);
		}
	}
}

static void createLogicalDevice(BkpVulkanContext * ctx, BkpGpuAdapter adapter, BkpAdapterCriteria * criteria)
{
	findQueueFamilies(adapter);
	int32_t uniqQ[MAX_QUEUE] = {-1, -1, -1, -1, -1, -1, -1, -1};

	if(adapter->graphicQueue.isSet) uniqQ[adapter->graphicQueue.index] = adapter->graphicQueue.index;
	if(adapter->computeQueue.isSet) uniqQ[adapter->computeQueue.index] = adapter->computeQueue.index;
	if(adapter->transfertQueue.isSet) uniqQ[adapter->transfertQueue.index] = adapter->transfertQueue.index;
	if(adapter->presentQueue.isSet) uniqQ[adapter->presentQueue.index] = adapter->presentQueue.index;

	VkDeviceQueueCreateInfo queueCreateInfo[MAX_QUEUE];
	size_t count = 0;
	float queuepriority = 1.0f;

	for(size_t i = 0 ; i < MAX_QUEUE; ++i)
	{
		if(uniqQ[i] < 0) continue;
		VkDeviceQueueCreateInfo qinf = {};
		qinf.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qinf.queueFamilyIndex = uniqQ[i];
		qinf.queueCount = 1;
		qinf.pQueuePriorities = &queuepriority;
		queueCreateInfo[count] = qinf;
		count++;
	}

	LOGC(eDEBUG, bkpTAG, bkpColor, "Creating %d Queue(s)", count);

	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &features13;

	/* BKP defaults — always requested, no output struct */
	VkPhysicalDeviceFeatures bkpDefaults = {};
	bkpDefaults.fillModeNonSolid  = VK_TRUE;
	bkpDefaults.samplerAnisotropy = VK_TRUE;
	applyFeatureHints(adapter, &deviceFeatures2.features, &bkpDefaults, NULL);

	/* user hints */
	if(criteria && criteria->hint)
		applyFeatureHints(adapter, &deviceFeatures2.features,
		                  &criteria->hint->requested, &criteria->hint->activated);

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &deviceFeatures2;
	createInfo.queueCreateInfoCount = count;
	createInfo.pQueueCreateInfos = queueCreateInfo;
	createInfo.pEnabledFeatures = NULL;

	createInfo.enabledExtensionCount = (uint32_t)bkpArraySize(ctx->szDeviceExt);
	createInfo.ppEnabledExtensionNames = ctx->szDeviceExt;

	if(bValidation_)
	{
		createInfo.enabledLayerCount = gValidationLayersCount;
		createInfo.ppEnabledLayerNames = gTszValidationLayers;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if(VK_SUCCESS != vkCreateDevice(adapter->gpu, &createInfo, ctx->allocator, &adapter->device))
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "failed to create logical device!");
	}

	//TODO handle more then 1 Queue based on configuration requirement and keep queue 0 of each family for renderer inner

	if(adapter->graphicQueue.isSet)
	{

		vkGetDeviceQueue(adapter->device, adapter->graphicQueue.index, 0, &adapter->graphicQueue.queue[0]);
		LOGC(eDEBUG, bkpTAG, bkpColor, "Graphic queue handler retrieved index : %u", adapter->graphicQueue.index);
	}

	if(adapter->computeQueue.isSet)
	{
		vkGetDeviceQueue(adapter->device, adapter->computeQueue.index, 0, &adapter->computeQueue.queue[0]);
		LOGC(eDEBUG, bkpTAG, bkpColor, "Compute queue handler retrieved index : %u", adapter->computeQueue.index);
	}

	if(adapter->transfertQueue.isSet)
	{
		vkGetDeviceQueue(adapter->device, adapter->transfertQueue.index, 0, &adapter->transfertQueue.queue[0]);
		LOGC(eDEBUG, bkpTAG, bkpColor, "Transfert queue handler retrieved index : %u", adapter->transfertQueue.index);
	}

	if(adapter->presentQueue.isSet)
	{
		vkGetDeviceQueue(adapter->device, adapter->presentQueue.index, 0, &adapter->presentQueue.queue[0]);
		LOGC(eDEBUG, bkpTAG, bkpColor, "Presentation queue handler retrieved index : %u", adapter->presentQueue.index);
	}

	LOGC(eDEBUG, bkpTAG, bkpColor, "Logical device created");

	return;
}


/*___________________________________________________________________*/
static void selectQueue(uint8_t family, BkpGpuQueue * queue, BkpGpuAdapter adapter, VkQueueFamilyProperties * qList, size_t listCount,size_t * vQ)
{

	size_t small;
	size_t index = 0;

	//LOGC(eDEBUG, bkpTAG, bkpColor, "\tvQ[%d, %d, %d]", vQ[0], vQ[1],vQ[2]);

	if(family != eQFAMILY_PRESENT)
	{
		VkQueueFlagBits flag = bkpGetVkQueueFlagBit(family);
		queue->isSet = BKP_FALSE;
		uint8_t trx[MAX_QUEUE_COUNT] = {0};
		uint8_t trxCount = 0;

		for(size_t i = 0; i < listCount; ++i)
		{
			if(qList[i].queueFlags & flag)
			{
				trx[trxCount] = i;
				++trxCount;
			}
		}

		index = trx[0];
		small = vQ[trx[index]];
		for(size_t i = 1; i < trxCount; ++i)
		{
			if(vQ[trx[i]] < small)
			{
				small = vQ[trx[i]];
				index = trx[i];
			}
		}
		queue->isSet = BKP_TRUE;
		queue->type = family;
		queue->index = (uint32_t)index;
		queue->priority = 1.0f;
		queue->properties = qList[index];
		++vQ[index];
		LOGC(eDEBUG, bkpTAG, bkpColor, "Queue #%u SELECTED for %s", index, bkpQueue2String(flag));
	}
	else
	{
		if(adapter->surface != VK_NULL_HANDLE)
		{
			adapter->presentQueue.isSet = BKP_FALSE;
			VkBool32 presentSupport = VK_FALSE;

			vkGetPhysicalDeviceSurfaceSupportKHR(adapter->gpu, adapter->graphicQueue.index, adapter->surface, &presentSupport);
			if(adapter->graphicQueue.isSet && presentSupport)
			{
				queue->type = eQFAMILY_PRESENT;
				queue->isSet = BKP_TRUE;
				queue->index = adapter->graphicQueue.index;
				queue->priority = 1.0f;
				queue->properties = adapter->graphicQueue.properties;
				index = adapter->graphicQueue.index;
				++vQ[index];
			}
			else
			{
				for(size_t i = 0; i < listCount; ++i)
				{
					presentSupport = VK_FALSE;
					vkGetPhysicalDeviceSurfaceSupportKHR(adapter->gpu, (uint32_t)i, adapter->surface, &presentSupport);
					if(VK_TRUE == presentSupport)
					{
						queue->type = eQFAMILY_PRESENT;
						queue->isSet = BKP_TRUE;
						queue->index = i;
						queue->priority = 1.0f;
						queue->properties = qList[i];
						++vQ[i];
						index = i;
						break;
					}
				}
			}
			LOGC(eDEBUG, bkpTAG, bkpColor, "Queue #%u SELECTED for presentation", index);
		}
	}
}

/*___________________________________________________________________*/
static void findQueueFamilies(BkpGpuAdapter adapter)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(adapter->gpu, &queueFamilyCount, NULL);
	if(0 == queueFamilyCount)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "\t No Queue family detected");
	}

	BkpArray(VkQueueFamilyProperties) queueFamily = (BkpArray(VkQueueFamilyProperties))bkpArrayCreateFrom(VkQueueFamilyProperties, adapter->memoryGroupId);
	bkpArrayResize(&queueFamily, queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(adapter->gpu, &queueFamilyCount, queueFamily);

	size_t vQ[MAX_QUEUE_COUNT] = {0};

#ifndef NDEBUG
	LOGC(eDEBUG, bkpTAG, bkpColor, "Queue Families:");
	size_t sQu = bkpArraySize(queueFamily);
	for(size_t i = 0; i < sQu; ++i)
	{
		int gfx = queueFamily[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? 1 : 0;
		int trx = queueFamily[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? 1 : 0;
		int cmp = queueFamily[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? 1 : 0;
		LOGC(eDEBUG, bkpTAG, bkpColor, "\tQueue%u -> Graphics : %u, Transfer : %u, Compute:%u", i,
			gfx, trx, cmp);
	}
#endif

	selectQueue(eQFAMILY_GRAPHIC, &adapter->graphicQueue, adapter, queueFamily, queueFamilyCount, vQ);
	selectQueue(eQFAMILY_TRANSFERT, &adapter->transfertQueue, adapter, queueFamily, queueFamilyCount, vQ);
	selectQueue(eQFAMILY_COMPUTE, &adapter->computeQueue, adapter, queueFamily, queueFamilyCount, vQ);
	selectQueue(eQFAMILY_PRESENT, &adapter->presentQueue, adapter, queueFamily, queueFamilyCount, vQ);

	bkpArrayDestroy(&queueFamily);
}

/*___________________________________________________________________*/
void bkpCreateCmdPool(BkpGpuAdapter adapter, BkpCommandPool * cmdPool)
{
	VkCommandPoolCreateInfo poolInfo = {};

	poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex        = cmdPool->pQueue->index;
	poolInfo.flags                   = cmdPool->flags;
	poolInfo.pNext                   = cmdPool->pNext;

	cmdPool->resetable = (poolInfo.flags & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) == VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;


	if(vkCreateCommandPool(adapter->device, &poolInfo, adapter->allocator, &cmdPool->commandPool) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "\tUnable to create command pool!");
	}
	LOGC(eDEBUG, bkpTAG, bkpColor, "\tcommand pool created!");
}

/*____________________________________________________________________________________*/
void bkpCleanupSwapChain(BkpGpuAdapter adapter, BkpSwapChain * swc)
{
	size_t imageViewCount = bkpArraySize(swc->imageViews);
	for (size_t i = 0; i < imageViewCount; ++i)
	{
		vkDestroyImageView(adapter->device, swc->imageViews[i], adapter->allocator);
	}

	bkpArrayDestroy(&swc->details.formats);
	bkpArrayDestroy(&swc->details.presentModes);
	bkpArrayDestroy(&swc->images);
	bkpArrayDestroy(&swc->imageViews);
	swc->details.formats = NULL;
	swc->details.presentModes = NULL;
	swc->images = NULL;
	swc->imageViews = NULL;

	if(adapter->isRenderSyncInit == BKP_TRUE)
	{
		size_t s = bkpArraySize(adapter->semaphoreRenderReady);
		for(size_t i = 0; i < s; ++i)
			vkDestroySemaphore(adapter->device, adapter->semaphoreRenderReady[i], adapter->allocator);

		s = bkpArraySize(adapter->semaphorePresentReady);
		for(size_t i = 0; i < s; ++i)
		{
			vkDestroySemaphore(adapter->device, adapter->semaphorePresentReady[i], adapter->allocator);
			bkpDestroyFence(adapter, &adapter->inFlightFences[i]);
		}
		bkpArrayDestroy(&adapter->semaphoreRenderReady);
		bkpArrayDestroy(&adapter->semaphorePresentReady);
		bkpArrayDestroy(&adapter->inFlightFences);
		bkpArrayDestroy(&adapter->imagesInFlight);
		adapter->semaphoreRenderReady = NULL;
		adapter->semaphorePresentReady = NULL;
		adapter->inFlightFences = NULL;
		adapter->imagesInFlight = NULL;
	}


	vkDestroySwapchainKHR(adapter->device, swc->swapChain, adapter->allocator);
}

/*____________________________________________________________________________________*/
void bkpDestroySampler(BkpGpuAdapter adapter, BkpSampler * sampler)
{
	vkDestroySampler(adapter->device, sampler->sampler, adapter->allocator);
}

/*_________________________________________________________________________________*/
VkCommandBufferBeginInfo bkpDefaultCmdBufferBeginInfo(VkCommandBufferUsageFlags flag)
{
	VkCommandBufferBeginInfo bg = {};
	bg.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bg.flags = flag;
	bg.pInheritanceInfo = NULL;

	return bg;
}

/*____________________________________________________________________________________*/
void submitCommandBuffer(BkpCommandBuffer * cmd, uint32_t index)
{
	cmd->state = eSTATE_SUBMITTED;
}

/*____________________________________________________________________________________*/
VkSwapchainCreateInfoKHR bkpDefaultSwapChain(uint32_t arrayLayers, VkImageUsageFlags usage, VkCompositeAlphaFlagBitsKHR compositeAlpha, VkBool32 clipped, VkPresentModeKHR presentMode)
{
	VkSwapchainCreateInfoKHR info = {};
	info.sType	= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.pNext	= NULL;
    info.flags	= 0;
    info.surface	= VK_NULL_HANDLE;
    info.minImageCount	= 0;
    info.imageFormat	= VK_FORMAT_B8G8R8A8_SRGB;
    //info.imageColorSpace	= 0;
    //info.imageExtent	= {0, 0};
    info.imageArrayLayers	= arrayLayers;
    info.imageUsage	= usage;
    info.imageSharingMode	= VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount	= 1;
    info.pQueueFamilyIndices	= NULL;
    //info.preTransform	=;
    info.compositeAlpha	= compositeAlpha;
    info.presentMode	= presentMode;
    info.clipped	= clipped;
    info.oldSwapchain	= VK_NULL_HANDLE;

	return info;
}

/*____________________________________________________________________________________*/
void bkpCreateSwapChain(BkpGpuAdapter adapter, BkpSwapChain * swapChain,
                        const VkPresentModeKHR * preferredModes, uint32_t preferredCount)
{
	querySwapChainSupport(adapter, swapChain, adapter->surface);

	size_t formatCount = bkpArraySize(swapChain->details.formats);
	size_t presentCount = bkpArraySize(swapChain->details.presentModes);

	if(0 == formatCount || 0 == presentCount)
	{
		LOGC(eDEBUG, bkpTAG, bkpColor, "\tProblem with SwapChain nfomat(%u) npresentModes(%u)!\n",
				formatCount,
				presentCount);
		return;
	}

	LOGC(eINFO, bkpTAG, bkpColor, "\tSwapChain : %u formats", formatCount);
	LOGC(eINFO, bkpTAG, bkpColor, "\tSwapChain : %u presentation modes", presentCount);
	LOGC(eINFO, bkpTAG, bkpColor, "*******************************************************************\n");

	LOGC(eDEBUG, bkpTAG, bkpColor, "Creating SwapChain : ");
	BkpSwapChainSupportDetails * sChainSupport = &swapChain->details;

	if (sChainSupport->capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) 
  {
		swapChain->info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (sChainSupport->capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
  {
		swapChain->info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(sChainSupport->formats, formatCount);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(sChainSupport->presentModes, presentCount,
			preferredModes, preferredCount);
	VkExtent2D extent = chooseSwapExtent2D(sChainSupport->capabilities);

	uint32_t imageCount = sChainSupport->capabilities.minImageCount + 1;
	LOGC(eDEBUG, bkpTAG, bkpColor, "SwapChain imageCount min: %u", imageCount);
	if(swapChain->imageCount > 0)
	{
		imageCount = swapChain->imageCount < imageCount ? imageCount : swapChain->imageCount;
	}

	if(sChainSupport->capabilities.maxImageCount > 0 && imageCount > sChainSupport->capabilities.maxImageCount)
	{
		imageCount = sChainSupport->capabilities.maxImageCount;
	}

	//should be customazibale
	LOGC(eINFO, bkpTAG, bkpColor, "SwapChain Images : %u (%ux%u)\n", imageCount, extent.width, extent.height);

	swapChain->info.surface =adapter->surface;
	swapChain->info.minImageCount = imageCount;
	swapChain->info.imageFormat = surfaceFormat.format;
	swapChain->info.imageColorSpace = surfaceFormat.colorSpace;
	swapChain->info.imageExtent = extent;

	if(adapter->frameInfo.maxFrameInFlight >= imageCount)
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "required inFlight frame greater than image count, reajusting %u -> %u\n", adapter->frameInfo.maxFrameInFlight, imageCount);
		adapter->frameInfo.maxFrameInFlight = imageCount;
	}

	if(adapter->graphicQueue.index != adapter->presentQueue.index)
	{
		uint32_t qfi[] = {adapter->graphicQueue.index, adapter->presentQueue.index};
		swapChain->info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChain->info.queueFamilyIndexCount = 2;
		swapChain->info.pQueueFamilyIndices = qfi;
	}
	else
	{
		swapChain->info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChain->info.queueFamilyIndexCount = 1;
		swapChain->info.pQueueFamilyIndices = &adapter->graphicQueue.index;
	}

	swapChain->info.preTransform = sChainSupport->capabilities.currentTransform; //no transformations
	swapChain->info.presentMode = presentMode;


	if(vkCreateSwapchainKHR(adapter->device, &swapChain->info, adapter->allocator, &swapChain->swapChain) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to create swap chain!");
	}

	adapter->frameInfo.winWidth  = extent.width;
	adapter->frameInfo.winHeight = extent.height;
	adapter->frameInfo.ratio = (float)((float)adapter->frameInfo.winWidth / adapter->frameInfo.winHeight);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Swap chain created!");
	swapChain->images = (BkpArray(VkImage))bkpArrayCreateFrom(VkImage, adapter->memoryGroupId);
	vkGetSwapchainImagesKHR(adapter->device, swapChain->swapChain, &imageCount, NULL);
	bkpArrayResize(&swapChain->images, imageCount);
	vkGetSwapchainImagesKHR(adapter->device, swapChain->swapChain, &imageCount, swapChain->images);

	adapter->frameInfo.imageCount = imageCount;
	swapChain->imageCount = imageCount;

	LOGC(eDEBUG, bkpTAG, bkpColor, "retrieved %u swap chain images", imageCount);

	createImageViews(adapter, swapChain);

	adapter->swapChain = swapChain;

	bkpCreateRenderSyncObjects(adapter, imageCount, adapter->frameInfo.maxFrameInFlight);

	return ;
}

/*____________________________________________________________________________________*/
BkpBool recreateSwapChain(BkpVulkanContext * ctx)
{
	/*
	ctx->recreatingSwapChain = BKP_TRUE;
	bkp::gfx::waitDevice(&context);

	//clean internal context objects
	pBkpOnResize(ctx);
	// create internal context Objects;
	ctx->recreatingSwapChain = BKP_FALSE;
	*/

	return BKP_TRUE;
}

/*_________________________________________________________________________________*/
static void createImageViews(BkpGpuAdapter adapter, BkpSwapChain * swapChain)
{

	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.pNext          = NULL;
	info.flags          = 0;
	info.viewType       = VK_IMAGE_VIEW_TYPE_2D;
	info.format         = swapChain->info.imageFormat;
	info.components.r	= VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.g	= VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.b	= VK_COMPONENT_SWIZZLE_IDENTITY;
	info.components.a	= VK_COMPONENT_SWIZZLE_IDENTITY;
	info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	info.subresourceRange.baseMipLevel   = 0;
	info.subresourceRange.levelCount     = 1;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount     = 1;

	swapChain->imageViews = (BkpArray(VkImageView))bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);
	bkpArrayReserve(&swapChain->imageViews, adapter->frameInfo.imageCount);
	for(uint32_t i = 0; i < adapter->frameInfo.imageCount; ++i)
	{
		bkpArrayPush(&swapChain->imageViews, createImageView(adapter->device, swapChain->images[i], &info, adapter->allocator));
	}

	return ;
}

/*_________________________________________________________________________________*/
static VkImageView createImageView(VkDevice device, VkImage image, VkImageViewCreateInfo * info, VkAllocationCallbacks * allocator)
{
	info->image = image;
	info->sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		//text->viewInfo.viewType = ;J
  //fprintf(stderr, "CREATING image view type : %d vs %d\n", info->viewType, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

	VkImageView imageView;
	if(vkCreateImageView(device, info, allocator, &imageView) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create image views!\n");
	}

	return imageView;
}

/*____________________________________________________________________________________*/
static void querySwapChainSupport(BkpGpuAdapter adapter, BkpSwapChain * sc, VkSurfaceKHR surface)
{
	uint32_t formatCount = 0;
	uint32_t presentCount = 0;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(adapter->gpu, surface, &(sc->details.capabilities));
	vkGetPhysicalDeviceSurfaceFormatsKHR(adapter->gpu, surface, &formatCount, NULL);
	vkGetPhysicalDeviceSurfacePresentModesKHR(adapter->gpu, surface, &presentCount, NULL);

	if(formatCount == 0)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Adapter has no surface format!\n");
	}
	if(presentCount == 0)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Adapter has no presentation capability!\n");
	}

	sc->details.formats = (BkpArray(VkSurfaceFormatKHR))bkpArrayCreateFrom(VkSurfaceFormatKHR, adapter->memoryGroupId);
	bkpArrayResize(&sc->details.formats, formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(adapter->gpu, surface, &formatCount, sc->details.formats);

	sc->details.presentModes = (BkpArray(VkPresentModeKHR))bkpArrayCreateFrom(VkPresentModeKHR, adapter->memoryGroupId);
	bkpArrayResize(&sc->details.presentModes, presentCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(adapter->gpu, surface, &presentCount, sc->details.presentModes);

	return;
}

/*____________________________________________________________________________________*/
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR * list, size_t size)
{
	for(size_t i = 0; i < size; ++i)
	{
		if(list[i].format == VK_FORMAT_B8G8R8A8_SRGB && list[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			LOGC(eINFO, bkpTAG, bkpColor, "Format : SRGB(8x4) Color_space SRGB_nonliear_KHR");
			return list[i];
		}
	}

	LOGC(eINFO, bkpTAG, bkpColor, "Format : using first one");
	return list[0];
}

/*____________________________________________________________________________________*/
static VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR * list, size_t listCount, const VkPresentModeKHR * prefered, size_t preferedCount)
{
	if(prefered[0] == VK_PRESENT_MODE_FIFO_KHR)
	{
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	static const char * szInfo[] = {"IMMEDIATE", "MAILBOX", "FIFO", "FIFO_RELAXED"};
	VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;

	LOGC(eDEBUG, bkpTAG, bkpColor, "GPU presentation modes:");
	for(size_t i = 0 ; i < listCount; ++i)
	{
		if(list[i] < 4)
			LOGC(eINFO, bkpTAG, bkpColor, "\t\tmode : %s", szInfo[list[i]]);
		else if(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR == list[i])
			LOGC(eINFO, bkpTAG, bkpColor, "\t\tmode : SHARED_DEMAND_REFRESH");
		else if(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR == list[i])
			LOGC(eINFO, bkpTAG, bkpColor, "\t\tmode : SHARED_CONTINUOUS_REFRESH");
		else
			LOGC(eINFO, bkpTAG, bkpColor, "\t\tmode : UNKNOWN(%d)", (int)list[i]);
	}

	for(size_t i = 0 ; i < preferedCount ; ++i)
	{
		const char * pname = prefered[i] < 4 ? szInfo[prefered[i]] : "EXTENDED";
		LOGC(eDEBUG, bkpTAG, bkpColor, "\tUser prefered presentation Mode : %s", pname);
		for(size_t j = 0 ; j < listCount; ++j)
		{
			if(list[j] == prefered[i])
			{
				LOGC(eINFO, bkpTAG, bkpColor, "Present mode Selected : %s", pname);
				return prefered[i];
			}
		}
		LOGC(eWARNING, bkpTAG, bkpColor, "\t%s NOT SUPPORTED", pname);
	}

	LOGC(eINFO, bkpTAG, bkpColor, "Present mode Selected : %s (fallback)", szInfo[mode]);
	return mode;
}

/*____________________________________________________________________________________*/
static VkExtent2D chooseSwapExtent2D(const VkSurfaceCapabilitiesKHR capabilities)
{
	if(capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		uint32_t width, height;
		bkpGetWindowSize(&width, &height);
		VkExtent2D actualExtent = {width, height};
		uint32_t min = capabilities.maxImageExtent.width < actualExtent.width ? capabilities.maxImageExtent.width : actualExtent.width;

		actualExtent.width = capabilities.minImageExtent.width > min ? capabilities.minImageExtent.width : min;

		min = capabilities.maxImageExtent.height < actualExtent.height ? capabilities.maxImageExtent.height : actualExtent.height;
		actualExtent.height = capabilities.minImageExtent.height > min ? capabilities.minImageExtent.height : min;

		return actualExtent;
	}
}

/*____________________________________________________________________________________*/
static VkFormat bkpFindSupportedFormat(VkPhysicalDevice gpu, VkFormat * tCandidate, size_t n, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (size_t i = 0; i < n; ++i)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(gpu, tCandidate[i], &props);
		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return tCandidate[i];
		}
		else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return tCandidate[i];
		}
	}

	LOGC(eFATAL, bkpTAG, bkpColor, "Unable to find supported format!\n");
	return tCandidate[0]; //will never get here.
}

/*____________________________________________________________________________________*/
static VkFormat findDepthFormat(VkPhysicalDevice gpu, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	VkFormat t[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return bkpFindSupportedFormat(gpu, t, 3, tiling, features);
}


/*____________________________________________________________________________________*/
BkpBool bkpCheckFormatFeatureSupport(BkpGpuAdapter adp, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags feature)
{   
  VkFormatProperties formatProps;
  vkGetPhysicalDeviceFormatProperties(adp->gpu, format, &formatProps);

  if (tiling == VK_IMAGE_TILING_OPTIMAL)
    return formatProps.optimalTilingFeatures & feature;

  if (tiling == VK_IMAGE_TILING_LINEAR)
    return formatProps.linearTilingFeatures & feature;

  return BKP_FALSE;
}

/*____________________________________________________________________________________*/
void bkpCreateImageResources(BkpGpuAdapter adapter, BkpImageResource * ress)
{
  bkpCreateImage(adapter, ress);
	if(ress->mipType != eMIPMAP_NONE)
	{
		bkpGenerateMipmaps(adapter, ress);
	}
	ress->imageViews = (BkpArray(VkImageView))bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);
	bkpArrayPush(&ress->imageViews, createImageView(adapter->device, ress->images[0], &ress->viewInfo, adapter->allocator));
}

/*____________________________________________________________________________________*/
void bkpCreateDepthResources(BkpGpuAdapter adapter, BkpImageResource * depth)
{
	depth->imageInfo.format        = findDepthFormat(adapter->gpu, depth->imageInfo.tiling, depth->features);
	//depth->imageInfo.queueFamilyIndexCount = ; ignore as sharing mode is exclusif
	//depth->imageInfo.pQueueFamilyIndices = ;
   	bkpCreateImage(adapter, depth);

    depth->viewInfo.format = depth->imageInfo.format;
	depth->viewInfo.flags  = depth->imageInfo.flags;
	depth->imageViews = (BkpArray(VkImageView))bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);
	bkpArrayPush(&depth->imageViews, createImageView(adapter->device, depth->images[0], &depth->viewInfo, adapter->allocator));
}

/*____________________________________________________________________________________*/
BkpImageResource bkpDefaultDepthBuffer()
{
	BkpImageResource depth = {};
	depth.imageInfo = (VkImageCreateInfo){};
	depth.imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth.imageInfo.pNext         = NULL;
	depth.imageInfo.flags         = 0;
	depth.imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	depth.imageInfo.mipLevels     = 1;
	depth.imageInfo.arrayLayers   = 1;
	depth.imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	depth.imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	depth.imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth.imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	depth.imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	depth.features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depth.bufferType = eBUFFER_GPU;

	depth.viewInfo = (VkImageViewCreateInfo){};
	depth.viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth.viewInfo.pNext = NULL;
    depth.viewInfo.flags = 0;
    depth.viewInfo.image = VK_NULL_HANDLE;
    depth.viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;;
    depth.viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth.viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth.viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth.viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth.viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth.viewInfo.subresourceRange.baseMipLevel = 0;
    depth.viewInfo.subresourceRange.levelCount = 1;
    depth.viewInfo.subresourceRange.baseArrayLayer = 0;
    depth.viewInfo.subresourceRange.layerCount = 1;

	return depth;
}

/*____________________________________________________________________________________*/
BkpImageResource bkpDefaultColorBuffer()
{
	BkpImageResource color = {};
	color.imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	color.imageInfo.imageType     = VK_IMAGE_TYPE_2D;
	color.imageInfo.mipLevels     = 1;
	color.imageInfo.arrayLayers   = 1;
	color.imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	color.imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	color.imageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	                              | VK_IMAGE_USAGE_SAMPLED_BIT;
	color.imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	color.imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	color.features            = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
	                          | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
	color.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	color.bufferType          = eBUFFER_GPU;

	color.viewInfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	color.viewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
	color.viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	color.viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	color.viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	color.viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	color.viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	color.viewInfo.subresourceRange.baseMipLevel   = 0;
	color.viewInfo.subresourceRange.levelCount     = 1;
	color.viewInfo.subresourceRange.baseArrayLayer = 0;
	color.viewInfo.subresourceRange.layerCount     = 1;

	return color;
}

/*____________________________________________________________________________________*/
void bkpCreateImage(BkpGpuAdapter adapter, BkpImageResource * res)
{
	BkpBufferInfo info = {};
	BkpBuffer buff;
	info.usage = res->imageInfo.usage;
	info.type = res->bufferType;
	info.isImage = BKP_TRUE;
	bkpAllocateBufferImage(adapter, &res->imageInfo, &buff, &info);

	res->bufferImages = (BkpArray(BkpBuffer))bkpArrayCreateFrom(BkpBuffer, adapter->memoryGroupId);
	bkpArrayPush(&res->bufferImages, buff);

	res->imageMemories = (BkpArray(VkDeviceMemory))bkpArrayCreateFrom(VkDeviceMemory, adapter->memoryGroupId);
  bkpArrayPush(&res->imageMemories, bkpGetDeviceMemory(adapter, buff));

	VkImage img;
	bkpGetBufferImage(buff, &img);

	res->images = (BkpArray(VkImage))bkpArrayCreateFrom(VkImage, adapter->memoryGroupId);
	bkpArrayPush(&res->images, img);

  res->imageViews = NULL;

	/*
	VkImage image = VK_NULL_HANDLE;
	if(vkCreateImage(adapter->device, &res->imageInfo, adapter->allocator, &image) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to create image!");
	}
	res->images.push_back(image);


	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(adapter->device, image, &memReq);

	LOG(eDEBUG, "TESTER %%", "align/size of image after req:%u/%u", memReq.alignment, memReq.size);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex   = findMemoryType(adapter->gpu, memReq.memoryTypeBits, res->memoryPropertyFlags);

	VkDeviceMemory imageMemory = VK_NULL_HANDLE;
	if(vkAllocateMemory(adapter->device, &allocInfo, adapter->allocator, &imageMemory) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to allocate image memory!");
	}
	res->imageMemories.push_back(imageMemory);

	vkBindImageMemory(adapter->device, image, imageMemory, 0);
	*/
}

/*____________________________________________________________________________________*/
void bkpCreateRenderTarget(BkpGpuAdapter adapter, BkpRenderTarget * Rt, BkpBool createFrameBuffer)
{
	VkAttachmentDescription * attachments = (VkAttachmentDescription *) alloca(sizeof(VkAttachmentDescription) * Rt->info.attachmentCount);
	for(uint32_t i = 0; i < Rt->info.attachmentCount; ++i)
	{
		attachments[i] = Rt->info.attachments[i].info;
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext           = Rt->pNext;
	renderPassInfo.flags           = Rt->flags;
	renderPassInfo.attachmentCount = (uint32_t)Rt->info.attachmentCount;
	renderPassInfo.pAttachments    = attachments;
	renderPassInfo.subpassCount    = (uint32_t)Rt->info.subpassCount;
	renderPassInfo.pSubpasses      = Rt->info.subpasses;
	renderPassInfo.dependencyCount = (uint32_t)Rt->info.dependencyCount;
	renderPassInfo.pDependencies   = Rt->info.dependencies;

	if(vkCreateRenderPass(adapter->device, &renderPassInfo, adapter->allocator, &Rt->renderPass) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create render pass!\n");
	}

  Rt->views = NULL;
  if(createFrameBuffer == BKP_TRUE)
  {
    if(Rt->info.attachmentCount > 0)
    {
      bkpCreateFrameBuffers(adapter, Rt);
      Rt->hasFrameBuffers = BKP_TRUE;
    }
    else
    {
      Rt->hasFrameBuffers = BKP_FALSE;
    }
  }

	Rt->beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	Rt->beginInfo.renderPass = Rt->renderPass;
	Rt->beginInfo.renderArea.offset = Rt->offset;
	Rt->beginInfo.renderArea.extent = Rt->extent;
	Rt->beginInfo.clearValueCount = Rt->clearValueCount;
	Rt->beginInfo.pClearValues = Rt->clearValues;
}

/*____________________________________________________________________________________*/
void bkpCreateFrameBuffers(BkpGpuAdapter adapter, BkpRenderTarget * Rt)
{
	Rt->frameBuffers = (BkpArray(VkFramebuffer))bkpArrayCreateFrom(VkFramebuffer, adapter->memoryGroupId);
	bkpArrayResize(&Rt->frameBuffers, Rt->info.targetCount);

	for(uint32_t i = 0; i < Rt->info.targetCount; ++i)
	{
		VkImageView * attachments = (VkImageView*)alloca(sizeof(VkImageView) * Rt->info.attachmentCount);

		for(uint32_t j = 0; j < Rt->info.attachmentCount; ++j)
		{
			uint32_t viewCount = Rt->info.attachments[j].viewCount;
			if(viewCount > 1)
			{
				if(viewCount >= Rt->info.targetCount)
				{
					attachments[j] = Rt->info.attachments[j].views[i];
				}
				else
				{
					LOGC(eFATAL, bkpTAG, bkpColor, "ImageView Count must not be less then Target imageCount\n");
				}
			}
			else
			{
				attachments[j] = Rt->info.attachments[j].views[0];
			}
		}

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext           = Rt->info.frameBufferNext;
		framebufferInfo.flags           = Rt->info.frameBufferFlags;
		framebufferInfo.renderPass      = Rt->renderPass;
		framebufferInfo.attachmentCount = (uint32_t)Rt->info.attachmentCount;
		framebufferInfo.pAttachments    = attachments;
		framebufferInfo.width           = Rt->info.frameBufferWidth;
		framebufferInfo.height          = Rt->info.frameBufferHeight;
		framebufferInfo.layers          = Rt->info.frameBufferLayers;

		if(vkCreateFramebuffer(adapter->device, &framebufferInfo, adapter->allocator, &Rt->frameBuffers[i]) != VK_SUCCESS)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create framebuffer!\n");
		}
	}

	return;
}

/*____________________________________________________________________________________*/
void bkpManualCreateFrameBuffers(BkpGpuAdapter adapter, BkpRenderTarget * Rt, VkImageViewCreateInfo * ci)
{
  Rt->hasFrameBuffers = BKP_TRUE;
	Rt->frameBuffers = (BkpArray(VkFramebuffer))bkpArrayCreateFrom(VkFramebuffer, adapter->memoryGroupId);
	Rt->views = (BkpArray(VkImageView))bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);

	bkpArrayResize(&Rt->frameBuffers, Rt->info.targetCount);
	bkpArrayResize(&Rt->views, Rt->info.targetCount);

  for (uint32_t i = 0; i < Rt->info.targetCount; i++) 
  {
    //fprintf(stderr, "\tIMAGE : %p", ci[i].image);
    Rt->views[i] = createImageView(adapter->device, ci[i].image, &ci[i], adapter->allocator);

    VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext           = NULL;
		framebufferInfo.flags           = 0;
    framebufferInfo.renderPass = Rt->renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &Rt->views[i];
    framebufferInfo.width = Rt->info.frameBufferWidth;
    framebufferInfo.height = Rt->info.frameBufferHeight;
    framebufferInfo.layers = 1;

		if(vkCreateFramebuffer(adapter->device, &framebufferInfo, adapter->allocator, &Rt->frameBuffers[i]) != VK_SUCCESS)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create framebuffer!\n");
		}
  }
}

/*____________________________________________________________________________________*/
VkSubpassDescription bkpDefaultSubpass(VkSubpassDescriptionFlags flags, VkPipelineBindPoint bindPoint)
{
	VkSubpassDescription desc = {};
	desc.flags                   = flags;
	desc.pipelineBindPoint       = bindPoint;
	desc.inputAttachmentCount    = 0;
	desc.pInputAttachments       = NULL;
	desc.colorAttachmentCount    = 0;
	desc.pColorAttachments       = NULL;
	desc.pDepthStencilAttachment = NULL;
	desc.pResolveAttachments     = NULL;
	desc.preserveAttachmentCount = 0;
	desc.pPreserveAttachments    = NULL;

	return desc;
}

/*____________________________________________________________________________________*/
void bkpPrepareFrame(BkpGpuAdapter adapter)
{
	VkResult res;

	if(!bkpWaitForFence(adapter, &adapter->inFlightFences[adapter->frameInfo.currentFrame], UINT64_MAX))
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "In flight fence wait Failure");
		return;
	}

	res = vkAcquireNextImageKHR(adapter->device, adapter->swapChain->swapChain,
			UINT64_MAX, adapter->semaphorePresentReady[adapter->frameInfo.currentFrame],
			VK_NULL_HANDLE, &adapter->frameInfo.imageAcquired);

	if(VK_ERROR_OUT_OF_DATE_KHR == res)
	{
		if(pBkpOnResize != NULL)
		{
			pBkpOnResize(adapter);
			bkpWindowResetFramebufferResized();
		}
		return;
	}
	else if(res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to acquire swapchain image!\n");
	}

/*
	if(adapter->imagesInFlight[adapter->frameInfo.imageAcquired].fence != VK_NULL_HANDLE)
	{
		if(!bkpWaitForFence(adapter, &adapter->inFlightFences[adapter->frameInfo.currentFrame], UINT64_MAX))
		{
			LOGC(eWARNING, bkpTAG, bkpColor, "In flight fence wait Failure");
			return;
		}
	}
	*/

	adapter->imagesInFlight[adapter->frameInfo.imageAcquired] = adapter->inFlightFences[adapter->frameInfo.currentFrame];
	bkpResetFence(adapter, &adapter->inFlightFences[adapter->frameInfo.currentFrame]);
}

/*____________________________________________________________________________________*/
void bkpSubmitFrameBasic(BkpGpuAdapter adapter, BkpCommandBuffer * cmd)
{
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore sigSem[] = {adapter->semaphoreRenderReady[adapter->frameInfo.imageAcquired]};
	VkSemaphore waitSem[] = {adapter->semaphorePresentReady[adapter->frameInfo.currentFrame]};

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.pWaitSemaphores = waitSem;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = sigSem;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pCommandBuffers = &cmd->cmds[adapter->frameInfo.currentFrame];
	submitInfo.commandBufferCount = 1;
	//	LOGC(eWARNING, bkpTAG, bkpColor, "index = %u / %u", adapter->frameInfo.currentFrame, adapter->frameInfo.maxFrameInFlight);

	if(vkQueueSubmit(adapter->graphicQueue.queue[0], 1, &submitInfo, adapter->inFlightFences[adapter->frameInfo.currentFrame].fence) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to submit draw command buffer!\n");
	}

	cmd->state = eSTATE_SUBMITTED;
}

/*____________________________________________________________________________________*/
void bkpSubmitFrame(BkpGpuAdapter adapter, BkpCommandBuffer ** cmds, uint32_t cmdCount, BkpWaitInfo * waitInfo)
{
	VkCommandBuffer vkcmds[16];
	if(cmdCount > 16) cmdCount = 16;
	for(uint32_t i = 0; i < cmdCount; ++i)
		vkcmds[i] = cmds[i]->cmds[adapter->frameInfo.currentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask    = waitInfo->waitStages;
	submitInfo.pWaitSemaphores      = waitInfo->waitSemaphores;
	submitInfo.waitSemaphoreCount   = (uint32_t)bkpArraySize(waitInfo->waitSemaphores);
	submitInfo.pSignalSemaphores    = waitInfo->signalSemaphores;
	submitInfo.signalSemaphoreCount = (uint32_t)bkpArraySize(waitInfo->signalSemaphores);
	submitInfo.pCommandBuffers      = vkcmds;
	submitInfo.commandBufferCount   = cmdCount;

	if(vkQueueSubmit(adapter->graphicQueue.queue[0], 1, &submitInfo,
	                 adapter->inFlightFences[adapter->frameInfo.currentFrame].fence) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to submit draw command buffer!\n");
	}

	for(uint32_t i = 0; i < cmdCount; ++i)
		cmds[i]->state = eSTATE_SUBMITTED;
}

/*____________________________________________________________________________________*/
void bkpSubmit(BkpGpuAdapter adapter, EGpuQueue queue,
               BkpCommandBuffer ** cmds, uint32_t cmdCount, uint32_t cmdIndex,
               BkpWaitInfo * waitInfo, BkpFence * fence)
{
	VkCommandBuffer vkcmds[16];
	if(cmdCount > 16) cmdCount = 16;
	for(uint32_t i = 0; i < cmdCount; ++i)
		vkcmds[i] = cmds[i]->cmds[cmdIndex];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pCommandBuffers    = vkcmds;
	submitInfo.commandBufferCount = cmdCount;
	if(waitInfo)
	{
		submitInfo.pWaitDstStageMask    = waitInfo->waitStages;
		submitInfo.pWaitSemaphores      = waitInfo->waitSemaphores;
		submitInfo.waitSemaphoreCount   = (uint32_t)bkpArraySize(waitInfo->waitSemaphores);
		submitInfo.pSignalSemaphores    = waitInfo->signalSemaphores;
		submitInfo.signalSemaphoreCount = (uint32_t)bkpArraySize(waitInfo->signalSemaphores);
	}

	BkpGpuQueue * q;
	switch(queue)
	{
		case eQFAMILY_COMPUTE:   q = &adapter->computeQueue;   break;
		case eQFAMILY_TRANSFERT: q = &adapter->transfertQueue; break;
		default:                 q = &adapter->graphicQueue;   break;
	}

	VkFence vkfence = fence ? fence->fence : VK_NULL_HANDLE;
	if(vkQueueSubmit(q->queue[0], 1, &submitInfo, vkfence) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to submit command buffer!\n");
	}

	for(uint32_t i = 0; i < cmdCount; ++i)
		cmds[i]->state = eSTATE_SUBMITTED;
}

/*____________________________________________________________________________________*/
void bkpPresentFrame(BkpGpuAdapter adapter)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.pResults = NULL;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &adapter->swapChain->swapChain;
	presentInfo.pImageIndices = &adapter->frameInfo.imageAcquired;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &adapter->semaphoreRenderReady[adapter->frameInfo.imageAcquired];
	/*
	if(adapter->semaphoreRenderReady[adapter->frameInfo.currentFrame] != VK_NULL_HANDLE)
	{
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &adapter->semaphoreRenderReady[adapter->frameInfo.currentFrame];
	}
	*/

	VkResult res = vkQueuePresentKHR(adapter->presentQueue.queue[0], &presentInfo);

	if(VK_ERROR_OUT_OF_DATE_KHR == res || VK_SUBOPTIMAL_KHR == res || bkpWindowGetFrameBufferResized() == 1)
	{
		if(pBkpOnResize != NULL)
		{
			pBkpOnResize(adapter);
			bkpWindowResetFramebufferResized();
		}
	}
	else if(res != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to present swapchain!\n");
	}

	adapter->frameInfo.currentFrame = (adapter->frameInfo.currentFrame + 1) % adapter->frameInfo.maxFrameInFlight;
	float currentTime = (float)glfwGetTime();
	adapter->frameInfo.deltaTime = currentTime - adapter->frameInfo.lastTime;
	adapter->frameInfo.lastTime = currentTime;
	adapter->frameInfo.frameCount++;
}

/*____________________________________________________________________________________*/
void bkpCreateBuffersGPU(BkpGpuAdapter adapter, BkpBufferGPU * sbu, VkBufferUsageFlags usage, EBufferType type, BkpBool do_map)
{
	BkpBufferInfo info = {};

	info.usage = usage;
	info.type  = type;
	sbu->size = bkpAlignSizeVk(sbu->size, bkpGetAlignmentVk(adapter, info.usage));
	info.size  = sbu->count * sbu->size;
	info.isImage = BKP_FALSE;

	bkpAllocateBuffer(adapter, &sbu->buffer, &info);

	if(do_map == BKP_DO_MAP)
	{
		bkpMapBuffer(adapter, sbu->buffer);
	}

	VkDeviceSize bufferSize;
	bkpGetBufferSize(sbu->buffer, &bufferSize);
	sbu->size = bufferSize / sbu->count;
}

/*____________________________________________________________________________________*/
void bkpDestroyBuffersGPU(BkpGpuAdapter adapter, BkpBufferGPU * sbu)
{
	bkpFreeBuffer(adapter, &sbu->buffer);
}

/*____________________________________________________________________________________*/
/*
void mapData(BkpVulkanContext * ctx, VkMemoryPropertyFlags propFlags, VkDeviceMemory  bufferMemory, void **data, size_t s_size, uint32_t offset)
{
	vkMapMemory(ctx->adapter->device, bufferMemory, 0, s_size, 0, data);
	if((propFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange mrange = {};
		mrange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mrange.memory = bufferMemory;
		mrange.offset = offset;
		mrange.size = s_size;
		vkFlushMappedMemoryRanges(ctx->adapter->device, 1, &mrange);
	}
}
*/

/*____________________________________________________________________________________*/
/*
void unmapData(BkpVulkanContext * ctx, VkDeviceMemory  bufferMemory)
{
	vkUnmapMemory(ctx->adapter->device, bufferMemory);
}
*/

/*____________________________________________________________________________________*/
void bkpCreateSampler(BkpGpuAdapter adapter, BkpSampler * sampler)
{
	sampler->info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	sampler->info.anisotropyEnable = adapter->deviceFeatures.samplerAnisotropy;
	if(sampler->info.anisotropyEnable)
	{
		float devMax = adapter->deviceProperties.limits.maxSamplerAnisotropy;
		if(sampler->info.maxAnisotropy <= 0.0f || sampler->info.maxAnisotropy > devMax)
			sampler->info.maxAnisotropy = devMax;
	}

	if(vkCreateSampler(adapter->device, &sampler->info, adapter->allocator, &sampler->sampler) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Failed to create sampler!");
	}
}

/*____________________________________________________________________________________*/
void bkpGenerateMipmaps(BkpGpuAdapter adapter, BkpImageResource * img)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(adapter->gpu, img->imageInfo.format, &formatProperties);

	if(img->imageInfo.tiling == VK_IMAGE_TILING_OPTIMAL)
	{
		if((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Texture format does not support linear blitting!");
		}
	}

	for(uint32_t layer = 0; layer < img->imageInfo.arrayLayers; ++layer)
	{
		BkpCommandBuffer cmdBuffer;
		bkpBeginCmdBufferUniqUsage(adapter, &cmdBuffer, eQFAMILY_GRAPHIC);

		VkImageMemoryBarrier2 barrier = {};
		barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.image               = img->images[0];
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask     = img->viewInfo.subresourceRange.aspectMask;
		barrier.subresourceRange.baseArrayLayer = layer;
		barrier.subresourceRange.layerCount     = 1;
		barrier.subresourceRange.levelCount     = 1;

		VkDependencyInfo dep = {};
		dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep.imageMemoryBarrierCount = 1;
		dep.pImageMemoryBarriers    = &barrier;

		uint32_t width = img->imageInfo.extent.width, height = img->imageInfo.extent.height;

		for(uint32_t i = 1; i < img->imageInfo.mipLevels; ++i)
		{
			/* mip[i-1]: TRANSFER_DST → TRANSFER_SRC (use ALL_TRANSFER: mip 0 is written by copy, rest by blit) */
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcStageMask   = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
			barrier.srcAccessMask  = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier.dstStageMask   = VK_PIPELINE_STAGE_2_BLIT_BIT;
			barrier.dstAccessMask  = VK_ACCESS_2_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier2(cmdBuffer.cmds[0], &dep);

			VkImageBlit blit = {};
			blit.srcOffsets[0] = (VkOffset3D){0, 0, 0};
			blit.srcOffsets[1] = (VkOffset3D){(int)width, (int)height, 1};
			blit.srcSubresource.aspectMask     = img->viewInfo.subresourceRange.aspectMask;
			blit.srcSubresource.mipLevel       = i - 1;
			blit.srcSubresource.baseArrayLayer = layer;
			blit.srcSubresource.layerCount     = 1;
			blit.dstOffsets[0] = (VkOffset3D){0, 0, 0};
			blit.dstOffsets[1] = (VkOffset3D){(int)width > 1 ? (int)width / 2 : 1, (int)height > 1 ? (int)height / 2 : 1, 1};
			blit.dstSubresource.aspectMask     = img->viewInfo.subresourceRange.aspectMask;
			blit.dstSubresource.mipLevel       = i;
			blit.dstSubresource.baseArrayLayer = layer;
			blit.dstSubresource.layerCount     = 1;
			vkCmdBlitImage(cmdBuffer.cmds[0],
					img->images[0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					img->images[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit, img->filter);

			/* mip[i-1]: TRANSFER_SRC → SHADER_READ_ONLY */
			barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			barrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
			vkCmdPipelineBarrier2(cmdBuffer.cmds[0], &dep);

			if(width  > 1) width  /= 2;
			if(height > 1) height /= 2;
		}

		/* last mip level: TRANSFER_DST → SHADER_READ_ONLY */
		barrier.subresourceRange.baseMipLevel = img->imageInfo.mipLevels - 1;
		barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcStageMask  = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		vkCmdPipelineBarrier2(cmdBuffer.cmds[0], &dep);

		bkpEndCmdBufferUniqUsage(adapter, &cmdBuffer);
	}
	img->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

/*____________________________________________________________________________________*/
void bkpCreateDefaultTexture(BkpGpuAdapter adapter, BkpImageResource * text, BkpBool isBlank)
{
	text->imageInfo = (VkImageCreateInfo){};
	text->imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	text->imageInfo.format 			  = VK_FORMAT_R8G8B8A8_SRGB;
	text->imageInfo.imageType		   = VK_IMAGE_TYPE_2D;
	text->imageInfo.tiling			  = VK_IMAGE_TILING_OPTIMAL;
	text->imageInfo.usage			   = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	text->filter			  = VK_FILTER_LINEAR;
	text->memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	text->bufferType = eBUFFER_GPU;
	text->mipType = eMIPMAP_USER_DEFINED;
	text->imageInfo.mipLevels		   = 3; //Warning should call function to generate mip
	text->imageInfo.arrayLayers		 = 1;
	text->imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	text->imageInfo.sharingMode		 = VK_SHARING_MODE_EXCLUSIVE;

	text->viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	text->viewInfo.pNext = NULL;
	text->viewInfo.flags = 0;
	text->viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	text->viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	text->viewInfo.components.r= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.components.g= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.components.b= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.components.a= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	text->viewInfo.subresourceRange.baseMipLevel = 0;
	text->viewInfo.subresourceRange.baseArrayLayer = 0;
	text->viewInfo.subresourceRange.layerCount = 1;

	stbi_uc * bitmap = NULL;

	VkDeviceSize size;

	uint32_t width  = 256;
	uint32_t height = 256;
	size_t TL = width * 4;
	size = width * height * 4;
	bitmap = (stbi_uc *) bkpAlloc(sizeof(stbi_uc) *size);

	if(isBlank == BKP_TRUE)
	{
		for (size_t i = 0; i < width; ++i)
		{
			size_t line = i * TL;
			for (size_t j = 0; j < TL; j += 4)
			{
				bitmap[line + j + 0] = 255;
				bitmap[line + j + 1] = 255;
				bitmap[line + j + 2] = 255;
				bitmap[line + j + 3] = 255;
			}
		}
	}
	else
	{
		uint8_t c1 = 192 , c2 = 0, ct;
		uint8_t c3 = c1;
		uint8_t c3default = 96;
		for (size_t i = 0; i < width; ++i)
		{
			size_t line = i * TL;
			for (size_t j = 0; j < TL; j += 4)
			{
				if(j % 64  == 0)
				{
					ct = c1;
					c1 = c2;
					c2 = ct;
					c3 = c1 == 0 ? c3default: c1;
				}
				bitmap[line + j + 0] = c1;
				bitmap[line + j + 1] = c1;
				bitmap[line + j + 2] = c3;
				bitmap[line + j + 3] = 255;
			}
			if(i % 32 == 0)
			{
				ct = c1;
				c1 = c2;
				c2 = ct;
				c3 = c1 == 0 ? c3default: c1;
			}
		}
	}

	text->hasAlpha = BKP_TRUE;

	bkpCreateTextureLayersFromData(adapter, &bitmap, 1, width, height, 1, text);

	bkpFree(bitmap);
}

/*____________________________________________________________________________________*/
void bkpCreateDefaultTextureLayers(BkpGpuAdapter adapter, BkpImageResource * text, int layerCount, BkpBool isBlank)
{
	text->imageInfo = (VkImageCreateInfo){};
	text->imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	text->imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	text->imageInfo.imageType		   = VK_IMAGE_TYPE_2D;
	text->imageInfo.tiling			  = VK_IMAGE_TILING_OPTIMAL;
	text->imageInfo.format			  = VK_FORMAT_R8G8B8A8_SRGB;
	text->imageInfo.usage			   = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	text->filter			  = VK_FILTER_LINEAR;
	text->memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	text->bufferType = eBUFFER_GPU;
	text->mipType = eMIPMAP_USER_DEFINED;
	text->imageInfo.mipLevels		   = 3;
	text->imageInfo.arrayLayers		 = layerCount;
	text->imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	text->imageInfo.sharingMode		 = VK_SHARING_MODE_EXCLUSIVE;

	text->viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	text->viewInfo.pNext = NULL;
	text->viewInfo.flags = 0;
	text->viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	text->viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	text->viewInfo.components.r= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.components.g= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.components.b= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.components.a= VK_COMPONENT_SWIZZLE_IDENTITY;
	text->viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	text->viewInfo.subresourceRange.baseMipLevel = 0;
	text->viewInfo.subresourceRange.baseArrayLayer = 0;
	text->viewInfo.subresourceRange.layerCount = layerCount;

	BkpArray(stbi_uc *) bitmaps = (BkpArray(stbi_uc *))bkpArrayCreateFrom(stbi_uc *, adapter->memoryGroupId);
	bkpArrayReserve(&bitmaps, layerCount);

	VkDeviceSize size;

	uint32_t width  = 256;
	uint32_t height = 256;
	size_t TL = width * 4;
	size = width * height * 4;

	for(size_t k = 0; k < layerCount; ++k)
	{
		stbi_uc * bitmap = (stbi_uc *) bkpAlloc(sizeof(stbi_uc) *size);
		if(isBlank == BKP_TRUE)
		{
			for (size_t i = 0; i < width; ++i)
			{
				size_t line = i * TL;
				for (size_t j = 0; j < TL; j += 4)
				{
					bitmap[line + j + 0] = 255;
					bitmap[line + j + 1] = 255;
					bitmap[line + j + 2] = 255;
					bitmap[line + j + 3] = 255;
				}
			}
		}
		else
		{
			uint8_t c1 = 192 , c2 = 0, ct;
			uint8_t c3 = c1;
			uint8_t c3default = 96;
			for (size_t i = 0; i < height; ++i)
			{
				size_t line = i * TL;
				for (size_t j = 0; j < TL; j += 4)
				{
					if(j % 64  == 0)
					{
						ct = c1;
						c1 = c2;
						c2 = ct;
						c3 = c1 == 0 ? c3default: c1;
					}
					bitmap[line + j + 0] = c1;
					bitmap[line + j + 1] = c1;
					bitmap[line + j + 2] = c3;
					bitmap[line + j + 3] = 255;
				}
				if(i % 32 == 0)
				{
					ct = c1;
					c1 = c2;
					c2 = ct;
					c3 = c1 == 0 ? c3default: c1;
				}
			}
		}

		text->hasAlpha = BKP_TRUE;
		bkpArrayPush(&bitmaps, bitmap);
	}

	bkpCreateTextureLayersFromData(adapter, bitmaps, layerCount, width, height, 1, text);

	for(size_t i = 0; i < layerCount; ++i)
	{
		bkpFree(bitmaps[i]);
	}

	bkpArrayDestroy(&bitmaps);
}

/*____________________________________________________________________________________*/
uint8_t bkpSaveSwapChainImage(BkpGpuAdapter adp, BkpSwapChain * res, uint32_t frame, const char * path, EImageType fileType)
{
  return saveImageToFile(adp, res->images[frame], res->info.imageFormat,
                 res->info.imageExtent.width, res->info.imageExtent.height,
                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_TILING_OPTIMAL, path, VK_IMAGE_ASPECT_COLOR_BIT, fileType);
}

/*____________________________________________________________________________________*/
uint8_t bkpSaveImage(BkpGpuAdapter adp, BkpImageResource * res, uint32_t frame, const char * path, EImageType fileType)
{
  VkImageAspectFlags asp = res->viewInfo.subresourceRange.aspectMask;
  return saveImageToFile(adp, res->images[frame], res->imageInfo.format,
                 res->imageInfo.extent.width, res->imageInfo.extent.height,
                 res->imageLayout, res->imageInfo.tiling, path, asp, fileType);
}

/*____________________________________________________________________________________*/
static uint8_t saveImageToFile(BkpGpuAdapter adp, VkImage srcImage, VkFormat format, int width, int height, VkImageLayout imageLayout, VkFormatFeatureFlags tiling, const char * path, VkImageAspectFlags aspect, EImageType imgType)
{
  uint8_t supportBlit = 1;
  VkFormatProperties formatProps;

  vkGetPhysicalDeviceFormatProperties(adp->gpu, format, &formatProps);

  VkFormatFeatureFlags  tilFeatures;
  if(tiling == VK_IMAGE_TILING_OPTIMAL)
  {
    tilFeatures =  formatProps.optimalTilingFeatures;
  }
  else
  {
    tilFeatures =  formatProps.linearTilingFeatures;
  }

  if(!(tilFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
  {
    supportBlit = 0;
		LOGC(eERROR, bkpTAG, bkpColor, "Device does not support blitting, using copy instead");
  }

  vkGetPhysicalDeviceFormatProperties(adp->gpu, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
  if(!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
  {
    supportBlit = 0;
		LOGC(eERROR, bkpTAG, bkpColor, "Device does not support linear blitting, using copy instead");
  }

  BkpImageResource tmpImage;
  tmpImage.imageInfo = (VkImageCreateInfo){};
  tmpImage.imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  tmpImage.imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  tmpImage.imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
  tmpImage.imageInfo.extent.width  = width;
  tmpImage.imageInfo.extent.height = height;
  tmpImage.imageInfo.extent.depth  = 1;
  tmpImage.imageInfo.mipLevels     = 1;
  tmpImage.imageInfo.arrayLayers   = 1;
  tmpImage.imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  tmpImage.imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  tmpImage.imageInfo.tiling        = VK_IMAGE_TILING_LINEAR;
  tmpImage.imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  tmpImage.imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  tmpImage.bufferType              = eBUFFER_GPU_CPU;

  tmpImage.imageMemories = NULL;
  bkpCreateImage(adp, &tmpImage);
  VkImage dstImage = tmpImage.images[0];
  VkDeviceMemory dstImageMemory = tmpImage.imageMemories[0];


	BkpCommandBuffer cmdBuffer;
	bkpBeginCmdBufferUniqUsage(adp, &cmdBuffer, eQFAMILY_GRAPHIC);

  {
    BkpImageBarrierInfo preCopy[2] = {
        { .image = dstImage,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          .srcStage = VK_PIPELINE_STAGE_2_NONE,              .srcAccess = VK_ACCESS_2_NONE,
          .dstStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,  .dstAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
          .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 },
        { .image = srcImage,
          .oldLayout = imageLayout, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          .srcStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,  .srcAccess = VK_ACCESS_2_MEMORY_READ_BIT,
          .dstStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,  .dstAccess = VK_ACCESS_2_TRANSFER_READ_BIT,
          .aspect = aspect, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 }
    };
    bkpCmdBarrierImages(cmdBuffer.cmds[0], preCopy, 2);
  }
  
  if(supportBlit)
  {
    VkOffset3D blitSize;
    blitSize.x = width;
    blitSize.y = height;
    blitSize.z = 1;
    VkImageBlit imageBlitRegion = (VkImageBlit){};
    imageBlitRegion.srcSubresource.aspectMask = aspect;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = blitSize;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = blitSize;

    vkCmdBlitImage(
      cmdBuffer.cmds[0],
      srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageBlitRegion,
      VK_FILTER_NEAREST);
  }
  else
{
    VkImageCopy imageCopyRegion = {};
    imageCopyRegion.srcSubresource.aspectMask = aspect;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = width;
    imageCopyRegion.extent.height = height;
    imageCopyRegion.extent.depth = 1;

    vkCmdCopyImage(
      cmdBuffer.cmds[0],
      srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageCopyRegion);
  }

  {
    BkpImageBarrierInfo postCopy[2] = {
        { .image = dstImage,
          .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_GENERAL,
          .srcStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,  .srcAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
          .dstStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,  .dstAccess = VK_ACCESS_2_MEMORY_READ_BIT,
          .aspect = VK_IMAGE_ASPECT_COLOR_BIT, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 },
        { .image = srcImage,
          .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .newLayout = imageLayout,
          .srcStage = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,  .srcAccess = VK_ACCESS_2_TRANSFER_READ_BIT,
          .dstStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,  .dstAccess = VK_ACCESS_2_MEMORY_READ_BIT,
          .aspect = aspect, .baseMip = 0, .mipCount = 1, .baseLayer = 0, .layerCount = 1 }
    };
    bkpCmdBarrierImages(cmdBuffer.cmds[0], postCopy, 2);
  }


  bkpEndCmdBufferUniqUsage(adp, &cmdBuffer);


  VkImageSubresource subR = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
  VkSubresourceLayout subL;
  vkGetImageSubresourceLayout(adp->device, dstImage, &subR, &subL);

	char * bitmap = NULL;
  vkMapMemory(adp->device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void **)&bitmap);
  bitmap += subL.offset;

  BkpBool colorSwizzle = BKP_FALSE;
  if(!supportBlit)
  {
    if(VK_FORMAT_B8G8R8A8_SRGB == format || VK_FORMAT_B8G8R8A8_UNORM == format || VK_FORMAT_B8G8R8A8_SNORM == format)
    {
      colorSwizzle = BKP_TRUE;
      char * data = bitmap;
      for (uint32_t y = 0; y < height; y++)
      {
        unsigned int *row = (unsigned int*)data;
        for (uint32_t x = 0; x < width; x++)
        {
          unsigned int tmp = *row;
          *row = *(row + 2);
          *(row + 2) = tmp;
          row++;
        }
        data += subL.rowPitch;
      }

    }
  }


  char * data = bitmap;
  BkpBool ret = BKP_FALSE;
  if(imgType == eBKP_IMAGE_TYPE_PPM)
  {
    ret = savePPM(path, data, width, height, colorSwizzle, subL.rowPitch);
  }
  else if(imgType == eBKP_IMAGE_TYPE_PNG)
  {
    ret = savePNG(path, data, width, height, colorSwizzle, subL.rowPitch);
  }

  vkUnmapMemory(adp->device, dstImageMemory);
  bkpDestroyImageResource(adp, &tmpImage);
  
  return ret;
}

static uint8_t savePPM(const char * path, char * data, int width , int height, BkpBool colorSwizzle, VkDeviceSize size)
{
  size_t s = strlen(path) + 5;
  char * fname = (char *) bkpAlloc(sizeof(char) * s);
  snprintf(fname, s, "%s.ppm\n", path);

  FILE * ou = fopen(fname, "w");
  bkpFree(fname);

  if(ou == NULL)
  {
    return BKP_FALSE;
  }

  char buff[32];
  sprintf(buff, "P6\n%d\n%d\n255\n",width, height);
  fwrite(buff, sizeof(char), strlen(buff), ou);

  for (uint32_t y = 0; y < height; y++)
  {
    unsigned int *row = (unsigned int*)data;
    for (uint32_t x = 0; x < width; x++)
    {
      if(colorSwizzle)
      {
        fwrite((char *)row + 2, sizeof(char), 1, ou);
        fwrite((char *)row + 1, sizeof(char), 1, ou);
        fwrite((char *)row, sizeof(char), 1, ou);
      }
      else
      {
        fwrite(row, sizeof(char), 3, ou);
      }
      row++;
    }
    data += size;
  }
  fclose(ou);

  return BKP_TRUE;
}

/*____________________________________________________________________________________*/
static uint8_t savePNG(const char * path, char * data, int width , int height, BkpBool colorSwizzle, VkDeviceSize size)
{
  size_t s = strlen(path) + 5;
  char * fname = (char *) bkpAlloc(sizeof(char) * s);
  snprintf(fname, s, "%s.png\n", path);

  stbi_uc * toPng = (stbi_uc *)bkpAlloc(sizeof(stbi_uc) * width * height * 4);
  for (uint32_t y = 0; y < height; y++)
  {
    unsigned int *row = (unsigned int*)data;
    uint32_t dx = 0;
    for (uint32_t x = 0; x < width; x++, dx+=4)
    {
      stbi_uc * p = (stbi_uc *) row;
      if(colorSwizzle)
      {
        toPng[width * 4 * y + dx] = p[2];
        toPng[width * 4 * y + dx + 2] = p[0];
      }
      else
      {
        toPng[width * 4 * y + dx] = p[0];
        toPng[width * 4 * y + dx + 2] = p[2];
      }
      toPng[width * 4 * y + dx + 1] = p[1];
      toPng[width * 4 * y + dx + 3] = 255;
      row++;
    }
    data += size;
  }

  BkpBool ret = stbi_write_png(fname, width, height, 4, toPng, width * 4);
  bkpFree(fname);
  bkpFree(toPng);
  return ret;
}

/*____________________________________________________________________________________*/
uint8_t * bkpGenerateImageDataRGBA(int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a, VkDeviceSize * size)
{
	uint8_t * bitmap = NULL;
	size_t TL = width * 4;
	VkDeviceSize sizeT = width * height * 4;

	bitmap = (uint8_t *) bkpAlloc(sizeof(uint8_t) * sizeT);

	for (size_t i = 0; i < height; ++i)
	{
		size_t line = i * TL;
		for (size_t j = 0; j < TL; j += 4)
		{
			bitmap[line + j + 0] = r;
			bitmap[line + j + 1] = g;
			bitmap[line + j + 2] = b;
			bitmap[line + j + 3] = a;
		}
	}

	*size = sizeT;
	return bitmap;
}

/*____________________________________________________________________________________*/
uint8_t * bkpGenerateImageDataDamier(int width, int height, int squaresPerSide,
                                     uint8_t r1, uint8_t g1, uint8_t b1,
                                     uint8_t r2, uint8_t g2, uint8_t b2,
                                     VkDeviceSize * size)
{
	VkDeviceSize sizeT = (VkDeviceSize)width * height * 4;
	uint8_t * bitmap = (uint8_t *)bkpAlloc(sizeof(uint8_t) * sizeT);

	int tileW = width  / squaresPerSide;
	int tileH = height / squaresPerSide;
	if(tileW < 1) tileW = 1;
	if(tileH < 1) tileH = 1;

	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x)
		{
			int light = ((x / tileW) + (y / tileH)) % 2 == 0;
			int idx = (y * width + x) * 4;
			bitmap[idx + 0] = light ? r1 : r2;
			bitmap[idx + 1] = light ? g1 : g2;
			bitmap[idx + 2] = light ? b1 : b2;
			bitmap[idx + 3] = 255;
		}
	}

	*size = sizeT;
	return bitmap;
}

/*____________________________________________________________________________________*/
uint8_t * bkpGenerateVerticalGradianImage(int width, int height, BkpColorGradianInfo * grad, size_t gradCount, VkDeviceSize * size)
{
    uint8_t * bitmap = NULL;
    size_t TL = width * 4;
    VkDeviceSize sizeT = width * height * 4;
    bitmap = (uint8_t *) bkpAlloc(sizeof(uint8_t) * sizeT);
    size_t currentLine = 0;

    for(size_t count = 0; count < gradCount; ++count)
    {
        uint8_t * pColUp = grad[count].color1;
        uint8_t * pColDown = grad[count].color2;
        if(grad[count].end < grad[count].begin)
        {
            pColUp = grad[count].color2;
            pColDown = grad[count].color1;
        }

        size_t lineBegin = height * grad[count].begin;
        size_t lineEnd = height * grad[count].end;
        float gradLines = lineEnd - lineBegin; 
        float deltaR = (float)(pColDown[0] - pColUp[0]) / gradLines;
        float deltaG = (float)(pColDown[1] - pColUp[1]) / gradLines;
        float deltaB = (float)(pColDown[2] - pColUp[2]) / gradLines;
        float deltaA = (float)(pColDown[3] - pColUp[3]) / gradLines;

        float r = pColUp[0];
        float g = pColUp[1];
        float b = pColUp[2];
        float a = pColUp[3];

        for (size_t i = currentLine; i < lineBegin; ++i)
        {
            size_t line = i * TL;
            for (size_t j = 0; j < TL; j += 4)
            {
                bitmap[line + j + 0] = pColUp[0];
                bitmap[line + j + 1] = pColUp[1];
                bitmap[line + j + 2] = pColUp[2];
                bitmap[line + j + 3] = pColUp[3];
            }
        }
        for (size_t i = lineBegin; i < lineEnd; ++i)
        {
            size_t line = i * TL;
            r += deltaR;
            g += deltaG;
            b += deltaB;
            a += deltaA;
      /*
            r = pColUp[0] + (pColDown[0] - pColUp[0]) * ((i - lineBegin) *(1 / gradLines));
            g = pColUp[1] + (pColDown[1] - pColUp[1]) * ((i - lineBegin) * (1 / gradLines));
            b = pColUp[2] + (pColDown[2] - pColUp[2]) * ((i - lineBegin) * (1 / gradLines));
            a = 255;
            */
            for (size_t j = 0; j < TL; j += 4)
            {
                bitmap[line + j + 0] =  (uint8_t)r;
                bitmap[line + j + 1] =  (uint8_t)g;
                bitmap[line + j + 2] =  (uint8_t)b;
                bitmap[line + j + 3] =  (uint8_t)a;
            }
        }
        currentLine = lineEnd;
        for (size_t i = lineEnd; i < height; ++i)
        {
            size_t line = i * TL;
            for (size_t j = 0; j < TL; j += 4)
            {
                bitmap[line + j + 0] = pColDown[0];
                bitmap[line + j + 1] = pColDown[1];
                bitmap[line + j + 2] = pColDown[2];
                bitmap[line + j + 3] = pColDown[3];
            }
        }
    }

    *size = sizeT;
    return bitmap;
}

/*____________________________________________________________________________________*/
void bkpCreateTextureLayersFromData(BkpGpuAdapter adapter, uint8_t ** bitmaps, size_t bitmapCount, int width, int height, int depth, BkpImageResource * text)
{
	VkDeviceSize layerSize = width * height * 4;
	VkDeviceSize size = layerSize * bitmapCount;
	BkpBuffer staginRes;

	if(text->mipType == eMIPMAP_RUNTIME_GENERATE)
	{
		uint32_t v = text->imageInfo.extent.width > text->imageInfo.extent.height ? text->imageInfo.extent.width : text->imageInfo.extent.height;
		text->imageInfo.mipLevels = (uint32_t)floor(log2(v)) + 1;
	}
	else if(text->mipType == eMIPMAP_USER_DEFINED)
	{
		uint32_t vUD = (uint32_t)(width > height ? width : height);
		uint32_t maxMip = vUD > 0 ? (uint32_t)floor(log2(vUD)) + 1 : 1;
		if(text->imageInfo.mipLevels < 1) text->imageInfo.mipLevels = 1;
		if(text->imageInfo.mipLevels > maxMip) text->imageInfo.mipLevels = maxMip;
	}
	else if(text->mipType == eMIPMAP_NONE)
	{
		text->imageInfo.mipLevels = 1;
	}
	else
	{
		LOGC(eWARNING, bkpTAG, bkpColor, "mipmap from file NOT implemented yet set mipmap to 1!");
		text->imageInfo.mipLevels = 1;
	}

	if(bitmapCount > 1)
	{
		//text->viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}

	text->imageInfo.extent.width = width;
	text->imageInfo.extent.height = height;
	text->imageInfo.extent.depth  = depth;
	text->imageInfo.arrayLayers = bitmapCount;
	text->viewInfo.subresourceRange.levelCount = text->imageInfo.mipLevels;
	text->viewInfo.subresourceRange.layerCount = bitmapCount;

	createStaggingImageBuffer(adapter, bitmaps, bitmapCount, layerSize, &staginRes, size);
	bkpCreateTextureLayers(adapter, text, staginRes);
	bkpFreeBuffer(adapter, &staginRes);
}

/*____________________________________________________________________________________*/
void bkpSetDefaultCubemapInfo(BkpImageResource * res, VkFormat fmt)
{
	bkpSetDefaultTextureInfo(res);
	res->imageInfo.flags   = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	res->imageInfo.format  = fmt;
	res->viewInfo.format   = fmt;
	res->viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	res->imageInfo.mipLevels = 1;
	res->mipType             = eMIPMAP_NONE;
	res->hasAlpha            = BKP_FALSE;
}

/*____________________________________________________________________________________*/
void bkpCreateCubemapFromData(BkpGpuAdapter adapter, uint8_t * faces[6], int faceSize, BkpImageResource * tex)
{
	tex->imageInfo.flags  |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	tex->viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	bkpCreateTextureLayersFromData(adapter, faces, 6, faceSize, faceSize, 1, tex);
}

/*____________________________________________________________________________________*/
BkpBool bkpCreateCubemapFromFiles(BkpGpuAdapter adapter, const char * paths[6], BkpImageResource * tex)
{
	int width, height, channel;
	stbi_uc * bitmaps[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

	bitmaps[0] = stbi_load(paths[0], &width, &height, &channel, STBI_rgb_alpha);
	if(bitmaps[0] == NULL)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Cubemap: failed to load face 0 '%s': %s", paths[0], stbi_failure_reason());
		return BKP_FALSE;
	}
	LOGC(eDEBUG, bkpTAG, bkpColor, "Cubemap face 0 loaded: '%s'", paths[0]);

	for(int i = 1; i < 6; ++i)
	{
		int w, h, c;
		bitmaps[i] = stbi_load(paths[i], &w, &h, &c, STBI_rgb_alpha);
		if(bitmaps[i] == NULL || w != width || h != height)
		{
			LOGC(eERROR, bkpTAG, bkpColor, "Cubemap: failed to load face %d '%s': %s", i, paths[i], stbi_failure_reason());
			if(bitmaps[i] != NULL)
			{
				stbi_image_free(bitmaps[i]);
				bitmaps[i] = NULL;
				LOGC(eERROR, bkpTAG, bkpColor,
				     "Cubemap: face %d has incompatible size (%dx%d vs %dx%d)", i, w, h, width, height);
			}
			for(int ii = 0; ii < i; ++ii) { if(bitmaps[ii]) stbi_image_free(bitmaps[ii]); }
			return BKP_FALSE;
		}
		LOGC(eDEBUG, bkpTAG, bkpColor, "Cubemap face %d loaded: '%s'", i, paths[i]);
	}

	tex->imageInfo.flags  |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	tex->viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	bkpCreateTextureLayersFromData(adapter, bitmaps, 6, width, height, 1, tex);

	for(int i = 0; i < 6; ++i) { if(bitmaps[i]) stbi_image_free(bitmaps[i]); }
	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
BkpBool bkpCreateTextureFromMemory(BkpGpuAdapter adapter, BkpMemInfo * buffMemInfo, BkpImageResource * text)
{

	int width, height, depth = 1, channel;
	stbi_uc * bitmap = NULL;
	uint8_t * data = ((uint8_t *)buffMemInfo->buffer) + buffMemInfo->offset;

	if(data != NULL)
	{
		bitmap = stbi_load_from_memory(data, buffMemInfo->size, &width, &height, &channel, STBI_rgb_alpha);
	}

	if(bitmap == NULL)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Failed to load memory image  %s", stbi_failure_reason());
		return BKP_FALSE;
	}

	text->hasAlpha = ((channel == 4) || (channel == 2)) ?  BKP_TRUE : BKP_FALSE;
	bkpCreateTextureLayersFromData(adapter, &bitmap, 1, width, height, depth, text);

    stbi_image_free(bitmap);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Texture memory loaded!");
	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
BkpBool bkpCreateTextureLayersFromMemory(BkpGpuAdapter adapter, BkpMemInfo * buffMemInfo, size_t memInfoCount, BkpImageResource * text)
{
	BkpArray(stbi_uc *) bitmaps = (BkpArray(stbi_uc *))bkpArrayCreateFrom(stbi_uc *, adapter->memoryGroupId);;
	bkpArrayReserve(&bitmaps, memInfoCount);

	int width, height, depth = 1, channel;
	uint8_t * data = ((uint8_t *)buffMemInfo[0].buffer) + buffMemInfo[0].offset;
	stbi_uc * bitmap = NULL;

	if(data == NULL) return BKP_FALSE;

	bitmap = stbi_load_from_memory(data, buffMemInfo[0].size, &width, &height, &channel, STBI_rgb_alpha);


	if(bitmap == NULL)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Failed to load memory image  %s", stbi_failure_reason());
		bkpArrayDestroy(&bitmaps);
		return BKP_FALSE;
	}
	bkpArrayPush(&bitmaps, bitmap);

	for(size_t i = 1; i < memInfoCount ; ++i)
	{
		int width_, height_, channel_;
		data = ((uint8_t *)buffMemInfo[i].buffer) + buffMemInfo[i].offset;

		if(data == NULL)
		{
			for(size_t k = 0; k < bkpArraySize(bitmaps); ++k)
			{
				stbi_image_free(bitmaps[k]);
			}
		}

		bitmap = stbi_load_from_memory(data, buffMemInfo[i].size, &width_, &height_, &channel_, STBI_rgb_alpha);

		if(bitmap == NULL || width != width_ || height != height_)
		{
			LOGC(eERROR, bkpTAG, bkpColor, "Loading texture buffer!");
			if(bitmap != NULL)
			{
				stbi_image_free(bitmap);
				LOGC(eERROR, bkpTAG, bkpColor, "Loading incompatible layer dimensions! w(%u, %u), h(%u, %u), c(%u, %u)",
						width, width_, height, height_, channel, channel_);
			}
			for(size_t k = 0; k < bkpArraySize(bitmaps); ++k)
			{
				bkpArrayDestroy(&bitmaps);
				stbi_image_free(bitmaps[k]);
			}
			return BKP_FALSE;
		}
		bkpArrayPush(&bitmaps, bitmap);
	}

	text->hasAlpha = ((channel == 4) || (channel == 2)) ?  BKP_TRUE : BKP_FALSE;

	bkpCreateTextureLayersFromData(adapter, bitmaps, memInfoCount, width, height, depth, text);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Texture memory loaded!");
    for(size_t k = 0; k < bkpArraySize(bitmaps); ++k)
    {
        stbi_image_free(bitmaps[k]);
    }
	bkpArrayDestroy(&bitmaps);
	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
BkpBool bkpCreateTextureLayersFromBitmap(BkpGpuAdapter adapter, BkpMemInfo * buffMemInfo, size_t memInfoCount, int width, int height, BkpBool hasAlpha, BkpImageResource * text)
{
	BkpArray(stbi_uc *) bitmaps = (BkpArray(stbi_uc *))bkpArrayCreateFrom(stbi_uc *, adapter->memoryGroupId);;
	bkpArrayResize(&bitmaps, memInfoCount);

	for(size_t i = 0; i < memInfoCount ; ++i)
	{
		bitmaps[i] = buffMemInfo[i].buffer;
	}

	text->hasAlpha = hasAlpha;

	bkpCreateTextureLayersFromData(adapter, bitmaps, memInfoCount, width, height, 1, text);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Texture memory loaded!");
	bkpArrayDestroy(&bitmaps);

	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
BkpBool bkpCreateTextureFromFile(BkpGpuAdapter adapter, const char * path, BkpImageResource * text)
{
	int width, height, channel;
	stbi_uc * bitmap = NULL;

	if(path != NULL)
	{
		bitmap = stbi_load(path, &width, &height, &channel, STBI_rgb_alpha);
	}

	if(bitmap == NULL)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Failed to load image '%s' %s", path, stbi_failure_reason());
		return BKP_FALSE;
	}
	text->hasAlpha = ((channel == 4) || (channel == 2)) ?  BKP_TRUE : BKP_FALSE;

	bkpCreateTextureLayersFromData(adapter, &bitmap, 1, width, height, 1, text);
	stbi_image_free(bitmap);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Texture '%s' loaded!", path);

	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
static void createStaggingImageBuffer(BkpGpuAdapter adapter, stbi_uc ** bitmaps, size_t bitmapCount, VkDeviceSize layerSize, BkpBuffer * stagging, VkDeviceSize size)
{

	BkpBufferInfo info = {};
	info.usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.type = eBUFFER_CPU_GPU;
	info.size = size;
	info.queueIndices[0] = adapter->graphicQueue.index;
	info.isImage = BKP_FALSE;
	info.qCount = 1;


	if(adapter->transfertQueue.isSet)
	{
		if(adapter->graphicQueue.index != adapter->transfertQueue.index)
		{
			info.queueIndices[1] = adapter->transfertQueue.index;
			++info.qCount;
		}
	}

	bkpAllocateBuffer(adapter, stagging, &info);
	bkpMapBuffer(adapter, *stagging);
	for(size_t i = 0; i < bitmapCount; ++i)
	{
		bkpUploadBufferData(adapter, *stagging, bitmaps[i], layerSize * i, layerSize);
	}
	bkpUnmapBuffer(adapter, *stagging);
}

/*____________________________________________________________________________________*/
void bkpCreateTexture(BkpGpuAdapter adapter, BkpImageResource * text, BkpBuffer staginRes)
{
	text->imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	bkpCreateImage(adapter, text);

	text->viewInfo.subresourceRange.levelCount = text->imageInfo.mipLevels;

	bkpImageTransition(adapter, text->images[0], text->imageInfo.mipLevels,
			text->imageInfo.arrayLayers, text->viewInfo.subresourceRange.aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBuffer b;
	VkDeviceSize boffset;
	bkpGetBuffer(staginRes, &b);
	bkpGetBufferOffset(staginRes, &boffset);
	bkpCopyBufferToImageLayers(adapter, b, boffset, text->images[0], text->viewInfo.subresourceRange.aspectMask,
			text->imageInfo.extent.width, text->imageInfo.extent.height, text->imageInfo.extent.depth);

	if(text->mipType != eMIPMAP_NONE)
	{
		bkpGenerateMipmaps(adapter, text);
	}
	else
	{
		bkpImageTransition(adapter, text->images[0], 1, text->imageInfo.arrayLayers,
				text->viewInfo.subresourceRange.aspectMask,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		text->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	text->imageViews = (BkpArray(VkImageView))bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);
	bkpArrayPush(&text->imageViews, createImageView(adapter->device, text->images[0], &text->viewInfo, adapter->allocator));
}

/*____________________________________________________________________________________*/
void bkpCreateTextureLayers(BkpGpuAdapter adapter, BkpImageResource * text, BkpBuffer staginRes)
{
	text->imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	bkpCreateImage(adapter, text);

	bkpImageTransition(adapter, text->images[0], text->imageInfo.mipLevels,
			text->imageInfo.arrayLayers, text->viewInfo.subresourceRange.aspectMask,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBuffer b;
	VkDeviceSize boffset;
	bkpGetBuffer(staginRes, &b);
	bkpGetBufferOffset(staginRes, &boffset);
	bkpCopyBufferToImageLayers(adapter, b, boffset, text->images[0], text->viewInfo.subresourceRange.aspectMask,
			text->imageInfo.extent.width, text->imageInfo.extent.height, text->viewInfo.subresourceRange.layerCount);

	if(text->mipType != eMIPMAP_NONE)
	{
		bkpGenerateMipmaps(adapter, text);
	}
	else
	{
		bkpImageTransition(adapter, text->images[0], 1, text->imageInfo.arrayLayers,
				text->viewInfo.subresourceRange.aspectMask,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		text->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	text->imageViews = (BkpArray(VkImageView))bkpArrayCreateFrom(VkImageView, adapter->memoryGroupId);
	bkpArrayPush(&text->imageViews, createImageView(adapter->device, text->images[0], &text->viewInfo, adapter->allocator));
}

/*____________________________________________________________________________________*/
void bkpBeginRenderPass(VkCommandBuffer commandBuffer, BkpRenderTarget * rt, uint32_t image_index)
{
	rt->beginInfo.framebuffer = rt->frameBuffers[image_index];
	vkCmdBeginRenderPass(commandBuffer, &rt->beginInfo, rt->passContents);
}

/*____________________________________________________________________________________*/
void bkpEndRenderPass(BkpCommandBuffer * commandBuffer, uint32_t image_index)
{
	vkCmdEndRenderPass(commandBuffer->cmds[image_index]);
}

/*____________________________________________________________________________________*/
BkpBool bkpCreateTextureLayersFromFile(BkpGpuAdapter adapter, const char ** path, size_t pathCount, BkpImageResource * text)
{
	int width, height, channel;
	BkpArray(stbi_uc *) bitmaps = (BkpArray(stbi_uc *))bkpArrayCreateFrom(stbi_uc *, adapter->memoryGroupId);
	bkpArrayReserve(&bitmaps, pathCount);

	stbi_uc * bitmap = stbi_load(path[0], &width, &height, &channel, STBI_rgb_alpha);

	if(bitmap == NULL)
	{
		LOGC(eERROR, bkpTAG, bkpColor, "Failed to load image '%s' %s", path[0], stbi_failure_reason());
		return BKP_FALSE;
	}

	bkpArrayPush(&bitmaps, bitmap);

	LOGC(eDEBUG, bkpTAG, bkpColor, "Texture '%s' loaded!", path[0]);

	for(size_t i = 1; i < pathCount; ++i)
	{
		int width_, height_, channel_;
		bitmap = stbi_load(path[i], &width_, &height_, &channel_, STBI_rgb_alpha);
		if(bitmap == NULL || width != width_ || height != height_)
		{
			LOGC(eERROR, bkpTAG, bkpColor, "Loading '%s' texture error {%s}", path[i], stbi_failure_reason());
			if(bitmap != NULL)
			{
				stbi_image_free(bitmap);
				LOGC(eERROR, bkpTAG, bkpColor, "Loading '%s' incompatible layer dimensions! w(%u, %u), h(%u, %u), c(%u, %u)", path[i],
						width, width_, height, height_, channel, channel_);
			}
			for(size_t ii = 0; ii < i; ++ii)
			{
				stbi_image_free(bitmaps[ii]);
			}
			bkpArrayDestroy(&bitmaps);
			return BKP_FALSE;
		}
		bkpArrayPush(&bitmaps, bitmap);
		LOGC(eDEBUG, bkpTAG, bkpColor, "Texture '%s' loaded!", path[i]);
	}

	bkpCreateTextureLayersFromData(adapter, bitmaps, pathCount, width, height, 1, text);

	for(size_t i = 1; i < pathCount; ++i)
	{
		stbi_image_free(bitmaps[i]);
	}

	bkpArrayDestroy(&bitmaps);
	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
void bkpImageTransition(BkpGpuAdapter adapter, VkImage image, uint32_t mipLevels, uint32_t arrayLayers, VkImageAspectFlags aspectMask, VkImageLayout old, VkImageLayout nouveau)
{
	BkpCommandBuffer cmdBuffer;
	bkpBeginCmdBufferUniqUsage(adapter, &cmdBuffer, eQFAMILY_GRAPHIC);

	VkImageMemoryBarrier2 barrier = {};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.oldLayout           = old;
	barrier.newLayout           = nouveau;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = image;
	barrier.subresourceRange.aspectMask     = aspectMask;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = arrayLayers;

	if(old == VK_IMAGE_LAYOUT_UNDEFINED && nouveau == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	}
	else if(old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && nouveau == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	}
	else
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unsupported layout transition!");
	}

	VkDependencyInfo dep = {};
	dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dep.imageMemoryBarrierCount = 1;
	dep.pImageMemoryBarriers    = &barrier;
	vkCmdPipelineBarrier2(cmdBuffer.cmds[0], &dep);

	bkpEndCmdBufferUniqUsage(adapter, &cmdBuffer);
}

/*____________________________________________________________________________________*/
/*
void bkpCopyBufferToImage(BkpVulkanContext * ctx, VkBuffer buffer, VkImage image, VkBufferImageCopy region)
{
	BkpCommandBuffer commandBuffer;

	bkpBeginCmdBufferUniqUsage(ctx->adapter, &commandBuffer, eQFAMILY_TRANSFERT);
	vkCmdCopyBufferToImage(commandBuffer.cmds[0], buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	bkpEndCmdBufferUniqUsage(ctx->adapter, &commandBuffer);
}
*/

/*____________________________________________________________________________________*/
void bkpCopyBufferToImageLayers(BkpGpuAdapter adapter, VkBuffer buffer, VkDeviceSize offset, VkImage image, VkImageAspectFlags aspectMask, uint32_t width, uint32_t height, uint32_t layerCount)
{
	BkpCommandBuffer commandBuffer;
	bkpBeginCmdBufferUniqUsage(adapter, &commandBuffer, eQFAMILY_TRANSFERT);

	VkBufferImageCopy region = {};
	region.bufferOffset = offset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = aspectMask;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = (VkOffset3D){ 0, 0, 0 };
	region.imageExtent = (VkExtent3D){ width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer.cmds[0], buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	bkpEndCmdBufferUniqUsage(adapter, &commandBuffer);
}

/*____________________________________________________________________________________*/
void bkpUploadTextureLayer(BkpGpuAdapter adapter, BkpImageResource * tex,
                           uint32_t layer, void * data, VkDeviceSize layerSize)
{
	/* Staging buffer */
	BkpBuffer staging;
	BkpBufferInfo binfo = {};
	binfo.usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	binfo.type            = eBUFFER_CPU_GPU;
	binfo.size            = layerSize;
	binfo.queueIndices[0] = adapter->graphicQueue.index;
	binfo.isImage         = BKP_FALSE;
	binfo.qCount          = 1;
	if(adapter->transfertQueue.isSet && adapter->graphicQueue.index != adapter->transfertQueue.index)
	{
		binfo.queueIndices[1] = adapter->transfertQueue.index;
		++binfo.qCount;
	}
	bkpAllocateBuffer(adapter, &staging, &binfo);
	bkpMapBuffer(adapter, staging);
	bkpUploadBufferData(adapter, staging, data, 0, layerSize);
	bkpUnmapBuffer(adapter, staging);

	/* Transition target layer: shader read → transfer dst */
	{
		BkpCommandBuffer cmd;
		bkpBeginCmdBufferUniqUsage(adapter, &cmd, eQFAMILY_GRAPHIC);
		VkImageMemoryBarrier2 b = {};
		b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		b.oldLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		b.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.image               = tex->images[0];
		b.srcStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		b.srcAccessMask       = VK_ACCESS_2_SHADER_READ_BIT;
		b.dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
		b.dstAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		b.subresourceRange.baseMipLevel   = 0;
		b.subresourceRange.levelCount     = 1;
		b.subresourceRange.baseArrayLayer = layer;
		b.subresourceRange.layerCount     = 1;
		VkDependencyInfo dep = {};
		dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep.imageMemoryBarrierCount = 1;
		dep.pImageMemoryBarriers    = &b;
		vkCmdPipelineBarrier2(cmd.cmds[0], &dep);
		bkpEndCmdBufferUniqUsage(adapter, &cmd);
	}

	/* Copy staging buffer into the specific array layer */
	{
		BkpCommandBuffer cmd;
		bkpBeginCmdBufferUniqUsage(adapter, &cmd, eQFAMILY_TRANSFERT);
		VkBuffer buf; VkDeviceSize boff;
		bkpGetBuffer(staging, &buf);
		bkpGetBufferOffset(staging, &boff);
		VkBufferImageCopy region = {};
		region.bufferOffset                    = boff;
		region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel       = 0;
		region.imageSubresource.baseArrayLayer = layer;
		region.imageSubresource.layerCount     = 1;
		region.imageOffset                     = (VkOffset3D){0, 0, 0};
		region.imageExtent                     = (VkExtent3D){tex->imageInfo.extent.width,
		                                                      tex->imageInfo.extent.height, 1};
		vkCmdCopyBufferToImage(cmd.cmds[0], buf, tex->images[0],
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		bkpEndCmdBufferUniqUsage(adapter, &cmd);
	}

	/* Transition layer back: transfer dst → shader read */
	{
		BkpCommandBuffer cmd;
		bkpBeginCmdBufferUniqUsage(adapter, &cmd, eQFAMILY_GRAPHIC);
		VkImageMemoryBarrier2 b = {};
		b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		b.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		b.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.image               = tex->images[0];
		b.srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
		b.srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		b.dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		b.dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT;
		b.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		b.subresourceRange.baseMipLevel   = 0;
		b.subresourceRange.levelCount     = 1;
		b.subresourceRange.baseArrayLayer = layer;
		b.subresourceRange.layerCount     = 1;
		VkDependencyInfo dep = {};
		dep.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep.imageMemoryBarrierCount = 1;
		dep.pImageMemoryBarriers    = &b;
		vkCmdPipelineBarrier2(cmd.cmds[0], &dep);
		bkpEndCmdBufferUniqUsage(adapter, &cmd);
	}

	bkpFreeBuffer(adapter, &staging);
}

/*____________________________________________________________________________________*/
void bkpDestroyCommandPool(BkpGpuAdapter adapter, BkpCommandPool * cmdpool)
{
	vkDestroyCommandPool(adapter->device, cmdpool->commandPool, adapter->allocator);
}

/*____________________________________________________________________________________*/
void bkpDestroyImageResource(BkpGpuAdapter adapter, BkpImageResource * imgRes)
{
	if(imgRes == NULL) return;

  if(imgRes->imageViews)
  {
    size_t viewCount = bkpArraySize(imgRes->imageViews);
    for (size_t i = 0; i < viewCount; ++i)
    {
      vkDestroyImageView(adapter->device, imgRes->imageViews[i], adapter->allocator);
    }
    bkpArrayDestroy(&imgRes->imageViews);
  }
  if(imgRes->bufferImages)
  {
    size_t count = bkpArraySize(imgRes->bufferImages);
    for (size_t i = 0; i < count; ++i)
      bkpFreeBufferImage(adapter, &imgRes->bufferImages[i]);
    bkpArrayDestroy(&imgRes->bufferImages);
  }
  if(imgRes->images)        bkpArrayDestroy(&imgRes->images);
  if(imgRes->imageMemories) bkpArrayDestroy(&imgRes->imageMemories);
}

/*____________________________________________________________________________________*/
void bkpDestroyRenderTarget(BkpGpuAdapter adapter, BkpRenderTarget * rt)
{
	if(rt == NULL) return;

	if(rt->hasFrameBuffers)
	{
		bkpDestroyFrameBuffers(adapter, rt->frameBuffers, (uint32_t)bkpArraySize(rt->frameBuffers));
		bkpArrayDestroy(&rt->frameBuffers);
	}

	vkDestroyRenderPass(adapter->device, rt->renderPass, adapter->allocator);


  if(rt->views != NULL)
	{
    size_t viewCount = bkpArraySize(rt->views);
    for (size_t i = 0; i < viewCount; ++i)
    {
      vkDestroyImageView(adapter->device, rt->views[i], adapter->allocator);
    }
		bkpArrayDestroy(&rt->views);
	}
}

/*____________________________________________________________________________________*/
void bkpDestroyFrameBuffers(BkpGpuAdapter adapter, VkFramebuffer * frameBuffers, uint32_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		vkDestroyFramebuffer(adapter->device, frameBuffers[i], adapter->allocator);
	}

}

/*____________________________________________________________________________________*/
void bkpSetResizeWindowCallBack(void (*func)(BkpGpuAdapter ctx))
{
	bkpWindowEnableResizable(1);
	pBkpOnResize = func;
}

/*____________________________________________________________________________________*/
void bkpUpdateWindowViewportScissor(VkCommandBuffer cmd, float width, float height)
{
	bkpUpdateViewport(cmd, 0.0f, 0.0f, width, height, 0.0f, 1.0f);
	bkpUpdateScissor(cmd, 0.0f, 0.0f, width, height);
}

/*____________________________________________________________________________________*/
void bkpUpdateViewport(VkCommandBuffer cmd, float x, float y, float width, float height, float minDepth, float maxDepth)
{
    VkViewport viewport = {};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height= height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
	vkCmdSetViewport(cmd, 0, 1, &viewport);
}

/*____________________________________________________________________________________*/
void bkpUpdateScissor(VkCommandBuffer cmd, float xOffset, float yOffset, float width, float height)
{
    VkRect2D scissor = {};
    scissor.offset  =  (VkOffset2D){xOffset, yOffset};
    scissor.extent  =   (VkExtent2D){(uint32_t)width, (uint32_t)height};
	vkCmdSetScissor(cmd, 0, 1, &scissor);
}

/*____________________________________________________________________________________*/
size_t bkp_vulkanGetDeviceAlignmentPower2(BkpGpuAdapter adapter, size_t size)
{
	size_t minUboAlign = adapter->deviceProperties.limits.minUniformBufferOffsetAlignment;

   --size;
    size |= size>>1;
    size |= size>>2;
    size |= size>>4;
    size |= size>>8;
    size |= size>>16;
    size |= size>>32;
    ++size;
    size = (size < minUboAlign) ? minUboAlign : size;

	return size;
}

/*____________________________________________________________________________________*/
void bkpCreateRenderSyncObjects(BkpGpuAdapter adapter, size_t imageCount, size_t maxFrameInFlight)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	adapter->semaphoreRenderReady = (BkpArray(VkSemaphore)) bkpArrayCreateFrom(VkSemaphore, adapter->memoryGroupId);
	adapter->semaphorePresentReady = (BkpArray(VkSemaphore)) bkpArrayCreateFrom(VkSemaphore, adapter->memoryGroupId);
	adapter->inFlightFences = (BkpArray(BkpFence)) bkpArrayCreateFrom(BkpFence, adapter->memoryGroupId);
	adapter->imagesInFlight = (BkpArray(BkpFence)) bkpArrayCreateFrom(BkpFence, adapter->memoryGroupId);
	bkpArrayResize(&adapter->semaphoreRenderReady, imageCount);
	bkpArrayResize(&adapter->semaphorePresentReady, maxFrameInFlight);
	bkpArrayResize(&adapter->inFlightFences, maxFrameInFlight);
	bkpArrayResize(&adapter->imagesInFlight, imageCount);

	for(size_t i = 0; i < imageCount; ++i)
	{
		if(vkCreateSemaphore(adapter->device, &semaphoreInfo, adapter->allocator, &adapter->semaphoreRenderReady[i]) != VK_SUCCESS)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create syncfhorinzation objects for frame!\n");
		}
	}

	for(size_t i = 0; i < maxFrameInFlight; ++i)
	{
		if(vkCreateSemaphore(adapter->device, &semaphoreInfo, adapter->allocator, &adapter->semaphorePresentReady[i]) != VK_SUCCESS)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create syncfhorinzation objects for frame!\n");
		}

		bkpCreateFence(adapter, &adapter->inFlightFences[i], BKP_TRUE);
	}

	adapter->isRenderSyncInit = BKP_TRUE;
	LOGC(eDEBUG, bkpTAG, bkpColor, "Synchronization objects created !");
	return;
}

/*____________________________________________________________________________________*/
void bkpCreateFence(BkpGpuAdapter adapter, BkpFence * fence, BkpBool createSignaled)
{
	fence->signaled = createSignaled;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags             = fence->signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	if(vkCreateFence(adapter->device, &fenceInfo, adapter->allocator, &fence->fence) != VK_SUCCESS)
	{
		LOGC(eFATAL, bkpTAG, bkpColor, "Unable to create Fence!\n");
	}

}

/*____________________________________________________________________________________*/
void bkpDestroyFence(BkpGpuAdapter adapter, BkpFence * fence)
{
	if(fence->fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(adapter->device, fence->fence, adapter->allocator);
		fence->fence = VK_NULL_HANDLE;
	}

	fence->signaled = BKP_FALSE;
}

/*____________________________________________________________________________________*/
void bkpInitializeWaitInfo(BkpWaitInfo * info, size_t id, size_t waitCount, size_t signalCount)
{
	info->waitStages = bkpArrayCreateFrom(VkPipelineStageFlags, id);
	info->waitSemaphores = bkpArrayCreateFrom(VkSemaphore, id);
	info->signalSemaphores = bkpArrayCreateFrom(VkSemaphore, id);
	if(waitCount > 0)
	{
		bkpArrayResize(&info->waitStages, waitCount);
		bkpArrayResize(&info->waitSemaphores, waitCount);
	}
	if(signalCount > 0)
	{
		bkpArrayResize(&info->signalSemaphores, signalCount);
	}
}

/*____________________________________________________________________________________*/
void bkpClearWaitInfo(BkpWaitInfo * info)
{
	bkpArrayDestroy(&info->waitStages);
	bkpArrayDestroy(&info->waitSemaphores);
	bkpArrayDestroy(&info->signalSemaphores);
}

/*____________________________________________________________________________________*/
BkpBool bkpWaitForFence(BkpGpuAdapter adapter, BkpFence * fence, uint64_t timeout_ms)
{
	if(fence->signaled) return BKP_TRUE;

	VkResult res = vkWaitForFences(adapter->device, 1, &fence->fence, VK_TRUE, timeout_ms);
	switch(res)
	{
		case VK_SUCCESS:
			fence->signaled = BKP_TRUE;
			return BKP_TRUE;
		case VK_TIMEOUT:
			LOGC(eERROR, bkpTAG, bkpColor, "VK_TIMEOUT while Fence waiting");
			break;
		case VK_ERROR_DEVICE_LOST:
			LOGC(eERROR, bkpTAG, bkpColor, "VK_ERROR_DEVICE_LOST while Fence waiting");
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			LOGC(eERROR, bkpTAG, bkpColor, "VK_ERROR_OUT_OF_HOST_MEMORY while Fence waiting");
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			LOGC(eERROR, bkpTAG, bkpColor, "VK_ERROR_OUT_OF_DEVICE_MEMORY while Fence waiting");
			break;
		default:
			LOGC(eERROR, bkpTAG, bkpColor, "Unknow error while Fence waiting");
			break;
	}

	return BKP_FALSE;
}

/*____________________________________________________________________________________*/
void bkpResetFence(BkpGpuAdapter adapter, BkpFence * fence)
{
	if(fence->signaled)
	{
		if(vkResetFences(adapter->device, 1, &fence->fence) != VK_SUCCESS)
		{
			LOGC(eFATAL, bkpTAG, bkpColor, "Fence reset Failed!\n");
		}
		fence->signaled = BKP_FALSE;
	}
}

/*____________________________________________________________________________________*/
/*____________________________________________________________________________________*/
BkpAttachment bkpDefaultRtAttachment(VkFormat format, VkSampleCountFlagBits samples, VkImageLayout finalLayout, const VkImageView * views, uint32_t viewCount)
{
	BkpAttachment att = {};
	att.info.flags = 0;
	att.info.format = format;
	att.info.samples = samples;
	att.info.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
	att.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	att.info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att.info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att.info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	att.info.finalLayout = finalLayout;
	att.views = views;
	att.viewCount = viewCount;

	return att;
}

/*____________________________________________________________________________________*/
void bkpSetDefaultImageInfo(VkImageCreateInfo * imageInfo)
{
	imageInfo->sType                          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo->imageType                      = VK_IMAGE_TYPE_2D;
	imageInfo->tiling                         = VK_IMAGE_TILING_OPTIMAL;
	imageInfo->format                         = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo->usage                          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo->mipLevels                      = 8; //Warning should call function to generate mip
	imageInfo->arrayLayers                    = 1;
	imageInfo->samples                        = VK_SAMPLE_COUNT_1_BIT;
	imageInfo->sharingMode                    = VK_SHARING_MODE_EXCLUSIVE;
}

/*____________________________________________________________________________________*/
void bkpSetDefaultImageViewInfo(VkImageViewCreateInfo * viewInfo)
{
	viewInfo->sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo->pNext                           = NULL;
	viewInfo->flags                           = 0;
	viewInfo->viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo->format                          = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo->components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo->components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo->components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo->components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo->subresourceRange.baseMipLevel   = 0;
	viewInfo->subresourceRange.baseArrayLayer = 0;
	viewInfo->subresourceRange.levelCount	    = 1;
	viewInfo->subresourceRange.layerCount     = 1;
}

/*____________________________________________________________________________________*/
void bkpSetDefaultTextureInfo(BkpImageResource * res)
{
	res->imageInfo = (VkImageCreateInfo){};
	res->imageInfo.sType                          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	res->imageInfo.format                         = VK_FORMAT_R8G8B8A8_SRGB;
	res->imageInfo.imageType                      = VK_IMAGE_TYPE_2D;
	res->imageInfo.tiling                         = VK_IMAGE_TILING_OPTIMAL;
	res->imageInfo.format                         = VK_FORMAT_R8G8B8A8_SRGB;
	res->imageInfo.usage                          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	res->filter                                   = VK_FILTER_LINEAR;
	res->memoryPropertyFlags                      = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	res->bufferType								  = eBUFFER_GPU;
	res->mipType                                  = eMIPMAP_USER_DEFINED;
	res->imageInfo.mipLevels                      = 8; //Warning should call function to generate mip
	res->imageInfo.arrayLayers                    = 1;
	res->imageInfo.samples                        = VK_SAMPLE_COUNT_1_BIT;
	res->imageInfo.sharingMode                    = VK_SHARING_MODE_EXCLUSIVE;

	res->viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	res->viewInfo.pNext                           = NULL;
	res->viewInfo.flags                           = 0;
	res->viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	res->viewInfo.format                          = VK_FORMAT_R8G8B8A8_SRGB;
	res->viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	res->viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	res->viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	res->viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
	res->viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	res->viewInfo.subresourceRange.baseMipLevel   = 0;
	res->viewInfo.subresourceRange.baseArrayLayer = 0;
	res->viewInfo.subresourceRange.levelCount	  = 1;
	res->viewInfo.subresourceRange.layerCount     = 1;
}

/*____________________________________________________________________________________*/
void bkpWaitForResizingFinished()
{
    uint32_t width = 0, height = 0;
    bkpGetWindowSize(&width, &height);

    while(width == 0 || height == 0)
    {
        bkpGetWindowSize(&width, &height);
        glfwWaitEvents();
		fprintf(stderr, "rien\n");
    }
}

/*____________________________________________________________________________________*/
const char * vkResullt2String(VkResult res, BkpBool ext)
{
	switch(res)
	{
		case VK_SUCCESS:
			return ext ? "VK_SUCCESS" : "VK_SUCCESS Command successfully completed";
		case VK_NOT_READY:
			return ext ? "VK_NOT_READY" : "VK_NOT_READY A fence or query has not yet completed";
		case VK_TIMEOUT:
			return ext ? "VK_TIMEOUT" : "VK_TIMEOUT A wait operation has not completed in the specified time";
		case VK_EVENT_SET:
			return ext ? "VK_EVENT_SET" : "VK_EVENT_SET An event is signaled";
		case VK_EVENT_RESET:
			return ext ? "VK_EVENT_RESET" : "VK_EVENT_RESET An event is unsignaled";
		case VK_INCOMPLETE:
			return ext ? "VK_INCOMPLETE" : "VK_INCOMPLETE A return array was too small for the result";
		case VK_SUBOPTIMAL_KHR:
			return ext ? "VK_SUBOPTIMAL_KHR" : "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
		case VK_THREAD_IDLE_KHR:
			return ext ? "VK_THREAD_IDLE_KHR" : "VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
		case VK_THREAD_DONE_KHR:
			return ext ? "VK_THREAD_DONE_KHR" : "VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.";
		case VK_OPERATION_DEFERRED_KHR:
			return ext ? "VK_OPERATION_DEFERRED_KHR" : "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return ext ? "VK_OPERATION_NOT_DEFERRED_KHR" : "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.";
			/*
		case VK_PIPELINE_COMPILE_REQUIRED:
			return ext ? "VK_PIPELINE_COMPILE_REQUIRED" : "VK_PIPELINE_COMPILE_REQUIRED A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";
			*/

		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return ext ? "VK_ERROR_OUT_OF_HOST_MEMORY" : "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed.";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return ext ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has failed.";
		case VK_ERROR_INITIALIZATION_FAILED:
			return ext ? "VK_ERROR_INITIALIZATION_FAILED" : "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could not be completed for implementation-specific reasons.";
		case VK_ERROR_DEVICE_LOST:
			return ext ? "VK_ERROR_DEVICE_LOST" : "VK_ERROR_DEVICE_LOST The logical or physical device has been lost. See Lost Device";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return ext ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return ext ? "VK_ERROR_LAYER_NOT_PRESENT" : "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or could not be loaded.";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return ext ? "VK_ERROR_EXTENSION_NOT_PRESENT" : "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not supported.";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return ext ? "VK_ERROR_FEATURE_NOT_PRESENT" : "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported.";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return ext ? "VK_ERROR_INCOMPATIBLE_DRIVER" : "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return ext ? "VK_ERROR_TOO_MANY_OBJECTS" : "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have already been created.";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return ext ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported on this device.";
		case VK_ERROR_FRAGMENTED_POOL:
			return ext ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
		case VK_ERROR_SURFACE_LOST_KHR:
			return ext ? "VK_ERROR_SURFACE_LOST_KHR" : "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available.";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return ext ? "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" : "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return ext ? "VK_ERROR_OUT_OF_DATE_KHR" : "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return ext ? "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" : "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
		case VK_ERROR_INVALID_SHADER_NV:
			return ext ? "VK_ERROR_INVALID_SHADER_NV" : "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return ext ? "VK_ERROR_OUT_OF_POOL_MEMORY" : "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return ext ? "VK_ERROR_INVALID_EXTERNAL_HANDLE" : "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a valid handle of the specified type.";
		case VK_ERROR_FRAGMENTATION:
			return ext ? "VK_ERROR_FRAGMENTATION" : "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due to fragmentation.";
		case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
			return ext ? "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" : "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed because the requested address is not available.";
			/*
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return ext ? "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" : "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.";
			*/
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return ext ? "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" : "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exclusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
			/*
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
			return ext ? "VK_ERROR_COMPRESSION_EXHAUSTED_EXT" : "VK_ERROR_COMPRESSION_EXHAUSTED_EXT An image creation failed because internal resources required for compression are exhausted. This must only be returned when fixed-rate compression is requested.";
		case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
			return ext ? "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR" : "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR The requested VkImageUsageFlags are not supported.";
		case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
			return ext ? "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR" : "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR The requested video picture layout is not supported.";
		case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
			return ext ? "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR" : "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR A video profile operation specified via VkVideoProfileInfoKHR::videoCodecOperation is not supported.";
		case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
			return ext ? "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR" : "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR Format parameters in a requested VkVideoProfileInfoKHR chain are not supported.";
		case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
			return ext ? "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR" : "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR Codec-specific parameters in a requested VkVideoProfileInfoKHR chain are not supported.";
		case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
			return ext ? "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR" : "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR The specified video Std header version is not supported.";
			*/
		case VK_ERROR_UNKNOWN:
			return ext ? "VK_ERROR_UNKNOWN" : "VK_ERROR_UNKNOWN An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
		default:
			return "Something went wrong with your error code";
	}
}

VkQueueFlagBits bkpGetVkQueueFlagBit(uint8_t family)
{
	switch(family)
	{
		case eQFAMILY_GRAPHIC:
			return VK_QUEUE_GRAPHICS_BIT;
		case eQFAMILY_COMPUTE:
			return VK_QUEUE_COMPUTE_BIT;
		case eQFAMILY_TRANSFERT:
			return VK_QUEUE_TRANSFER_BIT;
		case eQFAMILY_SPARSE_BINDING:
			return VK_QUEUE_SPARSE_BINDING_BIT;
			break;
		case eQFAMILY_PROTECTED:
			return VK_QUEUE_PROTECTED_BIT;
		case eQFAMILY_VIDEO_DECODE:
			return VK_QUEUE_VIDEO_DECODE_BIT_KHR;
		case eQFAMILY_VIDEO_ENCODE:
			return VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
		case eQFAMILY_OPTICAL_FLOW:
			return VK_QUEUE_OPTICAL_FLOW_BIT_NV;
	}

	return VK_QUEUE_FLAG_BITS_MAX_ENUM;
}

/*____________________________________________________________________________________*/

/*____________________________________________________________________________________*/
void bkpBeginRendering(VkCommandBuffer cmd, VkImageView imageView, VkExtent2D extent, VkClearColorValue clearColor)
{
    VkRenderingAttachmentInfo colorAttachment = {};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView   = imageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = clearColor;

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType                  = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset      = (VkOffset2D){0, 0};
    renderingInfo.renderArea.extent      = extent;
    renderingInfo.layerCount             = 1;
    renderingInfo.colorAttachmentCount   = 1;
    renderingInfo.pColorAttachments      = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);
}

/*____________________________________________________________________________________*/
void bkpEndRendering(VkCommandBuffer cmd)
{
    vkCmdEndRendering(cmd);
}

#ifdef __cplusplus
}
#endif

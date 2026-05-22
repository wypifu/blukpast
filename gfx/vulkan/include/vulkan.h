#ifndef DVULKAN_H_
#define DVULKAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gfx/vulkan/include/vk_types.h>

/*********************************************************************
 * Defines
 *********************************************************************/

	/*********************************************************************
	* Macro
	*********************************************************************/
#define MAX_QUEUE_COUNT 8
#define BKP_DO_MAP 1
#define BKP_DO_NOT_MAP 0
#define DO_CREATE_FRAME_BUFFER 1
#define DO_NOT_CREATE_FRAME_BUFFER 0

	/*********************************************************************
	 * Struct
	 *********************************************************************/

	/*********************************************************************
	 * Class
	 *********************************************************************/

	/*********************************************************************
	 * Global
	 *********************************************************************/
	extern const BkpBool NO_STAGGING;


	/**
	 * @defgroup vulkan_core Vulkan Core
	 * @brief Vulkan context initialisation, device selection, swap chain, and frame loop.
	 * @{
	 */

	//size_t bkp_vulkanGetDeviceAlignment(BkpVulkanContext * context, size_t size);

	/** @brief Override the global Vulkan memory page size (bytes) used by the internal allocator. */
	BKP_EXPORTED void bkpSetVulkanMemoryPage(size_t pageSize);
	/** @brief Override the per-adapter Vulkan memory page size (bytes). */
	BKP_EXPORTED void bkpSetVulkanAdapterMemoryPage(size_t pageSize);
	/** @brief Return the smallest power-of-two alignment ≥ @p size required by the device. */
	BKP_EXPORTED size_t bkp_vulkanGetDeviceAlignmentPower2(BkpGpuAdapter adapter, size_t size);

	/**
	 * @brief Create a Vulkan instance and surface.
	 *
	 * Must be called before @ref bkpActivateGpuAdapter.
	 *
	 * @param info    Instance configuration (extensions, validation layers, window handle…).
	 * @param context Output context; zero-initialise before calling.
	 * @return BKP_TRUE on success.
	 */
	BKP_EXPORTED BkpBool bkpInitVulkanContext(BkpVulkanContextInfo * info, BkpVulkanContext * context);

	/**
	 * @brief Create a logical device and initialise one GPU adapter.
	 *
	 * Creates the logical device, graphics/transfer command pools, and the GPU
	 * memory allocator.  Sets `adapter->frameInfo.maxFrameInFlight` from
	 * `ctx->info.maxFrameInFlight`.  Can be called multiple times on the same
	 * context to activate several physical devices independently.
	 *
	 * @note The `criteria` parameter is reserved for future use — the first
	 *       physical device enumerated is always selected.
	 *
	 * @param ctx      Context initialised by @ref bkpInitVulkanContext or @ref bkpInit.
	 * @param adapter  Output: pointer to the activated adapter.
	 * @param criteria Device selection hint (deviceType, index).  Currently ignored.
	 */
	BKP_EXPORTED void bkpActivateGpuAdapter(BkpVulkanContext * ctx, BkpGpuAdapter * adapter, BkpAdapterCriteria * criteria);

	/** @brief Block until the device is idle.  Call before releasing GPU resources. */
	BKP_EXPORTED void bkpWaitDevice(BkpGpuAdapter adapter);
	/** @brief Destroy the logical device and free adapter resources. */
	BKP_EXPORTED void bkpClearGpuAdapter(BkpGpuAdapter adapter);
	/** @brief Destroy the Vulkan instance and surface. */
	BKP_EXPORTED void bkpClearContext(BkpVulkanContext * context);

	/** @brief Create a command pool on the graphics queue family. */
	BKP_EXPORTED void bkpCreateCmdPool(BkpGpuAdapter adapter, BkpCommandPool * cmdPool);

	/** @brief Check whether a format + tiling combination supports the requested features. */
  BKP_EXPORTED BkpBool bkpCheckFormatFeatureSupport(BkpGpuAdapter adp, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags feature);
	/** @brief Return the first format from @p tCandidate that supports the given tiling and features. */
  BKP_EXPORTED VkFormat findSupportedFormat(VkPhysicalDevice gpu, VkFormat * tCandidate, size_t n, VkImageTiling tiling, VkFormatFeatureFlags features);

	/**
	 * @brief Return a partially-filled VkSwapchainCreateInfoKHR.
	 *
	 * `surface` and `minImageCount` are left at zero — @ref bkpCreateSwapChain
	 * fills them in.  The default format (`B8G8R8A8_SRGB`) is also overridden by
	 * @ref bkpCreateSwapChain with the best format supported by the surface.
	 *
	 * @param arrayLayers   Number of image array layers (1 for standard rendering).
	 * @param presentMode   Requested present mode; falls back to FIFO if unavailable.
	 */
	BKP_EXPORTED VkSwapchainCreateInfoKHR bkpDefaultSwapChain(uint32_t arrayLayers, VkImageUsageFlags, VkCompositeAlphaFlagBitsKHR, VkBool32 clipped, VkPresentModeKHR presentMode);

	/**
	 * @brief Create the swap chain and populate `swapChain->images` / `imageViews`.
	 *
	 * Queries surface capabilities, selects the best available format and present
	 * mode, then creates the VkSwapchainKHR.  If `adapter->frameInfo.maxFrameInFlight`
	 * exceeds the image count, it is silently reduced with a warning.
	 * `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` is added automatically when supported.
	 *
	 * **Prerequisite**: set `adapter->swapChain = &sc` before calling.
	 *
	 * @param swapChain       Must have `info` pre-filled via @ref bkpDefaultSwapChain.
	 *                        After the call, `sc.imageCount` and `sc.info.imageFormat`
	 *                        reflect the actual values chosen by the driver.
	 * @param preferredModes  Ordered list of desired present modes (most preferred first).
	 *                        Compatible with raw arrays, `vector.data()`, or `BkpArray`.
	 *                        Falls back to `VK_PRESENT_MODE_FIFO_KHR` if none match.
	 * @param preferredCount  Number of entries in `preferredModes`.
	 */
	BKP_EXPORTED void bkpCreateSwapChain(BkpGpuAdapter adapter, BkpSwapChain * swapChain,
	                                     const VkPresentModeKHR * preferredModes, uint32_t preferredCount);
	/** @brief Create the depth/stencil image for the current swap-chain extent. */
	BKP_EXPORTED void bkpCreateDepthResources(BkpGpuAdapter adapter, BkpImageResource * depth);
	BKP_EXPORTED void bkpCreateImageResources(BkpGpuAdapter adapter, BkpImageResource * ress);
	/** @brief Return a BkpImageResource pre-filled for a standard depth buffer.
	 *  Caller must set imageInfo.extent and imageInfo.format / viewInfo.format. */
	BKP_EXPORTED BkpImageResource bkpDefaultDepthBuffer();
	/** @brief Return a BkpImageResource pre-filled for an offscreen color render target
	 *  (COLOR_ATTACHMENT | SAMPLED, device-local).
	 *  Caller must set imageInfo.extent and imageInfo.format / viewInfo.format. */
	BKP_EXPORTED BkpImageResource bkpDefaultColorBuffer();
	/** @brief Allocate and bind GPU memory for an image described by `res->imageInfo`. */
	BKP_EXPORTED void bkpCreateImage(BkpGpuAdapter adapter, BkpImageResource * res);

	/** @brief Create a render pass + framebuffers described by @p Robj. */
	BKP_EXPORTED void bkpCreateRenderTarget(BkpGpuAdapter adapter, BkpRenderTarget * Robj, BkpBool createFrameBuffer);
	/** @brief (Re)create framebuffers for an existing render target. */
	BKP_EXPORTED void bkpCreateFrameBuffers(BkpGpuAdapter adapter, BkpRenderTarget * Rt);
	BKP_EXPORTED void bkpManualCreateFrameBuffers(BkpGpuAdapter adapter, BkpRenderTarget * Rt, VkImageViewCreateInfo * ci);
	BKP_EXPORTED VkSubpassDescription bkpDefaultSubpass(VkSubpassDescriptionFlags flags, VkPipelineBindPoint bindPoint);

	/**
	 * @brief Wait for the current frame's in-flight fence and acquire the next swap-chain image.
	 *
	 * Waits on `inFlightFences[currentFrame]`, then calls `vkAcquireNextImageKHR`
	 * and stores the result in `adapter->frameInfo.imageAcquired`.  Resets the fence
	 * after acquisition so it can be re-signaled by @ref bkpSubmitFrameBasic.
	 * If the swap chain is out of date, calls the resize callback registered with
	 * @ref bkpSetResizeWindowCallBack and returns early.
	 *
	 * Call once at the start of each frame, before recording any commands.
	 */
	BKP_EXPORTED void bkpPrepareFrame(BkpGpuAdapter adapter);

	/**
	 * @brief Submit one command buffer on the graphic queue using the adapter's internal sync objects.
	 *
	 * The simplest frame-loop submit — no configuration required.  Internally:
	 * - Picks `cmd->cmds[currentFrame]` from the provided buffer.
	 * - Waits on `semaphorePresentReady[currentFrame]` at `COLOR_ATTACHMENT_OUTPUT`.
	 * - Signals `semaphoreRenderReady[imageAcquired]` on completion (consumed by @ref bkpPresentFrame).
	 * - Submits with `inFlightFences[currentFrame]` so the next @ref bkpPrepareFrame
	 *   waits before reusing this slot.
	 *
	 * For multiple command buffers or custom sync, use @ref bkpSubmitFrame.
	 * For compute or other queues, use @ref bkpSubmit.
	 */
	BKP_EXPORTED void bkpSubmitFrameBasic(BkpGpuAdapter adapter, BkpCommandBuffer * cmd);

	/**
	 * @brief Submit multiple command buffers on the graphic queue with custom synchronisation.
	 *
	 * Each buffer is indexed at `adapter->frameInfo.currentFrame` — call after
	 * @ref bkpPrepareFrame.  Uses `inFlightFences[currentFrame]` as the completion
	 * fence.  All wait/signal semaphores come from `waitInfo`; the adapter's internal
	 * semaphores are not touched (unlike @ref bkpSubmitFrameBasic).
	 *
	 * Typical use: shadow pass + main pass submitted together, or any multi-stage
	 * graphic frame that needs explicit inter-pass synchronisation.
	 *
	 * @param cmds      Array of `cmdCount` pointers to @ref BkpCommandBuffer.
	 * @param cmdCount  Number of command buffers to submit in this batch.
	 * @param waitInfo  Wait stages, wait semaphores and signal semaphores.
	 */
	BKP_EXPORTED void bkpSubmitFrame(BkpGpuAdapter adapter, BkpCommandBuffer ** cmds, uint32_t cmdCount, BkpWaitInfo * waitInfo);

	/**
	 * @brief General-purpose submit — any queue, any command buffers, any index.
	 *
	 * Not tied to the frame loop.  Use for compute dispatches, transfer operations,
	 * or any work that must run on a queue other than the graphic queue.
	 *
	 * For in-frame compute that feeds the graphic pass:
	 * 1. Call `bkpSubmit` on `eQFAMILY_COMPUTE` with `cmdIndex = adapter->frameInfo.currentFrame`,
	 *    signalling a semaphore in `waitInfo`.
	 * 2. Add that semaphore to the `waitInfo` of the subsequent @ref bkpSubmitFrame call.
	 *
	 * For one-shot dispatches outside the frame loop, set `cmdIndex = 0` and pass
	 * a fence to wait for completion, or `NULL` for fire-and-forget.
	 *
	 * @param queue     Target queue: `eQFAMILY_GRAPHIC`, `eQFAMILY_COMPUTE`, or `eQFAMILY_TRANSFERT`.
	 * @param cmds      Array of `cmdCount` pointers to @ref BkpCommandBuffer.
	 * @param cmdCount  Number of command buffers to submit.
	 * @param cmdIndex  Index into each `BkpCommandBuffer::cmds[]` array (e.g. `currentFrame` or `0`).
	 * @param waitInfo  Wait/signal semaphores, or `NULL` for no synchronisation.
	 * @param fence     Fence signalled on completion, or `NULL` for fire-and-forget.
	 */
	BKP_EXPORTED void bkpSubmit(BkpGpuAdapter adapter, EGpuQueue queue,
	                             BkpCommandBuffer ** cmds, uint32_t cmdCount, uint32_t cmdIndex,
	                             BkpWaitInfo * waitInfo, BkpFence * fence);

	/**
	 * @brief Present the acquired swap-chain image and advance the frame counter.
	 *
	 * Waits on `semaphoreRenderReady[imageAcquired]`, presents, then increments
	 * `currentFrame` (mod `maxFrameInFlight`) and updates `deltaTime` / `frameCount`.
	 * Handles `VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR` by calling the
	 * resize callback.  Call after @ref bkpSubmitFrameBasic.
	 */
	BKP_EXPORTED void bkpPresentFrame(BkpGpuAdapter adapter);

	/**
	 * @brief Allocate a GPU buffer and optionally map it for CPU writes.
	 * @param do_map BKP_DO_MAP to keep a persistent CPU mapping, BKP_DO_NOT_MAP otherwise.
	 */
	BKP_EXPORTED void bkpCreateBuffersGPU(BkpGpuAdapter adapter, BkpBufferGPU * uniformBuffer, VkBufferUsageFlags usage, EBufferType type, BkpBool do_map);
	/** @brief Unmap (if mapped) and free a GPU buffer. */
	BKP_EXPORTED void bkpDestroyBuffersGPU(BkpGpuAdapter adapter, BkpBufferGPU * uniformBuffer);

	BKP_EXPORTED void mapData(BkpVulkanContext * context, VkMemoryPropertyFlags propFlags, VkDeviceMemory  bufferMemory, void **data, size_t s_size, uint32_t offset);
	BKP_EXPORTED void unmapData(BkpVulkanContext * context, VkDeviceMemory  bufferMemory);

	BKP_EXPORTED void bkpCreateSampler(BkpGpuAdapter adapter, BkpSampler * sampler);
  BKP_EXPORTED uint8_t bkpSaveSwapChainImage(BkpGpuAdapter adp, BkpSwapChain * res, uint32_t frame, const char * path, EImageType imgType);
  BKP_EXPORTED uint8_t bkpSaveImage(BkpGpuAdapter adapter, BkpImageResource * img, uint32_t frame, const char * path, EImageType imgType);
	BKP_EXPORTED uint8_t * bkpGenerateImageDataRGBA(int width, int heigth, uint8_t r, uint8_t g, uint8_t b, uint8_t a, VkDeviceSize * size);
	/**
	 * @brief Generate a checkerboard (damier) RGBA8 bitmap.
	 *
	 * Produces a @p width × @p height image with @p squaresPerSide × @p squaresPerSide
	 * alternating tiles.  Alpha is always 255.
	 * The returned buffer must be freed with @ref bkpFree.
	 *
	 * Apparent square size on screen is controlled by the UV tiling applied at
	 * render time (not by this function).  A value of 4 for @p squaresPerSide
	 * gives good texel density for a 256 × 256 texture.
	 *
	 * @param width          Image width in pixels.
	 * @param height         Image height in pixels.
	 * @param squaresPerSide Number of checker squares along each axis.
	 * @param r1,g1,b1       RGB of the first tile colour.
	 * @param r2,g2,b2       RGB of the second tile colour.
	 * @param size           Output: byte size of the returned buffer.
	 * @return               Heap-allocated RGBA pixel data (4 bytes per pixel).
	 */
	BKP_EXPORTED uint8_t * bkpGenerateImageDataDamier(int width, int height, int squaresPerSide,
	                                                   uint8_t r1, uint8_t g1, uint8_t b1,
	                                                   uint8_t r2, uint8_t g2, uint8_t b2,
	                                                   VkDeviceSize * size);
  BKP_EXPORTED uint8_t * bkpGenerateVerticalGradianImage(int width, int height, BkpColorGradianInfo * grad, size_t gradCount, VkDeviceSize * size);
	BKP_EXPORTED void bkpCreateDefaultTexture(BkpGpuAdapter adapter, BkpImageResource * text, BkpBool isBlank);
	BKP_EXPORTED void bkpCreateDefaultTextureLayers(BkpGpuAdapter adapter, BkpImageResource * text, int layourCount, BkpBool isBlank);
	/**
	 * @brief Load a single texture from an **encoded image in memory** (PNG, JPG, …).
	 *
	 * stb_image decodes the data pointed by @p buffMemInfo into an RGBA bitmap.
	 * Width, height and alpha flag are detected automatically.
	 * The caller must pre-initialise @p text with image/view/filter settings
	 * (format, tiling, usage, mipType, filter, memoryPropertyFlags) but must
	 * NOT set extent or mipLevels — those are filled from the decoded image.
	 *
	 * @param adapter     Active GPU adapter.
	 * @param buffMemInfo Pointer to a BkpMemInfo describing the encoded image blob
	 *                    { .buffer = ptr, .offset = byte_offset, .size = byte_count }.
	 * @param text        Pre-initialised BkpImageResource (single layer).
	 * @return BKP_TRUE on success, BKP_FALSE if decoding fails.
	 */
	BKP_EXPORTED BkpBool bkpCreateTextureFromMemory(BkpGpuAdapter adapter, BkpMemInfo * buffMemInfo, BkpImageResource * text);

	/**
	 * @brief Load a single texture from a **file path** on disk.
	 *
	 * stb_image decodes the file into an RGBA bitmap.
	 * Width, height and alpha flag are detected automatically.
	 * Same pre-initialisation requirements as bkpCreateTextureFromMemory().
	 *
	 * @param adapter Active GPU adapter.
	 * @param path    Null-terminated path to the image file (PNG, JPG, BMP, …).
	 * @param text    Pre-initialised BkpImageResource (single layer).
	 * @return BKP_TRUE on success, BKP_FALSE if the file cannot be loaded.
	 */
	BKP_EXPORTED BkpBool bkpCreateTextureFromFile(BkpGpuAdapter adapter, const char * path, BkpImageResource * text);

    /* Batch parallel texture upload:
     * - CPU decode of all count images is done in parallel (one thread per image)
     * - GPU upload is done sequentially after all decodes finish
     * sources[i].filePath != NULL → load from file; else use sources[i].encoded.
     * Each targets[i] must be pre-initialised with bkpSetDefaultTextureInfo(). */
    typedef struct {
        BkpMemInfo   encoded;    /* used when filePath == NULL (size == 0 → skip) */
        const char * filePath;   /* non-NULL → load from disk                     */
    } BkpTextureSource;
    BKP_EXPORTED void bkpUploadTextureBatch(BkpGpuAdapter adapter,
                                            BkpTextureSource * sources, size_t count,
                                            BkpImageResource * targets);

    /* Pack metallic + roughness + AO into a single R8G8B8A8_UNORM ORM texture
     * (R=AO, G=Roughness, B=Metallic, A=255).  The three images are decoded in
     * parallel.  aoPath may be NULL → R channel is filled with 0xFF.
     * metallicPath and roughnessPath must have identical pixel dimensions.
     * target must be pre-initialised with bkpSetDefaultTextureInfo(); the format
     * is overridden to VK_FORMAT_R8G8B8A8_UNORM (linear, not sRGB).           */
    BKP_EXPORTED BkpBool bkpLoadORMFromFiles(BkpGpuAdapter adapter,
                                              const char * metallicPath,
                                              const char * roughnessPath,
                                              const char * aoPath,
                                              BkpImageResource * target);
	/**
	 * @brief Load an array texture from **N file paths** on disk.
	 *
	 * Each file is decoded by stb_image into an RGBA bitmap.
	 * All layers must have identical pixel dimensions; the first file
	 * determines the reference size — mismatched layers abort with BKP_FALSE.
	 * The alpha flag is derived from the first file's channel count.
	 *
	 * @param adapter   Active GPU adapter.
	 * @param path      Array of @p pathCount null-terminated file paths.
	 * @param pathCount Number of layers (= number of files).
	 * @param text      Pre-initialised BkpImageResource.
	 * @return BKP_TRUE on success, BKP_FALSE on any load or dimension mismatch.
	 */
	BKP_EXPORTED BkpBool bkpCreateTextureLayersFromFile(BkpGpuAdapter adapter, const char ** path, size_t pathCount, BkpImageResource * text);

	/**
	 * @brief Load an array texture from **N encoded image blobs in memory** (PNG, JPG, …).
	 *
	 * Each BkpMemInfo entry holds a pointer + offset + byte-size for one
	 * encoded image blob. stb_image decodes every blob into an RGBA bitmap.
	 * All layers must have identical pixel dimensions; the first entry
	 * determines the reference size — mismatched layers abort with BKP_FALSE.
	 * The alpha flag is derived from the first image's channel count.
	 *
	 * @param adapter      Active GPU adapter.
	 * @param buffMemInfo  Array of @p memInfoCount BkpMemInfo, each describing
	 *                     one encoded image { .buffer, .offset, .size }.
	 * @param memInfoCount Number of layers.
	 * @param text         Pre-initialised BkpImageResource.
	 * @return BKP_TRUE on success, BKP_FALSE on decode error or dimension mismatch.
	 */
	BKP_EXPORTED BkpBool bkpCreateTextureLayersFromMemory(BkpGpuAdapter adapter, BkpMemInfo * buffMemInfo, size_t memInfoCount, BkpImageResource * text);

	/**
	 * @brief Load an array texture from **N raw RGBA bitmaps in memory**.
	 *
	 * Unlike bkpCreateTextureLayersFromMemory(), the image data is already
	 * decoded: each BkpMemInfo entry points to a raw pixel buffer (RGBA8,
	 * 4 bytes per pixel). The caller must supply width, height and hasAlpha
	 * explicitly — no stb_image decode is performed.
	 *
	 * Use this when the bitmap was generated programmatically
	 * (e.g. bkpGenerateVerticalGradianImage()) rather than loaded from a file.
	 *
	 * @param adapter      Active GPU adapter.
	 * @param buffMemInfo  Array of @p memInfoCount BkpMemInfo, each describing
	 *                     one raw bitmap { .buffer = rgba_ptr, .offset, .size }.
	 * @param memInfoCount Number of layers.
	 * @param width        Pixel width of every layer.
	 * @param height       Pixel height of every layer.
	 * @param hasAlpha     BKP_TRUE if the bitmap has meaningful alpha data.
	 * @param text         Pre-initialised BkpImageResource.
	 * @return BKP_TRUE on success.
	 */
	BKP_EXPORTED BkpBool bkpCreateTextureLayersFromBitmap(BkpGpuAdapter adapter, BkpMemInfo * buffMemInfo, size_t memInfoCount, int width, int height, BkpBool hasAlpha, BkpImageResource * text);

	/**
	 * @brief Low-level: upload an array texture from **N raw `uint8_t*` bitmaps**.
	 *
	 * This is the base function called by all higher-level bkpCreateTexture*
	 * variants. The bitmaps array must contain @p bitmapCount pointers to RGBA8
	 * pixel buffers of exactly width × height × 4 bytes each.
	 * This function sets text->imageInfo.extent and text->imageInfo.arrayLayers
	 * from the supplied parameters, computes mipLevels from text->mipType,
	 * then creates a staging buffer, uploads all layers, and finalises the
	 * GPU image (transitions, mipmaps, image view).
	 *
	 * @param adapter     Active GPU adapter.
	 * @param bitmaps     Array of @p bitmapCount raw RGBA8 pixel buffers.
	 * @param bitmapCount Number of layers (also sets arrayLayers).
	 * @param width       Pixel width of every layer.
	 * @param height      Pixel height of every layer.
	 * @param depth       VkExtent3D depth (1 for 2-D textures, >1 for 3-D).
	 * @param text        Pre-initialised BkpImageResource (format, tiling,
	 *                    usage, mipType, filter, memoryPropertyFlags must be set;
	 *                    extent and mipLevels are overwritten).
	 */
	BKP_EXPORTED void bkpCreateTextureLayersFromData(BkpGpuAdapter adapter, uint8_t ** bitmaps, size_t bitmapCount, int width, int height, int depth, BkpImageResource * text);

	/**
	 * @brief Upload RGBA pixel data into one layer of an existing 2-D texture array.
	 *
	 * The image must have been created with `arrayLayers` large enough to hold
	 * @p layer (i.e. pre-allocated).  The entire image is expected to be in
	 * `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`; the function transitions the
	 * target layer to `TRANSFER_DST`, uploads, then transitions back.
	 *
	 * Typical use: on-demand growth of a TTF font atlas when a glyph that was not
	 * packed at creation time is first requested.
	 *
	 * @param adapter    Active GPU adapter.
	 * @param tex        Texture array to update.
	 * @param layer      Zero-based index of the array layer to overwrite.
	 * @param data       RGBA pixel data (4 bytes/pixel, width×height rows).
	 * @param layerSize  Byte size of @p data (must equal width × height × 4).
	 */
	BKP_EXPORTED void bkpUploadTextureLayer(BkpGpuAdapter adapter, BkpImageResource * tex,
	                                        uint32_t layer, void * data, VkDeviceSize layerSize);

	/**
	 * @brief Low-level: finalise a **single-layer** GPU image from a pre-built staging buffer.
	 *
	 * The caller is responsible for allocating and filling @p staginRes before
	 * this call. text->imageInfo.extent and all VkImageCreateInfo/VkImageViewCreateInfo
	 * fields must already be set. This function creates the VkImage, transitions
	 * it to TRANSFER_DST, copies the staging buffer, generates mipmaps, then
	 * creates the VkImageView.
	 *
	 * @param adapter    Active GPU adapter.
	 * @param text       Fully configured BkpImageResource (single layer).
	 * @param staginRes  Staging buffer already uploaded with the pixel data.
	 */
	BKP_EXPORTED void bkpCreateTexture(BkpGpuAdapter adapter, BkpImageResource * text, BkpBuffer staginRes);

	/**
	 * @brief Low-level: finalise a **multi-layer** GPU image from a pre-built staging buffer.
	 *
	 * Same contract as bkpCreateTexture() but copies all layers packed
	 * sequentially in @p staginRes (layer_size × layerCount bytes total).
	 * text->imageInfo.arrayLayers and viewInfo.subresourceRange.layerCount
	 * must already reflect the layer count.
	 *
	 * @param adapter    Active GPU adapter.
	 * @param text       Fully configured BkpImageResource (N layers).
	 * @param staginRes  Staging buffer packed with all layers in order.
	 */
	BKP_EXPORTED void bkpCreateTextureLayers(BkpGpuAdapter adapter, BkpImageResource * text, BkpBuffer staginRes);
	BKP_EXPORTED void bkpGenerateMipmaps(BkpGpuAdapter adapter, BkpImageResource * img);
	BKP_EXPORTED void bkpImageTransition(BkpGpuAdapter adapter, VkImage image, uint32_t mipLevels, uint32_t arrayLayers, VkImageAspectFlags aspectMask, VkImageLayout old, VkImageLayout nouveau);

	BKP_EXPORTED void bkpCopyBufferToImage(BkpVulkanContext* gpu, VkBuffer buffer, VkImage image, VkBufferImageCopy region);
	BKP_EXPORTED void bkpCopyBufferToImageLayers(BkpGpuAdapter adapter, VkBuffer buffer, VkDeviceSize offset, VkImage image, VkImageAspectFlags aspectMask, uint32_t width, uint32_t height, uint32_t layerCount);

	BKP_EXPORTED void bkpBeginRenderPass(VkCommandBuffer commandBuffer,  BkpRenderTarget * rt, uint32_t image_index);
	BKP_EXPORTED void bkpEndRenderPass(BkpCommandBuffer * commandBuffer, uint32_t image_index);
	//void submitCommandBuffer(BkpCommandBuffer * cmd, uint32_t index);
	/*

	   BkpBool bkp_checkFormatFeatureSupport(BkpVulkanContext * context, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags feature);
*/
	/** @brief Destroy a command pool and all command buffers allocated from it. */
	BKP_EXPORTED void bkpDestroyCommandPool(BkpGpuAdapter adapter, BkpCommandPool * cmdpool);
	/** @brief Destroy an image, image view, and free the backing device memory. */
	BKP_EXPORTED void bkpDestroyImageResource(BkpGpuAdapter adapter, BkpImageResource * imgRes);
	/** @brief Destroy a render target (render pass + framebuffers). */
	BKP_EXPORTED void bkpDestroyRenderTarget(BkpGpuAdapter adapter, BkpRenderTarget * rt);
	BKP_EXPORTED void bkpDestroyFrameBuffers(BkpGpuAdapter adapter, VkFramebuffer * frameBuffers, uint32_t count);
	/**
	 * @brief Destroy the swap chain, its image views, and the frame-loop sync objects.
	 *
	 * Destroys all image views, the VkSwapchainKHR, and — if
	 * @ref bkpCreateRenderSyncObjects was called — the semaphores and fences.
	 * Call @ref bkpWaitDevice before this.
	 */
	BKP_EXPORTED void bkpCleanupSwapChain(BkpGpuAdapter adapter, BkpSwapChain * swc);

	BKP_EXPORTED void bkpDestroySampler(BkpGpuAdapter adapter, BkpSampler * sampler);
	BKP_EXPORTED VkCommandBufferBeginInfo bkpDefaultCmdBufferBeginInfo(VkCommandBufferUsageFlags flag);
	/** @brief Register a callback invoked when the window is resized. */
	BKP_EXPORTED void bkpSetResizeWindowCallBack(void (*func)(BkpGpuAdapter adp));
	/**
	 * @brief Set viewport (0, 0, w, h, 0, 1) and scissor (0, 0, w, h) in one call.
	 *
	 * Equivalent to calling @ref bkpUpdateViewport + @ref bkpUpdateScissor covering
	 * the full framebuffer.  Requires `VK_DYNAMIC_STATE_VIEWPORT` and
	 * `VK_DYNAMIC_STATE_SCISSOR` to be enabled in the pipeline.
	 */
	BKP_EXPORTED void bkpUpdateWindowViewportScissor(VkCommandBuffer cmd, float width, float height);
	BKP_EXPORTED void bkpUpdateViewport(VkCommandBuffer cmd, float x, float y, float width, float height, float minDepth, float maxDepth);
	BKP_EXPORTED void bkpUpdateScissor(VkCommandBuffer cmd, float xOffset, float yOffset, float width, float height);

	/** @} */ /* vulkan_core */
	BKP_EXPORTED size_t getDeviceAlignmentPower2(BkpVulkanContext * ctx, size_t size);


	/** @cond INTERNAL — called automatically by @ref bkpCreateSwapChain. */
	BKP_EXPORTED void bkpCreateRenderSyncObjects(BkpGpuAdapter adapter, size_t imageCount, size_t maxFrameInFlight);
	/** @endcond */
	BKP_EXPORTED void bkpCreateFence(BkpGpuAdapter adapter, BkpFence * fence, BkpBool createSignaled);
	BKP_EXPORTED void bkpResetFence(BkpGpuAdapter adapter, BkpFence * fence);
	BKP_EXPORTED BkpBool bkpWaitForFence(BkpGpuAdapter adapter, BkpFence * fence, uint64_t timeout_ms);
	BKP_EXPORTED void bkpDestroyFence(BkpGpuAdapter adapter, BkpFence * fence);
	BKP_EXPORTED void bkpInitializeWaitInfo(BkpWaitInfo * info, size_t id, size_t waitCount, size_t signalCount);
	BKP_EXPORTED void bkpClearWaitInfo(BkpWaitInfo * info);

	BKP_EXPORTED BkpAttachment bkpDefaultRtAttachment(VkFormat format, VkSampleCountFlagBits samples, VkImageLayout finalLayout, const VkImageView * views, uint32_t viewCount);

/*____________________________________________________________________________________*/
  BKP_EXPORTED void bkpSetDefaultImageInfo(VkImageCreateInfo * imageInfo);
  BKP_EXPORTED void bkpSetDefaultImageViewInfo(VkImageViewCreateInfo * viewInfo);

	BKP_EXPORTED void bkpSetDefaultTextureInfo(BkpImageResource * res);

	/**
	 * @brief Fill @p res with sensible defaults for a 6-face cubemap.
	 *
	 * Wraps @ref bkpSetDefaultTextureInfo and overrides:
	 * - `imageInfo.flags   = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT`
	 * - `viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE`
	 * - `imageInfo.mipLevels = 1`, `mipType = eMIPMAP_NONE`
	 *
	 * After this call, set your sampler address modes, filters, and then
	 * upload pixel data with @ref bkpCreateCubemapFromData.
	 *
	 * @param res  Output: zeroed BkpImageResource to initialise.
	 * @param fmt  Vulkan format for all 6 faces (e.g. `VK_FORMAT_R8G8B8A8_UNORM`).
	 */
	BKP_EXPORTED void bkpSetDefaultCubemapInfo(BkpImageResource * res, VkFormat fmt);

	/**
	 * @brief Upload a 6-face cubemap from six CPU pixel buffers.
	 *
	 * Sets `VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT` and `VK_IMAGE_VIEW_TYPE_CUBE`,
	 * then calls @ref bkpCreateTextureLayersFromData with `bitmapCount = 6`.
	 * The caller must have set the format (via @ref bkpSetDefaultCubemapInfo or
	 * manually) before calling this function.
	 *
	 * **Face order** follows the Vulkan cubemap convention:
	 * `0=+X, 1=−X, 2=+Y, 3=−Y, 4=+Z, 5=−Z`.
	 * The same pointer may appear in multiple slots (e.g. four gradient side
	 * faces sharing one pixel buffer).
	 *
	 * @param adapter   Active GPU adapter.
	 * @param faces     Array of 6 pointers to raw RGBA pixel data.
	 *                  Each buffer must be `faceSize × faceSize × 4` bytes.
	 * @param faceSize  Width **and** height of each square face in pixels.
	 * @param tex       Pre-initialised BkpImageResource (format and filter set).
	 */
	BKP_EXPORTED void bkpCreateCubemapFromData(BkpGpuAdapter adapter, uint8_t * faces[6], int faceSize, BkpImageResource * tex);

	/**
	 * @brief Load a cubemap from six image files (PNG, JPG, …).
	 *
	 * Loads each file with stb_image, verifies that all six faces share the
	 * same dimensions, sets `VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT` and
	 * `VK_IMAGE_VIEW_TYPE_CUBE`, then uploads via
	 * @ref bkpCreateTextureLayersFromData.
	 *
	 * The caller must have set the format (via @ref bkpSetDefaultCubemapInfo
	 * or manually) before calling this function.
	 *
	 * **Face order** follows the Vulkan cubemap convention:
	 * `paths[0]=+X, paths[1]=−X, paths[2]=+Y, paths[3]=−Y, paths[4]=+Z, paths[5]=−Z`.
	 *
	 * @param adapter  Active GPU adapter.
	 * @param paths    Array of 6 null-terminated file paths.
	 * @param tex      Pre-initialised BkpImageResource (format and filter set).
	 * @return `BKP_TRUE` on success, `BKP_FALSE` if any file fails to load or
	 *         if the face dimensions are inconsistent.
	 */
	BKP_EXPORTED BkpBool bkpCreateCubemapFromFiles(BkpGpuAdapter adapter, const char * paths[6], BkpImageResource * tex);
	BKP_EXPORTED const char * vkResullt2String(VkResult res, BkpBool shortMessage);
	BKP_EXPORTED VkQueueFlagBits bkpGetVkQueueFlagBit(uint8_t family);

	BKP_EXPORTED void bkpWaitForResizingFinished();

	/**
	 * @brief Begin a dynamic rendering pass (VK_KHR_dynamic_rendering) with one colour attachment.
	 *
	 * `loadOp = CLEAR`, `storeOp = STORE`.  No depth attachment — use
	 * @ref bkpCmdBeginRendering with a manual VkRenderingInfo if you need depth.
	 *
	 * **Prerequisite**: the image must already be in
	 * `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` — transition it with
	 * @ref bkpCmdBarrierImages before calling.
	 *
	 * @param imageView  View of the swap-chain image to render into.
	 * @param extent     Render area (typically the swap-chain extent).
	 * @param clearColor Clear colour applied at the start of the pass.
	 */
	BKP_EXPORTED void bkpBeginRendering(VkCommandBuffer cmd, VkImageView imageView, VkExtent2D extent, VkClearColorValue clearColor);

	/** @brief End the dynamic rendering pass started by @ref bkpBeginRendering. */
	BKP_EXPORTED void bkpEndRendering(VkCommandBuffer cmd);

#ifdef __cplusplus
}
#endif

#endif /* DVULKAN_H_ */

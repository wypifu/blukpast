#ifndef  DVK_PIPELINE_H_
#define  DVK_PIPELINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vulkan.h"
#include "vk_shader_module.h"

/*********************************************************************
 * Defines
 *********************************************************************/
 #define RATE_FRAME_FREQUENCY 0
 #define RATE_PIPELINE_FREQUENCY 1
 #define RATE_MODEL_FREQUENCY 2
 #define RATE_CUSTOM_FREQUENCY 3

#define MAX_VIEWPORT 32
#define MAX_SCISSORS 32
#define MAX_COLOR_ATTACHMENTS 32
#define MAX_DINAMIC_STAT 32

/*********************************************************************
 * Type def & enum
 *********************************************************************/
typedef enum {ePIPELINE_GRAPHIC, ePIPELINE_COMPUTE, ePIPELINE_COUNT} EVkPipeline ;

/*********************************************************************
 * Macro
 *********************************************************************/

/*********************************************************************
 * Struct
 *********************************************************************/

typedef struct
{
	VkPipelineLayoutCreateInfo info;
	VkPipelineLayout layout;
}BkpPipelineLayout;

typedef struct
{
    BkpShaderProgram * shaderProgram;

    BkpVertexLayout  vertexLayout;

    VkFormat colorAttachmentFormat;
    VkFormat depthAttachmentFormat;
	void * pNext;
	VkPipelineCreateFlags flags;
	VkViewport viewports[MAX_VIEWPORT];
	uint16_t viewportCount;
	VkRect2D scissors[MAX_SCISSORS];
	uint16_t scissorCount;
	VkPipelineColorBlendAttachmentState colorAttachments[MAX_COLOR_ATTACHMENTS];
	uint16_t colorAttachmentCount;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineTessellationStateCreateInfo tesselation;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampler;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineColorBlendStateCreateInfo colorBlending;
	VkPipelineDynamicStateCreateInfo dynamicState;
	uint32_t subpass;
	VkPipeline	basePipelineHandle;
	int32_t basePipelineIndex;

	VkDynamicState dynamicStates[MAX_DINAMIC_STAT];
	VkBool32 depthStencilEnabled;
	VkBool32 dynamicStateEnabled;
}BkpPipelineGraphicInfo;

typedef struct
{
    VkPipelineLayout * pipelineLayout;
    BkpShaderProgram * shaderProgram;
	//flags
}BkpPipelineComputeInfo;


typedef struct
{
	VkPipelineCacheCreateInfo info;
	VkPipelineCache pipelineCache;
}BkpPipelineCache;

typedef struct
{
    BkpPipelineGraphicInfo info;
    VkPipeline pipeline;
	BkpPipelineCache pipelineCache;
	BkpPipelineLayout pipelineLayout;
}BkpPipelineGraphic;

typedef struct
{
    BkpPipelineComputeInfo info;
    VkPipeline pipeline;
	BkpPipelineCache pipelineCache;
	BkpPipelineLayout pipelineLayout;
}BkpPipelineCompute;
/*********************************************************************
 * Global
 ********************************************************************/

/*********************************************************************
 * Struct
 *********************************************************************/

/*********************************************************************
 * Functions
 *********************************************************************/

//void createComputePipeline(Context * context);
/**
 * @brief Create a graphics pipeline using dynamic rendering (no render pass).
 *
 * Reads `pipeline->info.shaderProgram->stages`, `pipeline->info.colorAttachmentFormat`,
 * and `pipeline->pipelineLayout.layout`.  Uses `VkPipelineRenderingCreateInfo`
 * (VK_KHR_dynamic_rendering) — compatible with @ref bkpBeginRendering.
 *
 * **Prerequisite**: call @ref bkpCreatePipelineMinimalInfo (or fill `pipeline->info`
 * manually), then @ref bkpCreatePipelineLayout, before calling this.
 */
BKP_EXPORTED void bkpCreatePipelineGraphic(BkpGpuAdapter adapter, BkpPipelineGraphic * pipeline);
BKP_EXPORTED void bkpCreatePipelineCompute(BkpGpuAdapter adapter, BkpPipelineCompute * pipeline);

BKP_EXPORTED void bkpCreatePipelineCache(BkpGpuAdapter adapter, BkpPipelineCache * cache);
BKP_EXPORTED void bkpCreateDescriptorPool(BkpGpuAdapter adapter, BkpDescriptorPool * descpool);

/**
 * @brief Create a VkPipelineLayout from a pre-filled `layout->info`.
 *
 * `layout->info` is passed directly to `vkCreatePipelineLayout` — you must set
 * `layout->info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO` before calling,
 * even for an empty layout (no descriptors, no push constants).
 * Prefer @ref bkpCreatePipelineLayoutFromShader when using @ref bkpCreateShader —
 * it derives the layout from SPIR-V reflection and sets sType automatically.
 */
BKP_EXPORTED void bkpCreatePipelineLayout(BkpGpuAdapter adapter, BkpPipelineLayout * ppll);

/**
 * @brief Derive and create a VkPipelineLayout directly from shader reflection.
 *
 * Reads the descriptor set layouts and push constant ranges that were extracted
 * by @ref bkpCreateShader and creates the corresponding VkPipelineLayout.
 * Prefer this over @ref bkpCreatePipelineLayout when using reflected shaders.
 */
BKP_EXPORTED void bkpCreatePipelineLayoutFromShader(BkpGpuAdapter adapter, BkpShaderProgram * prog, BkpPipelineLayout * layout);
/**
 * @brief Create a `VkDescriptorSetLayout` from manually specified bindings.
 *
 * Fill `descSetLayout->bindings` and `descSetLayout->bindingCount` before calling.
 * The bindings are copied internally so the caller's array may be released afterwards.
 *
 * **Mutually exclusive with shader reflection**: do NOT call this on a
 * `BkpDescriptorSetLayout` that was already populated by @ref bkpCreateShader /
 * @ref bkpFinalizeShaderLayouts — those paths call this function internally.
 * Use one path or the other, never both on the same layout.
 */
BKP_EXPORTED void bkpCreateDescriptorSetLayout(BkpGpuAdapter adapter, BkpDescriptorSetLayout * descSetLayout);
BKP_EXPORTED void bkpCreateDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorSet * dset);
/** Write bindings for one specific descriptor set (identified by @p setIndex). */
BKP_EXPORTED void bkpWriteDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorSet * dset,
                                        uint32_t setIndex, BkpWriteDescriptorSetInfo * info);
/** Write bindings for all descriptor sets at once. @p infos must be an array of @c dset->count elements. */
BKP_EXPORTED void bkpWriteDescriptorSets(BkpGpuAdapter adapter, BkpDescriptorSet * dset,
                                         BkpWriteDescriptorSetInfo * infos);
BKP_EXPORTED void bkpCreateDescriptorSetWrite(BkpGpuAdapter adapter, BkpDescriptorSet * dset);

/* Single-set helpers */
BKP_EXPORTED void bkpAllocDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorPool * pool, VkDescriptorSetLayout layout, VkDescriptorSet * outSet);
BKP_EXPORTED void bkpFreeDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorPool * pool, VkDescriptorSet set);
BKP_EXPORTED void bkpWriteDescriptorBuffer(BkpGpuAdapter adapter, VkDescriptorSet set, uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size);
/**
 * @brief Write a combined image-sampler binding into a descriptor set.
 *
 * Convenience wrapper around `vkUpdateDescriptorSets` for the common case of
 * binding a texture + sampler pair to a single descriptor.  The write takes
 * effect immediately (no submit required).
 *
 * Typical usage — bind a sky gradient texture once after pipeline creation:
 * @code
 *   bkpWriteDescriptorImage(adp, descSet, 1,
 *       sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
 * @endcode
 *
 * @param adapter      Active GPU adapter.
 * @param set          Target `VkDescriptorSet`.
 * @param binding      Binding index declared in the GLSL layout.
 * @param sampler      `VkSampler` to bind.
 * @param imageView    `VkImageView` to bind.
 * @param imageLayout  Expected image layout at shader read time (usually
 *                     `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`).
 */
BKP_EXPORTED void bkpWriteDescriptorImage(BkpGpuAdapter adapter, VkDescriptorSet set, uint32_t binding, VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);

BKP_EXPORTED void bkpDestroyPipelineCache(BkpGpuAdapter adapter, BkpPipelineCache * cache);
BKP_EXPORTED void bkpDestroyDescriptorPool(BkpGpuAdapter adapter, BkpDescriptorPool * descriptorPool);
BKP_EXPORTED void bkpDestroyDescriptorSet(BkpGpuAdapter adapter, BkpDescriptorSet * dset);
BKP_EXPORTED void bkpDestroyDescriptorSetLayout(BkpGpuAdapter adapter, BkpDescriptorSetLayout * layout);
/**
 * @brief Destroy the `VkPipeline` handle inside `pipeline`.
 *
 * Only `pipeline->pipeline` is destroyed.  The pipeline layout
 * (`pipeline->pipelineLayout`) and the pipeline cache (`pipeline->pipelineCache`)
 * are **not** touched — call @ref bkpDestroyPipelineLayout and
 * @ref bkpDestroyPipelineCache separately after this.
 */
BKP_EXPORTED void bkpDestroyGraphicPipeline(BkpGpuAdapter adapter, BkpPipelineGraphic * pipeline);
BKP_EXPORTED void bkpDestroyComputePipeline(BkpGpuAdapter adapter, BkpPipelineCompute * pipeline);
/**
 * @brief Destroy the `VkPipelineLayout` handle inside `ppll`.
 *
 * Call this after @ref bkpDestroyGraphicPipeline (or @ref bkpDestroyComputePipeline)
 * since the pipeline must no longer reference the layout at destruction time.
 */
BKP_EXPORTED void bkpDestroyPipelineLayout(BkpGpuAdapter adapter, BkpPipelineLayout * ppll);

#ifdef __cplusplus
}
#endif

#endif

#ifndef  BKP_VK_SHADER_MODULE_H
#define  BKP_VK_SHADER_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vk_types.h"

#include <system/include/bkp_array.h>
#include <system/include/bkp_string.h>

/*********************************************************************
 * Defines
*********************************************************************/
#define MAX_FUNCTION_NAME 64
#define MAX_SPECIALIZATION_ENTRY_MAP 32

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
	uint32_t binding;
	uint32_t count;
	VkDescriptorType type;
	VkSampler * pSamplers;
	BkpBool isRuntimeArray; /* GLSL unsized array (bindless): set=0, binding=N sampler2D t[] */
}BkpSetBindingReflection;

/*_________________________________________________________________________________*/
typedef struct
{
	uint32_t set;
	BkpArray(BkpSetBindingReflection) bindings;
}BkpDescriptorSetRefletion;
/*_________________________________________________________________________________*/

typedef struct
{
  BkpString name;
  size_t size;
  size_t offset;
  VkShaderStageFlagBits stage;
}BkpPushConstantInfo;

/*_________________________________________________________________________________*/
typedef struct
{
	BkpArray(BkpDescriptorSetRefletion) sets;
	VkShaderModule shaderModule;
	VkShaderStageFlagBits stage;
	BkpString entryPoint;
	BkpArray(BkpPushConstantInfo) pushConstants;
}BkpShaderModule;

/*_________________________________________________________________________________*/
typedef struct
{
	//std::vector<SShaderStageInfo> info;
	BkpArray(VkPipelineShaderStageCreateInfo) stages;
	BkpArray(BkpDescriptorSetLayout) layouts;
	BkpArray(BkpPushConstantInfo) pushConstants;

    uint16_t layoutCount;
	BkpBool autoDestroyModules;
} BkpShaderProgram;


/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

BKP_EXPORTED void bkpDebugShaderLayouts(BkpShaderProgram * shaderProgram);
BKP_EXPORTED void bkpDebugShaderConstants(BkpShaderProgram * prog);
BKP_EXPORTED void bkpDebugDescriptorLayout(BkpDescriptorSetLayout * layout);
/**
 * @brief Load a SPIR-V file and run SPIRV-Reflect to populate `smodule`.
 *
 * Reads the `.spv` binary at `szPath`, creates a `VkShaderModule`, then reflects
 * descriptor sets, push constants, and the entry point into `smodule`.
 * The stage (vertex, fragment, compute …) is derived from the reflection data.
 *
 * **Fatal**: aborts if the file cannot be opened or the SPIR-V is malformed.
 *
 * Free with @ref bkpDestroyShaderModule.  If you pass the module to
 * @ref bkpCreateShader with `shaderProgram->autoDestroyModules = BKP_TRUE`,
 * the `VkShaderModule` handle is destroyed automatically when the program is
 * finalized — you only need to free the reflection arrays with
 * @ref bkpDestroyShaderModule in that case.
 *
 * @param szPath  Path to the compiled `.spv` file.
 * @return `BKP_TRUE` on success.
 */
BKP_EXPORTED BkpBool bkpCreateShaderModule(BkpGpuAdapter adapter, const char * szPath, BkpShaderModule * smodule);
BKP_EXPORTED BkpBool bkpCreateShaderModuleFromMemory(BkpGpuAdapter adapter, const char * src, size_t src_len, BkpShaderModule * smodule);

/**
 * @brief Reflect all modules, create `VkDescriptorSetLayout`s, and populate `shaderProgram`.
 *
 * Equivalent to calling @ref bkpReflectShader then @ref bkpFinalizeShaderLayouts
 * in one step.  Merges the reflected descriptor sets and push constants from all
 * `modules` into `shaderProgram->layouts`, `shaderProgram->stages`, and
 * `shaderProgram->pushConstants`.
 *
 * If `shaderProgram->autoDestroyModules == BKP_TRUE`, the `VkShaderModule` handles
 * inside each `BkpShaderModule` are destroyed after the `VkPipelineShaderStageCreateInfo`
 * array is built — the modules may be freed (stack or otherwise) after this call.
 *
 * Use the two-phase alternative (@ref bkpReflectShader + @ref bkpFinalizeShaderLayouts)
 * when you need to inject immutable samplers or variable-count descriptors between
 * reflection and layout creation.
 *
 * Free with @ref bkpDestroyShader.
 *
 * @param modules       Array of pointers to populated `BkpShaderModule`s.
 * @param module_count  Number of entries in `modules`.
 * @return `BKP_TRUE` on success.
 */
BKP_EXPORTED BkpBool bkpCreateShader(BkpGpuAdapter adapter, BkpShaderModule ** modules, uint32_t module_count, BkpShaderProgram * shaderProgram);

/* Two-phase alternative: reflect first, optionally configure, then finalize */
BKP_EXPORTED void bkpReflectShader(BkpGpuAdapter adapter, BkpShaderModule ** modules, uint32_t module_count, BkpShaderProgram * shaderProgram);
BKP_EXPORTED void bkpFinalizeShaderLayouts(BkpGpuAdapter adapter, BkpShaderProgram * shaderProgram);
/* Post-reflect helpers (call between bkpReflectShader and bkpFinalizeShaderLayouts) */
BKP_EXPORTED void bkpShaderSetImmutableSamplers(BkpShaderProgram * prog, uint32_t set, uint32_t binding, const VkSampler * samplers);
BKP_EXPORTED void bkpShaderSetVariableDescriptorMax(BkpShaderProgram * prog, uint32_t set, uint32_t maxCount);

/**
 * @brief Destroy all `VkDescriptorSetLayout`s and free internal arrays in `shaderProgram`.
 *
 * Destroys every layout in `shaderProgram->layouts` and releases the
 * `stages`, `layouts`, and `pushConstants` arrays.
 *
 * **Does not** destroy the `VkShaderModule` handles — those are either already
 * destroyed (if `autoDestroyModules` was `BKP_TRUE` during @ref bkpCreateShader)
 * or must be freed with @ref bkpDestroyShaderModule beforehand.
 */
BKP_EXPORTED void bkpDestroyShader(BkpGpuAdapter adapter,  BkpShaderProgram * shaderProgram);
BKP_EXPORTED VkSpecializationMapEntry bkpCreateSpecializationMapEntry(uint32_t constantID, uint32_t offset, size_t size);
BKP_EXPORTED VkSpecializationInfo bkpCreateSpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data);
BKP_EXPORTED void bkpDestroyConstantInfo(BkpArray(BkpPushConstantInfo) * info);
BKP_EXPORTED void bkpDestroyShaderModule(BkpGpuAdapter adapter, BkpShaderModule * smodule);

#ifdef __cplusplus
}
#endif

#endif

#ifndef __BLUKPAST_H
#define __BLUKPAST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gfx/include/gfx.h>
#include <system/include/bkp_log.h>
#include <system/include/bkp_allocator.h>
#include <system/include/bkp_input.h>
#include <math/include/bkp_vertex_geometries.h>
#include "type.h"
/*
#include "type.h"
*/


/**
 * @par Memory ownership convention
 * Any pointer returned by a `bkp*` function (e.g. `bkpGenerate*`, `bkpCreate*`,
 * `bkpLoad*`) is allocated with `bkpAlloc` and **must** be released with `bkpFree`.
 * Never use the system `free()` on such pointers.
 */

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

struct BkpLogCreateInfo
{
	int level;
	int output;
	const char * logPath;
	char append;
};

struct BkpContextMemoryInfo
{
	size_t generalPageMemorySize;
	size_t gfxPageMemorySize;
	size_t vulkanContextPageSize;
	uint16_t groupCount;
};

typedef struct
{
	BkpVulkanContextInfo vulkanContextInfo;
	struct BkpContextMemoryInfo contextMemoryInfo;
	struct BkpLogCreateInfo logInfo;
	const char * appName;
	void (* resizeWinCallback) (BkpGpuAdapter adp);
}BkpConfig;

typedef struct
{
	BkpVulkanContext vulkanContext;
}BkpContext;

typedef struct
{
	BkpBuffer buffer;
	size_t indicesOffset;
	size_t count;
	VkIndexType indexType;
	BkpBool hasIndices;
}BkpVertexGeometryInfo;



/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/
/*
void bkp_engineStart();
void bkp_engineStop();
*/

/**
 * @brief Initialize the full blukpast stack: memory allocator, logging, Vulkan instance and surface.
 *
 * Use @ref bkpDefaultConfig to get a pre-filled config, then override only what you need.
 * GPU device activation is a separate step — call @ref bkpActivateGpuAdapter after this
 * (allows activating multiple adapters independently).
 *
 * For finer control, the equivalent manual sequence is:
 *  1. `bkpStartAllocator`      — initialize the memory allocator
 *  2. `bkp_log_init`           — configure logging
 *  3. `bkpInitVulkanContext`   — create Vulkan instance, surface, enumerate physical devices
 *  4. `bkpActivateGpuAdapter`  — create logical device, queues, and GPU memory allocator
 *
 * @param context Zero-initialised output context.
 * @param config  Configuration; use @ref bkpDefaultConfig as a starting point.
 * @return BKP_TRUE on success.
 */
BKP_EXPORTED BkpBool bkpInit(BkpContext * context, BkpConfig * config);

/**
 * @brief Tear down the blukpast stack initialised by @ref bkpInit.
 *
 * Destroys the Vulkan instance and surface, stops the memory allocator, and
 * closes the log.  Call @ref bkpClearGpuAdapter and @ref bkpCleanupSwapChain
 * for every activated adapter **before** calling this.
 */
BKP_EXPORTED void bkpQuit(BkpContext * context);

/**
 * @brief Return a BkpConfig filled with sensible defaults.
 *
 * Defaults: 1920×1080, 2 frames in flight, GLFW window, discrete GPU preferred,
 * debug logging to terminal and file.  Override individual fields as needed.
 */
BKP_EXPORTED BkpConfig bkpDefaultConfig();
BKP_EXPORTED BkpBool bkpMakeMeshBuffer(BkpGpuAdapter adapter, size_t vertexCount,
                                    BkpVec3 * positions, BkpVec3 * normals, BkpVec3 * UVs, BkpVec3 * tangents,
                                    BkpVec4 * colors, size_t indiceCount, uint32_t * indices, uint16_t * indices16,
                                    BkpVertexGeometryInfo * geoInfo);
#define BKP_NORMALS_KEEP      0
#define BKP_NORMALS_RECOMPUTE 1

BKP_EXPORTED BkpBool bkpMakeMeshBufferEx(BkpGpuAdapter adapter, size_t vertexCount,
                                    BkpVec3 * positions, BkpVec3 * normals, BkpVec3 * UVs, BkpVec3 * tangents,
                                    BkpVec4 * colors, size_t indiceCount, uint32_t * indices, uint16_t * indices16,
                                    BkpVertexGeometryInfo * geoInfo, size_t scratchGroupId, int recomputeNormals);
BKP_EXPORTED void    bkpSetNormalThreshold(float value);
BKP_EXPORTED BkpBool bkpMakeSkinMeshBufferEx(BkpGpuAdapter adapter, size_t vertexCount,
                                    BkpVec3 * positions, BkpVec3 * normals, BkpVec3 * UVs, BkpVec3 * tangents,
                                    BkpVec4i * joints, BkpVec4 * weights,
                                    size_t indiceCount, uint32_t * indices, uint16_t * indices16,
                                    BkpVertexGeometryInfo * geoInfo, size_t scratchGroupId);
/**
 * @brief Generate GPU vertex + index buffers for a built-in geometry type.
 *
 * Selects the correct generation function for @p type, builds the CPU-side
 * `BkpVertexGeometry`, uploads it to device-local memory, then frees the
 * temporary CPU copy.  The resulting handles are stored in @p geo.
 *
 * The tessellation quality of circular primitives (sphere, cylinder, cone,
 * torus, tube, disk) is controlled globally by
 * @ref bkpSetDefaultSegmentForCircularGeometries — call that before this
 * function if the default of 64 segments is not suitable.
 *
 * @param adapter  Active GPU adapter.
 * @param type     One of the `eBKP_GEO_*` enum values.
 * @param geo      Output: populated with vertex buffer, index buffer, and
 *                 draw-call metadata.
 */
BKP_EXPORTED void bkpCreateVertexIndexBuffer(BkpGpuAdapter adapter, BkpEGeometryType type, BkpVertexGeometryInfo * geo);
BKP_EXPORTED void bkpMakeVertexIndexBuffer(BkpGpuAdapter adapter, BkpVertexGeometry * geo, BkpVertexGeometryInfo * geoInfo);

BKP_EXPORTED void bkpGenerateVertexUIBuffer(BkpGpuAdapter adapter, BkpVertexGeometryInfo * geoInfo, BkpVertexUI * vertices, size_t vertexCount);
BKP_EXPORTED void bkpGenerateVertexIndexBuffer16(BkpGpuAdapter adapter, BkpVertexGeometryInfo * geoInfo, BkpVertex * vertices, size_t vertexCount, uint16_t * indices, size_t indexCount);
BKP_EXPORTED void bkpGenerateVertexIndexBuffer(BkpGpuAdapter adapter, BkpVertexGeometryInfo * geoInfo, BkpVertex * vertices, size_t vertexCount, uint32_t * indices, size_t indexCount);

#ifdef __cplusplus
}
#endif

#endif


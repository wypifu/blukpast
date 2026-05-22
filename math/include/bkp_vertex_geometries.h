#ifndef BKP_VERTEX_GEOMETIES_ 
#define  BKP_VERTEX_GEOMETIES_

#ifdef __cplusplus
extern "C" {
#endif

#include "bkp_vect.h"
#include "bkp_vect.h"
#include <system/include/bkp_array.h>
#include <system/include/bkp_bool.h>

/**
 * @brief Built-in geometry types recognised by @ref bkpCreateVertexIndexBuffer.
 *
 * ## Standard vs. OPTIMIZED variants
 *
 * Double-sided geometries (PLANE, DISK_PLANE, TUBE) exist in two flavours:
 *
 * | Variant | Vertex count | Notes |
 * |---------|--------------|-------|
 * | Standard (`eBKP_GEO_PLANE`, …) | 2× — each face has its own vertices with correct per-face normals and tangents | Works with any fragment shader; use `VK_CULL_MODE_BACK_BIT` (default). |
 * | Optimized (`eBKP_GEO_PLANE_OPTIMIZED`, …) | 1× — front and back faces share vertices; reversed-winding indices produce the second face | **Requires** `gl_FrontFacing` in the fragment shader to flip normals for back-face fragments, and `VK_CULL_MODE_NONE` on the pipeline.  Cuts memory in half — significant at high segment counts (e.g. TUBE at 1024 segments: ~393 KB → ~196 KB). |
 *
 * Use the standard variant by default.  Reach for the OPTIMIZED variant only
 * when memory pressure is a real concern and you control the fragment shader.
 */
typedef enum {eBKP_GEO_LINE, eBKP_GEO_CIRCLE, eBKP_GEO_RECTANGLE, eBKP_GEO_BBOX, eBKP_GEO_ARC,
    eBKP_GEO_QUAD, eBKP_GEO_PLANE, eBKP_GEO_DISK, eBKP_GEO_DISK_PLANE,
    eBKP_GEO_CUBE, eBKP_GEO_SPHERE, eBKP_GEO_DOME, eBKP_GEO_CYLINDER, eBKP_GEO_CONE,
    eBKP_GEO_TUBE, eBKP_GEO_TORUS,
    eBKP_GEO_PLANE_OPTIMIZED, eBKP_GEO_DISK_PLANE_OPTIMIZED, eBKP_GEO_TUBE_OPTIMIZED,
    eBKP_GEO_COUNT
} BkpEGeometryType;

typedef struct
{
	BkpVec3 t; //tangent;
	BkpVec3 b; //bitangent;
}BkpTangentBitangent;

typedef struct
{
    BkpArray(BkpVec3) positions;
    BkpArray(BkpVec3) normals;
    BkpArray(BkpVec3) UVs;
    BkpArray(BkpVec3) tangents;
    //BkpArray(BkpVec4) colors;
    union
    {
        BkpArray(uint16_t) indices16;
        BkpArray(uint32_t) indices32;
    };

    BkpBool hasIndex;
    BkpBool index32;
}BkpVertexGeometry;


typedef struct
{
    BkpArray(BkpVec3) positions;
    BkpArray(BkpVec3) normals;
    BkpArray(BkpVec3) UVs;
    BkpArray(BkpVec3) tangents;
    BkpArray(BkpVec4i) influences;
    BkpArray(BkpVec4) weights;
    //BkpArray(BkpVec4) colors;
    union
    {
        BkpArray(uint16_t) indices16;
        BkpArray(uint32_t) indices32;
    };

    BkpBool hasIndex;
    BkpBool index32;
}BkpSkeletalVertexGeometry;

BKP_EXPORTED void bkpGenerateVertexGeometry16(size_t id, BkpVertexGeometry * geo, BkpVec3 * pos, size_t vPos, BkpVec3 * norm, BkpVec3 * uvs, BkpVec3 * tangs, uint16_t * indices);
BKP_EXPORTED void bkpGenerateVertexGeometry32(size_t id, BkpVertexGeometry * geo, BkpVec3 * pos, size_t vPos, BkpVec3 * norm, BkpVec3 * uvs, BkpVec3 * tangs, uint32_t * indices);


BKP_EXPORTED BkpTangentBitangent bkpCalculateTangent(BkpVec3 * p1, BkpVec3 * p2, BkpVec3 * p3, BkpVec3 * uv1, BkpVec3 * uv2, BkpVec3 * uv3);

BkpBool bkpCreateVertexGeometry(size_t id, BkpEGeometryType type, BkpVertexGeometry * geo);
void bkpDestroyVertexGeometry(BkpVertexGeometry * geo);
/**
 * @brief Set the global segment count for all circular geometry generators.
 *
 * Controls the tessellation quality of sphere, cylinder, cone, torus, tube,
 * and disk primitives.  The value is global and affects every subsequent call
 * to @ref bkpCreateVertexIndexBuffer (and the underlying `bkpCreateGeometry*`
 * functions) until changed again.
 *
 * | Segments | Use case |
 * |----------|----------|
 * | 16–24    | Background / LOD objects |
 * | 32       | Good balance between quality and vertex budget |
 * | 64       | Default — high quality for hero objects |
 * | 128+     | Close-up, high-fidelity; watch memory at 1024+ |
 *
 * @param value  Number of subdivisions around the circumference.  Must be ≥ 3.
 *
 * @see bkpGetDefaultSegmentForCircularGeometries
 */
BKP_EXPORTED void bkpSetDefaultSegmentForCircularGeometries(uint32_t value);
/** @brief Return the current global segment count for circular geometries. */
BKP_EXPORTED uint32_t bkpGetDefaultSegmentForCircularGeometries();

/*basic lines*/
BKP_EXPORTED void bkpCreateGeometryLine(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometryElipse(size_t id, float ratio, BkpVertexGeometry * geo, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryRectangle(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometryBBox(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometryArc(size_t id, BkpVertexGeometry * geo,  float length, float height, uint32_t segCount);

/*plane*/
BKP_EXPORTED void bkpCreateGeometryQuad(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometryPlane(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometryPlaneOptimized(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometryDisk(size_t id, BkpVertexGeometry * geo, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryDiskPlane(size_t id, BkpVertexGeometry * geo, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryDiskPlaneOptimized(size_t id, BkpVertexGeometry * geo, uint32_t segCount);

/*3D*/
BKP_EXPORTED void bkpCreateGeometryCube(size_t id, BkpVertexGeometry * geo);
BKP_EXPORTED void bkpCreateGeometrySpheroid(size_t id, BkpVertexGeometry * geo, float radiusRatio, float angle, uint32_t stackCount, uint32_t sectorCount, BkpBool faceOut);
BKP_EXPORTED void bkpCreateGeometryCylindroid(size_t id, BkpVertexGeometry * geo, float baseRatio, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryCone(size_t id, BkpVertexGeometry * geo, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryTuboid(size_t id, BkpVertexGeometry * geo, float baseRatio, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryTuboidOptimized(size_t id, BkpVertexGeometry * geo, float baseRatio, uint32_t segCount);
BKP_EXPORTED void bkpCreateGeometryTorus(size_t id, BkpVertexGeometry * geo, float ringRadius, float tubeRadius, uint32_t rings, uint32_t segments);

#ifdef __cplusplus
}
#endif

#endif

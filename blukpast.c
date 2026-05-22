#ifdef __cplusplus
extern "C" {
#endif

#include <system/include/bkp_allocator.h>
#include <system/include/bkp_random.h>
#include <math.h>
#include <string.h>

#include "include/blukpast.h"

/*Copyright*/
/*
 * AUTHOR: lilington
 * addr	: lilington@yahoo.fr
 *
 */

/**************************************************************************
*	Defines & Maro
**************************************************************************/
#define gDEFAULT_LOG_PATH "logs/"
#define gDEFAULT_APP_NAME "BKP Application"

/**************************************************************************
*	Structs, Enum, Union and Typesdef
**************************************************************************/

/**************************************************************************
*	Globals
**************************************************************************/

/**************************************************************************
*	Classes
**************************************************************************/

/***************************************************************************
*	Prototypes
**************************************************************************/

/**************************************************************************
*	Implementations
**************************************************************************/

BkpBool bkpInit(BkpContext * context, BkpConfig * config)
{
	config->vulkanContextInfo.appName = config->appName;
	if(config->resizeWinCallback != NULL)
	{
		bkpSetResizeWindowCallBack(config->resizeWinCallback);
	}

	bkpSetVulkanMemoryPage(config->contextMemoryInfo.vulkanContextPageSize);
	bkpSetVulkanAdapterMemoryPage(config->contextMemoryInfo.gfxPageMemorySize);

	bkp_log_init(config->logInfo.level, config->logInfo.output, config->logInfo.logPath, config->logInfo.append);

	bkpStartAllocator(config->contextMemoryInfo.groupCount, config->contextMemoryInfo.generalPageMemorySize);
	bkpInitVulkanContext(&config->vulkanContextInfo, &context->vulkanContext);

    bkpRandomInit();

	return BKP_TRUE;
}

/*____________________________________________________________________________________*/
void bkpQuit(BkpContext * context)
{
	bkpClearContext(&context->vulkanContext);
	bkpStopAllocator();
	bkp_Log_terminate();
}

/*____________________________________________________________________________________*/
BkpConfig bkpDefaultConfig()
{
	BkpConfig config = (BkpConfig){0};
	config.logInfo = (struct BkpLogCreateInfo){0};
	config.logInfo.level = eDEBUG;
	config.logInfo.output = eTERMOUT | eFILEOUT;
	config.logInfo.logPath = gDEFAULT_LOG_PATH;
	config.logInfo.append = eOAPPEND;
	config.resizeWinCallback = NULL;


	config.contextMemoryInfo = (struct BkpContextMemoryInfo){0};
	config.contextMemoryInfo.vulkanContextPageSize = SIZE_1_MIB;
	config.contextMemoryInfo.generalPageMemorySize = SIZE_1_MIB * 64.0f;
	config.contextMemoryInfo.gfxPageMemorySize = SIZE_1_MIB * 4.0f;
	config.contextMemoryInfo.groupCount = 16;

	config.vulkanContextInfo = (BkpVulkanContextInfo){0};
	config.vulkanContextInfo.winWidth = 1920;
	config.vulkanContextInfo.winHeight= 1080;
	config.vulkanContextInfo.appName = gDEFAULT_APP_NAME;
	config.vulkanContextInfo.maxFrameInFlight = 2;
	config.vulkanContextInfo.eWinType = eWIN_GLFW;
	config.vulkanContextInfo.forceIntegratedGPU = BKP_FALSE;
	config.vulkanContextInfo.headless = BKP_FALSE;
	config.vulkanContextInfo.fullScreen = BKP_FALSE;
	config.vulkanContextInfo.vmaMode = eVMA_BKP;

	return config;
}

typedef struct
{
  BkpVec3 * p;
  BkpVec3 * n;
  BkpVec3 * u;
  BkpVec3 * t;
  BkpVec4 * c;
  uint32_t * i32;
  uint16_t * i16;
  uint32_t icount;
  size_t count;
  size_t scratchGroupId;
}SIntet;

/*____________________________________________________________________________________*/
static void * prepareIndicesData(size_t vertexSize, size_t * size_indices, SIntet * sv, BkpVertexGeometryInfo * geo)
{
		geo->indicesOffset = vertexSize;

		if(sv->i32 != NULL)
		{
			geo->count = sv->icount;
			*size_indices = sizeof(uint32_t) * geo->count;
			geo->indexType = VK_INDEX_TYPE_UINT32;
      //LOG(eDEBUG, "bkp Mesh Generator", "index32");
			return sv->i32;
		}
		else
		{
			geo->count = sv->icount;
			*size_indices = sizeof(uint16_t) * geo->count;
			geo->indexType = VK_INDEX_TYPE_UINT16;
      //LOG(eDEBUG, "bkp Mesh Generator", "index16");
			return sv->i16;
		}
}

/*____________________________________________________________________________________*/
static void uploadIndices(BkpGpuAdapter adp, void * indiceData, size_t size_indices, BkpVertexGeometryInfo * geo)
{
  VkBuffer b;
  VkDeviceSize boff;
  bkpGetBuffer(geo->buffer, &b);
  bkpGetBufferOffset(geo->buffer, &boff);
  bkpUploadBufferData(adp, geo->buffer, indiceData, geo->indicesOffset, size_indices);
}
                                    
/*____________________________________________________________________________________*/
static void makeMeshPoint(BkpGpuAdapter adp, SIntet * sv, BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertexPoint) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertexPoint) vertices = (BkpVertexPoint *) bkpArrayCreateFrom(BkpVertexPoint, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
static void makeMeshBasic(BkpGpuAdapter adp, SIntet * sv, BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertexBasic) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertexBasic) vertices = (BkpVertexBasic *) bkpArrayCreateFrom(BkpVertexBasic, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
    vertices[i].normal = sv->n[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
static void makeMeshPointUV(BkpGpuAdapter adp, SIntet * sv, BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertexPointUV) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertexPointUV) vertices = (BkpVertexPointUV *) bkpArrayCreateFrom(BkpVertexPointUV, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
    vertices[i].uv = sv->u[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

static void makeMeshBasicUV(BkpGpuAdapter adp, SIntet * sv, BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertexBasicUV) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertexBasicUV) vertices = (BkpVertexBasicUV *) bkpArrayCreateFrom(BkpVertexBasicUV, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
    vertices[i].normal = sv->n[i];
    vertices[i].uv = sv->u[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
static void makeMeshVertex(BkpGpuAdapter adp, SIntet * sv,  BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertex) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertex) vertices = (BkpVertex *) bkpArrayCreateFrom(BkpVertex, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
    vertices[i].normal = sv->n[i];
    vertices[i].uv = sv->u[i];
    vertices[i].tangent = sv->t[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
static void makeMeshPointColor(BkpGpuAdapter adp, SIntet * sv, BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertexPointColored) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertexPointColored) vertices = (BkpVertexPointColored *) bkpArrayCreateFrom(BkpVertexPointColored, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
    vertices[i].color = sv->c[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
static void makeMeshBasicColor(BkpGpuAdapter adp, SIntet * sv, BkpVertexGeometryInfo * geo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertexBasicColored) * sv->count;
	geo->hasIndices = (sv->i16 != NULL || sv->i32 != NULL) ? BKP_TRUE: BKP_FALSE;
	geo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geo->hasIndices)
	{
    indiceData = prepareIndicesData(vertexSize, &size_indices, sv, geo);
	}
	else
	{
		geo->count = sv->count;
		geo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adp, &geo->buffer, &info);

  BkpArray(BkpVertexBasicColored) vertices = (BkpVertexBasicColored *) bkpArrayCreateFrom(BkpVertexBasicColored, sv->scratchGroupId);
	bkpArrayResize(&vertices, sv->count);
	for(size_t i = 0; i < sv->count; ++i)
	{
		vertices[i].position = sv->p[i];
    vertices[i].normal = sv->n[i];
    vertices[i].color = sv->c[i];
	}

	bkpUploadBufferData(adp, geo->buffer, vertices, 0, vertexSize);

	if(geo->hasIndices == BKP_TRUE)
	{
    uploadIndices(adp, indiceData, size_indices, geo);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
BKP_EXPORTED BkpBool bkpMakeMeshBuffer(BkpGpuAdapter adapter, size_t vertexCount,
                                    BkpVec3 * positions, BkpVec3 * normals, BkpVec3 * UVs, BkpVec3 * tangents,
                                    BkpVec4 * colors, size_t indiceCount, uint32_t * indices, uint16_t * indices16,
                                    BkpVertexGeometryInfo * geoInfo)
{
  return bkpMakeMeshBufferEx(adapter, vertexCount, positions, normals, UVs, tangents,
                             colors, indiceCount, indices, indices16, geoInfo, 0, BKP_NORMALS_KEEP);
}

static float gNormalThreshold = 1.0f;

BKP_EXPORTED void bkpSetNormalThreshold(float value) { gNormalThreshold = value; }

static void bkp_ensureNormals(BkpVec3 * pos, BkpVec3 * nor,
                               uint32_t * idx32, uint16_t * idx16,
                               size_t vertCount, size_t idxCount)
{
    if(!pos || !nor || vertCount < 3) return;

    /* pick first triangle */
    uint32_t a, b, c;
    if(idx32 || idx16)
    {
        if(idxCount < 3) return;
        a = idx32 ? idx32[0] : (uint32_t)idx16[0];
        b = idx32 ? idx32[1] : (uint32_t)idx16[1];
        c = idx32 ? idx32[2] : (uint32_t)idx16[2];
    }
    else { a = 0; b = 1; c = 2; }

    if(a >= vertCount || b >= vertCount || c >= vertCount) return;

    /* compute face normal of first triangle */
    float e1x = pos[b].x - pos[a].x, e1y = pos[b].y - pos[a].y, e1z = pos[b].z - pos[a].z;
    float e2x = pos[c].x - pos[a].x, e2y = pos[c].y - pos[a].y, e2z = pos[c].z - pos[a].z;
    float fnx = e1y*e2z - e1z*e2y, fny = e1z*e2x - e1x*e2z, fnz = e1x*e2y - e1y*e2x;
    float fLen = sqrtf(fnx*fnx + fny*fny + fnz*fnz);
    if(fLen < 1e-8f) return; /* degenerate first triangle */
    fnx /= fLen; fny /= fLen; fnz /= fLen;

    /* check stored normal at vertex a */
    float snx = nor[a].x, sny = nor[a].y, snz = nor[a].z;
    float sLen = sqrtf(snx*snx + sny*sny + snz*snz);

    int needRecompute = 0;
    if(sLen < 1e-5f)
    {
        needRecompute = 1;
    }
    else
    {
        snx /= sLen; sny /= sLen; snz /= sLen;
        float dx = snx - fnx, dy = sny - fny, dz = snz - fnz;
        if(sqrtf(dx*dx + dy*dy + dz*dz) > gNormalThreshold) needRecompute = 1;
    }

    if(!needRecompute) return;

    /* accumulate area-weighted face normals */
    memset(nor, 0, sizeof(BkpVec3) * vertCount);
    size_t triCount = (idx32 || idx16) ? idxCount / 3 : vertCount / 3;
    for(size_t tri = 0; tri < triCount; ++tri)
    {
        uint32_t ia, ib, ic;
        if(idx32 || idx16)
        {
            ia = idx32 ? idx32[tri*3+0] : (uint32_t)idx16[tri*3+0];
            ib = idx32 ? idx32[tri*3+1] : (uint32_t)idx16[tri*3+1];
            ic = idx32 ? idx32[tri*3+2] : (uint32_t)idx16[tri*3+2];
        }
        else { ia = tri*3; ib = tri*3+1; ic = tri*3+2; }

        if(ia >= vertCount || ib >= vertCount || ic >= vertCount) continue;

        float ax = pos[ib].x - pos[ia].x, ay = pos[ib].y - pos[ia].y, az = pos[ib].z - pos[ia].z;
        float bx = pos[ic].x - pos[ia].x, by = pos[ic].y - pos[ia].y, bz = pos[ic].z - pos[ia].z;
        float nx = ay*bz - az*by, ny = az*bx - ax*bz, nz = ax*by - ay*bx;

        nor[ia].x += nx; nor[ia].y += ny; nor[ia].z += nz;
        nor[ib].x += nx; nor[ib].y += ny; nor[ib].z += nz;
        nor[ic].x += nx; nor[ic].y += ny; nor[ic].z += nz;
    }

    /* normalize */
    for(size_t i = 0; i < vertCount; ++i)
    {
        float len = sqrtf(nor[i].x*nor[i].x + nor[i].y*nor[i].y + nor[i].z*nor[i].z);
        if(len > 1e-8f) { nor[i].x /= len; nor[i].y /= len; nor[i].z /= len; }
    }
}

BKP_EXPORTED BkpBool bkpMakeMeshBufferEx(BkpGpuAdapter adapter, size_t vertexCount,
                                    BkpVec3 * positions, BkpVec3 * normals, BkpVec3 * UVs, BkpVec3 * tangents,
                                    BkpVec4 * colors, size_t indiceCount, uint32_t * indices, uint16_t * indices16,
                                    BkpVertexGeometryInfo * geoInfo, size_t scratchGroupId, int recomputeNormals)
{
  if(recomputeNormals && normals)
      bkp_ensureNormals(positions, normals, indices, indices16, vertexCount, indiceCount);

  enum Bits {ePOINT, eBASIC= 1, ePOINTUV = 2, eBASICUV = 3, eVERTEX = 7, ePOINTC = 8, eBASICC = 9, eVERTEXC = 13};

  SIntet sv = {.p = positions, .n = normals, .u = UVs, .t = tangents, .c = colors,
               .i32 = indices, .i16 = indices16, .icount = indiceCount, .count = vertexCount,
               .scratchGroupId = scratchGroupId};

  if(sv.u != NULL && sv.c != NULL)
  {
    LOG(eWARNING, "bkp Mesh Generator", "Color + UV attribute for Vertex not supported yet, will use UV only");
    sv.c = NULL;
  }

  if(positions == NULL)
  {
    return BKP_FALSE;
  }
  
  int truc = 0, mask = 0;

  if(sv.n!= NULL)
  {
    truc |=  1;
  }

  if(sv.u != NULL)
  {
    mask = (1 << 1); truc |=  mask;
  }

  if(sv.t!= NULL)
  {
    mask = (1 << 2); truc |=  mask;
  }

  if(sv.c != NULL)
  {
    mask = (1 << 3); truc |=  mask;
  }

  switch(truc)
  {
    default:
      return BKP_FALSE;
    case ePOINT:   makeMeshPoint    (adapter, &sv, geoInfo); break;
    case eBASIC:   makeMeshBasic    (adapter, &sv, geoInfo); break;
    case ePOINTUV: makeMeshPointUV  (adapter, &sv, geoInfo); break;
    case eBASICUV: makeMeshBasicUV  (adapter, &sv, geoInfo); break;
    case eVERTEX:  makeMeshVertex   (adapter, &sv, geoInfo); break;
    case ePOINTC:  makeMeshPointColor(adapter, &sv, geoInfo); break;
    case eBASICC:  makeMeshBasicColor(adapter, &sv, geoInfo); break;
  }

  return BKP_TRUE;
}

/*____________________________________________________________________________________*/
BKP_EXPORTED BkpBool bkpMakeSkinMeshBufferEx(BkpGpuAdapter adapter, size_t vertexCount,
                                    BkpVec3 * positions, BkpVec3 * normals, BkpVec3 * UVs, BkpVec3 * tangents,
                                    BkpVec4i * joints, BkpVec4 * weights,
                                    size_t indiceCount, uint32_t * indices, uint16_t * indices16,
                                    BkpVertexGeometryInfo * geoInfo, size_t scratchGroupId)
{
    if(!positions || vertexCount == 0) return BKP_FALSE;

    BkpBufferInfo info = {};
    size_t size_indices = 0;
    size_t vertexSize = sizeof(BkpVertexSkin) * vertexCount;

    geoInfo->hasIndices    = (indices || indices16) ? BKP_TRUE : BKP_FALSE;
    geoInfo->indicesOffset = 0;

    if(geoInfo->hasIndices)
    {
        geoInfo->indicesOffset = vertexSize;
        geoInfo->count = indiceCount;
        if(indices)  { size_indices = sizeof(uint32_t) * indiceCount; geoInfo->indexType = VK_INDEX_TYPE_UINT32; }
        else         { size_indices = sizeof(uint16_t) * indiceCount; geoInfo->indexType = VK_INDEX_TYPE_UINT16; }
    }
    else
    {
        geoInfo->count     = vertexCount;
        geoInfo->indexType = VK_INDEX_TYPE_NONE_KHR;
    }

    info.usage   = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    info.type    = eBUFFER_GPU;
    info.size    = vertexSize + size_indices;
    info.isImage = BKP_FALSE;
    bkpAllocateBuffer(adapter, &geoInfo->buffer, &info);

    BkpArray(BkpVertexSkin) verts = (BkpVertexSkin *)bkpArrayCreateFrom(BkpVertexSkin, scratchGroupId);
    bkpArrayResize(&verts, vertexCount);

    BkpVec3  z3 = {.T = {0.0f, 0.0f, 0.0f}};
    BkpVec4i zj = {.T = {0, 0, 0, 0}};
    BkpVec4  zw = {.T = {0.0f, 0.0f, 0.0f, 0.0f}};
    for(size_t i = 0; i < vertexCount; ++i)
    {
        verts[i].position = positions[i];
        verts[i].normal   = normals   ? normals[i]  : z3;
        verts[i].uv       = UVs       ? UVs[i]      : z3;
        verts[i].tangent  = tangents  ? tangents[i] : z3;
        verts[i].joints   = joints    ? joints[i]   : zj;
        verts[i].weights  = weights   ? weights[i]  : zw;
    }

    bkpUploadBufferData(adapter, geoInfo->buffer, verts, 0, vertexSize);
    if(geoInfo->hasIndices)
    {
        void * idxData = indices ? (void *)indices : (void *)indices16;
        bkpUploadBufferData(adapter, geoInfo->buffer, idxData, vertexSize, size_indices);
    }
    bkpArrayDestroy(&verts);
    return BKP_TRUE;
}

/*____________________________________________________________________________________*/
BKP_EXPORTED void bkpMakeVertexIndexBuffer(BkpGpuAdapter adapter, BkpVertexGeometry * geo, BkpVertexGeometryInfo * geoInfo)
{
	BkpBufferInfo info = {};
	size_t size_indices = 0;
	size_t vertexSize = sizeof(BkpVertex) * bkpArraySize(geo->positions);
	geoInfo->hasIndices = geo->hasIndex;
	geoInfo->indicesOffset = 0;
	void * indiceData = NULL;

	if(geoInfo->hasIndices)
	{
		geoInfo->indicesOffset = vertexSize;

		if(geo->index32)
		{
			geoInfo->count = bkpArraySize(geo->indices32);
			size_indices = sizeof(uint32_t) * geoInfo->count;
			geoInfo->indexType = VK_INDEX_TYPE_UINT32;
			indiceData = geo->indices32;
		}
		else
		{
			geoInfo->count = bkpArraySize(geo->indices16);
			size_indices = sizeof(uint16_t) * geoInfo->count;
			geoInfo->indexType = VK_INDEX_TYPE_UINT16;
			indiceData = geo->indices16;
		}
	}
	else
	{
		geoInfo->count = bkpArraySize(geo->positions);
		geoInfo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adapter, &geoInfo->buffer, &info);


	BkpArray(BkpVertex) vertices = (BkpVertex *) bkpArrayCreateFrom(BkpVertex, adapter->memoryGroupId);
	bkpArrayResize(&vertices, bkpArraySize(geo->positions));

	for(size_t i = 0; i < bkpArraySize(geo->positions); ++i)
	{
		vertices[i].position = geo->positions[i];
		vertices[i].normal = geo->normals[i];
		vertices[i].uv = geo->UVs[i];
		vertices[i].tangent = geo->tangents[i];
	}

	bkpUploadBufferData(adapter, geoInfo->buffer, vertices, 0, vertexSize);

	VkBuffer b;
	VkDeviceSize boff;
	bkpGetBuffer(geoInfo->buffer, &b);
	bkpGetBufferOffset(geoInfo->buffer, &boff);
	if(geoInfo->hasIndices)
	{
		bkpUploadBufferData(adapter, geoInfo->buffer, indiceData, geoInfo->indicesOffset, size_indices);
	}

	bkpArrayDestroy(&vertices);
}

/*____________________________________________________________________________________*/
void bkpCreateVertexIndexBuffer(BkpGpuAdapter adapter, BkpEGeometryType type, BkpVertexGeometryInfo * geo)
{
	BkpVertexGeometry g = {};
	bkpCreateVertexGeometry(adapter->memoryGroupId, type, &g);
	bkpMakeVertexIndexBuffer(adapter, &g, geo);
	bkpDestroyVertexGeometry(&g);
}

/*____________________________________________________________________________________*/
BKP_EXPORTED void bkpGenerateVertexUIBuffer(BkpGpuAdapter adapter, BkpVertexGeometryInfo * geoInfo, BkpVertexUI * vertices, size_t vertexCount)
{
	BkpBufferInfo info = {};
	size_t vertexSize = sizeof(BkpVertexUI) * vertexCount;
  geoInfo->hasIndices = BKP_FALSE;
	geoInfo->indicesOffset = 0;

  geoInfo->count = vertexCount;
  geoInfo->indexType = VK_INDEX_TYPE_NONE_KHR;

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adapter, &geoInfo->buffer, &info);

	bkpUploadBufferData(adapter, geoInfo->buffer, vertices, 0, vertexSize);
}

/*____________________________________________________________________________________*/
void  bkpGenerateVertexIndexBuffer16(BkpGpuAdapter adapter, BkpVertexGeometryInfo * geoInfo, BkpVertex * vertices, size_t vertexCount, uint16_t * indices, size_t indexCount)
{
	BkpBufferInfo info = {};
	size_t size_indices = sizeof(uint16_t) * indexCount;
	size_t vertexSize = sizeof(BkpVertex) * vertexCount;
  geoInfo->hasIndices = indexCount > 0 ? BKP_TRUE : BKP_FALSE;
	geoInfo->indicesOffset = 0;

	if(geoInfo->hasIndices)
	{
    geoInfo->count = indexCount;
		geoInfo->indicesOffset = vertexSize;
    geoInfo->indexType = VK_INDEX_TYPE_UINT16;
	}
	else
	{
		geoInfo->count = vertexCount;
		geoInfo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adapter, &geoInfo->buffer, &info);

	bkpUploadBufferData(adapter, geoInfo->buffer, vertices, 0, vertexSize);

	VkBuffer b;
	VkDeviceSize boff;
	bkpGetBuffer(geoInfo->buffer, &b);
	bkpGetBufferOffset(geoInfo->buffer, &boff);
	if(geoInfo->hasIndices)
	{
		bkpUploadBufferData(adapter, geoInfo->buffer, indices, geoInfo->indicesOffset, size_indices);
	}

}

/*____________________________________________________________________________________*/
void bkpGenerateVertexIndexBuffer(BkpGpuAdapter adapter, BkpVertexGeometryInfo * geoInfo, BkpVertex * vertices, size_t vertexCount, uint32_t * indices, size_t indexCount)
{
	BkpBufferInfo info = {};
	size_t size_indices = sizeof(uint32_t) * indexCount;
	size_t vertexSize = sizeof(BkpVertex) * vertexCount;
  geoInfo->hasIndices = indexCount > 0 ? BKP_TRUE : BKP_FALSE;
	geoInfo->indicesOffset = 0;

	if(geoInfo->hasIndices)
	{
    geoInfo->count = indexCount;
		geoInfo->indicesOffset = vertexSize;
    geoInfo->indexType = VK_INDEX_TYPE_UINT32;
	}
	else
	{
		geoInfo->count = vertexCount;
		geoInfo->indexType = VK_INDEX_TYPE_NONE_KHR;
	}

	info.usage  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	info.type = eBUFFER_GPU;
	info.size =  vertexSize + size_indices;
	info.isImage = BKP_FALSE;
	bkpAllocateBuffer(adapter, &geoInfo->buffer, &info);

	bkpUploadBufferData(adapter, geoInfo->buffer, vertices, 0, vertexSize);

	VkBuffer b;
	VkDeviceSize boff;
	bkpGetBuffer(geoInfo->buffer, &b);
	bkpGetBufferOffset(geoInfo->buffer, &boff);
	if(geoInfo->hasIndices)
	{
		bkpUploadBufferData(adapter, geoInfo->buffer, indices, geoInfo->indicesOffset, size_indices);
	}

}

#ifdef __cplusplus
}
#endif


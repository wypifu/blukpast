#ifdef __cplusplus
extern "C" {
#endif

#include "math/include/bkp_vect.h"
#include "include/basics.h"
#include "include/bkp_linear_algebra.h"
#include "include/bkp_vertex_geometries.h"
#include <system/include/bkp_log.h>
#include <system/include/bkp_allocator.h>

static uint32_t gDefaultSegment = 64;

typedef BkpTangentBitangent TB;

BkpTangentBitangent bkpCalculateTangent(BkpVec3 * p1, BkpVec3 * p2, BkpVec3 * p3, BkpVec3 * uv1, BkpVec3 * uv2, BkpVec3 * uv3)
{
  BkpVec3 edge1 = bkpVec3Substract(p2, p1);
  BkpVec3 edge2 = bkpVec3Substract(p3, p1);
  BkpVec3 deltaUV1 = bkpVec3Substract(uv2, uv1);
  BkpVec3 deltaUV2 = bkpVec3Substract(uv3, uv1);
  float  f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
  TB tb;
  tb.t = bkpVec3(
    f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
    f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
    f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z));
  tb.b = bkpVec3(
    f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
    f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
    f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z));

  return tb;
}

static void generateVertexNormUvTang(size_t id, BkpVertexGeometry * geo, BkpVec3 * pos, size_t vSize, BkpVec3 * norm, BkpVec3 * uvs, BkpVec3 * tangs);

static void initiGeo(size_t id, BkpVertexGeometry * geo, size_t size, size_t index_size)
{
  geo->positions = bkpArrayCreateFrom(BkpVec3, id);
  geo->normals   = bkpArrayCreateFrom(BkpVec3, id);
  geo->UVs       = bkpArrayCreateFrom(BkpVec3, id);
  geo->tangents  = bkpArrayCreateFrom(BkpVec3, id);

  bkpArrayResize(&geo->positions, size);
  bkpArrayResize(&geo->normals, size);
  bkpArrayResize(&geo->UVs, size);
  bkpArrayResize(&geo->tangents, size);

  geo->index32 = BKP_FALSE;

  if(index_size == 0)
  {
    geo->hasIndex = BKP_FALSE;
    return;
  }

  geo->hasIndex = BKP_TRUE;
  if(index_size < 0xffff)
  {
    geo->indices16   = bkpArrayCreateFrom(uint16_t, id);
    bkpArrayResize(&geo->indices16, index_size);
  }
  else
{
    geo->indices32   = bkpArrayCreateFrom(uint32_t, id);
    bkpArrayResize(&geo->indices32, index_size);
    geo->index32 = BKP_TRUE;
  }
}

/*_________________________________________________________________________________*/
void bkpSetDefaultSegmentForCircularGeometries(uint32_t value)
{
  gDefaultSegment = value;
}

/*_________________________________________________________________________________*/
uint32_t bkpGetDefaultSegmentForCircularGeometries()
{
  return gDefaultSegment;
}

/*_________________________________________________________________________________*/
BkpBool bkpCreateVertexGeometry(size_t id, BkpEGeometryType type, BkpVertexGeometry * geo)
{
  switch(type)
  {
    default:
      return BKP_FALSE;
    case eBKP_GEO_LINE:
      bkpCreateGeometryLine(id, geo);
      break;
    case eBKP_GEO_CIRCLE:
      bkpCreateGeometryElipse(id, 1.0f, geo, gDefaultSegment);
      break;
    case eBKP_GEO_RECTANGLE:
      bkpCreateGeometryRectangle(id, geo);
      break;
    case eBKP_GEO_BBOX:
      bkpCreateGeometryBBox(id, geo);
      break;
    case eBKP_GEO_ARC:
      bkpCreateGeometryArc(id, geo, 1.0f, 0.5f, gDefaultSegment);
      break;
    case eBKP_GEO_QUAD:
      bkpCreateGeometryQuad(id, geo);
      break;
    case eBKP_GEO_PLANE:
      bkpCreateGeometryPlane(id, geo);
      break;
    case eBKP_GEO_DISK:
      bkpCreateGeometryDisk(id, geo, gDefaultSegment);
      break;
    case eBKP_GEO_DISK_PLANE:
      bkpCreateGeometryDiskPlane(id, geo, gDefaultSegment);
      break;
    case eBKP_GEO_CUBE:
      bkpCreateGeometryCube(id, geo);
      break;
    case eBKP_GEO_SPHERE:
      bkpCreateGeometrySpheroid(id, geo, 1.0f, 2 * BKP_PI, gDefaultSegment, gDefaultSegment, BKP_TRUE);
      break;
    case eBKP_GEO_DOME:
      bkpCreateGeometrySpheroid(id, geo, 1.0f, BKP_PI, gDefaultSegment, gDefaultSegment, BKP_FALSE);
      break;
    case eBKP_GEO_CYLINDER:
      bkpCreateGeometryCylindroid(id, geo, 1.0f, gDefaultSegment);
      break;
    case eBKP_GEO_CONE:
      bkpCreateGeometryCone(id, geo, gDefaultSegment);
      break;
    case eBKP_GEO_TUBE:
      bkpCreateGeometryTuboid(id, geo, 1.0f, gDefaultSegment);
      break;
    case eBKP_GEO_TORUS:
      bkpCreateGeometryTorus(id, geo, 0.35f, 0.15f, gDefaultSegment, gDefaultSegment);
      break;
    case eBKP_GEO_PLANE_OPTIMIZED:
      bkpCreateGeometryPlaneOptimized(id, geo);
      break;
    case eBKP_GEO_DISK_PLANE_OPTIMIZED:
      bkpCreateGeometryDiskPlaneOptimized(id, geo, gDefaultSegment);
      break;
    case eBKP_GEO_TUBE_OPTIMIZED:
      bkpCreateGeometryTuboidOptimized(id, geo, 1.0f, gDefaultSegment);
      break;
  }

  return BKP_TRUE;
}

/*_________________________________________________________________________________*/
void bkpDestroyVertexGeometry(BkpVertexGeometry * geo)
{
  bkpArrayDestroy(&geo->positions);
  bkpArrayDestroy(&geo->normals);
  bkpArrayDestroy(&geo->UVs);
  bkpArrayDestroy(&geo->tangents);
  if(geo->hasIndex)
  {
    if(geo->index32)
    {
      bkpArrayDestroy(&geo->indices32);
    }
    else
  {
      bkpArrayDestroy(&geo->indices16);
    }
  }
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryLine(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 2, 0);
  geo->positions[0] = bkpVec3(0.0f, 0.0f, 0.0f);
  geo->positions[1] = bkpVec3(1.0f, 0.0f, 0.0f);
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryElipse(size_t id, float ratio, BkpVertexGeometry * geo, uint32_t segCount)
{
  uint32_t indiceCount = segCount * 2;
  initiGeo(id, geo, segCount, indiceCount);

  BkpVec3 center = bkpVec3(0.0f, 0.0f, 0.0f);
  float radius = 0.5f;
  float alpha = (float)(2 * BKP_PI ) / (segCount);

  //LOG(eERROR, "RIEN", "segCount *2 + 1 = %u, indiceCount = %u", segCount * 2 + 1, bkpArraySize(geo->indices));
  if(ratio < 1)
  {
    ratio = 1;
  }

  for(uint32_t i = 0; i < segCount; ++i)
  {
    geo->positions[i] =  bkpVec3(center.x + (radius * cosf(i * alpha)),
                                 center.y + (radius * ratio * sinf(i * alpha)),
                                 0.0f);
  }

  for(uint32_t i = 0, k = 0; i < segCount - 1; ++i,k += 2)
  {
    if(geo->index32)
    {
      geo->indices32[k] = i;
      geo->indices32[k + 1] = i + 1;
    }
    else
  {
      geo->indices16[k] = i;
      geo->indices16[k + 1] = i + 1;
    }
  }
  if(geo->index32)
  {
    geo->indices32[segCount * 2 - 1] = 0;
  }
  {
    geo->indices16[segCount * 2 - 1] = 0;
  }
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryRectangle(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 8, 0);

  geo->positions[0] = bkpVec3(-0.5f, -0.5f, 0.0f);
  geo->positions[1] = bkpVec3( 0.5f, -0.5f, 0.0f);
  geo->positions[2] = bkpVec3( 0.5f, -0.5f, 0.0f);
  geo->positions[3] = bkpVec3( 0.5f,  0.5f, 0.0f);
  geo->positions[4] = bkpVec3( 0.5f,  0.5f, 0.0f);
  geo->positions[5] = bkpVec3(-0.5f,  0.5f, 0.0f);
  geo->positions[6] = bkpVec3(-0.5f,  0.5f, 0.0f);
  geo->positions[7] = bkpVec3(-0.5f, -0.5f, 0.0f);
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryBBox(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 8, 24);

  geo->positions[0] = bkpVec3(-0.5f, -0.5f, -0.5f);
  geo->positions[1] = bkpVec3(-0.5f, 0.5f, -0.5f);
  geo->positions[2] = bkpVec3( 0.5f,  0.5f, -0.5f);
  geo->positions[3] = bkpVec3( 0.5f, -0.5f, -0.5f);

  geo->positions[4] = bkpVec3(-0.5f, -0.5f, 0.5f);
  geo->positions[5] = bkpVec3(-0.5f,  0.5f, 0.5f);
  geo->positions[6] = bkpVec3( 0.5f,  0.5f, 0.5f);
  geo->positions[7] = bkpVec3( 0.5f, -0.5f, 0.5f);

  geo->indices16[0] = 0; geo->indices16[1] = 1;
  geo->indices16[2] = 1; geo->indices16[3] = 2;
  geo->indices16[4] = 2; geo->indices16[5] = 3;
  geo->indices16[6] = 3; geo->indices16[7] = 0;
  geo->indices16[8] = 4; geo->indices16[9] = 5;
  geo->indices16[10] = 5; geo->indices16[11] = 6;
  geo->indices16[12] = 6; geo->indices16[13] = 7;
  geo->indices16[14] = 7; geo->indices16[15] = 4;
  geo->indices16[16] = 1; geo->indices16[17] = 5;
  geo->indices16[18] = 2; geo->indices16[19] = 6;
  geo->indices16[20] = 0; geo->indices16[21] = 4;
  geo->indices16[22] = 3; geo->indices16[23] = 7;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryArc(size_t id, BkpVertexGeometry * geo, float length, float height, uint32_t segCount)
{
  uint32_t indiceCount = segCount * 2;
  initiGeo(id, geo, segCount + 1, indiceCount);

  BkpLine2D baseLine = bkpCreateLine2D(bkpVec2(-length * 0.5f, 0.0f), bkpVec2(length * 0.5f, 0.0f));
  BkpVec2 v = bkpVec2Scalar(&baseLine.normal, height);
  BkpVec2 high = bkpVec2Sum(&baseLine.center, &v);

  BkpLine2D sideLine1 = bkpCreateLine2D(baseLine.point1, high);
  BkpLine2D sideLine2 = bkpCreateLine2D(baseLine.point2, high);

  BkpVec2 center = bkpVec2Sum(&sideLine1.center, &sideLine1.normal);
  sideLine1 = bkpCreateLine2D(sideLine1.center, center);
  center = bkpVec2Sum(&sideLine2.center, &sideLine2.normal);
  sideLine2 = bkpCreateLine2D(sideLine2.center,center );

  bkpLineIntersect2D(sideLine1, sideLine2, &center );

  sideLine1 = bkpCreateLine2D(center , baseLine.point1);
  sideLine2 = bkpCreateLine2D(center , baseLine.point2);
  float angle = acosf(bkpDot2D(&sideLine1.direction, &sideLine2.direction));
  float radius = sideLine1.length;

  BkpLine2D h2lineCenter = bkpCreateLine2D(high, baseLine.center);
  BkpLine2D center2lineCenter = bkpCreateLine2D(center, baseLine.center);

  if(bkpDot2D(&h2lineCenter.direction, &center2lineCenter.direction) > 0.0f)
  {
    angle = BKP_PI * 2 - angle;
  }
  float alpha = (float)(angle) / (segCount);

  float angle0;
  BkpVec2 finalPoint;
  if(height < 0.0f)
  {
    float lDotXaxis = bkpDot2D(&sideLine1.direction, &bkpXvector2D);
    angle0 = acosf(lDotXaxis);
    float lDotYaxis = bkpDot2D(&sideLine1.direction, &bkpYvector2D);
    if(lDotYaxis < 0.0f)
    {
      angle0 = (BKP_PI * 2) - angle0;
    }
    finalPoint = baseLine.point2;
  }
  else
{
    float lDotXaxis = bkpDot2D(&sideLine2.direction, &bkpXvector2D);
    angle0 = acosf(lDotXaxis);
    float lDotYaxis = bkpDot2D(&sideLine2.direction, &bkpYvector2D);
    if(lDotYaxis < 0.0f)
    {
      angle0 = (BKP_PI * 2) - angle0;
    }
    finalPoint = baseLine.point1;
  }

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float beta = angle0 + (i * alpha);
    geo->positions[i] =  bkpVec3(center.x + (radius * cosf(beta)),center.y + (radius * sinf(beta)), 0.0f);
  }
  geo->positions[segCount] = bkpVec3(finalPoint.x, finalPoint.y, 0.0f);

  for(uint32_t i = 0, k = 0; i < segCount; ++i,k += 2)
  {
    if(geo->index32)
    {
      geo->indices32[k] = i;
      geo->indices32[k + 1] = i + 1;
    }
    else
  {
      geo->indices16[k] = i;
      geo->indices16[k + 1] = i + 1;
    }
  }
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryQuad(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 4, 6);

  geo->positions[0] = bkpVec3(-0.5f, -0.5f, 0.0f);
  geo->positions[1] = bkpVec3( 0.5f, -0.5f, 0.0f);
  geo->positions[2] = bkpVec3( 0.5f, 0.5f, 0.0f);
  geo->positions[3] = bkpVec3(-0.5f, 0.5f, 0.0f);
  geo->normals[0] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[1] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[2] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[3] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->UVs[0] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[1] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[2] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[3] = bkpVec3(0.0f, 0.0f, 0.0f);

  geo->indices16[0] = 1; geo->indices16[1] = 0; geo->indices16[2] = 3;
  geo->indices16[3] = 2; geo->indices16[4] = 1; geo->indices16[5] = 3;

  TB tb;
  tb = bkpCalculateTangent(&geo->positions[1], &geo->positions[0], &geo->positions[3], &geo->UVs[1], &geo->UVs[0], &geo->UVs[3]);
  geo->tangents[1] = geo->tangents[0] = geo->tangents[3] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[2], &geo->positions[1], &geo->positions[3], &geo->UVs[2], &geo->UVs[1], &geo->UVs[3]);
  geo->tangents[2] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryPlane(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 8, 12);

  geo->positions[0] = bkpVec3(-0.5f, -0.5f, 0.0f);
  geo->positions[1] = bkpVec3( 0.5f, -0.5f, 0.0f);
  geo->positions[2] = bkpVec3( 0.5f,  0.5f, 0.0f);
  geo->positions[3] = bkpVec3(-0.5f,  0.5f, 0.0f);
  geo->positions[4] = bkpVec3(-0.5f, -0.5f, 0.0f);
  geo->positions[5] = bkpVec3( 0.5f, -0.5f, 0.0f);
  geo->positions[6] = bkpVec3( 0.5f,  0.5f, 0.0f);
  geo->positions[7] = bkpVec3(-0.5f,  0.5f, 0.0f);

  geo->normals[0] = geo->normals[1] = geo->normals[2] = geo->normals[3] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[4] = geo->normals[5] = geo->normals[6] = geo->normals[7] = bkpVec3(0.0f, 0.0f,  1.0f);

  geo->UVs[0] = geo->UVs[4] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[1] = geo->UVs[5] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[2] = geo->UVs[6] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[3] = geo->UVs[7] = bkpVec3(0.0f, 0.0f, 0.0f);

  geo->indices16[0] = 1; geo->indices16[1] = 0; geo->indices16[2] = 3;
  geo->indices16[3] = 2; geo->indices16[4] = 1; geo->indices16[5] = 3;
  geo->indices16[6] = 7; geo->indices16[7] = 4; geo->indices16[8] = 5;
  geo->indices16[9] = 7; geo->indices16[10] = 5; geo->indices16[11] = 6;

  TB tb;
  tb = bkpCalculateTangent(&geo->positions[1], &geo->positions[0], &geo->positions[3],
                            &geo->UVs[1], &geo->UVs[0], &geo->UVs[3]);
  geo->tangents[1] = geo->tangents[0] = geo->tangents[3] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[2], &geo->positions[1], &geo->positions[3],
                            &geo->UVs[2], &geo->UVs[1], &geo->UVs[3]);
  geo->tangents[2] = tb.t;

  tb = bkpCalculateTangent(&geo->positions[7], &geo->positions[4], &geo->positions[5],
                            &geo->UVs[7], &geo->UVs[4], &geo->UVs[5]);
  geo->tangents[7] = geo->tangents[4] = geo->tangents[5] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[7], &geo->positions[5], &geo->positions[6],
                            &geo->UVs[7], &geo->UVs[5], &geo->UVs[6]);
  geo->tangents[6] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryPlaneOptimized(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 4, 12);

  geo->positions[0] = bkpVec3(-0.5f, -0.5f, 0.0f);
  geo->positions[1] = bkpVec3( 0.5f, -0.5f, 0.0f);
  geo->positions[2] = bkpVec3( 0.5f,  0.5f, 0.0f);
  geo->positions[3] = bkpVec3(-0.5f,  0.5f, 0.0f);

  BkpVec3 n = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[0] = geo->normals[1] = geo->normals[2] = geo->normals[3] = n;

  geo->UVs[0] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[1] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[2] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[3] = bkpVec3(0.0f, 0.0f, 0.0f);

  /* face 1 — CCW from -Z */
  geo->indices16[0] = 1; geo->indices16[1] = 0; geo->indices16[2] = 3;
  geo->indices16[3] = 2; geo->indices16[4] = 1; geo->indices16[5] = 3;
  /* face 2 — reversed winding; requires gl_FrontFacing normal flip in shader */
  geo->indices16[6] = 3; geo->indices16[7] = 0; geo->indices16[8] = 1;
  geo->indices16[9] = 3; geo->indices16[10] = 1; geo->indices16[11] = 2;

  TB tb;
  tb = bkpCalculateTangent(&geo->positions[1], &geo->positions[0], &geo->positions[3],
                            &geo->UVs[1], &geo->UVs[0], &geo->UVs[3]);
  geo->tangents[1] = geo->tangents[0] = geo->tangents[3] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[2], &geo->positions[1], &geo->positions[3],
                            &geo->UVs[2], &geo->UVs[1], &geo->UVs[3]);
  geo->tangents[2] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryDisk(size_t id, BkpVertexGeometry * geo, uint32_t segCount)
{
  uint32_t indiceCount = segCount  * 3;
  initiGeo(id, geo, segCount + 1, indiceCount);

  BkpVec3 center = bkpVec3(0.0f, 0.0f, 0.0f);
  BkpVec3 vNormal = bkpVec3(0.0f, 0.0f, -1.0f);
  float radius = 0.5f;
  float alpha = (float)(2 * BKP_PI ) / (segCount);

  geo->positions[0] = center;
  geo->normals[0] = vNormal;
  geo->UVs[0] = bkpVec3(0.5f, 0.5f, 0.0f);

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    float x = radius * cosin;
    float y = radius * sinus;
    geo->positions[1 + i] =  bkpVec3(x, y, 0.0f);
    geo->normals[1 + i] = vNormal;
    geo->UVs[1 + i] = bkpVec3(cosin * 0.5f + 0.5f, -sinus * 0.5f + 0.5f, 0.0f);
  }

  TB tb;
  for(uint32_t i = 0, j = 0; j < segCount - 1; i += 3, ++j)
  {
    if(geo->index32)
    {
      geo->indices32[i] = 0;
      geo->indices32[i + 1] = j + 2;
      geo->indices32[i + 2] = j + 1;
    }
    else
  {
      geo->indices16[i] = 0;
      geo->indices16[i + 1] = j + 2;
      geo->indices16[i + 2] = j + 1;
    }
    tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[j + 2], &geo->positions[j + 1], &geo->UVs[0], &geo->UVs[j + 2], &geo->UVs[j + 1]);
    geo->tangents[0] = geo->tangents[j + 2] = geo->tangents[j + 1] = tb.t;
  }

  if(geo->index32)
  {
    geo->indices32[indiceCount - 3] = 0;
    geo->indices32[indiceCount - 2] = 1;
    geo->indices32[indiceCount - 1] = segCount;
  }
  else
{
    geo->indices16[indiceCount - 3] = 0;
    geo->indices16[indiceCount - 2] = 1;
    geo->indices16[indiceCount - 1] = segCount;
  }
  tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[1], &geo->positions[segCount], &geo->UVs[0], &geo->UVs[1], &geo->UVs[segCount]);
  geo->tangents[0] = geo->tangents[1] = geo->tangents[segCount] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryDiskPlane(size_t id, BkpVertexGeometry * geo, uint32_t segCount)
{
  uint32_t indiceCount = segCount * 6;
  initiGeo(id, geo, segCount * 2 + 2, indiceCount);

  BkpVec3 center   = bkpVec3(0.0f, 0.0f, 0.0f);
  BkpVec3 vNormal  = bkpVec3(0.0f, 0.0f, -1.0f);
  BkpVec3 vNormal2 = bkpVec3(0.0f, 0.0f,  1.0f);
  float radius = 0.5f;
  float alpha  = (float)(2 * BKP_PI) / segCount;

  geo->positions[0]           = center; geo->normals[0]           = vNormal;  geo->UVs[0]           = bkpVec3(0.5f, 0.5f, 0.0f);
  geo->positions[segCount + 1] = center; geo->normals[segCount + 1] = vNormal2; geo->UVs[segCount + 1] = bkpVec3(0.5f, 0.5f, 0.0f);

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    BkpVec3 p   = bkpVec3(radius * cosin, radius * sinus, 0.0f);
    BkpVec3 uv  = bkpVec3(cosin * 0.5f + 0.5f, -sinus * 0.5f + 0.5f, 0.0f);
    geo->positions[1 + i]             = p;       geo->normals[1 + i]             = vNormal;  geo->UVs[1 + i]             = uv;
    geo->positions[segCount + 2 + i]  = p;       geo->normals[segCount + 2 + i]  = vNormal2; geo->UVs[segCount + 2 + i]  = uv;
  }

  uint32_t f2 = segCount * 3;
  TB tb;

  for(uint32_t i = 0, j = 0; j < segCount - 1; i += 3, ++j)
  {
    if(geo->index32)
    {
      geo->indices32[i]      = 0;           geo->indices32[i + 1]      = j + 2;             geo->indices32[i + 2]      = j + 1;
      geo->indices32[f2 + i] = segCount + 1; geo->indices32[f2 + i + 1] = segCount + 1 + j + 1; geo->indices32[f2 + i + 2] = segCount + 1 + j + 2;
    }
    else
    {
      geo->indices16[i]      = 0;           geo->indices16[i + 1]      = j + 2;             geo->indices16[i + 2]      = j + 1;
      geo->indices16[f2 + i] = segCount + 1; geo->indices16[f2 + i + 1] = segCount + 1 + j + 1; geo->indices16[f2 + i + 2] = segCount + 1 + j + 2;
    }
    tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[j + 2], &geo->positions[j + 1],
                              &geo->UVs[0], &geo->UVs[j + 2], &geo->UVs[j + 1]);
    geo->tangents[0] = geo->tangents[j + 2] = geo->tangents[j + 1] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[segCount + 1], &geo->positions[segCount + 1 + j + 1], &geo->positions[segCount + 1 + j + 2],
                              &geo->UVs[segCount + 1], &geo->UVs[segCount + 1 + j + 1], &geo->UVs[segCount + 1 + j + 2]);
    geo->tangents[segCount + 1] = geo->tangents[segCount + 1 + j + 1] = geo->tangents[segCount + 1 + j + 2] = tb.t;
  }

  if(geo->index32)
  {
    geo->indices32[f2 - 3] = 0;            geo->indices32[f2 - 2] = 1;              geo->indices32[f2 - 1] = segCount;
    geo->indices32[indiceCount - 3] = segCount + 1; geo->indices32[indiceCount - 2] = segCount * 2 + 1; geo->indices32[indiceCount - 1] = segCount + 2;
  }
  else
  {
    geo->indices16[f2 - 3] = 0;            geo->indices16[f2 - 2] = 1;              geo->indices16[f2 - 1] = segCount;
    geo->indices16[indiceCount - 3] = segCount + 1; geo->indices16[indiceCount - 2] = segCount * 2 + 1; geo->indices16[indiceCount - 1] = segCount + 2;
  }
  tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[1], &geo->positions[segCount],
                            &geo->UVs[0], &geo->UVs[1], &geo->UVs[segCount]);
  geo->tangents[0] = geo->tangents[1] = geo->tangents[segCount] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[segCount + 1], &geo->positions[segCount + 2], &geo->positions[segCount * 2 + 1],
                            &geo->UVs[segCount + 1], &geo->UVs[segCount + 2], &geo->UVs[segCount * 2 + 1]);
  geo->tangents[segCount + 1] = geo->tangents[segCount + 2] = geo->tangents[segCount * 2 + 1] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryDiskPlaneOptimized(size_t id, BkpVertexGeometry * geo, uint32_t segCount)
{
  uint32_t indiceCount = segCount * 6;
  initiGeo(id, geo, segCount + 1, indiceCount);

  BkpVec3 center  = bkpVec3(0.0f, 0.0f, 0.0f);
  BkpVec3 vNormal = bkpVec3(0.0f, 0.0f, -1.0f);
  float radius = 0.5f;
  float alpha  = (float)(2 * BKP_PI) / segCount;

  geo->positions[0] = center; geo->normals[0] = vNormal; geo->UVs[0] = bkpVec3(0.5f, 0.5f, 0.0f);

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    geo->positions[1 + i] = bkpVec3(radius * cosin, radius * sinus, 0.0f);
    geo->normals[1 + i]   = vNormal;
    geo->UVs[1 + i]       = bkpVec3(cosin * 0.5f + 0.5f, -sinus * 0.5f + 0.5f, 0.0f);
  }

  uint32_t f2 = segCount * 3;
  TB tb;

  for(uint32_t i = 0, j = 0; j < segCount - 1; i += 3, ++j)
  {
    if(geo->index32)
    {
      geo->indices32[i]      = 0; geo->indices32[i + 1]      = j + 2; geo->indices32[i + 2]      = j + 1;
      geo->indices32[f2 + i] = 0; geo->indices32[f2 + i + 1] = j + 1; geo->indices32[f2 + i + 2] = j + 2;
    }
    else
    {
      geo->indices16[i]      = 0; geo->indices16[i + 1]      = j + 2; geo->indices16[i + 2]      = j + 1;
      geo->indices16[f2 + i] = 0; geo->indices16[f2 + i + 1] = j + 1; geo->indices16[f2 + i + 2] = j + 2;
    }
    tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[j + 2], &geo->positions[j + 1],
                              &geo->UVs[0], &geo->UVs[j + 2], &geo->UVs[j + 1]);
    geo->tangents[0] = geo->tangents[j + 2] = geo->tangents[j + 1] = tb.t;
  }

  if(geo->index32)
  {
    geo->indices32[f2 - 3] = 0; geo->indices32[f2 - 2] = 1; geo->indices32[f2 - 1] = segCount;
    geo->indices32[indiceCount - 3] = 0; geo->indices32[indiceCount - 2] = segCount; geo->indices32[indiceCount - 1] = 1;
  }
  else
  {
    geo->indices16[f2 - 3] = 0; geo->indices16[f2 - 2] = 1; geo->indices16[f2 - 1] = segCount;
    geo->indices16[indiceCount - 3] = 0; geo->indices16[indiceCount - 2] = segCount; geo->indices16[indiceCount - 1] = 1;
  }
  tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[1], &geo->positions[segCount],
                            &geo->UVs[0], &geo->UVs[1], &geo->UVs[segCount]);
  geo->tangents[0] = geo->tangents[1] = geo->tangents[segCount] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryCube(size_t id, BkpVertexGeometry * geo)
{
  initiGeo(id, geo, 24, 36);
  geo->positions[0] = bkpVec3(-0.5f, -0.5f, -0.5f); //front
  geo->positions[1] = bkpVec3(0.5f, -0.5f, -0.5f);
  geo->positions[2] = bkpVec3(0.5f, 0.5f, -0.5f);
  geo->positions[3] = bkpVec3(-0.5f, 0.5f, -0.5f);
  geo->normals[0] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[1] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[2] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->normals[3] = bkpVec3(0.0f, 0.0f, -1.0f);
  geo->UVs[0] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[1] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[2] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[3] = bkpVec3(0.0f, 0.0f, 0.0f);

  geo->positions[4] = bkpVec3(-0.5f, -0.5f, 0.5f); //BACK
  geo->positions[5] = bkpVec3(0.5f, -0.5f, 0.5f);
  geo->positions[6] = bkpVec3(0.5f, 0.5f, 0.5f);
  geo->positions[7] = bkpVec3(-0.5f, 0.5f, 0.5f);
  geo->normals[4] = bkpVec3(0.0f, 0.0f, 1.0f);
  geo->normals[5] = bkpVec3(0.0f, 0.0f, 1.0f);
  geo->normals[6] = bkpVec3(0.0f, 0.0f, 1.0f);
  geo->normals[7] = bkpVec3(0.0f, 0.0f, 1.0f);
  geo->UVs[4] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[5] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[6] = bkpVec3(0.0f, 0.0f, 0.0f);
  geo->UVs[7] = bkpVec3(1.0f, 0.0f, 0.0f);

  geo->positions[8] = bkpVec3(-0.5f, -0.5f, 0.5f); //LEFT
  geo->positions[9] = bkpVec3(-0.5f, -0.5f, -0.5f);
  geo->positions[10] = bkpVec3(-0.5f, 0.5f, -0.5f);
  geo->positions[11] = bkpVec3(-0.5f, 0.5f, 0.5f);
  geo->normals[8] = bkpVec3(-1.0f, 0.0f, 0.0f);
  geo->normals[9] = bkpVec3(-1.0f, 0.0f, 0.0f);
  geo->normals[10] = bkpVec3(-1.0f, 0.0f, 0.0f);
  geo->normals[11] = bkpVec3(-1.0f, 0.0f, 0.0f);
  geo->UVs[8] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[9] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[10] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[11] = bkpVec3(0.0f, 0.0f, 0.0f);

  geo->positions[12] = bkpVec3(0.5f, -0.5f, -0.5f); // RIGHT
  geo->positions[13] = bkpVec3(0.5f, -0.5f, 0.5f);
  geo->positions[14] = bkpVec3(0.5f, 0.5f, 0.5f);
  geo->positions[15] = bkpVec3(0.5f, 0.5f, -0.5f);
  geo->normals[12] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->normals[13] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->normals[14] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->normals[15] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[12] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[13] = bkpVec3(1.0f, 1.0f, 0.0f);
  geo->UVs[14] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[15] = bkpVec3(0.0f, 0.0f, 0.0f);

  geo->positions[16] = bkpVec3(-0.5f, 0.5f, 0.5f); //TOP
  geo->positions[17] = bkpVec3(-0.5f, 0.5f, -0.5f);
  geo->positions[18] = bkpVec3(0.5f, 0.5f, -0.5f);
  geo->positions[19] = bkpVec3(0.5f, 0.5f, 0.5f);
  geo->normals[16] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->normals[17] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->normals[18] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->normals[19] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[16] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[17] = bkpVec3(0.0f, 0.0f, 0.0f);
  geo->UVs[18] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[19] = bkpVec3(1.0f, 1.0f, 0.0f);

  geo->positions[20] = bkpVec3(-0.5f, -0.5f, 0.5f); //BOTTOM
  geo->positions[21] = bkpVec3(-0.5f, -0.5f, -0.5f);
  geo->positions[22] = bkpVec3(0.5f, -0.5f, -0.5f);
  geo->positions[23] = bkpVec3(0.5f, -0.5f, 0.5f);
  geo->normals[20] = bkpVec3(0.0f, -1.0f, 0.0f);
  geo->normals[21] = bkpVec3(0.0f, -1.0f, 0.0f);
  geo->normals[22] = bkpVec3(0.0f, -1.0f, 0.0f);
  geo->normals[23] = bkpVec3(0.0f, -1.0f, 0.0f);
  geo->UVs[20] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[21] = bkpVec3(0.0f, 0.0f, 0.0f);
  geo->UVs[22] = bkpVec3(1.0f, 0.0f, 0.0f);
  geo->UVs[23] = bkpVec3(1.0f, 1.0f, 0.0f);

  TB tb;
  geo->indices16[0] = 0; geo->indices16[1] = 3; geo->indices16[2] = 1; geo->indices16[3] = 3; geo->indices16[4] = 2; geo->indices16[5] = 1;
  tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[3], &geo->positions[1], &geo->UVs[0], &geo->UVs[3], &geo->UVs[1]);
  geo->tangents[0] = geo->tangents[3] = geo->tangents[1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[3], &geo->positions[2], &geo->positions[1], &geo->UVs[3], &geo->UVs[2], &geo->UVs[1]);
  geo->tangents[2] = tb.t;

  geo->indices16[6] = 5; geo->indices16[7] = 6; geo->indices16[8] = 7; geo->indices16[9] = 4; geo->indices16[10] = 5; geo->indices16[11] = 7;
  tb = bkpCalculateTangent(&geo->positions[5], &geo->positions[6], &geo->positions[7], &geo->UVs[5], &geo->UVs[6], &geo->UVs[7]);
  geo->tangents[5] = geo->tangents[6] = geo->tangents[7] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[4], &geo->positions[5], &geo->positions[7], &geo->UVs[4], &geo->UVs[5], &geo->UVs[7]);
  geo->tangents[4] = tb.t;

  geo->indices16[12] = 9; geo->indices16[13] = 8; geo->indices16[14] = 11; geo->indices16[15] = 10; geo->indices16[16] = 9; geo->indices16[17] = 11;
  tb = bkpCalculateTangent(&geo->positions[9], &geo->positions[8], &geo->positions[11], &geo->UVs[9], &geo->UVs[8], &geo->UVs[11]);
  geo->tangents[9] = geo->tangents[8] = geo->tangents[11] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[10], &geo->positions[9], &geo->positions[11], &geo->UVs[10], &geo->UVs[9], &geo->UVs[11]);
  geo->tangents[10] = tb.t;

  geo->indices16[18] = 13; geo->indices16[19] = 12; geo->indices16[20] = 15; geo->indices16[21] = 14; geo->indices16[22] = 13; geo->indices16[23] = 15;
  tb = bkpCalculateTangent(&geo->positions[13], &geo->positions[12], &geo->positions[15], &geo->UVs[13], &geo->UVs[12], &geo->UVs[15]);
  geo->tangents[13] = geo->tangents[12] = geo->tangents[15] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[14], &geo->positions[13], &geo->positions[15], &geo->UVs[14], &geo->UVs[13], &geo->UVs[15]);
  geo->tangents[14] = tb.t;

  geo->indices16[24] = 18; geo->indices16[25] = 17; geo->indices16[26] = 16; geo->indices16[27] = 19; geo->indices16[28] = 18; geo->indices16[29] = 16;
  tb = bkpCalculateTangent(&geo->positions[18], &geo->positions[17], &geo->positions[16], &geo->UVs[18], &geo->UVs[17], &geo->UVs[16]);
  geo->tangents[16] = geo->tangents[17] = geo->tangents[18] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[19], &geo->positions[18], &geo->positions[16], &geo->UVs[19], &geo->UVs[18], &geo->UVs[16]);
  geo->tangents[19] = tb.t;

  geo->indices16[30] = 20; geo->indices16[31] = 21; geo->indices16[32] = 22; geo->indices16[33] = 20; geo->indices16[34] = 22; geo->indices16[35] = 23;
  tb = bkpCalculateTangent(&geo->positions[20], &geo->positions[21], &geo->positions[22], &geo->UVs[20], &geo->UVs[21], &geo->UVs[22]);
  geo->tangents[20] = geo->tangents[21] = geo->tangents[22] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[20], &geo->positions[22], &geo->positions[23], &geo->UVs[20], &geo->UVs[22], &geo->UVs[23]);
  geo->tangents[23] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometrySpheroid(size_t id, BkpVertexGeometry * geo, float radiusRatio, float angle, uint32_t latitudeCount, uint32_t longitudeCount, BkpBool faceOut)
{
  uint32_t pointCount = (latitudeCount + 1) * (longitudeCount + 1);
  uint32_t indiceCount = latitudeCount * longitudeCount * 6;
  initiGeo(id, geo, pointCount, indiceCount);

  float longitudeStep = (float)(angle / longitudeCount);
  float latitudeStep = (float)(angle / 2 / latitudeCount);
  float longitudeAngle, latitudeAngle;
  float x, y, z, xz;
  float s, t;
  float radius = 0.5f;
  uint32_t k = 0;

  if(radiusRatio > 1) radiusRatio = 1;

  for(int i = 0; i <= latitudeCount; ++i)
  {
    latitudeAngle = (float)(BKP_PI_HALF - i * latitudeStep); /* pi/2 → -pi/2 */

    /* Y is the polar axis (world up).
     * UV.t = 0 at north pole (+Y), t = 1 at south pole (-Y).
     * This matches standard equirectangular convention: V=0 at top of image = zenith. */
    xz = radius * cosf(latitudeAngle);
    y  = radius * radiusRatio * sinf(latitudeAngle);

    for(int j = 0; j <= longitudeCount; ++j)
    {
      longitudeAngle = j * longitudeStep;

      x = xz * cosf(longitudeAngle);
      z = xz * sinf(longitudeAngle);
      BkpVec3 v = bkpVec3(x, y, z);
      geo->positions[k] = v;

      if(faceOut) { geo->normals[k] = v; }
      else        { geo->normals[k] = bkpVec3(-x, -y, -z); }

      t = (float)i / latitudeCount; /* 0 = north (+Y), 1 = south (-Y) */
      s = (float)j / longitudeCount;
      geo->UVs[k] = bkpVec3(s, t, 0.0f);
      ++k;
    }
  }

  k = 0;
  int k1, k2;
  // k1--k1+1
  // |  / |
  // | /  |
  // k2--k2+1
  TB tb;
  for(int i = 0; i < latitudeCount; ++i)
  {
    k1 = i * (longitudeCount + 1);     // beginning of current stack
    k2 = k1 + longitudeCount + 1;      // beginning of next stack

    for(int j = 0; j < longitudeCount; ++j, ++k1, ++k2)
    {
      // 2 triangles per sector excluding first and last stacks
      if(faceOut)
      {
        if(i != 0)
        {
          if(geo->index32)
          {
            geo->indices32[k] = k1;
            geo->indices32[k + 1] = k1 + 1;
            geo->indices32[k + 2] = k2;
          }
          else
        {
            geo->indices16[k] = k1;
            geo->indices16[k + 1] = k1 + 1;
            geo->indices16[k + 2] = k2;
          }
          tb = bkpCalculateTangent(&geo->positions[k1], &geo->positions[k1 + 1], &geo->positions[k2], &geo->UVs[k1], &geo->UVs[k1 + 1], &geo->UVs[k2]);
          geo->tangents[k1] = geo->tangents[k1 + 1] = geo->tangents[k2] = tb.t;
          k += 3;
        }

        if(i != (latitudeCount - 1))
        {
          if(geo->index32)
          {
            geo->indices32[k] = k1 + 1;
            geo->indices32[k + 1] = k2 + 1;
            geo->indices32[k + 2] = k2;
          }
          else
        {
            geo->indices16[k] = k1 + 1;
            geo->indices16[k + 1] = k2 + 1;
            geo->indices16[k + 2] = k2;
          }
          tb = bkpCalculateTangent(&geo->positions[k1 + 1], &geo->positions[k2 + 1], &geo->positions[k2], &geo->UVs[k1 + 1], &geo->UVs[k2 + 1], &geo->UVs[k2]);
          geo->tangents[k1 + 1] = geo->tangents[k2 + 1] = geo->tangents[k2] = tb.t;
          k += 3;
        }
      }
      else
    {
        if(i != 0)
        {
          if(geo->index32)
          {
            geo->indices32[k] = k1;
            geo->indices32[k + 1] = k1 + 1;
            geo->indices32[k + 2] = k2;
          }
          else
        {
            geo->indices16[k] = k1;
            geo->indices16[k + 1] = k1 + 1;
            geo->indices16[k + 2] = k2;
          }
          tb = bkpCalculateTangent(&geo->positions[k1], &geo->positions[k1 + 1], &geo->positions[k2], &geo->UVs[k1], &geo->UVs[k1 + 1], &geo->UVs[k2]);
          geo->tangents[k1] = geo->tangents[k1 + 1] = geo->tangents[k2] = tb.t;
          k += 3;
        }

        if(i != (latitudeCount - 1))
        {
          if(geo->index32)
          {
            geo->indices32[k] = k1 + 1;
            geo->indices32[k + 1] = k2 + 1;
            geo->indices32[k + 2] = k2;
          }
          else
        {
            geo->indices16[k] = k1 + 1;
            geo->indices16[k + 1] = k2 + 1;
            geo->indices16[k + 2] = k2;
          }
          tb = bkpCalculateTangent(&geo->positions[k1 + 1], &geo->positions[k2 +1], &geo->positions[k2], &geo->UVs[k1 + 1], &geo->UVs[k2 + 1], &geo->UVs[k2]);
          geo->tangents[k1 + 1] = geo->tangents[k2 + 1] = geo->tangents[k2] = tb.t;
          k += 3;
        }

      }
    }
  }
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryCylindroid(size_t id, BkpVertexGeometry * geo, float baseRatio, uint32_t segCount)
{
  float radius1 = 0.5f;
  float radius2 = radius1 * baseRatio;
  uint32_t indiceCount = segCount  * 12;

  initiGeo(id, geo, segCount * 6 + 2, indiceCount);

  BkpVec3 c1  = bkpVec3(0.0f, -0.5f, 0.0f);
  BkpVec3 c2  = bkpVec3(0.0f,  0.5f, 0.0f);
  BkpVec3 vN1 = bkpVec3(0.0f, -1.0f, 0.0f);
  BkpVec3 vN2 = bkpVec3(0.0f,  1.0f, 0.0f);
  uint32_t p1g1 = segCount + 1;
  uint32_t p1g2 = segCount * 2 + 1;

  uint32_t p2g0 = segCount * 3 + 2;
  uint32_t p2g1 = segCount * 4 + 2;
  uint32_t p2g2 = segCount * 5 + 2;

  float alpha = (float)(2 * BKP_PI ) / (segCount);

  geo->positions[0] = c1;
  geo->normals[0] = vN1;
  geo->UVs[0] = bkpVec3(0.5f, 0.5f, 0.0f);
  geo->positions[p2g0 - 1] = c2;
  geo->normals[p2g0 - 1] = vN2;
  geo->UVs[p2g0 - 1] = bkpVec3(0.5f, 0.5f, 0.0f);

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    float x = radius1 * cosin;
    float z = radius1 * sinus;
    geo->positions[1 + i] =  bkpVec3(x, c1.y, z);
    geo->normals[1 + i] = vN1;
    geo->positions[p1g1 + i] =  geo->positions[1 + i];
    geo->positions[p1g2 + i] =  geo->positions[1 + i];
    geo->UVs[p1g1 + i] = bkpVec3(1.0f - (float)i / segCount, 1.0f, 0.0f);
    geo->UVs[p1g2 + i] = bkpVec3(1.0f - (float)i / segCount, 1.0f, 0.0f);

    geo->positions[p2g0 + i] =  bkpVec3(radius2 * cosin, c2.y, radius2 * sinus);
    geo->normals[p2g0 + i] = vN2;

    geo->UVs[1 + i] = bkpVec3(cosin * 0.5f + 0.5f, sinus * 0.5f + 0.5f, 0.0f);
    geo->UVs[p2g0 + i] = geo->UVs[1 + i];

    geo->positions[p2g1 + i] =  geo->positions[p2g0 + i];
    geo->positions[p2g2 + i] =  geo->positions[p2g0 + i];

    geo->UVs[p2g1 + i] = bkpVec3(1.0f - (float)i / segCount, 0.0f, 0.0f);
    geo->UVs[p2g2 + i] = bkpVec3(1.0f - (float)i / segCount, 0.0f, 0.0f);
  }

  uint32_t f0 = segCount * 3;
  uint32_t f1 = segCount * 6;
  TB tb;

  size_t p1, p2, q1, q2;
  for(uint32_t i = 0, j = 0, k = 0; j < segCount - 1; i += 3, k += 6,  ++j)
  {
    if(i % 2 == 0)
    {
      p1 = p1g1 + j;
      p2 = p1g1 + j + 1;
      q1 = p2g1 + j;
      q2 = p2g1 + j + 1;
    }
    else
  {
      p1 = p1g2 + j;
      p2 = p1g2 + j + 1;
      q1 = p2g2 + j;
      q2 = p2g2 + j + 1;
    }

    BkpVec3 v1 = bkpVec3Substract(&geo->positions[p2], &geo->positions[p1]);
    BkpVec3 v2 = bkpVec3Substract(&geo->positions[q1], &geo->positions[p1]);
    v1 =  bkpCross3D(&v2, &v1);
    geo->normals[p1] = geo->normals[p2] = geo->normals[q1] = geo->normals[q2] = v1;

    if(geo->index32)
    {
      geo->indices32[i] = 0;
      geo->indices32[i + 1] = j + 1;
      geo->indices32[i + 2] = j + 2;

      geo->indices32[f0 + i] = p2g0 - 1;
      geo->indices32[f0 + i + 1] = p2g0 + j + 1;
      geo->indices32[f0 + i + 2] = p2g0 + j;

      geo->indices32[f1 + k] = p1;
      geo->indices32[f1 + k + 1] = q1;
      geo->indices32[f1 + k + 2] = p2;
      geo->indices32[f1 + k + 3] = p2;
      geo->indices32[f1 + k + 4] = q1;
      geo->indices32[f1 + k + 5] = q2;
    }
    else
  {
      geo->indices16[i] = 0;
      geo->indices16[i + 1] = j + 1;
      geo->indices16[i + 2] = j + 2;

      geo->indices16[f0 + i] = p2g0 - 1;
      geo->indices16[f0 + i + 1] = p2g0 + j + 1;
      geo->indices16[f0 + i + 2] = p2g0 + j;

      geo->indices16[f1 + k] = p1;
      geo->indices16[f1 + k + 1] = q1;
      geo->indices16[f1 + k + 2] = p2;
      geo->indices16[f1 + k + 3] = p2;
      geo->indices16[f1 + k + 4] = q1;
      geo->indices16[f1 + k + 5] = q2;
    }

    tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[j + 1], &geo->positions[j + 2], &geo->UVs[0], &geo->UVs[j + 1], &geo->UVs[j + 2]);
    geo->tangents[0] = geo->tangents[j + 1] = geo->tangents[j + 2] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p2g0 - 1], &geo->positions[p2g0 + j + 1], &geo->positions[p2g0 + j], &geo->UVs[p2g0 - 1], &geo->UVs[p2g0 + j + 1], &geo->UVs[p2g0 + j]);
    geo->tangents[p2g0 - 1] = geo->tangents[p2g0 + j + 1] = geo->tangents[p2g0 + j] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p1], &geo->positions[q1], &geo->positions[p2], &geo->UVs[p1], &geo->UVs[q1], &geo->UVs[p2]);
    geo->tangents[p1] = geo->tangents[p2] = geo->tangents[q1] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[q1], &geo->positions[q2], &geo->UVs[p2], &geo->UVs[q1], &geo->UVs[q2]);
    geo->tangents[q2] =  tb.t;

  }

  if(segCount % 2 == 0)
  {
    p1 = segCount * 3;
    q1 = segCount * 6 - 1;
  }
  else
{
    p1 = segCount * 2;
    q1 = segCount * 5 + 1;
  }
  p2 = segCount * 2 + 1;
  q2 = segCount * 5 + 2;
  geo->UVs[p2] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[q2] = bkpVec3(0.0f, 0.0f, 0.0f);
  BkpVec3 x = geo->positions[segCount * 6 +2 - 1];
  BkpVec3 y = geo->positions[p2g2];
  BkpVec3 v1 = bkpVec3Substract(&x, &y);
  x = geo->positions[p2g0 -2];
  BkpVec3 v2 = bkpVec3Substract(&x, &y);
  v1 =  bkpCross3D(&v2, &v1);
  geo->normals[p1g2] = geo->normals[p2g0 - 2] = geo->normals[p2g2] = geo->normals[segCount * 6 + 1] = v1;

  if(geo->index32)
  {
    geo->indices32[f0 - 3] = 0;
    geo->indices32[f0 - 2] = segCount;
    geo->indices32[f0 - 1] = 1;

    geo->indices32[f1 - 3] = p2g0 - 1;
    geo->indices32[f1 - 2] = p2g0;
    geo->indices32[f1 - 1] = p2g1 - 1;

    geo->indices32[indiceCount - 6] = p2;
    geo->indices32[indiceCount - 5] = p1;
    geo->indices32[indiceCount - 4] = q1;
    geo->indices32[indiceCount - 3] = p2;
    geo->indices32[indiceCount - 2] = q1;
    geo->indices32[indiceCount - 1] = q2;
  }
  else
{
    geo->indices16[f0 - 3] = 0;
    geo->indices16[f0 - 2] = segCount;
    geo->indices16[f0 - 1] = 1;

    geo->indices16[f1 - 3] = p2g0 - 1;
    geo->indices16[f1 - 2] = p2g0;
    geo->indices16[f1 - 1] = p2g1 - 1;

    geo->indices16[indiceCount - 6] = p2;
    geo->indices16[indiceCount - 5] = p1;
    geo->indices16[indiceCount - 4] = q1;
    geo->indices16[indiceCount - 3] = p2;
    geo->indices16[indiceCount - 2] = q1;
    geo->indices16[indiceCount - 1] = q2;
  }
  tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[segCount], &geo->positions[1], &geo->UVs[0], &geo->UVs[segCount], &geo->UVs[1]);
  geo->tangents[0] = geo->tangents[segCount] = geo->tangents[1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p2g0 - 1], &geo->positions[p2g0], &geo->positions[p2g1 - 1], &geo->UVs[p2g0 - 1], &geo->UVs[p2g0], &geo->UVs[p2g1 - 1]);
  geo->tangents[p2g0 - 1] = geo->tangents[p2g0] = geo->tangents[p2g1 - 1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[p1], &geo->positions[q1], &geo->UVs[p2], &geo->UVs[p1], &geo->UVs[q1]);
  geo->tangents[p2] = geo->tangents[p1] = geo->tangents[q1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[q1], &geo->positions[q2], &geo->UVs[p2], &geo->UVs[q1], &geo->UVs[q2]);
  geo->tangents[q2] =  tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryCone(size_t id, BkpVertexGeometry * geo, uint32_t segCount)
{
  uint32_t indiceCount = segCount  * 6;
  initiGeo(id, geo, segCount * 4 + 1, indiceCount);

  BkpVec3 c1  = bkpVec3(0.0f, -0.5f, 0.0f);
  BkpVec3 c2  = bkpVec3(0.0f,  0.5f, 0.0f);
  BkpVec3 vN1 = bkpVec3(0.0f, -1.0f, 0.0f);

  geo->positions[0] = c1;
  geo->normals[0] = vN1;
  geo->UVs[0] = bkpVec3(0.5f, 0.5f, 0.0f);

  float alpha = (float)(2 * BKP_PI ) / (segCount);
  float radius = 0.5f;
  uint32_t f0_ = segCount * 2;
  uint32_t f0 = segCount * 3;

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    geo->positions[1 + i] =  bkpVec3(radius * cosin, c1.y, radius * sinus);
    geo->normals[1 + i] = vN1;
    geo->UVs[1 + i] = bkpVec3(cosin * 0.5f + 0.5f, sinus * 0.5f + 0.5f, 0.0f);

    geo->positions[1 + segCount + i] =  geo->positions[1 + i];
    geo->normals[1 + segCount + i] =  geo->positions[1 + i];
    geo->UVs[1 + segCount + i] = bkpVec3(1.0f - (float)i / segCount, 1.0f, 0.0f);

    geo->positions[1 + f0_ + i] =  geo->positions[1 + i];
    geo->normals[1 + f0_ + i] = geo->positions[1 + f0_ + i];
    geo->UVs[1 + f0_ + i] = bkpVec3(1.0f - (float)i / segCount, 1.0f, 0.0f);

    geo->positions[1 + f0 + i] =  c2;
    geo->normals[1 + f0 + i] = bkpVec3(0.0f, 1.0f, 0.0f);
    geo->UVs[1 + f0 + i] = bkpVec3(1.0f - (float)i / segCount, 0.0f, 0.0f);
  }

  TB tb;
  size_t p1, p2, p0;
  for(uint32_t i = 0, j = 0; j < segCount - 1; i += 3,  ++j)
  {
    p0 = f0 + j + 1;

    if(j % 2 == 0)
    {
      p1 = segCount + 1 + j;
      p2 = segCount + 1 + j + 1;
    }
    else
  {
      p1 = f0_ + 1 + j;
      p2 = f0_ + 1 + j + 1;
    }

    BkpVec3 v1 = bkpVec3Substract(&geo->positions[p2], &geo->positions[p0]);
    BkpVec3 v2 = bkpVec3Substract(&geo->positions[p1], &geo->positions[p0]);
    v1 =  bkpCross3D(&v1, &v2);
    geo->normals[p1] = geo->normals[p2] = geo->normals[p0] = v1;

    if(geo->index32)
    {
      geo->indices32[i] = 0;
      geo->indices32[i + 1] = j + 1;
      geo->indices32[i + 2] = j + 2;

      geo->indices32[f0 + i] = p1;
      geo->indices32[f0 + i + 1] = p0;
      geo->indices32[f0 + i + 2] = p2;
    }
    else
  {
      geo->indices16[i] = 0;
      geo->indices16[i + 1] = j + 1;
      geo->indices16[i + 2] = j + 2;

      geo->indices16[f0 + i] = p1;
      geo->indices16[f0 + i + 1] = p0;
      geo->indices16[f0 + i + 2] = p2;
    }
    tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[j + 1], &geo->positions[j + 2], &geo->UVs[0], &geo->UVs[j + 1], &geo->UVs[j + 2]);
    geo->tangents[0] = geo->tangents[j + 1] = geo->tangents[j + 2] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p1], &geo->positions[p0], &geo->positions[p2], &geo->UVs[p1], &geo->UVs[p0], &geo->UVs[p2]);
    geo->tangents[p0] = geo->tangents[p1] = geo->tangents[p2] = tb.t;
  }

  geo->UVs[segCount * 4] = bkpVec3(0.0f, 0.0f, 0.0f);
  geo->UVs[segCount * 2 + 1] = bkpVec3(0.0f, 1.0f, 0.0f);

  BkpVec3 v1 = bkpVec3Substract(&geo->positions[segCount * 4], &geo->positions[f0_]);
  BkpVec3 v2 = bkpVec3Substract(&geo->positions[segCount * 2 + 1], &geo->positions[f0_]);
  v1 =  bkpCross3D(&v1, &v2);
  geo->normals[segCount * 4] = geo->normals[segCount * 2 + 1] = geo->normals[f0_] = v1;

  if(geo->index32)
  {
    geo->indices32[f0 - 3] = 0;
    geo->indices32[f0 - 2] = segCount;
    geo->indices32[f0 - 1] = 1;

    geo->indices32[indiceCount - 3] = f0_;
    geo->indices32[indiceCount - 2] = segCount * 4;
    geo->indices32[indiceCount - 1] = segCount * 2 + 1;
  }
  else
{
    geo->indices16[f0 - 3] = 0;
    geo->indices16[f0 - 2] = segCount;
    geo->indices16[f0 - 1] = 1;

    geo->indices16[indiceCount - 3] = f0_;
    geo->indices16[indiceCount - 2] = segCount * 4;
    geo->indices16[indiceCount - 1] = segCount * 2 + 1;
  }
  tb = bkpCalculateTangent(&geo->positions[0], &geo->positions[segCount], &geo->positions[1], &geo->UVs[0], &geo->UVs[segCount], &geo->UVs[1]);
  geo->tangents[0] = geo->tangents[segCount] = geo->tangents[1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[f0_], &geo->positions[segCount * 4], &geo->positions[segCount * 2 + 1], &geo->UVs[f0_], &geo->UVs[segCount * 4], &geo->UVs[segCount * 2 + 1]);
  geo->tangents[f0_] = geo->tangents[segCount * 4] = geo->tangents[segCount * 2 + 1] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryTuboid(size_t id, BkpVertexGeometry * geo, float baseRatio, uint32_t segCount)
{
  float radius1 = 0.5f;
  float radius2 = radius1 * baseRatio;
  uint32_t indiceCount = segCount * 12;

  initiGeo(id, geo, segCount * 8, indiceCount);

  float c1y = -0.5f;
  float c2y =  0.5f;
  uint32_t p1g1   = segCount;
  uint32_t p2g0   = segCount * 2;
  uint32_t p2g1   = segCount * 3;
  uint32_t o_     = segCount * 4;
  uint32_t o_p1g1 = segCount * 5;
  uint32_t o_p2g0 = segCount * 6;
  uint32_t o_p2g1 = segCount * 7;

  float alpha = (float)(2 * BKP_PI) / segCount;

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    float x = radius1 * cosin;
    float z = radius1 * sinus;
    BkpVec3 pb = bkpVec3(x, c1y, z);
    BkpVec3 pt = bkpVec3(radius2 * cosin, c2y, radius2 * sinus);
    BkpVec3 uv_b = bkpVec3(1.0f - (float)i / segCount, 1.0f, 0.0f);
    BkpVec3 uv_t = bkpVec3(1.0f - (float)i / segCount, 0.0f, 0.0f);

    geo->positions[i]          = pb; geo->positions[p1g1 + i]   = pb;
    geo->positions[p2g0 + i]   = pt; geo->positions[p2g1 + i]   = pt;
    geo->positions[o_ + i]     = pb; geo->positions[o_p1g1 + i] = pb;
    geo->positions[o_p2g0 + i] = pt; geo->positions[o_p2g1 + i] = pt;

    geo->UVs[i]          = uv_b; geo->UVs[p1g1 + i]   = uv_b;
    geo->UVs[p2g0 + i]   = uv_t; geo->UVs[p2g1 + i]   = uv_t;
    geo->UVs[o_ + i]     = uv_b; geo->UVs[o_p1g1 + i] = uv_b;
    geo->UVs[o_p2g0 + i] = uv_t; geo->UVs[o_p2g1 + i] = uv_t;
  }

  uint32_t f0 = segCount * 6;
  size_t p1, p2, q1, q2, p11, p21, q11, q21;
  TB tb;

  for(uint32_t j = 0, k = 0; j < segCount - 1; k += 6, ++j)
  {
    if(j % 2 == 0)
    {
      p1 = j;          p2 = j + 1;          q1 = p2g0 + j;   q2 = p2g0 + j + 1;
      p11 = o_ + j;    p21 = o_ + j + 1;    q11 = o_p2g0 + j; q21 = o_p2g0 + j + 1;
    }
    else
    {
      p1 = p1g1 + j;   p2 = p1g1 + j + 1;  q1 = p2g1 + j;   q2 = p2g1 + j + 1;
      p11 = o_p1g1 + j; p21 = o_p1g1 + j + 1; q11 = o_p2g1 + j; q21 = o_p2g1 + j + 1;
    }

    BkpVec3 v1 = bkpVec3Substract(&geo->positions[p2], &geo->positions[p1]);
    BkpVec3 v2 = bkpVec3Substract(&geo->positions[q1], &geo->positions[p1]);
    v1 = bkpCross3D(&v1, &v2);
    geo->normals[p1] = geo->normals[p2] = geo->normals[q1] = geo->normals[q2] = v1;
    BkpVec3 neg = bkpNegateVec3(&v1);
    geo->normals[p11] = geo->normals[p21] = geo->normals[q11] = geo->normals[q21] = neg;

    if(geo->index32)
    {
      geo->indices32[k]     = p1; geo->indices32[k + 1] = p2; geo->indices32[k + 2] = q1;
      geo->indices32[k + 3] = p2; geo->indices32[k + 4] = q2; geo->indices32[k + 5] = q1;
      geo->indices32[f0 + k]     = p21; geo->indices32[f0 + k + 1] = p11; geo->indices32[f0 + k + 2] = q11;
      geo->indices32[f0 + k + 3] = q11; geo->indices32[f0 + k + 4] = q21; geo->indices32[f0 + k + 5] = p21;
    }
    else
    {
      geo->indices16[k]     = p1; geo->indices16[k + 1] = p2; geo->indices16[k + 2] = q1;
      geo->indices16[k + 3] = p2; geo->indices16[k + 4] = q2; geo->indices16[k + 5] = q1;
      geo->indices16[f0 + k]     = p21; geo->indices16[f0 + k + 1] = p11; geo->indices16[f0 + k + 2] = q11;
      geo->indices16[f0 + k + 3] = q11; geo->indices16[f0 + k + 4] = q21; geo->indices16[f0 + k + 5] = p21;
    }
    tb = bkpCalculateTangent(&geo->positions[p1], &geo->positions[p2], &geo->positions[q1],
                              &geo->UVs[p1], &geo->UVs[p2], &geo->UVs[q1]);
    geo->tangents[p1] = geo->tangents[p2] = geo->tangents[q1] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[q2], &geo->positions[q1],
                              &geo->UVs[p2], &geo->UVs[q2], &geo->UVs[q1]);
    geo->tangents[q2] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p21], &geo->positions[p11], &geo->positions[q11],
                              &geo->UVs[p21], &geo->UVs[p11], &geo->UVs[q11]);
    geo->tangents[p21] = geo->tangents[p11] = geo->tangents[q11] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[q11], &geo->positions[q21], &geo->positions[p21],
                              &geo->UVs[q11], &geo->UVs[q21], &geo->UVs[p21]);
    geo->tangents[q21] = tb.t;
  }

  /* last segment */
  if(segCount % 2 == 0)
  {
    p1 = segCount * 2 - 1; q1 = segCount * 4 - 1;
    p11 = o_p2g0 - 1;      q11 = segCount * 8 - 1;
  }
  else
  {
    p1 = segCount - 1;      q1 = segCount * 3 - 1;
    p11 = o_p1g1 - 1;       q11 = o_p2g1 - 1;
  }
  p2 = segCount; q2 = p2g1; p21 = o_p1g1; q21 = o_p2g1;
  geo->UVs[p2] = bkpVec3(0.0f, 1.0f, 0.0f); geo->UVs[q2]  = bkpVec3(0.0f, 0.0f, 0.0f);
  geo->UVs[p21] = bkpVec3(0.0f, 1.0f, 0.0f); geo->UVs[q21] = bkpVec3(0.0f, 0.0f, 0.0f);

  if(geo->index32)
  {
    geo->indices32[f0 - 6] = p1; geo->indices32[f0 - 5] = p2; geo->indices32[f0 - 4] = q1;
    geo->indices32[f0 - 3] = p2; geo->indices32[f0 - 2] = q2; geo->indices32[f0 - 1] = q1;
    geo->indices32[indiceCount - 6] = p11; geo->indices32[indiceCount - 5] = q11; geo->indices32[indiceCount - 4] = p21;
    geo->indices32[indiceCount - 3] = p21; geo->indices32[indiceCount - 2] = q11; geo->indices32[indiceCount - 1] = q21;
  }
  else
  {
    geo->indices16[f0 - 6] = p1; geo->indices16[f0 - 5] = p2; geo->indices16[f0 - 4] = q1;
    geo->indices16[f0 - 3] = p2; geo->indices16[f0 - 2] = q2; geo->indices16[f0 - 1] = q1;
    geo->indices16[indiceCount - 6] = p11; geo->indices16[indiceCount - 5] = q11; geo->indices16[indiceCount - 4] = p21;
    geo->indices16[indiceCount - 3] = p21; geo->indices16[indiceCount - 2] = q11; geo->indices16[indiceCount - 1] = q21;
  }
  BkpVec3 v1 = bkpVec3Substract(&geo->positions[p2], &geo->positions[p1]);
  BkpVec3 v2 = bkpVec3Substract(&geo->positions[q1], &geo->positions[p1]);
  v1 = bkpCross3D(&v1, &v2);
  geo->normals[p1] = geo->normals[p2] = geo->normals[q1] = geo->normals[q2] = v1;
  BkpVec3 neg = bkpNegateVec3(&v1);
  geo->normals[p11] = geo->normals[p21] = geo->normals[q11] = geo->normals[q21] = neg;

  tb = bkpCalculateTangent(&geo->positions[p1], &geo->positions[p2], &geo->positions[q1],
                            &geo->UVs[p1], &geo->UVs[p2], &geo->UVs[q1]);
  geo->tangents[p1] = geo->tangents[p2] = geo->tangents[q1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[q2], &geo->positions[q1],
                            &geo->UVs[p2], &geo->UVs[q2], &geo->UVs[q1]);
  geo->tangents[q2] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p11], &geo->positions[q11], &geo->positions[q21],
                            &geo->UVs[p11], &geo->UVs[q11], &geo->UVs[p21]);
  geo->tangents[p21] = geo->tangents[p11] = geo->tangents[q11] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p21], &geo->positions[q11], &geo->positions[q21],
                            &geo->UVs[p21], &geo->UVs[q11], &geo->UVs[q21]);
  geo->tangents[q21] = tb.t;
}

/*_________________________________________________________________________________*/
void bkpCreateGeometryTuboidOptimized(size_t id, BkpVertexGeometry * geo, float baseRatio, uint32_t segCount)
{
  float radius1 = 0.5f;
  float radius2 = radius1 * baseRatio;
  uint32_t indiceCount = segCount * 12;

  initiGeo(id, geo, segCount * 4, indiceCount);

  float c1y = -0.5f;
  float c2y =  0.5f;
  uint32_t p1g1 = segCount;
  uint32_t p2g0 = segCount * 2;
  uint32_t p2g1 = segCount * 3;

  float alpha = (float)(2 * BKP_PI) / segCount;

  for(uint32_t i = 0; i < segCount; ++i)
  {
    float cosin = cosf(i * alpha);
    float sinus = sinf(i * alpha);
    float x = radius1 * cosin;
    float z = radius1 * sinus;
    geo->positions[i]        = bkpVec3(x, c1y, z);
    geo->positions[p1g1 + i] = geo->positions[i];
    geo->UVs[i]              = bkpVec3(1.0f - (float)i / segCount, 1.0f, 0.0f);
    geo->UVs[p1g1 + i]      = geo->UVs[i];

    geo->positions[p2g0 + i] = bkpVec3(radius2 * cosin, c2y, radius2 * sinus);
    geo->positions[p2g1 + i] = geo->positions[p2g0 + i];
    geo->UVs[p2g0 + i]       = bkpVec3(1.0f - (float)i / segCount, 0.0f, 0.0f);
    geo->UVs[p2g1 + i]       = geo->UVs[p2g0 + i];
  }

  uint32_t f0 = segCount * 6;  /* inner face indices start */

  size_t p1, p2, q1, q2;
  TB tb;

  for(uint32_t j = 0, k = 0; j < segCount - 1; k += 6, ++j)
  {
    if(j % 2 == 0)
    {
      p1 = j;        p2 = j + 1;
      q1 = p2g0 + j; q2 = p2g0 + j + 1;
    }
    else
    {
      p1 = p1g1 + j;  p2 = p1g1 + j + 1;
      q1 = p2g1 + j;  q2 = p2g1 + j + 1;
    }

    BkpVec3 v1 = bkpVec3Substract(&geo->positions[p2], &geo->positions[p1]);
    BkpVec3 v2 = bkpVec3Substract(&geo->positions[q1], &geo->positions[p1]);
    v1 = bkpCross3D(&v1, &v2);
    geo->normals[p1] = geo->normals[p2] = geo->normals[q1] = geo->normals[q2] = v1;

    if(geo->index32)
    {
      /* outer face */
      geo->indices32[k]     = p1; geo->indices32[k + 1] = p2; geo->indices32[k + 2] = q1;
      geo->indices32[k + 3] = p2; geo->indices32[k + 4] = q2; geo->indices32[k + 5] = q1;
      /* inner face: reversed winding; gl_FrontFacing flips normal in shader */
      geo->indices32[f0 + k]     = q1; geo->indices32[f0 + k + 1] = p2; geo->indices32[f0 + k + 2] = p1;
      geo->indices32[f0 + k + 3] = q1; geo->indices32[f0 + k + 4] = q2; geo->indices32[f0 + k + 5] = p2;
    }
    else
    {
      geo->indices16[k]     = p1; geo->indices16[k + 1] = p2; geo->indices16[k + 2] = q1;
      geo->indices16[k + 3] = p2; geo->indices16[k + 4] = q2; geo->indices16[k + 5] = q1;
      geo->indices16[f0 + k]     = q1; geo->indices16[f0 + k + 1] = p2; geo->indices16[f0 + k + 2] = p1;
      geo->indices16[f0 + k + 3] = q1; geo->indices16[f0 + k + 4] = q2; geo->indices16[f0 + k + 5] = p2;
    }
    tb = bkpCalculateTangent(&geo->positions[p1], &geo->positions[p2], &geo->positions[q1],
                              &geo->UVs[p1], &geo->UVs[p2], &geo->UVs[q1]);
    geo->tangents[p1] = geo->tangents[p2] = geo->tangents[q1] = tb.t;
    tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[q2], &geo->positions[q1],
                              &geo->UVs[p2], &geo->UVs[q2], &geo->UVs[q1]);
    geo->tangents[q2] = tb.t;
  }

  /* last segment: close the seam */
  if(segCount % 2 == 0)
  {
    p1 = segCount * 2 - 1;
    q1 = segCount * 4 - 1;
  }
  else
  {
    p1 = segCount - 1;
    q1 = segCount * 3 - 1;
  }
  p2 = segCount;   /* = p1g1, first of second ring */
  q2 = p2g1;       /* = segCount*3, first of fourth ring */
  geo->UVs[p2] = bkpVec3(0.0f, 1.0f, 0.0f);
  geo->UVs[q2] = bkpVec3(0.0f, 0.0f, 0.0f);

  if(geo->index32)
  {
    geo->indices32[f0 - 6] = p1; geo->indices32[f0 - 5] = p2; geo->indices32[f0 - 4] = q1;
    geo->indices32[f0 - 3] = p2; geo->indices32[f0 - 2] = q2; geo->indices32[f0 - 1] = q1;
    geo->indices32[indiceCount - 6] = q1; geo->indices32[indiceCount - 5] = p2; geo->indices32[indiceCount - 4] = p1;
    geo->indices32[indiceCount - 3] = q1; geo->indices32[indiceCount - 2] = q2; geo->indices32[indiceCount - 1] = p2;
  }
  else
  {
    geo->indices16[f0 - 6] = p1; geo->indices16[f0 - 5] = p2; geo->indices16[f0 - 4] = q1;
    geo->indices16[f0 - 3] = p2; geo->indices16[f0 - 2] = q2; geo->indices16[f0 - 1] = q1;
    geo->indices16[indiceCount - 6] = q1; geo->indices16[indiceCount - 5] = p2; geo->indices16[indiceCount - 4] = p1;
    geo->indices16[indiceCount - 3] = q1; geo->indices16[indiceCount - 2] = q2; geo->indices16[indiceCount - 1] = p2;
  }
  BkpVec3 v1 = bkpVec3Substract(&geo->positions[p2], &geo->positions[p1]);
  BkpVec3 v2 = bkpVec3Substract(&geo->positions[q1], &geo->positions[p1]);
  v1 = bkpCross3D(&v1, &v2);
  geo->normals[p1] = geo->normals[p2] = geo->normals[q1] = geo->normals[q2] = v1;

  tb = bkpCalculateTangent(&geo->positions[p1], &geo->positions[p2], &geo->positions[q1],
                            &geo->UVs[p1], &geo->UVs[p2], &geo->UVs[q1]);
  geo->tangents[p1] = geo->tangents[p2] = geo->tangents[q1] = tb.t;
  tb = bkpCalculateTangent(&geo->positions[p2], &geo->positions[q2], &geo->positions[q1],
                            &geo->UVs[p2], &geo->UVs[q2], &geo->UVs[q1]);
  geo->tangents[q2] = tb.t;
}

/*_________________________________________________________________________________*/
BKP_EXPORTED void bkpGenerateVertexGeometry16(size_t id, BkpVertexGeometry * geo, BkpVec3 * pos, size_t vPos, BkpVec3 * norm, BkpVec3 * uvs, BkpVec3 * tangs, uint16_t * indices)
{
  generateVertexNormUvTang(id, geo, pos, vPos, norm, uvs, tangs);
  geo->index32 = BKP_FALSE;

  size_t s = bkpArraySize(indices);
  if(s == 0)
  {
    geo->hasIndex = BKP_FALSE;
    return;
  }

  geo->hasIndex = BKP_TRUE;
  geo->indices16   = bkpArrayCreateFrom(uint16_t, id);
  bkpArrayResize(&geo->indices16, s);
  for(size_t i = 0; i < s; ++i)
  {
    geo->indices16[i] = indices[i];
  }
}

BKP_EXPORTED void bkpGenerateVertexGeometry32(size_t id, BkpVertexGeometry * geo, BkpVec3 * pos, size_t vPos, BkpVec3 * norm, BkpVec3 * uvs, BkpVec3 * tangs, uint32_t * indices)
{
  generateVertexNormUvTang(id, geo, pos, vPos, norm, uvs, tangs);
  geo->index32 = BKP_FALSE;

  size_t s = bkpArraySize(indices);
  if(s == 0)
  {
    geo->hasIndex = BKP_FALSE;
    return;
  }

  geo->hasIndex = BKP_TRUE;
  geo->indices32   = bkpArrayCreateFrom(uint32_t, id);
  bkpArrayResize(&geo->indices32, s);
  geo->index32 = BKP_TRUE;
  for(size_t i = 0; i < s; ++i)
  {
    geo->indices32[i] = indices[i];
  }
}

/*_________________________________________________________________________________*/
//BKP_EXPORTED void bkpGenerateVertexGeometryFromVertexUI(size_t id, BkpVertexGeometry * geo, BkpVertexUI * v, size_t s);


/*_________________________________________________________________________________*/
static void generateVertexNormUvTang(size_t id, BkpVertexGeometry * geo, BkpVec3 * pos, size_t vSize, BkpVec3 * norm, BkpVec3 * uvs, BkpVec3 * tangs)
{
  geo->positions = bkpArrayCreateFrom(BkpVec3, id);
  geo->normals   = bkpArrayCreateFrom(BkpVec3, id);
  geo->UVs       = bkpArrayCreateFrom(BkpVec3, id);
  geo->tangents  = bkpArrayCreateFrom(BkpVec3, id);

  bkpArrayResize(&geo->positions, vSize);
  bkpArrayResize(&geo->normals, vSize);
  bkpArrayResize(&geo->UVs, vSize);
  bkpArrayResize(&geo->tangents, vSize);

  for(size_t i = 0; i < vSize; ++i)
  {
    geo->positions[i] = pos[i];
  }

  if(norm != NULL)
  {
    for(size_t i = 0; i < vSize; ++i)
    {
      geo->normals[i] = norm[i];
    }
  }

  if(uvs!= NULL)
  {
    for(size_t i = 0; i < vSize; ++i)
    {
      geo->UVs[i] = uvs[i];
    }
  }

  if(tangs != NULL)
  {
    for(size_t i = 0; i < vSize; ++i)
    {
      geo->tangents[i] = tangs[i];
    }
  }
}


/*_________________________________________________________________________________*/
void bkpCreateGeometryTorus(size_t id, BkpVertexGeometry * geo, float ringRadius, float tubeRadius, uint32_t rings, uint32_t segments)
{
  uint32_t vertexCount = (rings + 1) * (segments + 1);
  uint32_t indexCount  = rings * segments * 6;
  initiGeo(id, geo, vertexCount, indexCount);

  for(uint32_t i = 0; i <= rings; ++i)
  {
    float u  = (float)i / rings * 2.0f * BKP_PI;
    float cu = cosf(u), su = sinf(u);
    for(uint32_t j = 0; j <= segments; ++j)
    {
      float v  = (float)j / segments * 2.0f * BKP_PI;
      float cv = cosf(v), sv = sinf(v);
      uint32_t idx = i * (segments + 1) + j;

      geo->positions[idx] = bkpVec3((ringRadius + tubeRadius * cv) * cu,
                                     tubeRadius * sv,
                                     (ringRadius + tubeRadius * cv) * su);
      geo->normals[idx]   = bkpVec3(cv * cu, sv, cv * su);
      geo->UVs[idx]       = bkpVec3((float)i / rings, (float)j / segments, 0.0f);
      geo->tangents[idx]  = bkpVec3(-su, 0.0f, cu);
    }
  }

  uint32_t k = 0;
  for(uint32_t i = 0; i < rings; ++i)
  {
    for(uint32_t j = 0; j < segments; ++j)
    {
      uint32_t v00 = i       * (segments + 1) + j;
      uint32_t v10 = (i + 1) * (segments + 1) + j;
      uint32_t v01 = i       * (segments + 1) + (j + 1);
      uint32_t v11 = (i + 1) * (segments + 1) + (j + 1);

      if(geo->index32)
      {
        geo->indices32[k++] = v00; geo->indices32[k++] = v11; geo->indices32[k++] = v10;
        geo->indices32[k++] = v00; geo->indices32[k++] = v01; geo->indices32[k++] = v11;
      }
      else
      {
        geo->indices16[k++] = (uint16_t)v00; geo->indices16[k++] = (uint16_t)v11; geo->indices16[k++] = (uint16_t)v10;
        geo->indices16[k++] = (uint16_t)v00; geo->indices16[k++] = (uint16_t)v01; geo->indices16[k++] = (uint16_t)v11;
      }
    }
  }
}

/*_________________________________________________________________________________*/

#ifdef __cplusplus
}
#endif

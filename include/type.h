#ifndef BKP_TYPE_H
#define BKP_TYPE_H


#ifdef __cplusplus
extern "C" {
#endif

#include <math/include/bkpmath.h>
#include <system/include/bkp_export.h>
#include <stdalign.h>

/*********************************************************************
 * Defines
 *********************************************************************/
#define MAX_VERTEX_ATTRIBUTE 8
#define MAX_SWAPCHAIN_COUNT 16
#define VERTEX_QUAD_NO_INDICE_COUNT 6

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
	alignas(16) BkpMat4 model;
    alignas(16) BkpMat4 view;
    alignas(16) BkpMat4 projection;

    BkpVec4 color;
}BkpUniformBasicMVP;

/*___________________________________________________________________*/
typedef struct
{
    BkpVec3 position;
}BkpVertexPoint;

/*___________________________________________________________________*/
typedef struct
{
  BkpVec3 position;
  BkpVec4 color;
}BkpVertexPointColored;

/*___________________________________________________________________*/
typedef struct
{
  BkpVec3 position;
  BkpVec3 uv;
}BkpVertexPointUV;

/*___________________________________________________________________*/
typedef struct
{
  BkpVec2 position;
  BkpVec3 uv;
}BkpVertexUI;

/*___________________________________________________________________*/
typedef struct
{
  BkpVec3 position;
  BkpVec3 normal;
  BkpVec3 uv;
  BkpVec3 tangent;
}BkpVertex;

typedef struct
{
    BkpVec3 position;
    BkpVec3 normal;
    BkpVec3 uv;
    BkpVec3 tangent;
    BkpVec4 color;
}BkpVertexColored;

typedef struct
{
    BkpVec3  position;
    BkpVec3  normal;
    BkpVec3  uv;
    BkpVec3  tangent;
    BkpVec4i joints;
    BkpVec4  weights;
}BkpVertexSkin;

typedef struct
{
    BkpVec3 position;
    BkpVec3 normal;
}BkpVertexBasic;

typedef struct
{
  BkpVec3 position;
  BkpVec3 normal;
  BkpVec4 color;
}BkpVertexBasicColored;

typedef struct
{
    BkpVec3 position;
    BkpVec3 normal;
	BkpVec3 uv;
}BkpVertexBasicUV;

/*********************************************************************
 * Class
 *********************************************************************/

/*********************************************************************
 * Global
 *********************************************************************/

/*********************************************************************
 * Functions
 *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif


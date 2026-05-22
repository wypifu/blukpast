#ifndef BKP_BASICS_H_
#define BKP_BASICS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "bkp_float.h"

/*********************************************************************
 * Defines
*********************************************************************/
#define BKP_PI 3.1415926535897932
#define BKP_PI_DOUBLE 6.2831853071795864
#define BKP_PI_HALF 1.5707963267948966
#define BKP_PI_QUARTER .78539816339744830
#define BKP_PI_INV	.31830988618379067543
#define BKP_PI_DOUBLE_INV  .15915494309189533771
#define BKP_DEG2RAD(value)  (value)  * (BKP_PI   /  180.0f)
#define BKP_RAD2DEG(value)  (value)  *  (180.0f / BKP_PI)
#define BKP_DEG2RADF(value)  (float)((value)  * (float)(BKP_PI   /  180.0f))
#define BKP_RAD2DEGF(value)  (float)((value)  *  (float)(180.0f / BKP_PI))

#define BKP_INFINITY 1e30f
#define BKP_FLOAT_EPSILON 1.192092896e-07f

/*********************************************************************
 * Type def & enum
*********************************************************************/

/*********************************************************************
 * Macro
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Global
*********************************************************************/

/*********************************************************************
 * Struct
*********************************************************************/

/*********************************************************************
 * Functions
*********************************************************************/

BKP_EXPORTED int64_t bkpMin(int64_t int1, int64_t int2);
BKP_EXPORTED int64_t bkpMax(int64_t int1, int64_t int2);
BKP_EXPORTED float bkpMinf(float f1, float f2);
BKP_EXPORTED float bkpMaxf(float f1, float f2);

#ifdef __cplusplus
}
#endif

#endif


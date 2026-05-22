#ifndef BKP_LINEAR_ALGEBRA_H
#define BKP_LINEAR_ALGEBRA_H

#include "bkp_vect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {eNONE, eINTERSECT, eCOLINEAR} ELineResult ;

typedef struct
{
    BkpVec2 point1;
    BkpVec2 point2;
    BkpVec2 direction;
    BkpVec2 normal;
    BkpVec2 center;
    float length;
    float lengthSquare;
}BkpLine2D;

typedef struct
{
    BkpVec3 origin;
    BkpVec3 direction; // should be normalized
}BkpRay;

BKP_EXPORTED BkpLine2D bkpCreateLine2D(BkpVec2 point1, BkpVec2 point2);
BKP_EXPORTED BkpBool bkpLineIntersect2D(BkpLine2D l1, BkpLine2D l2, BkpVec2 * r);

BKP_EXPORTED BkpRay bkpCreateRay(BkpVec3 origin, BkpVec3 target);
BKP_EXPORTED ELineResult bkpRayIntersect(BkpRay r1, BkpRay r2, BkpVec3 * pos);

#ifdef __cplusplus
}
#endif

#endif /* BKP_VECT_H_ */
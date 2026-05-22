#include "include/bkp_linear_algebra.h"

#ifdef __cplusplus
extern "C" {
#endif


BkpLine2D bkpCreateLine2D(BkpVec2 point1, BkpVec2 point2)
{
    BkpLine2D l;
    l.point1 = point1;
    l.point2 = point2;
    l.direction = bkpVec2Substract(&l.point2, &l.point1);
    l.lengthSquare = l.length = bkpMagnitude2D(&l.direction);
    l.lengthSquare *= l.length;
    bkpSetNormalize2D(&l.direction);
    l.normal = bkpVec2(-l.direction.y, l.direction.x);
    BkpVec2 v = bkpVec2Scalar(&l.direction, l.length * 0.5f);
    l.center = bkpVec2Sum(&l.point1, &v);

    return l;
}

/*_________________________________________________________________________________*/
BkpBool bkpLineIntersect2D(BkpLine2D l1, BkpLine2D l2, BkpVec2 * r)
{

  float nu = ((l1.point1.x * l1.direction.y) - (l1.point1.y * l1.direction.x)) - ((l2.point1.x * l1.direction.y) - (l2.point1.y * l1.direction.x));
  float dn = (l1.direction.y * l2.direction.x) - (l1.direction.x * l2.direction.y);

  if(bkpFloatEq(dn, 0.0f) == BKP_TRUE)
  {
    /* not interested in infinit intersection point
    if(bkpFloatEq(nu, 0.0f) == BKP_TRUE)
    {
        *r = l2.point1;
        return BKP_TRUE;
    }
    */
    return BKP_FALSE; 
  }

  float d = nu / dn;
  BkpVec2 v = bkpVec2Scalar(&l2.direction, d);
  *r = bkpVec2Sum(&l2.point1, &v);

  return BKP_TRUE;
}

#ifdef __cplusplus
}
#endif
#include "include/bkp_linear_algebra.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/*_________________________________________________________________________________*/
BkpRay bkpCreateRay(BkpVec3 origin, BkpVec3 target)
{
  BkpRay r;
  r.origin = origin;
  r.direction = bkpVec3Substract(&target, &origin);
  bkpSetNormalize3D(&r.direction);

  return r;
}

/*_________________________________________________________________________________*/
ELineResult bkpRayIntersect(BkpRay r1, BkpRay r2, BkpVec3 * pos)
{
  float d = fabs(bkpDot3D(&r1.direction, &r2.direction));

  if(bkpFloatEq(d, 1.0f) == BKP_TRUE)
  {
    BkpVec3 v = bkpVec3Substract(&r1.origin, &r2.origin);
    bkpSetNormalize3D(&v);
    d = fabs(bkpDot3D(&r1.direction, &v));
    if(bkpFloatEq(d, 1.0f) == BKP_TRUE)
    {
      *pos = r1.origin;
      return eCOLINEAR;
    }
    else
    {
      return eNONE;
    }
  }

  float t2;

  /*
  r1.origin.x + t1 * r1.direction.x = r2.origin.x + r2.direction.x * t2;
  r1.origin.y + t1 * r1.direction.y = r2.origin.y + r2.direction.y * t2;
  r1.origin.z + t1 * r1.direction.z = r2.origin.z + r2.direction.z * t2;

  t1 =   (r2.origin.z - r1.poin.z + r2.direcion.z * t2) / r1.direction.z

  a =  r1.direction.x / r1.direction.z
  dem - r2.direction.x - a * r2.direction.z;

  t2 = (r1.origin.x - r2.origin.x + a * r2.origin.z - a * r1.origin.z) / dem 

  */
  if(bkpFloatEq(r1.direction.z, 0.0f) == BKP_FALSE)
  {
      float a = r1.direction.x / r1.direction.z;
      float dem = (r2.direction.x  -  a * r2.direction.z);

      if(bkpFloatEq(dem, 0.0f) == BKP_FALSE)
      {
        t2 = (r1.origin.x - r2.origin.x + a * r2.origin.z -  a * r1.origin.z ) / dem;
      }
      else
      {
        a = r1.direction.y / r1.direction.z ;
        dem = (r2.direction.y  -  a * r2.direction.z);
        if(bkpFloatEq(dem, 0.0f) == BKP_FALSE)
        {
          t2 = (r1.origin.y - r2.origin.y + a * r2.origin.z -  a * r1.origin.z ) / dem;
        }
        else
        {
          return eNONE;
        }
      }
  }
  else if(bkpFloatEq(r1.direction.y, 0.0f) == BKP_FALSE)
  {
    float a = r1.direction.x / r1.direction.y;
    float dem = (r2.direction.x  -  a * r2.direction.y);

    if(bkpFloatEq(dem, 0.0f) == BKP_FALSE)
    {
        t2 = (r1.origin.x - r2.origin.x + a * r2.origin.y -  a * r1.origin.y ) / dem;
    }
    else
    {
      a = r1.direction.z / r1.direction.y ;
      dem = (r2.direction.z  -  a * r2.direction.y);
      if(bkpFloatEq(dem, 0.0f) == BKP_FALSE)
      {
        t2 = (r1.origin.z - r2.origin.z + a * r2.origin.y -  a * r1.origin.y ) / dem;
      }
      else
      {
        return eNONE;
      }
    }
  }
  else if(bkpFloatEq(r1.direction.x, 0.0f) == BKP_FALSE)
  {
    float a = r1.direction.y / r1.direction.x;
    float dem = (r2.direction.y  -  a * r2.direction.x);

    if(bkpFloatEq(dem, 0.0f) == BKP_FALSE)
    {
        t2 = (r1.origin.y - r2.origin.y + a * r2.origin.x -  a * r1.origin.x ) / dem;
    }
    else
    {
      a = r1.direction.z / r1.direction.x ;
      dem = (r2.direction.z  -  a * r2.direction.x);
      if(bkpFloatEq(dem, 0.0f) == BKP_FALSE)
      {
        t2 = (r1.origin.z - r2.origin.z + a * r2.origin.x -  a * r1.origin.x ) / dem;
      }
      else
      {
        return eNONE;
      }
    }
  }
  else 
  {
    return eNONE;
  }

  pos->x = r2.origin.x + r2.direction.x * t2;
  pos->y = r2.origin.y + r2.direction.y * t2;
  pos->z = r2.origin.z + r2.direction.z * t2;

  return eINTERSECT;
}

#ifdef __cplusplus
}
#endif
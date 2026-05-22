#ifndef BKP_MAT_H_
#define BKP_MAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bkp_vect.h"

typedef struct
{
	union
	{
		BkpVec4 mData;
		struct
		{
			float m00, m01, m10, m11;
		};
	};

}BkpMat2;

/*_____________________________________________________________________________________________*/
typedef struct
{
		BkpVec3 mLine0, mLine1, mLine2;
}BkpMat3;

BKP_EXPORTED BkpMat2 bkpMat2(float a00, float a01, float a10, float a11);
BKP_EXPORTED BkpVec2 bkpMat2GetColumn0(const BkpMat2 * mat);
BKP_EXPORTED void bkpMat2SetColumn0(BkpMat2 * mat, BkpVec2 * v);
BKP_EXPORTED BkpVec2 bkpMat2GetColumn1(const BkpMat2 * mat);
BKP_EXPORTED void bkpMat2SetColumn1(BkpMat2 * mat, BkpVec2 * v);
BKP_EXPORTED BkpVec2 bkpMat2GetLine0(const BkpMat2 * mat);
BKP_EXPORTED void bkpMat2SetLine0(BkpMat2 * mat, BkpVec2 * v);
BKP_EXPORTED BkpVec2 bkpMat2GetLine1(const BkpMat2 * mat);
BKP_EXPORTED void bkpMat2SetLine1(BkpMat2 * mat, BkpVec2 * v);
BKP_EXPORTED BkpMat2 bkpMat2Sum(const BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED void  bkpMat2Add(BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED BkpMat2 bkpNegateMat2(BkpMat2 * m1);
BKP_EXPORTED BkpMat2 bkpMat2Substract(const BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED void bkpMat2Sub(BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED BkpMat2 bkpMat2xScalar(const BkpMat2 * m1, const float scalar);
BKP_EXPORTED void bkpMat2MultScalar(BkpMat2 * m1, const float scalar);
BKP_EXPORTED BkpMat2 bkpMat2xVec2(const BkpMat2 * m1, const BkpVec2 * v);
BKP_EXPORTED BkpMat2 bkpMat2xMat2(const BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED void bkpMat2MultMat2(BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED BkpVec2 bkpMat2DotVec2(const BkpMat2 * m1, const BkpVec2 * v);
BKP_EXPORTED BkpVec2 bkpVec2DotMat2(const BkpVec2 * v, const BkpMat2 * m);
BKP_EXPORTED BkpMat2 bkpMat2DotMat2(const BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED void bkpMat2Dot(BkpMat2 * m1, const BkpMat2 * m2);
BKP_EXPORTED void bkpSetIdentityMat2(BkpMat2 * m);
BKP_EXPORTED BkpMat2 bkpIdentityMat2();
BKP_EXPORTED BkpMat2 bkpNullMat2();
BKP_EXPORTED void bkpPrintMat2(const char * title, BkpMat2 * m);


BKP_EXPORTED BkpMat3 bkpMat3(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22);
BKP_EXPORTED BkpMat3 bkpMat3FromVec3(BkpVec3 v1, BkpVec3 v2, BkpVec3 v3);
BKP_EXPORTED BkpVec3 bkpMat3GetColumn0(const BkpMat3 * mat);
BKP_EXPORTED void bkpMat3SetColumn0(BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpMat3GetColumn1(const BkpMat3 * mat);
BKP_EXPORTED void bkpMat3SetColumn1(BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpMat3GetColumn2(const BkpMat3 * mat);
BKP_EXPORTED void bkpMat3SetColumn2(BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpMat3GetLine0(const BkpMat3 * mat);
BKP_EXPORTED void bkpMat3SetLine0(BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpMat3GetLine1(const BkpMat3 * mat);
BKP_EXPORTED void bkpMat3SetLine1(BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpMat3GetLine2(const BkpMat3 * mat);
BKP_EXPORTED void bkpMat3SetLine2(BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpMat2 bkpMat3GetUpper(const BkpMat3 * m);
BKP_EXPORTED BkpMat3 bkpMat3Sum(const BkpMat3 * m1, const BkpMat3 * m2);
BKP_EXPORTED void bkpMat3Add(BkpMat3 * m1, const BkpMat3 * m2);
BKP_EXPORTED BkpMat3 bkpNegateMat3(BkpMat3 * m1);
BKP_EXPORTED BkpMat3 bkpMat3Substract(const BkpMat3 * m1, const BkpMat3 * m2);
BKP_EXPORTED void bkpMat3Sub(BkpMat3 * m1, const BkpMat3 * m2);
BKP_EXPORTED BkpMat3 bkpMat3xScalar(const BkpMat3 * m1, const float scalar);
BKP_EXPORTED void bkpMat3MultScalar(BkpMat3 * m1, const float scalar);
BKP_EXPORTED BkpMat3 bkpMat3xVec3(const BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpMat3 bkpMat3xMat3(const BkpMat3 * m, const BkpMat3 * v);
BKP_EXPORTED void bkpMat3MultMat3(BkpMat3 * m1, const BkpMat3 * m2);
BKP_EXPORTED BkpVec3 bkpMat3DotVec3(const BkpMat3 * m, const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpVec3DotMat3(const BkpVec3 * v, const BkpMat3 * m);
BKP_EXPORTED void bkpPrintMat3(const char * title, BkpMat3 * m);
BKP_EXPORTED void bkpSetIdentityMat3(BkpMat3 * m);
BKP_EXPORTED BkpMat3 bkpIdentityMat3();
BKP_EXPORTED BkpMat3 bkpNullMat3();

/*_____________________________________________________________________________________________*/
typedef struct
{
	BkpVec4 mLine0, mLine1, mLine2, mLine3;
}BkpMat4;

BKP_EXPORTED BkpMat4 bkpMat4(float a00, float a01, float a02, float a03, float a10, float a11, float a12, float a13,
		float a20, float a21, float a22, float a23, float a30, float a31, float a32, float a33);
BKP_EXPORTED BkpVec4 bkpMat4GetColumn0(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetColumn0(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetColumn1(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetColumn1(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetColumn2(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetColumn2(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetColumn3(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetColumn3(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetLine0(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetLine0(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetLine1(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetLine1(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetLine2(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetLine2(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpMat4GetLine3(const BkpMat4 * mat);
BKP_EXPORTED void bkpMat4SetLine3(BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpMat2 bkpMat4GetUpper2D(const BkpMat4 * m);
BKP_EXPORTED BkpMat3 bkpMat4GetUpper3D(const BkpMat4 * m);
BKP_EXPORTED BkpMat4 bkpMat4Sum(const BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED void bkpMat4Add(BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED BkpMat4 bkpNegateMat4(BkpMat4 * m1);
BKP_EXPORTED BkpMat4 bkpMat4Substract(const BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED void bkpMat4Sub(BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED BkpMat4 bkpMat4xScalar(const BkpMat4 * m1, const float scalar);
BKP_EXPORTED void bkpMat4MultScalar(BkpMat4 * m1, const float scalar);
BKP_EXPORTED BkpMat4 bkpMat4xVec4(const BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpMat4 bkpMat4xMat4(const BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED void bkpMat4MultMat4(BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED BkpVec4 bkpMat4DotVec4(const BkpMat4 * m, const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpVec4DotMat4(const BkpVec4 * v, const BkpMat4 * m);
BKP_EXPORTED BkpMat4 bkpMat4DotMat4(const BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED void bkpMat4Dot(BkpMat4 * m1, const BkpMat4 * m2);
BKP_EXPORTED void bkpPrintMat4(const char * title, BkpMat4 * m);
BKP_EXPORTED void bkpSetIdentityMat4(BkpMat4 * m);
BKP_EXPORTED BkpMat4 bkpIdentityMat4();
BKP_EXPORTED BkpMat4 bkpNullMat4();

BKP_EXPORTED void bkpMat4SetScale(BkpMat4 * m, const BkpVec3 * v);
BKP_EXPORTED void bkpMat4SetTranslate(BkpMat4 * m, const BkpVec3 * v);
BKP_EXPORTED void bkpMat4SetRotate(BkpMat4 * m, const BkpVec3 * axis, float angle);
BKP_EXPORTED void bkpMat4SetRotateQuatAxis(BkpMat4 * m, const BkpVec3 * axis, float angle);
BKP_EXPORTED void bkpMat4SetRotateQuat(BkpMat4 * m, const BkpQuat * q);

/*_____________________________________________________________________________________________*/


BKP_EXPORTED float bkpMat2Det(const BkpMat2 * mat);
BKP_EXPORTED float bkpMat3Det(const BkpMat3 * mat);
BKP_EXPORTED float bkpMat4Det(const BkpMat4 * mat);

BKP_EXPORTED BkpMat2 bkpMat2Inverse(const BkpMat2 * mat);
BKP_EXPORTED BkpMat3 bkpMat3Inverse(const BkpMat3 * mat);
BKP_EXPORTED BkpMat4 bkpMat4Inverse(const BkpMat4 * mat);
BKP_EXPORTED BkpMat2 bkpMat2DetInverse(const BkpMat2 * mat, float determinant);
BKP_EXPORTED BkpMat3 bkpMat3DetInverse(const BkpMat3 * mat, float determinant);
BKP_EXPORTED BkpMat4 bkpMat4DetInverse(const BkpMat4 * mat, float determinant);

BKP_EXPORTED BkpMat2 bkpMat2Transpose(const BkpMat2 * mat);
BKP_EXPORTED BkpMat3 bkpMat3Transpose(const BkpMat3 * mat);
BKP_EXPORTED BkpMat4 bkpMat4Transpose(const BkpMat4 * mat);

BKP_EXPORTED BkpMat2 bkpMat2InverseTranspose(const BkpMat2 * mat);
BKP_EXPORTED BkpMat3 bkpMat3InverseTranspose(const BkpMat3 * mat);
BKP_EXPORTED BkpMat4 bkpMat4InverseTranspose(const BkpMat4 * mat);

BKP_EXPORTED BkpMat2 bkpMat2Rotation(float angle);

BKP_EXPORTED BkpMat4 bkpMat4GetRotationX(float angle);
BKP_EXPORTED BkpMat4 bkpMat4GetRotationY(float angle);
BKP_EXPORTED BkpMat4 bkpMat4GetRotationZ(float angle);
BKP_EXPORTED BkpMat4 bkpMat4GetRotation(const BkpVec3 * unit_axis, float radian);

BKP_EXPORTED BkpMat4 bkpMat4RotateX(const BkpMat4 * mat, float radian);
BKP_EXPORTED BkpMat4 bkpMat4RotateY(const BkpMat4 * mat, float radian);
BKP_EXPORTED BkpMat4 bkpMat4RotateZ(const BkpMat4 * mat, float radian);
BKP_EXPORTED BkpMat4 bkpMat4Rotate(const BkpMat4 * mat, const BkpVec3 * unit_axis, float radian);

BKP_EXPORTED BkpMat4 bkpMat4RotateQAxis(const BkpMat4 * mat, const BkpVec3 * unit_axis, float radian);
BKP_EXPORTED BkpMat4 bkpMat4RotateQ(const BkpMat4 * mat, const BkpQuat * q);
BKP_EXPORTED BkpMat4 bkpMat4RotateQCenter(const BkpMat4 * mat, const BkpQuat * quat, const BkpVec3 * center);

BKP_EXPORTED BkpMat4 bkpGetTranslation(const BkpVec3 * v);
BKP_EXPORTED BkpMat4 bkpTranslate(const BkpMat4 * mat, const BkpVec3 * v);
BKP_EXPORTED BkpMat4 bkpGetScaling(const BkpVec3 * v);
BKP_EXPORTED BkpMat4 bkpScale(const BkpMat4 * mat, const BkpVec3 * v);

BKP_EXPORTED BkpMat4 bkpOrtho(float left, float right, float bottom, float top, float near, float far);
BKP_EXPORTED BkpMat4 bkpPerspective(float vFOV, float ratio, float near, float far);
BKP_EXPORTED BkpMat4 bkpLookAt(BkpVec3 position, BkpVec3 target, BkpVec3 up);



#ifdef __cplusplus
}
#endif

#endif /* BKP_MAT_H_ */

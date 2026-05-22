#ifndef BKP_VECT_H_
#define BKP_VECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <immintrin.h>
#include <stdint.h>
#include "bkp_float.h"
#include "basics.h"

typedef struct
{
	union
	{
		struct {
			union {
				int32_t x, w, s, u;
			};
			union {
				int32_t y, h, t, v;
			};
		};
		int32_t T[2];
	};
}BkpVec2i;

/*_____________________________________________________________________________________________*/
typedef struct
{
	union
	{
		struct {
			union {
				float x, w, s, u;
			};
			union {
				float y, h, t, v;
			};
		};

		float T[2];
	};
}BkpVec2;

/*_____________________________________________________________________________________________*/
typedef struct
{
	union
	{
		__m128 m128;

		struct {
			union {
				float x, w, r, s, u;
			};
			union {
				float y, h, g, t, v;
			};
			union {
				float z, d, b, p, q;
			};
		};

		float T[3];
	};
}BkpVec3;

/*_____________________________________________________________________________________________*/
typedef struct
{
	union
	{
		struct {
			union {
				uint32_t x, r;
			};
			union {
				uint32_t y, g;
			};
			union {
				uint32_t z, b;
			};
			union {
				uint32_t w, a;
			};
		};

		uint32_t T[4];
	};
}BkpVec4i;

/*_____________________________________________________________________________________________*/
typedef struct
{
	union
	{
		__m128 m128;

		struct {
			union {
				float x, r;
			};
			union {
				float y, g;
			};
			union {
				float z, b;
			};
			union {
				float w, a;
			};
		};

		float T[4];
	};
}BkpVec4;

/*_____________________________________________________________________________________________*/
typedef struct
{
	union
	{
		__m128 m128;

		struct {
			float x, y, z, w;
		};

		float mT[4];
	};
}BkpQuat;

/*_____________________________________________________________________________________________*/

BKP_EXPORTED BkpVec2i bkpVec2i(int32_t x, int32_t y);
BKP_EXPORTED BkpVec2i bkpVec2iSum(const BkpVec2i *v1, const BkpVec2i * v2);
BKP_EXPORTED void bkpVec2iAdd(BkpVec2i * v1, const BkpVec2i * v2);
BKP_EXPORTED BkpVec2i bkpVec2iSubstract(const BkpVec2i * v1, const BkpVec2i * v2);
BKP_EXPORTED void bkpVec2iSub(BkpVec2i * v1, const BkpVec2i * v2);
BKP_EXPORTED BkpVec2i bkpVec2iScalar(const BkpVec2i * v, int32_t scalar);
BKP_EXPORTED void bkpVec2iMultScalar(BkpVec2i * v, int32_t scalar);
BKP_EXPORTED BkpVec2i bkpVec2ixVec2i(const BkpVec2i * v, const BkpVec2i * v1);
BKP_EXPORTED void bkpVec2iMultVec2i(BkpVec2i * v, const BkpVec2i * v1);
BKP_EXPORTED BkpVec2i bkpVec2iDivideScalar(const BkpVec2i * v, int32_t scalar);
BKP_EXPORTED void bkpVec2iDivScalar(BkpVec2i * v, int32_t scalar);
BKP_EXPORTED BkpVec2i bkpVec2iDivideVec2i(const BkpVec2i * v, const BkpVec2i * w);
BKP_EXPORTED void bkpVec2iDivVec2i(BkpVec2i * v, BkpVec2i * w);
BKP_EXPORTED void bkpPrintVec2i(const char * title, const BkpVec2i * v);

BKP_EXPORTED BkpVec2 bkpVec2(float x, float y);
BKP_EXPORTED BkpVec2 bkpVec2Sum(const BkpVec2 * v1, const BkpVec2 * v2);
BKP_EXPORTED void bkpVec2Add(BkpVec2 * v1, const BkpVec2 * v2);
BKP_EXPORTED BkpVec2 bkpVec2Substract(const BkpVec2 * v1, const BkpVec2 * v2);
BKP_EXPORTED void bkpVec2Sub(BkpVec2 * v1, const BkpVec2 * v2);
BKP_EXPORTED BkpVec2 bkpVec2Scalar(const BkpVec2 * v, float scalar);
BKP_EXPORTED void bkpVec2MultScalar(BkpVec2 * v, float scalar);
BKP_EXPORTED BkpVec2 bkpVec2xVec2(const BkpVec2 * v, const BkpVec2 * v1);
BKP_EXPORTED void bkpVec2MultVec2(BkpVec2 * v, const BkpVec2 * v1);
BKP_EXPORTED BkpVec2 bkpVec2DivideScalar(const BkpVec2 * v, float scalar);
BKP_EXPORTED void bkpVec2DivScalar(BkpVec2 * v, float scalar);
BKP_EXPORTED BkpVec2 bkpVec2DivideVec2(const BkpVec2 * v, const BkpVec2 * w);
BKP_EXPORTED void bkpVec2DivVec2(BkpVec2 * v, const BkpVec2 * w);
BKP_EXPORTED void bkpPrintVec2(const char * title, const BkpVec2 * v);

BKP_EXPORTED BkpVec3 bkpVec3(float x, float y, float z);
BKP_EXPORTED BkpVec3 bkpMVec3(__m128 m128_);
BKP_EXPORTED BkpVec3 bkpVec3Sum(const BkpVec3 * v1, const BkpVec3 * v2);
BKP_EXPORTED void bkpVec3Add(BkpVec3 * v1, const BkpVec3 * v2);
BKP_EXPORTED BkpVec3  bkpNegateVec3(const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpVec3Substract(const BkpVec3 * v1, const BkpVec3 * v2);
BKP_EXPORTED void bkpVec3Sub(BkpVec3 * v1, const BkpVec3 * v2);
BKP_EXPORTED BkpVec3 bkpVec3xScalar(const BkpVec3 * v, float scalar);
BKP_EXPORTED void bkpVec3MultScalar(BkpVec3 * v, float scalar);
BKP_EXPORTED BkpVec3 bkpVec3xVec3(const BkpVec3 * v, const BkpVec3 * v1);
BKP_EXPORTED void bkpVec3MultVec3(BkpVec3 * v, const BkpVec3 * v1);
BKP_EXPORTED BkpVec3 bkpVec3DivideScalar(const BkpVec3 * v, float scalar);
BKP_EXPORTED void bkpVec3DivScalar(BkpVec3 * v, float scalar);
BKP_EXPORTED BkpVec3 bkpNormalize3D(const BkpVec3 * v);
BKP_EXPORTED BkpVec3 bkpNormalized3D(const BkpVec3 v);
BKP_EXPORTED void bkpSetNormalize3D(BkpVec3 * v);
BKP_EXPORTED void bkpPrintVec3(const char * title, const BkpVec3 * v);

BKP_EXPORTED BkpVec4 bkpVec4FromV3(BkpVec3, float w);
BKP_EXPORTED BkpVec4 bkpVec4(float x, float y, float z, float w);
BKP_EXPORTED BkpVec4 bkpMVec4(__m128 m128_);
BKP_EXPORTED BkpVec4 bkpVec4Sum(const BkpVec4 * v1, const BkpVec4 * v2);
BKP_EXPORTED void bkpVec4Add(BkpVec4 * v1, const BkpVec4 * v2);
BKP_EXPORTED BkpVec4  bkpNegateVec4(const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpVec4Substract(const BkpVec4 * v1, const BkpVec4 * v2);
BKP_EXPORTED void bkpVec4Sub(BkpVec4 * v1, const BkpVec4 * v2);
BKP_EXPORTED BkpVec4 bkpVec4xScalar(const BkpVec4 * v, float scalar);
BKP_EXPORTED void bkpVec4MultScalar(BkpVec4 * v, float scalar);
BKP_EXPORTED BkpVec4 bkpVec4xVec4(const BkpVec4 * v, const BkpVec4 * v1);
BKP_EXPORTED void bkpVec4MultVec4(BkpVec4 * v, const BkpVec4 * v1);
BKP_EXPORTED BkpVec4 bkpVec4DivideScalar(const BkpVec4 * v, float scalar);
BKP_EXPORTED void bkpVec4DivScalar(BkpVec4 * v, float scalar);
BKP_EXPORTED BkpVec4 bkpVec4DivideVec4(const BkpVec4 * v, const BkpVec4 * v1);
BKP_EXPORTED void bkpVec4DivVec4(BkpVec4 * v, const BkpVec4 * v1);
BKP_EXPORTED BkpVec4 bkpNormalize4D(const BkpVec4 * v);
BKP_EXPORTED BkpVec4 bkpNormalized4D(const BkpVec4 v);
BKP_EXPORTED void bkpPrintVec4(const char * title, const BkpVec4 * v);

BKP_EXPORTED BkpVec3 bkpVec4GetXYZ(const BkpVec4 * v);
BKP_EXPORTED BkpVec3 bkpVec4GetXYW(const BkpVec4 * v);
BKP_EXPORTED BkpVec3 bkpVec4GetXZW(const BkpVec4 * v);
BKP_EXPORTED BkpVec3 bkpVec4GetYZW(const BkpVec4 * v);

BKP_EXPORTED BkpQuat bkpQuatIdentity();
BKP_EXPORTED BkpQuat bkpQuat(const BkpVec3 * v, float angle);
BKP_EXPORTED BkpQuat bkpQuat4f(float x, float y, float z, float w);
BKP_EXPORTED BkpQuat bkpQuatSum(const BkpQuat * q1, const BkpQuat * q2);
BKP_EXPORTED void bkpQuatAdd(BkpQuat * q1, const BkpQuat * q2);
BKP_EXPORTED BkpQuat  bkpNegateQuat(const BkpQuat * q);
BKP_EXPORTED BkpQuat bkpQuatSubstract(const BkpQuat * q1, const BkpQuat * q2);
BKP_EXPORTED void bkpQuatSub(BkpQuat * q1, const BkpQuat * q2);
BKP_EXPORTED BkpQuat bkpQuatxScalar(const BkpQuat * q, float scalar);
BKP_EXPORTED void bkpQuatMultScalar(BkpQuat * q, float scalar);
BKP_EXPORTED BkpQuat bkpQuatxQuat(const BkpQuat * q, const BkpQuat * q1);
BKP_EXPORTED void bkpQuatMultQuat(BkpQuat * q, const BkpQuat * q1);
BKP_EXPORTED BkpQuat bkpQuatDivideScalar(const BkpQuat * q, float scalar);
BKP_EXPORTED void bkpQuatDivScalar(BkpQuat * q, float scalar);
BKP_EXPORTED BkpQuat bkpNormalizeQuat(const BkpQuat * q);
BKP_EXPORTED BkpQuat bkpNormalizedQuat(const BkpQuat q);
BKP_EXPORTED BkpQuat bkpQuatRotation(const BkpVec3 * axis, float gradian);
BKP_EXPORTED BkpQuat bkpQuatRotationX(float gradian);
BKP_EXPORTED BkpQuat bkpQuatRotationY(float gradian);
BKP_EXPORTED BkpQuat bkpQuatRotationZ(float gradian);
BKP_EXPORTED void bkpPrintQuat(const char * title, const BkpQuat * q);

BKP_EXPORTED BkpVec3 bkpQuatAxis(const BkpQuat * q);
BKP_EXPORTED float bkpQuatAngle(const BkpQuat * q);
BKP_EXPORTED BkpQuat bkpConjugateQuat(const BkpQuat * q);
BKP_EXPORTED void bkpSetConjugateQuat(BkpQuat * q);
BKP_EXPORTED BkpQuat bkpInverseQuat(const BkpQuat * q);
BKP_EXPORTED void bkpInvQuat(BkpQuat * q);

BKP_EXPORTED BkpVec2i bkpVec2iXaxis();
BKP_EXPORTED BkpVec2i bkpVec2iYaxis();
BKP_EXPORTED BkpVec2 bkpVec2Xaxis();
BKP_EXPORTED BkpVec2 bkpVec2Yaxis();
BKP_EXPORTED BkpVec3 bkpVec3Xaxis();
BKP_EXPORTED BkpVec3 bkpVec3Yaxis();
BKP_EXPORTED BkpVec3 bkpVec3Zaxis();
BKP_EXPORTED BkpVec4 bkpVec4Xaxis();
BKP_EXPORTED BkpVec4 bkpVec4Yaxis();
BKP_EXPORTED BkpVec4 bkpVec4Zaxis();
BKP_EXPORTED BkpVec4 bkpVec4Waxis();

BKP_EXPORTED float bkpSlow_dot3D(const BkpVec3  * v1, const BkpVec3  * v2);
BKP_EXPORTED float bkpSlow_dot4D(const BkpVec4  * v1, const BkpVec4  * v2);
BKP_EXPORTED float bkpDot2D(const BkpVec2  * v1, const BkpVec2  * v2);
BKP_EXPORTED float bkpDot3D(const BkpVec3  * v1, const BkpVec3  * v2);
BKP_EXPORTED float bkpDot4D(const BkpVec4  * v1, const BkpVec4  * v2);
BKP_EXPORTED float bkpDotQuat(const BkpQuat * v1, const BkpQuat * v2);

BKP_EXPORTED BkpVec3 bkpSlow_cross3D(const BkpVec3  * v1, const BkpVec3  * v2);
BKP_EXPORTED BkpVec3 bkpCross3D(const BkpVec3  * v1, const BkpVec3  * v2);
BKP_EXPORTED BkpVec4 bkpCross4D(const BkpVec4  * v1, const BkpVec4  * v2);

BKP_EXPORTED float bkpSlow_magnitude3D(const BkpVec3  * v);
BKP_EXPORTED float bkpSlow_magnitude4D(const BkpVec4  * v);
BKP_EXPORTED float bkpMagnitude2D(const BkpVec2 * v);
BKP_EXPORTED float bkpMagnitude3D(const BkpVec3 * v);
BKP_EXPORTED float bkpMagnitude4D(const BkpVec4 * v);

BKP_EXPORTED float bkpMagnitudeSquared2D(const BkpVec2 * v);
BKP_EXPORTED float bkpMagnitudeSquared3D(const BkpVec3 * v);
BKP_EXPORTED float bkpMagnitudeSquared4D(const BkpVec4 * v);

BKP_EXPORTED BkpVec3 bkpSlow_normalize3D(const BkpVec3 * v);
BKP_EXPORTED BkpVec4 bkpSlow_normalize4D(const BkpVec4 * v);
BKP_EXPORTED void bkpSetNormalize2D(BkpVec2 * v);
BKP_EXPORTED BkpVec2 bkpNormalize2D(const BkpVec2 * v);
BKP_EXPORTED BkpVec2 bkpNormalized2D(const BkpVec2 v);

BKP_EXPORTED float bkpMagnitudeQuad(const BkpQuat * q);

BKP_EXPORTED BkpVec3 bkpProject3D(const BkpVec3 * v, const BkpVec3 * onto);
BKP_EXPORTED float bkpAngle3D(const BkpVec3 * v1, const BkpVec3 * v2);

BKP_EXPORTED int bkpVec2Compare(BkpVec2 * v1, BkpVec2 * v2);
BKP_EXPORTED int bkpVec3Compare(BkpVec3 * v1, BkpVec3 * v2);
BKP_EXPORTED int bkpVec4Compare(BkpVec4 * v1, BkpVec4 * v2);
BKP_EXPORTED int bkpVec2Compare2(BkpVec2 * v1, BkpVec2 * v2, float epsilon);
BKP_EXPORTED int bkpVec3Compare2(BkpVec3 * v1, BkpVec3 * v2, float epsilon);
BKP_EXPORTED int bkpVec4Compare2(BkpVec4 * v1, BkpVec4 * v2, float epsilon);

BKP_EXPORTED float bkpDistance2D(BkpVec2 * v1, BkpVec2 * v2);
BKP_EXPORTED float bkpDistance3D(BkpVec3 * v1, BkpVec3 * v2);
BKP_EXPORTED float bkpDistance4D(BkpVec4 * v1, BkpVec4 * v2);

BKP_EXPORTED extern BkpVec2 bkpXvector2D;
BKP_EXPORTED extern BkpVec2 bkpYvector2D;

BKP_EXPORTED extern BkpVec3 bkpXvector;
BKP_EXPORTED extern BkpVec3 bkpYvector;
BKP_EXPORTED extern BkpVec3 bkpZvector;

#ifdef __cplusplus
}
#endif

#endif /* BKP_VECT_H_ */

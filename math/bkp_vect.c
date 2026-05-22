#include "include/bkp_vect.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static const float gSLERP_THRESHOLD = 0.9995f;
union URet4
{
	__m128 m128;
	float t[4];
};

BkpVec2 bkpXvector2D = (BkpVec2){.x = 1.0f, .y = 0.0f};
BkpVec2 bkpYvector2D = (BkpVec2){.x = 0.0f, .y = 1.0f};

BkpVec3 bkpXvector = (BkpVec3){.x = 1.0f, .y = 0.0f, .z =0.0f};
BkpVec3 bkpYvector = (BkpVec3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
BkpVec3 bkpZvector = (BkpVec3){.x = 0.0f, .y = 0.0f, .z = 1.0f};

/*_____________________________________________________________________________________________
 * -------- 	                 Vector2 integer									---------
 _____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2i(int32_t x, int32_t y)
{
	return (BkpVec2i){.T[0] = x, .T[1] = y};
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2iSum(const BkpVec2i * v1, const BkpVec2i * v2)
{
	return (BkpVec2i){.T[0] = v1->x + v2->x, .T[1] = v1->y + v2->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2iAdd(BkpVec2i * v1, const BkpVec2i * v2)
{
	v1->x += v2->x;
	v1->y += v2->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2iSubstract(const BkpVec2i * v1, const BkpVec2i * v2)
{
	return (BkpVec2i){.T[0] = v1->x - v2->x, .T[1] = v1->y - v2->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2iSub(BkpVec2i * v1, const BkpVec2i * v2)
{
	v1->x -= v2->x;
	v1->y -= v2->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2ixScalar(const BkpVec2i * v, int32_t scalar)
{
	return (BkpVec2i){.T[0] = v->x * scalar, .T[1] = v->y * scalar};
}

/*_____________________________________________________________________________________________*/
void bkpVec2iMultScalar(BkpVec2i * v, int32_t scalar)
{
	v->x *= scalar;
	v->y *= scalar;
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2ixVec2i(const BkpVec2i * v, const BkpVec2i * w)
{
	return (BkpVec2i){.T[0] = v->x * w->x, .T[1] = v->y * w->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2iMultVec2i(BkpVec2i * v, const BkpVec2i * w)
{
	v->x *= w->x;
	v->y *= w->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2iDivideScalar(const BkpVec2i * v, int32_t scalar)
{
	return (BkpVec2i){.T[0] = v->x / scalar, .T[1] = v->y / scalar};
}

/*_____________________________________________________________________________________________*/
void bkpVec2iDivScalar(BkpVec2i * v, int32_t scalar)
{
	v->x /= scalar;
	v->y /= scalar;
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2iDivideVec2i(const BkpVec2i * v, const BkpVec2i * w)
{
	return (BkpVec2i){.T[0] = v->x / w->x, .T[1] = v->y / w->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2iDivVec2i(BkpVec2i * v, BkpVec2i * w)
{
	v->x /= w->x;
	v->y /= w->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2iXaxis()
{
	return (BkpVec2i){.T[0] = 1, .T[1] = 0};
}

/*_____________________________________________________________________________________________*/
BkpVec2i bkpVec2iYaxis()
{
	return (BkpVec2i){.T[0] = 0, .T[1] = 1};
}

/*_____________________________________________________________________________________________*/
void bkpPrintVec2i(const char * title, const BkpVec2i * v)
{
	fprintf(stdout, "%s (", title);
	fprintf(stdout, "%d, %d", v->x, v->y);
	fprintf(stdout, ")\n");
}

/*_____________________________________________________________________________________________
 * -------- 	                 Vector2 											---------
 _____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2(float x, float y)
{
	return (BkpVec2){.T[0] = x, .T[1] = y};
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2Sum(const BkpVec2 * v1, const BkpVec2 * v2)
{
	return (BkpVec2){.T[0] = v1->x + v2->x, .T[1] = v1->y + v2->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2Add(BkpVec2 * v1, const BkpVec2 * v2)
{
	v1->x += v2->x;
	v1->y += v2->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2Substract(const BkpVec2 * v1, const BkpVec2 * v2)
{
	return (BkpVec2){.T[0] = v1->x - v2->x, .T[1] = v1->y - v2->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2Sub(BkpVec2 * v1, const BkpVec2 * v2)
{
	v1->x -= v2->x;
	v1->y -= v2->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2Scalar(const BkpVec2 * v, float scalar)
{
	return (BkpVec2){.T[0] = v->x * scalar, .T[1] = v->y * scalar};
}

/*_____________________________________________________________________________________________*/
void bkpVec2MultScalar(BkpVec2 * v, float scalar)
{
	v->x *= scalar;
	v->y *= scalar;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2Vec2i(const BkpVec2 * v, const BkpVec2 * w)
{
	return (BkpVec2){.T[0] = v->x * w->x, .T[1] = v->y * w->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2MultVec2(BkpVec2 * v, const BkpVec2 * w)
{
	v->x *= w->x;
	v->y *= w->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2DivideScalar(const BkpVec2 * v, float scalar)
{
	return (BkpVec2){.T[0] = v->x / scalar, .T[1] = v->y / scalar};
}

/*_____________________________________________________________________________________________*/
void bkpVec2DivScalar(BkpVec2 * v, float scalar)
{
	v->x /= scalar;
	v->y /= scalar;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2DivideVec2(const BkpVec2 * v, const BkpVec2 * w)
{
	return (BkpVec2){.T[0] = v->x / w->x, .T[1] = v->y / w->y};
}

/*_____________________________________________________________________________________________*/
void bkpVec2DivVec2(BkpVec2 * v, const BkpVec2 * w)
{
	v->x /= w->x;
	v->y /= w->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2Xaxis()
{
	return (BkpVec2){.T[0] = 1.0f, .T[1] = 0.0f};
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2Yaxis()
{
	return (BkpVec2){.T[0] = 0.0f, .T[1] = 1.0f};
}

/*_____________________________________________________________________________________________*/
void bkpPrintVec2(const char * title, const BkpVec2 * v)
{
	fprintf(stdout, "%s (", title);
	fprintf(stdout, "%.4f, %.4f", v->x, v->y);
	fprintf(stdout, ")\n");
}

/*_____________________________________________________________________________________________
  -------- 		                 Vector3 											---------
  _____________________________________________________________________________________________*/

BkpVec3 bkpVec3(float x, float y, float z)
{
	return (BkpVec3) {.m128 = _mm_set_ps(0.0f, z, y, x)};
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMVec3(__m128 m128_)
{
	return (BkpVec3) {.m128 = m128_};
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3Sum(const BkpVec3 * v1, const BkpVec3 * v2)
{
	return (BkpVec3){.m128 = _mm_add_ps(v1->m128, v2->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpVec3Add(BkpVec3 * v1, const BkpVec3 * v2)
{
	v1->m128 = _mm_add_ps(v1->m128, v2->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec3  bkpNegateVec3(const BkpVec3 * v)
{
	return (BkpVec3) {.m128 = _mm_set_ps(0.0f, -v->z, -v->y, -v->x)};
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3Substract(const BkpVec3 * v1, const BkpVec3 * v2)
{
	return (BkpVec3){.m128 = _mm_sub_ps(v1->m128, v2->m128)};
}
/*_____________________________________________________________________________________________*/

void bkpVec3Sub(BkpVec3 * v1, const BkpVec3 * v2)
{
	v1->m128 = _mm_sub_ps(v1->m128, v2->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3xScalar(const BkpVec3 * v, float scalar)
{
	return (BkpVec3){.m128 = _mm_mul_ps(v->m128, _mm_set1_ps(scalar))};
}

/*_____________________________________________________________________________________________*/
void bkpVec3MultScalar(BkpVec3 * v, float scalar)
{
	v->m128 = _mm_mul_ps(v->m128, _mm_set1_ps(scalar));
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3xVec3(const BkpVec3 * v, const BkpVec3 * v1)
{
	return (BkpVec3){.m128 = _mm_mul_ps(v->m128, v1->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpVec3MultVec3(BkpVec3 * v, const BkpVec3 * v1)
{
	v->m128 = _mm_mul_ps(v->m128, v1->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3DivideScalar(const BkpVec3 * v, float scalar)
{
	return (BkpVec3){.m128 = _mm_div_ps(v->m128, _mm_set1_ps(scalar))};
}

/*_____________________________________________________________________________________________*/
void bkpVec3DivScalar(BkpVec3 * v, float scalar)
{
	v->m128 = _mm_div_ps(v->m128, _mm_set1_ps(scalar));
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3Xaxis()
{
	return bkpVec3(1.0f, 0.0f, 0.0f);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3Yaxis()
{
	return bkpVec3(0.0f, 1.0f, 0.0f);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3Zaxis()
{
	return bkpVec3(0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpNormalize3D(const BkpVec3 * v)
{
	return (BkpVec3){.m128 = _mm_div_ps(v->m128, _mm_sqrt_ps(_mm_dp_ps(v->m128, v->m128, 0xff)))};
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpNormalized3D(const BkpVec3 v)
{
	return (BkpVec3){.m128 = _mm_div_ps(v.m128, _mm_sqrt_ps(_mm_dp_ps(v.m128, v.m128, 0xff)))};
}

/*_____________________________________________________________________________________________*/
void bkpSetNormalize3D(BkpVec3 * v)
{
	v->m128 = _mm_div_ps(v->m128, _mm_sqrt_ps(_mm_dp_ps(v->m128, v->m128, 0xff)));
}

/*_____________________________________________________________________________________________*/
void bkpPrintVec3(const char * title, const BkpVec3 * v)
{
	fprintf(stdout, "%s (", title);
	fprintf(stdout, "%.4f, %.4f, %.4f", v->x, v->y, v->z);
	fprintf(stdout, ")\n");
}

/*_____________________________________________________________________________________________
  -------- 		                 Vector4 											---------
  _____________________________________________________________________________________________*/
BkpVec4 bkpVec4FromV3(BkpVec3 v, float w)
{
	return (BkpVec4) {.m128 = _mm_set_ps(w, v.z, v.y, v.x)};
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4(float x, float y, float z, float w)
{
	return (BkpVec4) {.m128 = _mm_set_ps(w, z, y, x)};
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMVec4(__m128 m128_)
{
	return (BkpVec4) {.m128 = m128_};
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4Sum(const BkpVec4 * v1, const BkpVec4 * v2)
{
	return (BkpVec4){.m128 = _mm_add_ps(v1->m128, v2->m128)};
}
/*_____________________________________________________________________________________________*/

void bkpVec4Add(BkpVec4 * v1, const BkpVec4 * v2)
{
	v1->m128 = _mm_add_ps(v1->m128, v2->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec4  bkpNegateVec4(const BkpVec4 * v)
{
	return (BkpVec4){.m128 = _mm_sub_ps(_mm_setzero_ps(), v->m128)};
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4Substract(const BkpVec4 * v1, const BkpVec4 * v2)
{
	return (BkpVec4){.m128 = _mm_sub_ps(v1->m128, v2->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpVec4Sub(BkpVec4 * v1, const BkpVec4 * v2)
{
	v1->m128 = _mm_sub_ps(v1->m128, v2->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4xScalar(const BkpVec4 * v, float scalar)
{
	return (BkpVec4){.m128 = _mm_mul_ps(v->m128, _mm_set1_ps(scalar))};
}

/*_____________________________________________________________________________________________*/
void bkpVec4MultScalar(BkpVec4 * v, float scalar)
{
	v->m128 = _mm_mul_ps(v->m128, _mm_set1_ps(scalar));
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4xVec4(const BkpVec4 * v, const BkpVec4 * v1)
{
	return (BkpVec4){.m128 = _mm_mul_ps(v->m128, v1->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpVec4MultVec4(BkpVec4 * v, const BkpVec4 * v1)
{
	v->m128 = _mm_mul_ps(v->m128, v1->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4DivideScalar(const BkpVec4 * v, float scalar)
{
	return (BkpVec4){.m128 = _mm_div_ps(v->m128, _mm_set1_ps(scalar))};
}

/*_____________________________________________________________________________________________*/
void bkpVec4DivScalar(BkpVec4 * v, float scalar)
{
	v->m128 = _mm_div_ps(v->m128, _mm_set1_ps(scalar));
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4DivideVec4(const BkpVec4 * v, const BkpVec4 * v1)
{
	return (BkpVec4){.m128 = _mm_div_ps(v->m128, v1->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpVec4DivVec4(BkpVec4 * v, const BkpVec4 * v1)
{
	v->m128 = _mm_div_ps(v->m128, v1->m128);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4Xaxis()
{
	return bkpVec4(1.0f, 0.0f, 0.0f, 0.0f);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4Yaxis()
{
	return bkpVec4(0.0f, 1.0f, 0.0f, 0.0f);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4Zaxis()
{
	return bkpVec4(0.0f, 0.0f, 1.0f, 0.0f);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4Waxis()
{
	return bkpVec4(0.0f, 0.0f, 0.0f, 1.0f);
}

BkpVec4 bkpNormalize4D(const BkpVec4 * v)
{
	return (BkpVec4) {.m128 = _mm_div_ps(v->m128, _mm_sqrt_ps(_mm_dp_ps(v->m128, v->m128, 0xff)))};
}

BkpVec4 bkpNormalized4D(const BkpVec4 v)
{
	return (BkpVec4) {.m128 = _mm_div_ps(v.m128, _mm_sqrt_ps(_mm_dp_ps(v.m128, v.m128, 0xff)))};
}

/*_____________________________________________________________________________________________*/
void bkpPrintVec4(const char * title, const BkpVec4 * v)
{
	fprintf(stdout, "%s (", title);
	fprintf(stdout, "%.4f, %.4f, %.4f, %.4f", v->x, v->y, v->z, v->w);
	fprintf(stdout, ")\n");
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec4GetXYZ(const BkpVec4 * v)
{
	return bkpVec3(v->x, v->y, v->z);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec4GetXYW(const BkpVec4 * v)
{
	return bkpVec3(v->x, v->y, v->w);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec4GetXZW(const BkpVec4 * v)
{
	return bkpVec3(v->x, v->z, v->w);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec4GetYZW(const BkpVec4 * v)
{
	return bkpVec3(v->y, v->z, v->w);
}

/*_____________________________________________________________________________________________
  -------- 		                 Quaternion    										---------
  _____________________________________________________________________________________________*/
BkpQuat bkpQuatIdentity()
{
	return (BkpQuat){.m128 = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f)};
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuat(const BkpVec3 * v, float angle)
{
	return (BkpQuat){.m128 = _mm_set_ps(angle, v->z, v->y, v->x)};
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuat4f(float x, float y, float z, float w)
{
	return (BkpQuat) {.m128 = _mm_set_ps(w, z, y, x)};
	//m128 = _mm_set_ps(x_, y_, z_, w_);
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatSum(const BkpQuat * q1, const BkpQuat * q2)
{
	return (BkpQuat){.m128 = _mm_add_ps(q1->m128, q2->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpQuatAdd(BkpQuat * q1, const BkpQuat * q2)
{
	q1->m128 = _mm_add_ps(q1->m128, q2->m128);
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatSubstract(const BkpQuat * q1, const BkpQuat * q2)
{
	return (BkpQuat){.m128 = _mm_sub_ps(q1->m128, q2->m128)};
}

/*_____________________________________________________________________________________________*/
void bkpQuatSub(BkpQuat * q1, const BkpQuat * q2)
{
	q1->m128 = _mm_sub_ps(q1->m128, q2->m128);
}

/*_____________________________________________________________________________________________*/
BkpQuat  bkpNegateQuat(const BkpQuat * q)
{
	return (BkpQuat){.m128 = _mm_sub_ps(_mm_setzero_ps(), q->m128)};
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatxScalar(const BkpQuat * q, float scalar)
{
	return (BkpQuat){.m128 = _mm_mul_ps(q->m128, _mm_set1_ps(scalar))};
}

/*_____________________________________________________________________________________________*/
void bkpQuatMultScalar(BkpQuat * q, float scalar)
{
	q->m128 = _mm_mul_ps(q->m128, _mm_set1_ps(scalar));
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatDivideScalar(const BkpQuat * q, float scalar)
{
	return (BkpQuat){.m128 = _mm_div_ps(q->m128, _mm_set1_ps(scalar))};
}

/*_____________________________________________________________________________________________*/
void bkpQuatDivScalar(BkpQuat * q, float scalar)
{
	q->m128 = _mm_div_ps(q->m128, _mm_set1_ps(scalar));
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatxQuat(const BkpQuat * q, const BkpQuat * q1)
{
	return bkpQuat4f(
		(q->w * q1->x) + (q->x * q1->w) + (q->y * q1->z) - (q->z * q1->y),
			(q->w * q1->y) + (q->y * q1->w) + (q->z * q1->x) - (q->x * q1->z),
			(q->w * q1->z) + (q->z * q1->w) + (q->x * q1->y) - (q->y * q1->x),
			(q->w * q1->w) - (q->x * q1->x) - (q->y * q1->y) - (q->z * q1->z)
	);
}

/*_____________________________________________________________________________________________*/
void bkpQuatMultQuat(BkpQuat * q, const BkpQuat * q1)
{
	float x_ = (q->w * q1->x) + (q->x * q1->w) + (q->y * q1->z) - (q->z * q1->y);
	float y_ = (q->w * q1->y) + (q->y * q1->w) + (q->z * q1->x) - (q->x * q1->z);
	float z_ = (q->w * q1->z) + (q->z * q1->w) + (q->x * q1->y) - (q->y * q1->x);
	float w_ = (q->w * q1->w) - (q->x * q1->x) - (q->y * q1->y) - (q->z * q1->z);
	q->x = x_;
	q->y = y_;
	q->z = z_;
	q->w = w_;
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpQuatAxis(const BkpQuat * q)
{
	return bkpVec3(q->x, q->y, q->z);
}

/*_____________________________________________________________________________________________*/
float bkpQuatAngle(const BkpQuat * q)
{
	return 2.0f * acosf(q->w);
}

/*_____________________________________________________________________________________________*/

BkpQuat bkpNormalizeQuat(const BkpQuat * q)
{
	return (BkpQuat) {.m128 = _mm_div_ps(q->m128, _mm_sqrt_ps(_mm_dp_ps(q->m128, q->m128, 0xff)))};
}
/*_____________________________________________________________________________________________*/

BkpQuat bkpNormalizedQuat(const BkpQuat q)
{
	return (BkpQuat) {.m128 = _mm_div_ps(q.m128, _mm_sqrt_ps(_mm_dp_ps(q.m128, q.m128, 0xff)))};
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpConjugateQuat(const BkpQuat * q)
{
	return bkpQuat4f(-q->x, -q->y, -q->z, q->w);
}

/*_____________________________________________________________________________________________*/
void bkpSetConjugateQuat(BkpQuat * q)
{
	q->x = -q->x;
	q->y = -q->y;
	q->z = -q->z;
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpInverseQuat(const BkpQuat * q)
{
	BkpQuat qConj = bkpConjugateQuat(q);
	return bkpQuatxScalar(&qConj, 1.0f / bkpDotQuat(q, q));
}

/*_____________________________________________________________________________________________*/
void bkpInvQuat(BkpQuat * q)
{
	float d = 1.0f / bkpDotQuat(q, q);

	bkpSetConjugateQuat(q);
	*q = bkpQuatxScalar(q, d);
}

/*_____________________________________________________________________________________________*/


/*_____________________________________________________________________________________________*/


/*_____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatRotation(const BkpVec3 * axis, float gradian)
{
	float angle = gradian * 0.5f;
	float s = sinf(angle);
	float c = cosf(angle);

	BkpVec3 v = bkpVec3xScalar(axis, s);
	return bkpQuat(&v, c);
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatRotationX(float gradian)
{
	float angle = gradian * 0.5f;
	float s = sinf(angle);
	float c = cosf(angle);

	return bkpQuat4f(s, 0.0f, 0.0f, c);
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatRotationY(float gradian)
{
	float angle = gradian * 0.5f;
	float s = sinf(angle);
	float c = cosf(angle);

	return bkpQuat4f(0.0f, s, 0.0f, c);
}

/*_____________________________________________________________________________________________*/
BkpQuat bkpQuatRotationZ(float gradian)
{
	float angle = gradian * 0.5f;
	float s = sinf(angle);
	float c = cosf(angle);

	return bkpQuat4f(0.0f, 0.0f, s, c);
}



/*_____________________________________________________________________________________________*/
void bkpPrintQuat(const char * title, const BkpQuat * q)
{
	fprintf(stdout, "%s (", title);
	fprintf(stdout, "%.4f, %.4f, %.4f, %.4f", q->x, q->y, q->z, q->w);
	fprintf(stdout, ")\n");
}


/*_____________________________________________________________________________________________
  -------- 		                 Operator ---------
  _____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________
  -------- 		                 Operations ---------
  _____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/
float slow_dot3D(const BkpVec3 * v1, const BkpVec3 * v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/*_____________________________________________________________________________________________*/
float slow_dot4D(const BkpVec4 * v1, const BkpVec4 * v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z + v1->w * v2->w;
}

/*_____________________________________________________________________________________________*/
float bkpDot2D(const BkpVec2 * v1, const BkpVec2 * v2)
{
	return v1->x * v2->x + v1->y * v2->y;
}

/*_____________________________________________________________________________________________*/
float bkpDot3D(const BkpVec3 * v1, const BkpVec3 * v2)
{

	union URet4 r;
	r.m128 = _mm_dp_ps(v1->m128, v2->m128, 0xff);

	return r.t[0];
}

/*_____________________________________________________________________________________________*/
float bkpDot4D(const BkpVec4 * v1, const BkpVec4 * v2)
{

	/* all algo are ordered by time for 99999999 iterations ;
	 *
	 * time: 1.2s
	 *
	 __m128 r  = _mm_mul_ps(v1.m128, v2.m128);
	 __m128 t0 = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0, 0, 0, 0));
	 __m128 t1 = _mm_shuffle_ps(r, r, _MM_SHUFFLE(1, 1, 1, 1));
	 __m128 t2 = _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 2, 2, 2));
	 __m128 t3 = _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 3, 3, 3));

	 URet4 u;
	 u.m128 = _mm_add_ps(t0, _mm_add_ps(t1, _mm_add_ps(t2, t3)));
	 return u.t[0];
	 */

	/*
	 * time : 1.1s
	 __m128 r = _mm_mul_ps(v1.m128, v2.m128);
	 __m128 shuf = _mm_shuffle_ps(r, r, _MM_SHUFFLE(2, 3, 0, 1));
	 __m128 sum  = _mm_add_ps(r, shuf);
	 shuf	= _mm_movehl_ps(shuf, sum);
	 sum 	= _mm_add_ss(sum, shuf);
	 return _mm_cvtss_f32(sum);
	 */

	/*slow_dot() time: 0.661s*/

	/*
	 * time : 0.615s
	 URet4 r;
	 r.m128 = _mm_mul_ps(v1.m128, v2.m128);
	 return r.t[0] + r.t[1] + r.t[2] + r.t[3];
	 */

	/* time :0.51s  however need MSSE4 CPU*/
	union URet4 r;
	r.m128 = _mm_dp_ps(v1->m128, v2->m128, 0xff);
	return r.t[0];
}

/*_____________________________________________________________________________________________*/
float bkpDotQuat(const BkpQuat * v1, const BkpQuat * v2)
{
	union URet4 r;
	r.m128 = _mm_dp_ps(v1->m128, v2->m128, 0xff);
	return r.t[0];
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpCross3D(const BkpVec3  * v1, const BkpVec3  * v2)
{
	return bkpVec3(
			v1->y * v2->z - v1->z * v2->y,
			v1->z * v2->x - v1->x * v2->z,
			v1->x * v2->y - v1->y * v2->x
			);
}

/*_____________________________________________________________________________________________*/
BkpVec3 slow_cross3D(const BkpVec3 * v1, const BkpVec3 * v2)
{
	/* same speed than slow */
	__m128 v1_yzx = _mm_shuffle_ps(v1->m128, v1->m128, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 v2_yzx = _mm_shuffle_ps(v2->m128, v2->m128, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 r	  = _mm_sub_ps(_mm_mul_ps(v1->m128, v2_yzx), _mm_mul_ps(v1_yzx, v2->m128));

	return (BkpVec3){.m128 = _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 0, 2, 1))};
}

/*_____________________________________________________________________________________________*/
BkpVec4 cross4D(const BkpVec4 * v1, const BkpVec4 * v2)
{
	__m128 v1_yzx = _mm_shuffle_ps(v1->m128, v1->m128, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 v1_zxy = _mm_shuffle_ps(v1->m128, v1->m128, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 v2_zxy = _mm_shuffle_ps(v2->m128, v2->m128, _MM_SHUFFLE(3, 1, 0, 1));
	__m128 v2_yzx = _mm_shuffle_ps(v2->m128, v2->m128, _MM_SHUFFLE(3, 0, 2, 1));
	return (BkpVec4){.m128 = _mm_sub_ps(_mm_mul_ps(v1_yzx, v2_zxy), _mm_mul_ps(v1_zxy, v2_yzx))};
}

/*_____________________________________________________________________________________________*/
float bkpSlow_magnitude3D(const BkpVec3 * v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

/*_____________________________________________________________________________________________*/
float bkpSlow_magnitude4D(const BkpVec4 * v)
{
	return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z + v->w * v->w);
}

/*_____________________________________________________________________________________________*/
float bkpMagnitude2D(const BkpVec2 * v)
{
	return sqrtf(v->x * v->x + v->y * v->y);
}

/*_____________________________________________________________________________________________*/
float bkpMagnitude3D(const BkpVec3 * v)
{
	union URet4 u;
	u.m128 = _mm_sqrt_ps(_mm_dp_ps(v->m128, v->m128, 0xff));
	return u.t[0];
}

/*_____________________________________________________________________________________________*/
float bkpMagnitude4D(const BkpVec4 * v)
{
	union URet4 u;
	u.m128 = _mm_sqrt_ps(_mm_dp_ps(v->m128, v->m128, 0xff));
	return u.t[0];
}


/*_____________________________________________________________________________________________*/
float bkpMagnitudeSquared2D(const BkpVec2 * v)
{
	return (v->x * v->x + v->y * v->y);
}

/*_____________________________________________________________________________________________*/
float bkpMagnitudeSquared3D(const BkpVec3 * v)
{
	union URet4 u;
	u.m128 = _mm_dp_ps(v->m128, v->m128, 0xff);
	return u.t[0];
}
/*_____________________________________________________________________________________________*/
float bkpMagnitudeSquared4D(const BkpVec4 * v)
{
	union URet4 u;
	u.m128 = _mm_dp_ps(v->m128, v->m128, 0xff);
	return u.t[0];
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpSlow_normalize3D(const BkpVec3 * v)
{
	float mag = bkpSlow_magnitude3D(v);
	return bkpVec3DivideScalar(v,  mag);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpSlow_normalize4D(const BkpVec4 * v)
{
	float mag = bkpSlow_magnitude4D(v);
	return bkpVec4DivideScalar(v,  mag);
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpNormalize2D(const BkpVec2 * v)
{
	float mag = bkpMagnitude2D(v);
	return bkpVec2DivideScalar(v,  mag);
}

/*_____________________________________________________________________________________________*/
void bkpSetNormalize2D(BkpVec2 * v)
{
	float mag = bkpMagnitude2D(v);
	*v = bkpVec2DivideScalar(v,  mag);
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpNormalized2D(const BkpVec2 v)
{
	float mag = bkpMagnitude2D(&v);
	return bkpVec2DivideScalar(&v,  mag);
}

/*_____________________________________________________________________________________________*/

float bkpMagnitudeQuad(const BkpQuat * q)
{
	union URet4 u;
	u.m128 = _mm_sqrt_ps(_mm_dp_ps(q->m128, q->m128, 0xff));
	return u.t[0];
}

/*_____________________________________________________________________________________________*/
   const BkpQuat bkpSlerpQuat(const BkpQuat * unitQuat0, const BkpQuat * unitQuat1, float t)
{
	BkpQuat start;
	float recipSinAngle, scale0, scale1, cosAngle, angle;
	cosAngle = bkpDotQuat(unitQuat0, unitQuat1);
	if (cosAngle < 0.0f)
	{
		cosAngle = -cosAngle;
		start = bkpNegateQuat(unitQuat0);
	}
	else
	{
		start = *unitQuat0;
	}
	if (cosAngle < gSLERP_THRESHOLD)
	{
		angle = acosf(cosAngle);
		recipSinAngle = (1.0f / sinf(angle));
		scale0 = (sinf(((1.0f - t) * angle)) * recipSinAngle);
		scale1 = (sinf((t * angle)) * recipSinAngle);
	}
	else
	{
		scale0 = (1.0f - t);
		scale1 = t;
	}
	BkpQuat q1 = bkpQuatxScalar(&start,  scale0);
	BkpQuat q2 = bkpQuatxScalar(unitQuat1, scale1);
	return bkpQuatSum(&q1, &q2);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpProject3D(const BkpVec3 * v, const BkpVec3 * onto)
{
	float k = bkpDot3D(v, onto) / bkpDot3D(onto, onto);
	BkpVec3 w = bkpNormalize3D(onto);
	return bkpVec3xScalar(&w, k);
}

/*_____________________________________________________________________________________________*/
float bkpAngle3D(const BkpVec3 * v1, const BkpVec3 * v2)
{
	return acosf(bkpDot3D(v1, v2) / (bkpMagnitude3D(v1) * bkpMagnitude3D(v2)));
}

/*_____________________________________________________________________________________________*/
int bkpVec2Compare(BkpVec2 * v1, BkpVec2 * v2)
{
	if(bkpFloatEq(bkpDistance2D(v1, v2), 0.0f) == 0) return 0;

	return 1;
}

/*_____________________________________________________________________________________________*/
int bkpVec3Compare(BkpVec3 * v1, BkpVec3 * v2)
{
	float mag = bkpDistance3D(v1, v2);
	if(bkpFloatEq(mag, 0.0f) == 0) return 0;

	return 1;
}

/*_____________________________________________________________________________________________*/
int bkpVec4Compare(BkpVec4 * v1, BkpVec4 * v2)
{
	if(bkpFloatEq(bkpDistance4D(v1, v2), 0.0f) == 0) return 0;

	return 1;

}

/*_____________________________________________________________________________________________*/
int bkpVec2Compare2(BkpVec2 * v1, BkpVec2 * v2, float epsilon)
{
	if(bkpFloatEq2(bkpDistance2D(v1, v2), 0.0f, epsilon) == 0) return 0;

	return 1;
}

/*_____________________________________________________________________________________________*/
int bkpVec3Compare2(BkpVec3 * v1, BkpVec3 * v2, float epsilon)
{
	if(bkpFloatEq2(bkpDistance3D(v1, v2), 0.0f, epsilon) == 0) return 0;

	return 1;
}

/*_____________________________________________________________________________________________*/
int bkpVec4Compare2(BkpVec4 * v1, BkpVec4 * v2, float epsilon)
{
	if(bkpFloatEq2(bkpDistance4D(v1, v2), 0.0f, epsilon) == 0) return 0;

	return 1;

}

/*_____________________________________________________________________________________________*/
float bkpDistance2D(BkpVec2 * v1, BkpVec2 * v2)
{
	BkpVec2 v = bkpVec2Substract(v1, v2);
	return bkpMagnitude2D(&v);
}

/*_____________________________________________________________________________________________*/
float bkpDistance3D(BkpVec3 * v1, BkpVec3 * v2)
{
	BkpVec3 v = bkpVec3Substract(v1, v2);
	return bkpMagnitude3D(&v);
}

/*_____________________________________________________________________________________________*/
float bkpDistance4D(BkpVec4 * v1, BkpVec4 * v2)
{
	BkpVec4 v = bkpVec4Substract(v1, v2);
	return bkpMagnitude4D(&v);
}

#ifdef __cplusplus
}
#endif

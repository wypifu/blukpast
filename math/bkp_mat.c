#include "include/bkp_mat.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*_____________________________________________________________________________________________
 * -------- 	                 BkpMatrice2											---------
 _____________________________________________________________________________________________*/
/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2(float a00, float a01, float a10, float a11)
{
	return (BkpMat2) {.mData = bkpVec4(a00, a01, a10, a11)};
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpMat2GetColumn0(const BkpMat2 * mat)
{
	return bkpVec2(mat->m00, mat->m10);
}

/*_____________________________________________________________________________________________*/
void bkpMat2SetColumn0(BkpMat2 * mat, BkpVec2 * v)
{
	mat->m00 = v->x;
	mat->m10 = v->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpMat2GetColumn1(const BkpMat2 * mat)
{
	return bkpVec2(mat->m01, mat->m11);
}

/*_____________________________________________________________________________________________*/
void bkpMat2SetColumn1(BkpMat2 * mat, BkpVec2 * v)
{
	mat->m01 = v->x;
	mat->m11 = v->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpMat2GetLine0(const BkpMat2 * mat)
{
	return bkpVec2(mat->m00, mat->m01);
}

/*_____________________________________________________________________________________________*/
void bkpMat2SetLine0(BkpMat2 * mat, BkpVec2 * v)
{
	mat->m00 = v->x;
	mat->m01 = v->y;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpMat2GetLine1(const BkpMat2 * mat)
{
	return bkpVec2(mat->m10, mat->m11);
}

/*_____________________________________________________________________________________________*/
void bkpMat2SetLine1(BkpMat2 * mat, BkpVec2 * v)
{
	mat->m10 = v->x;
	mat->m11 = v->y;
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2Sum(const BkpMat2 * m1, const BkpMat2 * m2)
{
	return (BkpMat2){.mData = bkpVec4Sum(&m1->mData, &m2->mData)};
}

/*_____________________________________________________________________________________________*/
void  bkpMat2Add(BkpMat2 * m1, const BkpMat2 * m2)
{
	bkpVec4Add(&m1->mData, &m2->mData);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpNegateMat2(BkpMat2 * m1)
{
	return (BkpMat2){.mData = bkpNegateVec4(&m1->mData)};
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2Substract(const BkpMat2 * m1, const BkpMat2 * m2)
{
	return (BkpMat2){.mData = bkpVec4Substract(&m1->mData, &m2->mData)};
}

/*_____________________________________________________________________________________________*/
void bkpMat2Sub(BkpMat2 * m1, const BkpMat2 * m2)
{
	bkpVec4Sub(&m1->mData, &m2->mData);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2xScalar(const BkpMat2 * m1, const float scalar)
{
	return (BkpMat2){.mData = bkpVec4xScalar(&m1->mData, scalar)};
}

/*_____________________________________________________________________________________________*/
void bkpMat2MultScalar(BkpMat2 * m1, const float scalar)
{
	bkpVec4MultScalar(&m1->mData, scalar);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2xVec2(const BkpMat2 * m1, const BkpVec2 * v)
{
	return bkpMat2(m1->m00 * v->x,
			m1->m01 * v->y,
			m1->m10 * v->x,
			m1->m11 * v->y);
}
/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2xMat2(const BkpMat2 * m1, const BkpMat2 * m2)
{
	return bkpMat2(m1->m00 * m2->m00,
			m1->m01 * m2->m01,
			m1->m10 * m2->m10,
			m1->m11 * m2->m11);
}

/*_____________________________________________________________________________________________*/
void bkpMat2MultMat2(BkpMat2 * m1, const BkpMat2 * m2)
{
	m1->m00 *= m2->m00;
	m1->m01 *= m2->m01;
	m1->m10 *= m2->m10;
	m1->m11 *= m2->m11;
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpMat2DotVec2(const BkpMat2 * m, const BkpVec2 * v)
{
	BkpVec2 c0 = bkpMat2GetColumn0(m);
	BkpVec2 c1 = bkpMat2GetColumn1(m);
	return bkpVec2(bkpDot2D(v, &c0), bkpDot2D(v, &c1));
}

/*_____________________________________________________________________________________________*/
BkpVec2 bkpVec2DotMat2(const BkpVec2 * v, const BkpMat2 * m)
{
	BkpVec2 l0 = bkpMat2GetLine0(m);
	BkpVec2 l1 = bkpMat2GetLine1(m);
	return bkpVec2(bkpDot2D(v, &l0), bkpDot2D(v, &l1));
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2DotMat2(const BkpMat2 * m1, const BkpMat2 * m2)
{
	const BkpVec2 lm0 = bkpMat2GetLine0(m2);
	const BkpVec2 lm1 = bkpMat2GetLine1(m2);
	const BkpVec2 cm0 = bkpMat2GetColumn0(m1);
	const BkpVec2 cm1 = bkpMat2GetColumn1(m1);

	return bkpMat2(bkpDot2D(&lm0, &cm0), bkpDot2D(&lm0, &cm1),
			bkpDot2D(&lm1, &cm0), bkpDot2D(&lm1, &cm1));
}

/*_____________________________________________________________________________________________*/
void bkpMat2Dot(BkpMat2 * m1, const BkpMat2 * m2)
{
	const BkpVec2 lm0 = bkpMat2GetLine0(m2);
	const BkpVec2 lm1 = bkpMat2GetLine1(m2);
	const BkpVec2 cm0 = bkpMat2GetColumn0(m1);
	const BkpVec2 cm1 = bkpMat2GetColumn1(m1);
	float _0_ = bkpDot2D(&lm0, &cm0);
	float _1_ = bkpDot2D(&lm0, &cm1);
	float _2_ = bkpDot2D(&lm1, &cm0);
	float _3_ = bkpDot2D(&lm1, &cm1);
	m1->m00 = _0_;
	m1->m01 = _1_;
	m1->m10 = _2_;
	m1->m11 = _3_;
}

/*_____________________________________________________________________________________________*/
void bkpPrintMat2(const char * title, BkpMat2 * m)
{
	fprintf(stdout, "%s\n  [\n", title);
	fprintf(stdout, "\t%.4f, %.4f\n\t%.4f, %.4f", m->m00, m->m01, m->m10, m->m11);
	fprintf(stdout, "\n  ]\n");
}

/*_____________________________________________________________________________________________*/
void bkpSetIdentityMat2(BkpMat2 * m)
{
	m->m00 = m->m11 = 1.0f;
	m->m01 = m->m10 = 0.0f;
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpIdentityMat2()
{
	return bkpMat2(1.0f, 0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpNullMat2()
{
	return bkpMat2(0.0f, 0.0f, 0.0f, 0.0f);
}


/*_____________________________________________________________________________________________
 * -------- 	                 BkpMatrice3											---------
 _____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22)
{
	return (BkpMat3){.mLine0 = bkpVec3(a00, a01, a02),
	.mLine1 = bkpVec3(a10, a11, a12),
	.mLine2 = bkpVec3(a20, a21, a22)};
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3FromVec3(BkpVec3 v1, BkpVec3 v2, BkpVec3 v3)
{
	return (BkpMat3){.mLine0 = v1,
	.mLine1 = v2,
	.mLine2 = v3};
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3GetColumn0(const BkpMat3 * mat)
{
	return bkpVec3(mat->mLine0.x, mat->mLine1.x, mat->mLine2.x);
}

/*_____________________________________________________________________________________________*/
void bkpMat3SetColumn0(BkpMat3 * m, const BkpVec3 * v)
{
	m->mLine0.x = v->x;
	m->mLine1.x = v->y;
	m->mLine2.x = v->z;
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3GetColumn1(const BkpMat3 * m)
{
	return bkpVec3(m->mLine0.y, m->mLine1.y, m->mLine2.y);
}

/*_____________________________________________________________________________________________*/
void bkpMat3SetColumn1(BkpMat3 * m, const BkpVec3 * v)
{
	m->mLine0.y = v->x;
	m->mLine1.y = v->y;
	m->mLine2.y = v->z;
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3GetColumn2(const BkpMat3 * m)
{
	return bkpVec3(m->mLine0.z, m->mLine1.z, m->mLine2.z);
}

/*_____________________________________________________________________________________________*/
void bkpMat3SetColumn2(BkpMat3 * m, const BkpVec3 * v)
{
	m->mLine0.z = v->x;
	m->mLine1.z = v->y;
	m->mLine2.z = v->z;
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3GetLine0(const BkpMat3 * mat)
{
	return mat->mLine0;
}

/*_____________________________________________________________________________________________*/
void bkpMat3SetLine0(BkpMat3 * m, const BkpVec3 * v)
{
	m->mLine0 = *v;
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3GetLine1(const BkpMat3 * mat)
{
	return mat->mLine1;
}

/*_____________________________________________________________________________________________*/
void bkpMat3SetLine1(BkpMat3 * m, const BkpVec3 * v)
{
	m->mLine1 = *v;
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3GetLine2(const BkpMat3 * mat)
{
	return mat->mLine2;
}

/*_____________________________________________________________________________________________*/
void bkpMat3SetLine2(BkpMat3 * m, const BkpVec3 * v)
{
	m->mLine2 = *v;
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat3GetUpper(const BkpMat3 * m)
{
	return bkpMat2(m->mLine0.x, m->mLine0.y, m->mLine1.x, m->mLine1.y);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3Sum(const BkpMat3 * m1, const BkpMat3 * m2)
{
	return (BkpMat3){.mLine0 = bkpVec3Sum(&m1->mLine0, &m2->mLine0),
		.mLine1 = bkpVec3Sum(&m1->mLine1, &m2->mLine1),
		.mLine2 = bkpVec3Sum(&m1->mLine2, &m2->mLine2)};
}

/*_____________________________________________________________________________________________*/
void bkpMat3Add(BkpMat3 * m1, const BkpMat3 * m2)
{
	bkpVec3Add(&m1->mLine0, &m2->mLine0);
	bkpVec3Add(&m1->mLine1, &m2->mLine1);
	bkpVec3Add(&m1->mLine2, &m2->mLine2);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpNegateMat3(BkpMat3 * m1)
{
	return (BkpMat3){.mLine0 = bkpNegateVec3(&m1->mLine0),
		.mLine1 = bkpNegateVec3(&m1->mLine1),
		.mLine2 = bkpNegateVec3(&m1->mLine2)};
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3Substract(const BkpMat3 * m1, const BkpMat3 * m2)
{
	return (BkpMat3){.mLine0 = bkpVec3Substract(&m1->mLine0, &m2->mLine0),
		.mLine1 = bkpVec3Substract(&m1->mLine1, &m2->mLine1),
		.mLine2 = bkpVec3Substract(&m1->mLine2, &m2->mLine2)};
}

/*_____________________________________________________________________________________________*/
void bkpMat3Sub(BkpMat3 * m1, const BkpMat3 * m2)
{
	bkpVec3Sub(&m1->mLine0, &m2->mLine0);
	bkpVec3Sub(&m1->mLine1, &m2->mLine1);
	bkpVec3Sub(&m1->mLine2, &m2->mLine2);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3xScalar(const BkpMat3 * m1, const float scalar)
{
	return (BkpMat3){.mLine0 = bkpVec3xScalar(&m1->mLine0, scalar),
		.mLine1 = bkpVec3xScalar(&m1->mLine1, scalar),
		.mLine2 = bkpVec3xScalar(&m1->mLine2, scalar)};
}

/*_____________________________________________________________________________________________*/
void bkpMat3MultScalar(BkpMat3 * m1, const float scalar)
{
	m1->mLine0 = bkpVec3xScalar(&m1->mLine0, scalar);
	m1->mLine1 = bkpVec3xScalar(&m1->mLine1, scalar);
	m1->mLine2 = bkpVec3xScalar(&m1->mLine2, scalar);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3xVec3(const BkpMat3 * m1, const BkpVec3 * v)
{
	return bkpMat3(m1->mLine0.x * v->x, m1->mLine0.y * v->y, m1->mLine0.z * v->z,
			m1->mLine1.x * v->x, m1->mLine1.y * v->y, m1->mLine1.z * v->z,
			m1->mLine2.x * v->x, m1->mLine2.y * v->y, m1->mLine2.z * v->z);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3xMat3(const BkpMat3 * m1, const BkpMat3 * m2)
{
	return bkpMat3(m1->mLine0.x * m2->mLine0.x, m1->mLine0.y * m2->mLine0.y, m1->mLine0.z * m2->mLine0.z,
			m1->mLine1.x * m2->mLine1.x, m1->mLine1.y * m2->mLine1.y, m1->mLine1.z * m2->mLine1.z,
			m1->mLine2.x * m2->mLine2.x, m1->mLine2.y * m2->mLine2.y, m1->mLine2.z * m2->mLine2.z);
}

/*_____________________________________________________________________________________________*/
void bkpMat3MultMat3(BkpMat3 * m1, const BkpMat3 * m2)
{
	bkpVec3MultVec3(&m1->mLine0, &m2->mLine0);
	bkpVec3MultVec3(&m1->mLine1, &m2->mLine1);
	bkpVec3MultVec3(&m1->mLine2, &m2->mLine2);
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpMat3DotVec3(const BkpMat3 * m, const BkpVec3 * v)
{
	BkpVec3 c0 = bkpMat3GetColumn0(m);
	BkpVec3 c1 = bkpMat3GetColumn1(m);
	BkpVec3 c2 = bkpMat3GetColumn2(m);
	return bkpVec3(bkpDot3D(v, &c0), bkpDot3D(v, &c1), bkpDot3D(v, &c2));
}

/*_____________________________________________________________________________________________*/
BkpVec3 bkpVec3DotMat3(const BkpVec3 * v, const BkpMat3 * m)
{
	return bkpVec3(bkpDot3D(&m->mLine0, v), bkpDot3D(&m->mLine1, v), bkpDot3D(&m->mLine2, v));
}

/*_____________________________________________________________________________________________*/
void bkpPrintMat3(const char * title, BkpMat3 * m)
{
	fprintf(stdout, "%s\n  [\n", title);
	fprintf(stdout, "    "); bkpPrintVec3("", &m->mLine0);
	fprintf(stdout, "    "); bkpPrintVec3("", &m->mLine1);
	fprintf(stdout, "    "); bkpPrintVec3("", &m->mLine2);
	fprintf(stdout, "  ]\n");
}

/*_____________________________________________________________________________________________*/
void bkpSetIdentityMat3(BkpMat3 * m)
{
	m->mLine0.x = m->mLine1.y = m->mLine2.z = 1.0f;
	m->mLine0.y = m->mLine1.x = m->mLine2.y = 0.0f;
	m->mLine0.z = m->mLine1.z = m->mLine2.x = 0.0f;
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpIdentityMat3()
{
	return (BkpMat3){
		.mLine0 = bkpVec3Xaxis(),
		.mLine1 = bkpVec3Yaxis(),
		.mLine2 = bkpVec3Zaxis(),
	};
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpNullMat3()
{
	return bkpMat3(0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f);
}

/*_____________________________________________________________________________________________
 * -------- 	                 BkpMatrice4											---------
 _____________________________________________________________________________________________*/

BkpMat4 bkpMat4(float a00, float a01, float a02, float a03, float a10, float a11, float a12, float a13,
		float a20, float a21, float a22, float a23, float a30, float a31, float a32, float a33)
{
	return (BkpMat4){
	.mLine0 = bkpVec4(a00, a01, a02, a03),
	.mLine1 = bkpVec4(a10, a11, a12, a13),
	.mLine2 = bkpVec4(a20, a21, a22, a23),
	.mLine3 = bkpVec4(a30, a31, a32, a33)};
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetColumn0(const BkpMat4 * mat)
{
	return bkpVec4(mat->mLine0.x, mat->mLine1.x, mat->mLine2.x, mat->mLine3.x);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetColumn0(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine0.x = v->x;
	m->mLine1.x = v->y;
	m->mLine2.x = v->z;
	m->mLine3.x = v->w;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetColumn1(const BkpMat4 * mat)
{
	return bkpVec4(mat->mLine0.y, mat->mLine1.y, mat->mLine2.y, mat->mLine3.y);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetColumn1(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine0.y = v->x;
	m->mLine1.y = v->y;
	m->mLine2.y = v->z;
	m->mLine3.y = v->w;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetColumn2(const BkpMat4 * mat)
{
	return bkpVec4(mat->mLine0.z, mat->mLine1.z, mat->mLine2.z, mat->mLine3.z);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetColumn2(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine0.z = v->x;
	m->mLine1.z = v->y;
	m->mLine2.z = v->z;
	m->mLine3.z = v->w;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetColumn3(const BkpMat4 * mat)
{
	return bkpVec4(mat->mLine0.w, mat->mLine1.w, mat->mLine2.w, mat->mLine3.w);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetColumn3(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine0.w = v->x;
	m->mLine1.w = v->y;
	m->mLine2.w = v->z;
	m->mLine3.w = v->w;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetLine0(const BkpMat4 * mat)
{
	return mat->mLine0;
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetLine0(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine0 = *v;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetLine1(const BkpMat4 * mat)
{
	return mat->mLine1;
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetLine1(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine1 = *v;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetLine2(const BkpMat4 * mat)
{
	return mat->mLine2;
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetLine2(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine2 = *v;
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4GetLine3(const BkpMat4 * mat)
{
	return mat->mLine3;
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetLine3(BkpMat4 * m, const BkpVec4 * v)
{
	m->mLine3 = *v;
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat4GetUpper2D(const BkpMat4 * m)
{
	return bkpMat2(m->mLine0.x, m->mLine0.y, m->mLine1.x, m->mLine1.y);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat4GetUpper3D(const BkpMat4 * m)
{
	return bkpMat3(m->mLine0.x, m->mLine0.y, m->mLine0.z,
			m->mLine1.x, m->mLine1.y, m->mLine1.z,
			m->mLine2.x, m->mLine2.y, m->mLine2.z);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4Sum(const BkpMat4 * m1, const BkpMat4 * m2)
{
	return (BkpMat4){.mLine0 = bkpVec4Sum(&m1->mLine0, &m2->mLine0),
		.mLine1 = bkpVec4Sum(&m1->mLine1, &m2->mLine1),
		.mLine2 = bkpVec4Sum(&m1->mLine2, &m2->mLine2),
		.mLine3 = bkpVec4Sum(&m1->mLine3, &m2->mLine3)};
}

/*_____________________________________________________________________________________________*/
void bkpMat4Add(BkpMat4 * m1, const BkpMat4 * m2)
{
	bkpVec4Add(&m1->mLine0, &m2->mLine0);
	bkpVec4Add(&m1->mLine1, &m2->mLine1);
	bkpVec4Add(&m1->mLine2, &m2->mLine2);
	bkpVec4Add(&m1->mLine3, &m2->mLine3);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpNegateMat4(BkpMat4 * m1)
{
	return (BkpMat4){.mLine0 = bkpNegateVec4(&m1->mLine0),
		.mLine1 = bkpNegateVec4(&m1->mLine1),
		.mLine2 = bkpNegateVec4(&m1->mLine2),
		.mLine3 = bkpNegateVec4(&m1->mLine3)};
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4Substract(const BkpMat4 * m1, const BkpMat4 * m2)
{
	return (BkpMat4){.mLine0 = bkpVec4Substract(&m1->mLine0, &m2->mLine0),
		.mLine1 = bkpVec4Substract(&m1->mLine1, &m2->mLine1),
		.mLine2 = bkpVec4Substract(&m1->mLine2, &m2->mLine2),
		.mLine3 = bkpVec4Substract(&m1->mLine3, &m2->mLine3)};
}

/*_____________________________________________________________________________________________*/
void bkpMat4Sub(BkpMat4 * m1, const BkpMat4 * m2)
{
	bkpVec4Sub(&m1->mLine0, &m2->mLine0);
	bkpVec4Sub(&m1->mLine1, &m2->mLine1);
	bkpVec4Sub(&m1->mLine2, &m2->mLine2);
	bkpVec4Sub(&m1->mLine3, &m2->mLine3);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4xScalar(const BkpMat4 * m1, const float scalar)
{
	return (BkpMat4){.mLine0 = bkpVec4xScalar(&m1->mLine0, scalar),
		.mLine1 = bkpVec4xScalar(&m1->mLine1, scalar),
		.mLine2 = bkpVec4xScalar(&m1->mLine2, scalar),
		.mLine3 = bkpVec4xScalar(&m1->mLine3, scalar)};
}

/*_____________________________________________________________________________________________*/
void bkpMat4MultScalar(BkpMat4 * m1, const float scalar)
{
	m1->mLine0 = bkpVec4xScalar(&m1->mLine0, scalar);
	m1->mLine1 = bkpVec4xScalar(&m1->mLine1, scalar);
	m1->mLine2 = bkpVec4xScalar(&m1->mLine2, scalar);
	m1->mLine3 = bkpVec4xScalar(&m1->mLine3, scalar);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4xVec4(const BkpMat4 * m1, const BkpVec4 * v)
{
	return bkpMat4(m1->mLine0.x * v->x, m1->mLine0.y * v->y, m1->mLine0.z * v->z, m1->mLine0.w * v->w,
			m1->mLine1.x * v->x, m1->mLine1.y * v->y, m1->mLine1.z * v->z, m1->mLine1.w * v->w,
			m1->mLine2.x * v->x, m1->mLine2.y * v->y, m1->mLine2.z * v->z, m1->mLine2.w * v->w,
			m1->mLine3.x * v->x, m1->mLine3.y * v->y, m1->mLine3.z * v->z, m1->mLine3.w * v->w);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4xMat4(const BkpMat4 * m1, const BkpMat4 * m2)
{
	return bkpMat4(m1->mLine0.x * m2->mLine0.x, m1->mLine0.y * m2->mLine0.y, m1->mLine0.z * m2->mLine0.z, m1->mLine0.w * m2->mLine0.w,
			m1->mLine1.x * m2->mLine1.x, m1->mLine1.y * m2->mLine1.y, m1->mLine1.z * m2->mLine1.z, m1->mLine1.w * m2->mLine1.w,
			m1->mLine2.x * m2->mLine2.x, m1->mLine2.y * m2->mLine2.y, m1->mLine2.z * m2->mLine2.z, m1->mLine2.w * m2->mLine2.w,
			m1->mLine3.x * m2->mLine3.x, m1->mLine3.y * m2->mLine3.y, m1->mLine3.z * m2->mLine3.z, m1->mLine3.w * m2->mLine3.w);
}

/*_____________________________________________________________________________________________*/
void bkpMat4MultMat4(BkpMat4 * m1, const BkpMat4 * m2)
{
	bkpVec4MultVec4(&m1->mLine0, &m2->mLine0);
	bkpVec4MultVec4(&m1->mLine1, &m2->mLine1);
	bkpVec4MultVec4(&m1->mLine2, &m2->mLine2);
	bkpVec4MultVec4(&m1->mLine3, &m2->mLine3);
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpMat4DotVec4(const BkpMat4 * m, const BkpVec4 * v)
{
	BkpVec4 c0 = bkpMat4GetColumn0(m);
	BkpVec4 c1 = bkpMat4GetColumn1(m);
	BkpVec4 c2 = bkpMat4GetColumn2(m);
	BkpVec4 c3 = bkpMat4GetColumn3(m);
	return bkpVec4(bkpDot4D(v, &c0), bkpDot4D(v, &c1), bkpDot4D(v, &c2), bkpDot4D(v, &c3));
}

/*_____________________________________________________________________________________________*/
BkpVec4 bkpVec4DotMat4(const BkpVec4 * v, const BkpMat4 * m)
{
	return bkpVec4(bkpDot4D(&m->mLine0, v), bkpDot4D(&m->mLine1, v),
			bkpDot4D(&m->mLine2, v), bkpDot4D(&m->mLine3, v));
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4DotMat4(const BkpMat4 * m, const BkpMat4 * mat)
{
	/* mLines are Vulkan columns: C_vk = m_vk * mat_vk requires computing mat_bkp * m_bkp row-major */
	return bkpMat4(
			mat->mLine0.x * m->mLine0.x + mat->mLine0.y * m->mLine1.x + mat->mLine0.z * m->mLine2.x + mat->mLine0.w * m->mLine3.x,
			mat->mLine0.x * m->mLine0.y + mat->mLine0.y * m->mLine1.y + mat->mLine0.z * m->mLine2.y + mat->mLine0.w * m->mLine3.y,
			mat->mLine0.x * m->mLine0.z + mat->mLine0.y * m->mLine1.z + mat->mLine0.z * m->mLine2.z + mat->mLine0.w * m->mLine3.z,
			mat->mLine0.x * m->mLine0.w + mat->mLine0.y * m->mLine1.w + mat->mLine0.z * m->mLine2.w + mat->mLine0.w * m->mLine3.w,

			mat->mLine1.x * m->mLine0.x + mat->mLine1.y * m->mLine1.x + mat->mLine1.z * m->mLine2.x + mat->mLine1.w * m->mLine3.x,
			mat->mLine1.x * m->mLine0.y + mat->mLine1.y * m->mLine1.y + mat->mLine1.z * m->mLine2.y + mat->mLine1.w * m->mLine3.y,
			mat->mLine1.x * m->mLine0.z + mat->mLine1.y * m->mLine1.z + mat->mLine1.z * m->mLine2.z + mat->mLine1.w * m->mLine3.z,
			mat->mLine1.x * m->mLine0.w + mat->mLine1.y * m->mLine1.w + mat->mLine1.z * m->mLine2.w + mat->mLine1.w * m->mLine3.w,

			mat->mLine2.x * m->mLine0.x + mat->mLine2.y * m->mLine1.x + mat->mLine2.z * m->mLine2.x + mat->mLine2.w * m->mLine3.x,
			mat->mLine2.x * m->mLine0.y + mat->mLine2.y * m->mLine1.y + mat->mLine2.z * m->mLine2.y + mat->mLine2.w * m->mLine3.y,
			mat->mLine2.x * m->mLine0.z + mat->mLine2.y * m->mLine1.z + mat->mLine2.z * m->mLine2.z + mat->mLine2.w * m->mLine3.z,
			mat->mLine2.x * m->mLine0.w + mat->mLine2.y * m->mLine1.w + mat->mLine2.z * m->mLine2.w + mat->mLine2.w * m->mLine3.w,

			mat->mLine3.x * m->mLine0.x + mat->mLine3.y * m->mLine1.x + mat->mLine3.z * m->mLine2.x + mat->mLine3.w * m->mLine3.x,
			mat->mLine3.x * m->mLine0.y + mat->mLine3.y * m->mLine1.y + mat->mLine3.z * m->mLine2.y + mat->mLine3.w * m->mLine3.y,
			mat->mLine3.x * m->mLine0.z + mat->mLine3.y * m->mLine1.z + mat->mLine3.z * m->mLine2.z + mat->mLine3.w * m->mLine3.z,
			mat->mLine3.x * m->mLine0.w + mat->mLine3.y * m->mLine1.w + mat->mLine3.z * m->mLine2.w + mat->mLine3.w * m->mLine3.w
			);
}

/*_____________________________________________________________________________________________*/
void bkpMat4Dot(BkpMat4 * m, const BkpMat4 * m2)
{
	const BkpVec4 cm0 = bkpMat4GetColumn0(m);
	const BkpVec4 cm1 = bkpMat4GetColumn1(m);
	const BkpVec4 cm2 = bkpMat4GetColumn2(m);
	const BkpVec4 cm3 = bkpMat4GetColumn3(m);

	float _0_ = bkpDot4D(&m2->mLine0, &cm0);
	float _1_ = bkpDot4D(&m2->mLine0, &cm1);
	float _2_ = bkpDot4D(&m2->mLine0, &cm2);
	float _3_ = bkpDot4D(&m2->mLine0, &cm3);
	float _4_ = bkpDot4D(&m2->mLine1, &cm0);
	float _5_ = bkpDot4D(&m2->mLine1, &cm1);
	float _6_ = bkpDot4D(&m2->mLine1, &cm2);
	float _7_ = bkpDot4D(&m2->mLine1, &cm3);
	float _8_ = bkpDot4D(&m2->mLine2, &cm0);
	float _9_ = bkpDot4D(&m2->mLine2, &cm1);
	float _10_ = bkpDot4D(&m2->mLine2, &cm2);
	float _11_ = bkpDot4D(&m2->mLine2, &cm3);
	float _12_ = bkpDot4D(&m2->mLine3, &cm0);
	float _13_ = bkpDot4D(&m2->mLine3, &cm1);
	float _14_ = bkpDot4D(&m2->mLine3, &cm2);
	float _15_ = bkpDot4D(&m2->mLine3, &cm3);

	m->mLine0.x = _0_; m->mLine0.y = _1_; m->mLine0.z = _2_; m->mLine0.w = _3_;
	m->mLine1.x = _4_; m->mLine1.y = _5_; m->mLine1.z = _6_; m->mLine1.w = _7_;
	m->mLine2.x = _8_; m->mLine2.y = _9_; m->mLine2.z = _10_; m->mLine2.w = _11_;
	m->mLine3.x = _12_; m->mLine3.y = _13_; m->mLine3.z = _14_; m->mLine3.w = _15_;
}
/*_____________________________________________________________________________________________*/
void bkpPrintMat4(const char * title, BkpMat4 * m)
{
	fprintf(stdout, "%s\n  [\n", title);
	fprintf(stdout, "    "); bkpPrintVec4("", &m->mLine0);
	fprintf(stdout, "    "); bkpPrintVec4("", &m->mLine1);
	fprintf(stdout, "    "); bkpPrintVec4("", &m->mLine2);
	fprintf(stdout, "    "); bkpPrintVec4("", &m->mLine3);
	fprintf(stdout, "  ]\n");
}

/*_____________________________________________________________________________________________*/
void bkpSetIdentityMat4(BkpMat4 * m)
{
	m->mLine0.x = m->mLine1.y = m->mLine2.z = m->mLine3.w = 1.0f;
	m->mLine0.y = m->mLine1.x = m->mLine2.y = m->mLine3.y = 0.0f;
	m->mLine0.z = m->mLine1.z = m->mLine2.x = m->mLine3.z = 0.0f;
	m->mLine0.w = m->mLine1.w = m->mLine2.w = m->mLine3.x = 0.0f;
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpIdentityMat4()
{
	return (BkpMat4){
		.mLine0 = bkpVec4Xaxis(),
		.mLine1 = bkpVec4Yaxis(),
		.mLine2 = bkpVec4Zaxis(),
		.mLine3 = bkpVec4Waxis(),
	};
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpNullMat4()
{
	return bkpMat4(0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetScale(BkpMat4 * m, const BkpVec3 * v)
{
	m->mLine0.x *= v->x;
	m->mLine0.y *= v->y;
	m->mLine0.z *= v->z;

	m->mLine1.x *= v->x;
	m->mLine1.y *= v->y;
	m->mLine1.z *= v->z;

	m->mLine2.x *= v->x;
	m->mLine2.y *= v->y;
	m->mLine2.z *= v->z;

	m->mLine3.x *= v->x;
	m->mLine3.y *= v->y;
	m->mLine3.z *= v->z;
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetTranslate(BkpMat4 * m, const BkpVec3 * v)
{
	m->mLine0.x += m->mLine0.w * v->x;
	m->mLine0.y += m->mLine0.w * v->y;
	m->mLine0.z += m->mLine0.w * v->z;
	m->mLine0.w = m->mLine0.w;

	m->mLine1.x += m->mLine1.w * v->x;
	m->mLine1.y += m->mLine1.w * v->y;
	m->mLine1.z += m->mLine1.w * v->z;
	m->mLine1.w = m->mLine1.w;

	m->mLine2.x += m->mLine2.w * v->x;
	m->mLine2.y += m->mLine2.w * v->y;
	m->mLine2.z += m->mLine2.w * v->z;
	m->mLine2.w = m->mLine2.w;

	m->mLine3.x += m->mLine3.w * v->x;
	m->mLine3.y += m->mLine3.w * v->y;
	m->mLine3.z += m->mLine3.w * v->z;
	m->mLine3.w = m->mLine3.w;
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetRotate(BkpMat4 * m, const BkpVec3 * axis, float angle)
{
	BkpMat4 r = bkpMat4GetRotation(axis, angle);
	bkpMat4Dot(m, &r);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetRotateQuatAxis(BkpMat4 * m, const BkpVec3 * axis, float angle)
{
	*m = bkpMat4RotateQAxis(m, axis, angle);
}

/*_____________________________________________________________________________________________*/
void bkpMat4SetRotateQuat(BkpMat4 * m, const BkpQuat * q)
{
	*m = bkpMat4RotateQ(m, q);
}

/*_____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________*/

/*_____________________________________________________________________________________________
  -------- 		                 Operations 										---------
  _____________________________________________________________________________________________*/

float bkpMat2Det(const BkpMat2 * mat)
{
	return mat->m00 * mat->m11 - mat->m01 * mat->m10;
}

/*_____________________________________________________________________________________________*/
float bkpMat3Det(const BkpMat3 * mat)
{
	BkpVec3 v = bkpCross3D(&mat->mLine1, &mat->mLine2);
	return bkpDot3D(&mat->mLine0, &v);
}

/*_____________________________________________________________________________________________*/
float bkpMat4Det(const BkpMat4 * mat)
{
	BkpMat3 t01 = bkpMat3(mat->mLine1.y, mat->mLine1.z, mat->mLine1.w,
			mat->mLine2.y, mat->mLine2.z, mat->mLine2.w,
			mat->mLine3.y, mat->mLine3.z, mat->mLine3.w);
	BkpMat3 t11 = bkpMat3(mat->mLine0.y, mat->mLine0.z, mat->mLine0.w,
			mat->mLine2.y, mat->mLine2.z, mat->mLine2.w,
			mat->mLine3.y, mat->mLine3.z, mat->mLine3.w);
	BkpMat3 t21 = bkpMat3(mat->mLine0.y, mat->mLine0.z, mat->mLine0.w,
			mat->mLine1.y, mat->mLine1.z, mat->mLine1.w,
			mat->mLine3.y, mat->mLine3.z, mat->mLine3.w);
	BkpMat3 t31 = bkpMat3(mat->mLine0.y, mat->mLine0.z, mat->mLine0.w,
			mat->mLine1.y, mat->mLine1.z, mat->mLine1.w,
			mat->mLine2.y, mat->mLine2.z, mat->mLine2.w);

	return (bkpMat3Det(&t01) * mat->mLine0.x) -
		(bkpMat3Det(&t11) * mat->mLine1.x) +
		(bkpMat3Det(&t21) * mat->mLine2.x) -
		(bkpMat3Det(&t31) * mat->mLine3.x);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2Inverse(const BkpMat2 * mat)
{
	return bkpMat2DetInverse(mat, bkpMat2Det(mat));
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3Inverse(const BkpMat3 * mat)
{
	return bkpMat3DetInverse(mat, bkpMat3Det(mat));
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4Inverse(const BkpMat4 * mat)
{
	return bkpMat4DetInverse(mat, bkpMat4Det(mat));
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2DetInverse(const BkpMat2 * mat, float determinant)
{
	float invDet = 1.0f / determinant;
	BkpMat2 m = bkpMat2( mat->m11, -mat->m01, -mat->m10, mat->m00);
	return bkpMat2xScalar(&m , invDet);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3DetInverse(const BkpMat3 * mat, float determinant)
{
	float invDet = 1.0f / determinant;

	BkpMat2 m00 =bkpMat2(mat->mLine1.y, mat->mLine2.y, mat->mLine1.z, mat->mLine2.z);
	BkpMat2 m01 =bkpMat2(mat->mLine0.y, mat->mLine2.y, mat->mLine0.z, mat->mLine2.z);
	BkpMat2 m02 =bkpMat2(mat->mLine0.y, mat->mLine1.y, mat->mLine0.z, mat->mLine1.z);

	BkpMat2 m10 =bkpMat2(mat->mLine1.x, mat->mLine2.x, mat->mLine1.z, mat->mLine2.z);
	BkpMat2 m11 =bkpMat2(mat->mLine0.x, mat->mLine2.x, mat->mLine0.z, mat->mLine2.z);
	BkpMat2 m12 =bkpMat2(mat->mLine0.x, mat->mLine1.x, mat->mLine0.z, mat->mLine1.z);

	BkpMat2 m20 =bkpMat2(mat->mLine1.x, mat->mLine2.x, mat->mLine1.y, mat->mLine2.y);
	BkpMat2 m21 =bkpMat2(mat->mLine0.x, mat->mLine2.x, mat->mLine0.y, mat->mLine2.y);
	BkpMat2 m22 =bkpMat2(mat->mLine0.x, mat->mLine1.x, mat->mLine0.y, mat->mLine1.y);

	BkpMat3 r = bkpMat3(bkpMat2Det(&m00), -bkpMat2Det(&m01), bkpMat2Det(&m02),
			-bkpMat2Det(&m10), bkpMat2Det(&m11), -bkpMat2Det(&m12),
			bkpMat2Det(&m20), -bkpMat2Det(&m21), bkpMat2Det(&m22));

	return bkpMat3xScalar(&r , invDet);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4DetInverse(const BkpMat4 * mat, float determinant)
{
	float invDet = 1.0f / determinant;

	BkpMat3 m0 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine1), bkpVec4GetYZW(&mat->mLine2), bkpVec4GetYZW(&mat->mLine3));
	BkpMat3 m1 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine1), bkpVec4GetXZW(&mat->mLine2), bkpVec4GetXZW(&mat->mLine3));
	BkpMat3 m2 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine1), bkpVec4GetXYW(&mat->mLine2), bkpVec4GetXYW(&mat->mLine3));
	BkpMat3 m3 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine1), bkpVec4GetXYZ(&mat->mLine2), bkpVec4GetXYZ(&mat->mLine3));

	BkpMat3 m4 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine0), bkpVec4GetYZW(&mat->mLine2), bkpVec4GetYZW(&mat->mLine3));
	BkpMat3 m5 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine0), bkpVec4GetXZW(&mat->mLine2), bkpVec4GetXZW(&mat->mLine3));
	BkpMat3 m6 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine0), bkpVec4GetXYW(&mat->mLine2), bkpVec4GetXYW(&mat->mLine3));
	BkpMat3 m7 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine0), bkpVec4GetXYZ(&mat->mLine2), bkpVec4GetXYZ(&mat->mLine3));

	BkpMat3 m8 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine0), bkpVec4GetYZW(&mat->mLine1), bkpVec4GetYZW(&mat->mLine3));
	BkpMat3 m9 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine0), bkpVec4GetXZW(&mat->mLine1), bkpVec4GetXZW(&mat->mLine3));
	BkpMat3 m10 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine0), bkpVec4GetXYW(&mat->mLine1), bkpVec4GetXYW(&mat->mLine3));
	BkpMat3 m11 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine0), bkpVec4GetXYZ(&mat->mLine1), bkpVec4GetXYZ(&mat->mLine3));

	BkpMat3 m12 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine0), bkpVec4GetYZW(&mat->mLine1), bkpVec4GetYZW(&mat->mLine2));
	BkpMat3 m13 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine0), bkpVec4GetXZW(&mat->mLine1), bkpVec4GetXZW(&mat->mLine2));
	BkpMat3 m14 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine0), bkpVec4GetXYW(&mat->mLine1), bkpVec4GetXYW(&mat->mLine2));
	BkpMat3 m15 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine0), bkpVec4GetXYZ(&mat->mLine1), bkpVec4GetXYZ(&mat->mLine2));

	BkpMat4 m = bkpMat4(bkpMat3Det(&m0), -bkpMat3Det(&m4), bkpMat3Det(&m8), -bkpMat3Det(&m12),
			-bkpMat3Det(&m1), bkpMat3Det(&m5), -bkpMat3Det(&m9), bkpMat3Det(&m13),
			bkpMat3Det(&m2), -bkpMat3Det(&m6), bkpMat3Det(&m10), -bkpMat3Det(&m14),
			-bkpMat3Det(&m3), bkpMat3Det(&m7), -bkpMat3Det(&m11), bkpMat3Det(&m15));

	return bkpMat4xScalar(&m, invDet);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2Transpose(const BkpMat2 * mat)
{
	return bkpMat2(mat->m00, mat->m10, mat->m01, mat->m11);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3Transpose(const BkpMat3 * mat)
{
	return bkpMat3(mat->mLine0.x, mat->mLine1.x, mat->mLine2.x,
			mat->mLine0.y, mat->mLine1.y, mat->mLine2.y,
			mat->mLine0.z, mat->mLine1.z, mat->mLine2.z);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4Transpose(const BkpMat4 * mat)
{
	return bkpMat4(mat->mLine0.x, mat->mLine1.x, mat->mLine2.x, mat->mLine3.x,
			mat->mLine0.y, mat->mLine1.y, mat->mLine2.y, mat->mLine3.y,
			mat->mLine0.z, mat->mLine1.z, mat->mLine2.z, mat->mLine3.z,
			mat->mLine0.w, mat->mLine1.w, mat->mLine2.w, mat->mLine3.w);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2InverseTranspose(const BkpMat2 * mat)
{
	float invDet = 1.0f / bkpMat2Det(mat);
	BkpMat2 m = bkpMat2(mat->m11, -mat->m10, -mat->m01, mat->m00);
	return bkpMat2xScalar(&m, invDet);
}

/*_____________________________________________________________________________________________*/
BkpMat3 bkpMat3InverseTranspose(const BkpMat3 * mat)
{
	float invDet = 1.0f / bkpMat3Det(mat);
	BkpMat2 m0 = bkpMat2(mat->mLine1.y, mat->mLine2.y, mat->mLine1.z, mat->mLine2.z);
	BkpMat2 m1 = bkpMat2(mat->mLine1.x, mat->mLine2.x, mat->mLine1.z, mat->mLine2.z);
	BkpMat2 m2 = bkpMat2(mat->mLine1.x, mat->mLine2.x, mat->mLine1.y, mat->mLine2.y);

	BkpMat2 m3 = bkpMat2(mat->mLine0.y, mat->mLine2.y, mat->mLine0.z, mat->mLine2.z);
	BkpMat2 m4 = bkpMat2(mat->mLine0.x, mat->mLine2.x, mat->mLine0.z, mat->mLine2.z);
	BkpMat2 m5 = bkpMat2(mat->mLine0.x, mat->mLine2.x, mat->mLine0.y, mat->mLine2.y);

	BkpMat2 m6 = bkpMat2(mat->mLine0.y, mat->mLine1.y, mat->mLine0.z, mat->mLine1.z);
	BkpMat2 m7 = bkpMat2(mat->mLine0.x, mat->mLine1.x, mat->mLine0.z, mat->mLine1.z);
	BkpMat2 m8 = bkpMat2(mat->mLine0.x, mat->mLine1.x, mat->mLine0.y, mat->mLine1.y);

	BkpMat3 m = bkpMat3(
			bkpMat2Det(&m0), -bkpMat2Det(&m1), bkpMat2Det(&m2),
			-bkpMat2Det(&m3), bkpMat2Det(&m4), -bkpMat2Det(&m5),
			bkpMat2Det(&m6), -bkpMat2Det(&m7), bkpMat2Det(&m8));

	return bkpMat3xScalar(&m, invDet);

}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4InverseTranspose(const BkpMat4 * mat)
{
	float invDet = 1.0f / bkpMat4Det(mat);

	BkpMat3 m0 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine1), bkpVec4GetYZW(&mat->mLine2), bkpVec4GetYZW(&mat->mLine3));
	BkpMat3 m1 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine1), bkpVec4GetXZW(&mat->mLine2), bkpVec4GetXZW(&mat->mLine3));
	BkpMat3 m2 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine1), bkpVec4GetXYW(&mat->mLine2), bkpVec4GetXYW(&mat->mLine3));
	BkpMat3 m3 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine1), bkpVec4GetXYZ(&mat->mLine2), bkpVec4GetXYZ(&mat->mLine3));

	BkpMat3 m4 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine0), bkpVec4GetYZW(&mat->mLine2), bkpVec4GetYZW(&mat->mLine3));
	BkpMat3 m5 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine0), bkpVec4GetXZW(&mat->mLine2), bkpVec4GetXZW(&mat->mLine3));
	BkpMat3 m6 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine0), bkpVec4GetXYW(&mat->mLine2), bkpVec4GetXYW(&mat->mLine3));
	BkpMat3 m7 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine0), bkpVec4GetXYZ(&mat->mLine2), bkpVec4GetXYZ(&mat->mLine3));

	BkpMat3 m8 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine0), bkpVec4GetYZW(&mat->mLine1), bkpVec4GetYZW(&mat->mLine3));
	BkpMat3 m9 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine0), bkpVec4GetXZW(&mat->mLine1), bkpVec4GetXZW(&mat->mLine3));
	BkpMat3 m10 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine0), bkpVec4GetXYW(&mat->mLine1), bkpVec4GetXYW(&mat->mLine3));
	BkpMat3 m11 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine0), bkpVec4GetXYZ(&mat->mLine1), bkpVec4GetXYZ(&mat->mLine3));

	BkpMat3 m12 = bkpMat3FromVec3(bkpVec4GetYZW(&mat->mLine0), bkpVec4GetYZW(&mat->mLine1), bkpVec4GetYZW(&mat->mLine2));
	BkpMat3 m13 = bkpMat3FromVec3(bkpVec4GetXZW(&mat->mLine0), bkpVec4GetXZW(&mat->mLine1), bkpVec4GetXZW(&mat->mLine2));
	BkpMat3 m14 = bkpMat3FromVec3(bkpVec4GetXYW(&mat->mLine0), bkpVec4GetXYW(&mat->mLine1), bkpVec4GetXYW(&mat->mLine2));
	BkpMat3 m15 = bkpMat3FromVec3(bkpVec4GetXYZ(&mat->mLine0), bkpVec4GetXYZ(&mat->mLine1), bkpVec4GetXYZ(&mat->mLine2));


	BkpMat4 m = bkpMat4(bkpMat3Det(&m0), -bkpMat3Det(&m4), bkpMat3Det(&m8), -bkpMat3Det(&m12),
			-bkpMat3Det(&m1), bkpMat3Det(&m5), -bkpMat3Det(&m9), bkpMat3Det(&m13),
			bkpMat3Det(&m2), -bkpMat3Det(&m6), bkpMat3Det(&m10), -bkpMat3Det(&m14),
			-bkpMat3Det(&m3), bkpMat3Det(&m7), -bkpMat3Det(&m11), bkpMat3Det(&m15));

	return bkpMat4xScalar(&m, invDet);
}

/*_____________________________________________________________________________________________*/
BkpMat2 bkpMat2Rotation(float angle)
{
	float cos_ = cosf(angle);
	float sin_ = sinf(angle);

	return bkpMat2(cos_, -sin_, sin_, cos_);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4GetRotationX(float angle)
{
	float sin_ = sinf(angle);
	float cos_ = cosf(angle);

	return bkpMat4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, cos_, sin_, 0.0f,
			0.0f, -sin_, cos_, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4GetRotationY(float angle)
{
	float sin_ = sinf(angle);
	float cos_ = cosf(angle);

	return bkpMat4(
			cos_, 0.0f, -sin_, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			sin_, 0.0f, cos_, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4GetRotationZ(float angle)
{
	float sin_ = sinf(angle);
	float cos_ = cosf(angle);

	return bkpMat4(
			cos_, sin_, 0.0f, 0.0f,
			-sin_, cos_, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4GetRotation(const BkpVec3 * axis, float radian)
{
	float c = cosf(radian);
	float s = sinf(radian);

	return bkpMat4(
			c + axis->x * axis->x * (1 - c), 				axis->y * axis->x * (1 - c) + axis->z * s, 	axis->z * axis->x * (1 - c) - axis->y * s 	, 0.0f,
			axis->x * axis->y * (1 - c) - axis->z * s, 	c + axis->y * axis->y * (1 - c), 				axis->z * axis->y * (1 - c) + axis->x * s 	, 0.0f,
			axis->x * axis->z * (1 - c) + axis->y * s, 	axis->y * axis->z * (1 - c) - axis->x * s, 	c + axis->z * axis->z * (1 - c)   		, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4RotateX(const BkpMat4 * mat, float radian)
{
	BkpMat4 r = bkpMat4GetRotationX(radian);
	return bkpMat4DotMat4(mat, &r);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4RotateY(const BkpMat4 * mat, float radian)
{
	BkpMat4 r = bkpMat4GetRotationY(radian);
	return bkpMat4DotMat4(mat, &r);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4RotateZ(const BkpMat4 * mat, float radian)
{
	BkpMat4 r = bkpMat4GetRotationZ(radian);
	return bkpMat4DotMat4(mat, &r);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4Rotate(const BkpMat4 * mat, const BkpVec3 * axis, float radian)
{
	BkpMat4 r = bkpMat4GetRotation(axis, radian);
	return bkpMat4DotMat4(mat, &r);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4RotateQAxis(const BkpMat4 * mat, const BkpVec3 * unit_axis, float radian)
{
	BkpQuat q = bkpQuatRotation(unit_axis, radian);

	BkpMat4 m = bkpIdentityMat4();

	m.mLine0.x = 1.0f - (2.0f * q.y * q.y) - (2.0f * q.z * q.z);
	m.mLine0.y = (2.0f * q.x * q.y) - (2.0f * q.z * q.w);
	m.mLine0.z = (2.0f * q.x * q.z) + (2.0f * q.y * q.w);

	m.mLine1.x = (2.0f * q.x * q.y) + (2.0f * q.z * q.w);
	m.mLine1.y = 1.0f - (2.0f * q.x * q.x) - (2.0f * q.z * q.z);
	m.mLine1.z = (2.0f * q.y * q.z) - (2.0f * q.x * q.w);

	m.mLine2.x = (2.0f * q.x * q.z) - (2.0f * q.y * q.w);
	m.mLine2.y = (2.0f * q.y * q.z) + (2.0f * q.x * q.w);
	m.mLine2.z = 1.0f - (2.0f * q.x * q.x) - (2.0f * q.y * q.y);

	return bkpMat4DotMat4(mat, &m);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4RotateQ(const BkpMat4 * mat, const BkpQuat * q)
{
	BkpMat4 m = bkpIdentityMat4();

	m.mLine0.x = 1.0f - (2.0f * q->y * q->y) - (2.0f * q->z * q->z);
	m.mLine0.y = (2.0f * q->x * q->y) - (2.0f * q->z * q->w);
	m.mLine0.z = (2.0f * q->x * q->z) + (2.0f * q->y * q->w);

	m.mLine1.x = (2.0f * q->x * q->y) + (2.0f * q->z * q->w);
	m.mLine1.y = 1.0f - (2.0f * q->x * q->x) - (2.0f * q->z * q->z);
	m.mLine1.z = (2.0f * q->y * q->z) - (2.0f * q->x * q->w);

	m.mLine2.x = (2.0f * q->x * q->z) - (2.0f * q->y * q->w);
	m.mLine2.y = (2.0f * q->y * q->z) + (2.0f * q->x * q->w);
	m.mLine2.z = 1.0f - (2.0f * q->x * q->x) - (2.0f * q->y * q->y);

	return bkpMat4DotMat4(mat, &m);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpMat4RotateQCenter(const BkpMat4 * mat, const BkpQuat * quat, const BkpVec3 * center)
{
	BkpVec3 axis = bkpQuatAxis(quat);
	BkpQuat q = bkpQuatRotation(&axis, quat->w);

	float x2 = q.x * q.x;
	float y2 = q.y * q.y;
	float z2 = q.z * q.z;
	float w2 = q.w * q.w;

	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float xw = q.x * q.w;

	float yz = q.y * q.z;
	float yw = q.y * q.w;

	float zw = q.z * q.w;

	BkpMat4 R = bkpMat4(
			x2 - y2 - z2 + w2,
			2.0f * (xy + zw),
			2.0f * (xz - yw),
			0.0f,

			2.0f * (xy - zw),
			(-x2) + y2 - z2 + w2,
			2.0f * (yz + xw),
			0.0f,

			2.0f * (xz + yw),
			2.0f * (yz - xw),
			(-x2) - y2 + z2 + w2,
			0.0f,

			center->x - center->x * R.mLine0.x - center->y * R.mLine0.y - center->z * R.mLine0.z,
			center->y - center->x * R.mLine1.x - center->y * R.mLine1.y - center->z * R.mLine1.z,
			center->z - center->x * R.mLine2.x - center->y * R.mLine2.y - center->z * R.mLine2.z,
			1.0f);
	return  bkpMat4DotMat4(mat, &R);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpGetTranslation(const BkpVec3 * v)
{
	return bkpMat4(1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			v->x, v->y, v->z, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpTranslate(const BkpMat4 * mat, const BkpVec3 * v)
{
	BkpMat4 m = bkpGetTranslation(v);
	return bkpMat4DotMat4(mat, &m);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpGetScaling(const BkpVec3 * v)
{
	return bkpMat4(v->x, 0.0f, 0.0f, 0.0f,
			0.0f, v->y, 0.0f, 0.0f,
			0.0f, 0.0f, v->z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpScale(const BkpMat4 * mat, const BkpVec3 * v)
{
	BkpMat4 m = bkpGetScaling(v);
	return bkpMat4DotMat4(mat, &m);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpOrtho(float left, float right, float bottom, float top, float near, float far)
{
	float rl = 1.0f / (right - left),
		  tb = 1.0f / (top - bottom),
		  fn = 1.0f / (far - near);

	//RightHand 0 to 1 see Greydog_C for details
	return bkpMat4(
			2.0f * rl           ,  0.0f	                , 0.0f			  , 0.0f,
			0.0f                ,  2.0f * tb            , 0.0f			  , 0.0f,
			0.0f	            ,  0.0f  		        , -fn             , 0.0f,
			-(right + left) * rl, -(top + bottom) * tb  , -(near) * fn    , 1.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpPerspective(float vFOV, float ratio, float near, float far)
{
	float f = 1.0f / tanf(vFOV/ 2.0f);

	// RightHand 0 to 1
	return bkpMat4(
			f / ratio,      0.0f,                         0.0f,  0.0f,
			0.0f 	 ,      f   ,                         0.0f,  0.0f,
			0.0f  	 ,      0.0f,           far / (near - far), -1.0f,
			0.0f 	 ,      0.0f, -(far * near) / (far - near),  0.0f);
}

/*_____________________________________________________________________________________________*/
BkpMat4 bkpLookAt(BkpVec3 position, BkpVec3 target, BkpVec3 up)
{
	BkpVec3 z = bkpVec3Substract(&target, &position);
	bkpSetNormalize3D(&z);
	bkpSetNormalize3D(&up);
	BkpVec3 x = bkpCross3D(&z, &up);
	bkpSetNormalize3D(&x);
	BkpVec3 y = bkpCross3D(&x, &z);

	return bkpMat4(
			x.x				 , y.x			    , -z.x				, 0.0f,
			x.y				 , y.y			    , -z.y				, 0.0f,
			x.z				 , y.z			    , -z.z				, 0.0f,
			-bkpDot3D(&x, &position), -bkpDot3D(&y, &position), bkpDot3D(&z, &position)  , 1.0f);
}

#ifdef __cplusplus
}
#endif

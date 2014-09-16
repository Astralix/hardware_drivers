/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


#ifndef __gc_vgsh_matrix_h_
#define __gc_vgsh_matrix_h_


/* Matrix access macro. */
#define _M(_m, _r, _c)  _m->m[_r][_c]


typedef struct _VGMatrix3x3
{
    _VGTemplate     m[3][3];
}_VGMatrix3x3;


typedef struct _VGVector2
{
    _VGTemplate     x, y;
} _VGVector2;

typedef struct _VGVector3
{
    _VGTemplate     x, y, z;
} _VGVector3;

typedef struct _VGVector4
{
    _VGTemplate     x, y, z, w;
} _VGVector4;


#define V3_SET(v, s1, s2, s3) \
    { v.x = s1; v.y = s2; v.z = s3;}


#define V2_SET(v, s1, s2) \
    { v.x = s1; v.y = s2;}

#define V2_SETV(v1, v2) \
    { v1.x = v2.x; v1.y = v2.y; }

#define V2_ADD(v, xx, yy) \
    { v.x += xx; v.y += yy; }

#define V2_ADDV(v1, v2) \
    { v1.x += v2.x; v1.y += v2.y; }

#define V2_SUB(v,xx,yy) \
    { v.x -= xx; v.y -= yy;}

#define V2_SUBV(v1, v2) \
    { v1.x -= v2.x; v1.y -= v2.y; }

#define V2_MUL(v, s) \
    { v.x = MULTIPLY(v.x, s); v.y = MULTIPLY(v.y, s); }

#define V2_DIV(v, s) \
    { v.x = DIVIDE(v.x, s); v.y = DIVIDE(v.y, s); }

#define V2_NORM(v) SQRT( MULTIPLY(v.x,v.x) + MULTIPLY(v.y,v.y) )

#define V2_NORMALIZE(v) \
    { _VGTemplate n = V2_NORM(v); v.x = DIVIDE(v.x, n); v.y = DIVIDE(v.y, n); }

#define V2_DOT(v1, v2) \
    (MULTIPLY(v1.x, v2.x) + MULTIPLY(v1.y, v2.y))




/*for opengl comun major matrix*/
#define SET_COLMNMAJOR_MAT(mat, m00, m01, m02, m03, \
                                m10, m11, m12, m13, \
                                m20, m21, m22, m23, \
                                m30, m31, m32, m33) { \
   mat[0][0] = m00; mat[1][0] = m01; mat[2][0] = m02; mat[3][0] = m03;\
   mat[0][1] = m10; mat[1][1] = m11; mat[2][1] = m12; mat[3][1] = m13;\
   mat[0][2] = m20; mat[1][2] = m21; mat[2][2] = m22; mat[3][2] = m23;\
   mat[0][3] = m30; mat[1][3] = m31; mat[2][3] = m32; mat[3][3] = m33;}


#define SET_GAL_MATRIX(mat, m00, m01, m02, m03, \
                            m10, m11, m12, m13, \
                            m20, m21, m22, m23, \
                            m30, m31, m32, m33) { \
   mat[0] = m00; mat[4] = m01; mat[8]  = m02; mat[12] = m03;\
   mat[1] = m10; mat[5] = m11; mat[9]  = m12; mat[13] = m13;\
   mat[2] = m20; mat[6] = m21; mat[10] = m22; mat[14] = m23;\
   mat[3] = m30; mat[7] = m31; mat[11] = m32; mat[15] = m33;}


#define vgmSET_GAL_MATRIX(mat, m00, m01, m02, m03, \
                            m10, m11, m12, m13, \
                            m20, m21, m22, m23, \
                            m30, m31, m32, m33) { \
   mat[0] = m00; mat[4] = m10; mat[8]  = m20; mat[12] = m30;\
   mat[1] = m01; mat[5] = m11; mat[9]  = m21; mat[13] = m31;\
   mat[2] = m02; mat[6] = m12; mat[10] = m22; mat[14] = m32;\
   mat[3] = m03; mat[7] = m13; mat[11] = m23; mat[15] = m33;}


#define AFFINE_TRANSFORM(vout, mat, v) \
    V2_SET(vout, v.x * mat.m[0][0] + v.y * mat.m[0][1] + mat.m[0][2], \
               v.x * mat.m[1][0] + v.y * mat.m[1][1] + mat.m[1][2])


#define AFFINE_TANGENT_TRANSFORM(vout, mat, v)  \
    V2_SET(vout, v.x * mat.m[0][0] + v.y * mat.m[0][1], v.x * mat.m[1][0] + v.y * mat.m[1][1])

#define MATRIX_DET(mat) \
    mat.m[0][0] * (mat.m[1][1]*mat.m[2][2] - mat.m[2][1]*mat.m[1][2]) + \
    mat.m[0][1] * (mat.m[2][0]*mat.m[1][2] - mat.m[1][0]*mat.m[2][2]) + \
    mat.m[0][2] * (mat.m[1][0]*mat.m[2][1] - mat.m[2][0]*mat.m[1][1])


#define FORCE_AFFINE(mat) { \
        mat[2][0] = 0.0f; mat[2][1] = 0.0f; mat[2][2] = 1.0f; }



#define MATRIX_MUL_VEC3(m, v, vout) \
    vout.x = v.x*m[0][0]+v.y*m[0][1]+v.z*m[0][2]; vout.y = v.x*m[1][0]+v.y*m[1][1]+v.z*m[1][2]; vout.z = v.x*m[2][0]+v.y*m[2][1]+v.z*m[2][2];

void _vgSetMatrix(_VGMatrix3x3 *matrix, _VGTemplate m00, _VGTemplate m01, _VGTemplate m02,
                                     _VGTemplate m10, _VGTemplate m11, _VGTemplate m12,
                                     _VGTemplate m20, _VGTemplate m21, _VGTemplate m22);

void _VGMatrixCtor(_VGMatrix3x3 *matrix);
void _VGMatrixDtor(_VGMatrix3x3 *matrix);

void MatrixScale(_VGMatrix3x3 *matrix, _VGTemplate tempsx, _VGTemplate tempsy);
void MatrixTranslate(_VGMatrix3x3* matrix, _VGTemplate temptx, _VGTemplate tempty);
void MatrixRotate(_VGMatrix3x3* matrix, _VGTemplate tempAngle);
gctINT32 InvertMatrix(_VGMatrix3x3 *m, _VGMatrix3x3 *mout);

void matrixToGAL(_VGMatrix3x3 *matrix, VGfloat m[16]);
void _MatrixToGAL(_VGMatrix3x3 *matrix, VGfloat m[16]);

gctBOOL IsMatrixEqual(_VGMatrix3x3 *matrix1, _VGMatrix3x3 *matrix2);
void MultMatrix(const _VGMatrix3x3 *matrix1, const _VGMatrix3x3 *matrix2,  _VGMatrix3x3 *matrixOut);
gctBOOL isAffine(_VGMatrix3x3 *matrix);

#endif /* __gc_vgsh_matrix_h_ */

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


#include "gc_vgsh_precomp.h"

void _VGMatrixCtor(_VGMatrix3x3 *matrix)
{
    _vgSetMatrix(matrix, 1.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f);
}

void _VGMatrixDtor(_VGMatrix3x3 *matrix)
{
}


static _VGMatrix3x3* getCurrentMatrix(_VGContext* context)
{
	gcmHEADER_ARG("context=0x%x", context);

    gcmASSERT(context);
    switch(context->matrixMode)
    {
    case VG_MATRIX_PATH_USER_TO_SURFACE:
		gcmFOOTER_ARG("return=0x%x", &context->pathUserToSurface);
        return &context->pathUserToSurface;

    case VG_MATRIX_IMAGE_USER_TO_SURFACE:
		gcmFOOTER_ARG("return=0x%x", &context->imageUserToSurface);
        return &context->imageUserToSurface;

    case VG_MATRIX_FILL_PAINT_TO_USER:
		gcmFOOTER_ARG("return=0x%x", &context->fillPaintToUser);
        return &context->fillPaintToUser;
    case VG_MATRIX_GLYPH_USER_TO_SURFACE:
		gcmFOOTER_ARG("return=0x%x", &context->glyphUserToSurface);
        return &context->glyphUserToSurface;


    default:
        gcmASSERT(context->matrixMode == VG_MATRIX_STROKE_PAINT_TO_USER);
		gcmFOOTER_ARG("return=0x%x", &context->strokePaintToUser);
        return &context->strokePaintToUser;
    }
}

gctBOOL isAffine(_VGMatrix3x3 *matrix)
{
	gcmHEADER_ARG("matrix=0x%x", matrix);

    if (matrix->m[2][0] == 0.0f && matrix->m[2][1] == 0.0f && matrix->m[2][2] == 1.0f)
	{
		gcmFOOTER_ARG("return=%s", "TRUE");
        return gcvTRUE;
	}

	gcmFOOTER_ARG("return=%s", "FALSE");
	return gcvFALSE;
}

void ForceAffine(_VGMatrix3x3 *matrix)
{
	gcmHEADER_ARG("matrix=0x%x", matrix);

    matrix->m[2][0] = 0.0f;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][2] = 1.0f;

	gcmFOOTER_NO();
}

void  vgLoadIdentity(void)
{
    _VGMatrix3x3* matrix;
	_VGContext* context;
    gcmHEADER();
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGLOADIDENTITY, 0);

    matrix = getCurrentMatrix(context);

    _vgSetMatrix(matrix, 1.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f);

    gcmFOOTER_NO();
}


void  vgLoadMatrix(const VGfloat * m)
{
    _VGMatrix3x3* matrix;
	_VGContext* context;
    gcmHEADER_ARG("m=0x%x", m);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGLOADMATRIX, 0);

    OVG_IF_ERROR(!m || !isAligned(m,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    matrix = getCurrentMatrix(context);

    _vgSetMatrix(matrix,      m[0], m[3], m[6],
                           m[1], m[4], m[7],
                           m[2], m[5], m[8]);

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(matrix);
    }

    gcmFOOTER_NO();
}

void  vgGetMatrix(VGfloat * m)
{
    _VGMatrix3x3* matrix;
	_VGContext* context;

    gcmHEADER_ARG("m=0x%x", m);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETMATRIX, 0);

    OVG_IF_ERROR(!m || !isAligned(m,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    matrix = getCurrentMatrix(context);

    m[0] = _M(matrix,0,0);
    m[1] = _M(matrix,1,0);
    m[2] = _M(matrix,2,0);

    m[3] = _M(matrix,0,1);
    m[4] = _M(matrix,1,1);
    m[5] = _M(matrix,2,1);

    m[6] = _M(matrix,0,2);
    m[7] = _M(matrix,1,2);
    m[8] = _M(matrix,2,2);

    gcmFOOTER_NO();
}

/*use row-major order*/
void _vgSetMatrix(_VGMatrix3x3 *matrix, _VGTemplate m00, _VGTemplate m01, _VGTemplate m02,
                                     _VGTemplate m10, _VGTemplate m11, _VGTemplate m12,
                                     _VGTemplate m20, _VGTemplate m21, _VGTemplate m22)
{
    _M(matrix, 0,0) = m00;
    _M(matrix, 0,1) = m01;
    _M(matrix, 0,2) = m02;

    _M(matrix, 1,0) = m10;
    _M(matrix, 1,1) = m11;
    _M(matrix, 1,2) = m12;

    _M(matrix, 2,0) = m20;
    _M(matrix, 2,1) = m21;
    _M(matrix, 2,2) = m22;
}

void MultMatrix(const _VGMatrix3x3 *matrix1, const _VGMatrix3x3 *matrix2,  _VGMatrix3x3 *matrixOut)
{
    gctINT32 i,j;

	gcmHEADER_ARG("matrix=0x%x matrix2=0x%x matrixOut=0x%x", matrix1, matrix2, matrixOut);

    for(i=0;i<3;i++)
    {
        for(j=0;j<3;j++)
        {
            matrixOut->m[i][j] =
            MULTIPLY(matrix1->m[i][0], matrix2->m[0][j]) +
            MULTIPLY(matrix1->m[i][1], matrix2->m[1][j]) +
            MULTIPLY(matrix1->m[i][2], matrix2->m[2][j]);
        }
    }

	gcmFOOTER_NO();
}


void  vgMultMatrix(const VGfloat * m)
{
    _VGMatrix3x3 *matrix, mulMatrix, outMatrix;
	_VGContext* context;
    gcmHEADER_ARG("m=0x%x", m);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGMULTMATRIX, 0);

    OVG_IF_ERROR(!m || !isAligned(m,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    matrix = getCurrentMatrix(context);

    _vgSetMatrix(&mulMatrix, m[0], m[3], m[6],
                           m[1], m[4], m[7],
                           m[2], m[5], m[8]);

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(&mulMatrix);
    }

    MultMatrix(matrix, &mulMatrix, &outMatrix);

    gcoOS_MemCopy(matrix, outMatrix.m, sizeof(outMatrix.m));

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(matrix);
    }

    gcmFOOTER_NO();
}

void MatrixTranslate(_VGMatrix3x3* matrix, _VGTemplate temptx, _VGTemplate tempty)
{
    /* Multiply matrices.

            1       0       tx
            0       1       ty
            0       0       1
    */

	gcmHEADER_ARG("matrix=0x%x temptx=%f tempy=%f", matrix, temptx, tempty);

    _M(matrix, 0,2) += MULTIPLY(_M(matrix, 0,0), temptx)
                     + MULTIPLY(_M(matrix, 0,1), tempty);
    _M(matrix, 1,2) += MULTIPLY(_M(matrix, 1,0), temptx)
                     + MULTIPLY(_M(matrix, 1,1), tempty);

	gcmFOOTER_NO();
}

void  vgTranslate(VGfloat tx, VGfloat ty)
{
    _VGMatrix3x3* matrix;
    _VGTemplate  temptx;
    _VGTemplate  tempty;
	_VGContext* context;

    gcmHEADER_ARG("tx=%f ty=%f",
                   tx, ty);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGTRANSLATE, 0);

    matrix = getCurrentMatrix(context);

    temptx = TEMPLATE(tx);
    tempty = TEMPLATE(ty);

    _M(matrix, 0,2) += MULTIPLY(_M(matrix, 0,0), temptx)
                     + MULTIPLY(_M(matrix, 0,1), tempty);
    _M(matrix, 1,2) += MULTIPLY(_M(matrix, 1,0), temptx)
                     + MULTIPLY(_M(matrix, 1,1), tempty);
    _M(matrix, 2,2) += MULTIPLY(_M(matrix, 2,0), temptx)
                     + MULTIPLY(_M(matrix, 2,1), tempty);

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(matrix);
    }

    gcmFOOTER_NO();
}

void MatrixScale(_VGMatrix3x3 *matrix, _VGTemplate tempsx, _VGTemplate tempsy)
{

	gcmHEADER_ARG("matrix=0x%x tempsx=%f tempsy=%f", matrix, tempsx, tempsy);

    /* Multiply matrices.

            sx      0       0
            0       sy      0
            0       0       1
    */
    _M(matrix, 0,0) = MULTIPLY(_M(matrix, 0,0), tempsx);
    _M(matrix, 0,1) = MULTIPLY(_M(matrix, 0,1), tempsy);

    _M(matrix, 1,0) = MULTIPLY(_M(matrix, 1,0), tempsx);
    _M(matrix, 1,1) = MULTIPLY(_M(matrix, 1,1), tempsy);

	gcmFOOTER_NO();
}

void  vgScale(VGfloat sx, VGfloat sy)
{
    _VGMatrix3x3* matrix;
    _VGTemplate  tempsx;
    _VGTemplate  tempsy;
	_VGContext* context;

    gcmHEADER_ARG("sx=%f sy=%f", sx, sy);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSCALE, 0);

    matrix = getCurrentMatrix(context);

    tempsx = TEMPLATE(sx);
    tempsy = TEMPLATE(sy);

    _M(matrix, 0,0) = MULTIPLY(_M(matrix, 0,0), tempsx);
    _M(matrix, 0,1) = MULTIPLY(_M(matrix, 0,1), tempsy);

    _M(matrix, 1,0) = MULTIPLY(_M(matrix, 1,0), tempsx);
    _M(matrix, 1,1) = MULTIPLY(_M(matrix, 1,1), tempsy);

    _M(matrix, 2,0) = MULTIPLY(_M(matrix, 2,0), tempsx);
    _M(matrix, 2,1) = MULTIPLY(_M(matrix, 2,1), tempsy);

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(matrix);
    }

#if USE_SCAN_LINE
    AllPathDirty(context, VGTessPhase_ALL);
#endif

    gcmFOOTER_NO();
}


void  vgShear(VGfloat shx, VGfloat shy)
{
    _VGMatrix3x3* matrix;
    _VGMatrix3x3  matrixOut;
    _VGTemplate  tempshx;
    _VGTemplate  tempshy;
    _VGMatrix3x3  tempMatrix;
	_VGContext* context;

    gcmHEADER_ARG("shx=%f shy=%f", shx, shy);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSHEAR, 0);

    matrix = getCurrentMatrix(context);

    tempshx = TEMPLATE(shx);
    tempshy = TEMPLATE(shy);


    /* Multiply matrices.

            1       shx     0
            shy     1       0
            0       0       1
    */

    _vgSetMatrix(&tempMatrix,
        1.0f,    tempshx, 0.0f,
        tempshy, 1.0f,    0.0f,
        0.0f,    0.0f,    1.0f);


    MultMatrix(matrix, &tempMatrix, &matrixOut);

    gcoOS_MemCopy(matrix->m, matrixOut.m, sizeof(matrixOut.m));

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(matrix);
    }

    gcmFOOTER_NO();
}

void MatrixRotate(_VGMatrix3x3* matrix, _VGTemplate tempRad)
{
    _VGMatrix3x3  matrixOut;
    _VGMatrix3x3  tempMatrix;

    _VGTemplate   sina;
    _VGTemplate   cosa;

	gcmHEADER_ARG("matrix=0x%x tempRad=%f", matrix, tempRad);

    sina = SIN(tempRad);
    cosa = COS(tempRad);

    _vgSetMatrix(&tempMatrix,
        cosa, -sina, 0.0f,
        sina,  cosa, 0.0f,
        0.0f,  0.0f, 1.0f);

    /* Multiply matrices.

            cosa        -sina       0
            sina        cosa        0
            0           0           1
    */


    MultMatrix(matrix, &tempMatrix, &matrixOut);

    gcoOS_MemCopy(matrix->m, matrixOut.m, sizeof(matrixOut.m));

	gcmFOOTER_NO();
}

void  vgRotate(VGfloat angle)
{
    _VGMatrix3x3* matrix;
    _VGTemplate   tempAngle;
    _VGTemplate   tempRad;
	_VGContext* context;

    gcmHEADER_ARG("angle=%f", angle);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGROTATE, 0);
    matrix = getCurrentMatrix(context);

    tempAngle = TEMPLATE(inputFloat(angle));
    tempRad   = MULTIPLY(PI_DIV_180, tempAngle);

    MatrixRotate(matrix, tempRad);

    if (context->matrixMode != VG_MATRIX_IMAGE_USER_TO_SURFACE)
    {
        ForceAffine(matrix);
    }

#if USE_SCAN_LINE
    AllPathDirty(context, VGTessPhase_ALL);
#endif

    gcmFOOTER_NO();
}


gctINT32 InvertMatrix(_VGMatrix3x3 *m, _VGMatrix3x3 *mout)
{
  gctBOOL affine = isAffine(m);

  VGfloat v0 = m->m[1][1]*m->m[2][2] - m->m[2][1]*m->m[1][2];
  VGfloat v1 = m->m[2][0]*m->m[1][2] - m->m[1][0]*m->m[2][2];
  VGfloat v2 = m->m[1][0]*m->m[2][1] - m->m[2][0]*m->m[1][1];
  VGfloat v = m->m[0][0]*v0 + m->m[0][1]*v1 + m->m[0][2]*v2;

  gcmHEADER_ARG("m=0x%x mout=0x%x", m, mout);

  if( v == 0.0f )
  {
	  mout->m[0][0] = 1.0f;
	  mout->m[1][0] = 0.0f;
	  mout->m[2][0] = 0.0f;
	  mout->m[0][1] = 0.0f;
	  mout->m[1][1] = 1.0f;
	  mout->m[2][1] = 0.0f;
	  mout->m[0][2] = 0.0f;
	  mout->m[1][2] = 0.0f;
	  mout->m[2][2] = 1.0f;
	  gcmFOOTER_ARG("return=%d", 0);

      return 0;
  }
  v = 1.0f / v;

  /* Calculate inverse */
  mout->m[0][1] = v * (m->m[2][1]*m->m[0][2] - m->m[0][1]*m->m[2][2]);
  mout->m[0][0] = v * v0;
  mout->m[2][0] = v * v2;
  mout->m[1][1] = v * (m->m[0][0]*m->m[2][2] - m->m[2][0]*m->m[0][2]);
  mout->m[2][1] = v * (m->m[2][0]*m->m[0][1] - m->m[0][0]*m->m[2][1]);
  mout->m[0][2] = v * (m->m[0][1]*m->m[1][2] - m->m[1][1]*m->m[0][2]);
  mout->m[1][0] = v * v1;
  mout->m[1][2] = v * (m->m[1][0]*m->m[0][2] - m->m[0][0]*m->m[1][2]);
  mout->m[2][2] = v * (m->m[0][0]*m->m[1][1] - m->m[1][0]*m->m[0][1]);

  if (affine)
      ForceAffine(mout);

	  gcmFOOTER_ARG("return=%d", 1);

  return 1;
}

void matrixToGAL(_VGMatrix3x3 *matrix, VGfloat m[16])
{
	gcmHEADER_ARG("matrix=0x%x m=0x%x", matrix, m);

    m[0] = _M(matrix,0,0);
    m[1] = _M(matrix,1,0);
    m[2] =  0.0f;
    m[3] = _M(matrix,2,0);

    m[4] = _M(matrix,0,1);
    m[5] = _M(matrix,1,1);
    m[6] = 0.0f;
    m[7] = _M(matrix,2,1);

    m[8]  = 0.0f;
    m[9]  = 0.0f;
    m[10] = 1.0f;
    m[11] = 0.0f;


    m[12] = _M(matrix,0,2);
    m[13] = _M(matrix,1,2);
    m[14] =  0;
    m[15] = _M(matrix,2,2);

	gcmFOOTER_NO();
}

void _MatrixToGAL(_VGMatrix3x3 *matrix, VGfloat m[16])
{
	gcmHEADER_ARG("matrix=0x%x m=0x%x", matrix, m);

    m[0] = _M(matrix,0,0);
    m[1] = _M(matrix,0,1);
    m[2] =  0.0f;
    m[3] = _M(matrix,0,2);

    m[4] = _M(matrix,1,0);
    m[5] = _M(matrix,1,1);
    m[6] = 0.0f;
    m[7] = _M(matrix,1,2);

    m[8]  = 0.0f;
    m[9]  = 0.0f;
    m[10] = 1.0f;
    m[11] = 0.0f;


    m[12] = _M(matrix,2,0);
    m[13] = _M(matrix,2,1);
    m[14] =  0;
    m[15] = _M(matrix,2,2);

	gcmFOOTER_NO();
}

gctBOOL IsMatrixEqual(_VGMatrix3x3 *matrix1, _VGMatrix3x3 *matrix2)
{
    gctINT32 i,j;

	gcmHEADER_ARG("matrix1=0x%x matrix2=0x%x", matrix1, matrix2);

    for(i=0;i<3;i++)
    {
        for(j=0;j<3;j++)
        {
            if(matrix1->m[i][j] != matrix2->m[i][j])
			{
				gcmFOOTER_ARG("return=%s", "FALSE");
				return gcvFALSE;
			}
        }
    }

	gcmFOOTER_ARG("return=%s", "TRUE");
    return gcvTRUE;
}


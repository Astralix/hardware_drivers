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


/*------------------------------------------------------------------------
 *
 * VGU library for OpenVG 1.0.1 Reference Implementation
 * -----------------------------------------------------
 *
 * Copyright (c) 2007 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and /or associated documentation files
 * (the "Materials "), to deal in the Materials without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Materials,
 * and to permit persons to whom the Materials are furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR
 * THE USE OR OTHER DEALINGS IN THE MATERIALS.
 * -------------------------------------------------------------------*/

#include "gc_vgsh_precomp.h"


static void append(VGPath path, gctINT32 numSegments, const VGubyte* segments, gctINT32 numCoordinates, const VGfloat* coordinates)
{
    gctINT32 i = 0;

    VGPathDatatype datatype;
    VGfloat scale;
    VGfloat bias;

    gcmHEADER_ARG("path=%d numSegments=%d segments=0x%x numCoordinates=%d coordinates=0x%x",
        path, numSegments, segments, numCoordinates, coordinates);

    datatype = (VGPathDatatype)vgGetParameteri(path, VG_PATH_DATATYPE);
    scale = vgGetParameterf(path, VG_PATH_SCALE);
    bias = vgGetParameterf(path, VG_PATH_BIAS);

    gcmASSERT(numCoordinates <= 26);

    switch(datatype)
    {
        case VG_PATH_DATATYPE_S_8:
        {
			_VGint8    data[26] = {0};
            for(i=0;i<numCoordinates;i++)
                data[i] = (_VGint8)gcoMATH_Floor((coordinates[i] - bias) / scale + 0.5f);   /*add 0.5 for correct rounding*/
            vgAppendPathData(path, numSegments, segments, data);
            break;
        }

        case VG_PATH_DATATYPE_S_16:
        {
			_VGint16   data[26] = {0};
            for(i=0;i<numCoordinates;i++)
                data[i] = (_VGint16)gcoMATH_Floor((coordinates[i] - bias) / scale + 0.5f);  /*add 0.5 for correct rounding*/
            vgAppendPathData(path, numSegments, segments, data);
            break;
        }

        case VG_PATH_DATATYPE_S_32:
        {
			gctINT32   data[26] = {0};
            for(i=0;i<numCoordinates;i++)
                data[i] = (gctINT32)gcoMATH_Floor((coordinates[i] - bias) / scale + 0.5f);  /*add 0.5 for correct rounding*/
            vgAppendPathData(path, numSegments, segments, data);
            break;
        }

        default:
        {
			gctFLOAT data[26] = {0};
            gcmASSERT(datatype == VG_PATH_DATATYPE_F);

            for(i=0;i<numCoordinates;i++)
                data[i] = (gctFLOAT)((coordinates[i] - bias) / scale);
            vgAppendPathData(path, numSegments, segments, data);
            break;
        }
    }

    gcmFOOTER_NO();
}


VGUErrorCode   vguLine(VGPath path, VGfloat x0, VGfloat y0, VGfloat x1, VGfloat y1)
{
    VGErrorCode error;
    static const VGubyte segments[2] = {VG_MOVE_TO | VG_ABSOLUTE, VG_LINE_TO | VG_ABSOLUTE};
    VGfloat data[4];

    gcmHEADER_ARG("path=%d x0=%f y0=%f x1=%f y1=%f",
                   path, x0, y0, x1, y1);

    data[0] = x0;
    data[1] = y0;
    data[2] = x1;
    data[3] = y1;

    vgGetError();   /*clear the error state*/
    append(path, 2, segments, 4, data);

    error = vgGetError();
    if(error == VG_BAD_HANDLE_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_HANDLE_ERROR);
        return VGU_BAD_HANDLE_ERROR;
    }
    else if(error == VG_PATH_CAPABILITY_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_PATH_CAPABILITY_ERROR);
        return VGU_PATH_CAPABILITY_ERROR;
    }

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


VGUErrorCode   vguPolygon(VGPath path, const VGfloat * points, VGint count, VGboolean closed)
{
    gctINT32 i;
    VGubyte segments[1] = {VG_MOVE_TO | VG_ABSOLUTE};
    VGfloat data[2];

    VGErrorCode error = vgGetError();   /*clear the error state*/

    gcmHEADER_ARG("path=%d points=0x%x count=%d closed=%s",
                   path, points, count,
                   closed ? "VG_TRUE" : "VG_FALSE");

    if(!points || (( *(_VGuint32*)(VGfloat**)&points) & 3) || count <= 0)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }

    for(i=0;i<count;i++)
    {
        data[0] = points[i*2+0];
        data[1] = points[i*2+1];
        append(path, 1, segments, 2, data);
        segments[0] = VG_LINE_TO | VG_ABSOLUTE;
    }
    if(closed)
    {
        segments[0] = VG_CLOSE_PATH;
        append(path, 1, segments, 0, data);
    }

    error = vgGetError();
    if(error == VG_BAD_HANDLE_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_HANDLE_ERROR);
        return VGU_BAD_HANDLE_ERROR;
    }
    else if(error == VG_PATH_CAPABILITY_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_PATH_CAPABILITY_ERROR);
        return VGU_PATH_CAPABILITY_ERROR;
    }

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


VGUErrorCode   vguRect(VGPath path, VGfloat x, VGfloat y, VGfloat width, VGfloat height)
{
    VGErrorCode error;

    static const VGubyte segments[5] = {VG_MOVE_TO | VG_ABSOLUTE,
                                        VG_HLINE_TO | VG_ABSOLUTE,
                                        VG_VLINE_TO | VG_ABSOLUTE,
                                        VG_HLINE_TO | VG_ABSOLUTE,
                                        VG_CLOSE_PATH};
    VGfloat data[5];

    gcmHEADER_ARG("path=%d x=%f y=%f width=%f height=%f",
                   path, x, y, width, height);

	vgGetError();  /*clear the error state*/
    if(width <= 0 || height <= 0)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }

    data[0] = x;
    data[1] = y;
    data[2] = x + width;
    data[3] = y + height;
    data[4] = x;

    append(path, 5, segments, 5, data);

    error = vgGetError();
    if(error == VG_BAD_HANDLE_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_HANDLE_ERROR);
        return VGU_BAD_HANDLE_ERROR;
    }
    else if(error == VG_PATH_CAPABILITY_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_PATH_CAPABILITY_ERROR);
        return VGU_PATH_CAPABILITY_ERROR;
    }

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


VGUErrorCode   vguRoundRect(VGPath path, VGfloat x, VGfloat y, VGfloat width, VGfloat height, VGfloat arcWidth, VGfloat arcHeight)
{
    VGErrorCode error;
    static const VGubyte segments[10] = {VG_MOVE_TO | VG_ABSOLUTE,
                                         VG_HLINE_TO | VG_ABSOLUTE,
                                         VG_SCCWARC_TO | VG_ABSOLUTE,
                                         VG_VLINE_TO | VG_ABSOLUTE,
                                         VG_SCCWARC_TO | VG_ABSOLUTE,
                                         VG_HLINE_TO | VG_ABSOLUTE,
                                         VG_SCCWARC_TO | VG_ABSOLUTE,
                                         VG_VLINE_TO | VG_ABSOLUTE,
                                         VG_SCCWARC_TO | VG_ABSOLUTE,
                                         VG_CLOSE_PATH};
    VGfloat data[26];

    gcmHEADER_ARG("path=%d x=%f y=%f width=%f height=%f arcWidth=%f arcHeight=%f",
                   path, x, y, width, height, arcWidth, arcHeight);

	vgGetError();   /*clear the error state*/
    if(width <= 0 || height <= 0)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }

    arcWidth = CLAMP(arcWidth, 0.0f, width);
    arcHeight = CLAMP(arcHeight, 0.0f, height);

    data[0] = x + arcWidth/2;
    data[1] = y;
    data[2] = x + width - arcWidth/2;
    data[3] = arcWidth/2;
    data[4] = arcHeight/2;
    data[5] = 0;
    data[6] = x + width;
    data[7] = y + arcHeight/2;
    data[8] = y + height - arcHeight/2;
    data[9] = arcWidth/2;
    data[10] = arcHeight/2;
    data[11] =  0;
    data[12] = x + width - arcWidth/2;
    data[13] = y + height;
    data[14] = x + arcWidth/2;
    data[15] = arcWidth/2;
    data[16] = arcHeight/2;
    data[17] = 0;
    data[18] = x;
    data[19] = y + height - arcHeight/2;
    data[20] = y + arcHeight/2;
    data[21] = arcWidth/2;
    data[22] = arcHeight/2;
    data[23] = 0;
    data[24] = x + arcWidth/2;
    data[25] = y;


    append(path, 10, segments, 26, data);

    error = vgGetError();
    if(error == VG_BAD_HANDLE_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_HANDLE_ERROR);
        return VGU_BAD_HANDLE_ERROR;
    }
    else if(error == VG_PATH_CAPABILITY_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_PATH_CAPABILITY_ERROR);
        return VGU_PATH_CAPABILITY_ERROR;
    }

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


VGUErrorCode   vguEllipse(VGPath path, VGfloat cx, VGfloat cy, VGfloat width, VGfloat height)
{
    VGErrorCode error;

    static const VGubyte segments[4] = {VG_MOVE_TO | VG_ABSOLUTE,
                                        VG_SCCWARC_TO | VG_ABSOLUTE,
                                        VG_SCCWARC_TO | VG_ABSOLUTE,
                                        VG_CLOSE_PATH};
    VGfloat data[12];

    gcmHEADER_ARG("path=%d cx=%f cy=%f width=%f height=%f",
                   path, cx, cy, width, height);

	vgGetError();   /*clear the error state*/
    if(width <= 0 || height <= 0)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }

    data[0]  = cx + width / 2;
    data[1]  = cy;
    data[2]  = width / 2;
    data[3]  = height / 2;
    data[4]  = 0;
    data[5]  = cx - width / 2;
    data[6]  = cy;
    data[7]  = width / 2;
    data[8]  = height / 2;
    data[9]  = 0;
    data[10] = cx + width / 2;
    data[11] = cy;

    append(path, 4, segments, 12, data);

    error = vgGetError();
    if(error == VG_BAD_HANDLE_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_HANDLE_ERROR);
        return VGU_BAD_HANDLE_ERROR;
    }
    else if(error == VG_PATH_CAPABILITY_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_PATH_CAPABILITY_ERROR);
        return VGU_PATH_CAPABILITY_ERROR;
    }

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


static VGfloat Round(VGfloat value, VGfloat r)
{
    gcmHEADER_ARG("value=%f r=%f", value, r);
    gcmFOOTER_ARG("return=%f", ((VGint)(value / r + 0.5f))*r);

    return ((VGint)(value / r + 0.5f))*r;
}
#define TAIL 0.0001f

VGUErrorCode   vguArc(VGPath path, VGfloat x, VGfloat y, VGfloat width, VGfloat height,
                      VGfloat startAngle, VGfloat angleExtent, VGUArcType arcType)
{
    VGfloat w;
    VGfloat h;
    VGfloat a;
    VGubyte segments[1];
    VGfloat data[5];
    VGfloat endAngle;

    VGErrorCode error = vgGetError();   /*clear the error state*/

    gcmHEADER_ARG("path=%d x=%f y=%f width=%f height=%f startAngle=%f angleExtent=%f arcType=0x%x",
                   path, x, y, width, startAngle, angleExtent, arcType);

    if((arcType != VGU_ARC_OPEN && arcType != VGU_ARC_CHORD && arcType != VGU_ARC_PIE) || width <= 0.0f || height <= 0.0f)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }

    startAngle = DEG_TO_RAD(startAngle);
    angleExtent = DEG_TO_RAD(angleExtent);

    w = width/2.0f;
    h = height/2.0f;


    segments[0] = VG_MOVE_TO | VG_ABSOLUTE;
    data[0] = Round(x + w * (VGfloat)gcoMATH_Cosine(startAngle), TAIL);
    data[1] = Round(y + h * (VGfloat)gcoMATH_Sine(startAngle), TAIL);
    append(path, 1, segments, 2, data);

    data[0] = w;
    data[1] = h;
    data[2] = 0.0f;
    endAngle = startAngle + angleExtent;
    if(angleExtent >= 0.0f)
    {
        segments[0] = VG_SCCWARC_TO | VG_ABSOLUTE;
        for( a = startAngle + gcdPI;a < endAngle; a += gcdPI)
        {
            data[3] = Round(x + w * (VGfloat)gcoMATH_Cosine(a), TAIL);
            data[4] = Round(y + h * (VGfloat)gcoMATH_Sine(a), TAIL);
            append(path, 1, segments, 5, data);
        }
    }
    else
    {
        segments[0] = VG_SCWARC_TO | VG_ABSOLUTE;
        for( a = startAngle - gcdPI;a > endAngle; a -= gcdPI)
        {
            data[3] = Round(x + w * (VGfloat)gcoMATH_Cosine(a), TAIL);
            data[4] = Round(y + h * (VGfloat)gcoMATH_Sine(a), TAIL);
            append(path, 1, segments, 5, data);
        }
    }
    data[3] = Round(x + w * (VGfloat)gcoMATH_Cosine(endAngle), TAIL);
    data[4] = Round(y + h * (VGfloat)gcoMATH_Sine(endAngle), TAIL);
    append(path, 1, segments, 5, data);

    if(arcType == VGU_ARC_CHORD)
    {
        segments[0] = VG_CLOSE_PATH;
        append(path, 1, segments, 0, data);
    }
    else if(arcType == VGU_ARC_PIE)
    {
        segments[0] = VG_LINE_TO | VG_ABSOLUTE;
        data[0] = x;
        data[1] = y;
        append(path, 1, segments, 2, data);
        segments[0] = VG_CLOSE_PATH;
        append(path, 1, segments, 0, data);
    }

    error = vgGetError();
    if(error == VG_BAD_HANDLE_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_HANDLE_ERROR);
        return VGU_BAD_HANDLE_ERROR;
    }
    else if(error == VG_PATH_CAPABILITY_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_PATH_CAPABILITY_ERROR);
        return VGU_PATH_CAPABILITY_ERROR;
    }

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}
#undef TAIL

VGUErrorCode   vguComputeWarpQuadToSquare(VGfloat sx0, VGfloat sy0, VGfloat sx1, VGfloat sy1,
                                          VGfloat sx2, VGfloat sy2, VGfloat sx3, VGfloat sy3, VGfloat * matrix)
{

	VGfloat      mat[9] = {0.0f};
    VGUErrorCode ret;
    _VGMatrix3x3 m, mout;
    gctBOOL         nonsingular;

    gcmHEADER_ARG("sx0=%f sy0=%f sx1=%f sy1=%f sx2=%f sy2=%f sx3=%f sy3=%f matrix=0x%x",
                   sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, matrix);

    if(!matrix || (*(_VGuint32*)(void**)&matrix) & 3)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }


    ret = vguComputeWarpSquareToQuad(sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, mat);
    if(ret == VGU_BAD_WARP_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_WARP_ERROR);
        return VGU_BAD_WARP_ERROR;
    }

    _vgSetMatrix(&m, mat[0], mat[3], mat[6],
                 mat[1], mat[4], mat[7],
                 mat[2], mat[5], mat[8]);

    nonsingular = InvertMatrix(&m, &mout);
    if(!nonsingular)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_WARP_ERROR);
        return VGU_BAD_WARP_ERROR;
    }

    matrix[0] = mout.m[0][0];
    matrix[1] = mout.m[1][0];
    matrix[2] = mout.m[2][0];
    matrix[3] = mout.m[0][1];
    matrix[4] = mout.m[1][1];
    matrix[5] = mout.m[2][1];
    matrix[6] = mout.m[0][2];
    matrix[7] = mout.m[1][2];
    matrix[8] = mout.m[2][2];

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


VGUErrorCode   vguComputeWarpSquareToQuad(VGfloat dx0, VGfloat dy0, VGfloat dx1, VGfloat dy1,
                                          VGfloat dx2, VGfloat dy2, VGfloat dx3, VGfloat dy3, VGfloat * matrix)
{

    VGfloat sumx, sumy, diffx1, diffy1, diffx2, diffy2, det;
    VGfloat oodet, g, h;

    gcmHEADER_ARG("dx0=%f dy0=%f dx1=%f dy1=%f dx2=%f dy2=%f dx3=%f dy3=%f matrix=0x%x",
                   dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, matrix);

    if(!matrix || (*(_VGuint32*)(void**)&matrix) & 3)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }


    diffx1 = dx1 - dx3;
    diffy1 = dy1 - dy3;
    diffx2 = dx2 - dx3;
    diffy2 = dy2 - dy3;

    det = diffx1*diffy2 - diffx2*diffy1;

    if(det == 0.0f)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_WARP_ERROR);
        return VGU_BAD_WARP_ERROR;
    }

    sumx = dx0 - dx1 + dx3 - dx2;
    sumy = dy0 - dy1 + dy3 - dy2;

    if(sumx == 0.0f && sumy == 0.0f)
    {   /*affine mapping*/
        matrix[0] = dx1 - dx0;
        matrix[1] = dy1 - dy0;
        matrix[2] = 0.0f;
        matrix[3] = dx3 - dx1;
        matrix[4] = dy3 - dy1;
        matrix[5] = 0.0f;
        matrix[6] = dx0;
        matrix[7] = dy0;
        matrix[8] = 1.0f;
        gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
        return VGU_NO_ERROR;
    }

    oodet = 1.0f / det;
    g = (sumx*diffy2 - diffx2*sumy) * oodet;
    h = (diffx1*sumy - sumx*diffy1) * oodet;

    matrix[0] = dx1-dx0+g*dx1;
    matrix[1] = dy1-dy0+g*dy1;
    matrix[2] = g;
    matrix[3] = dx2-dx0+h*dx2;
    matrix[4] = dy2-dy0+h*dy2;
    matrix[5] = h;
    matrix[6] = dx0;
    matrix[7] = dy0;
    matrix[8] = 1.0f;

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}


VGUErrorCode   vguComputeWarpQuadToQuad(VGfloat dx0, VGfloat dy0, VGfloat dx1, VGfloat dy1,
                                        VGfloat dx2, VGfloat dy2, VGfloat dx3, VGfloat dy3,
                                        VGfloat sx0, VGfloat sy0, VGfloat sx1, VGfloat sy1,
                                        VGfloat sx2, VGfloat sy2, VGfloat sx3, VGfloat sy3, VGfloat * matrix)
{
	VGfloat qtos[9] = {0.0f}, stoq[9] = {0.0f};
    VGUErrorCode ret1, ret2;
    _VGMatrix3x3 m1, m2, r;

    gcmHEADER_ARG("dx0=%f dy0=%f dx1=%f dy1=%f dx2=%f dy2=%f dx3=%f dy3=%f "
                  "sx0=%f sy0=%f sx1=%f sy1=%f sx2=%f sy2=%f sx3=%f sy3=%f matrix=0x%x",
                  dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, matrix);

    if(!matrix || (*(_VGuint32*)(void**)&matrix) & 3)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_ILLEGAL_ARGUMENT_ERROR);
        return VGU_ILLEGAL_ARGUMENT_ERROR;
    }

    ret1 = vguComputeWarpQuadToSquare(sx0, sy0, sx1, sy1, sx2, sy2, sx3, sy3, qtos);
    if(ret1 == VGU_BAD_WARP_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_WARP_ERROR);
        return VGU_BAD_WARP_ERROR;
    }

    ret2 = vguComputeWarpSquareToQuad(dx0, dy0, dx1, dy1, dx2, dy2, dx3, dy3, stoq);
    if(ret2 == VGU_BAD_WARP_ERROR)
    {
        gcmFOOTER_ARG("return=0x%x", VGU_BAD_WARP_ERROR);
        return VGU_BAD_WARP_ERROR;
    }

    _vgSetMatrix(&m1, qtos[0], qtos[3], qtos[6],
                 qtos[1], qtos[4], qtos[7],
                 qtos[2], qtos[5], qtos[8]);

    _vgSetMatrix(&m2, stoq[0], stoq[3], stoq[6],
                 stoq[1], stoq[4], stoq[7],
                 stoq[2], stoq[5], stoq[8]);

    MultMatrix(&m2, &m1, &r);

    matrix[0] = r.m[0][0];
    matrix[1] = r.m[1][0];
    matrix[2] = r.m[2][0];
    matrix[3] = r.m[0][1];
    matrix[4] = r.m[1][1];
    matrix[5] = r.m[2][1];
    matrix[6] = r.m[0][2];
    matrix[7] = r.m[1][2];
    matrix[8] = r.m[2][2];

    gcmFOOTER_ARG("return=0x%x", VGU_NO_ERROR);
    return VGU_NO_ERROR;
}

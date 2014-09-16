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


#ifndef __gc_vgsh_math_h_
#define __gc_vgsh_math_h_

/* Conversion routines. */
#define PI_DIV_180_F            0.017453292519943295769236907684886f

#define PI_MUL_2_F              6.283185307179586476925286766559f
#define PI_DIV_2_F              1.5707963267948966192313216916398f

#define  TEMPLATE(x)          x
#define  _VGTemplate          gctFLOAT
#define  ADD(x1, x2)         (gctFLOAT)gcoMATH_Add(x1, x2)
#define  MULTIPLY(x1, x2)    (gctFLOAT)gcoMATH_Multiply(x1, x2)
#define  DIVIDE(x1, x2)      (gctFLOAT)gcoMATH_Divide(x1, x2)
#define  SIN(a)              (gctFLOAT)gcoMATH_Sine(a)
#define  COS(a)              (gctFLOAT)gcoMATH_Cosine(a)
#define  SQRT(a)             (gctFLOAT)gcoMATH_SquareRoot(a)
#define  ACOS(a)             (gctFLOAT)gcoMATH_ArcCosine(a)
#define  FLOOR(a)            (gctFLOAT)gcoMATH_Floor(a)
#define  ABS(a)              (gctFLOAT)gcoMATH_Absolute(a)
#define  CEIL(a)             (gctFLOAT)gcoMATH_Ceiling(a)
#define  CONSTANT_ONE        1.0f
#define  PI_DIV_180          PI_DIV_180_F

#endif /* __gc_vgsh_math_h_ */

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




#include <math.h>

#define gcoMATH_ModuloInt(X, Y)  (gctINT)((X) % (Y))
#define gcoMATH_ModuloUInt(X, Y) (gctUINT)((X) % (Y))
#define gcoMATH_Modulo(X, Y)     (gctFLOAT)(fmodf((X),(Y)))

#define gcoMATH_Sine(X)       (gctFLOAT)(sin((X)))
#define gcoMATH_Cosine(X)     (gctFLOAT)(cos((X)))
#define gcoMATH_Floor(X)      (gctFLOAT)(floor((X)))
#define gcoMATH_Ceiling(X)    (gctFLOAT)(ceil((X)))
#define gcoMATH_Exp(X)        (gctFLOAT)(exp((X)))
#define gcoMATH_Absolute(X)   (gctFLOAT)(fabsf((X)))
#define gcoMATH_ArcCosine(X)  (gctFLOAT)(acos((X)))
#define gcoMATH_Tangent(X)    (gctFLOAT)(tan((X)))
#define gcoMATH_ArcSine(X)    (gctFLOAT)(asin((X)))
#define gcoMATH_ArcTangent(X) (gctFLOAT)(atan((X)))

#define gcoMATH_Log2(X)        (gctFLOAT)(log((X)) / log(2.0f))

#define gcoMATH_SquareRoot(X) (gctFLOAT)(sqrtf((X)))
#define gcoMATH_ReciprocalSquareRoot(X) (gctFLOAT)(1.0f / sqrtf((X)))

#define gcoMATH_DivideInt(X, Y)    (gctINT)((X) / (Y))
#define gcoMATH_DivideUInt(X, Y)   (gctUINT)((X) / (Y))
#define gcoMATH_DivideUInt64(X, Y) (gctUINT64)((X) / (Y))
#define gcoMATH_Add(X, Y)          (gctFLOAT)((X) + (Y))
#define gcoMATH_Multiply(X, Y)     (gctFLOAT)((X) * (Y))
#define gcoMATH_Divide(X, Y)       (gctFLOAT)((X) / (Y))
#define gcoMATH_Power(X, Y)        (gctFLOAT)(pow((X),(Y)))
#define gcoMATH_DivideFromUInteger(X, Y) (gctFLOAT)(X) / (gctFLOAT)(Y)

#define gcoMATH_UInt2Float(X)  (gctFLOAT)(X)
#define gcoMATH_Int2Float(X)   (gctFLOAT)(X)
#define gcoMATH_Float2UInt(X)  (gctUINT) (X)
#define gcoMATH_Float2Int(X)   (gctINT)  (X)
#define gcoMATH_Float2Int64(X) (gctINT64)  (X)
#define gcoMATH_Fixed2Float(X) (gctFLOAT)((X) / 65536.0f)
#define gcoMATH_Float2NormalizedUInt8(X) (gctUINT8)((X) * 255.0f)

#define gcoMATH_MultiplyFixed(X, Y) \
    (gctFIXED_POINT)( ((gctINT64) (X) * (Y)) >> 16 )

#define gcoMATH_DivideFixed(X, Y) \
    (gctFIXED_POINT)( (((gctINT64) (X)) << 16) / (Y) )

#define gcoMATH_MultiplyDivideFixed(X, Y, Z) \
    (gctFIXED_POINT)( (gctINT64) (X) * (Y) / (Z) )


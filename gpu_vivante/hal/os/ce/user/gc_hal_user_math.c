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




#include "gc_hal_user.h"
#include <math.h>

gctUINT32
gcoMATH_Log2in5dot5(
    IN gctINT X
    )
{
    return (X > 0)
        ? (gctUINT32) (log(X) / log(2.0f) * 32.0f)
        : 0;
}

gctUINT32
gcoMATH_FloatAsUInt(
    IN gctFLOAT X
    )
{
    union __anon_1
    {
        gctFLOAT f;
        gctUINT32 u;
    }
    var;

    var.f = X;
    return var.u;
}

gctFLOAT
gcoMATH_UIntAsFloat(
    IN gctUINT32 X
    )
{
    union __anon_1
    {
        gctFLOAT f;
        gctUINT32 u;
    }
    var;

    var.u = X;
    return var.f;
}

gctBOOL
gcoMATH_CompareEqualF(
    IN gctFLOAT X,
    IN gctFLOAT Y
    )
{
    union __anon_1
    {
        gctFLOAT f;
        gctUINT32 u;
    }
    var1, var2;

    var1.f = X;
    var2.f = Y;

    return (var1.u == var2.u);
}

/* Return result in 16bit UINT format.
 * Don't round up the UINT input.
 */
gctUINT16
gcoMATH_UInt8AsFloat16(
    IN gctUINT8 X
    )
{
    gctUINT16 exp, x16, mask;

    if (X == 0x0)
        return (gctUINT16)0x0;

    x16  = X;
    exp  = 15;
    mask = 0x1 << 8;

    while((x16 & mask) == 0)
    {
        exp--;
        x16 <<= 1;
    }

    x16 = (x16 & 0xFF) << 2;

    return (exp << 10) | x16;
}

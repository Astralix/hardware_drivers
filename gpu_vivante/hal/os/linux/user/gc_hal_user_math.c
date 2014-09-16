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




#include "gc_hal_user_math.h"
#include "gc_hal_user_linux.h"


gctUINT32
gcoMATH_Log2in5dot5(
    IN gctINT X
    )
{
    gctUINT32 res = 0;

    if ( X <= 1 )
    {
        return 0;
    }

    if (!(X & 0xFF))
    {
        X >>= 8;
        res += 8 * 32;
    }
    if (!(X & 0xF))
    {
        X >>= 4;
        res += 4 * 32;
    }
    if (!(X & 0x3))
    {
        X >>= 2;
        res += 2 * 32;
    }
    if (!(X & 0x1))
    {
        X >>= 1;
        res += 32;
    }

    switch(X)
    {
    case 1:
        break;
    case 3:
        /* Return res + log_2(3)*32.f = res + (gctUIN32)50.7188f */
        res += 50;
        break;
    case 5:
        /* Return res + log_2(5)*32.f = res + (gctUIN32)74.301699f */
        res += 74;
        break;
    default:
        /* Return res + log_2(x)*32.f = res + log_e(x) / log_e(2) * 32.f
                                      = res + (gctUIN32)46.166241f
        */
        res += (gctUINT32)(logf(X) * 46.166241f);
    }

    return res;
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

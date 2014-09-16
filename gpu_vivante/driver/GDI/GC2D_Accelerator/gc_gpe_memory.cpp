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



#include "gc_gpe_precomp.h"

gceSTATUS GC2D_Accelerator::CreateGALSurface(
        IN gctUINT Width,
        IN gctUINT Height,
        IN gceSURF_FORMAT Format,
        IN gcePOOL Pool,
        OUT gcoSURF * Surface
        )
{
    return gcoSURF_Construct(mHal,
                             Width,
                             Height,
                             1,
                             gcvSURF_BITMAP,
                             Format,
                             Pool,
                             Surface);
}


gceSTATUS GC2D_Accelerator::DestroyGALSurface(
        IN gcoSURF Surface
        )
{
    return gcoSURF_Destroy(Surface);
}

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




#include "gc_hal_user_hardware_precomp.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

/*******************************************************************************
**
**  gcoHARDWARE_CopyData
**
**  Copy linear data from user memory to video memory.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR Memory
**          Pointer to the gcsSURF_NODE structure that defines the video memory
**          to copy the user data into.
**
**      gctUINT32 Offset
**          Offset into video memory to start copying data into.
**
**      gctCONST_POINTER Buffer
**          Pointer to user data to copy.
**
**      gctSIZE_T Bytes
**          Number of byte to copy.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoHARDWARE_CopyData(
    IN gcsSURF_NODE_PTR Memory,
    IN gctUINT32 Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory=0x%x Offset=%u Buffer=0x%x Bytes=%d",
                  Memory, Offset, Buffer, Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Memory != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    do
    {
        /* Verify that the surface is locked. */
        gcmVERIFY_NODE_LOCK(Memory);

        /* Copy the memory using the CPU. */
        gcmERR_BREAK(gcoOS_MemCopy(Memory->logical + Offset, Buffer, Bytes));

#ifndef __QNXNTO__
        /* Flush the CPU cache. */
        gcmERR_BREAK(gcoSURF_NODE_Cache(Memory,
                                      Memory->logical + Offset,
                                      Bytes,
                                      gcvCACHE_CLEAN));
#endif
    }
    while (gcvFALSE);

    /* Return result. */
    gcmFOOTER();
    return status;
}


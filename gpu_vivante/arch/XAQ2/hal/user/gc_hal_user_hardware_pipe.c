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

/******************************************************************************\
****************************** gcoHARDWARE API Code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_SelectPipe
**
**  Select the current pipe for this hardare context.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an AQHARWDARE object.
**
**      gcePIPE_SELECT Pipe
**          Pipe to select.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_SelectPipe(
    IN gcoHARDWARE Hardware,
    IN gcePIPE_SELECT Pipe
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Pipe=%d", Hardware, Pipe);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Is 2D pipe present? */
    if ((Pipe == gcvPIPE_2D) && !Hardware->hw2DEngine)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Don't do anything if the pipe has already been selected. */
    if (Hardware->currentPipe != Pipe)
    {
#if !defined(VIVANTE_NO_3D)
        /* Set new pipe. */
        Hardware->currentPipe = Pipe;

        /* Flush current pipe. */
        gcmONERROR(gcoHARDWARE_FlushPipe());

        /* Send sempahore and stall. */
        gcmONERROR(gcoHARDWARE_Semaphore(
            gcvWHERE_COMMAND, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE_STALL
            ));

        /* Perform a load state. */
        gcmONERROR(gcoHARDWARE_LoadState32(
            Hardware,
            0x03800,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (Pipe) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            ));
#else
        /* Not supposed to be pipe switching in NO_3D driver. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
#endif
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}


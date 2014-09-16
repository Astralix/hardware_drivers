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

#ifndef VIVANTE_NO_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

/******************************************************************************\
****************************** gcoHARDWARE API Code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_ClearRect
**
**  Append a command buffer with a CLEAR command.
**
**  INPUT:
**
**      gctUINT32 Address
**          Base address of surface to clear.
**
**      gctPOINTER Memory
**          Base address of surface to clear.
**
**      gctUINT32 Stride
**          Stride of surface.
**
**      gctINT32 Left
**          Left coordinate of rectangle to clear.
**
**      gctINT32 Top
**          Top coordinate of rectangle to clear.
**
**      gctINT32 Right
**          Right coordinate of rectangle to clear.
**
**      gctINT32 Bottom
**          Bottom coordinate of rectangle to clear.
**
**      gceSURF_FORMAT Format
**          Format of surface to clear.
**
**      gctUINT32 ClearValue
**          Value to be used for clearing the surface.
**
**      gctUINT8 ClearMask
**          Byte-mask to be used for clearing the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ClearRect(
    IN gctUINT32 Address,
    IN gctPOINTER Memory,
    IN gctUINT32 Stride,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ClearValue,
	IN gctUINT8 ClearMask,
	IN gctUINT32 AlignedWidth,
	IN gctUINT32 AlignedHeight
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gctINT32 tileWidth, tileHeight;

	gcmHEADER_ARG("Address=0x%08x Memory=0x%x Stride=%u Left=%d "
				  "Top=%d Right=%d Bottom=%d Format=%d ClearValue=0x%08x "
				  "ClearMask=0x%02x AlignedWidth=%u AlignedHeight=%u",
				  Address, Memory, Stride, Left, Top, Right, Bottom,
				  Format, ClearValue, ClearMask, AlignedWidth, AlignedHeight);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Query the tile size. */
    /* Supertiled destination? */
    if ((Stride & 0x80000000U) != 0)
    {
        /* Set the tile size. */
        tileWidth  = 64;
        tileHeight = 64;
    }

    /* Not supertiled. */
    else
    {
        /* Query the tile size. */
        gcmONERROR(gcoHARDWARE_QueryTileSize(gcvNULL, gcvNULL,
                                             &tileWidth, &tileHeight,
                                             gcvNULL));
    }

    if (hardware->pixelPipes == 1)
    {
        /* Remove multipipe flag if only 1 pipe exists. */
        Stride &= ~0x40000000U;
    }

    /* Try hardware clear. */
    status = gcoHARDWARE_Clear(Address,
                               Stride,
                               Left,
                               Top,
                               Right,
                               Bottom,
                               Format,
                               ClearValue,
                               ClearMask,
							   AlignedWidth,
							   AlignedHeight);

    if (gcmIS_ERROR(status))
    {
        /* Subdivide rectangular region into sides (sw clear) and center tile aligned piece (hw clear). */
        gctINT32 centerLeft, centerRight, centerTop, centerBottom;
        gctBOOL stall = gcvTRUE;

        tileHeight *= hardware->pixelPipes;
        tileWidth  *= hardware->pixelPipes;

        centerTop = gcmALIGN(Top, tileHeight);
        if (Bottom % tileHeight )
            centerBottom = gcmALIGN(Bottom, tileHeight) - tileHeight;
        else
            centerBottom = Bottom;

        centerLeft = gcmALIGN(Left, tileWidth);
        if (Right % tileWidth )
            centerRight = gcmALIGN(Right, tileWidth) - tileWidth;
        else
            centerRight = Right;

        if ((centerBottom > centerTop) && (centerRight > centerLeft) )
        {
            /* Try hardware clear. */
            status = gcoHARDWARE_Clear(Address,
                                       Stride,
                                       centerLeft,
                                       centerTop,
                                       centerRight,
                                       centerBottom,
                                       Format,
                                       ClearValue,
                                       ClearMask,
							           AlignedWidth,
							           AlignedHeight);

            if (gcmIS_ERROR(status))
            {
                /* Use software clear. */
                gcmONERROR(gcoHARDWARE_ClearSoftware(hardware,
                                                     Memory,
                                                     Stride,
                                                     centerLeft,
                                                     centerTop,
                                                     centerRight,
                                                     centerBottom,
                                                     Format,
                                                     ClearValue,
                                                     ClearMask,
                                                     stall,
                                                     AlignedHeight));

                stall = gcvFALSE;
            }

            if (centerTop > Top )
            {
                /* Use software clear. */
                gcmONERROR(gcoHARDWARE_ClearSoftware(hardware,
                                                     Memory,
                                                     Stride,
                                                     Left,
                                                     Top,
                                                     Right,
                                                     centerTop,
                                                     Format,
                                                     ClearValue,
                                                     ClearMask,
                                                     stall,
                                                     AlignedHeight));
                stall = gcvFALSE;
            }

            if (Bottom > centerBottom )
            {
                /* Use software clear. */
                gcmONERROR(gcoHARDWARE_ClearSoftware(hardware,
                                                     Memory,
                                                     Stride,
                                                     Left,
                                                     centerBottom,
                                                     Right,
                                                     Bottom,
                                                     Format,
                                                     ClearValue,
                                                     ClearMask,
                                                     stall,
                                                     AlignedHeight));

                stall = gcvFALSE;
            }

            if (Left < centerLeft )
            {
                /* Use software clear. */
                gcmONERROR(gcoHARDWARE_ClearSoftware(hardware,
                                                     Memory,
                                                     Stride,
                                                     Left,
                                                     centerTop,
                                                     centerLeft,
                                                     centerBottom,
                                                     Format,
                                                     ClearValue,
                                                     ClearMask,
                                                     stall,
                                                     AlignedHeight));

                stall = gcvFALSE;
            }

            if (centerRight < Right )
            {
                /* Use software clear. */
                gcmONERROR(gcoHARDWARE_ClearSoftware(hardware,
                                                     Memory,
                                                     Stride,
                                                     centerRight,
                                                     centerTop,
                                                     Right,
                                                     centerBottom,
                                                     Format,
                                                     ClearValue,
                                                     ClearMask,
                                                     stall,
                                                     AlignedHeight));

                stall = gcvFALSE;
            }
        }
        else
        {
            /* Use software clear. */
            gcmONERROR(gcoHARDWARE_ClearSoftware(hardware,
                                                 Memory,
                                                 Stride,
                                                 Left,
                                                 Top,
                                                 Right,
                                                 Bottom,
                                                 Format,
                                                 ClearValue,
                                                 ClearMask,
                                                 gcvTRUE,
                                                 AlignedHeight));
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Clear
**
**  Append a command buffer with a CLEAR command.
**
**  INPUT:
**
**      gctUINT32 Address
**          Base address of surface to clear.
**
**      gctUINT32 Stride
**          Stride of surface.
**
**      gctINT32 Left
**          Left coordinate of rectangle to clear.
**
**      gctINT32 Top
**          Top coordinate of rectangle to clear.
**
**      gctINT32 Right
**          Right coordinate of rectangle to clear.
**
**      gctINT32 Bottom
**          Bottom coordinate of rectangle to clear.
**
**      gceSURF_FORMAT Format
**          Format of surface to clear.
**
**      gctUINT32 ClearValue
**          Value to be used for clearing the surface.
**
**      gctUINT8 ClearMask
**          Byte-mask to be used for clearing the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Clear(
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ClearValue,
	IN gctUINT8 ClearMask,
	IN gctUINT32 AlignedWidth,
	IN gctUINT32 AlignedHeight
    )
{
    gceSTATUS status;
    gctUINT32 format, swizzle, isYUVformat;
    gceTILING tiling;
    gctINT32 tileWidth, tileHeight;
    gctUINT32 leftMask, topMask;
    gctUINT32 stride;
    gcsPOINT rectSize;
    gcoHARDWARE hardware;

	gcmHEADER_ARG("Address=0x%08x Stride=%u Left=%d Top=%d "
				  "Right=%d Bottom=%d Format=%d ClearValue=0x%08x "
				  "ClearMask=0x%02x AlignedWidth=%u AlignedHeight=%u",
				  Address, Stride, Left, Top, Right, Bottom, Format,
				  ClearValue, ClearMask, AlignedWidth, AlignedHeight);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    switch (Format)
    {
    case gcvSURF_X4R4G4B4:
    case gcvSURF_X1R5G5B5:
    case gcvSURF_R5G6B5:
    case gcvSURF_X4B4G4R4:
    case gcvSURF_X1B5G5R5:
        if (ClearMask == 0x7)
        {
            /* When the format has no alpha channel, fake the ClearMask to
            ** include alpha channel clearing.   This will allow us to use
            ** resolve clear. */
            ClearMask = 0xF;
        }
        break;

    default:
        break;
    }

    if ((ClearMask != 0xF)
    &&  (Format != gcvSURF_X8R8G8B8)
    &&  (Format != gcvSURF_A8R8G8B8)
    &&  (Format != gcvSURF_D24S8)
    &&  (Format != gcvSURF_D24X8)
    )
    {
        /* Don't clear with mask when channels are not byte sized. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (hardware->pixelPipes < 2)
    {
        /* Remove multi-pipe bit if we only have 1 pixel pipe. */
        Stride &= ~0x40000000U;
    }

    /* Supertiled destination? */
    if ((Stride & 0x80000000U) != 0)
    {
        /* Set the tiling mode. */
        if ((Stride & 0x40000000U) != 0)
        {
            tiling = gcvMULTI_SUPERTILED;

            /* Multi super tiled, need 2x1 super tile aligned. */
            tileWidth  = 64;
            tileHeight = 128;
        }
        else
        {
            tiling = gcvSUPERTILED;

            /* Set the tile size. */
            tileWidth  = 64;
            tileHeight = 64;
        }
    }

    /* Not supertiled. */
    else
    {
        /* Set the tiling mode. */
        if ((Stride & 0x40000000U) != 0)
        {
            tiling = gcvMULTI_TILED;

            /* when multi tile the tile size is 8x4 and the tile is interleaved */
	        /* So it needs 4 8x4 tile align*/
            tileWidth  = 16;
            tileHeight = 8;
        }
        else
        {
            tiling = gcvTILED;

            /* Query the tile size. */
            gcmONERROR(gcoHARDWARE_QueryTileSize(gcvNULL, gcvNULL,
                                                 &tileWidth, &tileHeight,
                                                 gcvNULL));
        }
    }

    /* All sides must be tile aligned. */
    leftMask = tileWidth - 1;
    topMask  = tileHeight - 1;

    /* Convert the format. */
    gcmONERROR(gcoHARDWARE_TranslateDestinationFormat(Format,
                                                      &format,
                                                      &swizzle,
                                                      &isYUVformat));

  	rectSize.x = Right  - Left;
	rectSize.y = Bottom - Top;

    /* For resolve clear, we need to be 4x1 tile aligned. */
    if (((Left           & leftMask)   == 0)
    &&  ((Top            & topMask)    == 0)
	&&  ((rectSize.x & (hardware->resolveAlignmentX - 1)) == 0)
	&&  ((rectSize.y & (hardware->resolveAlignmentY - 1)) == 0)
    )
    {
        gctUINT32 config, control;
        gctUINT32 dither[2] = { ~0U, ~0U };
        gctUINT32 offset, address, bitsPerPixel;

        /* Set up the starting address of clear rectangle. */
        gcmONERROR(
            gcoHARDWARE_ConvertFormat(Format,
                                      &bitsPerPixel,
                                      gcvNULL));

        /* Compute the origin offset. */
        gcmONERROR(
            gcoHARDWARE_ComputeOffset(Left, Top,
                                      Stride & ~0xC0000000U,
                                      bitsPerPixel / 8,
                                      tiling, &offset));

        /* Determine the starting address. */
        address = Address + offset;

        /* Build AQRsConfig register. */
        config = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)));

        /* Build AQRsClearControl register. */
        control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (ClearMask) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)));

        /* Determine the stride. */
        stride = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:0) - (0 ? 17:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:0) - (0 ? 17:0) + 1))))))) << (0 ? 17:0))) | (((gctUINT32) ((gctUINT32) ((Stride <<  2)) & ((gctUINT32) ((((1 ? 17:0) - (0 ? 17:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:0) - (0 ? 17:0) + 1))))))) << (0 ? 17:0)))
               | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) ((Stride >> 31)) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)))
			   | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) ((Stride >> 30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
               ;

        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

        /* Flush cache. */
        gcmONERROR(gcoHARDWARE_FlushPipe());

        /* Program registers. */
        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x01604,
                                           config));

        gcmONERROR(gcoHARDWARE_LoadState(0x01630,
                                         2, dither));

        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x01614,
                                           stride));

        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x01640,
                                           ClearValue));

        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x0163C,
                                           control));

        /* Append new configuration register. */
        gcmONERROR(
            gcoHARDWARE_LoadState32(hardware,
                                    0x016A0,
                                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))));

		if (hardware->pixelPipes == 2)
		{
            /* Extra pages needed to offset sub-buffers to different banks. */
            gctUINT32 bankOffsetBytes = 0;
		    gctUINT32 dstAddress[2];
            gctUINT32 topBufferSize = gcmALIGN(AlignedHeight / 2,
                                      (Stride >> 31) ? 64 : 4)
                                      * (Stride & ~0xC0000000U);
#if gcdENABLE_BANK_ALIGNMENT
            gceSURF_TYPE type = gcvSURF_RENDER_TARGET;


            if ((Format == gcvSURF_D16)
              ||(Format == gcvSURF_D24S8)
              ||(Format == gcvSURF_D32)
              ||(Format == gcvSURF_D24X8))
            {
                type = gcvSURF_DEPTH;
            }
            gcmONERROR(
                    gcoSURF_GetBankOffsetBytes(gcvNULL,
                        type,
                        topBufferSize,
                        &bankOffsetBytes));
            /* TODO: If surface doesn't have enough padding, then don't offset it. */
#endif

            dstAddress[0] = address;
            dstAddress[1] = address
                          + bankOffsetBytes
                          + topBufferSize;

			gcmONERROR(
				gcoHARDWARE_LoadState(0x016E0,
									  2,
									  dstAddress));
		}
		else
		{
			gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                               0x01610,
                                               address));
        }

        gcmONERROR(gcoHARDWARE_ProgramResolve(hardware,
                                              rectSize));

        if (((Format == gcvSURF_D16)
            || (Format == gcvSURF_D24S8)
            || (Format == gcvSURF_D24X8)
            )
            &&  hardware->earlyDepth
        )
        {
            /* Make raster wait for clear to be done. */
            gcmONERROR(gcoHARDWARE_Semaphore(gcvWHERE_RASTER,
                                             gcvWHERE_PIXEL,
                                             gcvHOW_SEMAPHORE));
        }

        /* Target has dirty pixels. */
        hardware->colorStates.dirty = gcvTRUE;
    }
    else
    {
        /* Removed 2D clear as it does not work for tiled buffers. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
_ClearSoftware(
    IN gctPOINTER Address1,
    IN gctPOINTER Address2,
    IN gctUINT32 Stride,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gctUINT32 channelMask[4],
    IN gctUINT32 ClearValue,
    IN gctUINT8 ClearMask,
    IN gctUINT32 bitsPerPixel
    )
{
    gctINT32 x, y;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 offset, tLeft;
    gctBOOL MultiPipe = gcvFALSE;
    gceTILING tiling;

    gcmASSERT(Top < Bottom);
    gcmASSERT(Left < Right);

    /* Multitiled destination? */
    if ((Stride & 0x40000000U) != 0)
    {
        /* Remove the multitiled bit. */
        Stride &= ~0x40000000U;

        /* Set the MultiPipe mode. */
        MultiPipe = gcvTRUE;
    }

    /* Supertiled destination? */
    if ((Stride & 0x80000000U) != 0)
    {
        /* Set the tiling mode. */
        tiling = gcvSUPERTILED;

        /* Remove the supertiled bit. */
        Stride &= ~0x80000000U;
    }

    /* No, query tile size. */
    else
    {
        /* Set the tiling mode. */
        tiling = gcvTILED;
    }

    tLeft = (bitsPerPixel == 16) ? gcmALIGN(Left - 1, 2) : Left;

    for (y = Top;  y < Bottom; y++)
    {
        for (x = tLeft; x < Right; x++)
        {
            gctUINT8_PTR address;
            gctUINT32 tx, ty;

            if (MultiPipe)
            {
                gctUINT32 oddTile = ((x >> 3) & 0x1) ^ ((y >> 2) & 0x1);

                if (oddTile == 0)
                {
                    address = (gctUINT8_PTR) Address1;
                }
                else
                {
                    address = (gctUINT8_PTR) Address2;
                }

                tx = (x & 0xFFFFFFF7) | (((y >> 2) & 0x1) << 3);
                ty = (y & 0x3) | ((y >> 3) << 2);
            }
            else
            {
                tx = x;
                ty = y;
                address = (gctUINT8_PTR) Address1;
            }

            /* Compute the origin offset. */
            gcmONERROR(
                gcoHARDWARE_ComputeOffset(tx, ty,
                                          Stride,
                                          bitsPerPixel / 8,
                                          tiling, &offset));

            address += offset;

            /* Draw only if x,y within clear rectangle. */
            if (bitsPerPixel == 32)
            {
                switch (ClearMask)
                {
                case 0x1:
                    /* Common: Clear stencil only. */
                    * address = (gctUINT8) ClearValue;
                    break;

                case 0xE:
                    /* Common: Clear depth only. */
                                       address[1] = (gctUINT8)  (ClearValue >> 8);
                    * (gctUINT16_PTR) &address[2] = (gctUINT16) (ClearValue >> 16);
                    break;

                case 0xF:
                    /* Common: Clear everything. */
                    * (gctUINT32_PTR) address = ClearValue;
                    break;

                default:
                    if (ClearMask & 0x1)
                    {
                        /* Clear byte 0. */
                        address[0] = (gctUINT8) ClearValue;
                    }

                    if (ClearMask & 0x2)
                    {
                        /* Clear byte 1. */
                        address[1] = (gctUINT8) (ClearValue >> 8);
                    }

                    if (ClearMask & 0x4)
                    {
                        /* Clear byte 2. */
                        address[2] = (gctUINT8) (ClearValue >> 16);
                    }

                    if (ClearMask & 0x8)
                    {
                        /* Clear byte 3. */
                        address[3] = (gctUINT8) (ClearValue >> 24);
                    }
                }
            }

            else if (bitsPerPixel == 16)
            {
                if ((x + 1) == Right)
                {
                    /* Dont write on Right pixel. */
                    channelMask[0] = channelMask[0] & 0x0000FFFF;
                    channelMask[1] = channelMask[1] & 0x0000FFFF;
                    channelMask[2] = channelMask[2] & 0x0000FFFF;
                    channelMask[3] = channelMask[3] & 0x0000FFFF;
                }

                if ((x + 1) == Left)
                {
                    /* Dont write on Left pixel. */
                    channelMask[0] = channelMask[0] & 0xFFFF0000;
                    channelMask[1] = channelMask[1] & 0xFFFF0000;
                    channelMask[2] = channelMask[2] & 0xFFFF0000;
                    channelMask[3] = channelMask[3] & 0xFFFF0000;
                }

                if (ClearMask & 0x1)
                {
                    /* Clear byte 0. */
                    *(gctUINT32_PTR) address = (*(gctUINT32_PTR) address & ~channelMask[0])
                                             | (ClearValue &  channelMask[0]);
                }

                if (ClearMask & 0x2)
                {
                    /* Clear byte 1. */
                    *(gctUINT32_PTR) address = (*(gctUINT32_PTR) address & ~channelMask[1])
                                             | (ClearValue &  channelMask[1]);
                }

                if (ClearMask & 0x4)
                {
                    /* Clear byte 2. */
                    *(gctUINT32_PTR) address = (*(gctUINT32_PTR) address & ~channelMask[2])
                                             | (ClearValue &  channelMask[2]);
                }

                if (ClearMask & 0x8)
                {
                    /* Clear byte 3. */
                    *(gctUINT32_PTR) address = (*(gctUINT32_PTR) address & ~channelMask[3])
                                             | (ClearValue &  channelMask[3]);
                }

                if ((x + 1) == Left)
                {
                    /* Restore channel mask. */
                    channelMask[0] = channelMask[0] | (channelMask[0] >> 16);
                    channelMask[1] = channelMask[1] | (channelMask[1] >> 16);
                    channelMask[2] = channelMask[2] | (channelMask[2] >> 16);
                    channelMask[3] = channelMask[3] | (channelMask[3] >> 16);
                }

                if ((x + 1) == Right)
                {
                    /* Restore channel mask. */
                    channelMask[0] = channelMask[0] | (channelMask[0] << 16);
                    channelMask[1] = channelMask[1] | (channelMask[1] << 16);
                    channelMask[2] = channelMask[2] | (channelMask[2] << 16);
                    channelMask[3] = channelMask[3] | (channelMask[3] << 16);
                }

		/* 16bpp pixels clear 1 extra pixel at a time. */
		x++;
            }
        }
    }

OnError:
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ClearSoftware
**
**  Clear the buffer with software implementation. Buffer is assumed to be
**  tiled.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctPOINTER LogicalAddress
**          Base address of surface to clear.
**
**      gctUINT32 Stride
**          Stride of surface.
**
**      gctINT32 Left
**          Left coordinate of rectangle to clear.
**
**      gctINT32 Top
**          Top coordinate of rectangle to clear.
**
**      gctINT32 Right
**          Right coordinate of rectangle to clear.
**
**      gctINT32 Bottom
**          Bottom coordinate of rectangle to clear.
**
**      gceSURF_FORMAT Format
**          Format of surface to clear.
**
**      gctUINT32 ClearValue
**          Value to be used for clearing the surface.
**
**      gctUINT8 ClearMask
**          Byte-mask to be used for clearing the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ClearSoftware(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER Address,
    IN gctUINT32 Stride,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ClearValue,
    IN gctUINT8 ClearMask,
    IN gctBOOL Stall,
	IN gctUINT32 AlignedHeight
    )
{
    gceSTATUS status;
    gctUINT32 bitsPerPixel;
    gctUINT32 channelMask[4];
    static gctBOOL printed = gcvFALSE;

    gcmHEADER_ARG("Hardware=0x%x Address=%x Stride=%d "
                  "Left=%d Top=%d Right=%d Bottom=%d "
                  "Format=%d ClearValue=%d ClearMask=%d",
                  Hardware, Address, Stride,
                  Left, Top, Right, Bottom,
                  Format, ClearValue, ClearMask);

    /* For a clear that is not tile aligned, our hardware might not be able to
       do it.  So here is the software implementation. */
    gcmGETHARDWARE(Hardware);

    /* Verify the arguements. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (!printed)
    {
        printed = gcvTRUE;

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
                      "%s: Performing a software clear!",
                      __FUNCTION__);
    }

    if (Stall)
    {
        /* Flush the pipe. */
        gcmONERROR(gcoHARDWARE_FlushPipe());

        /* Commit the command queue. */
        gcmONERROR(gcoHARDWARE_Commit());

        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall());
    }

    /* Query pixel depth. */
    gcmONERROR(gcoHARDWARE_ConvertFormat(Format,
                                         &bitsPerPixel,
                                         gcvNULL));

    switch (Format)
    {
    case gcvSURF_X4R4G4B4: /* 12-bit RGB color without alpha channel. */
        channelMask[0] = 0x000F;
        channelMask[1] = 0x00F0;
        channelMask[2] = 0x0F00;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_D16:      /* 16-bit Depth. */
    case gcvSURF_A4R4G4B4: /* 12-bit RGB color with alpha channel. */
        channelMask[0] = 0x000F;
        channelMask[1] = 0x00F0;
        channelMask[2] = 0x0F00;
        channelMask[3] = 0xF000;
        break;

    case gcvSURF_X1R5G5B5: /* 15-bit RGB color without alpha channel. */
        channelMask[0] = 0x001F;
        channelMask[1] = 0x03E0;
        channelMask[2] = 0x7C00;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_A1R5G5B5: /* 15-bit RGB color with alpha channel. */
        channelMask[0] = 0x001F;
        channelMask[1] = 0x03E0;
        channelMask[2] = 0x7C00;
        channelMask[3] = 0x8000;
        break;

    case gcvSURF_R5G6B5: /* 16-bit RGB color without alpha channel. */
        channelMask[0] = 0x001F;
        channelMask[1] = 0x07E0;
        channelMask[2] = 0xF800;
        channelMask[3] = 0x0;
        break;

    case gcvSURF_D24S8:    /* 24-bit Depth with 8 bit Stencil. */
    case gcvSURF_D24X8:    /* 24-bit Depth with 8 bit Stencil. */
    case gcvSURF_X8R8G8B8: /* 24-bit RGB without alpha channel. */
    case gcvSURF_A8R8G8B8: /* 24-bit RGB with alpha channel. */
        channelMask[0] = 0x000000FF;
        channelMask[1] = 0x0000FF00;
        channelMask[2] = 0x00FF0000;
        channelMask[3] = 0xFF000000;
        break;

    default:
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER();
        return status;
    }

    /* Expand 16-bit mask into 32-bit mask. */
    if (bitsPerPixel == 16)
    {
        channelMask[0] = channelMask[0] | (channelMask[0] << 16);
        channelMask[1] = channelMask[1] | (channelMask[1] << 16);
        channelMask[2] = channelMask[2] | (channelMask[2] << 16);
        channelMask[3] = channelMask[3] | (channelMask[3] << 16);
    }

    if (Hardware->pixelPipes > 1)
    {
        gctUINT32 bankOffsetBytes = 0;
        gctPOINTER Address2;
        gctUINT32 topBufferSize = gcmALIGN(AlignedHeight / 2,
                                  (Stride >> 31) ? 64 : 4)
                                  * (Stride & ~0xC0000000U);
#if gcdENABLE_BANK_ALIGNMENT
        gceSURF_TYPE type = gcvSURF_RENDER_TARGET;

        if ((Format == gcvSURF_D16)
          ||(Format == gcvSURF_D24S8)
          ||(Format == gcvSURF_D32)
          ||(Format == gcvSURF_D24X8))
        {
            type = gcvSURF_DEPTH;
        }

        gcmONERROR(
                gcoSURF_GetBankOffsetBytes(gcvNULL,
                    type,
                    topBufferSize,
                    &bankOffsetBytes
                    ));

        /* TODO: If surface doesn't have enough padding, then don't offset it. */
#endif

        Address2 = (gctPOINTER)((gctUINT32)Address
                            + bankOffsetBytes
                            + topBufferSize);

        gcmONERROR(
            _ClearSoftware(Address, Address2, Stride,
                           Left, Top, Right, Bottom,
                           channelMask, ClearValue, ClearMask,
                           bitsPerPixel));
    }
    else
    {
        gcmONERROR(
            _ClearSoftware(Address, Address, Stride,
                           Left, Top, Right, Bottom,
                           channelMask, ClearValue, ClearMask,
                           bitsPerPixel));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ClearTileStatus
**
**  Append a command buffer with a CLEAR TILE STATUS command.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR Surface
**          Pointer of the surface to clear.
**
**      gctUINT32 Address
**          Base address of tile status to clear.
**
**      gceSURF_TYPE Type
**          Type of surface to clear.
**
**      gctUINT32 ClearValue
**          Value to be used for clearing the surface.
**
**      gctUINT8 ClearMask
**          Byte-mask to be used for clearing the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ClearTileStatus(
    IN gcsSURF_INFO_PTR Surface,
    IN gctUINT32 Address,
    IN gctSIZE_T Bytes,
    IN gceSURF_TYPE Type,
    IN gctUINT32 ClearValue,
    IN gctUINT8 ClearMask
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gctSIZE_T bytes;
    gcsPOINT rectSize;
    gctUINT32 config, control;
    gctUINT32 dither[2] = { ~0U, ~0U };
    gctUINT32 clearAddress = 0;
    gctUINT32 fillColor;

    gcmHEADER_ARG("Surface=0x%x Address=0x%x "
                    "Bytes=%d Type=%d ClearValue=%d ClearMask=%d",
                    Surface, Address,
                    Bytes, Type, ClearValue, ClearMask);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    if (ClearMask != 0xF)
    {
        /* Only supported when all bytes are to be cleared. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Query the tile status size. */
    gcmONERROR(
        gcoHARDWARE_QueryTileStatus(Surface->alignedWidth,
                                    Surface->alignedHeight,
                                    Surface->size,
                                    &bytes,
                                    gcvNULL,
                                    &fillColor));

    if (Bytes > 0)
    {
        bytes = Bytes;
    }

    /* Compute the hw specific clear window. */
    gcmONERROR(
        gcoHARDWARE_ComputeClearWindow(bytes,
                                       (gctUINT32_PTR)&rectSize.x,
                                       (gctUINT32_PTR)&rectSize.y));

    switch (Type)
    {
    case gcvSURF_RENDER_TARGET:
        clearAddress        = 0x01660;
        Surface->clearValue = ClearValue;
        break;

    case gcvSURF_DEPTH:
        clearAddress        = 0x0166C;
        Surface->clearValue = ClearValue;
        break;

    case gcvSURF_HIERARCHICAL_DEPTH:
        clearAddress          = 0x016A8;
        Surface->clearValueHz = ClearValue;
        break;

    default:
        /* Surface type is not supported. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Build AQRsConfig register. */
    config = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)));

    /* Build AQRsClearControl register. */
    control = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)));

    /* Switch to 3D pipe. */
    gcmONERROR(
        gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Flush cache. */
    gcmONERROR(
        gcoHARDWARE_FlushPipe());

    /* Flush tile status cache. */
    gcmONERROR(
        gcoHARDWARE_FlushTileStatus(Surface, gcvFALSE));

    /* !!!! HACK-O-MATIC !!!
        GC500 seems to lock up inside the clear when tile status is enabled
        for render targets. The following patch will fix it for now - until
        we figure out the true reason.
    */
    gcmONERROR(
        gcoHARDWARE_Semaphore(gcvWHERE_RASTER,
                              gcvWHERE_PIXEL,
                              gcvHOW_SEMAPHORE_STALL));

    /* Program registers. */
    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware,
                                0x01604,
                                config));

    gcmONERROR(
        gcoHARDWARE_LoadState(0x01630,
                              2, dither));

	if (hardware->pixelPipes == 2)
	{
        gctUINT32 address[2];

        address[0] = Address;
        address[1] = Address
				   + (bytes / 2);

		gcmONERROR(
			gcoHARDWARE_LoadState(0x016E0,
                                  2,
								  address));
	}
	else
	{
        gcmONERROR(
            gcoHARDWARE_LoadState32(hardware,
                                    0x01610,
                                    Address));
    }

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware,
                                0x01614,
                                rectSize.x * 4));

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware,
                                0x01640,
                                fillColor));

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware,
                                0x0163C,
                                control));

    /* Append new configuration register. */
    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware,
                                0x016A0,
                                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))));

    gcmONERROR(
        gcoHARDWARE_ProgramResolve(hardware,
                                   rectSize));

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware,
                                clearAddress,
                                ClearValue));

    if ((Type == gcvSURF_DEPTH) && hardware->earlyDepth)
    {
        /* Make raster wait for clear to be done. */
        gcmONERROR(
            gcoHARDWARE_Semaphore(gcvWHERE_RASTER,
                                  gcvWHERE_PIXEL,
                                  gcvHOW_SEMAPHORE));
    }

    /* Target has dirty pixels. */
    hardware->colorStates.dirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ComputeClearWindow
**
**  Compute the Width Height for clearing,
**  when only the bytes to clear are known.
**
**  INPUT:
**
**      gctSIZE_T Bytes
**          Number of Bytes to clear.
**
**  OUTPUT:
**
**      gctUINT32 Width
**          Width of clear window.
**
**      gctUINT32 Height
**          Height of clear window.
**
*/
gceSTATUS
gcoHARDWARE_ComputeClearWindow(
    IN gctSIZE_T Bytes,
    OUT gctUINT32 *Width,
    OUT gctUINT32 *Height
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gctUINT32 width, height, alignY;

    gcmHEADER_ARG("Bytes=%d Width=%d Height=%d",
                  Bytes, gcmOPT_VALUE(Width), gcmOPT_VALUE(Height));

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

	/* Compute alignment. */
	alignY = (hardware->resolveAlignmentY * 2 * hardware->pixelPipes) - 1;

	/* Compute rectangular width and height. */
	width  = hardware->resolveAlignmentX;
	height = Bytes / (width * 4);

    /* Too small? */
    if (height < (hardware->resolveAlignmentY * hardware->pixelPipes))
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

	while ((width < 8192) && ((height & alignY) == 0))
	{
		width  *= 2;
		height /= 2;
	}

	gcmASSERT((gctSIZE_T) (width * height * 4) == Bytes);

    if ((gctSIZE_T) (width * height * 4) == Bytes)
    {
        *Width  = width;
        *Height = height;
    }
    else
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */


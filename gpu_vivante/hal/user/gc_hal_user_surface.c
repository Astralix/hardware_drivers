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




/**
**  @file
**  gcoSURF object for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"

#define gcmALVM         iface.u.AllocateLinearVideoMemory

#define _GC_OBJ_ZONE    gcvZONE_SURFACE

/******************************************************************************\
**************************** gcoSURF API Support Code **************************
\******************************************************************************/

static gceSTATUS
_Lock(
#if gcdENABLE_VG
    IN gcoSURF Surface,
    IN gceHARDWARE_TYPE CurrentType
#else
    IN gcoSURF Surface
#endif
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    if (CurrentType == gcvHARDWARE_VG)
    {
        gcmONERROR(
            gcoVGHARDWARE_Lock(gcvNULL,
                         &Surface->info.node,
                         gcvNULL,
                         gcvNULL));
    }
    else
#endif
    {
        /* Lock the video memory. */
        gcmONERROR(
            gcoHARDWARE_Lock(
                         &Surface->info.node,
                         gcvNULL,
                         gcvNULL));
    }

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                  "Locked surface 0x%x: physical=0x%08X logical=0x%x lockCount=%d",
                  &Surface->info.node,
                  Surface->info.node.physical,
                  Surface->info.node.logical,
                  Surface->info.node.lockCount);
#ifndef VIVANTE_NO_3D
    /* Lock the hierarchical Z node. */
    if (Surface->info.hzNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Lock(&Surface->info.hzNode,
                             gcvNULL,
                             gcvNULL));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Locked HZ surface 0x%x: physical=0x%08X logical=0x%x "
                      "lockCount=%d",
                      &Surface->info.hzNode,
                      Surface->info.hzNode.physical,
                      Surface->info.hzNode.logical,
                      Surface->info.hzNode.lockCount);

        /* Only 1 address. */
        Surface->info.hzNode.count = 1;
    }

    /* Lock the tile status buffer. */
    if (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Lock the tile status buffer. */
        gcmONERROR(
            gcoHARDWARE_Lock(&Surface->info.tileStatusNode,
                             gcvNULL,
                             gcvNULL));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Locked tile status 0x%x: physical=0x%08X logical=0x%x "
                      "lockedCount=%d",
                      &Surface->info.tileStatusNode,
                      Surface->info.tileStatusNode.physical,
                      Surface->info.tileStatusNode.logical,
                      Surface->info.tileStatusNode.lockCount);

        /* Only 1 address. */
        Surface->info.tileStatusNode.count = 1;

        /* Check if this is the forst lock. */
        if (Surface->info.tileStatusNode.firstLock)
        {
            /* Fill the tile status memory with the filler. */
            gcmONERROR(
                gcoOS_MemFill(Surface->info.tileStatusNode.logical,
                              (gctUINT8) Surface->info.tileStatusNode.filler,
                              Surface->info.tileStatusNode.size));

            /* Flush the node from cache. */
            gcmONERROR(
                gcoSURF_NODE_Cache(&Surface->info.tileStatusNode,
                                 Surface->info.tileStatusNode.logical,
                                 Surface->info.tileStatusNode.size,
                                 gcvCACHE_CLEAN));

            /* Dump the memory write. */
            gcmDUMP_BUFFER(gcvNULL,
                           "memory",
                           Surface->info.tileStatusNode.physical,
                           Surface->info.tileStatusNode.logical,
                           0,
                           Surface->info.tileStatusNode.size);

            /* No longer first lock. */
            Surface->info.tileStatusNode.firstLock = gcvFALSE;
        }
    }

    /* Lock the hierarchical Z tile status buffer. */
    if (Surface->info.hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Lock the tile status buffer. */
        gcmONERROR(
            gcoHARDWARE_Lock(&Surface->info.hzTileStatusNode,
                             gcvNULL,
                             gcvNULL));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Locked HZ tile status 0x%x: physical=0x%08X logical=0x%x "
                      "lockedCount=%d",
                      &Surface->info.hzTileStatusNode,
                      Surface->info.hzTileStatusNode.physical,
                      Surface->info.hzTileStatusNode.logical,
                      Surface->info.hzTileStatusNode.lockCount);

        /* Only 1 address. */
        Surface->info.hzTileStatusNode.count = 1;

        /* Check if this is the forst lock. */
        if (Surface->info.hzTileStatusNode.firstLock)
        {
            /* Fill the tile status memory with the filler. */
            gcmONERROR(
                gcoOS_MemFill(Surface->info.hzTileStatusNode.logical,
                              (gctUINT8) Surface->info.hzTileStatusNode.filler,
                              Surface->info.hzTileStatusNode.size));

            /* Flush the node from cache. */
            gcmONERROR(
                gcoSURF_NODE_Cache(&Surface->info.hzTileStatusNode,
                                 Surface->info.hzTileStatusNode.logical,
                                 Surface->info.hzTileStatusNode.size,
                                 gcvCACHE_CLEAN));

            /* Dump the memory write. */
            gcmDUMP_BUFFER(gcvNULL,
                           "memory",
                           Surface->info.hzTileStatusNode.physical,
                           Surface->info.hzTileStatusNode.logical,
                           0,
                           Surface->info.hzTileStatusNode.size);

            /* No longer first lock. */
            Surface->info.hzTileStatusNode.firstLock = gcvFALSE;
        }
    }
#endif /* VIVANTE_NO_3D */
    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_Unlock(
#if gcdENABLE_VG
    IN gcoSURF Surface,
    IN gceHARDWARE_TYPE CurrentType
#else
    IN gcoSURF Surface
#endif
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    if (CurrentType == gcvHARDWARE_VG)
    {
        /* Unlock the surface. */
        gcmONERROR(
            gcoVGHARDWARE_Unlock(gcvNULL,
                               &Surface->info.node,
                               Surface->info.type));
    }
    else
#endif
    {
        /* Unlock the surface. */
        gcmONERROR(
            gcoHARDWARE_Unlock(
                               &Surface->info.node,
                               Surface->info.type));
    }

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                  "Unlocked surface 0x%x: lockCount=%d",
                  &Surface->info.node,
                  Surface->info.node.lockCount);

#ifndef VIVANTE_NO_3D
    /* Unlock the hierarchical Z buffer. */
    if (Surface->info.hzNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->info.hzNode,
                               gcvSURF_HIERARCHICAL_DEPTH));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Unlocked HZ surface 0x%x: lockCount=%d",
                      &Surface->info.hzNode,
                      Surface->info.hzNode.lockCount);
    }

    /* Unlock the tile status buffer. */
    if (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->info.tileStatusNode,
                               gcvSURF_TILE_STATUS));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Unlocked tile status 0x%x: lockCount=%d",
                      &Surface->info.hzNode,
                      Surface->info.hzNode.lockCount);
    }

    /* Unlock the hierarchical tile status buffer. */
    if (Surface->info.hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        gcmONERROR(
            gcoHARDWARE_Unlock(&Surface->info.hzTileStatusNode,
                               gcvSURF_TILE_STATUS));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Unlocked HZ tile status 0x%x: lockCount=%d",
                      &Surface->info.hzNode,
                      Surface->info.hzNode.lockCount);
    }
#endif /* VIVANTE_NO_3D */

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the errror. */
    return status;
}

#ifndef VIVANTE_NO_3D
static gceSTATUS
_AllocateTileStatus(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
    gctSIZE_T bytes;
    gctUINT alignment;
    gcsHAL_INTERFACE iface;

    /* Query the linear size for the tile status buffer. */
    status = gcoHARDWARE_QueryTileStatus(Surface->info.alignedWidth,
                                         Surface->info.alignedHeight,
                                         Surface->info.size,
                                         &bytes,
                                         &alignment,
                                         &Surface->info.tileStatusNode.filler);

    /* Tile status supported? */
    if (status == gcvSTATUS_NOT_SUPPORTED)
    {
        return gcvSTATUS_OK;
    }
    else if (gcmIS_ERROR(status))
    {
        return status;
    }

    /* Copy filler. */
    Surface->info.hzTileStatusNode.filler = Surface->info.tileStatusNode.filler;

    /* Allocate the tile status buffer. */
    iface.command     = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    gcmALVM.bytes     = bytes;
    gcmALVM.alignment = alignment;
    gcmALVM.type      = gcvSURF_TILE_STATUS;
    gcmALVM.pool      = gcvPOOL_DEFAULT;

    if (gcmIS_ERROR(gcoHAL_Call(gcvNULL, &iface)))
    {
        /* Commit any command buffer and wait for idle hardware. */
        status = gcoHAL_Commit(gcvNULL, gcvTRUE);

        if (gcmIS_SUCCESS(status))
        {
            /* Try allocating again. */
            status = gcoHAL_Call(gcvNULL, &iface);
        }
    }

    if (gcmIS_SUCCESS(status))
    {
        /* Set the node for the tile status buffer. */
        Surface->info.tileStatusNode.u.normal.node = gcmALVM.node;
        Surface->info.tileStatusNode.u.normal.cacheable = gcvFALSE;
        Surface->info.tileStatusNode.pool          = gcmALVM.pool;
        Surface->info.tileStatusNode.size          = gcmALVM.bytes;
        Surface->info.tileStatusNode.firstLock     = gcvTRUE;

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Allocated tile status 0x%x: pool=%d size=%u",
                      &Surface->info.tileStatusNode,
                      Surface->info.tileStatusNode.pool,
                      Surface->info.tileStatusNode.size);

        /* Allocate tile status for hierarchical Z buffer. */
        if (Surface->info.hzNode.pool != gcvPOOL_UNKNOWN)
        {
            /* Query the linear size for the tile status buffer. */
            status = gcoHARDWARE_QueryTileStatus(0,
                                                 0,
                                                 Surface->info.hzNode.size,
                                                 &bytes,
                                                 &alignment,
                                                 gcvNULL);

            /* Tile status supported? */
            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                return gcvSTATUS_OK;
            }

            /* Allocate the tile status buffer. */
            iface.command     = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
            gcmALVM.bytes     = bytes;
            gcmALVM.alignment = alignment;
            gcmALVM.type      = gcvSURF_TILE_STATUS;
            gcmALVM.pool      = gcvPOOL_DEFAULT;

            if (gcmIS_SUCCESS(gcoHAL_Call(gcvNULL, &iface)))
            {
                /* Set the node for the tile status buffer. */
                Surface->info.hzTileStatusNode.u.normal.node = gcmALVM.node;
                Surface->info.hzTileStatusNode.u.normal.cacheable = gcvFALSE;
                Surface->info.hzTileStatusNode.pool          = gcmALVM.pool;
                Surface->info.hzTileStatusNode.size          = gcmALVM.bytes;
                Surface->info.hzTileStatusNode.firstLock     = gcvTRUE;

                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                              "Allocated HZ tile status 0x%x: pool=%d size=%u",
                              &Surface->info.hzTileStatusNode,
                              Surface->info.hzTileStatusNode.pool,
                              Surface->info.hzTileStatusNode.size);
            }
        }
    }

    /* Return the status. */
    return status;
}
#endif /* VIVANTE_NO_3D */

static gceSTATUS
_FreeSurface(
#if gcdENABLE_VG
    IN gcoSURF Surface,
    IN gceHARDWARE_TYPE CurrentType
#else
    IN gcoSURF Surface
#endif
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* We only manage valid and non-user pools. */
    if ((Surface->info.node.pool != gcvPOOL_UNKNOWN)
    &&  (Surface->info.node.pool != gcvPOOL_USER)
    )
    {
#if gcdENABLE_VG
        /* Unlock the video memory. */
        gcmONERROR(_Unlock(Surface, CurrentType));

        if (CurrentType == gcvHARDWARE_VG)
        {
            /* Free the video memory. */
            gcmONERROR(
                gcoVGHARDWARE_ScheduleVideoMemory(gcvNULL, Surface->info.node.u.normal.node, gcvFALSE));
        }
        else
        {
            /* Free the video memory. */
            gcmONERROR(
                gcoHARDWARE_ScheduleVideoMemory(&Surface->info.node));
        }

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Freed surface 0x%x",
                      &Surface->info.node);

#else
        /* Unlock the video memory. */
        gcmONERROR(_Unlock(Surface));

        if (!(Surface->info.hints & gcvSURF_NO_VIDMEM))
        {
            /* Free the video memory. */
            gcmONERROR(
                gcoHARDWARE_ScheduleVideoMemory(&Surface->info.node));

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                          "Freed surface 0x%x",
                          &Surface->info.node);
        }
#endif
        /* Mark the memory as freed. */
        Surface->info.node.pool = gcvPOOL_UNKNOWN;
    }

#ifndef VIVANTE_NO_3D
    if (Surface->info.hzNode.pool != gcvPOOL_UNKNOWN)
    {
        /* Free the hierarchical Z video memory. */
        gcmONERROR(
            gcoHARDWARE_ScheduleVideoMemory(&Surface->info.hzNode));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "Freed HZ surface 0x%x",
                      &Surface->info.hzNode);

        /* Mark the memory as freed. */
        Surface->info.hzNode.pool = gcvPOOL_UNKNOWN;
    }

    if (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        if (!(Surface->info.hints & gcvSURF_NO_VIDMEM))
        {
            /* Free the tile status memory. */
            gcmONERROR(
                gcoHARDWARE_ScheduleVideoMemory(&Surface->info.tileStatusNode));

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                          "Freed tile status 0x%x",
                          &Surface->info.tileStatusNode);
        }

        /* Mark the tile status as freed. */
        Surface->info.tileStatusNode.pool = gcvPOOL_UNKNOWN;
    }

    if (Surface->info.hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
    {
        if (!(Surface->info.hints & gcvSURF_NO_VIDMEM))
        {
            /* Free the hierarchical Z tile status memory. */
            gcmONERROR(
                gcoHARDWARE_ScheduleVideoMemory(&Surface->info.hzTileStatusNode));

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                          "Freed HZ tile status 0x%x",
                          &Surface->info.hzTileStatusNode);
        }

        /* Mark the tile status as freed. */
        Surface->info.hzTileStatusNode.pool = gcvPOOL_UNKNOWN;
    }
#endif /* VIVANTE_NO_3D */

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if gcdENABLE_BANK_ALIGNMENT

#if !gcdBANK_BIT_START
#error gcdBANK_BIT_START not defined.
#endif

#if !gcdBANK_BIT_END
#error gcdBANK_BIT_END not defined.
#endif

/*******************************************************************************
**  gcoSURF_GetBankOffsetBytes
**
**  Return the bytes needed to offset sub-buffers to different banks.
**
**  ARGUMENTS:
**
**      gceSURF_TYPE Type
**          Type of buffer.
**
**      gctUINT32 TopBufferSize
**          Size of the top buffer, needed to compute offset of the second buffer.
**
**  OUTPUT:
**
**      gctUINT32_PTR Bytes
**          Pointer to a variable that will receive the byte offset needed.
**
*/
gceSTATUS
gcoSURF_GetBankOffsetBytes(
    IN gcoSURF Surfce,
    IN gceSURF_TYPE Type,
    IN gctUINT32 TopBufferSize,
    OUT gctUINT32_PTR Bytes
    )
{
    gctUINT32 bank;
    /* To retrieve the bank. */
    static const gctUINT32 bankMask = (0xFFFFFFFF << gcdBANK_BIT_START)
                                    ^ (0xFFFFFFFF << (gcdBANK_BIT_END + 1));

    gcmHEADER_ARG("Type=%d TopBufferSize=%x Bytes=0x%x", Type, TopBufferSize, Bytes);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes != gcvNULL);

    switch(Type)
    {
    case gcvSURF_RENDER_TARGET:
        bank = (TopBufferSize & bankMask) >> (gcdBANK_BIT_START);

        /* Put second buffer (c1 or z1) 5 banks away. */
        if (bank <= 5)
        {
            *Bytes += (5 - bank) << (gcdBANK_BIT_START);
        }
        else
        {
            *Bytes += (8 + 5 - bank) << (gcdBANK_BIT_START);
        }

#if gcdBANK_CHANNEL_BIT
        /* Add a channel offset at the channel bit. */
        *Bytes += (1 << gcdBANK_CHANNEL_BIT);
#endif
        break;

    case gcvSURF_DEPTH:
        bank = (TopBufferSize & bankMask) >> (gcdBANK_BIT_START);

        /* Put second buffer (c1 or z1) 5 banks away. */
        if (bank <= 5)
        {
            *Bytes += (5 - bank) << (gcdBANK_BIT_START);
        }
        else
        {
            *Bytes += (8 + 5 - bank) << (gcdBANK_BIT_START);
        }

#if gcdBANK_CHANNEL_BIT
        /* Subtract the channel bit, as it's added by kernel side. */
        if (*Bytes >= (1 << gcdBANK_CHANNEL_BIT))
        {
            *Bytes -= (1 << gcdBANK_CHANNEL_BIT);
        }
#endif
        break;

    default:
        /* No alignment needed. */
        *Bytes = 0;
    }

    gcmFOOTER_ARG("*Bytes=0x%x", *Bytes);
    return gcvSTATUS_OK;
}
#endif

static gceSTATUS
_AllocateSurface(
    IN gcoSURF Surface,
#if gcdENABLE_VG
    IN gceHARDWARE_TYPE CurrentType,
#endif
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gcePOOL Pool
    )
{
    gceSTATUS status;
    gctUINT32 bitsPerPixel;
    gcsHAL_INTERFACE iface;
    /* Extra pages needed to offset sub-buffers to different banks. */
    gctUINT32 bankOffsetBytes = 0;

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
    /* Extra room for tile to bitmap flip resovle start address adjustment */
    gctUINT32 flipOffsetBytes = 0;
    gctUINT   oldHeight;

    oldHeight = Height;
#endif
    gctBOOL tileStatusInVirtual;

#if gcdENABLE_VG
    if (CurrentType == gcvHARDWARE_VG)
    {
        /* Compute bits per pixel. */
        gcmONERROR(
            gcoVGHARDWARE_ConvertFormat(gcvNULL,
                                      Format,
                                      &bitsPerPixel,
                                      gcvNULL));
    }
    else
#endif
    {
        /* Compute bits per pixel. */
        gcmONERROR(
            gcoHARDWARE_ConvertFormat(
                                  Format,
                                  &bitsPerPixel,
                                  gcvNULL));
    }

    /* Set dimensions of surface. */
    Surface->info.rect.left   = 0;
    Surface->info.rect.top    = 0;
    Surface->info.rect.right  = Width;
    Surface->info.rect.bottom = Height;

    /* Set the number of planes. */
    Surface->depth = Depth;

    /* Initialize rotation. */
    Surface->info.rotation    = gcvSURF_0_DEGREE;
#ifndef VIVANTE_NO_3D
    Surface->info.orientation = gcvORIENTATION_TOP_BOTTOM;
    Surface->resolvable       = gcvTRUE;
#endif /* VIVANTE_NO_3D */

    /* Obtain canonical type of surface. */
    Surface->info.type   = (gceSURF_TYPE) ((gctUINT32) Type & 0xFF);
    /* Get 'hints' of this surface. */
    Surface->info.hints  = (gceSURF_TYPE) ((gctUINT32) Type & ~0xFF);
    /* Set format of surface. */
    Surface->info.format = Format;
    Surface->info.tiling = gcvLINEAR;

    /* Set aligned surface size. */
    Surface->info.alignedWidth  = Width;
    Surface->info.alignedHeight = Height;
    Surface->info.is16Bit       = (bitsPerPixel == 16);

#ifndef VIVANTE_NO_3D
    /* Init superTiled info. */
    Surface->info.superTiled = gcvFALSE;
#endif /* VIVANTE_NO_3D */

    /* Reset the node. */
    Surface->info.node.valid          = gcvFALSE;
    Surface->info.node.lockCount      = 0;
    Surface->info.node.lockedInKernel = 0;
    Surface->info.node.count          = 0;
    Surface->info.node.size           = 0;
    Surface->info.node.firstLock      = gcvTRUE;
    Surface->info.node.pool           = gcvPOOL_UNKNOWN;

    /* User pool? */
    if (Pool == gcvPOOL_USER)
    {
        /* Init the node as the user allocated. */
        Surface->info.node.pool                    = gcvPOOL_USER;
        Surface->info.node.u.wrapped.logicalMapped = gcvFALSE;
        Surface->info.node.u.wrapped.mappingInfo   = gcvNULL;
        Surface->info.node.size                    = Surface->info.alignedWidth
                                                   * (bitsPerPixel / 8)
                                                   * Surface->info.alignedHeight
                                                   * Surface->depth;
    }

    /* No --> allocate video memory. */
    else
    {
#if gcdENABLE_VG
        if (CurrentType == gcvHARDWARE_VG)
        {
            gcmONERROR(
                gcoVGHARDWARE_AlignToTile(gcvNULL,
                                          Surface->info.type,
                                          &Surface->info.alignedWidth,
                                          &Surface->info.alignedHeight));
        }
        else
#endif
        {
            /* Align width and height to tiles. */
#ifndef VIVANTE_NO_3D
            if ((Surface->info.samples.x > 1 || Surface->info.samples.y > 1)
            &&  (Surface->info.vaa == gcvFALSE)
            )
            {
                Width  = gcoMATH_DivideUInt(Width,  Surface->info.samples.x);
                Height = gcoMATH_DivideUInt(Height, Surface->info.samples.y);

                gcmONERROR(
                    gcoHARDWARE_AlignToTileCompatible(Surface->info.type,
                                                      Format,
                                                      &Width,
                                                      &Height,
                                                      &Surface->info.tiling,
                                                      &Surface->info.superTiled));

                Surface->info.alignedWidth  = Width  * Surface->info.samples.x;
                Surface->info.alignedHeight = Height * Surface->info.samples.y;
            }
            else
            {
#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
                /* TODO: Fix alignment for DEPTH_NO_TILE_STATUS! */
                gceSURF_TYPE type = (Type & ~gcvSURF_NO_VIDMEM) & ~gcvSURF_FLIP;

                gcmONERROR(
                    gcoHARDWARE_AlignToTileCompatible(type,
                                                      Format,
                                                      &Surface->info.alignedWidth,
                                                      &Surface->info.alignedHeight,
                                                      &Surface->info.tiling,
                                                      &Surface->info.superTiled));

                if ((oldHeight != Surface->info.alignedHeight)
                &&  (Type == gcvSURF_FLIP_BITMAP)
                )
                {
                    oldHeight = Surface->info.alignedHeight + 1;

                    gcmONERROR(
                        gcoHARDWARE_AlignToTileCompatible(type,
                                                          Format,
                                                          gcvNULL,
                                                          &oldHeight,
                                                          gcvNULL,
                                                          gcvNULL));

                     flipOffsetBytes = (oldHeight - Surface->info.alignedHeight)
                                     * Surface->info.alignedWidth;
                }
#   else
                /* TODO: Fix alignment for DEPTH_NO_TILE_STATUS! */
                gcmONERROR(
                    gcoHARDWARE_AlignToTileCompatible(Type & ~gcvSURF_NO_VIDMEM,
                                                      Format,
                                                      &Surface->info.alignedWidth,
                                                      &Surface->info.alignedHeight,
                                                      &Surface->info.tiling,
                                                      &Surface->info.superTiled));
#   endif
            }
#else
                gcmONERROR(
                    gcoHARDWARE_AlignToTileCompatible(Surface->info.type,
                                                      Format,
                                                      &Surface->info.alignedWidth,
                                                      &Surface->info.alignedHeight,
                                                      &Surface->info.tiling,
                                                      gcvNULL));
#endif /* VIVANTE_NO_3D */
        }

        if (!(Type & gcvSURF_NO_VIDMEM))
        {
#if gcdENABLE_BANK_ALIGNMENT
            gctUINT32 topBufferSize = gcmALIGN(Surface->info.alignedHeight / 2,
                                         Surface->info.superTiled ? 64 : 4)
                                    * (Surface->info.alignedWidth * bitsPerPixel / 8);

            gcmONERROR(
                gcoSURF_GetBankOffsetBytes(gcvNULL,
                                           Surface->info.type,
                                           topBufferSize,
                                           &bankOffsetBytes
                                           ));
#endif

            iface.command     = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
            gcmALVM.bytes     = Surface->info.alignedWidth * bitsPerPixel / 8
                              * Surface->info.alignedHeight
                              * Depth;
            gcmALVM.bytes    += bankOffsetBytes;

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
            gcmALVM.bytes    += flipOffsetBytes;
#endif
            gcmALVM.alignment = 64;
            gcmALVM.pool      = Pool;
            gcmALVM.type      = Surface->info.type;

            /* Call kernel API. */
            gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

            /* Get allocated node in video memory. */
            Surface->info.node.u.normal.node = gcmALVM.node;
            Surface->info.node.pool          = gcmALVM.pool;
            Surface->info.node.size          = gcmALVM.bytes;
        }
    }

    /* Determine the surface placement parameters. */
    switch (Format)
    {
    case gcvSURF_YV12:
#if defined(ANDROID)
        /* Per google's requirement, we need u/v plane align to 16, and there is no gap between YV plane */
        Surface->info.vOffset
            = Surface->info.alignedWidth
            * Height;

        Surface->info.stride
            = Surface->info.alignedWidth;

        Surface->info.uStride =
        Surface->info.vStride
            = (Surface->info.alignedWidth / 2 + 0xf) & ~0xf;

        Surface->info.uOffset
            = Surface->info.vOffset
            + Surface->info.vStride * Height / 2;

        Surface->info.size
            = Surface->info.vOffset
            + Surface->info.vStride * Height;
#else
        /*  WxH Y plane followed by (W/2)x(H/2) V and U planes. */
        Surface->info.vOffset
            = Surface->info.alignedWidth
            * Surface->info.alignedHeight;

        Surface->info.uOffset
            = Surface->info.vOffset
            + Surface->info.vOffset / 4;

        Surface->info.stride
            = Surface->info.alignedWidth;

        Surface->info.uStride =
        Surface->info.vStride
            = Surface->info.alignedWidth / 2;

        Surface->info.size
            = Surface->info.vOffset
            + Surface->info.vOffset / 2;
#endif
        break;

    case gcvSURF_I420:
#if defined(ANDROID)
        /* Per google's requirement, we need u/v plane align to 16, and there is no gap between YV plane */
        Surface->info.uOffset
            = Surface->info.alignedWidth
            * Height;

        Surface->info.stride
            = Surface->info.alignedWidth;

        Surface->info.uStride =
        Surface->info.vStride
            = (Surface->info.alignedWidth / 2 + 0xf) & ~0xf;

        Surface->info.vOffset
            = Surface->info.vOffset
            + Surface->info.vStride * Height / 2;

        Surface->info.size
            = Surface->info.vOffset
            + Surface->info.vStride * Height;
#else
        /*  WxH Y plane followed by (W/2)x(H/2) U and V planes. */
        Surface->info.uOffset
            = Surface->info.alignedWidth
            * Surface->info.alignedHeight;

        Surface->info.vOffset
            = Surface->info.uOffset
            + Surface->info.uOffset / 4;

        Surface->info.stride
            = Surface->info.alignedWidth;

        Surface->info.uStride
            = Surface->info.vStride
            = Surface->info.alignedWidth / 2;

        Surface->info.size
            = Surface->info.uOffset
            + Surface->info.uOffset / 2;
#endif
        break;

    case gcvSURF_NV12:
    case gcvSURF_NV21:
        /*  WxH Y plane followed by (W)x(H/2) interleaved U/V plane. */
        Surface->info.uOffset
            = Surface->info.vOffset
            = Surface->info.alignedWidth
            * Surface->info.alignedHeight;

        Surface->info.stride
            = Surface->info.uStride
            = Surface->info.vStride
            = Surface->info.alignedWidth;

        Surface->info.size
            = Surface->info.uOffset
            + Surface->info.uOffset / 2;
        break;

    case gcvSURF_NV16:
    case gcvSURF_NV61:
        Surface->info.uOffset
            = Surface->info.vOffset
            = Surface->info.alignedWidth
            * Surface->info.alignedHeight;

        Surface->info.stride
            = Surface->info.uStride
            = Surface->info.vStride
            = Surface->info.alignedWidth;

        Surface->info.size
            = Surface->info.uOffset
            + Surface->info.uOffset;
        break;

    default:
        Surface->info.uOffset = Surface->info.vOffset = 0;
        Surface->info.uStride = Surface->info.vStride = 0;

        Surface->info.stride
            = Surface->info.alignedWidth
            * bitsPerPixel / 8;

        Surface->info.size
            = Surface->info.stride
            * Surface->info.alignedHeight;
    }

    /* Add any offset bytes added between sub-buffers. */
    Surface->info.size += bankOffsetBytes;

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
    Surface->info.size += flipOffsetBytes;
#endif

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                  "Allocated surface 0x%x: pool=%d size=%dx%d bytes=%u",
                  &Surface->info.node,
                  Surface->info.node.pool,
                  Surface->info.alignedWidth,
                  Surface->info.alignedHeight,
                  Surface->info.size);

#ifndef VIVANTE_NO_3D
    /* No Hierarchical Z buffer allocated. */
    Surface->info.hzNode.pool           = gcvPOOL_UNKNOWN;
    Surface->info.hzNode.valid          = gcvFALSE;
    Surface->info.hzNode.lockCount      = 0;
    Surface->info.hzNode.lockedInKernel = 0;
    Surface->info.hzNode.count          = 0;
    Surface->info.hzNode.size           = 0;

    /* Check if this is a depth buffer and the GPU supports hierarchical Z. */
    if ((Type == gcvSURF_DEPTH)
    &&  (gcoHAL_IsFeatureAvailable(gcvNULL,
                                   gcvFEATURE_HZ) == gcvSTATUS_TRUE)
    )
    {
        gctSIZE_T bytes;

        /* Compute the hierarchical Z buffer size.  Allocate enough for
        ** 16-bit min/max values. */
        bytes = gcmALIGN((Surface->info.size + 63) / 64 * 4, 256);

        /* Allocate the hierarchical Z buffer. */
        iface.command     = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        gcmALVM.bytes     = bytes;
        gcmALVM.alignment = 64;
        gcmALVM.type      = gcvSURF_HIERARCHICAL_DEPTH;
        gcmALVM.pool      = Pool;

        if (gcmIS_SUCCESS(gcoHAL_Call(gcvNULL, &iface)))
        {
            /* Save hierarchical Z buffer info. */
            Surface->info.hzNode.pool          = gcmALVM.pool;
            Surface->info.hzNode.size          = gcmALVM.bytes;
            Surface->info.hzNode.u.normal.node = gcmALVM.node;
            Surface->info.hzNode.firstLock     = gcvTRUE;
            Surface->info.hzNode.u.normal.cacheable  = gcvFALSE;

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                          "Allocated HZ surface 0x%x: pool=%d size=%u",
                          &Surface->info.hzNode,
                          Surface->info.hzNode.pool,
                          Surface->info.hzNode.size);
        }
    }

    /* No tile status buffer allocated. */
    Surface->info.tileStatusNode.pool             = gcvPOOL_UNKNOWN;
    Surface->info.tileStatusNode.valid            = gcvFALSE;
    Surface->info.tileStatusNode.lockCount        = 0;
    Surface->info.tileStatusNode.lockedInKernel   = 0;
    Surface->info.tileStatusNode.count            = 0;
    Surface->info.hzTileStatusNode.pool           = gcvPOOL_UNKNOWN;
    Surface->info.hzTileStatusNode.valid          = gcvFALSE;
    Surface->info.hzTileStatusNode.lockCount      = 0;
    Surface->info.hzTileStatusNode.lockedInKernel = 0;
    Surface->info.hzTileStatusNode.count          = 0;

    /* Default state of tileStatusDisabled. */
    Surface->info.tileStatusDisabled = gcvFALSE;

    /* Set default fill color. */
    switch (Format)
    {
    case gcvSURF_D16:
        Surface->info.clearValue   = 0xFFFFFFFF;
        gcmONERROR(gcoHARDWARE_HzClearValueControl(Format,
                                                   Surface->info.clearValue,
                                                   &Surface->info.clearValueHz,
                                                   gcvNULL));
        break;

    case gcvSURF_D24X8:
    case gcvSURF_D24S8:
        Surface->info.clearValue   = 0xFFFFFF00;
        gcmONERROR(gcoHARDWARE_HzClearValueControl(Format,
                                                   Surface->info.clearValue,
                                                   &Surface->info.clearValueHz,
                                                   gcvNULL));
        break;

    default:
        Surface->info.clearValue = 0x00000000;
        break;
    }

    tileStatusInVirtual = gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_MC20);

    /* Verify if the type requires a tile status buffer:
    ** - do not turn on fast clear if the surface is virtual;
    ** - for user pools we don't have the address of the surface yet,
    **   delay tile status determination until we map the surface. */
    if ((Surface->info.node.pool != gcvPOOL_USER) &&
        (Surface->info.node.pool != gcvPOOL_VIRTUAL || tileStatusInVirtual) &&
        (((Type & ~gcvSURF_NO_VIDMEM) == gcvSURF_RENDER_TARGET) ||
        (Type == gcvSURF_DEPTH)
        )
    )
    {
#if gcdENABLE_VG
        gcmASSERT(CurrentType != gcvHARDWARE_VG);
#endif
        _AllocateTileStatus(Surface);
    }
#endif /* VIVANTE_NO_3D */

    if (Pool != gcvPOOL_USER)
    {
        if (!(Type & gcvSURF_NO_VIDMEM))
        {
#if gcdENABLE_VG
            /* Lock the surface. */
            gcmONERROR(_Lock(Surface, CurrentType));
#else
            /* Lock the surface. */
            gcmONERROR(_Lock(Surface));
#endif
        }
    }

    /* Success. */
    return gcvSTATUS_OK;

OnError:

#if gcdENABLE_VG
    _FreeSurface(Surface, CurrentType);
#else
    /* Free the memory allocated to the surface. */
    _FreeSurface(Surface);
#endif
    /* Return the status. */
    return status;
}

static gceSTATUS
_UnmapUserBuffer(
    IN gcoSURF Surface,
    IN gctBOOL ForceUnmap
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Surface=0x%x ForceUnmap=%d", Surface, ForceUnmap);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Cannot be negative. */
        gcmASSERT(Surface->info.node.lockCount >= 0);

        /* Nothing is mapped? */
        if (Surface->info.node.lockCount == 0)
        {
            /* Nothing to do. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Make sure the reference couner is proper. */
        if (Surface->info.node.lockCount > 1)
        {
            /* Forced unmap? */
            if (ForceUnmap)
            {
                /* Invalid reference count. */
                gcmASSERT(gcvFALSE);
            }
            else
            {
                /* Decrement. */
                Surface->info.node.lockCount -= 1;

                /* Done for now. */
                status = gcvSTATUS_OK;
                break;
            }
        }

        /* Unmap the logical memory. */
        if (Surface->info.node.u.wrapped.logicalMapped)
        {
            gcmERR_BREAK(gcoHAL_UnmapMemory(
                gcvNULL,
                gcmINT2PTR(Surface->info.node.physical),
                Surface->info.size,
                Surface->info.node.logical
                ));

            Surface->info.node.physical = ~0U;
            Surface->info.node.u.wrapped.logicalMapped = gcvFALSE;
        }

        /* Unmap the physical memory. */
        if (Surface->info.node.u.wrapped.mappingInfo != gcvNULL)
        {
            gceHARDWARE_TYPE currentType;

            /* Save the current hardware type */
            gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &currentType));

            if (Surface->info.node.u.wrapped.mappingHardwareType != currentType)
            {
                /* Change to the mapping hardware type */
                gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL,
                                                    Surface->info.node.u.wrapped.mappingHardwareType));
            }

            gcmERR_BREAK(gcoHAL_ScheduleUnmapUserMemory(
                gcvNULL,
                Surface->info.node.u.wrapped.mappingInfo,
                Surface->info.size,
                Surface->info.node.physical,
                Surface->info.node.logical
                ));

            Surface->info.node.logical = gcvNULL;
            Surface->info.node.u.wrapped.mappingInfo = gcvNULL;

            if (Surface->info.node.u.wrapped.mappingHardwareType != currentType)
            {
                /* Restore the current hardware type */
                gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, currentType));
            }
        }

#if gcdSECURE_USER
        gcmERR_BREAK(gcoHAL_ScheduleUnmapUserMemory(
            gcvNULL,
            gcvNULL,
            Surface->info.size,
            0,
            Surface->info.node.logical));
#endif

        /* Reset the surface. */
        Surface->info.node.lockCount = 0;
        Surface->info.node.valid = gcvFALSE;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
******************************** gcoSURF API Code *******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoSURF_Construct
**
**  Create a new gcoSURF object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctUINT Width
**          Width of surface to create in pixels.
**
**      gctUINT Height
**          Height of surface to create in lines.
**
**      gctUINT Depth
**          Depth of surface to create in planes.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gcePOOL Pool
**          Pool to allocate surface from.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to the variable that will hold the gcoSURF object pointer.
*/
gceSTATUS
gcoSURF_Construct(
    IN gcoHAL Hal,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gcePOOL Pool,
    OUT gcoSURF * Surface
    )
{
    gcoSURF surface = gcvNULL;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif

    gcmHEADER_ARG("Width=%u Height=%u Depth=%u Type=%d Format=%d Pool=%d",
                  Width, Height, Depth, Type, Format, Pool);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    /* Allocate the gcoSURF object. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _gcoSURF),
                       &pointer));

    gcmONERROR(
        gcoOS_ZeroMemory(pointer, gcmSIZEOF(struct _gcoSURF)));

    surface = pointer;

    /* Initialize the gcoSURF object.*/
    surface->object.type    = gcvOBJ_SURF;

    surface->info.dither    = gcvFALSE;

    surface->info.offset    = 0;

#ifndef VIVANTE_NO_3D
    /* 1 sample per pixel, no VAA. */
    surface->info.samples.x = 1;
    surface->info.samples.y = 1;
    surface->info.vaa       = gcvFALSE;
    surface->info.dirty     = gcvTRUE;

    surface->info.colorType = gcvSURF_COLOR_UNKNOWN;

    surface->info.hzNode.pool = gcvPOOL_UNKNOWN;
    surface->info.tileStatusNode.pool = gcvPOOL_UNKNOWN;
    surface->info.hzTileStatusNode.pool = gcvPOOL_UNKNOWN;
    surface->info.node.physical = ~0U;
    surface->info.node.physical2 = ~0U;
    surface->info.node.physical3 = ~0U;
#endif /* VIVANTE_NO_3D */

    if (Type & gcvSURF_CACHEABLE)
    {
        gcmASSERT(Pool != gcvPOOL_USER);
        surface->info.node.u.normal.cacheable = gcvTRUE;
        Type &= ~gcvSURF_CACHEABLE;
    }
    else if (Pool != gcvPOOL_USER)
    {
        surface->info.node.u.normal.cacheable = gcvFALSE;
    }

#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentType);
    /* Allocate surface. */
    gcmONERROR(
        _AllocateSurface(surface,
                         currentType,
                         Width, Height, Depth,
                         Type,
                         Format,
                         Pool));
#else
    /* Allocate surface. */
    gcmONERROR(
        _AllocateSurface(surface,
                         Width, Height, Depth,
                         Type,
                         Format,
                         Pool));
#endif

    surface->referenceCount = 1;

#if defined(ANDROID) && gcdDEFER_RESOLVES
    /* Create a signal for SF-app synchronization. */
    if (Type & gcvSURF_RENDER_TARGET)
    {
        gcmONERROR(gcoOS_CreateSignal(
                    gcvNULL,
                    gcvFALSE,
                    &surface->resolveSubmittedSignal
                    ));
    }
#endif

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                  "Created gcoSURF 0x%x",
                  surface);

    /* Return pointer to the gcoSURF object. */
    *Surface = surface;

    /* Success. */
    gcmFOOTER_ARG("*Surface=0x%x", *Surface);
    return gcvSTATUS_OK;

OnError:
    /* Free the allocated memory. */
    if (surface != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, surface);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Destroy
**
**  Destroy an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Destroy(
    IN gcoSURF Surface
    )
{
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif
    gcsTLS_PTR tls;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Decrement surface reference count. */
    if (--Surface->referenceCount != 0)
    {
        /* Still references to this surface. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

#ifndef VIVANTE_NO_3D

    gcmVERIFY_OK(gcoOS_GetTLS(&tls));

#if gcdENABLE_VG
    currentType = tls->currentType;

    /* Unset VG target. */
    if (tls->engineVG != gcvNULL)
    {
        gcmVERIFY_OK(
            gcoVG_UnsetTarget(tls->engineVG,
            Surface));
    }
#endif

    /* Special case for 3D surfaces. */
    if (tls->engine3D != gcvNULL)
    {
        /* If this is a render target, unset it. */
        if ((Surface->info.type == gcvSURF_RENDER_TARGET)
        ||  (Surface->info.type == gcvSURF_TEXTURE)
        )
        {
            gcmVERIFY_OK(
                gco3D_UnsetTarget(tls->engine3D, Surface));
        }

        /* If this is a depth buffer, unset it. */
        else if (Surface->info.type == gcvSURF_DEPTH)
        {
            gcmVERIFY_OK(
                gco3D_UnsetDepth(tls->engine3D, Surface));
        }
    }
#endif

    if (gcPLS.hal->dump != gcvNULL)
    {
        /* Dump the deletion. */
        gcmVERIFY_OK(
            gcoDUMP_Delete(gcPLS.hal->dump, Surface->info.node.physical));
    }

    /* User-allocated surface? */
    if (Surface->info.node.pool == gcvPOOL_USER)
    {
        gcmVERIFY_OK(
            _UnmapUserBuffer(Surface, gcvTRUE));
    }

#if defined(ANDROID) && gcdDEFER_RESOLVES
    /* Destroy signal used for SF-app synchronization. */
    if (Surface->info.type & gcvSURF_RENDER_TARGET)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(
                        gcvNULL,
                        Surface->resolveSubmittedSignal
                        ));
    }
#endif

#if gcdENABLE_VG
    /* Free the video memory. */
    gcmVERIFY_OK(_FreeSurface(Surface, currentType));
#else
    /* Free the video memory. */
    gcmVERIFY_OK(_FreeSurface(Surface));
#endif

    /* Mark gcoSURF object as unknown. */
    Surface->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoSURF object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Surface));

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                  "Destroyed gcoSURF 0x%x",
                  Surface);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_QueryVidMemNode
**
**  Query the video memory node attributes of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gcuVIDMEM_NODE_PTR * Node
**          Pointer to a variable receiving the video memory node.
**
**      gcePOOL * Pool
**          Pointer to a variable receiving the pool the video memory node originated from.
**
**      gctUINT_PTR Bytes
**          Pointer to a variable receiving the video memory node size in bytes.
**
*/
gceSTATUS
gcoSURF_QueryVidMemNode(
    IN gcoSURF Surface,
    OUT gcuVIDMEM_NODE_PTR * Node,
    OUT gcePOOL * Pool,
    OUT gctUINT_PTR Bytes
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Node != gcvNULL);
    gcmVERIFY_ARGUMENT(Pool != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes != gcvNULL);

    /* Return the video memory attributes. */
    *Node = Surface->info.node.u.normal.node;
    *Pool = Surface->info.node.pool;
    *Bytes = Surface->info.node.size;

    /* Success. */
    gcmFOOTER_ARG("*Node=0x%x *Pool=%d *Bytes=%d", *Node, *Pool, *Bytes);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetUsage
**
**  Set usage attribute of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_USAGE Usage
**          Usage purpose for the surface.
**
**  OUTPUT:
**      None
**
*/
gceSTATUS
gcoSURF_SetUsage(
    IN gcoSURF Surface,
    IN gceSURF_USAGE Usage
    )
{
    Surface->info.usage = Usage;
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_QueryUsage
**
**  Return usage attribute of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**      gceSURF_USAGE *Usage
**          Usage purpose for the surface.
**
*/
gceSTATUS
gcoSURF_QueryUsage(
    IN gcoSURF Surface,
    OUT gceSURF_USAGE *Usage
    )
{
    if (Usage)
    {
        *Usage = Surface->info.usage;
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_MapUserSurface
**
**  Store the logical and physical pointers to the user-allocated surface in
**  the gcoSURF object and map the pointers as necessary.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object to be destroyed.
**
**      gctUINT Alignment
**          Alignment of each pixel row in bytes.
**
**      gctPOINTER Logical
**          Logical pointer to the user allocated surface or gcvNULL if no
**          logical pointer has been provided.
**
**      gctUINT32 Physical
**          Physical pointer to the user allocated surface or ~0 if no
**          physical pointer has been provided.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_MapUserSurface(
    IN gcoSURF Surface,
    IN gctUINT Alignment,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gctBOOL logicalMapped = gcvFALSE;
    gctPOINTER mappingInfo = gcvNULL;

    gctPOINTER logical = gcvNULL;
    gctUINT32 physical = 0;
    gctUINT32 bitsPerPixel;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif

    gcmHEADER_ARG("Surface=0x%x Alignment=%u Logical=0x%x Physical=%08x",
              Surface, Alignment, Logical, Physical);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Has to be user-allocated surface. */
        if (Surface->info.node.pool != gcvPOOL_USER)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Already mapped? */
        if (Surface->info.node.lockCount > 0)
        {
            if ((Logical != gcvNULL) &&
                (Logical != Surface->info.node.logical))
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            if ((Physical != ~0U) &&
                (Physical != Surface->info.node.physical))
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Success. */
            break;
        }
#if gcdENABLE_VG
        gcmGETCURRENTHARDWARE(currentType);
#endif
        /* Set new alignment. */
        if (Alignment != 0)
        {
            gctUINT32 stride;
#if gcdENABLE_VG
            if (currentType == gcvHARDWARE_VG)
            {
                gcmERR_BREAK(gcoVGHARDWARE_ConvertFormat(
                    gcvNULL,
                    Surface->info.format,
                    &bitsPerPixel,
                    gcvNULL
                    ));
            }
            else
#endif
            {
                /* Compute bits per pixel. */
                gcmERR_BREAK(gcoHARDWARE_ConvertFormat(
                    Surface->info.format,
                    &bitsPerPixel,
                    gcvNULL
                    ));
            }

            /* Compute the unaligned stride. */
            stride = Surface->info.alignedWidth * bitsPerPixel / 8;

            /* Align the stide (Alignment can be not a power of number). */
            if (gcoMATH_ModuloUInt(stride, Alignment) != 0)
            {
                stride = gcoMATH_DivideUInt(stride, Alignment)  * Alignment
                       + Alignment;
            }

            /* Set the new stride. */
            Surface->info.stride = stride;

            /* Compute the new size. */
            Surface->info.size
                = stride * Surface->info.alignedHeight;
        }

        /* Map logical pointer if not specified. */
        if (Logical == gcvNULL)
        {
            if (Physical == ~0U)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Map the logical pointer. */
            gcmERR_BREAK(gcoHAL_MapMemory(
                gcvNULL,
                gcmINT2PTR(Physical),
                Surface->info.size,
                &logical
                ));

            /* Mark as mapped. */
            logicalMapped = gcvTRUE;
        }
        else
        {
            /* Set the logical pointer. */
            logical = Logical;
        }

        /* Map physical pointer if not specified. */
        if (Physical == ~0U)
        {
            if (Logical == gcvNULL)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Map the physical pointer. */
            gcmERR_BREAK(gcoOS_MapUserMemory(
                gcvNULL,
                Logical,
                Surface->info.size,
                &mappingInfo,
                &physical
                ));
        }
        else
        {
            /* Set the physical pointer. */
            physical = Physical;
        }

        /* Validate the surface. */
        Surface->info.node.valid = gcvTRUE;

        /* Set the lock count. */
        Surface->info.node.lockCount++;

#if gcdENABLE_VG
        if (currentType == gcvHARDWARE_VG)
        {
            /* Compute bits per pixel. */
            gcmERR_BREAK(gcoVGHARDWARE_ConvertFormat(
                gcvNULL,
                Surface->info.format,
                &bitsPerPixel,
                gcvNULL
                ));
        }
        else
#endif
        {
            /* Compute bits per pixel. */
            gcmERR_BREAK(gcoHARDWARE_ConvertFormat(
                Surface->info.format,
                &bitsPerPixel,
                gcvNULL
                ));
        }
        /* Set the node parameters. */
        Surface->info.node.u.wrapped.logicalMapped = logicalMapped;
        Surface->info.node.u.wrapped.mappingInfo   = mappingInfo;
        gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL,
                                            &Surface->info.node.u.wrapped.mappingHardwareType));
        Surface->info.node.logical                 = logical;
        Surface->info.node.physical                = physical;
        Surface->info.node.count                   = 1;
    }
    while (gcvFALSE);

    /* Roll back. */
    if (gcmIS_ERROR(status))
    {
        if (logicalMapped)
        {
            gcmVERIFY_OK(gcoHAL_UnmapMemory(
                gcvNULL,
                gcmINT2PTR(physical),
                Surface->info.size,
                logical
                ));
        }

        if (mappingInfo != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_UnmapUserMemory(
                gcvNULL,
                logical,
                Surface->info.size,
                mappingInfo,
                physical
                ));
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_IsValid
**
**  Verify whether the surface is a valid gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      surface is valid.
*/
gceSTATUS
gcoSURF_IsValid(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set return value. */
    status = ((Surface != gcvNULL)
        && (Surface->object.type != gcvOBJ_SURF))
        ? gcvSTATUS_FALSE
        : gcvSTATUS_TRUE;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoSURF_IsTileStatusSupported
**
**  Verify whether the tile status is supported by the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      tile status is supported.
*/
gceSTATUS
gcoSURF_IsTileStatusSupported(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set return value. */
    status = (Surface->info.tileStatusNode.pool == gcvPOOL_UNKNOWN)
        ? gcvSTATUS_NOT_SUPPORTED
        : gcvSTATUS_TRUE;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetTileStatus
**
**  Set tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetTileStatus(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        if (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
        {
            /* Enable tile status. */
            gcmERR_BREAK(
                gcoHARDWARE_SetTileStatus(&Surface->info,
                                          Surface->info.tileStatusNode.physical,
                                          &Surface->info.hzTileStatusNode));
        }

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_EnableTileStatus
**
**  Enable tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_EnableTileStatus(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Enable tile status. */
        gcmERR_BREAK(
            gcoHARDWARE_EnableTileStatus(&Surface->info,
										 (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN) ?
                                         Surface->info.tileStatusNode.physical : 0,
                                         &Surface->info.hzTileStatusNode));

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_DisableTileStatus
**
**  Disable tile status for the specified surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctBOOL Decompress
**          Set if the render target needs to decompressed by issuing a resolve
**          onto itself.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_DisableTileStatus(
    IN gcoSURF Surface,
    IN gctBOOL Decompress
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        if (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
        {
            /* Disable tile status. */
            gcmERR_BREAK(
                gcoHARDWARE_DisableTileStatus(&Surface->info,
                                              Decompress));
        }

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_SetLinearResolveAddress
**
**  Set the new resolve Address for linear surface, this should only
**  for 3D composition.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to a gcoSURF object.
**
**      gctUINT32 Address
**          Physicali Address
**
**      gctPOINTER  Memory
**          Logical Address
**
**  RETURNS:
**
**      The return value of the function is set to gcvSTATUS_TRUE if the
**      surface is valid.
*/

gceSTATUS
gcoSURF_SetLinearResolveAddress(
    IN gcoSURF Surface,
    IN gctUINT32 Address,
    IN gctPOINTER Memory
    )
{
#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x, Address=0x%x, Memory=0x%x",
                  Surface,Address,Memory);

    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    Surface->info.linearResolvePhysical  = Address;

    Surface->info.linearResolveLogical  = Memory;

    status = gcvSTATUS_TRUE;

    gcmFOOTER();

    return status;
#else
    return gcvSTATUS_TRUE;
#endif
}

#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoSURF_GetSize
**
**  Get the size of an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT * Width
**          Pointer to variable that will receive the width of the gcoSURF
**          object.  If 'Width' is gcvNULL, no width information shall be returned.
**
**      gctUINT * Height
**          Pointer to variable that will receive the height of the gcoSURF
**          object.  If 'Height' is gcvNULL, no height information shall be returned.
**
**      gctUINT * Depth
**          Pointer to variable that will receive the depth of the gcoSURF
**          object.  If 'Depth' is gcvNULL, no depth information shall be returned.
*/
gceSTATUS
gcoSURF_GetSize(
    IN gcoSURF Surface,
    OUT gctUINT * Width,
    OUT gctUINT * Height,
    OUT gctUINT * Depth
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Width != gcvNULL)
    {
        /* Return the width. */
        *Width =
#ifndef VIVANTE_NO_3D
            gcoMATH_DivideUInt(Surface->info.rect.right,
                                    Surface->info.samples.x);
#else
            Surface->info.rect.right;
#endif
    }

    if (Height != gcvNULL)
    {
        /* Return the height. */
        *Height =
#ifndef VIVANTE_NO_3D
            gcoMATH_DivideUInt(Surface->info.rect.bottom,
                                     Surface->info.samples.y);
#else
            Surface->info.rect.bottom;
#endif
    }

    if (Depth != gcvNULL)
    {
        /* Return the depth. */
        *Depth = Surface->depth;
    }

    /* Success. */
    gcmFOOTER_ARG("*Width=%u *Height=%u *Depth=%u",
                  (Width  == gcvNULL) ? 0 : *Width,
                  (Height == gcvNULL) ? 0 : *Height,
                  (Depth  == gcvNULL) ? 0 : *Depth);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetAlignedSize
**
**  Get the aligned size of an gcoSURF object.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT * Width
**          Pointer to variable that receives the aligned width of the gcoSURF
**          object.  If 'Width' is gcvNULL, no width information shall be returned.
**
**      gctUINT * Height
**          Pointer to variable that receives the aligned height of the gcoSURF
**          object.  If 'Height' is gcvNULL, no height information shall be
**          returned.
**
**      gctINT * Stride
**          Pointer to variable that receives the stride of the gcoSURF object.
**          If 'Stride' is gcvNULL, no stride information shall be returned.
*/
gceSTATUS
gcoSURF_GetAlignedSize(
    IN gcoSURF Surface,
    OUT gctUINT * Width,
    OUT gctUINT * Height,
    OUT gctINT * Stride
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Width != gcvNULL)
    {
        /* Return the aligned width. */
        *Width = Surface->info.alignedWidth;
    }

    if (Height != gcvNULL)
    {
        /* Return the aligned height. */
        *Height = Surface->info.alignedHeight;
    }

    if (Stride != gcvNULL)
    {
        /* Return the stride. */
        *Stride = Surface->info.stride;
    }

    /* Success. */
    gcmFOOTER_ARG("*Width=%u *Height=%u *Stride=%d",
                  (Width  == gcvNULL) ? 0 : *Width,
                  (Height == gcvNULL) ? 0 : *Height,
                  (Stride == gcvNULL) ? 0 : *Stride);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetAlignment
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceSURF_TYPE Type
**          Type of surface.
**
**      gceSURF_FORMAT Format
**          Format of surface.
**
**  OUTPUT:
**
**      gctUINT * addressAlignment
**          Pointer to the variable of address alignment.
**      gctUINT * xAlignmenet
**          Pointer to the variable of x Alignment.
**      gctUINT * yAlignment
**          Pointer to the variable of y Alignment.
*/
gceSTATUS
gcoSURF_GetAlignment(
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    OUT gctUINT * AddressAlignment,
    OUT gctUINT * XAlignment,
    OUT gctUINT * YAlignment
    )
{
    gceSTATUS status;
    gctUINT32 bitsPerPixel;
    gctUINT xAlign = (gcvSURF_TEXTURE == Type) ? 4 : 16;
    gctUINT yAlign = 4;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif

    gcmHEADER_ARG("Type=%d Format=%d", Type, Format);

    /* Compute alignment factors. */
    if (XAlignment != gcvNULL)
    {
        *XAlignment = xAlign;
    }

    if (YAlignment != gcvNULL)
    {
        *YAlignment = yAlign;
    }
#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentType);
    if (currentType == gcvHARDWARE_VG)
    {
        /* Compute bits per pixel. */
        gcmONERROR(gcoVGHARDWARE_ConvertFormat(gcvNULL,
                                             Format,
                                             &bitsPerPixel,
                                             gcvNULL));
    }
    else
#endif
    {
        /* Compute bits per pixel. */
        gcmONERROR(gcoHARDWARE_ConvertFormat(Format,
                                             &bitsPerPixel,
                                             gcvNULL));
    }

    if (AddressAlignment != gcvNULL)
    {
        *AddressAlignment = xAlign * yAlign * bitsPerPixel / 8;
    }

    gcmFOOTER_ARG("*XAlignment=0x%x  *YAlignment=0x%x *AddressAlignment=0x%x",
		gcmOPT_VALUE(XAlignment), gcmOPT_VALUE(YAlignment), gcmOPT_VALUE(AddressAlignment));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  gcoSURF_GetFormat
**
**  Get surface type and format.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gceSURF_TYPE * Type
**          Pointer to variable that receives the type of the gcoSURF object.
**          If 'Type' is gcvNULL, no type information shall be returned.
**
**      gceSURF_FORMAT * Format
**          Pointer to variable that receives the format of the gcoSURF object.
**          If 'Format' is gcvNULL, no format information shall be returned.
*/
gceSTATUS
gcoSURF_GetFormat(
    IN gcoSURF Surface,
    OUT gceSURF_TYPE * Type,
    OUT gceSURF_FORMAT * Format
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Type != gcvNULL)
    {
        /* Return the surface type. */
        *Type = Surface->info.type;
    }

    if (Format != gcvNULL)
    {
        /* Return the surface format. */
        *Format = Surface->info.format;
    }

    /* Success. */
    gcmFOOTER_ARG("*Type=%d *Format=%d",
                  (Type   == gcvNULL) ? 0 : *Type,
                  (Format == gcvNULL) ? 0 : *Format);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetTiling
**
**  Get surface tiling.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gceTILING * Tiling
**          Pointer to variable that receives the tiling of the gcoSURF object.
*/
gceSTATUS
gcoSURF_GetTiling(
    IN gcoSURF Surface,
    OUT gceTILING * Tiling
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if (Tiling != gcvNULL)
    {
        /* Return the surface tiling. */
        *Tiling = Surface->info.tiling;
    }

    /* Success. */
    gcmFOOTER_ARG("*Tiling=%d",
                  (Tiling == gcvNULL) ? 0 : *Tiling);
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_Lock
**
**  Lock the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Physical address array of the surface:
**          For YV12, Address[0] is for Y channel,
**                    Address[1] is for V channel and
**                    Address[2] is for U channel;
**          For I420, Address[0] is for Y channel,
**                    Address[1] is for U channel and
**                    Address[2] is for V channel;
**          For NV12, Address[0] is for Y channel and
**                    Address[1] is for UV channel;
**          For all other formats, only Address[0] is used to return the
**          physical address.
**
**      gctPOINTER * Memory
**          Logical address array of the surface:
**          For YV12, Memory[0] is for Y channel,
**                    Memory[1] is for V channel and
**                    Memory[2] is for U channel;
**          For I420, Memory[0] is for Y channel,
**                    Memory[1] is for U channel and
**                    Memory[2] is for V channel;
**          For NV12, Memory[0] is for Y channel and
**                    Memory[1] is for UV channel;
**          For all other formats, only Memory[0] is used to return the logical
**          address.
*/
gceSTATUS
gcoSURF_Lock(
    IN gcoSURF Surface,
    OPTIONAL OUT gctUINT32 * Address,
    OPTIONAL OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentType);
    /* Lock the surface. */
    gcmONERROR(_Lock(Surface, currentType));
#else
    /* Lock the surface. */
    gcmONERROR(_Lock(Surface));
#endif
    /* Determine surface addresses. */
    switch (Surface->info.format)
    {
    case gcvSURF_YV12:
    case gcvSURF_I420:
        Surface->info.node.count = 3;

        Surface->info.node.logical2  = Surface->info.node.logical
                                     + Surface->info.uOffset;

        Surface->info.node.physical2 = Surface->info.node.physical
                                     + Surface->info.uOffset;

        Surface->info.node.logical3  = Surface->info.node.logical
                                     + Surface->info.vOffset;

        Surface->info.node.physical3 = Surface->info.node.physical
                                     + Surface->info.vOffset;
        break;

    case gcvSURF_NV12:
    case gcvSURF_NV21:
    case gcvSURF_NV16:
    case gcvSURF_NV61:
        Surface->info.node.count = 2;

        Surface->info.node.logical2  = Surface->info.node.logical
                                     + Surface->info.uOffset;

        Surface->info.node.physical2 = Surface->info.node.physical
                                     + Surface->info.uOffset;
        break;

    default:
        Surface->info.node.count = 1;
    }

    /* Set result. */
    if (Address != gcvNULL)
    {
        if (Surface->info.format == gcvSURF_YV12)
        {
            Address[2] = Surface->info.node.physical2;
            Address[1] = Surface->info.node.physical3;
            Address[0] = Surface->info.node.physical;
        }
        else
        {
            switch (Surface->info.node.count)
            {
            case 3:
                Address[2] = Surface->info.node.physical3;

                /* FALLTHROUGH */
            case 2:
                Address[1] = Surface->info.node.physical2;

                /* FALLTHROUGH */
            case 1:
                Address[0] = Surface->info.node.physical;

                /* FALLTHROUGH */
            default:
                break;
            }
        }
    }

    if (Memory != gcvNULL)
    {
        if (Surface->info.format == gcvSURF_YV12)
        {
            Memory[2] = Surface->info.node.logical2;
            Memory[1] = Surface->info.node.logical3;
            Memory[0] = Surface->info.node.logical;
        }
        else
        {
            switch (Surface->info.node.count)
            {
            case 3:
                Memory[2] = Surface->info.node.logical3;

                /* FALLTHROUGH */
            case 2:
                Memory[1] = Surface->info.node.logical2;

                /* FALLTHROUGH */
            case 1:
                Memory[0] = Surface->info.node.logical;

                /* FALLTHROUGH */
            default:
                break;
            }
        }
    }

    /* Success. */
    gcmFOOTER_ARG("*Address=%08X *Memory=0x%x",
                  (Address == gcvNULL) ? 0 : *Address,
                  (Memory  == gcvNULL) ? gcvNULL : *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Unlock
**
**  Unlock the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctPOINTER Memory
**          Pointer to mapped memory.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Unlock(
    IN gcoSURF Surface,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif
    gcmHEADER_ARG("Surface=0x%x Memory=0x%x", Surface, Memory);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentType);
    /* Unlock the surface. */
    gcmONERROR(_Unlock(Surface, currentType));
#else
    /* Unlock the surface. */
    gcmONERROR(_Unlock(Surface));
#endif
    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Fill
**
**  Fill surface with specified value.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gcsPOINT_PTR Origin
**          Pointer to the origin of the area to be filled.
**          Assumed to (0, 0) if gcvNULL is given.
**
**      gcsSIZE_PTR Size
**          Pointer to the size of the area to be filled.
**          Assumed to the size of the surface if gcvNULL is given.
**
**      gctUINT32 Value
**          Value to be set.
**
**      gctUINT32 Mask
**          Value mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Fill(
    IN gcoSURF Surface,
    IN gcsPOINT_PTR Origin,
    IN gcsSIZE_PTR Size,
    IN gctUINT32 Value,
    IN gctUINT32 Mask
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoSURF_Blend
**
**  Alpha blend two surfaces together.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to the source gcoSURF object.
**
**      gcoSURF DestSurface
**          Pointer to the destination gcoSURF object.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin within the source.
**          If gcvNULL is specified, (0, 0) origin is assumed.
**
**      gcsPOINT_PTR DestOrigin
**          The origin within the destination.
**          If gcvNULL is specified, (0, 0) origin is assumed.
**
**      gcsSIZE_PTR Size
**          The size of the area to be blended.
**
**      gceSURF_BLEND_MODE Mode
**          One of the blending modes.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Blend(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsSIZE_PTR Size,
    IN gceSURF_BLEND_MODE Mode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoSURF_ClearRect
**
**  Clear a rectangular surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctINT Left
**          Left coordinate of rectangle to clear.
**
**      gctINT Top
**          Top coordinate of rectangle to clear.
**
**      gctINT Right
**          Right coordinate of rectangle to clear.
**
**      gctINT Bottom
**          Bottom coordinate of rectangle to clear.
**
**      gctUINT Flags
**          Flags to determine what should be cleared:
**
**              gcvCLEAR_COLOR   - Clear color.
**              gcvCLEAR_DEPTH   - Clear depth.
**              gcvCLEAR_STENCIL - Clear stencil.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ClearRect(
    IN gcoSURF Surface,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctUINT Flags
    )
{
    gceSTATUS status;
    gco3D engine;
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gctUINT flags = Flags;

    gcmHEADER_ARG("Surface=0x%x Left=%d Top=%d Right=%d Bottom=%d Flags=%u",
              Surface, Left, Top, Right, Bottom, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Flags != 0);

    if (Surface->info.vaa)
    {
        /* Add VAA to flags. */
        flags |= gcvCLEAR_HAS_VAA;
    }

    /* Adjust the rect according to the msaa sample. */
    Left   *= Surface->info.samples.x;
    Right  *= Surface->info.samples.x;
    Top    *= Surface->info.samples.y;
    Bottom *= Surface->info.samples.y;

    /* Test for entire surface clear. */
    if ((Left == 0)
    &&  (Top == 0)
    &&  (Right >= Surface->info.rect.right)
    &&  (Bottom >= Surface->info.rect.bottom)
    )
    {
        /* It is an entire surface clear. */
        status = gcoSURF_Clear(Surface, flags);
        gcmFOOTER();
        return status;
    }

    do
    {
        /* Extract the gco3D object pointer. */
        gcmERR_BREAK(gcoHAL_Get3DEngine(gcvNULL, &engine));

        /* Lock the surface. */
        gcmERR_BREAK(gcoSURF_Lock(Surface, address, memory));

        do
        {
            gctUINT32 stride = Surface->info.stride;

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                          "gcoSURF_Clear: Clearing surface 0x%x @ 0x%08X",
                          Surface,
                          Surface->info.node.physical);

            /* Flush the tile status and decompress the buffers. */
            gcmERR_BREAK(gcoSURF_DisableTileStatus(Surface, gcvTRUE));

            if (Surface->info.superTiled)
            {
                /* Set super-tiled flag. */
                stride |= (1U << 31);
            }

            if ((Surface->info.type == gcvSURF_RENDER_TARGET)
            ||  (Surface->info.type == gcvSURF_DEPTH)
            )
            {
                /* Set multi-tiled flag. */
                stride |= (1U << 30);
            }

            /* Clear the surface. */
            gcmERR_BREAK(
                gco3D_ClearRect(engine,
                                Surface->info.node.physical,
                                memory[0],
                                stride,
                                Surface->info.format,
                                Left, Top,
                                Right, Bottom,
                                Surface->info.alignedWidth,
                                Surface->info.alignedHeight,
                                flags));

            /* Flush the CPU cache. */
            status = gcoSURF_NODE_Cache(&Surface->info.node,
                                      memory[0],
                                      Surface->info.size,
                                      gcvCACHE_CLEAN);
        }
        while (gcvFALSE);

        /* Unlock the surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(Surface, memory[0]));
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Clear
**
**  Clear the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctUINT Flags
**          Flags to determine what should be cleared:
**
**              gcvCLEAR_COLOR   - Clear color.
**              gcvCLEAR_DEPTH   - Clear depth.
**              gcvCLEAR_STENCIL - Clear stencil.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Clear(
    IN gcoSURF Surface,
    IN gctUINT Flags
    )
{
    gceSTATUS status;
    gco3D engine;
    gctPOINTER memory[3] = {gcvNULL};
    gctUINT flags = Flags;
    gctUINT32 stride;

    gcmHEADER_ARG("Surface=0x%x Flags=%u", Surface, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Flags != 0);

    do
    {
        if (Surface->info.vaa)
        {
            /* Add VAA to flags. */
            flags |= gcvCLEAR_HAS_VAA;
        }

        /* Extract the gco3D object pointer. */
        gcmERR_BREAK(
            gcoHAL_Get3DEngine(gcvNULL, &engine));

        /* Lock the surface. */
        gcmERR_BREAK(
            gcoSURF_Lock(Surface, gcvNULL, memory));

        /* Flush the tile status before a clear. */
        gcmERR_BREAK(
            gcoHARDWARE_FlushTileStatus(&Surface->info,
                                        gcvFALSE));

        /* Attempt to clear using tile status. */
        if (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
        {
            do
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                              "gcoSURF_Clear: Clearing tile status 0x%x @ 0x%08X for"
                              "surface 0x%x",
                              Surface->info.tileStatusNode,
                              Surface->info.tileStatusNode.physical,
                              Surface);

                /* Clear the tile status. */
                status = gco3D_ClearTileStatus(engine,
                                               &Surface->info,
                                               Surface->info.tileStatusNode.physical,
                                               flags);

                if (gcmIS_ERROR(status))
                {
                    /* Print error only if it's really an error. */
                    if (status != gcvSTATUS_NOT_SUPPORTED)
                    {
                        gcmERR_BREAK(status);
                    }

                    /* Otherwise just quietly return. */
                    break;
                }

                if (gcmIS_SUCCESS(status)
                &&  (flags & gcvCLEAR_DEPTH)
                &&  (Surface->info.hzTileStatusNode.pool != gcvPOOL_UNKNOWN)
                )
                {
                    /* Clear the hierarchical Z tile status. */
                    gcmERR_BREAK(
                        gco3D_ClearHzTileStatus(engine,
                                                &Surface->info,
                                                &Surface->info.hzTileStatusNode));
                }

                if (status == gcvSTATUS_SKIP)
                {
                    /* Nothing needed clearing, and no error has occurred. */
                    status = gcvSTATUS_OK;
                }
                else
                {
                    /* Turn the tile status on again. */
                    Surface->info.tileStatusDisabled = gcvFALSE;

                    /* Reset the tile status. */
                    gcmERR_BREAK(
                        gcoSURF_EnableTileStatus(Surface));
                }
            }
            while (gcvFALSE);

            /* Success? */
            if (gcmIS_SUCCESS(status))
            {
                break;
            }
        }

        /* Clear using conventional means. */
        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_SURFACE,
                      "gcoSURF_Clear: Clearing surface 0x%x @ 0x%08X",
                      Surface,
                      Surface->info.node.physical);

        /* Disable the tile status. */
        gcmERR_BREAK(
            gcoHARDWARE_DisableTileStatus(&Surface->info,
                                          gcvTRUE));

        stride = Surface->info.stride;

        if (Surface->info.superTiled)
        {
            /* Set super tiled flag. */
            stride |= (1U << 31);
        }

        if ((Surface->info.type == gcvSURF_RENDER_TARGET)
        ||  (Surface->info.type == gcvSURF_DEPTH)
        )
        {
            /* Set multi-tiled flag. */
            stride |= (1U << 30);
        }

        /* Clear the surface. */
        gcmERR_BREAK(
            gco3D_ClearRect(engine,
                            Surface->info.node.physical,
                            memory[0],
                            stride,
                            Surface->info.format,
                            0,
                            0,
                            Surface->info.alignedWidth,
                            Surface->info.alignedHeight,
                            Surface->info.alignedWidth,
                            Surface->info.alignedHeight,
                            flags));

        if ((flags & gcvCLEAR_DEPTH)
        &&  (Surface->info.hzNode.size > 0)
        )
        {
            gcmERR_BREAK(
                gco3D_ClearRect(engine,
                                Surface->info.hzNode.physical,
                                Surface->info.hzNode.logical,
                                Surface->info.hzNode.size,
                                gcvSURF_UNKNOWN,
                                0,
                                0,
                                1,
                                1,
                                1,
                                1,
                                gcvCLEAR_HZ));
        }
    }
    while (gcvFALSE);

    /* Unlock the surface. */
    if (memory[0] != gcvNULL)
    {
        status = gcoSURF_Unlock(Surface, memory[0]);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  depr_gcoSURF_Resolve
**
**  Resolve the surface to the frame buffer.  Resolve means that the frame is
**  finished and should be displayed into the frame buffer, either by copying
**  the data or by flipping to the surface, depending on the hardware's
**  capabilities.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DestSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into, or gcvNULL if the resolve surface is
**          a physical address.
**
**      gctUINT32 DestAddress
**          Physical address of the destination surface.
**
**      gctPOINTER DestBits
**          Logical address of the destination surface.
**
**      gctINT DestStride
**          Stride of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_TYPE DestType
**          Type of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_FORMAT DestFormat
**          Format of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DestWidth
**          Width of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DestHeight
**          Height of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
depr_gcoSURF_Resolve(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gctUINT32 DestAddress,
    IN gctPOINTER DestBits,
    IN gctINT DestStride,
    IN gceSURF_TYPE DestType,
    IN gceSURF_FORMAT DestFormat,
    IN gctUINT DestWidth,
    IN gctUINT DestHeight
    )
{
    gcsPOINT rectOrigin;
    gcsPOINT rectSize;
    gceSTATUS status;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x DestAddress=%08x DestBits=0x%x "
              "DestStride=%d DestType=%d DestFormat=%d DestWidth=%u "
              "DestHeight=%u",
              SrcSurface, DestSurface, DestAddress, DestBits, DestStride,
              DestType, DestFormat, DestWidth, DestHeight);

    /* Validate the source surface. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);

    /* Fill in coordinates. */
    rectOrigin.x = 0;
    rectOrigin.y = 0;

    if (DestSurface == gcvNULL)
    {
        rectSize.x = DestWidth;
        rectSize.y = DestHeight;
    }
    else
    {
        rectSize.x = DestSurface->info.alignedWidth;
        rectSize.y = DestSurface->info.alignedHeight;
    }

    /* Call generic function. */
    status = depr_gcoSURF_ResolveRect(
        SrcSurface, DestSurface,
        DestAddress, DestBits, DestStride, DestType, DestFormat,
        DestWidth, DestHeight,
        &rectOrigin, &rectOrigin, &rectSize
        );
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  depr_gcoSURF_ResolveRect
**
**  Resolve a rectangluar area of a surface to another surface.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DestSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into, or gcvNULL if the resolve surface is
**          a physical address.
**
**      gctUINT32 DestAddress
**          Physical address of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gctPOINTER DestBits
**          Logical address of the destination surface.
**
**      gctINT DestStride
**          Stride of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_TYPE DestType
**          Type of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gceSURF_FORMAT DestFormat
**          Format of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DestWidth
**          Width of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gctUINT DestHeight
**          Height of the destination surface.
**          If 'DestSurface' is not gcvNULL, this parameter is ignored.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be resolved.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be resolved.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
depr_gcoSURF_ResolveRect(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gctUINT32 DestAddress,
    IN gctPOINTER DestBits,
    IN gctINT DestStride,
    IN gceSURF_TYPE DestType,
    IN gceSURF_FORMAT DestFormat,
    IN gctUINT DestWidth,
    IN gctUINT DestHeight,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctPOINTER destination[3] = {gcvNULL};
    gctPOINTER mapInfo = gcvNULL;
    gcsSURF_INFO_PTR destInfo;
    struct _gcsSURF_INFO _destInfo;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x DestAddress=%08x DestBits=0x%x "
              "DestStride=%d DestType=%d DestFormat=%d DestWidth=%u "
              "DestHeight=%u SrcOrigin=0x%x DestOrigin=0x%x RectSize=0x%x",
              SrcSurface, DestSurface, DestAddress, DestBits, DestStride,
              DestType, DestFormat, DestWidth, SrcOrigin, DestOrigin,
              RectSize);

    /* Validate the source surface. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);

    do
    {
        gcsPOINT rectSize;

        /* Destination surface specified? */
        if (DestSurface != gcvNULL)
        {
            /* Set the pointer to the structure. */
            destInfo = &DestSurface->info;

            /* Lock the surface. */
            if (DestBits == gcvNULL)
            {
                gcmERR_BREAK(
                    gcoSURF_Lock(DestSurface,
                                 gcvNULL,
                                 destination));
            }
        }

        /* Create a surface wrapper. */
        else
        {
            /* Fill the surface info structure. */
            _destInfo.type          = DestType;
            _destInfo.format        = DestFormat;
            _destInfo.rect.left     = 0;
            _destInfo.rect.top      = 0;
            _destInfo.rect.right    = DestWidth;
            _destInfo.rect.bottom   = DestHeight;
            _destInfo.alignedWidth  = DestWidth;
            _destInfo.alignedHeight = DestHeight;
            _destInfo.rotation      = gcvSURF_0_DEGREE;
            _destInfo.orientation   = gcvORIENTATION_TOP_BOTTOM;
            _destInfo.stride        = DestStride;
            _destInfo.size          = DestStride * DestHeight;
            _destInfo.node.valid    = gcvTRUE;
            _destInfo.node.pool     = gcvPOOL_UNKNOWN;
            _destInfo.node.physical = DestAddress;
            _destInfo.node.logical  = DestBits;
            _destInfo.samples.x     = 1;
            _destInfo.samples.y     = 1;

            /* Set the pointer to the structure. */
            destInfo = &_destInfo;

            gcmERR_BREAK(
                gcoHARDWARE_AlignToTileCompatible(DestType,
                                                  DestFormat,
                                                  &_destInfo.alignedWidth,
                                                  &_destInfo.alignedHeight,
                                                  &_destInfo.tiling,
                                                  &_destInfo.superTiled));

            /* Map the user memory. */
            if (DestBits != gcvNULL)
            {
                gcmERR_BREAK(
                    gcoOS_MapUserMemory(gcvNULL,
                                        DestBits,
                                        destInfo->size,
                                        &mapInfo,
                                        &destInfo->node.physical));
            }
        }

        /* Determine the resolve size. */
        if ((DestOrigin->x == 0)
        &&  (DestOrigin->y == 0)
        &&  (RectSize->x == destInfo->rect.right)
        &&  (RectSize->y == destInfo->rect.bottom)
        )
        {
            /* Full destination resolve, a special case. */
            rectSize.x = destInfo->alignedWidth;
            rectSize.y = destInfo->alignedHeight;
        }
        else
        {
            rectSize.x = RectSize->x;
            rectSize.y = RectSize->y;
        }

        /* Perform resolve. */
        gcmERR_BREAK(
            gcoHARDWARE_ResolveRect(&SrcSurface->info,
                                    destInfo,
                                    SrcOrigin,
                                    DestOrigin,
                                    &rectSize));
    }
    while (gcvFALSE);

    /* Unlock the surface. */
    if (destination[0] != gcvNULL)
    {
        /* Unlock the resolve buffer. */
        gcmVERIFY_OK(
            gcoSURF_Unlock(DestSurface,
                           destination[0]));
    }

    /* Unmap the surface. */
    if (mapInfo != gcvNULL)
    {
        /* Unmap the user memory. */
        gcmVERIFY_OK(
            gcoHAL_ScheduleUnmapUserMemory(gcvNULL,
                                           mapInfo,
                                           destInfo->size,
                                           destInfo->node.physical,
                                           DestBits));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Resample
**
**  Determine the ratio between the two non-multisampled surfaces and resample
**  based on the ratio.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface.
**
**      gcoSURF DestSurface
**          Pointer to a gcoSURF object that represents the destination surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Resample(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface
    )
{
    gceSTATUS status;
    gcsSAMPLES srcSamples = { 0, 0 };
    gcsSAMPLES destSamples = { 0, 0 };
    gcsPOINT rectOrigin = { 0, 0 };
    gcsPOINT rectSize;
    gctUINT32 srcPhysical[3] = {~0U}, destPhysical[3] = {~0U};
    gctPOINTER srcLogical[3] = {gcvNULL}, destLogical[3] = {gcvNULL};
    gctUINT i;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x", SrcSurface, DestSurface);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(DestSurface, gcvOBJ_SURF);

    /* Both surfaces have to be non-multisampled. */
    if ((SrcSurface->info.samples.x  != 1)
    ||  (SrcSurface->info.samples.y  != 1)
    ||  (DestSurface->info.samples.x != 1)
    ||  (DestSurface->info.samples.y != 1)
    )
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Determine the samples and fill in coordinates. */
    if (SrcSurface->info.rect.right == DestSurface->info.rect.right)
    {
        srcSamples.x  = 1;
        destSamples.x = 1;
        rectSize.x    = SrcSurface->info.alignedWidth;
    }

    else if (SrcSurface->info.rect.right == DestSurface->info.rect.right * 2)
    {
        srcSamples.x  = 2;
        destSamples.x = 1;
        rectSize.x    = DestSurface->info.alignedWidth;
    }

    else if (SrcSurface->info.rect.right * 2 == DestSurface->info.rect.right)
    {
        srcSamples.x  = 1;
        destSamples.x = 2;
        rectSize.x    = SrcSurface->info.alignedWidth;
    }

    else
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (SrcSurface->info.rect.bottom == DestSurface->info.rect.bottom)
    {
        srcSamples.y  = 1;
        destSamples.y = 1;
        rectSize.y    = SrcSurface->info.alignedHeight;
    }

    else if (SrcSurface->info.rect.bottom == DestSurface->info.rect.bottom * 2)
    {
        srcSamples.y  = 2;
        destSamples.y = 1;
        rectSize.y    = DestSurface->info.alignedHeight;
    }

    else if (SrcSurface->info.rect.bottom * 2 == DestSurface->info.rect.bottom)
    {
        srcSamples.y  = 1;
        destSamples.y = 2;
        rectSize.y    = SrcSurface->info.alignedHeight;
    }

    else
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Overwrite surface samples. */
    SrcSurface->info.samples  = srcSamples;
    DestSurface->info.samples = destSamples;

    /* Verify the two surfaces have the same depth. */
    if (SrcSurface->depth != DestSurface->depth)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (SrcSurface->depth > 1)
    {
        /* Lock source surface if larger than 1 depth. */
        gcmONERROR(gcoSURF_Lock(SrcSurface, srcPhysical, srcLogical));

        /* Lock destination surface if larger than 1 depth. */
        gcmONERROR(gcoSURF_Lock(DestSurface, destPhysical, destLogical));

        for (i = 0; i < SrcSurface->depth; ++i)
        {
            /* Call generic function. */
            gcmONERROR(gcoSURF_ResolveRect(SrcSurface,
                                           DestSurface,
                                           &rectOrigin,
                                           &rectOrigin,
                                           &rectSize));

            /* Move to next depth. */
            SrcSurface->info.node.physical  += SrcSurface->info.size;
            SrcSurface->info.node.logical   += SrcSurface->info.size;
            DestSurface->info.node.physical += DestSurface->info.size;
            DestSurface->info.node.logical  += DestSurface->info.size;
        }

        /* Restore addresses. */
        SrcSurface->info.node.physical  = srcPhysical[0];
        SrcSurface->info.node.logical   = srcLogical[0];
        DestSurface->info.node.physical = destPhysical[0];
        DestSurface->info.node.logical  = destLogical[0];

        /* Unlock surfaces. */
        gcmVERIFY_OK(gcoSURF_Unlock(SrcSurface,  srcLogical[0]));
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destLogical[0]));
    }
    else
    {
        /* Call generic function. */
        gcmONERROR(gcoSURF_ResolveRect(SrcSurface,
                                       DestSurface,
                                       &rectOrigin,
                                       &rectOrigin,
                                       &rectSize));
    }

    /* Restore samples. */
    SrcSurface->info.samples.x  = 1;
    SrcSurface->info.samples.y  = 1;
    DestSurface->info.samples.x = 1;
    DestSurface->info.samples.y = 1;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Restore samples. */
    SrcSurface->info.samples.x  = 1;
    SrcSurface->info.samples.y  = 1;
    DestSurface->info.samples.x = 1;
    DestSurface->info.samples.y = 1;

    if (srcLogical[0] != gcvNULL)
    {
        /* Restore addresses. */
        SrcSurface->info.node.physical = srcPhysical[0];
        SrcSurface->info.node.logical  = srcLogical[0];

        /* Unlock surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(SrcSurface, srcLogical[0]));


    }

    if (destLogical[0] != gcvNULL)
    {
        /* Restore addresses. */
        DestSurface->info.node.physical = destPhysical[0];
        DestSurface->info.node.logical  = destLogical[0];

        /* Unlock surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destLogical[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Resolve
**
**  Resolve the surface to the frame buffer.  Resolve means that the frame is
**  finished and should be displayed into the frame buffer, either by copying
**  the data or by flipping to the surface, depending on the hardware's
**  capabilities.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DestSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Resolve(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface
    )
{
    gcsPOINT rectOrigin;
    gcsPOINT rectSize;
    gceSTATUS status;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x", SrcSurface, DestSurface);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(DestSurface, gcvOBJ_SURF);

    /* Fill in coordinates. */
    rectOrigin.x = 0;
    rectOrigin.y = 0;
    rectSize.x   = DestSurface->info.alignedWidth;
    rectSize.y   = DestSurface->info.alignedHeight;

    /* Call generic function. */
    status = gcoSURF_ResolveRect(
        SrcSurface, DestSurface,
        &rectOrigin, &rectOrigin, &rectSize
        );

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_PrepareRemoteResolveRect
**
**  Save the resovle info the kernel.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be resolved.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be resolved.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_PrepareRemoteResolveRect(
    IN gcoSURF SrcSurface,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gcsVIDMEM_NODE_SHARED_INFO DynamicInfo;

    gcmHEADER_ARG("SrcSurface=0x%x SrcOrigin=0x%x "
                  "DestOrigin=0x%x RectSize=0x%x",
                  SrcSurface, SrcOrigin, DestOrigin, RectSize);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(SrcOrigin != gcvNULL);
    gcmVERIFY_ARGUMENT(DestOrigin != gcvNULL);
    gcmVERIFY_ARGUMENT(RectSize != gcvNULL);

    do
    {
        gcmERR_BREAK(gcoHARDWARE_FlushPipe());
        gcmERR_BREAK(gcoHARDWARE_FlushTileStatus(&SrcSurface->info,
                                                 gcvFALSE));
        DynamicInfo.SrcOrigin.x = SrcOrigin->x;
        DynamicInfo.SrcOrigin.y = SrcOrigin->y;
        DynamicInfo.DestOrigin.x = DestOrigin->x;
        DynamicInfo.DestOrigin.y = DestOrigin->y;
        DynamicInfo.RectSize.width = RectSize->x;
        DynamicInfo.RectSize.height = RectSize->y;
        DynamicInfo.tileStatusDisabled = SrcSurface->info.tileStatusDisabled;
        DynamicInfo.clearValue = SrcSurface->info.clearValue;

        gcmERR_BREAK(gcoHAL_SetSharedInfo(0,
                                          gcvNULL,
                                          0,
                                          SrcSurface->info.node.u.normal.node,
                                          (gctUINT8_PTR)(&DynamicInfo),
                                          gcvVIDMEM_INFO_GENERIC));

    }
    while(gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_ResolveFromStoredRect
**
**  Resolve using the rectangle info previously saved in the vid mem node,
**  possibly by another process. Rectangle must be aligned.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DestSurface
**          gcoSURF object that represents the destination surface
**          to resolve into.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ResolveFromStoredRect(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsPOINT Origin, RectSize;
    gcsVIDMEM_NODE_SHARED_INFO DynamicInfo;
    gctBOOL fullSize;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x",
                  SrcSurface, DestSurface);

    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(DestSurface, gcvOBJ_SURF);

    do
    {
        gcmERR_BREAK(gcoHAL_GetSharedInfo(0,
                                          0,
                                          gcvNULL,
                                          0,
                                          SrcSurface->info.node.u.normal.node,
                                          (gctUINT8_PTR)(&DynamicInfo),
                                          gcvVIDMEM_INFO_DIRTY_RECTANGLE));

        fullSize = ((DynamicInfo.RectSize.width == 0) ||
                            (DynamicInfo.RectSize.height == 0) ||
                            ((gctUINT)DynamicInfo.SrcOrigin.x + (gctUINT)DynamicInfo.RectSize.width > SrcSurface->info.alignedWidth) ||
                            ((gctUINT)DynamicInfo.SrcOrigin.y + (gctUINT)DynamicInfo.RectSize.height > SrcSurface->info.alignedHeight));

        if (fullSize)
        {
            gcmERR_BREAK(gcoSURF_Resolve(SrcSurface,
                                         DestSurface));
        }
        else
        {
            /* Src and Dst have identical geometries, so we use the same origin and size. */
            Origin.x = DynamicInfo.SrcOrigin.x;
            Origin.y = DynamicInfo.SrcOrigin.y;
            RectSize.x = DynamicInfo.RectSize.width;
            RectSize.y = DynamicInfo.RectSize.height;

            gcmERR_BREAK(gcoSURF_ResolveRect(SrcSurface,
                                             DestSurface,
                                             &Origin,
                                             &Origin,
                                             &RectSize));
        }
    }
    while(gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_ExportRenderTarget
**
**  Save the Surface Info into the kernel.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ExportRenderTarget(
    IN gcoSURF SrcSurface
)
{
    gceSTATUS status;

    gcmHEADER_ARG("SrcSurface=0x%x", SrcSurface);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    do
    {
        gcmERR_BREAK(gcoHAL_SetSharedInfo((gctUINT32)gcoOS_GetCurrentProcessID(),
                                          (gctUINT8_PTR)(SrcSurface),
                                          sizeof(*SrcSurface),
                                          gcvNULL,
                                          0,
                                          gcvVIDMEM_INFO_GENERIC));
    }
    while(gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_ImportRenderTarget
**
**  Save the Surface Info into the kernel.
**
**  INPUT:
**
**      gctUINT32 Pid
**          The process ID that saved the info.
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ImportRenderTarget(
    IN gctUINT32 Pid,
    IN gcoSURF SrcSurface
)
{
    gceSTATUS status;

    gcmHEADER_ARG("Pid=%d SrcSurface=0x%x", Pid, SrcSurface);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    do
    {
        gcmERR_BREAK(gcoHAL_GetSharedInfo(Pid,
                                          Pid,
                                          (gctUINT8_PTR)(SrcSurface),
                                          sizeof(*SrcSurface),
                                          gcvNULL,
                                          0,
                                          gcvVIDMEM_INFO_GENERIC));
    }
    while(gcvFALSE);
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_RemoteResolveRect
**
**  Resolve using the info that process Pid saved.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DestSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_RemoteResolveRect(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gctBOOL *resolveDiscarded
    )
{
    gceSTATUS status;
    gcsPOINT SrcOrigin, DestOrigin, RectSize;
    gctPOINTER source[3];
    gctBOOL savedtileStatusDisabled;
    gcsSURF_INFO_PTR currentSurface = gcvNULL;
    gcsVIDMEM_NODE_SHARED_INFO DynamicInfo;
    gctUINT32 savedclearValue;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x",
                  SrcSurface, DestSurface);

    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(DestSurface, gcvOBJ_SURF);
    do
    {
        gcmERR_BREAK(gcoHAL_GetSharedInfo(0,
                                          0,
                                          gcvNULL,
                                          0,
                                          SrcSurface->info.node.u.normal.node,
                                          (gctUINT8_PTR)(&DynamicInfo),
                                          gcvVIDMEM_INFO_GENERIC));
        if ((DynamicInfo.RectSize.width == 0) || (DynamicInfo.RectSize.height == 0))
        {
            /* Discard. */
            *resolveDiscarded = gcvTRUE;
            break;
        }

        savedtileStatusDisabled = SrcSurface->info.tileStatusDisabled;
        savedclearValue = SrcSurface->info.clearValue;
        SrcOrigin.x = DynamicInfo.SrcOrigin.x;
        SrcOrigin.y = DynamicInfo.SrcOrigin.y;
        DestOrigin.x = DynamicInfo.DestOrigin.x;
        DestOrigin.y = DynamicInfo.DestOrigin.y;
        RectSize.x = DynamicInfo.RectSize.width;
        RectSize.y = DynamicInfo.RectSize.height;
        SrcSurface->info.tileStatusDisabled = DynamicInfo.tileStatusDisabled;
        SrcSurface->info.clearValue = DynamicInfo.clearValue;

        gcmERR_BREAK(gcoHARDWARE_FlushPipe());
        gcmERR_BREAK(gcoHARDWARE_FlushTileStatus(&SrcSurface->info,
                                                 gcvFALSE));

        gcmERR_BREAK(gcoHARDWARE_GetCurrentSurface(&currentSurface));
        gcmERR_BREAK(gcoHARDWARE_SetCurrentSurface(&SrcSurface->info));
        gcmERR_BREAK(gcoSURF_Lock(SrcSurface, gcvNULL, source));
        gcmERR_BREAK(gcoSURF_SetTileStatus(SrcSurface));
        gcmERR_BREAK(gcoSURF_ResolveRect(SrcSurface,
                                         DestSurface,
                                         &SrcOrigin,
                                         &DestOrigin,
                                         &RectSize));

        gcmERR_BREAK(gcoSURF_DisableTileStatus(SrcSurface, gcvFALSE));
        gcmERR_BREAK(gcoHARDWARE_SetCurrentSurface(currentSurface));
        SrcSurface->info.tileStatusDisabled = savedtileStatusDisabled;
        SrcSurface->info.clearValue = savedclearValue;
        gcmERR_BREAK(gcoSURF_Unlock(SrcSurface, source[0]));
    }
    while(gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_GetRTSignal
**
**  Return the "resolve submitted indicator" signal.
**
**  INPUT:
**
**      gcoSURF RTSurface
**          A gcoSURF object that represents a render target
**
**  OUTPUT:
**
**      resolveSubmittedSignal
**          Signal used by SF to indicate that resolve for this surface
**          has been submitted to hardware.
*/
gceSTATUS
gcoSURF_GetRTSignal(
    IN gcoSURF RTSurface,
    OUT gctSIGNAL * resolveSubmittedSignal
    )
{
#if defined(ANDROID) && gcdDEFER_RESOLVES
    gcmHEADER_ARG("RTSurface=0x%x resolveSubmittedSignal=0x%x",
                  RTSurface, resolveSubmittedSignal);

    gcmVERIFY_OBJECT(RTSurface, gcvOBJ_SURF);
    gcmASSERT(RTSurface->info.type == gcvSURF_RENDER_TARGET);

    *resolveSubmittedSignal = RTSurface->resolveSubmittedSignal;

    /* Return the status. */
    gcmFOOTER_NO();
#endif
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_ResolveRect
**
**  Resolve a rectangluar area of a surface to another surface.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to a gcoSURF object that represents the source surface
**          to be resolved.
**
**      gcoSURF DestSurface
**          Pointer to a gcoSURF object that represents the destination surface
**          to resolve into.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be resolved.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be resolved.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ResolveRect(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctPOINTER source[3];
    gctPOINTER target[3];

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x SrcOrigin=0x%x "
                  "DestOrigin=0x%x RectSize=0x%x",
                  SrcSurface, DestSurface, SrcOrigin, DestOrigin, RectSize);

    /* Validate the surfaces. */
    gcmVERIFY_OBJECT(SrcSurface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(DestSurface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(SrcOrigin != gcvNULL);
    gcmVERIFY_ARGUMENT(DestOrigin != gcvNULL);
    gcmVERIFY_ARGUMENT(RectSize != gcvNULL);

    do
    {
        gcsPOINT rectSize;
        gctUINT32 physical[3] = {0};
        gctINT maxWidth;
        gctINT maxHeight;

        /* Reset lock pointers. */
        source[0] = gcvNULL;
        target[0] = gcvNULL;

        /* Lock the surfaces. */
        gcmERR_BREAK(gcoSURF_Lock(SrcSurface, gcvNULL, source));
        gcmERR_BREAK(gcoSURF_Lock(DestSurface, physical, target));

        gcmERR_BREAK(
            gcoHARDWARE_FlushTileStatus(&SrcSurface->info,
                                        gcvFALSE));

#ifndef __QNXNTO__
        /* Fix bug3508, there is garbage when running tutorial3_es20 on BG2 Android*/
        if (SrcSurface->info.type == gcvSURF_TEXTURE)
        {
            gcoSURF_CPUCacheOperation(SrcSurface, gcvCACHE_INVALIDATE);
        }

        if (SrcSurface->info.type == gcvSURF_BITMAP)
        {
            /* Flush the CPU cache. Source would've been rendered by the CPU. */
            gcmERR_BREAK(gcoSURF_NODE_Cache(
                &SrcSurface->info.node,
                source[0],
                SrcSurface->info.size,
                gcvCACHE_CLEAN));
        }

        if (DestSurface->info.type == gcvSURF_BITMAP)
        {
            gcmERR_BREAK(gcoSURF_NODE_Cache(
                &DestSurface->info.node,
                target[0],
                DestSurface->info.size,
                gcvCACHE_FLUSH));
        }
#endif

        /* Determine the resolve size. */
        if ((DestOrigin->x == 0) &&
            (DestOrigin->y == 0) &&
            (RectSize->x == DestSurface->info.rect.right) &&
            (RectSize->y == DestSurface->info.rect.bottom))
        {
            /* Full destination resolve, a special case. */
            rectSize.x = DestSurface->info.alignedWidth;
            rectSize.y = DestSurface->info.alignedHeight;
        }
        else
        {
            rectSize.x = RectSize->x;
            rectSize.y = RectSize->y;
        }

        /* Disable the source surface alignment for now. This in some cases
           will cause resolve to go beyond the source surface (might happen
           for texture, for example, since it is 4x4 and not 16x4 aligned),
           but since this is not destructive behaviour, it should be OK for
           now. Currently, if the source algnment is enabled, it will cause
           gcoHARDWARE_ResolveRect to choose a different way of resolving,
           which is using 2D and has other issues as well and will fail. */
#ifndef __QNXNTO__
        /* Make sure we don't go beyond the source surface. */
        maxWidth  = SrcSurface->info.alignedWidth  - SrcOrigin->x;
        maxHeight = SrcSurface->info.alignedHeight - SrcOrigin->y;

        rectSize.x = gcmMIN(maxWidth,  rectSize.x);
        rectSize.y = gcmMIN(maxHeight, rectSize.y);
#endif

        /* Make sure we don't go beyond the target surface. */
        maxWidth  = DestSurface->info.alignedWidth  - DestOrigin->x;
        maxHeight = DestSurface->info.alignedHeight - DestOrigin->y;

        rectSize.x = gcmMIN(maxWidth,  rectSize.x);
        rectSize.y = gcmMIN(maxHeight, rectSize.y);

        if (DestSurface->info.hzNode.valid)
        {
            /* Disable any HZ attached to destination. */
            DestSurface->info.hzNode.size = 0;
        }


        /* Special case a resolve from the depth buffer with tile status. */
        if ((SrcSurface->info.type == gcvSURF_DEPTH)
        &&  (SrcSurface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN)
        )
        {
            /* Resolve a depth buffer. */
            gcmERR_BREAK(
                gcoHARDWARE_ResolveDepth((SrcSurface->info.tileStatusNode.pool == gcvPOOL_UNKNOWN)
                                         ? ~0U
                                         : SrcSurface->info.tileStatusNode.physical,
                                         &SrcSurface->info,
                                         &DestSurface->info,
                                         SrcOrigin,
                                         DestOrigin,
                                         &rectSize));
        }
        else
        {
#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
            if(SrcSurface->info.linearResolvePhysical != 0 ||
               SrcSurface->info.linearResolveLogical  != 0 )
            {
                gctUINT32 physical = SrcSurface->info.node.physical;
                gctPOINTER logical = SrcSurface->info.node.logical;

                SrcSurface->info.node.physical  = SrcSurface->info.linearResolvePhysical;
                SrcSurface->info.node.logical   = SrcSurface->info.linearResolveLogical;

                /* Perform resolve. */
                gcmERR_BREAK(
                    gcoHARDWARE_ResolveRect(&SrcSurface->info,
                                            &DestSurface->info,
                                            SrcOrigin,
                                            DestOrigin,
                                            &rectSize));

                SrcSurface->info.node.physical = physical;
                SrcSurface->info.node.logical  = logical;
            }
            else
#endif
            {
            /* Perform resolve. */
            gcmERR_BREAK(
                gcoHARDWARE_ResolveRect(&SrcSurface->info,
                                        &DestSurface->info,
                                        SrcOrigin,
                                        DestOrigin,
                                        &rectSize));
            }
        }
    }
    while (gcvFALSE);

    /* Unlock the surfaces. */
    if (target[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, target[0]));
    }

    if (source[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(SrcSurface, source[0]));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoSURF_SetClipping
**
**  Set cipping rectangle to the size of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetClipping(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        gco2D engine;
        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));
        gcmERR_BREAK(gco2D_SetClipping(engine, &Surface->info.rect));
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Clear2D
**
**  Clear one or more rectangular areas.
**
**  INPUT:
**
**      gcoSURF DestSurface
**          Pointer to the destination surface.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of rectangles
**          pointed to by Rect parameter must have at least RectCount items.
**          Note, that for masked source blits only one destination rectangle
**          is supported.
**
**      gcsRECT_PTR DestRect
**          Pointer to a list of destination rectangles.
**
**      gctUINT32 LoColor
**          Low 32-bit clear color values.
**
**      gctUINT32 HiColor
**          high 32-bit clear color values.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Clear2D(
    IN gcoSURF DestSurface,
    IN gctUINT32 RectCount,
    IN gcsRECT_PTR DestRect,
    IN gctUINT32 LoColor,
    IN gctUINT32 HiColor
    )
{
    gceSTATUS status;
    gctPOINTER destMemory[3] = {gcvNULL};
    gco2D engine;

    gcmHEADER_ARG("DestSurface=0x%x RectCount=%u DestRect=0x%x LoColor=%u HiColor=%u",
              DestSurface, RectCount, DestRect, LoColor, HiColor);

    do
    {
        /* Validate the object. */
        gcmBADOBJECT_BREAK(DestSurface, gcvOBJ_SURF);

        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));

        /* Use surface rect if not specified. */
        if (DestRect == gcvNULL)
        {
            if (RectCount != 1)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            DestRect = &DestSurface->info.rect;
        }

        /* Lock the destination. */
        gcmERR_BREAK(gcoSURF_Lock(
            DestSurface,
            gcvNULL,
            destMemory
            ));

        /* Program the destination. */
        gcmERR_BREAK(gco2D_SetTargetEx(
            engine,
            DestSurface->info.node.physical,
            DestSurface->info.stride,
            DestSurface->info.rotation,
            DestSurface->info.alignedWidth,
            DestSurface->info.alignedHeight
            ));

        gcmERR_BREAK(gco2D_SetTransparencyAdvanced(
            engine,
            gcv2D_OPAQUE,
            gcv2D_OPAQUE,
            gcv2D_OPAQUE
            ));

        /* Form a CLEAR command. */
        gcmERR_BREAK(gco2D_Clear(
            engine,
            RectCount,
            DestRect,
            LoColor,
            gcvFALSE,
            0xCC,
            0xCC
            ));
    }
    while (gcvFALSE);

    /* Unlock the destination. */
    if (destMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destMemory[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Line
**
**  Draw one or more Bresenham lines.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object.
**
**      gctUINT32 LineCount
**          The number of lines to draw. The array of line positions pointed
**          to by Position parameter must have at least LineCount items.
**
**      gcsRECT_PTR Position
**          Points to an array of positions in (x0, y0)-(x1, y1) format.
**
**      gcoBRUSH Brush
**          Brush to use for drawing.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Line(
    IN gcoSURF DestSurface,
    IN gctUINT32 LineCount,
    IN gcsRECT_PTR Position,
    IN gcoBRUSH Brush,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop
    )
{
    gceSTATUS status;
    gctPOINTER destMemory[3] = {gcvNULL};
    gco2D engine;

    gcmHEADER_ARG("DestSurface=0x%x LineCount=%u Position=0x%x Brush=0x%x FgRop=%02x "
              "BgRop=%02x",
              DestSurface, LineCount, Position, Brush, FgRop, BgRop);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(DestSurface, gcvOBJ_SURF);

    do
    {
        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));

        /* Lock the destination. */
        gcmERR_BREAK(gcoSURF_Lock(
            DestSurface,
            gcvNULL,
            destMemory
            ));

        /* Program the destination. */
        gcmERR_BREAK(gco2D_SetTargetEx(
            engine,
            DestSurface->info.node.physical,
            DestSurface->info.stride,
            DestSurface->info.rotation,
            DestSurface->info.alignedWidth,
            DestSurface->info.alignedHeight
            ));

        gcmERR_BREAK(gco2D_SetTransparencyAdvanced(
            engine,
            gcv2D_OPAQUE,
            gcv2D_OPAQUE,
            gcv2D_OPAQUE
            ));

        /* Draw a LINE command. */
        gcmERR_BREAK(gco2D_Line(
            engine,
            LineCount,
            Position,
            Brush,
            FgRop,
            BgRop,
            DestSurface->info.format
            ));
    }
    while (gcvFALSE);

    /* Unlock the destination. */
    if (destMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destMemory[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Blit
**
**  Generic rectangular blit.
**
**  INPUT:
**
**      OPTIONAL gcoSURF SrcSurface
**          Pointer to the source surface.
**
**      gcoSURF DestSurface
**          Pointer to the destination surface.
**
**      gctUINT32 RectCount
**          The number of rectangles to draw. The array of rectangles
**          pointed to by Rect parameter must have at least RectCount items.
**          Note, that for masked source blits only one destination rectangle
**          is supported.
**
**      OPTIONAL gcsRECT_PTR SrcRect
**          If RectCount is 1, SrcRect represents an sbsolute rectangle within
**          the source surface.
**          If RectCount is greater then 1, (right,bottom) members of SrcRect
**          are ignored and (left,top) members are used as the offset from
**          the origin of each destination rectangle in DestRect list to
**          determine the corresponding source rectangle. In this case the width
**          and the height of the source are assumed the same as of the
**          corresponding destination rectangle.
**
**      gcsRECT_PTR DestRect
**          Pointer to a list of destination rectangles.
**
**      OPTIONAL gcoBRUSH Brush
**          Brush to use for drawing.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**      OPTIONAL gceSURF_TRANSPARENCY Transparency
**          gcvSURF_OPAQUE - each pixel of the bitmap overwrites the destination.
**          gcvSURF_SOURCE_MATCH - source pixels compared against register value
**              to determine the transparency. In simple terms, the transaprency
**              comes down to selecting the ROP code to use. Opaque pixels use
**              foreground ROP and transparent ones use background ROP.
**          gcvSURF_SOURCE_MASK - monochrome source mask defines transparency.
**          gcvSURF_PATTERN_MASK - pattern mask defines transparency.
**
**      OPTIONAL gctUINT32 TransparencyColor
**          This value is used in gcvSURF_SOURCE_MATCH transparency mode.
**          The value is compared against each pixel to determine transparency
**          of the pixel. If the values found equal, the pixel is transparent;
**          otherwise it is opaque.
**
**      OPTIONAL gctPOINTER Mask
**          A pointer to monochrome mask for masked source blits.
**
**      OPTIONAL gceSURF_MONOPACK MaskPack
**          Determines how many horizontal pixels are there per each 32-bit
**          chunk of monochrome mask. For example, if set to gcvSURF_PACKED8,
**          each 32-bit chunk is 8-pixel wide, which also means that it defines
**          4 vertical lines of pixel mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Blit(
    IN OPTIONAL gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gctUINT32 RectCount,
    IN OPTIONAL gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DestRect,
    IN OPTIONAL gcoBRUSH Brush,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN OPTIONAL gceSURF_TRANSPARENCY Transparency,
    IN OPTIONAL gctUINT32 TransparencyColor,
    IN OPTIONAL gctPOINTER Mask,
    IN OPTIONAL gceSURF_MONOPACK MaskPack
    )
{
    gceSTATUS status;

    /*gcoHARDWARE hardware;*/
    gco2D engine;

    gce2D_TRANSPARENCY srcTransparency;
    gce2D_TRANSPARENCY dstTransparency;
    gce2D_TRANSPARENCY patTransparency;

    gctBOOL useBrush;
    gctBOOL useSource;

    gctBOOL stretchBlt = gcvFALSE;
    gctBOOL relativeSource = gcvFALSE;

    gctPOINTER srcMemory[3]  = {gcvNULL};
    gctPOINTER destMemory[3] = {gcvNULL};

    gctBOOL useSoftEngine = gcvFALSE;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x RectCount=%u SrcRect=0x%x "
              "DestRect=0x%x Brush=0x%x FgRop=%02x BgRop=%02x Transparency=%d "
              "TransparencyColor=%08x Mask=0x%x MaskPack=%d",
              SrcSurface, DestSurface, RectCount, SrcRect, DestRect, Brush,
              FgRop, BgRop, Transparency, TransparencyColor, Mask, MaskPack);

    do
    {
        gctUINT32 destFormat, destFormatSwizzle, destIsYUV;

        /* Validate the object. */
        gcmBADOBJECT_BREAK(DestSurface, gcvOBJ_SURF);

        /* Is 2D Hardware available? */
        if (!gcoHARDWARE_Is2DAvailable())
        {
            /* No, use software renderer. */
            gcmERR_BREAK(gcoHARDWARE_UseSoftware2D(gcvTRUE));
            useSoftEngine = gcvTRUE;
        }

        /* Is the destination format supported? */
        if (gcmIS_ERROR(gcoHARDWARE_TranslateDestinationFormat(
                DestSurface->info.format,
                &destFormat, &destFormatSwizzle, &destIsYUV)))
        {
            /* No, use software renderer. */
            gcmERR_BREAK(gcoHARDWARE_UseSoftware2D(gcvTRUE));
            useSoftEngine = gcvTRUE;
        }

        /* Translate the specified transparency mode. */
        gcmERR_BREAK(gcoHARDWARE_TranslateSurfTransparency(
            Transparency,
            &srcTransparency,
            &dstTransparency,
            &patTransparency
            ));

        /* Determine the resource usage. */
        gcoHARDWARE_Get2DResourceUsage(
            FgRop, BgRop,
            srcTransparency,
            &useSource, &useBrush, gcvNULL
            );

        /* Use surface rect if not specified. */
        if (DestRect == gcvNULL)
        {
            if (RectCount != 1)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            DestRect = &DestSurface->info.rect;
        }

        /* Get 2D engine. */
        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));

        /* Setup the brush if needed. */
        if (useBrush)
        {
            /* Flush the brush. */
            gcmERR_BREAK(gco2D_FlushBrush(
                engine,
                Brush,
                DestSurface->info.format
                ));
        }

        /* Setup the source if needed. */
        if (useSource)
        {
            /* Validate the object. */
            gcmBADOBJECT_BREAK(SrcSurface, gcvOBJ_SURF);

            /* Use surface rect if not specified. */
            if (SrcRect == gcvNULL)
            {
                SrcRect = &SrcSurface->info.rect;
            }

            /* Lock the source. */
            gcmERR_BREAK(gcoSURF_Lock(
                SrcSurface,
                gcvNULL,
                srcMemory
                ));

            /* Determine the relative flag. */
            relativeSource = (RectCount > 1) ? gcvTRUE : gcvFALSE;

            /* Program the source. */
            if (Mask == gcvNULL)
            {
                gctBOOL equal;

                /* Check whether this should be a stretch/shrink blit. */
                if ( (gcsRECT_IsOfEqualSize(SrcRect, DestRect, &equal) ==
                          gcvSTATUS_OK) &&
                     !equal )
                {
                    /* Calculate the stretch factors. */
                    gcmERR_BREAK(gco2D_SetStretchRectFactors(
                        engine,
                        SrcRect, DestRect
                        ));

                    /* Mark as stretch blit. */
                    stretchBlt = gcvTRUE;
                }

                gcmERR_BREAK(gco2D_SetColorSourceEx(
                    engine,
                    useSoftEngine ?
                        (gctUINT32)SrcSurface->info.node.logical
                        : SrcSurface->info.node.physical,
                    SrcSurface->info.stride,
                    SrcSurface->info.format,
                    SrcSurface->info.rotation,
                    SrcSurface->info.alignedWidth,
                    SrcSurface->info.alignedHeight,
                    relativeSource,
                    Transparency,
                    TransparencyColor
                    ));

                gcmERR_BREAK(gco2D_SetSource(
                    engine,
                    SrcRect
                    ));
            }
        }

        /* Lock the destination. */
        gcmERR_BREAK(gcoSURF_Lock(
            DestSurface,
            gcvNULL,
            destMemory
            ));

        gcmERR_BREAK(gco2D_SetTargetEx(
            engine,
            useSoftEngine ?
                (gctUINT32)DestSurface->info.node.logical
                : DestSurface->info.node.physical,
            DestSurface->info.stride,
            DestSurface->info.rotation,
            DestSurface->info.alignedWidth,
            DestSurface->info.alignedHeight
            ));

        /* Masked sources need to be handled differently. */
        if (useSource && (Mask != gcvNULL))
        {
            gctUINT32 streamPackHeightMask;
            gcsSURF_FORMAT_INFO_PTR srcFormat[2];
            gctUINT32 srcAlignedLeft, srcAlignedTop;
            gctINT32 tileWidth, tileHeight;
            gctUINT32 tileHeightMask;
            gctUINT32 maxHeight;
            gctUINT32 srcBaseAddress;
            gcsRECT srcSubRect;
            gcsRECT destSubRect;
            gcsRECT maskRect;
            gcsPOINT maskSize;
            gctUINT32 lines2render;
            gctUINT32 streamWidth;
            gceSURF_MONOPACK streamPack;

            /* Compute the destination size. */
            gctUINT32 destWidth  = DestRect->right  - DestRect->left;
            gctUINT32 destHeight = DestRect->bottom - DestRect->top;

            /* Query tile size. */
            gcmASSERT(SrcSurface->info.type == gcvSURF_BITMAP);
            gcoHARDWARE_QueryTileSize(
                &tileWidth, &tileHeight,
                gcvNULL, gcvNULL,
                gcvNULL
                );

            tileHeightMask = tileHeight - 1;

            /* Determine left source coordinate. */
            srcSubRect.left = SrcRect->left & 7;

            /* Assume 8-pixel packed stream. */
            streamWidth = gcmALIGN(srcSubRect.left + destWidth, 8);

            /* Do we fit? */
            if (streamWidth == 8)
            {
                streamPack = gcvSURF_PACKED8;
                streamPackHeightMask = ~3U;
            }

            /* Nope, don't fit. */
            else
            {
                /* Assume 16-pixel packed stream. */
                streamWidth = gcmALIGN(srcSubRect.left + destWidth, 16);

                /* Do we fit now? */
                if (streamWidth == 16)
                {
                    streamPack = gcvSURF_PACKED16;
                    streamPackHeightMask = ~1U;
                }

                /* Still don't. */
                else
                {
                    /* Assume unpacked stream. */
                    streamWidth = gcmALIGN(srcSubRect.left + destWidth, 32);
                    streamPack = gcvSURF_UNPACKED;
                    streamPackHeightMask = ~0U;
                }
            }

            /* Determine the maxumum stream height. */
            maxHeight  = gcoMATH_DivideUInt(gco2D_GetMaximumDataCount() << 5,
                                            streamWidth);
            maxHeight &= streamPackHeightMask;

            /* Determine the sub source rectangle. */
            srcSubRect.top    = SrcRect->top & tileHeightMask;
            srcSubRect.right  = srcSubRect.left + destWidth;
            srcSubRect.bottom = srcSubRect.top;

            /* Init destination subrectangle. */
            destSubRect.left   = DestRect->left;
            destSubRect.top    = DestRect->top;
            destSubRect.right  = DestRect->right;
            destSubRect.bottom = destSubRect.top;

            /* Determine the number of lines to render. */
            lines2render = srcSubRect.top + destHeight;

            /* Determine the aligned source coordinates. */
            srcAlignedLeft = SrcRect->left - srcSubRect.left;
            srcAlignedTop  = SrcRect->top  - srcSubRect.top;
            gcmASSERT((srcAlignedLeft % tileWidth) == 0);

            /* Get format characteristics. */
            gcmERR_BREAK(gcoSURF_QueryFormat(SrcSurface->info.format, srcFormat));

            /* Determine the initial source address. */
            srcBaseAddress
                = (useSoftEngine ?
                        (gctUINT32)SrcSurface->info.node.logical
                        : SrcSurface->info.node.physical)
                +   srcAlignedTop  * SrcSurface->info.stride
                + ((srcAlignedLeft * srcFormat[0]->bitsPerPixel) >> 3);

            /* Set initial mask coordinates. */
            maskRect.left   = srcAlignedLeft;
            maskRect.top    = srcAlignedTop;
            maskRect.right  = maskRect.left + streamWidth;
            maskRect.bottom = maskRect.top;

            /* Set mask size. */
            maskSize.x = SrcSurface->info.rect.right;
            maskSize.y = SrcSurface->info.rect.bottom;

            do
            {
                /* Determine the area to render in this pass. */
                srcSubRect.top = srcSubRect.bottom & tileHeightMask;
                srcSubRect.bottom = srcSubRect.top + lines2render;
                if (srcSubRect.bottom > (gctINT32) maxHeight)
                    srcSubRect.bottom = maxHeight & ~tileHeightMask;

                destSubRect.top = destSubRect.bottom;
                destSubRect.bottom
                    = destSubRect.top
                    + (srcSubRect.bottom - srcSubRect.top);

                maskRect.top = maskRect.bottom;
                maskRect.bottom = maskRect.top + srcSubRect.bottom;

                /* Set source rectangle size. */
                gcmERR_BREAK(gco2D_SetSource(
                    engine,
                    &srcSubRect
                    ));

                /* Configure masked source. */
                gcmERR_BREAK(gco2D_SetMaskedSource(
                    engine,
                    srcBaseAddress,
                    SrcSurface->info.stride,
                    SrcSurface->info.format,
                    relativeSource,
                    streamPack
                    ));

                /* Do the blit. */
                gcmERR_BREAK(gco2D_MonoBlit(
                    engine,
                    Mask,
                    &maskSize,
                    &maskRect,
                    MaskPack,
                    streamPack,
                    &destSubRect,
                    FgRop,
                    BgRop,
                    DestSurface->info.format
                    ));

                /* Update the source address. */
                srcBaseAddress += srcSubRect.bottom * SrcSurface->info.stride;

                /* Update the line counter. */
                lines2render -= srcSubRect.bottom;
            }
            while (lines2render);
        }
        else if (stretchBlt)
        {
            gcmERR_BREAK(gco2D_StretchBlit(
                engine,
                RectCount,
                DestRect,
                FgRop,
                BgRop,
                DestSurface->info.format
                ));
        }
        else
        {
            gcmERR_BREAK(gco2D_Blit(
                engine,
                RectCount,
                DestRect,
                FgRop,
                BgRop,
                DestSurface->info.format
                ));
        }
    }
    while (gcvFALSE);

    /* Unlock the source. */
    if (srcMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(SrcSurface, srcMemory[0]));
    }

    /* Unlock the destination. */
    if (destMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destMemory[0]));
    }

    /*gcmGETHARDWARE(hardware);*/

    if (useSoftEngine)
    {
        /* Disable software renderer. */
        gcmVERIFY_OK(gcoHARDWARE_UseSoftware2D(gcvFALSE));
    }

/*OnError:*/
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_MonoBlit
**
**  Monochrome blit.
**
**  INPUT:
**
**      gcoSURF DestSurface
**          Pointer to the destination surface.
**
**      gctPOINTER Source
**          A pointer to the monochrome bitmap.
**
**      gceSURF_MONOPACK SourcePack
**          Determines how many horizontal pixels are there per each 32-bit
**          chunk of monochrome bitmap. For example, if set to gcvSURF_PACKED8,
**          each 32-bit chunk is 8-pixel wide, which also means that it defines
**          4 vertical lines of pixels.
**
**      gcsPOINT_PTR SourceSize
**          Size of the source monochrome bitmap in pixels.
**
**      gcsPOINT_PTR SourceOrigin
**          Top left coordinate of the source within the bitmap.
**
**      gcsRECT_PTR DestRect
**          Pointer to a list of destination rectangles.
**
**      OPTIONAL gcoBRUSH Brush
**          Brush to use for drawing.
**
**      gctUINT8 FgRop
**          Foreground ROP to use with opaque pixels.
**
**      gctUINT8 BgRop
**          Background ROP to use with transparent pixels.
**
**      gctBOOL ColorConvert
**          The values of FgColor and BgColor parameters are stored directly in
**          internal color registers and are used either directly as the source
**          color or converted to the format of destination before actually
**          used. The later happens if ColorConvert is not zero.
**
**      gctUINT8 MonoTransparency
**          This value is used in gcvSURF_SOURCE_MATCH transparency mode.
**          The value can be either 0 or 1 and is compared against each mono
**          pixel to determine transparency of the pixel. If the values found
**          equal, the pixel is transparent; otherwise it is opaque.
**
**      gceSURF_TRANSPARENCY Transparency
**          gcvSURF_OPAQUE - each pixel of the bitmap overwrites the destination.
**          gcvSURF_SOURCE_MATCH - source pixels compared against register value
**              to determine the transparency. In simple terms, the transaprency
**              comes down to selecting the ROP code to use. Opaque pixels use
**              foreground ROP and transparent ones use background ROP.
**          gcvSURF_SOURCE_MASK - monochrome source mask defines transparency.
**          gcvSURF_PATTERN_MASK - pattern mask defines transparency.
**
**      gctUINT32 FgColor
**          The values are used to represent foreground color
**          of the source. If the values are in destination format, set
**          ColorConvert to 0. Otherwise, provide the values in ARGB8 format
**          and set ColorConvert to 1 to instruct the hardware to convert the
**          values to the destination format before they are actually used.
**
**      gctUINT32 BgColor
**          The values are used to represent background color
**          of the source. If the values are in destination format, set
**          ColorConvert to 0. Otherwise, provide the values in ARGB8 format
**          and set ColorConvert to 1 to instruct the hardware to convert the
**          values to the destination format before they are actually used.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_MonoBlit(
    IN gcoSURF DestSurface,
    IN gctPOINTER Source,
    IN gceSURF_MONOPACK SourcePack,
    IN gcsPOINT_PTR SourceSize,
    IN gcsPOINT_PTR SourceOrigin,
    IN gcsRECT_PTR DestRect,
    IN OPTIONAL gcoBRUSH Brush,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gctBOOL ColorConvert,
    IN gctUINT8 MonoTransparency,
    IN gceSURF_TRANSPARENCY Transparency,
    IN gctUINT32 FgColor,
    IN gctUINT32 BgColor
    )
{
    gceSTATUS status;

    gco2D engine;

    gce2D_TRANSPARENCY srcTransparency;
    gce2D_TRANSPARENCY dstTransparency;
    gce2D_TRANSPARENCY patTransparency;

    gctBOOL useBrush;
    gctBOOL useSource;

    gctUINT32 destWidth;
    gctUINT32 destHeight;

    gctUINT32 maxHeight;
    gctUINT32 streamPackHeightMask;
    gcsPOINT sourceOrigin;
    gcsRECT srcSubRect;
    gcsRECT destSubRect;
    gcsRECT streamRect;
    gctUINT32 lines2render;
    gctUINT32 streamWidth;
    gceSURF_MONOPACK streamPack;

    gctPOINTER destMemory[3] = {gcvNULL};

    gctBOOL useSotfEngine = gcvFALSE;

    gcmHEADER_ARG("DestSurface=0x%x Source=0x%x SourceSize=0x%x SourceOrigin=0x%x "
              "DestRect=0x%x Brush=0x%x FgRop=%02x BgRop=%02x ColorConvert=%d "
              "MonoTransparency=%u Transparency=%d FgColor=%08x BgColor=%08x",
              DestSurface, Source, SourceSize, SourceOrigin, DestRect, Brush,
              FgRop, BgRop, ColorConvert, MonoTransparency, Transparency,
              FgColor, BgColor);

    do
    {
        gctUINT32 destFormat, destFormatSwizzle, destIsYUV;

        /* Validate the object. */
        gcmBADOBJECT_BREAK(DestSurface, gcvOBJ_SURF);

        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));

        /* Is the destination format supported? */
        if (gcmIS_ERROR(gcoHARDWARE_TranslateDestinationFormat(
                DestSurface->info.format,
                &destFormat, &destFormatSwizzle, &destIsYUV)))
        {
            /* No, use software renderer. */
            gcmERR_BREAK(gcoHARDWARE_UseSoftware2D(gcvTRUE));
            useSotfEngine = gcvTRUE;
        }

        /* Translate the specified transparency mode. */
        gcmERR_BREAK(gcoHARDWARE_TranslateSurfTransparency(
            Transparency,
            &srcTransparency,
            &dstTransparency,
            &patTransparency
            ));

        /* Determine the resource usage. */
        gcoHARDWARE_Get2DResourceUsage(
            FgRop, BgRop,
            srcTransparency,
            &useSource, &useBrush, gcvNULL
            );

        /* Source must be used. */
        if (!useSource)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Use surface rect if not specified. */
        if (DestRect == gcvNULL)
        {
            DestRect = &DestSurface->info.rect;
        }

        /* Default to 0 origin. */
        if (SourceOrigin == gcvNULL)
        {
            SourceOrigin = &sourceOrigin;
            SourceOrigin->x = 0;
            SourceOrigin->y = 0;
        }

        /* Lock the destination. */
        gcmERR_BREAK(gcoSURF_Lock(
            DestSurface,
            gcvNULL,
            destMemory
            ));

        gcmERR_BREAK(gco2D_SetTargetEx(
            engine,
            useSotfEngine ?
                (gctUINT32)DestSurface->info.node.logical
                : DestSurface->info.node.physical,
            DestSurface->info.stride,
            DestSurface->info.rotation,
            DestSurface->info.alignedWidth,
            DestSurface->info.alignedHeight
            ));

        /* Setup the brush if needed. */
        if (useBrush)
        {
            /* Flush the brush. */
            gcmERR_BREAK(gco2D_FlushBrush(
                engine,
                Brush,
                DestSurface->info.format
                ));
        }

        /* Compute the destination size. */
        destWidth  = DestRect->right  - DestRect->left;
        destHeight = DestRect->bottom - DestRect->top;

        /* Determine the number of lines to render. */
        lines2render = destHeight;

        /* Determine left source coordinate. */
        srcSubRect.left = SourceOrigin->x & 7;

        /* Assume 8-pixel packed stream. */
        streamWidth = gcmALIGN(srcSubRect.left + destWidth, 8);

        /* Do we fit? */
        if (streamWidth == 8)
        {
            streamPack = gcvSURF_PACKED8;
            streamPackHeightMask = ~3U;
        }

        /* Nope, don't fit. */
        else
        {
            /* Assume 16-pixel packed stream. */
            streamWidth = gcmALIGN(srcSubRect.left + destWidth, 16);

            /* Do we fit now? */
            if (streamWidth == 16)
            {
                streamPack = gcvSURF_PACKED16;
                streamPackHeightMask = ~1U;
            }

            /* Still don't. */
            else
            {
                /* Assume unpacked stream. */
                streamWidth = gcmALIGN(srcSubRect.left + destWidth, 32);
                streamPack = gcvSURF_UNPACKED;
                streamPackHeightMask = ~0U;
            }
        }

        /* Set the rectangle value. */
        srcSubRect.top = srcSubRect.right = srcSubRect.bottom = 0;

        /* Set source rectangle size. */
        gcmERR_BREAK(gco2D_SetSource(
            engine,
            &srcSubRect
            ));

        /* Program the source. */
        gcmERR_BREAK(gco2D_SetMonochromeSource(
            engine,
            ColorConvert,
            MonoTransparency,
            streamPack,
            gcvFALSE,
            Transparency,
            FgColor,
            BgColor
            ));

        /* Determine the maxumum stream height. */
        maxHeight  = gcoMATH_DivideUInt(gco2D_GetMaximumDataCount() << 5,
                                        streamWidth);
        maxHeight &= streamPackHeightMask;

        /* Set the stream rectangle. */
        streamRect.left   = SourceOrigin->x - srcSubRect.left;
        streamRect.top    = SourceOrigin->y;
        streamRect.right  = streamRect.left + streamWidth;
        streamRect.bottom = streamRect.top;

        /* Init destination subrectangle. */
        destSubRect.left   = DestRect->left;
        destSubRect.top    = DestRect->top;
        destSubRect.right  = DestRect->right;
        destSubRect.bottom = destSubRect.top;

        do
        {
            /* Determine the area to render in this pass. */
            gctUINT32 currLines = (lines2render > maxHeight)
                ? maxHeight
                : lines2render;

            streamRect.top     = streamRect.bottom;
            streamRect.bottom += currLines;

            destSubRect.top     = destSubRect.bottom;
            destSubRect.bottom += currLines;

            /* Do the blit. */
            gcmERR_BREAK(gco2D_MonoBlit(
                engine,
                Source,
                SourceSize,
                &streamRect,
                SourcePack,
                streamPack,
                &destSubRect,
                FgRop, BgRop,
                DestSurface->info.format
                ));

            /* Update the line counter. */
            lines2render -= currLines;
        }
        while (lines2render);
    }
    while (gcvFALSE);

    /* Unlock the destination. */
    if (destMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destMemory[0]));
    }

    if (useSotfEngine)
    {
        /* Disable software renderer. */
        gcmVERIFY_OK(gcoHARDWARE_UseSoftware2D(gcvFALSE));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_FilterBlit
**
**  Filter blit.
**
**  INPUT:
**
**      gcoSURF SrcSurface
**          Pointer to the source surface.
**
**      gcoSURF DestSurface
**          Pointer to the destination surface.
**
**      gcsRECT_PTR SrcRect
**          Coordinates of the entire source image.
**
**      gcsRECT_PTR DestRect
**          Coordinates of the entire destination image.
**
**      gcsRECT_PTR DestSubRect
**          Coordinates of a sub area within the destination to render.
**          If DestSubRect is gcvNULL, the complete image will be rendered
**          using coordinates set by DestRect.
**          If DestSubRect is not gcvNULL and DestSubRect and DestRect are
**          no equal, DestSubRect is assumed to be within DestRect and
**          will be used to reneder the sub area only.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_FilterBlit(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DestRect,
    IN gcsRECT_PTR DestSubRect
    )
{
    gceSTATUS status;
    gcsRECT destSubRect, srcRect, destRect;
    gctBOOL ditherBy3D = gcvFALSE;
    gctBOOL ditherNotSupported = gcvFALSE;
    gctBOOL rotateWalkaround =  gcvFALSE;
    gctBOOL enable2DDither =  gcvFALSE;

    gctPOINTER srcMemory[3] = {gcvNULL, };
    gctPOINTER destMemory[3] = {gcvNULL, };

    gcsSURF_INFO_PTR tempSurf = gcvNULL;
    gcsSURF_INFO_PTR temp2Surf = gcvNULL;

    gco2D engine = gcvNULL;

    gceSURF_ROTATION srcRotBackup = (gceSURF_ROTATION)-1, dstRotBackup = (gceSURF_ROTATION)-1;

    gcmHEADER_ARG("SrcSurface=0x%x DestSurface=0x%x SrcRect=0x%x DestRect=0x%x "
              "DestSubRect=0x%x",
              SrcSurface, DestSurface, SrcRect, DestRect, DestSubRect);

    if (SrcSurface == gcvNULL || DestSurface == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    do
    {
        gcsSURF_FORMAT_INFO_PTR srcFormat[2];
        gcsSURF_FORMAT_INFO_PTR destFormat[2];

        /* Verify the surfaces. */
        gcmBADOBJECT_BREAK(SrcSurface, gcvOBJ_SURF);
        gcmBADOBJECT_BREAK(DestSurface, gcvOBJ_SURF);

        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));

        /* Use surface rect if not specified. */
        if (SrcRect == gcvNULL)
        {
            SrcRect = &SrcSurface->info.rect;
        }

        /* Use surface rect if not specified. */
        if (DestRect == gcvNULL)
        {
            DestRect = &DestSurface->info.rect;
        }

        /* Make sure the destination sub rectangle is set. */
        if (DestSubRect == gcvNULL)
        {
            destSubRect.left   = 0;
            destSubRect.top    = 0;
            destSubRect.right  = DestRect->right  - DestRect->left;
            destSubRect.bottom = DestRect->bottom - DestRect->top;

            DestSubRect = &destSubRect;
        }

        gcmERR_BREAK(gcoSURF_QueryFormat(SrcSurface->info.format, srcFormat));
        gcmERR_BREAK(gcoSURF_QueryFormat(DestSurface->info.format, destFormat));

        if ((SrcSurface->info.dither || DestSurface->info.dither)
            && ((srcFormat[0]->bitsPerPixel > destFormat[0]->bitsPerPixel)
            || (srcFormat[0]->fmtClass == gcvFORMAT_CLASS_YUV)))
        {
            if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_2D_DITHER))
            {
                gcmERR_BREAK(gco2D_EnableDither(
                    engine,
                    gcvTRUE));

                enable2DDither = gcvTRUE;
            }
            else if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_PIPE_3D))
            {
                ditherBy3D = gcvTRUE;
            }
            else
            {
                /* Hardware does not support dither. */
                ditherNotSupported = gcvTRUE;
            }
        }

        if ((SrcSurface->info.rotation != gcvSURF_0_DEGREE || DestSurface->info.rotation != gcvSURF_0_DEGREE)
            && !gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_2D_FILTERBLIT_FULLROTATION))
        {
            rotateWalkaround = gcvTRUE;
        }
        else if (ditherBy3D && (((DestSubRect->right - DestSubRect->left) & 15)
            || ((DestSubRect->bottom - DestSubRect->top) & 3)))
        {
            rotateWalkaround = gcvTRUE;
        }

        /* Lock the destination. */
        gcmERR_BREAK(gcoSURF_Lock(
            DestSurface,
            gcvNULL,
            destMemory
            ));

        /* Lock the source. */
        gcmERR_BREAK(gcoSURF_Lock(
            SrcSurface,
            gcvNULL,
            srcMemory
            ));

        if (ditherBy3D || rotateWalkaround)
        {
            gcsRECT tempRect;

            srcRotBackup = SrcSurface->info.rotation;
            dstRotBackup = DestSurface->info.rotation;

            srcRect = *SrcRect;
            destRect = *DestRect;
            destSubRect = *DestSubRect;

            if (rotateWalkaround)
            {
                if (SrcSurface->info.rotation != gcvSURF_0_DEGREE)
                {
                    SrcSurface->info.rotation = gcvSURF_0_DEGREE;
                    gcmERR_BREAK(gcsRECT_RelativeRotation(srcRotBackup, &DestSurface->info.rotation));

                    gcmERR_BREAK(gcsRECT_Rotate(
                        &srcRect,
                        srcRotBackup,
                        gcvSURF_0_DEGREE,
                        SrcSurface->info.alignedWidth,
                        SrcSurface->info.alignedHeight));

                    destSubRect.left += destRect.left;
                    destSubRect.right += destRect.left;
                    destSubRect.top += destRect.top;
                    destSubRect.bottom += destRect.top;

                    gcmERR_BREAK(gcsRECT_Rotate(
                        &destSubRect,
                        dstRotBackup,
                        DestSurface->info.rotation,
                        DestSurface->info.alignedWidth,
                        DestSurface->info.alignedHeight));

                    gcmERR_BREAK(gcsRECT_Rotate(
                        &destRect,
                        dstRotBackup,
                        DestSurface->info.rotation,
                        DestSurface->info.alignedWidth,
                        DestSurface->info.alignedHeight));

                    destSubRect.left -= destRect.left;
                    destSubRect.right -= destRect.left;
                    destSubRect.top -= destRect.top;
                    destSubRect.bottom -= destRect.top;
                }

                tempRect.left = 0;
                tempRect.top = 0;
                tempRect.right = destRect.right - destRect.left;
                tempRect.bottom = destRect.bottom - destRect.top;

                gcmERR_BREAK(gcoHARDWARE_Get2DTempSurface(
                    tempRect.right,
                    tempRect.bottom,
                    ditherBy3D ? gcvSURF_A8R8G8B8 : DestSurface->info.format,
                    &tempSurf));

                tempSurf->rotation = gcvSURF_0_DEGREE;
            }
#ifndef VIVANTE_NO_3D
            else
            {
                /* Only dither. */
                gctBOOL swap = (DestSurface->info.rotation == gcvSURF_90_DEGREE)
                    || (DestSurface->info.rotation == gcvSURF_270_DEGREE);

                tempRect.left = 0;
                tempRect.top = 0;
                tempRect.right = destRect.right - destRect.left;
                tempRect.bottom = destRect.bottom - destRect.top;

                gcmERR_BREAK(gcoHARDWARE_Get2DTempSurface(
                    swap ? tempRect.bottom : tempRect.right,
                    swap ? tempRect.right : tempRect.bottom,
                    gcvSURF_A8R8G8B8,
                    &tempSurf
                    ));

                tempSurf->rotation = DestSurface->info.rotation;
            }
#endif /* VIVANTE_NO_3D */

            gcmERR_BREAK(gco2D_FilterBlitEx(
                engine,
                SrcSurface->info.node.physical,
                SrcSurface->info.stride,
                SrcSurface->info.node.physical2,
                SrcSurface->info.uStride,
                SrcSurface->info.node.physical3,
                SrcSurface->info.vStride,
                SrcSurface->info.format,
                SrcSurface->info.rotation,
                SrcSurface->info.alignedWidth,
                SrcSurface->info.alignedHeight,
                &srcRect,
                tempSurf->node.physical,
                tempSurf->stride,
                tempSurf->format,
                tempSurf->rotation,
                tempSurf->alignedWidth,
                tempSurf->alignedHeight,
                &tempRect,
                &destSubRect
                ));

            tempRect = destSubRect;
#ifndef VIVANTE_NO_3D
            if (ditherBy3D)
            {
                gcsPOINT srcOrigin, destOrigin, rectSize;
                gcsSURF_INFO * DitherDest;

                if (rotateWalkaround)
                {
                    srcOrigin.x = tempRect.left;
                    srcOrigin.y = tempRect.top;

                    rectSize.x = tempRect.right - tempRect.left;
                    rectSize.y = tempRect.bottom - tempRect.top;

                    destOrigin.x = 0;
                    destOrigin.y = 0;

                    tempRect.left = 0;
                    tempRect.top = 0;
                    tempRect.right = rectSize.x;
                    tempRect.bottom = rectSize.y;

                    gcmERR_BREAK(gcoHARDWARE_Get2DTempSurface(
                        tempRect.right,
                        tempRect.bottom,
                        DestSurface->info.format,
                        &temp2Surf
                        ));

                    temp2Surf->rotation = gcvSURF_0_DEGREE;

                    DitherDest = temp2Surf;
                }
                else
                {
                    destSubRect.left += destRect.left;
                    destSubRect.right += destRect.left;
                    destSubRect.top += destRect.top;
                    destSubRect.bottom += destRect.top;

                    if (DestSurface->info.rotation != gcvSURF_0_DEGREE)
                    {
                        gcmERR_BREAK(gcsRECT_Rotate(
                            &destSubRect,
                            DestSurface->info.rotation,
                            gcvSURF_0_DEGREE,
                            DestSurface->info.alignedWidth,
                            DestSurface->info.alignedHeight));

                        gcmERR_BREAK(gcsRECT_Rotate(
                            &tempRect,
                            DestSurface->info.rotation,
                            gcvSURF_0_DEGREE,
                            tempSurf->alignedWidth,
                            tempSurf->alignedHeight));

                        DestSurface->info.rotation = gcvSURF_0_DEGREE;
                        tempSurf->rotation = gcvSURF_0_DEGREE;
                    }

                    srcOrigin.x = tempRect.left;
                    srcOrigin.y = tempRect.top;

                    destOrigin.x = destSubRect.left;
                    destOrigin.y = destSubRect.top;

                    rectSize.x = destSubRect.right - destSubRect.left;
                    rectSize.y = destSubRect.bottom - destSubRect.top;

                    DitherDest = &DestSurface->info;
                }

                gcmERR_BREAK(gcoHARDWARE_FlushPipe());

                gcmERR_BREAK(gcoHARDWARE_SetDither(gcvTRUE));

                rectSize.x = gcmALIGN(rectSize.x, 16);
                rectSize.y = gcmALIGN(rectSize.y, 4);

                gcmERR_BREAK(gcoHARDWARE_ResolveRect(
                        tempSurf,
                        DitherDest,
                        &srcOrigin, &destOrigin, &rectSize));

                gcmERR_BREAK(gcoHARDWARE_FlushPipe());

                gcmERR_BREAK(gcoHARDWARE_SetDither(gcvFALSE));
            }
#endif /* VIVANTE_NO_3D */
            if (rotateWalkaround)
            {
                /* bitblit rorate. */
                gcsSURF_INFO * srcSurf = ditherBy3D ? temp2Surf : tempSurf;
                gceSURF_ROTATION tSrcRot = (gceSURF_ROTATION)-1, tDestRot = (gceSURF_ROTATION)-1;
                gctBOOL bMirror = gcvFALSE;

                destSubRect.left += destRect.left;
                destSubRect.right += destRect.left;
                destSubRect.top += destRect.top;
                destSubRect.bottom += destRect.top;

                if (!gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_2D_BITBLIT_FULLROTATION))
                {
                    tDestRot = DestSurface->info.rotation;

                    gcmERR_BREAK(gcsRECT_RelativeRotation(srcSurf->rotation, &tDestRot));
                    switch (tDestRot)
                    {
                    case gcvSURF_0_DEGREE:
                        tSrcRot = tDestRot = gcvSURF_0_DEGREE;

                        break;

                    case gcvSURF_90_DEGREE:
                        tSrcRot = gcvSURF_0_DEGREE;
                        tDestRot = gcvSURF_90_DEGREE;

                        break;

                    case gcvSURF_180_DEGREE:
                        tSrcRot = tDestRot = gcvSURF_0_DEGREE;
                        bMirror = gcvTRUE;

                        break;

                    case gcvSURF_270_DEGREE:
                        tSrcRot = gcvSURF_90_DEGREE;
                        tDestRot = gcvSURF_0_DEGREE;

                        break;

                    default:
                        status = gcvSTATUS_NOT_SUPPORTED;

                        break;
                    }

                    gcmERR_BREAK(status);

                    gcmERR_BREAK(gcsRECT_Rotate(
                        &tempRect,
                        srcSurf->rotation,
                        tSrcRot,
                        srcSurf->alignedWidth,
                        srcSurf->alignedHeight));

                    gcmERR_BREAK(gcsRECT_Rotate(
                        &destSubRect,
                        DestSurface->info.rotation,
                        tDestRot,
                        DestSurface->info.alignedWidth,
                        DestSurface->info.alignedHeight));

                    srcSurf->rotation = tSrcRot;
                    DestSurface->info.rotation = tDestRot;

                    if (bMirror)
                    {
                        gcmERR_BREAK(gco2D_SetBitBlitMirror(
                            engine,
                            gcvTRUE,
                            gcvTRUE));
                    }
                }

                gcmERR_BREAK(gco2D_SetClipping(
                    engine,
                    &destSubRect));

                gcmERR_BREAK(gco2D_SetColorSourceEx(
                    engine,
                    srcSurf->node.physical,
                    srcSurf->stride,
                    srcSurf->format,
                    srcSurf->rotation,
                    srcSurf->alignedWidth,
                    srcSurf->alignedHeight,
                    gcvFALSE,
                    gcvSURF_OPAQUE,
                    0
                    ));

                gcmERR_BREAK(gco2D_SetSource(
                    engine,
                    &tempRect
                    ));

                gcmERR_BREAK(gco2D_SetTargetEx(
                    engine,
                    DestSurface->info.node.physical,
                    DestSurface->info.stride,
                    DestSurface->info.rotation,
                    DestSurface->info.alignedWidth,
                    DestSurface->info.alignedHeight
                    ));

                gcmERR_BREAK(gco2D_Blit(
                    engine,
                    1,
                    &destSubRect,
                    0xCC,
                    0xCC,
                    DestSurface->info.format
                    ));

                if (bMirror)
                {
                    gcmERR_BREAK(gco2D_SetBitBlitMirror(
                        engine,
                        gcvFALSE,
                        gcvFALSE));
                }
            }
        }
        else
        {
            /* Call gco2D object to complete the blit. */
            gcmERR_BREAK(gco2D_FilterBlitEx(
                engine,
                SrcSurface->info.node.physical,
                SrcSurface->info.stride,
                SrcSurface->info.node.physical2,
                SrcSurface->info.uStride,
                SrcSurface->info.node.physical3,
                SrcSurface->info.vStride,
                SrcSurface->info.format,
                SrcSurface->info.rotation,
                SrcSurface->info.alignedWidth,
                SrcSurface->info.alignedHeight,
                SrcRect,
                DestSurface->info.node.physical,
                DestSurface->info.stride,
                DestSurface->info.format,
                DestSurface->info.rotation,
                DestSurface->info.alignedWidth,
                DestSurface->info.alignedHeight,
                DestRect,
                DestSubRect
                ));
        }
    }
    while (gcvFALSE);

    if (enable2DDither)
    {
        gcmVERIFY_OK(gco2D_EnableDither(
            engine,
            gcvFALSE));
    }

    if (srcRotBackup != (gceSURF_ROTATION)-1)
    {
        SrcSurface->info.rotation = srcRotBackup;
    }

    if (dstRotBackup != (gceSURF_ROTATION)-1)
    {
        DestSurface->info.rotation = dstRotBackup;
    }

    /* Unlock the source. */
    if (srcMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(SrcSurface, srcMemory[0]));
    }

    /* Unlock the destination. */
    if (destMemory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(DestSurface, destMemory[0]));
    }

    /* Free temp buffer. */
    if (tempSurf != gcvNULL)
    {
        gcmVERIFY_OK(gcoHARDWARE_Put2DTempSurface(tempSurf));
    }

    if (temp2Surf != gcvNULL)
    {
        gcmVERIFY_OK(gcoHARDWARE_Put2DTempSurface(temp2Surf));
    }

    if (ditherNotSupported)
    {
        status = gcvSTATUS_NOT_SUPPORT_DITHER;
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_EnableAlphaBlend
**
**  Enable alpha blending engine in the hardware and disengage the ROP engine.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctUINT8 SrcGlobalAlphaValue
**          Global alpha value for the color components.
**
**      gctUINT8 DstGlobalAlphaValue
**          Global alpha value for the color components.
**
**      gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode
**          Per-pixel alpha component mode.
**
**      gceSURF_PIXEL_ALPHA_MODE DstAlphaMode
**          Per-pixel alpha component mode.
**
**      gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode
**          Global/per-pixel alpha values selection.
**
**      gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode
**          Global/per-pixel alpha values selection.
**
**      gceSURF_BLEND_FACTOR_MODE SrcFactorMode
**          Final blending factor mode.
**
**      gceSURF_BLEND_FACTOR_MODE DstFactorMode
**          Final blending factor mode.
**
**      gceSURF_PIXEL_COLOR_MODE SrcColorMode
**          Per-pixel color component mode.
**
**      gceSURF_PIXEL_COLOR_MODE DstColorMode
**          Per-pixel color component mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_EnableAlphaBlend(
    IN gcoSURF Surface,
    IN gctUINT8 SrcGlobalAlphaValue,
    IN gctUINT8 DstGlobalAlphaValue,
    IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
    IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
    IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
    IN gceSURF_BLEND_FACTOR_MODE DstFactorMode,
    IN gceSURF_PIXEL_COLOR_MODE SrcColorMode,
    IN gceSURF_PIXEL_COLOR_MODE DstColorMode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x SrcGlobalAlphaValue=%u DstGlobalAlphaValue=%u "
              "SrcAlphaMode=%d DstAlphaMode=%d SrcGlobalAlphaMode=%d "
              "DstGlobalAlphaMode=%d SrcFactorMode=%d DstFactorMode=%d "
              "SrcColorMode=%d DstColorMode=%d",
              Surface, SrcGlobalAlphaValue, DstGlobalAlphaValue, SrcAlphaMode,
              DstAlphaMode, SrcGlobalAlphaMode, DstGlobalAlphaMode,
              SrcFactorMode, DstFactorMode, SrcColorMode, DstColorMode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do {
        gco2D engine;

        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));

        gcmERR_BREAK(gco2D_EnableAlphaBlend(
                engine,
                (gctUINT32)SrcGlobalAlphaValue << 24,
                (gctUINT32)DstGlobalAlphaValue << 24,
                SrcAlphaMode,
                DstAlphaMode,
                SrcGlobalAlphaMode,
                DstGlobalAlphaMode,
                SrcFactorMode,
                DstFactorMode,
                SrcColorMode,
                DstColorMode
                ));

    } while (gcvFALSE);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_DisableAlphaBlend
**
**  Disable alpha blending engine in the hardware and engage the ROP engine.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_DisableAlphaBlend(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        gco2D engine;
        gcmERR_BREAK(gcoHAL_Get2DEngine(gcvNULL, &engine));
        gcmERR_BREAK(gco2D_DisableAlphaBlend(engine));
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoSURF_CopyPixels
**
**  Copy a rectangular area from one surface to another with format conversion.
**
**  INPUT:
**
**      gcoSURF Source
**          Pointer to the source surface.
**
**      gcoSURF Target
**          Pointer to the target surface.
**
**      gctINT SourceX, SourceY
**          Source surface origin.
**
**      gctINT TargetX, TargetY
**          Target surface origin.
**
**      gctINT Width, Height
**          The size of the area. If Height is negative, the area will
**          be copied with a vertical flip.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_CopyPixels(
    IN gcoSURF Source,
    IN gcoSURF Target,
    IN gctINT SourceX,
    IN gctINT SourceY,
    IN gctINT TargetX,
    IN gctINT TargetY,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status, last;
    gctPOINTER srcMemory[3] = {gcvNULL};
    gctPOINTER trgMemory[3] = {gcvNULL};

    gcmHEADER_ARG("Source=0x%x Target=0x%x SourceX=%d SourceY=%d TargetX=%d TargetY=%d "
              "Width=%d Height=%d",
              Source, Target, SourceX, SourceY, TargetX, TargetY, Width,
              Height);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Source, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(Target, gcvOBJ_SURF);

    do
    {
        /* Lock the surfaces. */
        gcmERR_BREAK(
            gcoSURF_Lock(Source, gcvNULL, srcMemory));
        gcmERR_BREAK(
            gcoSURF_Lock(Target, gcvNULL, trgMemory));

        if (Source->info.node.pool != gcvPOOL_USER)
        {
            gcmERR_BREAK(
                gcoSURF_NODE_Cache(&Source->info.node,
                                 srcMemory[0],
                                 Source->info.size,
                                 gcvCACHE_FLUSH));
        }
        if (Target->info.node.pool != gcvPOOL_USER)
        {
            gcmERR_BREAK(
                gcoSURF_NODE_Cache(&Target->info.node,
                                 trgMemory[0],
                                 Target->info.size,
                                 gcvCACHE_FLUSH));
        }

        /* Flush the surfaces. */
        gcmERR_BREAK(gcoSURF_Flush(Source));
        gcmERR_BREAK(gcoSURF_Flush(Target));

#ifndef VIVANTE_NO_3D
        /* Disable the tile status and decompress the buffers. */
        gcmERR_BREAK(
            gcoSURF_DisableTileStatus(Source, gcvTRUE));

        /* Disable the tile status for the destination. */
        gcmERR_BREAK(
            gcoSURF_DisableTileStatus(Target, gcvTRUE));

#endif /* VIVANTE_NO_3D */

        /* Read the pixel. */
        gcmERR_BREAK(
            gcoHARDWARE_CopyPixels(&Source->info,
                                   &Target->info,
                                   SourceX, SourceY,
                                   TargetX, TargetY,
                                   Width, Height));
    }
    while (gcvFALSE);

    /* Unlock the surfaces. */
    if (srcMemory[0] != gcvNULL)
    {
        gcmCHECK_STATUS(
            gcoSURF_Unlock(Source, srcMemory[0]));
    }

    if (trgMemory[0] != gcvNULL)
    {
        gcmCHECK_STATUS(
            gcoSURF_Unlock(Target, trgMemory[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_ReadPixel
**
**  gcoSURF_ReadPixel reads and returns the current value of the pixel from
**  the specified surface. The pixel value is returned in the specified format.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctPOINTER Memory
**          Pointer to the actual surface bits returned by gcoSURF_Lock.
**
**      gctINT X, Y
**          Coordinates of the pixel.
**
**      gceSURF_FORMAT Format
**          Format of the pixel value to be returned.
**
**  OUTPUT:
**
**      gctPOINTER PixelValue
**          Pointer to the placeholder for the result.
*/
gceSTATUS
gcoSURF_ReadPixel(
    IN gcoSURF Surface,
    IN gctPOINTER Memory,
    IN gctINT X,
    IN gctINT Y,
    IN gceSURF_FORMAT Format,
    OUT gctPOINTER PixelValue
    )
{
    gceSTATUS status, last;
    gctPOINTER srcMemory[3] = {gcvNULL};
    gcsSURF_INFO target;

    gcmHEADER_ARG("Surface=0x%x Memory=0x%x X=%d Y=%d Format=%d",
                  Surface, Memory, X, Y, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Flush the surface. */
        gcmERR_BREAK(
            gcoSURF_Flush(Surface));

#ifndef VIVANTE_NO_3D
        /* Flush the tile status and decompress the buffers. */
        gcmERR_BREAK(
            gcoSURF_DisableTileStatus(Surface, gcvTRUE));
#endif /* VIVANTE_NO_3D */

        /* Lock the source surface. */
        gcmERR_BREAK(
            gcoSURF_Lock(Surface, gcvNULL, srcMemory));

        /* Fill in the target structure. */
        target.type          = gcvSURF_BITMAP;
        target.format        = Format;

        target.rect.left     = 0;
        target.rect.top      = 0;
        target.rect.right    = 1;
        target.rect.bottom   = 1;

        target.alignedWidth  = 1;
        target.alignedHeight = 1;

        target.rotation      = gcvSURF_0_DEGREE;

        target.stride        = 0;
        target.size          = 0;

        target.node.valid    = gcvTRUE;
        target.node.logical  = (gctUINT8_PTR) PixelValue;

        target.samples.x     = 1;
        target.samples.y     = 1;

        /* Read the pixel. */
        gcmERR_BREAK(
            gcoHARDWARE_CopyPixels(&Surface->info,
                                   &target,
                                   X, Y,
                                   0, 0,
                                   1, 1));
    }
    while (gcvFALSE);

    /* Unlock the source surface. */
    if (srcMemory[0] != gcvNULL)
    {
        gcmCHECK_STATUS(
            gcoSURF_Unlock(Surface, srcMemory[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_WritePixel
**
**  gcoSURF_WritePixel writes a color value to a pixel of the specified surface.
**  The pixel value is specified in the specified format.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctPOINTER Memory
**          Pointer to the actual surface bits returned by gcoSURF_Lock.
**
**      gctINT X, Y
**          Coordinates of the pixel.
**
**      gceSURF_FORMAT Format
**          Format of the pixel value to be returned.
**
**      gctPOINTER PixelValue
**          Pointer to the pixel value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_WritePixel(
    IN gcoSURF Surface,
    IN gctPOINTER Memory,
    IN gctINT X,
    IN gctINT Y,
    IN gceSURF_FORMAT Format,
    IN gctPOINTER PixelValue
    )
{
    gceSTATUS status, last;
    gctPOINTER trgMemory[3] = {gcvNULL};
    gcsSURF_INFO source;

    gcmHEADER_ARG("Surface=0x%x Memory=0x%x X=%d Y=%d Format=%d PixelValue=0x%x",
              Surface, Memory, X, Y, Format, PixelValue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    do
    {
        /* Flush the surface. */
        gcmERR_BREAK(
            gcoSURF_Flush(Surface));

#ifndef VIVANTE_NO_3D
        /* Disable the tile status and decompress the buffers. */
        gcmERR_BREAK(
            gcoSURF_DisableTileStatus(Surface, gcvTRUE));
#endif /* VIVANTE_NO_3D */

        /* Lock the source surface. */
        gcmERR_BREAK(
            gcoSURF_Lock(Surface, gcvNULL, trgMemory));

        /* Fill in the source structure. */
        source.type          = gcvSURF_BITMAP;
        source.format        = Format;

        source.rect.left     = 0;
        source.rect.top      = 0;
        source.rect.right    = 1;
        source.rect.bottom   = 1;

        source.alignedWidth  = 1;
        source.alignedHeight = 1;

        source.rotation      = gcvSURF_0_DEGREE;

        source.stride        = 0;
        source.size          = 0;

        source.node.valid    = gcvTRUE;
        source.node.logical  = (gctUINT8_PTR) PixelValue;

        source.samples.x     = 1;
        source.samples.y     = 1;

        /* Read the pixel. */
        gcmERR_BREAK(
            gcoHARDWARE_CopyPixels(&source,
                                   &Surface->info,
                                   0, 0,
                                   X, Y,
                                   1, 1));
    }
    while (gcvFALSE);

    /* Unlock the source surface. */
    if (trgMemory[0] != gcvNULL)
    {
        gcmCHECK_STATUS(gcoSURF_Unlock(Surface, trgMemory[0]));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

gceSTATUS
gcoSURF_NODE_Cache(
    IN gcsSURF_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x, Operation=%d",Node, Operation);

    if (Node->pool == gcvPOOL_USER)
    {
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

#if !gcdPAGED_MEMORY_CACHEABLE
    if (Node->u.normal.cacheable == gcvFALSE)
    {
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
#endif

    switch (Operation)
    {
        case gcvCACHE_CLEAN:
            gcmONERROR(gcoOS_CacheClean(gcvNULL, Node->u.normal.node, Logical, Bytes));
            break;

        case gcvCACHE_INVALIDATE:
            gcmONERROR(gcoOS_CacheInvalidate(gcvNULL, Node->u.normal.node, Logical, Bytes));
            break;

        case gcvCACHE_FLUSH:
            gcmONERROR(gcoOS_CacheFlush(gcvNULL, Node->u.normal.node, Logical, Bytes));
            break;

        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            break;
    }

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_CPUCacheOperation
**
**  Perform the specified CPU cache operation on the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_CPU_CACHE_OP_TYPE Operation
**          Cache operation to be performed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_CPUCacheOperation(
    IN gcoSURF Surface,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER source[3] = {0};
    gctBOOL locked = gcvFALSE;

    gcmHEADER_ARG("Surface=0x%x, Operation=%d", Surface, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Lock the surfaces. */
    gcmONERROR(gcoSURF_Lock(Surface, gcvNULL, source));
    locked = gcvTRUE;

    gcmONERROR(gcoSURF_NODE_Cache(&Surface->info.node,
                                  source[0],
                                  Surface->info.size,
                                  Operation));

    /* Unlock the surfaces. */
    gcmONERROR(gcoSURF_Unlock(Surface, source[0]));
    locked = gcvFALSE;

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    if (locked)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(Surface, source[0]));
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_Flush
**
**  Flush the caches to make sure the surface has all pixels.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_Flush(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Flush the current pipe. */
    status = gcoHARDWARE_FlushPipe();
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoSURF_FillFromTile
**
**  Fill the surface from the information in the tile status buffer.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_FillFromTile(
    IN gcoSURF Surface
    )
{
    gceSTATUS status;
    gco3D engine;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    if ((Surface->info.type == gcvSURF_RENDER_TARGET)
     && (Surface->info.tileStatusNode.pool != gcvPOOL_UNKNOWN))
    {
        /* Extract the gco3D object pointer. */
        gcmONERROR(
            gcoHAL_Get3DEngine(gcvNULL, &engine));

        /* Flush the TileStatus cache. */
        gcmONERROR(
            gcoHARDWARE_FlushTileStatus(&Surface->info,
                                        gcvFALSE));

        /* Fill the surface from the TileStatus buffer. */
        gcmONERROR(
            gcoHARDWARE_FillFromTileStatus(&Surface->info));
    }
    else
    {
        /* Set return value. */
        status = gcvSTATUS_NOT_SUPPORTED;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoSURF_SetSamples
**
**  Set the number of samples per pixel.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctUINT Samples
**          Number of samples per pixel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetSamples(
    IN gcoSURF Surface,
    IN gctUINT Samples
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT width = 0, height = 0;

    gcmHEADER_ARG("Surface=0x%x Samples=%u", Surface, Samples);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Make sure this is not user-allocated surface. */
    if (Surface->info.node.pool == gcvPOOL_USER)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    switch (Samples)
    {
    case 16: /* VAA sampling. */
        if (gcoHAL_IsFeatureAvailable(gcvNULL,
                                      gcvFEATURE_VAA) == gcvSTATUS_TRUE)
        {
            gctBOOL valid;

            if (Surface->info.type == gcvSURF_RENDER_TARGET)
            {
                /* VAA is only supported on RGB888 surfaces. */
                switch (Surface->info.format)
                {
                case gcvSURF_X8R8G8B8:
                case gcvSURF_A8R8G8B8:
                    /* Render targets need 2 samples per pixel. */
                    width = (Surface->info.samples.x != 2)
                          ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.right,
                                                        Surface->info.samples.x) * 2
                          : Surface->info.rect.right;

                    Surface->info.samples.x = 2;

                    valid = gcvTRUE;
                    break;

                default:
                    valid = gcvFALSE;
                    break;
                }
            }
            else
            {
                /* Any other target needs 1 sample per pixel. */
                width = (Surface->info.samples.x != 1)
                      ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.right,
                                                    Surface->info.samples.x)
                      : Surface->info.rect.right;

                Surface->info.samples.x = 1;
                valid = gcvTRUE;
            }

            if (valid)
            {
                /* Normal height. */
                height = (Surface->info.samples.y != 1)
                       ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.bottom,
                                                     Surface->info.samples.y)
                       : Surface->info.rect.bottom;

                Surface->info.samples.y = 1;
                Surface->info.vaa       = (Surface->info.samples.x == 2);
                break;
            }
        }
        /* Fall through to 4x MSAA. */

        /* fall through */
    case 4: /* 2x2 sampling. */
        width  = (Surface->info.samples.x != 2)
               ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.right,
                                             Surface->info.samples.x) * 2
               : Surface->info.rect.right;
        height = (Surface->info.samples.y != 2)
               ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.bottom,
                                             Surface->info.samples.y) * 2
               : Surface->info.rect.bottom;

        Surface->info.samples.x = 2;
        Surface->info.samples.y = 2;
        Surface->info.vaa       = gcvFALSE;
        break;

    case 2: /* 2x1 sampling. */
        width  = (Surface->info.samples.x != 2)
               ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.right,
                                             Surface->info.samples.x) * 2
               : Surface->info.rect.right;
        height = (Surface->info.samples.y != 1)
               ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.bottom,
                                             Surface->info.samples.y)
               : Surface->info.rect.bottom;

        Surface->info.samples.x = 2;
        Surface->info.samples.y = 1;
        Surface->info.vaa       = gcvFALSE;
        break;

    case 0:
    case 1: /* 1x1 sampling. */
        width  = (Surface->info.samples.x != 1)
               ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.right,
                                             Surface->info.samples.x)
               : Surface->info.rect.right;
        height = (Surface->info.samples.y != 1)
               ? (gctINT) gcoMATH_DivideUInt(Surface->info.rect.bottom,
                                             Surface->info.samples.y)
               : Surface->info.rect.bottom;

        Surface->info.samples.x = 1;
        Surface->info.samples.y = 1;
        Surface->info.vaa       = gcvFALSE;
        break;

    default:
        /* Invalid multi-sample request. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if ((Surface->info.rect.right  != width)
    ||  (Surface->info.rect.bottom != height)
    )
    {
        gceSURF_TYPE type = Surface->info.type;
#if gcdENABLE_VG
        gceHARDWARE_TYPE currentType;
#endif
        if (Surface->info.tileStatusNode.pool == gcvPOOL_UNKNOWN)
        {
            type |= gcvSURF_NO_TILE_STATUS;
        }

#if gcdENABLE_VG
        gcmGETCURRENTHARDWARE(currentType);
        /* Destroy existing surface memory. */
        gcmONERROR(_FreeSurface(Surface, currentType));

        /* Allocate new surface. */
        gcmONERROR(
            _AllocateSurface(Surface,
                             currentType,
                             width, height, Surface->depth,
                             type,
                             Surface->info.format,
                             gcvPOOL_DEFAULT));

#else
        /* Destroy existing surface memory. */
        gcmONERROR(_FreeSurface(Surface));

        /* Allocate new surface. */
        gcmONERROR(
            _AllocateSurface(Surface,
                             width, height, Surface->depth,
                             type,
                             Surface->info.format,
                             gcvPOOL_DEFAULT));
#endif
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_GetSamples
**
**  Get the number of samples per pixel.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gctUINT_PTR Samples
**          Pointer to variable receiving the number of samples per pixel.
**
*/
gceSTATUS
gcoSURF_GetSamples(
    IN gcoSURF Surface,
    OUT gctUINT_PTR Samples
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Samples != gcvNULL);

    /* Return samples. */
    *Samples = Surface->info.samples.x * Surface->info.samples.y;

    /* Test for VAA. */
    if (Surface->info.vaa)
    {
        gcmASSERT(*Samples == 2);

        /* Setup to 16 samples for VAA. */
        *Samples = 16;
    }

    /* Success. */
    gcmFOOTER_ARG("*Samples=%u", *Samples);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetResolvability
**
**  Set the resolvability of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gctBOOL Resolvable
**          gcvTRUE if the surface is resolvable or gcvFALSE if not.  This is
**          required for alignment purposes.
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_SetResolvability(
    IN gcoSURF Surface,
    IN gctBOOL Resolvable
    )
{
    gcmHEADER_ARG("Surface=0x%x Resolvable=%d", Surface, Resolvable);

    /* Verif the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the resolvability. */
    Surface->resolvable = Resolvable;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoSURF_SetOrientation
**
**  Set the orientation of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceORIENTATION Orientation
**          The requested surface orientation.  Orientation can be one of the
**          following values:
**
**              gcvORIENTATION_TOP_BOTTOM - Surface is from top to bottom.
**              gcvORIENTATION_BOTTOM_TOP - Surface is from bottom to top.
**
**  OUTPUT:
**
*/
gceSTATUS
gcoSURF_SetOrientation(
    IN gcoSURF Surface,
    IN gceORIENTATION Orientation
    )
{
    gcmHEADER_ARG("Surface=0x%x Orientation=%d", Surface, Orientation);

    /* Verif the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the orientation. */
    Surface->info.orientation = Orientation;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_QueryOrientation
**
**  Query the orientation of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gceORIENTATION * Orientation
**          Pointer to a variable receiving the surface orientation.
**
*/
gceSTATUS
gcoSURF_QueryOrientation(
    IN gcoSURF Surface,
    OUT gceORIENTATION * Orientation
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(Orientation != gcvNULL);

    /* Return the orientation. */
    *Orientation = Surface->info.orientation;

    /* Success. */
    gcmFOOTER_ARG("*Orientation=%d", *Orientation);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetColorType
**
**  Set the color type of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_COLOR_TYPE colorType
**          color type of the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetColorType(
    IN gcoSURF Surface,
    IN gceSURF_COLOR_TYPE ColorType
    )
{
    gcmHEADER_ARG("Surface=0x%x ColorType=%d", Surface, ColorType);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Set the color type. */
    Surface->info.colorType = ColorType;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_GetColorType
**
**  Get the color type of the surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**  OUTPUT:
**
**      gceSURF_COLOR_TYPE *colorType
**          pointer to the variable receiving color type of the surface.
**
*/
gceSTATUS
gcoSURF_GetColorType(
    IN gcoSURF Surface,
    OUT gceSURF_COLOR_TYPE *ColorType
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_ARGUMENT(ColorType != gcvNULL);

    /* Return the color type. */
    *ColorType = Surface->info.colorType;

    /* Success. */
    gcmFOOTER_ARG("*ColorType=%d", *ColorType);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetRotation
**
**  Set the surface ration angle.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_ROTATION Rotation
**          Rotation angle.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetRotation(
    IN gcoSURF Surface,
    IN gceSURF_ROTATION Rotation
    )
{
    gcmHEADER_ARG("Surface=0x%x Rotation=%d", Surface, Rotation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Support only 2D surfaces. */
    if (Surface->info.type != gcvSURF_BITMAP)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Set new rotation. */
    Surface->info.rotation = Rotation;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoSURF_SetDither
**
**  Set the surface dither flag.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the surface.
**
**      gceSURF_ROTATION Dither
**          ditherable or not.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetDither(
    IN gcoSURF Surface,
    IN gctBOOL Dither
    )
{
    gcmHEADER_ARG("Surface=0x%x Dither=%d", Surface, Dither);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Support only 2D surfaces. */
    if (Surface->info.type != gcvSURF_BITMAP)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Set new rotation. */
    Surface->info.dither = Dither;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoSURF_Copy
**
**  Copy one tiled surface to another tiled surfaces.  This is used for handling
**  unaligned surfaces.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to the gcoSURF object that describes the surface to copy
**          into.
**
**      gcoSURF Source
**          Pointer to the gcoSURF object that describes the source surface to
**          copy from.
**
**  OUTPUT:
**
**      Nothing.
**
*/
gceSTATUS
gcoSURF_Copy(
    IN gcoSURF Surface,
    IN gcoSURF Source
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT8_PTR source = gcvNULL, target = gcvNULL;

#if gcdENABLE_VG
    gceHARDWARE_TYPE currentType;
#endif

    gcmHEADER_ARG("Surface=0x%x Source=0x%x", Surface, Source);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    gcmVERIFY_OBJECT(Source, gcvOBJ_SURF);

    if ((Surface->info.tiling != gcvTILED)
     || (Source->info.tiling != gcvTILED))
    {
        /* Both surfaces need to be tiled. */
        gcmFOOTER();
        return gcvSTATUS_INVALID_REQUEST;
    }

    do
    {
        gctUINT y;
        gctUINT sourceOffset, targetOffset;
        gctINT height;
        gctPOINTER pointer[3] = { gcvNULL };

#if gcdENABLE_VG
        gcmGETCURRENTHARDWARE(currentType);
        if (currentType == gcvHARDWARE_VG)
        {
            /* Flush the pipe. */
            gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(gcvNULL));

            /* Commit and stall the pipe. */
            gcmERR_BREAK(gcoHAL_Commit(gcvNULL, gcvTRUE));

            /* Get the tile height. */
            gcmERR_BREAK(gcoVGHARDWARE_QueryTileSize(
                                                   gcvNULL, gcvNULL,
                                                   gcvNULL, &height,
                                                   gcvNULL));
        }
        else
#endif
        {
            /* Flush the pipe. */
            gcmERR_BREAK(gcoHARDWARE_FlushPipe());

            /* Commit and stall the pipe. */
            gcmERR_BREAK(gcoHAL_Commit(gcvNULL, gcvTRUE));

            /* Get the tile height. */
            gcmERR_BREAK(gcoHARDWARE_QueryTileSize(gcvNULL, gcvNULL,
                                                   gcvNULL, &height,
                                                   gcvNULL));
        }
        /* Lock the surfaces. */
        gcmERR_BREAK(gcoSURF_Lock(Source, gcvNULL, pointer));

        source = pointer[0];

        gcmERR_BREAK(gcoSURF_Lock(Surface, gcvNULL, pointer));

        target = pointer[0];

        /* Reset initial offsets. */
        sourceOffset = 0;
        targetOffset = 0;

        /* Loop target surface, one row of tiles at a time. */
        for (y = 0; y < Surface->info.alignedHeight; y += height)
        {
            /* Copy one row of tiles. */
            gcmVERIFY_OK(gcoOS_MemCopy(target + targetOffset,
                                       source + sourceOffset,
                                       Surface->info.stride * height));

            /* Move to next row of tiles. */
            sourceOffset += Source->info.stride  * height;
            targetOffset += Surface->info.stride * height;
        }
    }
    while (gcvFALSE);

    if (source != gcvNULL)
    {
        /* Unlock source surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(Source, source));
    }

    if (target != gcvNULL)
    {
        /* Unlock target surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(Surface, target));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoSURF_ConstructWrapper
**
**  Create a new gcoSURF wrapper object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoSURF * Surface
**          Pointer to the variable that will hold the gcoSURF object pointer.
*/
gceSTATUS
gcoSURF_ConstructWrapper(
    IN gcoHAL Hal,
    OUT gcoSURF * Surface
    )
{
    gcoSURF surface;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Surface != gcvNULL);

    do
    {
        /* Allocate the gcoSURF object. */
        gcmONERROR(gcoOS_Allocate(
            gcvNULL,
            gcmSIZEOF(struct _gcoSURF),
            &pointer
            ));

        surface = pointer;

        /* Reset the object. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            surface, gcmSIZEOF(struct _gcoSURF)
            ));

        /* Initialize the gcoSURF object.*/
        surface->object.type = gcvOBJ_SURF;

#ifndef VIVANTE_NO_3D
        /* 1 sample per pixel. */
        surface->info.samples.x = 1;
        surface->info.samples.y = 1;
#endif /* VIVANTE_NO_3D */

        /* One plane. */
        surface->depth = 1;

        /* Initialize the node. */
        surface->info.node.pool      = gcvPOOL_USER;
        surface->info.node.physical  = ~0U;
        surface->info.node.physical2 = ~0U;
        surface->info.node.physical3 = ~0U;
        surface->info.node.count     = 1;
        surface->referenceCount = 1;

        /* Return pointer to the gcoSURF object. */
        *Surface = surface;

        /* Success. */
        gcmFOOTER_ARG("*Surface=0x%x", *Surface);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetBuffer
**
**  Set the underlying buffer for the surface wrapper.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gctINT Stride
**          Surface stride. Is set to ~0 the stride will be autocomputed.
**
**      gctPOINTER Logical
**          Logical pointer to the user allocated surface or gcvNULL if no
**          logical pointer has been provided.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetBuffer(
    IN gcoSURF Surface,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gctUINT Stride,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x Type=%d Format=%d Stride=%u Logical=0x%x "
                  "Physical=%08x",
                  Surface, Type, Format, Stride, Logical, Physical);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Has to be user-allocated surface. */
    if (Surface->info.node.pool != gcvPOOL_USER)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Unmap the current buffer if any. */
    gcmONERROR(_UnmapUserBuffer(Surface, gcvTRUE));

    /* Determine the stride parameters. */
    Surface->autoStride = (Stride == ~0U);

    /* Set surface parameters. */
    Surface->info.type   = Type;
    Surface->info.format = Format;
    Surface->info.stride = Stride;

    /* Set node pointers. */
    Surface->logical  = (gctUINT8_PTR) Logical;
    Surface->physical = Physical;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_SetVideoBuffer
**
**  Set the underlying video buffer for the surface wrapper.
**  The video plane addresses should be specified invidually.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gctINT Stride
**          Surface stride. Is set to ~0 the stride will be autocomputed.
**
**      gctPOINTER LogicalPlane1
**          Logical pointer to the first plane of the user allocated surface
**          or gcvNULL if no logical pointer has been provided.
**
**      gctUINT32 PhysicalPlane1
**          Physical pointer to the user allocated surface or ~0 if no
**          physical pointer has been provided.
**
**  OUTPUT:
**
**      Nothing.
*/

/*******************************************************************************
**
**  gcoSURF_SetWindow
**
**  Set the size of the surface in pixels and map the underlying buffer set by
**  gcoSURF_SetBuffer if necessary.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**      gctINT X, Y
**          The origin of the surface.
**
**      gctINT Width, Height
**          Size of the surface in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_SetWindow(
    IN gcoSURF Surface,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height
    )
{
    gceSTATUS status;
    gctUINT32 offsetX;
    gctUINT32 offsetY;
    gctUINT32 bitsPerPixel;
#if gcdSECURE_USER
    gcsHAL_INTERFACE iface;
#endif
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentHW;
#endif
    gcmHEADER_ARG("Surface=0x%x X=%u Y=%u Width=%u Height=%u",
                  Surface, X, Y, Width, Height);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Unmap the current buffer if any. */
    gcmONERROR(
        _UnmapUserBuffer(Surface, gcvTRUE));

    /* Make sure at least one of the surface pointers is set. */
    if ((Surface->logical == gcvNULL) && (Surface->physical == ~0U))
    {
        gcmONERROR(gcvSTATUS_INVALID_ADDRESS);
    }

    /* Set initial aligned width and height. */
    Surface->info.alignedWidth  = Width;
    Surface->info.alignedHeight = Height;

#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentHW);

    if (currentHW == gcvHARDWARE_VG)
    {
        /* Compute bits per pixel. */
        gcmONERROR(
            gcoVGHARDWARE_ConvertFormat(gcvNULL,
                                      Surface->info.format,
                                      &bitsPerPixel,
                                      gcvNULL));
    }
    else
#endif
    {
        /* Compute bits per pixel. */
        gcmONERROR(
            gcoHARDWARE_ConvertFormat(Surface->info.format,
                                      &bitsPerPixel,
                                      gcvNULL));
    }

    /* Stride is the same as the width? */
    if (Surface->autoStride)
    {
        /* Compute the stride. */
        Surface->info.stride = Width * bitsPerPixel / 8;
    }
    else
    {
#if gcdENABLE_VG
        if (currentHW == gcvHARDWARE_VG)
        {
            gcmONERROR(
                gcoVGHARDWARE_AlignToTile(gcvNULL,
                                          Surface->info.type,
                                          &Surface->info.alignedWidth,
                                          &Surface->info.alignedHeight));
        }
        else
#endif
        {
            /* Align the surface size. */
#ifndef VIVANTE_NO_3D
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(Surface->info.type,
                                                  Surface->info.format,
                                                  &Surface->info.alignedWidth,
                                                  &Surface->info.alignedHeight,
                                                  &Surface->info.tiling,
                                                  &Surface->info.superTiled));
#else
            gcmONERROR(
                gcoHARDWARE_AlignToTileCompatible(Surface->info.type,
                                                  Surface->info.format,
                                                  &Surface->info.alignedWidth,
                                                  &Surface->info.alignedHeight,
                                                  &Surface->info.tiling,
                                                  gcvNULL));
#endif /* VIVANTE_NO_3D */
        }
    }

    /* Set the rectangle. */
    Surface->info.rect.left   = 0;
    Surface->info.rect.top    = 0;
    Surface->info.rect.right  = Width;
    Surface->info.rect.bottom = Height;

    offsetX = X * bitsPerPixel / 8;
    offsetY = Y * Surface->info.stride;

    /* Compute the surface size. */
    Surface->info.size
        = Surface->info.stride
        * Surface->info.alignedHeight;

    /* Need to map physical pointer? */
    if (Surface->physical == ~0U)
    {
        /* Map the physical pointer. */
        gcmONERROR(
            gcoOS_MapUserMemory(gcvNULL,
                                (gctUINT8_PTR) Surface->logical + offsetY,
                                Surface->info.size,
                                &Surface->info.node.u.wrapped.mappingInfo,
                                &Surface->info.node.physical));

        gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL,
                                            &Surface->info.node.u.wrapped.mappingHardwareType));
    }
    else
    {
        Surface->info.node.physical = Surface->physical + offsetY;
    }

    /* Need to map logical pointer? */
    if (Surface->logical == gcvNULL)
    {
        gctPOINTER pointer = gcvNULL;

        /* Map the logical pointer. */
        gcmONERROR(
            gcoHAL_MapMemory(gcvNULL,
                             gcmINT2PTR(Surface->physical + offsetY),
                             Surface->info.size,
                             &pointer));

        Surface->info.node.logical                 = pointer;
        Surface->info.node.u.wrapped.logicalMapped = gcvTRUE;
    }
    else
    {
        Surface->info.node.logical = (gctUINT8_PTR) Surface->logical + offsetY;
    }

    Surface->info.node.logical  += offsetX;
    Surface->info.node.physical += offsetX;

#if gcdSECURE_USER
    iface.command = gcvHAL_MAP_USER_MEMORY;
    iface.u.MapUserMemory.memory  = Surface->info.node.logical;
    iface.u.MapUserMemory.address = Surface->info.node.physical;
    iface.u.MapUserMemory.size    = Surface->info.size;
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    Surface->info.node.physical = (gctUINT32) Surface->info.node.logical;
#endif

    /* Validate the node. */
    Surface->info.node.lockCount = 1;
    Surface->info.node.valid     = gcvTRUE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSURF_ReferenceSurface
**
**  Increase reference count of a surface.
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoSURF_ReferenceSurface(
    IN gcoSURF Surface
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    Surface->referenceCount++;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  gcoSURF_QueryReferenceCount
**
**  Query reference count of a surface
**
**  INPUT:
**
**      gcoSURF Surface
**          Pointer to gcoSURF object.
**
**  OUTPUT:
**
**      gctINT32 ReferenceCount
**          Reference count of a surface
*/
gceSTATUS
gcoSURF_QueryReferenceCount(
    IN gcoSURF Surface,
    OUT gctINT32 * ReferenceCount
    )
{
    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    *ReferenceCount = Surface->referenceCount;

    gcmFOOTER_ARG("*ReferenceCount=%d", *ReferenceCount);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSURF_SetOffset(
    IN gcoSURF Surface,
    IN gctUINT Offset
    )
{
    gceSTATUS   status = gcvSTATUS_OK;

    Surface->info.offset = Offset;

    return status;
}

gceSTATUS
gcoSURF_IsRenderable(
    IN gcoSURF Surface
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Check whether the surface is renderable. */
    status = gcoHARDWARE_IsSurfaceRenderable(&Surface->info);

    /* Return status. */
    gcmFOOTER();
    return status;
}

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
#include "gc_hal_compiler.h"
#include "gc_hal_engine.h"

#ifndef VIVANTE_NO_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE



static gceSTATUS
_AutoSetEarlyDepth(
    IN gcoHARDWARE Hardware,
    OUT gctBOOL_PTR Enable OPTIONAL
    )
{
    gctBOOL enable = Hardware->earlyDepth;

    if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))))
    {
        enable = gcvFALSE;
    }

    /* GC500 does not support early-depth when tile status is enabled at D16. */
    else
    if ((Hardware->chipModel == gcv500)
    &&  (Hardware->chipRevision < 3)
    &&  (Hardware->depthStates.surface != gcvNULL)
    &&  (Hardware->depthStates.surface->format == gcvSURF_D16)
    )
    {
        enable = gcvFALSE;
    }

    /* Early-depth should be disabled when stencil mode is not set to keep. */
    else
    if ((Hardware->stencilStates.mode != gcvSTENCIL_NONE)
    &&  (!Hardware->stencilKeepFront[0]
        || !Hardware->stencilKeepFront[1]
        || !Hardware->stencilKeepFront[2]
        )
    )
    {
        enable = gcvFALSE;
    }

    /* Early-depth should be disabled when two-sided stencil mode is not set
    ** keep. */
    else
    if ((Hardware->stencilStates.mode == gcvSTENCIL_DOUBLE_SIDED)
    &&  (!Hardware->stencilKeepBack[0]
        || !Hardware->stencilKeepBack[1]
        || !Hardware->stencilKeepBack[2]
        )
    )
    {
        enable = gcvFALSE;
    }

    else
    if ((Hardware->depthStates.surface != gcvNULL)
    &&  (Hardware->depthStates.surface->hzNode.size > 0)
    )
    {
        enable = gcvFALSE;
    }

    if (Enable != gcvNULL)
    {
        *Enable = enable;
    }

    if (Hardware->depthStates.early != enable)
    {
        Hardware->depthStates.early = enable;
        Hardware->depthConfigDirty  = gcvTRUE;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvertFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT Format,
    OUT gctUINT32 * HardwareFormat
    )
{
    switch (Format)
    {
    case gcvSURF_X4R4G4B4:
        *HardwareFormat = 0x0;
        return gcvSTATUS_OK;

    case gcvSURF_A4R4G4B4:
        *HardwareFormat = 0x1;
        return gcvSTATUS_OK;

    case gcvSURF_X1R5G5B5:
        *HardwareFormat = 0x2;
        return gcvSTATUS_OK;

    case gcvSURF_A1R5G5B5:
        *HardwareFormat = 0x3;
        return gcvSTATUS_OK;

    case gcvSURF_R5G6B5:
        *HardwareFormat = 0x4;
        return gcvSTATUS_OK;

    case gcvSURF_YUY2:
        if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 24:24) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))))
        {
            *HardwareFormat = 0x7;
            return gcvSTATUS_OK;
        }
        else
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }

    case gcvSURF_X8R8G8B8:
        *HardwareFormat = 0x5;
        return gcvSTATUS_OK;

    case gcvSURF_A8R8G8B8:
        *HardwareFormat = 0x6;
        return gcvSTATUS_OK;


    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }
}

gceSTATUS
gcoHARDWARE_SetRenderTarget(
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;
	gcoHARDWARE hardware;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmGETHARDWARE(hardware);

    /* Copy back the dirty bit. */
    if (hardware->colorStates.surface != gcvNULL)
    {
        hardware->colorStates.surface->dirty = hardware->colorStates.dirty;
    }

    /* Set current render target. */
    hardware->colorStates.surface = Surface;

    if (Surface != gcvNULL)
    {
        /* Make sure the surface is locked. */
        gcmASSERT(Surface->node.valid);

        /* Copy dirty flag. */
        hardware->colorStates.dirty = Surface->dirty;

        /* Save configuration. */
        hardware->samples = Surface->samples;

        hardware->vaa
            = Surface->vaa
                ? (Surface->format == gcvSURF_A8R8G8B8)
                    ? gcvVAA_COVERAGE_8
                    : gcvVAA_COVERAGE_16
                : gcvVAA_NONE;

        /* Schedule the surface to be reprogrammed. */
        hardware->colorConfigDirty = gcvTRUE;
        hardware->colorTargetDirty = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthBuffer(
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;
	gcoHARDWARE hardware;

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmGETHARDWARE(hardware);

    /* Set current depth buffer. */
    hardware->depthStates.surface = Surface;

    /* Set super-tiling. */
    hardware->depthStates.config =
        ((((gctUINT32) (hardware->depthStates.config)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) ((Surface == gcvNULL) ? 0 : Surface->superTiled) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26)));

    /* Enable/disable Eearly Depth. */
    gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));

    /* Make depth and stencil dirty. */
    hardware->depthConfigDirty = gcvTRUE;
    hardware->depthTargetDirty = gcvTRUE;
    hardware->stencilDirty     = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetViewport(
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gceSTATUS status;
    gcoHARDWARE Hardware;

    gcmHEADER_ARG("Left=%d Top=%d Right=%d Bottom=%d",
                  Left, Top, Right, Bottom);

    gcmGETHARDWARE(Hardware);

    /* Save the states. */
    Hardware->viewportStates.left   = Left;
    Hardware->viewportStates.top    = Top;
    Hardware->viewportStates.right  = Right;
    Hardware->viewportStates.bottom = Bottom;
    Hardware->viewportDirty         = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetScissors(
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gceSTATUS status;
	gcoHARDWARE hardware;

    gcmHEADER_ARG("Left=%d Top=%d Right=%d Bottom=%d",
                  Left, Top, Right, Bottom);

    gcmGETHARDWARE(hardware);

    /* Save the states. */
    hardware->scissorStates.left   = Left;
    hardware->scissorStates.top    = Top;
    hardware->scissorStates.right  = Right;
    hardware->scissorStates.bottom = Bottom;
    hardware->scissorDirty         = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----- Alpha Blend States ---------------------------------------------------*/

gceSTATUS
gcoHARDWARE_SetBlendEnable(
    IN gctBOOL Enabled
    )
{
    gceSTATUS status;
	gcoHARDWARE hardware;

    gcmHEADER_ARG("Enabled=%d", Enabled);

    gcmGETHARDWARE(hardware);

    /* Save the state. */
    hardware->alphaStates.blend = Enabled;
    hardware->alphaDirty        = gcvTRUE;
    hardware->colorConfigDirty  = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetBlendFunctionSource(
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("FunctionRGB=%d FunctionAlpha=%d",
                  FunctionRGB, FunctionAlpha);

    gcmGETHARDWARE(hardware);

    /* Save the states. */
    hardware->alphaStates.srcFuncColor = FunctionRGB;
    hardware->alphaStates.srcFuncAlpha = FunctionAlpha;
    hardware->alphaDirty               = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetBlendFunctionTarget(
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("FunctionRGB=%d FunctionAlpha=%d",
                  FunctionRGB, FunctionAlpha);

    gcmGETHARDWARE(hardware);

    /* Save the states. */
    hardware->alphaStates.trgFuncColor = FunctionRGB;
    hardware->alphaStates.trgFuncAlpha = FunctionAlpha;
    hardware->alphaDirty               = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetBlendMode(
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("ModeRGB=%d ModeAlpha=%d",
                  ModeRGB, ModeAlpha);

    gcmGETHARDWARE(hardware);

    /* Save the states. */
    hardware->alphaStates.modeColor = ModeRGB;
    hardware->alphaStates.modeAlpha = ModeAlpha;
    hardware->alphaDirty            = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetBlendColor(
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Red=%d Green=%d Blue=%d Alpha=%d",
                  Red, Green, Blue, Alpha);

    gcmGETHARDWARE(hardware);

    /* Save the state. */
    hardware->alphaStates.color
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (Red) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (Green) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (Blue) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (Alpha) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)));
    hardware->alphaDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetBlendColorX(
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;
    gctUINT8 red, green, blue, alpha;
    gctFIXED_POINT redSat, greenSat, blueSat, alphaSat;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Red=%d Green=%d Blue=%d Alpha=%d",
                  Red, Green, Blue, Alpha);

    gcmGETHARDWARE(hardware);

    /* Saturate fixed point values. */
    redSat   = gcmMIN(gcmMAX(Red, 0), gcvONE_X);
    greenSat = gcmMIN(gcmMAX(Green, 0), gcvONE_X);
    blueSat  = gcmMIN(gcmMAX(Blue, 0), gcvONE_X);
    alphaSat = gcmMIN(gcmMAX(Alpha, 0), gcvONE_X);

    /* Convert fixed point to 8-bit. */
    red   = (gctUINT8) (gcmXMultiply(redSat, (255 << 16)) >> 16);
    green = (gctUINT8) (gcmXMultiply(greenSat, (255 << 16)) >> 16);
    blue  = (gctUINT8) (gcmXMultiply(blueSat, (255 << 16)) >> 16);
    alpha = (gctUINT8) (gcmXMultiply(alphaSat, (255 << 16)) >> 16);

    /* Save the state. */
    hardware->alphaStates.color
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (red) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (green) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (blue) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (alpha) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)));
    hardware->alphaDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetBlendColorF(
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gctUINT8 red, green, blue, alpha;

    gcmHEADER_ARG("Red=%f Green=%f Blue=%f Alpha=%f",
                  Red, Green, Blue, Alpha);

    gcmGETHARDWARE(hardware);

    /* Convert floating point to 8-bit. */
    red   = gcoMATH_Float2NormalizedUInt8(gcmMIN(gcmMAX(Red, 0.0f), 1.0f));
    green = gcoMATH_Float2NormalizedUInt8(gcmMIN(gcmMAX(Green, 0.0f), 1.0f));
    blue  = gcoMATH_Float2NormalizedUInt8(gcmMIN(gcmMAX(Blue, 0.0f), 1.0f));
    alpha = gcoMATH_Float2NormalizedUInt8(gcmMIN(gcmMAX(Alpha, 0.0f), 1.0f));

    /* Save the state. */
    hardware->alphaStates.color
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (red) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (green) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (blue) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (alpha) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)));
    hardware->alphaDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----- Depth States. --------------------------------------------------------*/

gceSTATUS
gcoHARDWARE_SetDepthCompare(
    IN gceCOMPARE DepthCompare
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("DepthCompare=%d", DepthCompare);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the depth compare. */
    if (hardware->depthStates.compare != DepthCompare)
    {
        hardware->depthStates.compare = DepthCompare;
        hardware->depthConfigDirty    = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthWrite(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save write enable. */
    if (hardware->depthStates.write != Enable)
    {
        hardware->depthStates.write = Enable;
        hardware->depthConfigDirty  = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthMode(
    IN gceDEPTH_MODE DepthMode
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("DepthMode=%d", DepthMode);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save depth type. */
    if (hardware->depthStates.mode != DepthMode)
    {
        hardware->depthStates.mode = DepthMode;
        hardware->depthConfigDirty = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthRangeX(
    IN gceDEPTH_MODE DepthMode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("DepthMode=%d Near=%f Far=%f",
                  DepthMode,
                  gcoMATH_Fixed2Float(Near),
                  gcoMATH_Fixed2Float(Far));

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save depth type. */
    hardware->depthStates.mode = DepthMode;
    hardware->depthStates.near = gcoMATH_Fixed2Float(Near);
    hardware->depthStates.far  = gcoMATH_Fixed2Float(Far);
    hardware->depthConfigDirty = gcvTRUE;
    hardware->depthRangeDirty  = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthRangeF(
    IN gceDEPTH_MODE DepthMode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("DepthMode=%d Near=%f Far=%f",
                  DepthMode, Near, Far);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save depth type. */
    hardware->depthStates.mode = DepthMode;
    hardware->depthStates.near = Near;
    hardware->depthStates.far  = Far;
    hardware->depthConfigDirty = gcvTRUE;
    hardware->depthRangeDirty  = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetLastPixelEnable(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Enable=%d", Enable);


    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));
    gcmONERROR(gcoHARDWARE_LoadState32(
        gcvNULL, 0x00C18, Enable
        ));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthScaleBiasX(
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("DepthScale=%d DepthBias=%d",
                  DepthScale, DepthBias);

    gcmGETHARDWARE(hardware);

    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_DEPTH_BIAS_FIX)
        != gcvSTATUS_TRUE)
    {
        DepthScale = 0x0;
        DepthBias  = 0x0;
    }

    /* Switch to 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Program the scale register. */
    gcmONERROR(gcoHARDWARE_LoadState32x(
        hardware, 0x00C10, DepthScale
        ));

    /* Program the bias register. */
    gcmONERROR(gcoHARDWARE_LoadState32x(
        hardware, 0x00C14, DepthBias
        ));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthScaleBiasF(
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    )
{
    gceSTATUS status;
    gcuFLOAT_UINT32 scale, bias;

    gcmHEADER_ARG("DepthScale=%f DepthBias=%f",
                  DepthScale, DepthBias);


    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_DEPTH_BIAS_FIX)
        != gcvSTATUS_TRUE)
    {
        scale.f = 0.0f;
        bias.f  = 0.0f;
    }
    else
    {
        scale.f = DepthScale;
        bias.f  = DepthBias;
    }

    /* Switch to 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Program the scale register. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        gcvNULL, 0x00C10, scale.u
        ));

    /* Program the bias register. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        gcvNULL, 0x00C14, bias.u
        ));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetColorWrite(
    IN gctUINT8 Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    if (hardware->colorStates.colorWrite != Enable)
    {
        /* Reprogram PE dither table. */
        hardware->peDitherDirty        = gcvTRUE;
    }

    /* Save the color write. */
    hardware->colorStates.colorWrite = Enable;
    hardware->colorConfigDirty       = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetEarlyDepth(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Only handle when the IP has Early Depth */
    if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x0 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))))
    {
        /* Set early depth flag. */
        hardware->earlyDepth = Enable;

        /* Auto enable/disable early depth. */
        gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* Used to disable all early-z culling modes. */
gceSTATUS
gcoHARDWARE_SetAllEarlyDepthModes(
    IN gctBOOL Disable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Disable=%d", Disable);

    gcmGETHARDWARE(hardware);

    if (hardware->disableAllEarlyDepth != Disable)
    {
        /* Set disable all early depth flag. */
        hardware->disableAllEarlyDepth = Disable;
        hardware->depthConfigDirty  = gcvTRUE;

        /* Enable/Disable the EEZ. */
        gcmONERROR(gcoHARDWARE_LoadState32(
            gcvNULL, 0x00E08,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (!Disable) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
            ));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----- Stencil States -------------------------------------------------------*/

gceSTATUS
gcoHARDWARE_SetStencilMode(
    IN gceSTENCIL_MODE Mode
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Mode=%d", Mode);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save stencil mode. */
    hardware->stencilStates.mode = Mode;
    hardware->stencilDirty       = gcvTRUE;

    /* Enable/disable Eearly Depth. */
    gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilMask(
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Mask=0x%x", Mask);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil mask. */
    hardware->stencilStates.mask = Mask;
    hardware->stencilDirty       = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilWriteMask(
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Mask=0x%x", Mask);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil write mask. */
    hardware->stencilStates.writeMask = Mask;
    hardware->stencilDirty            = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilReference(
    IN gctUINT8 Reference,
    IN gctBOOL Front
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Reference=0x%x Front=%d",
                  Reference, Front);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil reference. */
    if (Front)
    {
        hardware->stencilStates.referenceFront = Reference;
    }
    else
    {
        hardware->stencilStates.referenceBack = Reference;
    }
    hardware->stencilDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilCompare(
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Where=%d Compare=%d",
                  Where, Compare);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil compare. */
    if (Where == gcvSTENCIL_FRONT)
    {
        hardware->stencilStates.compareFront = Compare;
    }
    else
    {
        hardware->stencilStates.compareBack = Compare;
    }
    hardware->stencilDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilPass(
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Where=%d Operation=%d",
                  Where, Operation);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil pass operation. */
    if (Where == gcvSTENCIL_FRONT)
    {
        hardware->stencilStates.passFront = Operation;
        hardware->stencilKeepFront[0]     = (Operation == gcvSTENCIL_KEEP);
    }
    else
    {
        hardware->stencilStates.passBack = Operation;
        hardware->stencilKeepBack[0]     = (Operation == gcvSTENCIL_KEEP);
    }

    /* Enable/disable Eearly Depth. */
    gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));

    /* Invalidate stencil. */
    hardware->stencilDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilFail(
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Where=%d Operation=%d",
                  Where, Operation);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil fail operation. */
    if (Where == gcvSTENCIL_FRONT)
    {
        hardware->stencilStates.failFront = Operation;
        hardware->stencilKeepFront[1]     = (Operation == gcvSTENCIL_KEEP);
    }
    else
    {
        hardware->stencilStates.failBack = Operation;
        hardware->stencilKeepBack[1]     = (Operation == gcvSTENCIL_KEEP);
    }

    /* Enable/disable Eearly Depth. */
    gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));

    /* Invalidate stencil. */
    hardware->stencilDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilDepthFail(
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Where=%d Operation=%d",
                  Where, Operation);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the stencil depth fail operation. */
    if (Where == gcvSTENCIL_FRONT)
    {
        hardware->stencilStates.depthFailFront = Operation;
        hardware->stencilKeepFront[2]          = (Operation == gcvSTENCIL_KEEP);
    }
    else
    {
        hardware->stencilStates.depthFailBack = Operation;
        hardware->stencilKeepBack[2]          = (Operation == gcvSTENCIL_KEEP);
    }

    /* Enable/disable Eearly Depth. */
    gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));

    /* Invalidate stencil. */
    hardware->stencilDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetStencilAll(
    IN gcsSTENCIL_INFO_PTR Info
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Info=0x%x", Info);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    /* We only need to update stencil states if there is a stencil buffer. */
    if ((hardware->depthStates.surface         != gcvNULL)
    &&  (hardware->depthStates.surface->format == gcvSURF_D24S8)
    )
    {
        /* Copy the states. */
       hardware->stencilStates = *Info;

        /* Some more updated based on a few states. */
        hardware->stencilKeepFront[0]
            = (Info->passFront      == gcvSTENCIL_KEEP);
        hardware->stencilKeepFront[1]
            = (Info->failFront      == gcvSTENCIL_KEEP);
        hardware->stencilKeepFront[2]
            = (Info->depthFailFront == gcvSTENCIL_KEEP);

        hardware->stencilKeepBack[0]
            = (Info->passBack      == gcvSTENCIL_KEEP);
        hardware->stencilKeepBack[1]
            = (Info->failBack      == gcvSTENCIL_KEEP);
        hardware->stencilKeepBack[2]
            = (Info->depthFailBack == gcvSTENCIL_KEEP);

        /* Enable/disable Eearly Depth. */
        gcmONERROR(_AutoSetEarlyDepth(hardware, gcvNULL));

        /* Mark stencil states as dirty. */
        hardware->stencilDirty = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----- Alpha Test States ----------------------------------------------------*/

gceSTATUS
gcoHARDWARE_SetAlphaTest(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the state. */
    hardware->alphaStates.test = Enable;
    hardware->alphaDirty       = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAlphaCompare(
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Compare=%d", Compare);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the state. */
    hardware->alphaStates.compare = Compare;
    hardware->alphaDirty          = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAlphaReference(
    IN gctUINT8 Reference
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Reference=%u", Reference);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the state. */
    hardware->alphaStates.reference = Reference;
    hardware->alphaDirty            = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAlphaReferenceX(
    IN gctFIXED_POINT Reference
    )
{
    gceSTATUS status;
    gctFIXED_POINT saturated;
    gctUINT8 reference;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Reference=%f",
                  gcoMATH_Fixed2Float(Reference));

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Saturate fixed point between 0 and 1. */
    saturated = gcmMIN(gcmMAX(Reference, 0), gcvONE_X);

    /* Convert fixed point to gctUINT8. */
    reference = (gctUINT8) (gcmXMultiply(saturated, 255 << 16) >> 16);

    /* Save the state. */
    hardware->alphaStates.reference = reference;
    hardware->alphaDirty            = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAlphaReferenceF(
    IN gctFLOAT Reference
    )
{
    gceSTATUS status;
    gctFLOAT saturated;
    gctUINT8 reference;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Reference=%f", Reference);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Saturate floating point between 0 and 1. */
    saturated = gcmMIN(gcmMAX(Reference, 0.0f), 1.0f);

    /* Convert floating point to gctUINT8. */
    reference = gcoMATH_Float2NormalizedUInt8(saturated);

    /* Save the state. */
    hardware->alphaStates.reference = reference;
    hardware->alphaDirty            = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAlphaAll(
    IN gcsALPHA_INFO_PTR Info
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Info=0x%x", Info);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    /* Copy the states. */
    gcmONERROR(gcoOS_MemCopy(
        &hardware->alphaStates, Info, gcmSIZEOF(hardware->alphaStates)
        ));

    /* Mark alpha states as dirty. */
    hardware->alphaDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----------------------------------------------------------------------------*/

gceSTATUS
gcoHARDWARE_BindIndex(
    IN gctUINT32 Address,
    IN gceINDEX_TYPE IndexType
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gctUINT32 indexSize = 0, endian;

    gcmHEADER_ARG("Address=0x%x IndexType=%d",
                  Address, IndexType);

    gcmGETHARDWARE(hardware);

    endian = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)));

    switch (IndexType)
    {
    case gcvINDEX_8:
        indexSize = 0x0;
        break;

    case gcvINDEX_16:
        indexSize = 0x1;

        if (hardware->bigEndian)
        {
            endian = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)));
        }
        break;

    case gcvINDEX_32:
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 31:31) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))))
        {
            indexSize = 0x2;

            if (hardware->bigEndian)
            {
                endian = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)));
            }
            break;
        }

        /* Fall through. */
    default:
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Program the AQCmdStreamBaseAddr register. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        hardware, 0x00644, Address
        ));

    /* Program the AQCmdStreamCtrl register. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        hardware, 0x00648, indexSize | endian
        ));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAPI(
    IN gceAPI Api
    )
{
    gceSTATUS status;
    gctUINT32 system = 0;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Api=%d", Api);

    gcmGETHARDWARE(hardware);

    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Don't do anything if the pipe has already been selected. */
    if (hardware->currentApi != Api)
    {
        /* Set new pipe. */
        hardware->currentApi = Api;

        /* Set PA System Mode: D3D or OPENGL. */
        switch (Api)
        {
        case gcvAPI_OPENGL:
        case gcvAPI_OPENVG:
        case gcvAPI_OPENCL:
            hardware->api = gcvAPI_OPENGL;
            system = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                   | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));
            break;

        case gcvAPI_D3D:
            hardware->api = gcvAPI_D3D;
            system = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                   | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));
            break;

        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        hardware->centroidsDirty = gcvTRUE;

        gcmONERROR(gcoHARDWARE_LoadState32(hardware, 0x00A28, system));

        /* Set Common API Mode: OpenGL, OpenVG, or OpenCL.
           This is for VS and PS to handle texel conversion. */
        switch (Api)
        {
        case gcvAPI_OPENGL:
            system = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
            break;

        case gcvAPI_OPENVG:
            system = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
            break;

        case gcvAPI_OPENCL:
            hardware->api = gcvAPI_OPENCL;
            system = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
            break;

        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        gcmONERROR(gcoHARDWARE_LoadState32(hardware, 0x0384C, system));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----- Primitive assembly states. -------------------------------------------*/

gceSTATUS
gcoHARDWARE_SetAntiAliasLine(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.aaLine = Enable;
    hardware->paConfigDirty   = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAALineTexSlot(
    IN gctUINT TexSlot
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("TexSlot=%d", TexSlot);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.aaLineTexSlot = TexSlot;
    hardware->paConfigDirty          = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetShading(
    IN gceSHADING Shading
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Shading=%d", Shading);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.shading = Shading;
    hardware->paConfigDirty    = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetCulling(
    IN gceCULL Mode
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Mode=%d", Mode);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.culling = Mode;
    hardware->paConfigDirty    = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetPointSizeEnable(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.pointSize = Enable;
    hardware->paConfigDirty      = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetPointSprite(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.pointSprite = Enable;
    hardware->paConfigDirty        = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetFill(
    IN gceFILL Mode
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Mode=%d", Mode);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.fillMode = Mode;
    hardware->paConfigDirty     = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetAALineWidth(
    IN gctFLOAT Width
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Width=%f", Width);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the state. */
    hardware->paStates.aaLineWidth = Width;
    hardware->paLineDirty          = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*------------------------------------------------------------------------------
**----------------------------------------------------------------------------*/

gceSTATUS
gcoHARDWARE_Initialize3D(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gctFLOAT limit;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* AQVertexElementConfig. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        gcvNULL, 0x03814,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
        ));

    /* AQRasterControl. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        gcvNULL, 0x00E00,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
        ));

    /* W Clip limit. */
    limit = 1.0f / 8388607.0f;
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x00A2C,
                                       *(gctUINT32 *) &limit));

#if gcd6000_SUPPORT
    gcmONERROR(gcoHARDWARE_LoadState32(Hardware,
                                       0x03A00,
                                       0x6));
#endif

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetDepthOnly(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Save the state. */
    hardware->depthStates.only = Enable;
    hardware->depthConfigDirty = gcvTRUE;

    if (Enable)
    {
        gcmASSERT(hardware->depthStates.surface != gcvNULL);

        hardware->vaa             = gcvVAA_NONE;
        hardware->samples         = hardware->depthStates.surface->samples;
        hardware->msaaConfigDirty = gcvTRUE;
        hardware->msaaModeDirty   = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetCentroids(
    IN gctUINT32 Index,
    IN gctPOINTER Centroids
    )
{
    gceSTATUS status;
    /*gcoHARDWARE hardware;*/
    gctUINT32 data[4], i;
    gctUINT8_PTR inputCentroids = (gctUINT8_PTR) Centroids;

    gcmHEADER_ARG("Index=%d Centroids=0x%x",
                    Index, Centroids);

    /*gcmGETHARDWARE(hardware);*/

    /* Verify the arguments. */
    /*gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);*/

    gcmONERROR(gcoOS_ZeroMemory(data, sizeof(data)));

    for (i = 0; i < 16; ++i)
    {
        switch (i & 3)
        {
        case 0:
            /* Set for centroid 0, 4, 8, or 12. */
            data[i / 4] = ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2]) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                             | ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2 + 1]) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)));
            break;

        case 1:
            /* Set for centroid 1, 5, 9, or 13. */
            data[i / 4] = ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2]) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                             | ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2 + 1]) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)));
            break;

        case 2:
            /* Set for centroid 2, 6, 10, or 14. */
            data[i / 4] = ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2]) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
                             | ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2 + 1]) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)));
            break;

        case 3:
            /* Set for centroid 3, 7, 11, or 15. */
            data[i / 4] = ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2]) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
                             | ((((gctUINT32) (data[i / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (inputCentroids[i * 2 + 1]) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));
            break;
        }
    }

    /* Program the centroid table. */
    gcmONERROR(gcoHARDWARE_LoadState(
        0x00E40 + Index * 16, 4, data
        ));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_IsSurfaceRenderable(
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gctUINT32 format;
    gcsSURF_FORMAT_INFO_PTR info[2];

    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    gcmONERROR(gcoSURF_QueryFormat(Surface->format, info));

    if ((Surface->type == gcvSURF_TEXTURE) &&
        (hardware->pixelPipes > 1) &&
        (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_TEXTURE_TILED_READ) != gcvSTATUS_TRUE))
    {
        gcmONERROR(gcvSTATUS_NOT_ALIGNED);
    }

    if ((hardware->pixelPipes > 1)
    && (Surface->tiling != gcvMULTI_TILED)
    && (Surface->tiling != gcvMULTI_SUPERTILED) )
    {
        gcmONERROR(gcvSTATUS_NOT_ALIGNED);
    }

    /* If surface is 16-bit, it has to be 8x4 aligned. */
    if (Surface->is16Bit)
    {
        if ((Surface->alignedWidth  & 7)
        ||  (Surface->alignedHeight & 3)
        )
        {
            gcmONERROR(gcvSTATUS_NOT_ALIGNED);
        }
    }

    /* Otherwise, the surface has to be 16x4 aligned. */
    else
    {
        if ((Surface->alignedWidth  & 15)
        ||  (Surface->alignedHeight &  3)
        )
        {
            gcmONERROR(gcvSTATUS_NOT_ALIGNED);
        }
    }

    /* Convert the surface format to a hardware format. */
    status = _ConvertFormat(hardware, Surface->format, &format);

    if (gcmIS_ERROR(status))
    {
        switch (Surface->format)
        {
        case gcvSURF_D16:
        case gcvSURF_D24X8:
        case gcvSURF_D24S8:
        case gcvSURF_D32:
            status = gcvSTATUS_OK;
            break;

        default:
            break;
        }
    }

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_SetLogicOp(
    IN gctUINT8 Rop
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Rop=%d", Rop);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Update ROP2 code. */
    hardware->colorStates.rop = Rop & 0xF;

    /* Schedule the surface to be reprogrammed. */
    if (hardware->colorStates.rop != 0xC)
    {
        hardware->colorConfigDirty = gcvTRUE;
    }

    /* Program the scale register. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        gcvNULL,
        0x014A4,
        (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (hardware->colorStates.rop) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))))
        ));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
******************************** State Flushers ********************************
\******************************************************************************/

gceSTATUS
gcoHARDWARE_FlushViewport(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsRECT viewport;
    gctUINT32 xScale, yScale, xOffset, yOffset;
    gctFLOAT wClip;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->viewportDirty)
    {
        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

        if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x0 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))))
        {
            viewport.left   = Hardware->viewportStates.left   * Hardware->samples.x;
            viewport.top    = Hardware->viewportStates.top    * Hardware->samples.y;
            viewport.right  = Hardware->viewportStates.right  * Hardware->samples.x;
            viewport.bottom = Hardware->viewportStates.bottom * Hardware->samples.y;
        }
        else
        {
            viewport.left   = Hardware->viewportStates.left;
            viewport.top    = Hardware->viewportStates.top;
            viewport.right  = Hardware->viewportStates.right;
            viewport.bottom = Hardware->viewportStates.bottom;
        }

        /* Compute viewport states. */
        xScale  = (viewport.right - viewport.left) << 15;
        xOffset = (viewport.left << 16) + xScale;

        if (viewport.top < viewport.bottom)
        {
            yScale = (Hardware->api == gcvAPI_OPENGL)
                         ? (-(viewport.bottom - viewport.top) << 15)
                         : ((viewport.bottom - viewport.top) << 15);
            yOffset = (Hardware->api == gcvAPI_OPENGL)
                          ? (viewport.top << 16) - yScale
                          : (viewport.top << 16) + yScale;
        }
        else
        {
            yScale = (Hardware->api == gcvAPI_OPENGL)
                         ? ((viewport.top - viewport.bottom) << 15)
                         : (-(viewport.top - viewport.bottom) << 15);
            yOffset = (Hardware->api == gcvAPI_OPENGL)
                          ? (viewport.bottom << 16) + yScale
                          : (viewport.bottom << 16) - yScale;
        }

        if ((Hardware->chipModel == gcv500) &&
             (Hardware->api == gcvAPI_OPENGL) )
        {
            xOffset -= 0x8000;
            yOffset -= 0x8000;
        }

        wClip = gcmMAX((Hardware->viewportStates.right
                        - Hardware->viewportStates.left
                        ),
                       (Hardware->viewportStates.bottom
                        - Hardware->viewportStates.top
                        )) / (2.0f * 8388607.0f - 8192.0f);

        /* Determine the size of the buffer to reserve. */
        reserveSize

            /* Scale registers. */
            = gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

            /* Offset registers. */
            + gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

            /* New PA clip registers. */
            + 4 * gcmSIZEOF(gctUINT);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0280, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0280) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0280,
                xScale
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0281,
                yScale
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0283, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0283) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0283,
                xOffset
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0284,
                yOffset
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* New PA clip registers. */
        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x02A0, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x02A0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x02A0, *(gctUINT32_PTR) &wClip);     gcmENDSTATEBATCH(reserve, memory);};

        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x02A1, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x02A1) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvTRUE, 0x02A1, 8192 << 16);     gcmENDSTATEBATCH(reserve, memory);};

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);

        /* Mark viewport as updated. */
        Hardware->viewportDirty = gcvFALSE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushScissor(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsRECT scissor;
    gctINT fixClip;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    if (Hardware->scissorDirty)
    {
        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

        if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x0 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))))
        {
            scissor.left   = Hardware->scissorStates.left   * Hardware->samples.x;
            scissor.top    = Hardware->scissorStates.top    * Hardware->samples.y;
            scissor.right  = Hardware->scissorStates.right  * Hardware->samples.x;
            scissor.bottom = Hardware->scissorStates.bottom * Hardware->samples.y;
        }
        else
        {
            scissor.left   = Hardware->scissorStates.left;
            scissor.top    = Hardware->scissorStates.top;
            scissor.right  = Hardware->scissorStates.right;
            scissor.bottom = Hardware->scissorStates.bottom;
        }

		/* Trivial case. */
        if ((scissor.left >= scissor.right) || (scissor.top >= scissor.bottom))
        {
            scissor.left   = 1;
            scissor.top    = 1;
            scissor.right  = 1;
            scissor.bottom = 1;
        }

        fixClip = (Hardware->vaa == gcvVAA_NONE) ? 11 : 0;

        /* Determine the size of the buffer to reserve. */
        reserveSize = gcmALIGN((1 + 4) * gcmSIZEOF(gctUINT32), 8)

                    /* New setup clip registers. */
                    + 4 * sizeof(gctUINT32);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0300, 4 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (4 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0300) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0300,
                scissor.left << 16
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0301,
                scissor.top << 16
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0302,
                (scissor.right << 16) + fixClip
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvTRUE, 0x0303,
                (scissor.bottom << 16) + fixClip
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* Program new states for setup clipping. */
        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0308, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0308) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvTRUE, 0x0308, (scissor.right << 16) + 0xFFFF);     gcmENDSTATEBATCH(reserve, memory);};

        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0309, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0309) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvTRUE, 0x0309, (scissor.bottom << 16) + 0xFFFF);     gcmENDSTATEBATCH(reserve, memory);};

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);

        /* Mark viewport as updated. */
        Hardware->scissorDirty = gcvFALSE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushAlpha(
    IN gcoHARDWARE Hardware
    )
{
    gctBOOL floatPiple = gcvFALSE;

    static const gctUINT32 xlateCompare[] =
    {
        /* gcvCOMPARE_NEVER */
        0x0,
        /* gcvCOMPARE_NOT_EQUAL */
        0x5,
        /* gcvCOMPARE_LESS */
        0x1,
        /* gcvCOMPARE_LESS_OR_EQUAL */
        0x3,
        /* gcvCOMPARE_EQUAL */
        0x2,
        /* gcvCOMPARE_GREATER */
        0x4,
        /* gcvCOMPARE_GREATER_OR_EQUAL */
        0x6,
        /* gcvCOMPARE_ALWAYS */
        0x7,
    };

    static const gctUINT32 xlateFuncSource[] =
    {
        /* gcvBLEND_ZERO */
        0x0,
        /* gcvBLEND_ONE */
        0x1,
        /* gcvBLEND_SOURCE_COLOR */
        0x2,
        /* gcvBLEND_INV_SOURCE_COLOR */
        0x3,
        /* gcvBLEND_SOURCE_ALPHA */
        0x4,
        /* gcvBLEND_INV_SOURCE_ALPHA */
        0x5,
        /* gcvBLEND_TARGET_COLOR */
        0x8,
        /* gcvBLEND_INV_TARGET_COLOR */
        0x9,
        /* gcvBLEND_TARGET_ALPHA */
        0x6,
        /* gcvBLEND_INV_TARGET_ALPHA */
        0x7,
        /* gcvBLEND_SOURCE_ALPHA_SATURATE */
        0xA,
        /* gcvBLEND_CONST_COLOR */
        0xD,
        /* gcvBLEND_INV_CONST_COLOR */
        0xE,
        /* gcvBLEND_CONST_ALPHA */
        0xB,
        /* gcvBLEND_INV_CONST_ALPHA */
        0xC,
    };

    static const gctUINT32 xlateFuncTarget[] =
    {
        /* gcvBLEND_ZERO */
        0x0,
        /* gcvBLEND_ONE */
        0x1,
        /* gcvBLEND_SOURCE_COLOR */
        0x2,
        /* gcvBLEND_INV_SOURCE_COLOR */
        0x3,
        /* gcvBLEND_SOURCE_ALPHA */
        0x4,
        /* gcvBLEND_INV_SOURCE_ALPHA */
        0x5,
        /* gcvBLEND_TARGET_COLOR */
        0x8,
        /* gcvBLEND_INV_TARGET_COLOR */
        0x9,
        /* gcvBLEND_TARGET_ALPHA */
        0x6,
        /* gcvBLEND_INV_TARGET_ALPHA */
        0x7,
        /* gcvBLEND_SOURCE_ALPHA_SATURATE */
        0xA,
        /* gcvBLEND_CONST_COLOR */
        0xD,
        /* gcvBLEND_INV_CONST_COLOR */
        0xE,
        /* gcvBLEND_CONST_ALPHA */
        0xB,
        /* gcvBLEND_INV_CONST_ALPHA */
        0xC,
    };

    static const gctUINT32 xlateMode[] =
    {
        /* gcvBLEND_ADD */
        0x0,
        /* gcvBLEND_SUBTRACT */
        0x1,
        /* gcvBLEND_REVERSE_SUBTRACT */
        0x2,
        /* gcvBLEND_MIN */
        0x3,
        /* gcvBLEND_MAX */
        0x4,
    };

    gceSTATUS status;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->alphaDirty)
    {
        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

        /* Determine the size of the buffer to reserve. */
        floatPiple = gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_HALF_FLOAT_PIPE);

        if (floatPiple)
        {
            reserveSize = gcmALIGN((1 + 3 + 2 + 2) * gcmSIZEOF(gctUINT32), 8);
        }
        else
        {
            reserveSize = gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8);
        }

        reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0508, 3 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (3 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0508) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0508,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (Hardware->alphaStates.test) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (xlateCompare[Hardware->alphaStates.compare]) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (Hardware->alphaStates.reference) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0509,
                Hardware->alphaStates.color
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x050A,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (Hardware->alphaStates.blend) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (Hardware->alphaStates.blend) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (xlateFuncSource[Hardware->alphaStates.srcFuncColor]) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (xlateFuncSource[Hardware->alphaStates.srcFuncAlpha]) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (xlateFuncTarget[Hardware->alphaStates.trgFuncColor]) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (xlateFuncTarget[Hardware->alphaStates.trgFuncAlpha]) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) ((gctUINT32) (xlateMode[Hardware->alphaStates.modeColor]) & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (xlateMode[Hardware->alphaStates.modeAlpha]) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

            if (floatPiple)
            {
                gctUINT32 colorLow;
                gctUINT32 colorHeigh;
                gctUINT16 colorR;
                gctUINT16 colorG;
                gctUINT16 colorB;
                gctUINT16 colorA;

                colorR = gcoMATH_UInt8AsFloat16((((((gctUINT32) (Hardware->alphaStates.color)) >> (0 ? 23:16)) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1)))))) ));
                colorG = gcoMATH_UInt8AsFloat16((((((gctUINT32) (Hardware->alphaStates.color)) >> (0 ? 15:8)) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1)))))) ));
                colorB = gcoMATH_UInt8AsFloat16((((((gctUINT32) (Hardware->alphaStates.color)) >> (0 ? 7:0)) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1)))))) ));
                colorA = gcoMATH_UInt8AsFloat16((((((gctUINT32) (Hardware->alphaStates.color)) >> (0 ? 31:24)) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1)))))) ));
                colorLow = colorR | (colorG << 16);
                colorHeigh = colorB | (colorA << 16);

                {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x052C, 1 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x052C) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x052C,
                    colorLow
                    );

                gcmENDSTATEBATCH(
                    reserve, memory
                    );

                {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x052D, 1 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x052D) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x052D,
                    colorHeigh
                    );

                gcmENDSTATEBATCH(
                    reserve, memory
                    );
            }

        {
            gctUINT16 reference;

            if (floatPiple)
            {
                reference = gcoMATH_UInt8AsFloat16(Hardware->alphaStates.reference);
            }
            else
            {
                /* Change to 12bit. */
                reference = (Hardware->alphaStates.reference << 4) | (Hardware->alphaStates.reference >> 4);
            }

            /* Setup the extra reference state. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0528, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0528) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0528, ((((gctUINT32) (~0U)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) & ((((gctUINT32) (~0U)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (reference) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) );     gcmENDSTATEBATCH(reserve, memory);};
        }

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);

        /* Mark alpha as updated. */
        Hardware->alphaDirty = gcvFALSE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushStencil(
    IN gcoHARDWARE Hardware
    )
{
    static const gctUINT8 xlateMode[] =
    {
        /* gcvSTENCIL_NONE */
        0x0,
        /* gcvSTENCIL_SINGLE_SIDED */
        0x1,
        /* gcvSTENCIL_DOUBLE_SIDED */
        0x2
    };

    static const gctUINT8 xlateCompare[] =
    {
        /* gcvCOMPARE_NEVER */
        0x0,
        /* gcvCOMPARE_NOT_EQUAL */
        0x5,
        /* gcvCOMPARE_LESS */
        0x1,
        /* gcvCOMPARE_LESS_OR_EQUAL */
        0x3,
        /* gcvCOMPARE_EQUAL */
        0x2,
        /* gcvCOMPARE_GREATER */
        0x4,
        /* gcvCOMPARE_GREATER_OR_EQUAL */
        0x6,
        /* gcvCOMPARE_ALWAYS */
        0x7
    };

    static const gctUINT8 xlateOperation[] =
    {
        /* gcvSTENCIL_KEEP */
        0x0,
        /* gcvSTENCIL_REPLACE */
        0x2,
        /* gcvSTENCIL_ZERO */
        0x1,
        /* gcvSTENCIL_INVERT */
        0x5,
        /* gcvSTENCIL_INCREMENT */
        0x6,
        /* gcvSTENCIL_DECREMENT */
        0x7,
        /* gcvSTENCIL_INCREMENT_SATURATE */
        0x3,
        /* gcvSTENCIL_DECREMENT_SATURATE */
        0x4,
    };

    gceSTATUS status;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->stencilDirty)
    {
        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

        if (Hardware->stencilEnabled)
        {
            gctBOOL flushNeeded;

            /* Determine if we need a stencil flush (synchronize the stencil
            ** buffers between PE and RA). */
            flushNeeded
                = !Hardware->flushedDepth
                && Hardware->depthStates.early
                && (
                    (Hardware->stencilStates.compareFront != gcvCOMPARE_ALWAYS)
                 || (Hardware->stencilStates.compareBack  != gcvCOMPARE_ALWAYS)
                );

            /* Determine the size of the buffer to reserve. */
            reserveSize = gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);

            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

            if (flushNeeded)
            {
                /* Mark depth cache as flushed. */
                Hardware->flushedDepth = gcvTRUE;

                /* Append flush state. */
                reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            }

            /* Reserve space in the command buffer. */
            gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0506, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0506) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                /* Setup the stencil operation state. */
                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0506,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (xlateCompare[Hardware->stencilStates.compareFront]) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (xlateOperation[Hardware->stencilStates.passFront]) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (xlateOperation[Hardware->stencilStates.failFront]) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) ((gctUINT32) (xlateOperation[Hardware->stencilStates.depthFailFront]) & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) | (((gctUINT32) ((gctUINT32) (xlateCompare[Hardware->stencilStates.compareBack]) & ((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20))) | (((gctUINT32) ((gctUINT32) (xlateOperation[Hardware->stencilStates.passBack]) & ((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:24) - (0 ? 26:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24))) | (((gctUINT32) ((gctUINT32) (xlateOperation[Hardware->stencilStates.failBack]) & ((gctUINT32) ((((1 ? 26:24) - (0 ? 26:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (xlateOperation[Hardware->stencilStates.depthFailBack]) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))
                    );

                /* Setup the stencil config state. */
                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0507,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (xlateMode[Hardware->stencilStates.mode]) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (Hardware->stencilStates.referenceFront) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (Hardware->stencilStates.mask) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (Hardware->stencilStates.writeMask) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                    );

                gcmSETFILLER(
                    reserve, memory
                    );

            gcmENDSTATEBATCH(
                reserve, memory
                );

            /* Setup the extra reference state. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0528, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0528) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0528, ((((gctUINT32) (~0U)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) & ((((gctUINT32) (~0U)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (Hardware->stencilStates.referenceBack) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) );     gcmENDSTATEBATCH(reserve, memory);};


            /* If early-depth is enabled and we need a depth cache flush, do it! */
            if (flushNeeded)
            {
                {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};

                /* Validate the state buffer. */
                gcmENDSTATEBUFFER(reserve, memory, reserveSize);

                /* We also have to stall between RA and PE to make sure the flush
                ** has happened. */
                gcmONERROR(gcoHARDWARE_Semaphore(
                    gcvWHERE_RASTER, gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL
                    ));
            }
            else
            {
                /* Validate the state buffer. */
                gcmENDSTATEBUFFER(reserve, memory, reserveSize);
            }
        }

        else
        {
            /* Determine the size of the buffer to reserve. */
            reserveSize
                = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

            /* Reserve space in the command buffer. */
            gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

            /* Disable stencil. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0507, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0507) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0507, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) );     gcmENDSTATEBATCH(reserve, memory);};

            /* Validate the state buffer. */
            gcmENDSTATEBUFFER(reserve, memory, reserveSize);
        }

        /* Mark stencil as updated. */
        Hardware->stencilDirty = gcvFALSE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushTarget(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsSURF_INFO_PTR surface = gcvNULL;
    gctUINT32 destinationRead = 0;
    gctUINT32 samples = 0;
    gctBOOL flushNeeded = gcvFALSE;
    gctUINT batchCount;
    gctBOOL colorBatchLoad;
    gctBOOL colorSplitLoad;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* At least configuration has to be dirty. */
    gcmASSERT(Hardware->colorConfigDirty);

    /* Determine destination read control. */
    destinationRead
        = (Hardware->colorStates.colorCompression
          || Hardware->alphaStates.blend
          || (Hardware->colorStates.colorWrite != 0xF)
          || (Hardware->colorStates.rop != 0xC)
          || ((((gctUINT32) (Hardware->memoryConfig)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x0 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))))
          )
          ? 0x0
          : 0x1;

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HARDWARE,
             "Read control: compression=%d alpha=%d writeEnable=%X ==> %d",
             Hardware->colorStates.colorCompression,
             Hardware->alphaStates.blend,
             Hardware->colorStates.colorWrite,
             destinationRead);

    /* Determine whether the flush is needed. */
    if (destinationRead != Hardware->colorStates.destinationRead)
    {
        /* Flush the cache. */
        flushNeeded = !Hardware->flushedColor;

        /* Update the read control value. */
        Hardware->colorStates.destinationRead = destinationRead;
    }

    if (Hardware->colorTargetDirty)
    {
        gctUINT32 cacheMode;

        gcmASSERT(Hardware->colorConfigDirty);

        /* Get a shortcut to the surface. */
        surface = Hardware->colorStates.surface;

        if (surface == gcvNULL)
        {
            /* Set the batch count. */
            batchCount = 1;

            /* Setup load flags. */
            colorBatchLoad = gcvFALSE;
            colorSplitLoad = gcvFALSE;

            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* Configuration state. */
                = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
        else
        {
            /* Convert format to pixel engine format. */
            gcmONERROR(_ConvertFormat(
                Hardware, surface->format, &Hardware->colorStates.format
                ));

            /* Set the supertiled state. */
            Hardware->colorStates.superTiled
                = Hardware->colorStates.surface->superTiled;

            /* Compute the numnber of samples. */
            samples = surface->samples.x * surface->samples.y;

            /* Determine the cache mode (equation is taken from RTL). */
            cacheMode = (samples == 4) && !surface->is16Bit;

            /* Determine whether the flush is needed. */
            if (cacheMode != Hardware->colorStates.cacheMode)
            {
                /* Flush the cache. */
                flushNeeded = !Hardware->flushedColor;

                /* Update the cache mode. */
                Hardware->colorStates.cacheMode = cacheMode;
            }

            if (Hardware->pixelPipes > 1
             || Hardware->renderTargets > 1)
            {
                /* Set the batch count. */
                batchCount = 1;

                /* Setup load flags. */
                colorBatchLoad = gcvFALSE;
                colorSplitLoad = gcvTRUE;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Configuration state. */
                    = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)


                    /* Stride state. */
                    + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

                if (Hardware->pixelPipes > 1)
                {
                    /* Surface addresses. */
                    reserveSize += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
                }
                else
                {
                    /* Surface addresses. */
                    reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
                }
            }
            else
            {
                /* Set the batch count. */
                batchCount = 3;

                /* Setup load flags. */
                colorBatchLoad = gcvTRUE;
                colorSplitLoad = gcvFALSE;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Color states. */
                    = gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8);
            }
        }
    }
    else
    {
        /* Set the batch count. */
        batchCount = 1;

        /* Setup load flags. */
        colorBatchLoad = gcvFALSE;
        colorSplitLoad = gcvFALSE;

        /* Determine the size of the buffer to reserve. */
        reserveSize

            /* Configuration state. */
            = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
    }

    if (flushNeeded)
    {
        /* Mark color cache as flushed. */
        Hardware->flushedColor = gcvTRUE;

        /* Add flush state. */
        reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
    }

    if (Hardware->peDitherDirty)
    {
        /* Add flush state. */
        reserveSize += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
    }

    /* Switch to 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* We need to flush the cache if the cache mode is changing. */
    if (flushNeeded)
    {
        /* Flush the cache. */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) );gcmENDSTATEBATCH(reserve, memory);};
    }

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x050B, batchCount );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (batchCount ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x050B) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        /* Program AQPixelConfig.Format field. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x050B,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (Hardware->colorStates.format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))) | (((gctUINT32) ((gctUINT32) (Hardware->colorStates.superTiled) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (Hardware->colorStates.destinationRead) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (Hardware->colorStates.colorWrite) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
            );

        if (colorBatchLoad)
        {
            /* Program AQPixelAddress register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x050C,
                surface->node.physical + surface->offset
                );

            /* Program AQPixelStride register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x050D,
                surface->stride
                );
        }

    gcmENDSTATEBATCH(
        reserve, memory
        );

    if (Hardware->peDitherDirty)
    {
        gctUINT32 dither[2];

        if (Hardware->colorStates.colorWrite != 0xF)
        {
            /* Reset dither table, if any color channel is disabled. */
            dither[0] = dither[1] = ~0U;
        }
        else
        {
            dither[0] = Hardware->dither[0];
            dither[1] = Hardware->dither[1];
        }

        /* Append RESOLVE_DITHER commands. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x052A, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x052A) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x052A,
                dither[0]
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x052B,
                dither[1]
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* Reset dirty flags. */
        Hardware->peDitherDirty = gcvFALSE;
    }

    if (colorSplitLoad)
    {
       if(Hardware->pixelPipes > 1)
       {
            gctUINT32 bankOffsetBytes = 0;
            gctINT topBufferSize = gcmALIGN(surface->alignedHeight / 2,
                                            surface->superTiled ? 64 : 4)
                                 * surface->stride;

            /* Extra pages needed to offset sub-buffers to different banks. */
#if gcdENABLE_BANK_ALIGNMENT
            gcmONERROR(gcoSURF_GetBankOffsetBytes(
                gcvNULL,
                surface->type, topBufferSize, &bankOffsetBytes
                ));

            /* If surface doesn't have enough padding, then don't offset it. */
            if (surface->size <
                ((surface->alignedHeight * surface->stride) + bankOffsetBytes))
            {
                bankOffsetBytes = 0;
            }
#endif

            gcmASSERT(Hardware->pixelPipes == 2);

            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0518, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0518) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0518,
                    surface->node.physical
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0518 + 1,
                    surface->node.physical + topBufferSize + bankOffsetBytes
                    );

                gcmSETFILLER(
                    reserve, memory
                    );

            gcmENDSTATEBATCH(
                reserve, memory
                );
       }
       else
       {
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0518, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0518) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0518, surface->node.physical );     gcmENDSTATEBATCH(reserve, memory);};
            /* Program AQPixelAddress register. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x050C, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x050C) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x050C, surface->node.physical );     gcmENDSTATEBATCH(reserve, memory);};
       }

        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x050D, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x050D) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x050D, surface->stride );     gcmENDSTATEBATCH(reserve, memory);};
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    if (Hardware->colorTargetDirty && (surface != gcvNULL))
    {
        /* Setup multi-sampling. */
        Hardware->msaaConfigDirty = gcvTRUE;
        Hardware->msaaModeDirty   = gcvTRUE;
    }

    /* Reset dirty flags. */
    Hardware->colorConfigDirty = gcvFALSE;
    Hardware->colorTargetDirty = gcvFALSE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushDepth(
    IN gcoHARDWARE Hardware
    )
{
    static const gctUINT32 xlateCompare[] =
    {
        /* gcvCOMPARE_NEVER */
        0x0,
        /* gcvCOMPARE_NOT_EQUAL */
        0x5,
        /* gcvCOMPARE_LESS */
        0x1,
        /* gcvCOMPARE_LESS_OR_EQUAL */
        0x3,
        /* gcvCOMPARE_EQUAL */
        0x2,
        /* gcvCOMPARE_GREATER */
        0x4,
        /* gcvCOMPARE_GREATER_OR_EQUAL */
        0x6,
        /* gcvCOMPARE_ALWAYS */
        0x7,
    };

    static const gctUINT32 xlateMode[] =
    {
        /* gcvDEPTH_NONE */
        0x0,
        /* gcvDEPTH_Z */
        0x1,
        /* gcvDEPTH_W */
        0x2,
    };

    gceSTATUS status;
    gcsSURF_INFO_PTR surface;

    gctUINT depthBatchAddress;
    gctUINT depthBatchCount;
    gctBOOL depthBatchLoad;
    gctBOOL depthSplitLoad;
    gctBOOL needFiller;

    gctUINT hzBatchCount = 0;
    gctUINT32 hzControl = 0;
    gctBOOL hzSupported;

    gctUINT32 samples = 0;
    gctUINT32 cacheMode;
    gctBOOL flushNeeded = gcvFALSE;
    gctBOOL stallNeeded = gcvFALSE;

    gcuFLOAT_UINT32 depthNear, depthFar;
    gcuFLOAT_UINT32 depthNormalize;
    gcuFLOAT_UINT32 zOffset, zScale;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Get a shortcut to the surface. */
    surface = Hardware->depthStates.surface;

    /* Test if configuration is dirty. */
    if (Hardware->depthConfigDirty)
    {
        gctBOOL early   = (Hardware->depthStates.early
                           && !Hardware->disableAllEarlyDepth
                           );
        gctBOOL peDepth = (Hardware->depthStates.write
                           || !early
                           );

        /* Switching from PE Depth ON to PE Depth Off? */
        if (!peDepth && Hardware->previousPEDepth)
        {
            /* Need flush and stall. */
            stallNeeded = gcvTRUE;
        }
    }

    if (Hardware->depthTargetDirty)
    {
        gcmASSERT(Hardware->depthConfigDirty);

        /* Configuration is always set when the surface is changed. */
        depthBatchAddress = 0x0500;

        if (surface == gcvNULL)
        {
            /* Define hierarchical Z state load parameters. */
            hzBatchCount = 1;
            hzControl = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

            /* Define depth state load parameters. */
            if (Hardware->depthRangeDirty)
            {
                depthBatchCount = 4;
                depthBatchLoad  = gcvFALSE;
                depthSplitLoad  = gcvFALSE;
                needFiller      = gcvTRUE;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Depth states. */
                    = gcmALIGN((1 + 4) * gcmSIZEOF(gctUINT32), 8)

                    /* Hierarchical Z states. */
                    + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                    /* Viewport states. */
                    + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
            }
            else
            {
                depthBatchCount = 1;
                depthBatchLoad  = gcvFALSE;
                depthSplitLoad  = gcvFALSE;
                needFiller      = gcvFALSE;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Depth states. */
                    = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                    /* Hierarchical Z states. */
                    + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            }

            /* No stencil. */
            Hardware->stencilEnabled = gcvFALSE;
            Hardware->stencilDirty   = gcvTRUE;
        }
        else
        {
            /* Define depth state load parameters. */
            if (Hardware->depthRangeDirty)
            {
                if (Hardware->pixelPipes > 1
                 || Hardware->renderTargets > 1)
                {
                    depthBatchCount = 4;
                    depthBatchLoad  = gcvFALSE;
                    depthSplitLoad  = gcvTRUE;
                    needFiller      = gcvTRUE;

                    /* Set the initial size. */
                    reserveSize

                        /* Depth states. */
                        = gcmALIGN((1 + 4) * gcmSIZEOF(gctUINT32), 8)

                        /* Stride state. */
                        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                        /* Viewport states. */
                        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;

                    if (Hardware->pixelPipes > 1)
                    {
                        /* Depth address states. */
                        reserveSize += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
                    }
                    else
                    {
                        /* Depth address state. */
                        reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
                    }
                }
                else
                {
                    depthBatchCount = 6;
                    depthBatchLoad  = gcvTRUE;
                    depthSplitLoad  = gcvFALSE;
                    needFiller      = gcvTRUE;

                    /* Set the initial size. */
                    reserveSize

                        /* Depth states. */
                        = gcmALIGN((1 + 6) * gcmSIZEOF(gctUINT32), 8)

                        /* Viewport states. */
                        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
                }
            }
            else
            {
                depthBatchCount = 1;
                depthBatchLoad  = gcvFALSE;
                depthSplitLoad  = gcvTRUE;
                needFiller      = gcvFALSE;

                /* Set the initial size. */
                if (Hardware->pixelPipes > 1)
                {
                    reserveSize

                        /* Configuration state. */
                        = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                        /* Depth address states. */
                        + gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

                        /* Stride state. */
                        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
                }
                else
                {
                    reserveSize

                        /* Configuration state. */
                        = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                        /* Surface states. */
                        + gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
                }
            }

            /* Determine whether hierarchical Z is supported. */
            hzSupported = (surface->hzNode.size > 0);

            /* Dispatch on depth buffer format. */
            switch (surface->format)
            {
            case gcvSURF_D16:
                /* 16-bit depth buffer. */
                Hardware->depthStates.config = ((((gctUINT32) (Hardware->depthStates.config)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

                if (hzSupported && !Hardware->disableAllEarlyDepth)
                {
                    hzBatchCount = 2;

                    gcmONERROR(gcoHARDWARE_HzClearValueControl(surface->format,
                                                               0,
                                                               gcvNULL,
                                                               &hzControl));

                    /* Add hierarchical Z states. */
                    reserveSize += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
                }
                else
                {
                    hzBatchCount = 1;

                    hzControl = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

                    /* Add hierarchical Z states. */
                    reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
                }

                /* Setup for maximum depth value. */
                Hardware->maxDepth = 0xFFFF;

                /* No stencil. */
                Hardware->stencilEnabled = gcvFALSE;
                Hardware->stencilDirty   = gcvTRUE;
                break;

            case gcvSURF_D24S8:
            case gcvSURF_D24X8:
                /* 24-bit depth buffer. */
                Hardware->depthStates.config = ((((gctUINT32) (Hardware->depthStates.config)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x1  & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

                if (hzSupported && !Hardware->disableAllEarlyDepth)
                {
                    hzBatchCount = 2;

                    gcmONERROR(gcoHARDWARE_HzClearValueControl(surface->format,
                                                               0,
                                                               gcvNULL,
                                                               &hzControl));

                    /* Add hierarchical Z states. */
                    reserveSize += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
                }
                else
                {
                    hzBatchCount = 1;

                    hzControl = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)));

                    /* Add hierarchical Z states. */
                    reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
                }

                /* Setup for maximum depth value. */
                Hardware->maxDepth = 0xFFFFFF;

                /* Stencil. */
                Hardware->stencilEnabled = (surface->format == gcvSURF_D24S8);
                Hardware->stencilDirty   = gcvTRUE;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Compute the numnber of samples. */
            samples = surface->samples.x * surface->samples.y;

            /* Determine the cache mode (equation is taken from RTL). */
            cacheMode = (samples == 4) && !surface->is16Bit;

            /* We need to flush the cache if the cache mode is changing. */
            if (cacheMode != Hardware->depthStates.cacheMode)
            {
                /* Flush the cache. */
                flushNeeded = !Hardware->flushedDepth;

                /* Mark depth cache as flushed. */
                Hardware->flushedDepth = gcvTRUE;

                /* Update the cache mode. */
                Hardware->depthStates.cacheMode = cacheMode;
            }
        }
    }

    /* Depth surface is not dirty. */
    else
    {
        if (Hardware->depthConfigDirty)
        {
            /* The batch starts with the configuration. */
            depthBatchAddress = 0x0500;

            /* Define depth state load parameters. */
            if (Hardware->depthRangeDirty)
            {
                depthBatchCount = 4;
                depthBatchLoad  = gcvFALSE;
                depthSplitLoad  = gcvFALSE;
                needFiller      = gcvTRUE;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Depth states. */
                    = gcmALIGN((1 + 4) * gcmSIZEOF(gctUINT32), 8)

                    /* Viewport states. */
                    + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
            }
            else
            {
                depthBatchCount = 1;
                depthBatchLoad  = gcvFALSE;
                depthSplitLoad  = gcvFALSE;
                needFiller      = gcvFALSE;

                /* Determine the size of the buffer to reserve. */
                reserveSize = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            }
        }
        else
        {
            /* Define depth state load parameters. */
            if (Hardware->depthRangeDirty)
            {
                depthBatchAddress = 0x0501;
                depthBatchCount = 3;
                depthBatchLoad  = gcvFALSE;
                depthSplitLoad  = gcvFALSE;
                needFiller      = gcvFALSE;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Depth range states. */
                    = gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8)

                    /* Viewport states. */
                    + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
            }
            else
            {
                /* Nothing to be done. */
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
        }

        /* Define hierarchical Z state load parameters. */
        hzBatchCount = 0;
    }

    /* Add the flush state. */
    if (flushNeeded || stallNeeded)
    {
        reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
    }

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    if (flushNeeded || stallNeeded)
    {
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};
    }

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, depthBatchAddress, depthBatchCount );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (depthBatchCount ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (depthBatchAddress) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        if (Hardware->depthConfigDirty)
        {
			gceCOMPARE compare ;
            gctBOOL early   = (Hardware->depthStates.early
                               && !Hardware->disableAllEarlyDepth
                               );
            gctBOOL peDepth = (Hardware->depthStates.write
                               || Hardware->stencilStates.mode
                               || !early
                               );

            /* Save PE Depth state. */
            Hardware->previousPEDepth = peDepth;


			compare = Hardware->depthStates.compare;
            if (surface == gcvNULL)
            {
                compare = gcvCOMPARE_ALWAYS;
            }

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0500,
                  Hardware->depthStates.config
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (xlateMode[Hardware->depthStates.mode]) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))) | (((gctUINT32) ((gctUINT32) (Hardware->depthStates.only) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (early) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (Hardware->depthStates.write) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (xlateCompare[compare]) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) ((gctUINT32) (!peDepth) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)))
                );
        }

        if (Hardware->depthRangeDirty)
        {
            if (Hardware->depthStates.mode == gcvDEPTH_W)
            {
                depthNear.f = Hardware->depthStates.near;
                depthFar.f  = Hardware->depthStates.far;

                depthNormalize.f = (depthFar.u != depthNear.u)
                    ? gcoMATH_Divide(
                        gcoMATH_UInt2Float(Hardware->maxDepth),
                        gcoMATH_Add(depthFar.f, -depthNear.f)
                        )
                    : gcoMATH_UInt2Float(Hardware->maxDepth);
            }
            else
            {
                depthNear.f = 0.0f;
                depthFar.f  = 1.0f;

                depthNormalize.f = gcoMATH_UInt2Float(Hardware->maxDepth);
            }

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0501,
                depthNear.u
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0502,
                depthFar.u
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0503,
                depthNormalize.u
                );
        }

        if (depthBatchLoad)
        {
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0504,
                surface->node.physical
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0505,
                surface->stride
                );
        }

        if (needFiller)
        {
            gcmSETFILLER(
                reserve, memory
                );
        }

    gcmENDSTATEBATCH(
        reserve, memory
        );

    if (depthSplitLoad)
    {
        if (Hardware->pixelPipes > 1
         || Hardware->renderTargets > 1)
        {
            if (Hardware->pixelPipes > 1)
            {
                gctUINT32 bankOffsetBytes = 0;
                gctINT topBufferSize = gcmALIGN(surface->alignedHeight / 2,
                                                surface->superTiled ? 64 : 4)
                                     * surface->stride;

                /* Extra pages needed to offset sub-buffers to different banks. */
#if gcdENABLE_BANK_ALIGNMENT
                gcmONERROR(gcoSURF_GetBankOffsetBytes(
                    gcvNULL,
                    surface->type,
                    topBufferSize,
                    &bankOffsetBytes
                    ));

                /* If surface doesn't have enough padding, then don't offset it. */
                if (surface->size <
                    ((surface->alignedHeight * surface->stride) + bankOffsetBytes))
                {
                    bankOffsetBytes = 0;
                }
#endif

                gcmASSERT(Hardware->pixelPipes == 2);

                /* Program depth addresses. */
                {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0520, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0520) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0520,
                        surface->node.physical
                        );

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0520 + 1,
                        surface->node.physical + topBufferSize + bankOffsetBytes
                        );

                    gcmSETFILLER(
                        reserve, memory
                        );

                gcmENDSTATEBATCH(
                    reserve, memory
                    );
            }
            else /* renderTargets > 1. */
            {
                /* Program depth addresses. */
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0520, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0520) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0520, surface->node.physical );     gcmENDSTATEBATCH(reserve, memory);};

                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0504, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0504) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0504, surface->node.physical );     gcmENDSTATEBATCH(reserve, memory);};
            }

            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0505, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0505) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0505, surface->stride );     gcmENDSTATEBATCH(reserve, memory);};
        }
        else
        {
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0504, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0504) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0504,
                    surface->node.physical
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0505,
                    surface->stride
                    );

                gcmSETFILLER(
                    reserve, memory
                    );

            gcmENDSTATEBATCH(
                reserve, memory
                );
        }
    }

    if (hzBatchCount != 0)
    {
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0515, hzBatchCount );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (hzBatchCount ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0515) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0515,
                hzControl
                );

            if (hzBatchCount == 2)
            {
                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0516,
                    surface->hzNode.physical
                    );

                gcmSETFILLER(
                    reserve, memory
                    );
            }

        gcmENDSTATEBATCH(
            reserve, memory
            );
    }

    if (Hardware->depthRangeDirty)
    {
        zOffset.f = Hardware->depthStates.near;
        zScale.f  = gcoMATH_Add(
            Hardware->depthStates.far, -Hardware->depthStates.near
            );

        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0282, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0282) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0282, zScale.u );     gcmENDSTATEBATCH(reserve, memory);};

        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0285, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0285) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0285, zOffset.u );     gcmENDSTATEBATCH(reserve, memory);};
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    if (stallNeeded)
    {
        /* Send semaphore-stall from RA to PE. */
        gcmONERROR(gcoHARDWARE_Semaphore(gcvWHERE_RASTER,
                                         gcvWHERE_PIXEL,
                                         gcvHOW_SEMAPHORE));
    }

    if (Hardware->depthTargetDirty
    && (Hardware->colorStates.surface == gcvNULL)
    && (Hardware->depthStates.surface != gcvNULL))
    {
        Hardware->vaa             = gcvVAA_NONE;
        Hardware->samples         = Hardware->depthStates.surface->samples;
        Hardware->msaaConfigDirty = gcvTRUE;
        Hardware->msaaModeDirty   = gcvTRUE;
    }

    /* Reset dirty flags. */
    Hardware->depthConfigDirty = gcvFALSE;
    Hardware->depthRangeDirty  = gcvFALSE;
    Hardware->depthTargetDirty = gcvFALSE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushSampling(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gctUINT samples;
    gctBOOL programTables = gcvFALSE;
    gctUINT32 msaaMode = 0;
    gctUINT32 vaaMode = 0;
    gctUINT msaaEnable;
    gctUINT32_PTR sampleCoords = gcvNULL;
    gcsCENTROIDS_PTR centroids = gcvNULL;
    gctUINT32 jitterIndex = 0;
    gctINT i, tables = 0;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Configuration is always loaded. */
    gcmASSERT(Hardware->msaaConfigDirty);
    reserveSize = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

    if (Hardware->vaa == gcvVAA_NONE)
    {
        /* Compute centroids. */
        if (Hardware->centroidsDirty)
        {
            gcmONERROR(gcoHARDWARE_ComputeCentroids(
                Hardware, 1, &Hardware->sampleCoords2, &Hardware->centroids2
                ));

            gcmONERROR(gcoHARDWARE_ComputeCentroids(
                Hardware, 3, Hardware->sampleCoords4, Hardware->centroids4
                ));

            Hardware->centroidsDirty = gcvFALSE;
        }

        samples = Hardware->samples.x * Hardware->samples.y;

        switch (samples)
        {
        case 1:
            vaaMode  = 0x0;
            msaaMode = 0x0;
            Hardware->sampleEnable = 0x0;
            break;

        case 2:
            vaaMode  = 0x0;
            msaaMode = 0x1;
            Hardware->sampleEnable = 0x3;

            if (Hardware->msaaModeDirty)
            {
                programTables = gcvTRUE;
                sampleCoords  = &Hardware->sampleCoords2;
                centroids     = &Hardware->centroids2;
                tables        = 1;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Jitter. */
                    += gcmALIGN((1 + 1)  * gcmSIZEOF(gctUINT32), 8)

                    /* Sample coordinates. */
                    +  gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                    /* Centroids. */
                    +  gcmALIGN((1 + 4) * gcmSIZEOF(gctUINT32), 8);
            }
            break;

        case 4:
            vaaMode  = 0x0;
            msaaMode = 0x2;
            Hardware->sampleEnable = 0xF;

            if (Hardware->msaaModeDirty)
            {
                programTables = gcvTRUE;
                sampleCoords  = Hardware->sampleCoords4;
                centroids     = Hardware->centroids4;
                jitterIndex   = Hardware->jitterIndex;
                tables        = 3;

                /* Determine the size of the buffer to reserve. */
                reserveSize

                    /* Jitter. */
                    += gcmALIGN((1 + 1)  * gcmSIZEOF(gctUINT32), 8)

                    /* Sample coordinates. */
                    +  gcmALIGN((1 + 3)  * gcmSIZEOF(gctUINT32), 8)

                    /* Centroids. */
                    +  gcmALIGN((1 + 12) * gcmSIZEOF(gctUINT32), 8);
            }
            break;

        default:
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }
    else
    {
        vaaMode = ((Hardware->chipModel < gcv600) || (Hardware->vaa == gcvVAA_COVERAGE_8))
            ? 0x2
            : 0x1;

        msaaMode = 0x0;

        Hardware->sampleEnable = 0x1;

        if (Hardware->msaaModeDirty)
        {
            programTables = gcvTRUE;

            sampleCoords = (vaaMode == 0x2)
                ? &Hardware->vaa8SampleCoords
                : &Hardware->vaa16SampleCoords;

            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* Jitter. */
                += gcmALIGN((1 + 1)  * gcmSIZEOF(gctUINT32), 8)

                /* Sample coordinates. */
                +  gcmALIGN((1 + 1)  * gcmSIZEOF(gctUINT32), 8);
        }
    }

    /* Determine enable value. */
    msaaEnable = Hardware->sampleMask & Hardware->sampleEnable;

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Set multi-sample mode. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E06, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E06) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0E06, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (msaaMode) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (vaaMode) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (msaaEnable) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) );     gcmENDSTATEBATCH(reserve, memory);};

    if (programTables)
    {
        /* Set multi-sample jitter. */
        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0381, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0381) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0381, jitterIndex );     gcmENDSTATEBATCH(reserve, memory);};

        if (Hardware->vaa == gcvVAA_NONE)
        {
            /* Program sample coordinates. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0384, tables );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (tables ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0384) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                for (i = 0; i < tables; i += 1)
                {
                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0384 + i,
                        sampleCoords[i]
                        );
                }

            gcmENDSTATEBATCH(
                reserve, memory
                );

            /* Program the centroid table. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0390, tables * 4 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (tables * 4 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0390) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                for (i = 0; i < tables; i += 1)
                {
                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0390 + i * 4 + 0,
                        centroids[i].value[0]
                        );

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0390 + i * 4 + 1,
                        centroids[i].value[1]
                        );

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0390 + i * 4 + 2,
                        centroids[i].value[2]
                        );

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0390 + i * 4 + 3,
                        centroids[i].value[3]
                        );
                }

                gcmSETFILLER(
                    reserve, memory
                    );

            gcmENDSTATEBATCH(
                reserve, memory
                );
        }
        else
        {
            /* Program coverage. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0384, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0384) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0384, sampleCoords[0] );     gcmENDSTATEBATCH(reserve, memory);};
        }
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Reset dirty flags. */
    Hardware->msaaConfigDirty = gcvFALSE;
    Hardware->msaaModeDirty   = gcvFALSE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushPA(
    IN gcoHARDWARE Hardware
    )
{
    static const gctUINT32 xlateCulling[] =
    {
        /* gcvCULL_NONE */
        0x0,
        /* gcvCULL_CCW*/
        0x2,
        /* gcvCULL_CW*/
        0x1
    };

    static const gctUINT32 xlateFill[] =
    {
        /* gcvFILL_POINT */
        0x0,
        /* gcvFILL_WIRE_FRAME */
        0x1,
        /* gcvFILL_SOLID */
        0x2
    };

    static const gctUINT32 xlateShade[] =
    {
        /* gcvSHADING_SMOOTH */
        0x1,
        /* gcvSHADING_FLAT_D3D */
        0x0,
        /* gcvSHADING_FLAT_OPENGL */
        0x0
    };

    gceSTATUS status;
    gctUINT batchAddress;
    gctUINT batchCount;
    gctBOOL needFiller;

    gcuFLOAT_UINT32 width;
    gcuFLOAT_UINT32 adjustSub;
    gcuFLOAT_UINT32 adjustAdd;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    if (Hardware->paLineDirty)
    {
        if (Hardware->paConfigDirty)
        {
            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* Line scale. */
                = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                /* PA states. */
                + gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8);

            batchAddress = 0x028D;
            batchCount   = 3;
            needFiller   = gcvFALSE;
        }
        else
        {
            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* Line scale. */
                = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

                /* PA states. */
                + gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);

            batchAddress = 0x028E;
            batchCount   = 2;
            needFiller   = gcvTRUE;
        }
    }
    else
    {
        if (Hardware->paConfigDirty)
        {
            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* PA states. */
                = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

            batchAddress = 0x028D;
            batchCount   = 1;
            needFiller   = gcvFALSE;
        }
        else
        {
            /* Nothing to be done. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    if (Hardware->paLineDirty)
    {
        width.f = Hardware->paStates.aaLineWidth / 2.0f;

        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0286, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0286) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0286, width.u );     gcmENDSTATEBATCH(reserve, memory);};
    }

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, batchAddress, batchCount );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (batchCount ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (batchAddress) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        if (Hardware->paConfigDirty)
        {
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x028D,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))) | (((gctUINT32) ((gctUINT32) (Hardware->paStates.pointSize) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->paStates.pointSprite) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (xlateCulling[Hardware->paStates.culling]) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (xlateFill[Hardware->paStates.fillMode]) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (xlateShade[Hardware->paStates.shading]) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (Hardware->paStates.aaLine) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (Hardware->paStates.aaLineTexSlot) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
                );

            Hardware->paConfigDirty = gcvFALSE;
        }

        if (Hardware->paLineDirty)
        {
            adjustSub.f = Hardware->paStates.aaLineWidth / 2.0f;
            adjustAdd.f = -adjustSub.f + Hardware->paStates.aaLineWidth;

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x028E,
                adjustSub.u
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x028F,
                adjustAdd.u
                );

            Hardware->paLineDirty = gcvFALSE;
        }

        if (needFiller)
        {
            gcmSETFILLER(
                reserve, memory
                );
        }

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushShaders(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE PrimitiveType
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL pointList, bypass;
    gctUINT32 msaa, scale;
    gctUINT32 output, bufferCount;
    gctUINT32 i, address, count, bytes;
    gctUINT32_PTR buffer;
    gctSIZE_T offset;
    gcsHINT_PTR hints;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x Type=%d", Hardware, PrimitiveType);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Special case for shared instruction memory. */
    if ((Hardware->instructionCount > 256)
        && (Hardware->instructionCount <= 1024)
        )
    {
        /* Semaphore/stall the entire pipe. */
        gcmONERROR(gcoHARDWARE_Semaphore(gcvWHERE_COMMAND,
                                         gcvWHERE_PIXEL,
                                         gcvHOW_SEMAPHORE_STALL));
    }

    /* Init the state buffer. */
    bufferCount = Hardware->shaderStates.stateBufferSize;
    buffer      = Hardware->shaderStates.stateBuffer;

    /* Determine the size of the buffer to reserve. */
    reserveSize

        /* Pixel shader control. */
        = gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8)

        /* Single state loads. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 4;

    /* Loop through all bytes. */
    for (offset = 0; offset < bufferCount;)
    {
        /* Make sure this is a load state command. */
        if (!((((gctUINT32) (*buffer)) >> (0 ? 31:27) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1)))))) == (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))))
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        /* Get the aligned count of (command + data). */
        count = 1 + ((((((gctUINT32) (*buffer)) >> (0 ? 25:16)) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1)))))) ) | 1);

        /* Compute the number of bytes. */
        bytes = count * 4;

        /* Adjust buffer address and byte count. */
        buffer += count;
        offset += bytes;

        /* Update the reserve count. */
        reserveSize += bytes;
    }

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Init the state buffer. */
    buffer = Hardware->shaderStates.stateBuffer;

    /* Loop through all bytes. */
    for (offset = 0; offset < bufferCount;)
    {
        /* Extract fields. */
        address = (((((gctUINT32) (*buffer)) >> (0 ? 15:0)) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1)))))) );
        count   = (((((gctUINT32) (*buffer)) >> (0 ? 25:16)) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1)))))) );

        /* Skip the command. */
        buffer += 1;

        /* Load the states. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, address, count);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (count) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            for (i = 0; i < count; i += 1)
            {
                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, address + i,
                    *buffer++
                    );
            }

            if ((count & 1) == 0)
            {
                gcmSETFILLER(reserve, memory);

                buffer += 1;
                offset += 4 * (1 + count + 1);
            }
            else
            {
                offset += 4 * (1 + count);
            }

        gcmENDSTATEBATCH(
            reserve, memory
            );
    }

    /* Determine MSAA mode. */
    msaa = ((Hardware->samples.x * Hardware->samples.y > 1)
         && (Hardware->sampleMask != 0)
         && !Hardware->vaa
         )
         ? 1
         : 0;

    /* Determine bypass mode. */
    bypass = (msaa == 0) && Hardware->depthStates.only;

    /* Shortcut to hints. */
    hints = Hardware->shaderStates.hints;

    /* Point list primitive? */
    pointList = (PrimitiveType == gcvPRIMITIVE_POINT_LIST);

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0402, 3 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (3 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0402) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        /* Set input count. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0402,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (hints->fsInputCount + msaa) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
            );

        /* Set temporary register control. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0403,
            hints->fsMaxTemp + msaa
            );

        /* Set bypass for depth-only modes. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0404,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (bypass) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (gcvTRUE) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))
            );

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Set element count. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x028C, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x028C) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x028C, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (bypass ? 0 : hints->elementCount) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) );     gcmENDSTATEBATCH(reserve, memory);};

    /* Set component count. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E07, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E07) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0E07, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:0) - (0 ? 6:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:0) - (0 ? 6:0) + 1))))))) << (0 ? 6:0))) | (((gctUINT32) ((gctUINT32) (bypass ? 0 : hints->componentCount) & ((gctUINT32) ((((1 ? 6:0) - (0 ? 6:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:0) - (0 ? 6:0) + 1))))))) << (0 ? 6:0))) );     gcmENDSTATEBATCH(reserve, memory);};

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
    if (Hardware->chipModel < gcv1000)
    {
        gcmASSERT(hints->componentCount == (hints->elementCount * 4));
    }
#endif

    /* Scale to 50%. */
    scale
        = (Hardware->chipModel == gcv400)
            ?  hints->balanceMax
            : (hints->balanceMin + hints->balanceMax) / 2;

    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x020C, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x020C) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x020C, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (scale) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (hints->balanceMin) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) );     gcmENDSTATEBATCH(reserve, memory);};

    if (bypass)
    {
        /* In bypass mode, we only need the osition output and the point
        ** size if present and the primitive is a point list. */
        output
            = 1 + ((hints->vsHasPointSize && pointList) ? 1 : 0);
    }
    else
    {
        /* If the Vertex Shader generates a point size, and the primitive is
        ** not a point, we don't need to send it down. */
        output
            = (hints->vsHasPointSize && !pointList)
                ? hints->vsOutputCount - 1
                : hints->vsOutputCount;
    }

    /* Set output count. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0201, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0201) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0201, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (output) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) );     gcmENDSTATEBATCH(reserve, memory);};

    /* Reset dirty. */
    Hardware->shaderDirty = gcvFALSE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_HzClearValueControl(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ZClearValue,
    OUT gctUINT32 * HzClearValue OPTIONAL,
    OUT gctUINT32 * HzControl OPTIONAL
    )
{
    gceSTATUS status;
    gctUINT32 clearValue = 0, control = 0;

    gcmHEADER_ARG("Format=%d ZClearValue=0x%x", Format, ZClearValue);

    /* Dispatch on format. */
    switch (Format)
    {
    case gcvSURF_D16:
        /* 16-bit Z. */
        clearValue = ZClearValue;
        control    = 0x5;
        break;

    case gcvSURF_D24X8:
    case gcvSURF_D24S8:
        /* 24-bit Z. */
        clearValue = ZClearValue >> 8;
        control    = 0x8;
        break;

    default:
        /* Not supported. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (HzClearValue != gcvNULL)
    {
        /* Return clear value. */
        *HzClearValue = clearValue;
    }

    if (HzControl != gcvNULL)
    {
        /* Return control. */
        *HzControl = control;
    }

    /* Success. */
    gcmFOOTER_ARG("*HzClearValue=0x%x *HzControl=0x%x",
                  gcmOPT_VALUE(HzClearValue), gcmOPT_VALUE(HzControl));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif /* VIVANTE_NO_3D */


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




/*
 * gco3D object for user HAL layers.
 */

#include "gc_hal_user_precomp.h"

#ifndef VIVANTE_NO_3D

#if gcdNULL_DRIVER < 2

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_3D

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

/* gco3D object. */
struct _gco3D
{
    /* Object. */
    gcsOBJECT                   object;

    /* Render target. */
    gcoSURF                     target;
    gctPOINTER                  targetMemory;

    /* Depth buffer. */
    gcoSURF                     depth;
    gctPOINTER                  depthMemory;

    /* Clear color. */
    gctBOOL                     clearColorDirty;
    gceVALUE_TYPE               clearColorType;
    gcuVALUE                    clearColorRed;
    gcuVALUE                    clearColorGreen;
    gcuVALUE                    clearColorBlue;
    gcuVALUE                    clearColorAlpha;

    /* Clear depth value. */
    gctBOOL                     clearDepthDirty;
    gceVALUE_TYPE               clearDepthType;
    gcuVALUE                    clearDepth;

    /* Clear stencil value. */
    gctBOOL                     clearStencilDirty;
    gctUINT                     clearStencil;

    /* Converted clear values. */
    gctUINT32                   hwClearColor;
    gceSURF_FORMAT              hwClearColorFormat;
    gctUINT32                   hwClearVAA;
    gctUINT8                    hwClearColorMask;
    gctUINT32                   hwClearDepth;
    gctUINT32                   hwClearHzDepth;
    gctUINT8                    hwClearDepthMask;
    gctUINT8                    hwClearStencilMask;
    gceSURF_FORMAT              hwClearDepthFormat;

    /* Buffer write enable mask. */
    gctUINT8                    colorEnableMask;
    gctBOOL                     depthEnableMask;
    gctUINT8                    stencilEnableMask;

    /* Depth mode. */
    gceDEPTH_MODE               depthMode;

    /* API type. */
    gceAPI                      apiType;
};


/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

/*******************************************************************************
**
**  _ConvertValue
**
**  Convert a value to a 32-bit color or depth value.
**
**  INPUT:
**
**      gceVALUE_TYPE ValueType
**          Type of value.
**
**      gcuVALUE Value
**          Value data.
**
**      gctUINT Bits
**          Number of bits used in output.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gctUINT32
**          Converted or casted value.
*/
static gctUINT32
_ConvertValue(
    IN gceVALUE_TYPE ValueType,
    IN gcuVALUE Value,
    IN gctUINT Bits
    )
{
    /* Setup maximum value. */
    gctUINT maxValue = (Bits == 32) ? ~0 : ((1 << Bits) - 1);
    gcmASSERT(Bits <= 32);

    /* Dispatch on value type. */
    switch (ValueType)
    {
    case gcvVALUE_UINT:
        /* Convert integer into color value. */
        return (Bits > 8) ? 0 : (Value.uintValue >> (8 - Bits));

    case gcvVALUE_FIXED:
        /* Convert fixed point (0.0 - 1.0) into color value. */
        return (gctUINT32) (((gctUINT64) maxValue * Value.fixedValue) >> 16);

    case gcvVALUE_FLOAT:
        /* Convert floating point (0.0 - 1.0) into color value. */
        return gcoMATH_Float2UInt(gcoMATH_Multiply(gcoMATH_UInt2Float(maxValue),
                                                   Value.floatValue));

    default:
        return 0;
    }
}

/******************************************************************************\
********************************* gco3D API Code ********************************
\******************************************************************************/

/*******************************************************************************
**
**  gco3D_Construct
**
**  Contruct a new gco3D object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gco3D * Engine
**          Pointer to a variable receiving the gco3D object pointer.
*/
gceSTATUS
gco3D_Construct(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    )
{
    gco3D engine = gcvNULL;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    /* Allocate the gco3D object. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _gco3D),
                       &pointer));

    engine = pointer;

    /* Zero out entire object. */
    gcmVERIFY_OK(
        gcoOS_ZeroMemory(engine, gcmSIZEOF(struct _gco3D)));

    /* Initialize the gco3D object. */
    engine->object.type = gcvOBJ_3D;

    /* Mark everything as dirty. */
    engine->clearColorDirty   = gcvTRUE;
    engine->clearDepthDirty   = gcvTRUE;
    engine->clearStencilDirty = gcvTRUE;

    /* Set default API type. */
    engine->apiType = gcvAPI_OPENGL;

    /* Initialize 3D hardware. */
    gcmONERROR(
        gcoHARDWARE_Initialize3D(gcvNULL));

    /* Return pointer to the gco3D object. */
    *Engine = engine;

    /* Success. */
    gcmFOOTER_ARG("*Engine=0x%x", *Engine);
    return gcvSTATUS_OK;

OnError:
    if (engine != gcvNULL)
    {
        /* Unroll. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, engine));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_Destroy
**
**  Destroy an gco3D object.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_Destroy(
    IN gco3D Engine
    )
{
    gcmHEADER_ARG("Engine=0x%x", Engine);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Mark the gco3D object as unknown. */
    Engine->object.type = gcvOBJ_UNKNOWN;

    /* Free the gco3D object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Engine));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetAPI
**
**  Set 3D API type.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gco3D * Engine
**          Pointer to a variable receiving the gco3D object pointer.
*/
gceSTATUS
gco3D_SetAPI(
    IN gco3D Engine,
    IN gceAPI ApiType
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x ApiType=%d", Engine, ApiType);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the API. */
    Engine->apiType = ApiType;

    /* Propagate to hardware. */
    status = gcoHARDWARE_SetAPI(ApiType);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTarget
**
**  Set the current render target used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object that becomes the new render target or
**          gcvNULL to delete the current render target.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTarget(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x", Engine, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Surface != gcvNULL)
    {
        gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    }

    /* Only process if different than current target. */
    if (Engine->target != Surface)
    {
        /* Verify if target is properly aligned. */
        if ((Surface != gcvNULL)
        &&  Surface->resolvable
        &&  (  (Surface->info.alignedWidth  & 15)
            || (Surface->info.alignedHeight & 3)
            )
        )
        {
            gcmONERROR(gcvSTATUS_NOT_ALIGNED);
        }

        /* Unset previous target if any. */
        if (Engine->target != gcvNULL)
        {
            /* Disable tile status. */
            gcmONERROR(
                gcoSURF_DisableTileStatus(Engine->target, gcvFALSE));

            /* Unlock the current render target. */
            gcmVERIFY_OK(
                gcoSURF_Unlock(Engine->target, Engine->targetMemory));

            /* Reset mapped memory pointer. */
            Engine->targetMemory = gcvNULL;
        }

        /* Set new target. */
        if (Surface == gcvNULL)
        {
            /* Reset current target. */
            Engine->target = gcvNULL;

            /* Invalidate target. */
            gcmONERROR(
                gcoHARDWARE_SetRenderTarget(gcvNULL));
        }
        else
        {
            gctPOINTER targetMemory[3] = {gcvNULL};

            /* Set new render target. */
            Engine->target = Surface;

            /* Lock the surface. */
            gcmONERROR(
                gcoSURF_Lock(Surface, gcvNULL, targetMemory));

            Engine->targetMemory = targetMemory[0];

            /* Program render target. */
            gcmONERROR(
                gcoHARDWARE_SetRenderTarget(&Surface->info));

            /* Enable tile status. */
            gcmONERROR(
                gcoSURF_EnableTileStatus(Surface));
        }
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
gco3D_UnsetTarget(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x", Engine, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Only unset surface if it is the current render target. */
    if (Engine->target == Surface)
    {
        /* Disable tile status. */
        gcmONERROR(
            gcoSURF_DisableTileStatus(Surface, gcvFALSE));

        /* Remove render target. */
        gcmONERROR(
            gcoHARDWARE_SetRenderTarget(gcvNULL));

        /* Unlock the current render target. */
        gcmONERROR(
            gcoSURF_Unlock(Surface, Engine->targetMemory));

        /* Reset mapped memory pointer. */
        Engine->targetMemory = gcvNULL;

        /* Reset render target. */
        Engine->target = gcvNULL;
    }

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
**  gco3D_SetDepth
**
**  Set the current depth buffer used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcoSURF Surface
**          Pointer to an gcoSURF object that becomes the new depth buffer or
**          gcvNULL to delete the current depth buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x", Engine, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Surface != gcvNULL)
    {
        gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);
    }

    /* Only process if different than current depth buffer. */
    if (Engine->depth != Surface)
    {
        /* Verify if depth buffer is properly aligned. */
        if ((Surface != gcvNULL)
        &&  Surface->resolvable
        &&  (  (Surface->info.alignedWidth  & 15)
            || (Surface->info.alignedHeight & 3)
            )
        )
        {
            gcmONERROR(gcvSTATUS_NOT_ALIGNED);
        }

        /* Unset previous depth buffer if any. */
        if (Engine->depth != gcvNULL)
        {
            /* Disable previous tile status. */
            gcmONERROR(
                gcoSURF_DisableTileStatus(Engine->depth, gcvFALSE));

            /* Unlock the current depth surface. */
            gcmONERROR(
                gcoSURF_Unlock(Engine->depth, Engine->depthMemory));

            /* Reset mapped memory pointer. */
            Engine->depthMemory = gcvNULL;
        }

        /* Set new depth buffer. */
        if (Surface == gcvNULL)
        {
            /* Reset current depth buffer. */
            Engine->depth = gcvNULL;

            /* Disable depth. */
            gcmONERROR(
                gcoHARDWARE_SetDepthBuffer(gcvNULL));
        }
        else
        {
            gctPOINTER depthMemory[3] = {gcvNULL};

            /* Set new depth buffer. */
            Engine->depth = Surface;

            /* Lock depth buffer. */
            gcmONERROR(
                gcoSURF_Lock(Surface,
                             gcvNULL,
                             depthMemory));
            Engine->depthMemory = depthMemory[0];

            /* Set depth buffer. */
            gcmONERROR(
                gcoHARDWARE_SetDepthBuffer(&Surface->info));

            /* Enable tile status. */
            gcmONERROR(
                gcoSURF_EnableTileStatus(Surface));
        }
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
gco3D_UnsetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x", Engine, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Only unset surface if it is the current depth buffer. */
    if (Engine->depth == Surface)
    {
        /* Disable tile status. */
        gcmONERROR(
            gcoSURF_DisableTileStatus(Surface, gcvFALSE));

        /* Unlock the current render target. */
        gcmONERROR(
            gcoSURF_Unlock(Surface, Engine->depthMemory));

        /* Reset mapped memory pointer. */
        Engine->depthMemory = gcvNULL;

        /* Reset depth buffer. */
        Engine->depth = gcvNULL;

        /* Disable depth. */
        gcmONERROR(
            gcoHARDWARE_SetDepthBuffer(gcvNULL));
    }

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
**  gco3D_SetViewport
**
**  Set the current viewport used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT32 Left
**          Left coordinate of viewport.
**
**      gctINT32 Top
**          Top coordinate of viewport.
**
**      gctINT32 Right
**          Right coordinate of viewport.
**
**      gctINT32 Bottom
**          Bottom coordinate of viewport.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetViewport(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Left=%d Top=%d Right=%d Bottom=%d",
                  Engine, Left, Top, Right, Bottom);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program viewport. */
    status = gcoHARDWARE_SetViewport(Left, Top, Right, Bottom);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetScissors
**
**  Set the current scissors used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT32 Left
**          Left coordinate of scissors.
**
**      gctINT32 Top
**          Top coordinate of scissors.
**
**      gctINT32 Right
**          Right coordinate of scissors.
**
**      gctINT32 Bottom
**          Bottom coordinate of scissors.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetScissors(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Left=%d Top=%d Right=%d Bottom=%d",
                  Engine, Left, Top, Right, Bottom);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program scissors. */
    status = gcoHARDWARE_SetScissors(Left, Top, Right, Bottom);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetClearColor
**
**  Set the current clear color used for future primitives.  The clear color is
**  specified in unsigned integers between 0 and 255.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Red
**          Clear color for red channel.
**
**      gctUINT8 Green
**          Clear color for green channel.
**
**      gctUINT8 Blue
**          Clear color for blue channel.
**
**      gctUINT8 Alpha
**          Clear color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearColor(
    IN gco3D Engine,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    gcmHEADER_ARG("Engine=0x%x Red=%u Green=%u Blue=%u Alpha=%u",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear color. */
    if ((Engine->clearColorType            != gcvVALUE_UINT)
    ||  (Engine->clearColorRed.uintValue   != Red)
    ||  (Engine->clearColorGreen.uintValue != Green)
    ||  (Engine->clearColorBlue.uintValue  != Blue)
    ||  (Engine->clearColorAlpha.uintValue != Alpha)
    )
    {
        /* Set the new clear colors.    Saturate between 0 and 255. */
        Engine->clearColorDirty           = gcvTRUE;
        Engine->clearColorType            = gcvVALUE_UINT;
        Engine->clearColorRed.uintValue   = Red;
        Engine->clearColorGreen.uintValue = Green;
        Engine->clearColorBlue.uintValue  = Blue;
        Engine->clearColorAlpha.uintValue = Alpha;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearColorX
**
**  Set the current clear color used for future primitives.  The clear color is
**  specified in fixed point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Red
**          Clear color for red channel.
**
**      gctFIXED_POINT Green
**          Clear color for green channel.
**
**      gctFIXED_POINT Blue
**          Clear color for blue channel.
**
**      gctFIXED_POINT Alpha
**          Clear color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear color. */
    if ((Engine->clearColorType             != gcvVALUE_FIXED)
    ||  (Engine->clearColorRed.fixedValue   != Red)
    ||  (Engine->clearColorGreen.fixedValue != Green)
    ||  (Engine->clearColorBlue.fixedValue  != Blue)
    ||  (Engine->clearColorAlpha.fixedValue != Alpha)
    )
    {
        /* Set the new clear color.  Saturate between 0.0 and 1.0. */
        Engine->clearColorDirty            = gcvTRUE;
        Engine->clearColorType             = gcvVALUE_FIXED;
        Engine->clearColorRed.fixedValue   = gcmCLAMP(Red,   0, gcvONE_X);
        Engine->clearColorGreen.fixedValue = gcmCLAMP(Green, 0, gcvONE_X);
        Engine->clearColorBlue.fixedValue  = gcmCLAMP(Blue,  0, gcvONE_X);
        Engine->clearColorAlpha.fixedValue = gcmCLAMP(Alpha, 0, gcvONE_X);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearColorF
**
**  Set the current clear color used for future primitives.  The clear color is
**  specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Red
**          Clear color for red channel.
**
**      gctFLOAT Green
**          Clear color for green channel.
**
**      gctFLOAT Blue
**          Clear color for blue channel.
**
**      gctFLOAT Alpha
**          Clear color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear color. */
    if ((Engine->clearColorType             != gcvVALUE_FLOAT)
    ||  (Engine->clearColorRed.floatValue   != Red)
    ||  (Engine->clearColorGreen.floatValue != Green)
    ||  (Engine->clearColorBlue.floatValue  != Blue)
    ||  (Engine->clearColorAlpha.floatValue != Alpha)
    )
    {
        /* Set the new clear color.  Saturate between 0.0 and 1.0. */
        Engine->clearColorDirty            = gcvTRUE;
        Engine->clearColorType             = gcvVALUE_FLOAT;
        Engine->clearColorRed.floatValue   = gcmCLAMP(Red,   0.0f, 1.0f);
        Engine->clearColorGreen.floatValue = gcmCLAMP(Green, 0.0f, 1.0f);
        Engine->clearColorBlue.floatValue  = gcmCLAMP(Blue,  0.0f, 1.0f);
        Engine->clearColorAlpha.floatValue = gcmCLAMP(Alpha, 0.0f, 1.0f);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearDepthX
**
**  Set the current clear depth value used for future primitives.  The clear
**  depth value is specified in fixed point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Depth
**          Clear depth value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearDepthX(
    IN gco3D Engine,
    IN gctFIXED_POINT Depth
    )
{
    gcmHEADER_ARG("Engine=0x%x Depth=%f", Engine, gcoMATH_Fixed2Float(Depth));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear depth value. */
    if ((Engine->clearDepthType        != gcvVALUE_FIXED)
    ||  (Engine->clearDepth.fixedValue != Depth)
    )
    {
        /* Set the new clear depth value.  Saturate between 0.0 and 1.0. */
        Engine->clearDepthDirty       = gcvTRUE;
        Engine->clearDepthType        = gcvVALUE_FIXED;
        Engine->clearDepth.fixedValue = gcmCLAMP(Depth, 0, gcvONE_X);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearDepthF
**
**  Set the current clear depth value used for future primitives.  The clear
**  depth value is specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Depth
**          Clear depth value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearDepthF(
    IN gco3D Engine,
    IN gctFLOAT Depth
    )
{
    gcmHEADER_ARG("Engine=0x%x Depth=%f", Engine, Depth);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear depth value. */
    if ((Engine->clearDepthType != gcvVALUE_FLOAT)
    ||  (Engine->clearDepth.floatValue != Depth)
    )
    {
        /* Set the new clear depth value.  Saturate between 0.0 and 1.0. */
        Engine->clearDepthDirty       = gcvTRUE;
        Engine->clearDepthType        = gcvVALUE_FLOAT;
        Engine->clearDepth.floatValue = gcmCLAMP(Depth, 0.0f, 1.0f);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetClearStencil
**
**  Set the current clear stencil value used for future primitives.  The clear
**  stencil value is specified in unsigned integer.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT32 Stencil
**          Clear stencil value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetClearStencil(
    IN gco3D Engine,
    IN gctUINT32 Stencil
    )
{
    gcmHEADER_ARG("Engine=0x%x Stencil=%u", Engine, Stencil);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Test for a different clear stencil value. */
    if (Engine->clearStencil != Stencil)
    {
        /* Set te new clear stencil value.  Mask with 0xFF. */
        Engine->clearStencilDirty = gcvTRUE;
        Engine->clearStencil      = Stencil & 0xFF;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gco3D_SetShading
**
**  Set the current shading technique used for future primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSHADING Shading
**          Shading technique to select.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetShading(
    IN gco3D Engine,
    IN gceSHADING Shading
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Shading=%d", Engine, Shading);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program shading. */
    status = gcoHARDWARE_SetShading(Shading);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ComputeClear(
    IN gco3D Engine,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Flags
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Format=%d Flags=0x%x", Engine, Format, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(Flags != 0);

    /* Test for clearing render target. */
    if (Flags & gcvCLEAR_COLOR)
    {
        /* Get color write enable mask bits. */
        gctUINT8 colorMask = ((Engine->colorEnableMask & 0x1) << 2) /* Red   */
                           |  (Engine->colorEnableMask & 0x2)       /* Green */
                           | ((Engine->colorEnableMask & 0x4) >> 2) /* Blue  */
                           |  (Engine->colorEnableMask & 0x8);      /* Alpha */

        /* Set the color mask. */
        Engine->hwClearColorMask = colorMask;

        /* Test for different clear color or render target format. */
        if (Engine->clearColorDirty
        ||  (Engine->hwClearColorFormat != Format)
        ||  ((Flags & gcvCLEAR_HAS_VAA) != Engine->hwClearVAA)
        )
        {
            /* Dispatch on render target format. */
            switch (Format)
            {
            case gcvSURF_X4R4G4B4: /* 12-bit RGB color without alpha channel. */
            case gcvSURF_A4R4G4B4: /* 12-bit RGB color with alpha channel. */
                Engine->hwClearColor
                    /* Convert red into 4-bit. */
                    = (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorRed, 4) << 8)
                    /* Convert green into 4-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorGreen, 4) << 4)
                    /* Convert blue into 4-bit. */
                    | _ConvertValue(Engine->clearColorType,
                                    Engine->clearColorBlue, 4)
                    /* Convert alpha into 4-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorAlpha, 4) << 12);

                /* Expand 16-bit color into 32-bit color. */
                Engine->hwClearColor |= Engine->hwClearColor << 16;

                break;

            case gcvSURF_X1R5G5B5: /* 15-bit RGB color without alpha channel. */
            case gcvSURF_A1R5G5B5: /* 15-bit RGB color with alpha channel. */
                Engine->hwClearColor
                    /* Convert red into 5-bit. */
                    = (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorRed, 5) << 10)
                    /* Convert green into 5-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorGreen, 5) << 5)
                    /* Convert blue into 5-bit. */
                    | _ConvertValue(Engine->clearColorType,
                                    Engine->clearColorBlue, 5)
                    /* Convert alpha into 1-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorAlpha, 1) << 15);

                /* Expand 16-bit color into 32-bit color. */
                Engine->hwClearColor |= Engine->hwClearColor << 16;

                break;

            case gcvSURF_R5G6B5: /* 16-bit RGB color without alpha channel. */
                Engine->hwClearColor
                    /* Convert red into 5-bit. */
                    = (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorRed, 5) << 11)
                    /* Convert green into 6-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorGreen, 6) << 5)
                    /* Convert blue into 5-bit. */
                    | _ConvertValue(Engine->clearColorType,
                                    Engine->clearColorBlue, 5);

                /* Expand 16-bit color into 32-bit color. */
                Engine->hwClearColor |= Engine->hwClearColor << 16;

                break;

            case gcvSURF_YUY2:
                {
                    gctUINT8 r, g, b;
                    gctUINT8 y, u, v;

                    /* Query YUY2 render target support. */
                    if (gcoHAL_IsFeatureAvailable(gcvNULL,
                                                  gcvFEATURE_YUY2_RENDER_TARGET)
                        != gcvSTATUS_TRUE)
                    {
                        /* No, reject. */
                        gcmFOOTER_NO();
                        return gcvSTATUS_INVALID_ARGUMENT;
                    }

                    /* Get 8-bit RGB values. */
                    r = (gctUINT8) _ConvertValue(Engine->clearColorType,
                                      Engine->clearColorRed, 8);

                    g = (gctUINT8) _ConvertValue(Engine->clearColorType,
                                      Engine->clearColorGreen, 8);

                    b = (gctUINT8) _ConvertValue(Engine->clearColorType,
                                      Engine->clearColorBlue, 8);

                    /* Convert to YUV. */
                    gcoHARDWARE_RGB2YUV(r, g, b, &y, &u, &v);

                    /* Set the clear value. */
                    Engine->hwClearColor =  ((gctUINT32) y)
                                         | (((gctUINT32) u) <<  8)
                                         | (((gctUINT32) y) << 16)
                                         | (((gctUINT32) v) << 24);
                }
                break;

            case gcvSURF_X8R8G8B8: /* 24-bit RGB without alpha channel. */
            case gcvSURF_A8R8G8B8: /* 24-bit RGB with alpha channel. */
                Engine->hwClearColor
                    /* Convert red to 8-bit. */
                    = (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorRed, 8) << 16)
                    /* Convert green to 8-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorGreen, 8) << 8)
                    /* Convert blue to 8-bit. */
                    | _ConvertValue(Engine->clearColorType,
                                    Engine->clearColorBlue, 8)
                    /* Convert alpha to 8-bit. */
                    | (_ConvertValue(Engine->clearColorType,
                                     Engine->clearColorAlpha, 8) << 24);

                /* Test for VAA. */
                if (Flags & gcvCLEAR_HAS_VAA)
                {
                    if (Format == gcvSURF_X8R8G8B8)
                    {
                        /* Convert to C4R4G4B4C4R4G4B4. */
                        Engine->hwClearColor
                            = ((Engine->hwClearColor & 0x00000F)      )
                            | ((Engine->hwClearColor & 0x0000F0) << 12)
                            | ((Engine->hwClearColor & 0x000F00) >>  4)
                            | ((Engine->hwClearColor & 0x00F000) <<  8)
                            | ((Engine->hwClearColor & 0x0F0000) >>  8)
                            | ((Engine->hwClearColor & 0xF00000) <<  4);
                    }
                }
                break;

            default:
                gcmTRACE(
                    gcvLEVEL_ERROR,
                    "%s(%d): Unknown format=%d",
                    __FUNCTION__, __LINE__, Format
                    );

                /* Invalid surface format. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Clear color has been converted. */
            Engine->clearColorDirty    = gcvFALSE;
            Engine->hwClearColorFormat = Format;
            Engine->hwClearVAA         = (Flags & gcvCLEAR_HAS_VAA);
        }
    }

    /* Test for clearing depth or stencil buffer. */
    if (Flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
    {
        /* Get color write enable mask bits */
        gctBOOL  depthMask   = Engine->depthEnableMask;
        gctUINT8 stencilMask = Engine->stencilEnableMask;

		switch (Format)
		{
		case gcvSURF_D16: /* 16-bit depth without stencil. */
			/* Write to all bytes for depth, no bytes for stencil. */
			Engine->hwClearDepthMask   = depthMask ? 0xF : 0x0;
			Engine->hwClearStencilMask = 0x0;
			break;

		case gcvSURF_D24S8: /* 24-bit depth with 8-bit stencil. */
			/* Write to upper 3 bytes for depth, lower byte for stencil. */
			Engine->hwClearDepthMask   = depthMask   ? 0xE : 0x0;
			Engine->hwClearStencilMask = stencilMask ? 0x1 : 0x0;
			break;

		case gcvSURF_D24X8: /* 24-bit depth with no stencil. */
			/* Write all bytes for depth. */
			Engine->hwClearDepthMask   = depthMask ? 0xF : 0x0;
			Engine->hwClearStencilMask = 0x0;
			break;

		default:
			/* Invalid depth buffer format. */
			break;
		}

        /* Test for different clear depth value or depth buffer format. */
        if (Engine->clearDepthDirty
        ||  Engine->clearStencilDirty
        ||  (Engine->hwClearDepthFormat != Format)
        )
        {
            /* Dispatch on depth format. */
            switch (Format)
            {
            case gcvSURF_D16: /* 16-bit depth without stencil. */
                /* Convert depth value to 16-bit. */
                Engine->hwClearDepth = _ConvertValue(Engine->clearDepthType,
                                                     Engine->clearDepth,
                                                     16);

                /* Expand 16-bit depth value into 32-bit. */
                Engine->hwClearDepth |= (Engine->hwClearDepth << 16);

                break;

            case gcvSURF_D24S8: /* 24-bit depth with 8-bit stencil. */
                /* Convert depth value to 24-bit. */
                Engine->hwClearDepth = _ConvertValue(Engine->clearDepthType,
                                                     Engine->clearDepth,
                                                     24) << 8;

                /* Combine with masked stencil value. */
                Engine->hwClearDepth |= Engine->clearStencil & 0xFF;
                break;

            case gcvSURF_D24X8: /* 24-bit depth with no stencil. */
                /* Convert depth value to 24-bit. */
                Engine->hwClearDepth = _ConvertValue(Engine->clearDepthType,
                                                     Engine->clearDepth,
                                                     24) << 8;
                break;

            default:
                /* Invalid depth buffer format. */
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Compuet HZ clear value. */
            gcmONERROR(gcoHARDWARE_HzClearValueControl(Format,
                                                       Engine->hwClearDepth,
                                                       &Engine->hwClearHzDepth,
                                                       gcvNULL));

            /* Clear depth value has been converted. */
            Engine->clearDepthDirty    = gcvFALSE;
            Engine->clearStencilDirty  = gcvFALSE;
            Engine->hwClearDepthFormat = Format;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_ClearRect
**
**  Perform a clear with the current clear colors, depth values, or stencil
**  values, based on the specified flags. Rectangle is specified in opengl style
**  with 0,0 as left bottom and
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT32 Address
**          Base address of surface to clear.
**
**      gctPOINTER Memory
**          Base address of surface to clear.
**
**      gctUINT32 Stride
**          Stride of surface to clear.
**
**      gceSURF_FORMAT Format
**          Format of surface to clear.
**
**      gctINT32 Left
**          Left corner of surface to clear.
**
**      gctINT32 Top
**          Top corner of surface to clear.
**
**      gctINT32 Right
**          Right of surface to clear.
**
**      gctINT32 Bottom
**          Bottom of surface to clear.
**
**      gctUINT32 Width
**          Width of surface to clear.
**
**      gctUINT32 Height
**          Height of surface to clear.
**
**      gctUINT32 Flags
**          Flags that specifiy the clear operations to perform.  Can be any
**          combination of:
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
gco3D_ClearRect(
    IN gco3D Engine,
    IN gctUINT32 Address,
    IN gctPOINTER Memory,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gctUINT32 Width,
    IN gctUINT32 Height,
    IN gctUINT32 Flags
    )
{
    gctUINT8 mask;
    gceSTATUS status;
    gctINT32 left, top, right, bottom;

    gcmHEADER_ARG("Engine=0x%x Address=0x%08x Memory=0x%x Stride=%u Format=%d "
                  "Left=%d Top=%d Right=%d Bottom=%d Width=%d Height=%d "
                  "Flags=0x%x",
                  Engine, Address, Memory, Stride, Format, Left, Top, Right,
                  Bottom, Width, Height, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(Flags != 0);

    /* Clamp the coordinates to surface dimensions. */
    left   = gcmMAX(Left,   0);
    top    = gcmMAX(Top,    0);
    right  = gcmMIN(Right,  (gctINT32) Width);
    bottom = gcmMIN(Bottom, (gctINT32) Height);

    /* Test for a valid rectangle. */
    if ((left < right) && (top < bottom))
    {
        /* Compute clear values. */
        gcmONERROR(_ComputeClear(Engine, Format, Flags));

        /* Test for clearing render target. */
        if (Flags & gcvCLEAR_COLOR)
        {
            /* Send the clear command to the hardware. */
            gcmONERROR(
                gcoHARDWARE_ClearRect(Address,
                                      Memory,
                                      Stride,
                                      left, top, right, bottom,
                                      Engine->hwClearColorFormat,
                                      Engine->hwClearColor,
                                      Engine->hwClearColorMask,
                                      Width,
                                      Height));

        }

        /* Test for clearing depth or stencil buffer. */
        if (Flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
        {
            /* Zero mask. */
            mask = 0;

            if (Flags & gcvCLEAR_DEPTH)
            {
                /* Enable depth clearing.*/
                mask |= Engine->hwClearDepthMask;
            }

            if (Flags & gcvCLEAR_STENCIL)
            {
                /* Enable stencil clearing.*/
                mask |= Engine->hwClearStencilMask;
            }

            if (mask != 0)
            {
                /* Send clear command to hardware. */
    			gcmONERROR(
                    gcoHARDWARE_ClearRect(Address,
                                          Memory,
                                          Stride,
                                          left, top, right, bottom,
                                          Engine->hwClearDepthFormat,
                                          Engine->hwClearDepth,
                                          mask,
                                          Width,
                                          Height));
            }
        }

        if (Flags & gcvCLEAR_HZ)
        {
            gctUINT width, height;

            /* Compute the hw specific clear window. */
            gcmONERROR(
                gcoHARDWARE_ComputeClearWindow(Stride,
                                               &width,
                                               &height));

            /* Send clear command to hardware. */
            gcmONERROR(
                gcoHARDWARE_ClearRect(Address,
                                      Memory,
                                      width * 4,
                                      0, 0, width, height,
                                      gcvSURF_A8R8G8B8,
                                      Engine->hwClearHzDepth,
                                      0xF,
                                      width,
                                      height));
        }
    }

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
**  gco3D_Clear
**
**  Perform a clear with the current clear colors, depth values, or stencil
**  values, based on the specified flags.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT32 Address
**          Base address of surface to clear.
**
**      gctUINT32 Stride
**          Stride of surface to clear.
**
**      gceSURF_FORMAT Format
**          Format of surface to clear.
**
**      gctUINT32 Width
**          Width of surface to clear.
**
**      gctUINT32 Height
**          Height of surface to clear.
**
**      gctUINT32 Flags
**          Flags that specifiy the clear operations to perform.  Can be any
**          combination of:
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
gco3D_Clear(
    IN gco3D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Width,
    IN gctUINT32 Height,
    IN gctUINT32 Flags
    )
{
    gctUINT8 mask;
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Address=0x%08x Stride=%u Format=%d Width=%u "
                  "Height=%u Flags=0x%x",
                  Engine, Address, Stride, Format, Width, Height, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(Flags != 0);

    /* Compute clear values. */
    gcmONERROR(_ComputeClear(Engine, Format, Flags));

    /* Test for clearing render target. */
    if (Flags & gcvCLEAR_COLOR)
    {
        /* Send the clear command to the hardware. */
        gcmONERROR(
            gcoHARDWARE_Clear(Address,
                              Stride,
                              0, 0, Width, Height,
                              Engine->hwClearColorFormat,
                              Engine->hwClearColor,
                              Engine->hwClearColorMask,
                              Width, Height));

    }

    /* Test for clearing depth or stencil buffer. */
    if (Flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
    {
        /* Zero mask. */
        mask = 0;

        if (Flags & gcvCLEAR_DEPTH)
        {
            /* Enable depth clearing.*/
            mask |= Engine->hwClearDepthMask;
        }

        if (Flags & gcvCLEAR_STENCIL)
        {
            /* Enable stencil clearing.*/
            mask |= Engine->hwClearStencilMask;
        }

        if (mask != 0)
        {
            /* Send clear command to hardware. */
            gcmONERROR(
                gcoHARDWARE_Clear(Address,
                                  Stride,
                                  0, 0, Width, Height,
                                  Engine->hwClearDepthFormat,
                                  Engine->hwClearDepth,
                                  mask,
                                  Width, Height));
        }
    }

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
**  gco3D_ClearTileStatus
**
**  Perform a tile status clear with the current clear colors, depth values, or
**  stencil values, based on the specified flags.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcsSURF_INFO_PTR Surface
**          Pointer of the surface to clear.
**
**      gctUINT32 TileStatusAddress
**          Base address of the tile status buffer to clear.
**
**      gctUINT32 Flags
**          Flags that specifiy the clear operations to perform.  Can be any
**          combination of:
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
gco3D_ClearTileStatus(
    IN gco3D Engine,
    IN gcsSURF_INFO_PTR Surface,
    IN gctUINT32 TileStatusAddress,
    IN gctUINT32 Flags
    )
{
    gctUINT8 mask;
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x TileStatusAddress=0x%08x Flags=x%x",
                  Engine, Surface, TileStatusAddress, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(Flags != 0);

    /* Compute clear values. */
    gcmONERROR(
        _ComputeClear(Engine, Surface->format, Flags));

    /* Flush the tile status cache. */
    gcmONERROR(
        gcoHARDWARE_FlushTileStatus(Surface,
                                    gcvFALSE));

    /* Test for clearing render target. */
    if (Flags & gcvCLEAR_COLOR)
    {
        /* Send the clear command to the hardware. */
        status =
            gcoHARDWARE_ClearTileStatus(Surface,
                                        TileStatusAddress,
                                        0,
                                        gcvSURF_RENDER_TARGET,
                                        Engine->hwClearColor,
                                        Engine->hwClearColorMask);

        if (status == gcvSTATUS_NOT_SUPPORTED)
        {
            goto OnError;
        }

        gcmONERROR(status);
    }

    /* Test for clearing depth or stencil buffer. */
    if (Flags & (gcvCLEAR_DEPTH | gcvCLEAR_STENCIL))
    {
        /* Zero mask. */
        mask = 0;

        if (Flags & gcvCLEAR_DEPTH)
        {
            /* Enable depth clearing.*/
            mask |= Engine->hwClearDepthMask;
        }

        if (Flags & gcvCLEAR_STENCIL)
        {
            /* Enable stencil clearing.*/
            mask |= Engine->hwClearStencilMask;
        }

        if (mask != 0)
        {
            /* Send clear command to hardware. */
            status = gcoHARDWARE_ClearTileStatus(Surface,
                                                 TileStatusAddress,
                                                 0,
                                                 gcvSURF_DEPTH,
                                                 Engine->hwClearDepth,
                                                 mask);

            if (gcmIS_ERROR(status))
            {
                /* Print error only if it's really an error. */
                if (status != gcvSTATUS_NOT_SUPPORTED)
                {
                    gcmONERROR(status);
                }

                /* Otherwise just quietly return. */
                goto OnError;
            }

            /* Send semaphore from RASTER to PIXEL. */
            gcmONERROR(
                gcoHARDWARE_Semaphore(gcvWHERE_RASTER,
                                      gcvWHERE_PIXEL,
                                      gcvHOW_SEMAPHORE));
        }
        else
        {
            gcmFOOTER_ARG("%s", "gcvSTATUS_SKIP");
            return gcvSTATUS_SKIP;
        }
    }

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
**  gco3D_ClearHzTileStatus
**
**  Perform a hierarchical Z tile status clear with the current depth values.
**  Note that this function does not recompute the clear colors, since it must
**  be called after gco3D_ClearTileStatus has cleared the depth tile status.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcsSURF_INFO_PTR Surface
**          Pointer of the depth surface to clear.
**
**      gcsSURF_NODE_PTR TileStatusAddress
**          Pointer to the hierarhical Z tile status node toclear.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_ClearHzTileStatus(
    IN gco3D Engine,
    IN gcsSURF_INFO_PTR Surface,
    IN gcsSURF_NODE_PTR TileStatus
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Surface=0x%x TileStatus=0x%x",
                  Engine, Surface, TileStatus);

    /* Send clear command to hardware. */
    gcmONERROR(
        gcoHARDWARE_ClearTileStatus(Surface,
                                    TileStatus->physical,
                                    TileStatus->size,
                                    gcvSURF_HIERARCHICAL_DEPTH,
                                    Engine->hwClearHzDepth,
                                    0xF));

    /* Send semaphore from RASTER to PIXEL. */
    gcmONERROR(
        gcoHARDWARE_Semaphore(gcvWHERE_RASTER,
                              gcvWHERE_PIXEL,
                              gcvHOW_SEMAPHORE));

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
**  gco3D_EnableBlending
**
**  Enable or disable blending.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable blending, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableBlending(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending. */
    status = gcoHARDWARE_SetBlendEnable(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendFunction
**
**  Set blending function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceBLEND_UNIT Unit
**          Unit to set blending function for.  Can be one of the following
**          values:
**
**              gcvBLEND_SOURCE - Set source blending function.
**              gcvBLEND_TARGET - Set target blending function.
**
**      gceBLEND_FUNCTION FunctionRGB
**          Blending function for RGB channels.
**
**      gceBLEND_FUNCTION FunctionAlpha
**          Blending function for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendFunction(
    IN gco3D Engine,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Unit=%d FunctionRGB=%d FunctionAlpha=%d",
                  Engine, Unit, FunctionRGB, FunctionAlpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    if (Unit == gcvBLEND_SOURCE)
    {
        /* Program source blend function. */
        status = gcoHARDWARE_SetBlendFunctionSource(FunctionRGB,
                                                    FunctionAlpha);
    }
    else
    {
        /* Program target blend function. */
        status = gcoHARDWARE_SetBlendFunctionTarget(FunctionRGB,
                                                    FunctionAlpha);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendMode
**
**  Set blending mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcvBLEND_Mode ModeRGB
**          Blending mode for RGB channels.
**
**      gceBLEND_MODE ModeAlpha
**          Blending mode for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendMode(
    IN gco3D Engine,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x ModeRGB=%d ModeAlpha=%d",
                  Engine, ModeRGB, ModeAlpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending. */
    status = gcoHARDWARE_SetBlendMode(ModeRGB,
                                      ModeAlpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendColor
**
**  Set the current blending color.  The blending color is specified in unsigned
**  integers between 0 and 255.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT Red
**          Blending color for red channel.
**
**      gctUINT Green
**          Blending color for green channel.
**
**      gctUINT Blue
**          Blending color for blue channel.
**
**      gctUINT Alpha
**          Blending color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendColor(
    IN gco3D Engine,
    IN gctUINT Red,
    IN gctUINT Green,
    IN gctUINT Blue,
    IN gctUINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%u Green=%u Blue=%u Alpha=%u",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending color.  Clamp between 0 and 255. */
    status = gcoHARDWARE_SetBlendColor((gctUINT8) gcmMIN(Red,   255),
                                       (gctUINT8) gcmMIN(Green, 255),
                                       (gctUINT8) gcmMIN(Blue,  255),
                                       (gctUINT8) gcmMIN(Alpha, 255));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendColorX
**
**  Set the current blending color.  The blending color is specified in fixed
**  point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Red
**          Blending color for red channel.
**
**      gctFIXED_POINT Green
**          Blending color for green channel.
**
**      gctFIXED_POINT Blue
**          Blending color for blue channel.
**
**      gctFIXED_POINT Alpha
**          Blending color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending color.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetBlendColorX(gcmCLAMP(Red,   0, gcvONE_X),
                                        gcmCLAMP(Green, 0, gcvONE_X),
                                        gcmCLAMP(Blue,  0, gcvONE_X),
                                        gcmCLAMP(Alpha, 0, gcvONE_X));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetBlendColorF
**
**  Set the current blending color.  The blending color is specified in floating
**  point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Red
**          Blending color for red channel.
**
**      gctFLOAT Green
**          Blending color for green channel.
**
**      gctFLOAT Blue
**          Blending color for blue channel.
**
**      gctFLOAT Alpha
**          Blending color for alpha channel.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetBlendColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program blending color.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetBlendColorF(gcmCLAMP(Red,   0.0f, 1.0f),
                                        gcmCLAMP(Green, 0.0f, 1.0f),
                                        gcmCLAMP(Blue,  0.0f, 1.0f),
                                        gcmCLAMP(Alpha, 0.0f, 1.0f));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetCulling
**
**  Set culling mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceCULL Mode
**          Culling mode.  Can be one of the following values:
**
**              gcvCULL_NONE - Don't cull any triangle.
**              gcvCULL_CCW  - Cull counter clock-wise triangles.
**              gcvCULL_CW   - Cull clock-wise triangles.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetCulling(
    IN gco3D Engine,
    IN gceCULL Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the culling mode. */
    status = gcoHARDWARE_SetCulling(Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetPointSizeEnable
**
**  Enable/Disable point size.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable point sprite, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetPointSizeEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the point size enable/disable. */
    status = gcoHARDWARE_SetPointSizeEnable(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetPointSprite
**
**  Set point sprite.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable point sprite, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetPointSprite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the point sprite enable/disable. */
    status = gcoHARDWARE_SetPointSprite(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFill
**
**  Set fill mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceFILL Mode
**          Fill mode.  Can be one of the following values:
**
**              gcvFILL_POINT      - Draw points at each vertex.
**              gcvFILL_WIRE_FRAME - Draw triangle borders.
**              gcvFILL_SOLID      - Fill triangles.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFill(
    IN gco3D Engine,
    IN gceFILL Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the fill mode. */
    status = gcoHARDWARE_SetFill(Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthCompare.
**
**  Set the depth compare.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceCOMPARE Compare
**          Depth compare.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Compare=%d", Engine, Compare);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program depth compare. */
    status = gcoHARDWARE_SetDepthCompare(Compare);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableDepthWrite.
**
**  Enable or disable writing of depth values.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable writing of depth values, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableDepthWrite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Save depth write enable bit */
    Engine->depthEnableMask = Enable;

    /* Program the depth write. */
    status = gcoHARDWARE_SetDepthWrite(Engine->depthMode != gcvDEPTH_NONE
                                       ? Enable
                                       : gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthMode.
**
**  Set the depth mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceDEPTH_MODE Mode
**          Mode for depth.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthMode(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Save depth mode. */
    Engine->depthMode = Mode;

    /* Program the depth mode. */
    status = gcoHARDWARE_SetDepthMode(Mode);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    /* Update depth write bit. */
    status = gcoHARDWARE_SetDepthWrite(Mode != gcvDEPTH_NONE
                                       ? Engine->depthEnableMask
                                       : gcvFALSE);
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthRangeX.
**
**  Set the depth range, specified in fixed point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceDEPTH_MODE Mode
**          Mode for depth.
**
**      gctFIXED_POINT Near
**          Near value for depth specified in fixed point.
**
**      gctFIXED_POINT Far
**          Far value for depth specified in fixed point.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthRangeX(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d Near=%f Far=%f",
                  Engine, Mode, gcoMATH_Fixed2Float(Near),
                  gcoMATH_Fixed2Float(Far));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth range.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetDepthRangeX(Mode,
                                        gcmCLAMP(Near, 0, gcvONE_X),
                                        gcmCLAMP(Far,  0, gcvONE_X));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthRangeF.
**
**  Set the depth range, specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceDEPTH_MODE Mode
**          Mode for depth.
**
**      gctFLOAT Near
**          Near value for depth specified in floating point.
**
**      gctFLOAT Far
**          Far value for depth specified in floating point.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthRangeF(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d Near=%f Far=%f",
                  Engine, Mode, Near, Far);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth range.  Clamp between 0.0 and 1.0. */
    status = gcoHARDWARE_SetDepthRangeF(Mode,
                                        gcmCLAMP(Near, 0.0f, 1.0f),
                                        gcmCLAMP(Far,  0.0f, 1.0f));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetLastPixelEnable.
**
**  Set the depth range, specified in floating point.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**      gctBOOL Enable
**          Enable the last pixel for line drawing (always 0 for openGL)
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetLastPixelEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the Last Pixel Enable register */
    status = gcoHARDWARE_SetLastPixelEnable(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthScaleBiasF.
**
**  Set the depth range, depth scale and bias.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT depthScale
**          Depth Scale to additional scale to the Z
**
**      gctFLOAT depthBias
**          Depth Bias addition to the Z
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthScaleBiasF(
    IN gco3D Engine,
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x DepthScale=%f DepthBias=%f",
                  Engine, DepthScale, DepthBias);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth scale and Bias */
    status = gcoHARDWARE_SetDepthScaleBiasF(DepthScale,
                                            DepthBias);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthScaleBiasX.
**
**  Set the depth range, depth scale and bias.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT depthScale
**          Depth Scale to additional scale to the Z
**
**      gctFIXED_POINT depthBias
**          Depth Bias addition to the Z
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthScaleBiasX(
    IN gco3D Engine,
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x DepthScale=%f DepthBias=%f",
                  Engine, gcoMATH_Fixed2Float(DepthScale),
                  gcoMATH_Fixed2Float(DepthBias));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the depth scale and Bias */
    status = gcoHARDWARE_SetDepthScaleBiasX(DepthScale,
                                            DepthBias);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableDither.
**
**  Enable or disable dithering.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable dithering, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableDither(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program dithering. */
    status = gcoHARDWARE_SetDither(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetColorWrite.
**
**  Set color write enable bits.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Enable
**          Color write enable bits:
**
**              bit 0: writing of blue enabled when set.
**              bit 1: writing of green enabled when set.
**              bit 2: writing of red enabled when set.
**              bit 3: writing of alpha enabled when set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetColorWrite(
    IN gco3D Engine,
    IN gctUINT8 Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Save the color write mask bits */
    Engine->colorEnableMask = Enable;

    /* Program color write enable bits. */
    status = gcoHARDWARE_SetColorWrite(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetEarlyDepth.
**
**  Enable or disable early depth.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable early depth, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetEarlyDepth(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program early depth. */
    status = gcoHARDWARE_SetEarlyDepth(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAllEarlyDepthModes.
**
**  Enable or disable all early depth operations.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Disable
**          gcvTRUE to disable early depth operations, gcvTRUE to enable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAllEarlyDepthModes(
    IN gco3D Engine,
    IN gctBOOL Disable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Disable=%d", Engine, Disable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program early depth. */
    status = gcoHARDWARE_SetAllEarlyDepthModes(Disable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetDepthOnly.
**
**  Enable or disable depth-only mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable depth-only mode, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetDepthOnly(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program depth only. */
    status =  gcoHARDWARE_SetDepthOnly(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilMode.
**
**  Set stencil mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_MODE Mode
**          Stencil mode.  Can be one of the following:
**
**              gcvSTENCIL_NONE        - Disable stencil.
**              gcvSTENCIL_SINGLE_SIDED - Use single-sided stencil.
**              gcvSTENCIL_DOUBLE_SIDED - Use double-sided stencil.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilMode(
    IN gco3D Engine,
    IN gceSTENCIL_MODE Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mode=%d", Engine, Mode);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil mode. */
    status = gcoHARDWARE_SetStencilMode(Mode);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilMask.
**
**  Set stencil mask.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Mask
**          Stencil mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mask=0x%x", Engine, Mask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil mask. */
    status = gcoHARDWARE_SetStencilMask(Mask);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilWriteMask.
**
**  Set stencil write mask.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Mask
**          Stencil write mask.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilWriteMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Mask=0x%x", Engine, Mask);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Save the stencil mask bits. */
    Engine->stencilEnableMask = Mask;

    /* Program the stencil write mask. */
    status = gcoHARDWARE_SetStencilWriteMask(Mask);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilReference.
**
**  Set stencil reference.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Reference
**          Stencil reference.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctBOOL  Front
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%u", Engine, Reference);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil reference. */
    status = gcoHARDWARE_SetStencilReference(Reference, Front);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilCompare.
**
**  Set stencil compare for either the front or back stencil.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil compare to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil compare.
**              gcvSTENCIL_BACK  - Set the back stencil compare.
**
**      gceCOMPARE Compare
**          Stencil compare mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilCompare(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Compare=%d", Engine, Where, Compare);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare operation. */
    status = gcoHARDWARE_SetStencilCompare(Where, Compare);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilPass.
**
**  Set stencil operation for either the front or back stencil when the stencil
**  compare passes.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil operation to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil operation.
**              gcvSTENCIL_BACK  - Set the back stencil operation.
**
**      gceSTENCIL_OPERATION Operation
**          Stencil operation when stencil compare passes.  Can be one of the
**          following:
**
**              gcvSTENCIL_KEEP               - Keep the current stencil value.
**              gcvSTENCIL_REPLACE            - Replace the stencil value with
**                                              the reference value.
**              gcvSTENCIL_ZERO               - Set the stencil value to zero.
**              gcvSTENCIL_INVERT             - Invert the stencil value.
**              gcvSTENCIL_INCREMENT          - Increment the stencil value.
**              gcvSTENCIL_DECREMENT          - Decrement the stencil value.
**              gcvSTENCIL_INCREMENT_SATURATE - Increment the stencil value, but
**                                              don't overflow.
**              gcvSTENCIL_DECREMENT_SATURATE - Decrement the stncil value, but
**                                              don't underflow.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilPass(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Operation=%d",
                  Engine, Where, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare pass operation. */
    status = gcoHARDWARE_SetStencilPass(Where, Operation);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilFail.
**
**  Set stencil operation for either the front or back stencil when the stencil
**  compare fails.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil operation to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil operation.
**              gcvSTENCIL_BACK  - Set the back stencil operation.
**
**      gceSTENCIL_OPERATION Operation
**          Stencil operation when stencil compare fails.  Can be one of the
**          following:
**
**              gcvSTENCIL_KEEP               - Keep the current stencil value.
**              gcvSTENCIL_REPLACE            - Replace the stencil value with
**                                             the reference value.
**              gcvSTENCIL_ZERO               - Set the stencil value to zero.
**              gcvSTENCIL_INVERT             - Invert the stencil value.
**              gcvSTENCIL_INCREMENT          - Increment the stencil value.
**              gcvSTENCIL_DECREMENT          - Decrement the stencil value.
**              gcvSTENCIL_INCREMENT_SATURATE - Increment the stencil value, but
**                                             don't overflow.
**              gcvSTENCIL_DECREMENT_SATURATE - Decrement the stncil value, but
**                                             don't underflow.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Operation=%d",
                  Engine, Where, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare fail operation. */
    status = gcoHARDWARE_SetStencilFail(Where, Operation);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilDepthFail.
**
**  Set stencil operation for either the front or back stencil when the depth
**  compare fails.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceSTENCIL_WHERE Where
**          Select which stencil operation to set.  Can be one of the following:
**
**              gcvSTENCIL_FRONT - Set the front stencil operation.
**              gcvSTENCIL_BACK  - Set the back stencil operation.
**
**      gceSTENCIL_OPERATION Operation
**          Stencil operation when depth compare fails.  Can be one of the
**          following:
**
**              gcvSTENCIL_KEEP               - Keep the current stencil value.
**              gcvSTENCIL_REPLACE            - Replace the stencil value with
**                                             the reference value.
**              gcvSTENCIL_ZERO               - Set the stencil value to zero.
**              gcvSTENCIL_INVERT             - Invert the stencil value.
**              gcvSTENCIL_INCREMENT          - Increment the stencil value.
**              gcvSTENCIL_DECREMENT          - Decrement the stencil value.
**              gcvSTENCIL_INCREMENT_SATURATE - Increment the stencil value, but
**                                             don't overflow.
**              gcvSTENCIL_DECREMENT_SATURATE - Decrement the stncil value, but
**                                             don't underflow.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilDepthFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Where=%d Operation=%d",
                  Engine, Where, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the stencil compare depth fail operation. */
    status = gcoHARDWARE_SetStencilDepthFail(Where,
                                             Operation);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetStencilAll.
**
**  Set all stncil states in one blow.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcsSTENCIL_INFO_PTR Info
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetStencilAll(
    IN gco3D Engine,
    IN gcsSTENCIL_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Info=0x%x", Engine, Info);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    /* Save the stencil mask bits. */
    Engine->stencilEnableMask = Info->writeMask;

    /* Program the stencil compare depth fail operation. */
    status = gcoHARDWARE_SetStencilAll(Info);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaTest.
**
**  Enable or disable alpha test.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable alpha test, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaTest(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test. */
    status = gcoHARDWARE_SetAlphaTest(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaCompare.
**
**  Set the alpha test compare mode.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gceCOMPARE Compare
**          Alpha test compare mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Compare=%d", Engine, Compare);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test compare. */
    status = gcoHARDWARE_SetAlphaCompare(Compare);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaReference
**
**  Set the alpha reference value.  The alpha reference value is specified as a
**  unsigned integer between 0 and 255.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Reference
**          Alpha reference value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaReference(
    IN gco3D Engine,
    IN gctUINT8 Reference
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%u", Engine, Reference);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test reference. */
    status = gcoHARDWARE_SetAlphaReference(Reference);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaReferenceX
**
**  Set the alpha reference value.  The alpha reference value is specified as a
**  fixed point between 0 and 1.0.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFIXED_POINT Reference
**          Alpha reference value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaReferenceX(
    IN gco3D Engine,
    IN gctFIXED_POINT Reference
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%f",
                  Engine, gcoMATH_Fixed2Float(Reference));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test reference. */
    status = gcoHARDWARE_SetAlphaReferenceX(Reference);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaReferenceF
**
**  Set the alpha reference value.  The alpha reference value is specified as a
**  floating point between 0 and 1.0.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Reference
**          Alpha reference value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaReferenceF(
    IN gco3D Engine,
    IN gctFLOAT Reference
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Reference=%f", Engine, Reference);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program the alpha test reference. */
    status = gcoHARDWARE_SetAlphaReferenceF(Reference);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAntiAliasLine.
**
**  Enable or disable anti-alias line.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          gcvTRUE to enable anti-alias line, gcvFALSE to disable.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAntiAliasLine(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program anti-alias line. */
    status = gcoHARDWARE_SetAntiAliasLine(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAALineWidth.
**
**  Set anti-alias line width scale.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctFLOAT Width
**          Anti-alias line width.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAALineWidth(
    IN gco3D Engine,
    IN gctFLOAT Width
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Width=%f", Engine, Width);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program anti-alias line width. */
    status = gcoHARDWARE_SetAALineWidth(Width);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAALineTexSlot.
**
**  Set anti-alias line texture slot.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctINT32 TexSlot
**          Anti-alias line texture slot number.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAALineTexSlot(
    IN gco3D Engine,
    IN gctUINT TexSlot
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x TexSlot=%u", Engine, TexSlot);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program anti-alias line width. */
    status = gcoHARDWARE_SetAALineTexSlot(TexSlot);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawPrimitives
**
**  Draw a number of primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
**
**      gctINT StartVertex
**          Starting vertex number to start drawing.  The starting vertex is
**          multiplied by the stream stride to compute the starting offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT StartVertex,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d StartVertex=%d PrimitiveCount=%lu",
                  Engine, Type, StartVertex, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawPrimitives(Type,
                                        StartVertex,
                                        PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawPrimitivesCount
**
**  Draw an array of of primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
**
**      gctINT StartVertex
**          Starting vertex array of number to start drawing.  The starting vertex is
**          multiplied by the stream stride to compute the starting offset.
**
**      gctSIZE_T VertexCount
**          Number of vertices to draw per primitive.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawPrimitivesCount(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d StartVertex=%d PrimitiveCount=%lu",
                  Engine, Type, StartVertex, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(StartVertex != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(VertexCount != gcvNULL);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawPrimitivesCount(Type,
                                             StartVertex,
                                             VertexCount,
                                             PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawPrimitivesOffset
**
**  Draw a number of primitives using offsets.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
**
**      gctINT32 StartOffset
**          Starting offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d StartOffset=%d PrimitiveCount=%lu",
                  Engine, Type, StartOffset, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawPrimitivesOffset(Type,
                                              StartOffset,
                                              PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawIndexedPrimitives
**
**  Draw a number of indexed primitives.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                            first and last line will also be
**                                            connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                            out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                            out in a fan.
**
**      gctINT BaseVertex
**          Base vertex number which will be added to any indexed vertex read
**          from the index buffer.
**
**      gctINT StartIndex
**          Starting index number to start drawing.  The starting index is
**          multiplied by the index buffer stride to compute the starting
**          offset.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawIndexedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT BaseVertex,
    IN gctINT StartIndex,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d BaseVertex=%d StartIndex=%d "
                  "PrimitiveCount=%lu",
                  Engine, Type, BaseVertex, StartIndex, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawIndexedPrimitives(Type,
                                               BaseVertex,
                                               StartIndex,
                                               PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_DrawIndexedPrimitivesOffset
**
**  Draw a number of indexed primitives using offsets.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gcePRIMITIVE Type
**          Type of the primitives to draw.  Can be one of the following:
**
**              gcvPRIMITIVE_POINT_LIST     - List of points.
**              gcvPRIMITIVE_LINE_LIST      - List of lines.
**              gcvPRIMITIVE_LINE_STRIP     - List of connecting lines.
**              gcvPRIMITIVE_LINE_LOOP      - List of connecting lines where the
**                                           first and last line will also be
**                                           connected.
**              gcvPRIMITIVE_TRIANGLE_LIST  - List of triangles.
**              gcvPRIMITIVE_TRIANGLE_STRIP - List of connecting triangles layed
**                                           out in a strip.
**              gcvPRIMITIVE_TRIANGLE_FAN   - List of connecting triangles layed
**                                           out in a fan.
**
**      gctINT BaseOffset
**          Base offset which will be added to any indexed vertex offset read
**          from the index buffer.
**
**      gctINT StartOffset
**          Starting offset into the index buffer to start drawing.
**
**      gctSIZE_T PrimitiveCount
**          Number of primitives to draw.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_DrawIndexedPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Type=%d BaseOffset=%d StartOffset=%d "
                  "PrimitiveCount=%lu",
                  Engine, Type, BaseOffset, StartOffset, PrimitiveCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);
    gcmDEBUG_VERIFY_ARGUMENT(PrimitiveCount > 0);

    /* Call the gcoHARDWARE object. */
    status = gcoHARDWARE_DrawIndexedPrimitivesOffset(Type,
                                                     BaseOffset,
                                                     StartOffset,
                                                     PrimitiveCount);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAntiAlias
**
**  Enable or disable anti-alias.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctBOOL Enable
**          Enable anti-aliasing when gcvTRUE or disable anto-aliasing when
**          gcvFALSE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAntiAlias(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Enable=%d", Engine, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Call hardware. */
    status = gcoHARDWARE_SetAntiAlias(Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_WriteBuffer
**
**  Write data into the command buffer.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer that contains the data to be copied.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to copy.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_WriteBuffer(
    IN gco3D Engine,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Data=0x%x Bytes=%lu Aligned=%d",
                  Engine, Data, Bytes, Aligned);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Call hardware. */
    status = gcoHARDWARE_WriteBuffer(gcvNULL, Data, Bytes, Aligned);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFragmentConfiguration
**
**  Set the fragment processor configuration.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctBOOL ColorFromStream
**          Selects whether the fragment color comes from the color stream or
**          from the constant.
**
**      gctBOOL EnableFog
**          Fog enable flag.
**
**      gctBOOL EnableSmoothPoint
**          Antialiased point enable flag.
**
**      gctUINT32 ClipPlanes
**          Clip plane enable bits:
**              [0] for plane 0;
**              [1] for plane 1;
**              [2] for plane 2;
**              [3] for plane 3;
**              [4] for plane 4;
**              [5] for plane 5.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFragmentConfiguration(
    IN gco3D Engine,
    IN gctBOOL ColorFromStream,
    IN gctBOOL EnableFog,
    IN gctBOOL EnableSmoothPoint,
    IN gctUINT32 ClipPlanes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x ColorFromStream=%d EnableFog=%d "
                  "EnableSmoothPoint=%d ClipPlanes=%u",
                  Engine, ColorFromStream, EnableFog, EnableSmoothPoint,
                  ClipPlanes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_SetFragmentConfiguration(ColorFromStream,
                                                  EnableFog,
                                                  EnableSmoothPoint,
                                                  ClipPlanes);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_EnableTextureStage
**
**  Enable/disable texture stage operation.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gctBOOL Enable
**          Stage enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_EnableTextureStage(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Enable=%d", Engine, Stage, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_EnableTextureStage(Stage, Enable);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTextureColorMask
**
**  Program the channel enable masks for the color texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gctBOOL ColorEnabled
**          Determines whether RGB color result from the color texture
**          function affects the overall result or should be ignored.
**
**      gctBOOL AlphaEnabled
**          Determines whether A color result from the color texture
**          function affects the overall result or should be ignored.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTextureColorMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d ColorEnabled=%d AlphaEnabled=%d",
                  Engine, Stage, ColorEnabled, AlphaEnabled);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_SetTextureColorMask(Stage,
                                             ColorEnabled,
                                             AlphaEnabled);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTextureAlphaMask
**
**  Program the channel enable masks for the alpha texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gctBOOL ColorEnabled
**          Determines whether RGB color result from the alpha texture
**          function affects the overall result or should be ignored.
**
**      gctBOOL AlphaEnabled
**          Determines whether A color result from the alpha texture
**          function affects the overall result or should be ignored.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTextureAlphaMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d ColorEnabled=%d AlphaEnabled=%d",
                  Engine, Stage, ColorEnabled, AlphaEnabled);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set configuration. */
    status = gcoHARDWARE_SetTextureAlphaMask(Stage,
                                             ColorEnabled,
                                             AlphaEnabled);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFragmentColor
**
**  Program the constant fragment color to be used when there is no color
**  defined stream.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      Red, Green, Blue, Alpha
**          Color value to be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFragmentColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFragmentColorX(Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetFragmentColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFragmentColorF(Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetFogColor
**
**  Program the constant fog color to be used in the fog equation when fog
**  is enabled.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      Red, Green, Blue, Alpha
**          Color value to be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetFogColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFogColorX(Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetFogColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetFogColorF(gcvNULL,
                                      Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetTetxureColor
**
**  Program the constant texture color to be used when selected by the tetxure
**  function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      Red, Green, Blue, Alpha
**          Color value to be set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetTetxureColorX(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, gcoMATH_Fixed2Float(Red), gcoMATH_Fixed2Float(Green),
                  gcoMATH_Fixed2Float(Blue), gcoMATH_Fixed2Float(Alpha));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetTetxureColorX(Stage,
                                          Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gco3D_SetTetxureColorF(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Red=%f Green=%f Blue=%f Alpha=%f",
                  Engine, Stage, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the color. */
    status = gcoHARDWARE_SetTetxureColorF(Stage,
                                          Red, Green, Blue, Alpha);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetColorTextureFunction
**
**  Configure color texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gceTEXTURE_FUNCTION Function
**          Texture function.
**
**      gceTEXTURE_SOURCE Source0, Source1, Source2
**          The source of the value for the function.
**
**      gceTEXTURE_CHANNEL Channel0, Channel1, Channel2
**          Determines whether the value comes from the color, alpha channel;
**          straight or inversed.
**
**      gctINT Scale
**          Result scale value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetColorTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Function=%d Source0=%d Channel0=%d "
                  "Source1=%d Channel1=%d Source2=%d Channel2=%d",
                  Engine, Stage, Function, Source0, Channel0, Source1, Channel1,
                  Source2, Channel2);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the texture function. */
    status = gcoHARDWARE_SetColorTextureFunction(Stage,
                                                 Function,
                                                 Source0, Channel0,
                                                 Source1, Channel1,
                                                 Source2, Channel2,
                                                 Scale);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetAlphaTextureFunction
**
**  Configure alpha texture function.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctINT Stage
**          Target stage number.
**
**      gceTEXTURE_FUNCTION Function
**          Texture function.
**
**      gceTEXTURE_SOURCE Source0, Source1, Source2
**          The source of the value for the function.
**
**      gceTEXTURE_CHANNEL Channel0, Channel1, Channel2
**          Determines whether the value comes from the color, alpha channel;
**          straight or inversed.
**
**      gctINT Scale
**          Result scale value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetAlphaTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Stage=%d Function=%d Source0=%d Channel0=%d "
                  "Source1=%d Channel1=%d Source2=%d Channel2=%d",
                  Engine, Stage, Function, Source0, Channel0, Source1, Channel1,
                  Source2, Channel2);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Set the texture function. */
    status = gcoHARDWARE_SetAlphaTextureFunction(Stage,
                                                 Function,
                                                 Source0, Channel0,
                                                 Source1, Channel1,
                                                 Source2, Channel2,
                                                 Scale);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_Semaphore
**
**  Send sempahore and stall until sempahore is signalled.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gceWHERE From
**          Semaphore source.
**
**      GCHWERE To
**          Sempahore destination.
**
**      gceHOW How
**          What to do.  Can be a one of the following values:
**
**              gcvHOW_SEMAPHORE            Send sempahore.
**              gcvHOW_STALL                Stall.
**              gcvHOW_SEMAPHORE_STALL  Send semaphore and stall.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gco3D_Semaphore(
    IN gco3D Engine,
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x From=%d To=%d How=%d", Engine, From, To, How);

    /* Route to hardware. */
    status = gcoHARDWARE_Semaphore(From, To, How);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gco3D_SetCentroids
**
**  Set the subpixels center.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gctUINT32 Index
**          Indicate which matrix need to be set.
**
**      gctPOINTER Centroids
**          Ponter to the centroid array.
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetCentroids(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctPOINTER Centroids
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Index=%u Centroids=0x%x",
                  Engine, Index, Centroids);

    /* Route to hardware. */
    status = gcoHARDWARE_SetCentroids(Index, Centroids);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gco3D_InvokeThreadWalker
**
**  Start OCL thread walker.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to the gco3D object.
**
**      gcsTHREAD_WALKER_INFO_PTR Info
**          Pointer to the informational structure.
*/
gceSTATUS
gco3D_InvokeThreadWalker(
    IN gco3D Engine,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%08X Info=0x%08X", Engine, Info);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

	/* Route to hardware. */
	status = gcoHARDWARE_InvokeThreadWalker(Info);

	/* Return the status. */
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**
**  gco3D_SetLogicOp.
**
**  Set logic op.
**
**  INPUT:
**
**      gco3D Engine
**          Pointer to an gco3D object.
**
**      gctUINT8 Rop
**          4 bit rop code.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gco3D_SetLogicOp(
    IN gco3D Engine,
    IN gctUINT8 Rop
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Engine=0x%x Rop=%d", Engine, Rop);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Engine, gcvOBJ_3D);

    /* Program logicOp. */
    status = gcoHARDWARE_SetLogicOp(Rop);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gco3D_Construct(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    )
{
    *Engine = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_Destroy(
    IN gco3D Engine
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAPI(
    IN gco3D Engine,
    IN gceAPI ApiType
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTarget(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_UnsetTarget(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_UnsetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetViewport(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetScissors(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearColor(
    IN gco3D Engine,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearDepthX(
    IN gco3D Engine,
    IN gctFIXED_POINT Depth
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearDepthF(
    IN gco3D Engine,
    IN gctFLOAT Depth
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetClearStencil(
    IN gco3D Engine,
    IN gctUINT32 Stencil
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetShading(
    IN gco3D Engine,
    IN gceSHADING Shading
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_ClearRect(
    IN gco3D Engine,
    IN gctUINT32 Address,
    IN gctPOINTER Memory,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gctUINT32 Width,
    IN gctUINT32 Height,
    IN gctUINT32 Flags
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_Clear(
    IN gco3D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Width,
    IN gctUINT32 Height,
    IN gctUINT32 Flags
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_ClearTileStatus(
    IN gco3D Engine,
    IN gcsSURF_INFO_PTR Surface,
    IN gctUINT32 TileStatusAddress,
    IN gctUINT32 Flags
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_ClearHzTileStatus(
    IN gco3D Engine,
    IN gcsSURF_INFO_PTR Surface,
    IN gcsSURF_NODE_PTR TileStatus
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableBlending(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendFunction(
    IN gco3D Engine,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendMode(
    IN gco3D Engine,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendColor(
    IN gco3D Engine,
    IN gctUINT Red,
    IN gctUINT Green,
    IN gctUINT Blue,
    IN gctUINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetBlendColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetCulling(
    IN gco3D Engine,
    IN gceCULL Mode
    )
{
    return gcvSTATUS_OK;
}
gceSTATUS gco3D_SetPointSizeEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetPointSprite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFill(
    IN gco3D Engine,
    IN gceFILL Mode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableDepthWrite(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthMode(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthRangeX(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthRangeF(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetLastPixelEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthScaleBiasF(
    IN gco3D Engine,
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthScaleBiasX(
    IN gco3D Engine,
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableDither(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetColorWrite(
    IN gco3D Engine,
    IN gctUINT8 Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetEarlyDepth(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetDepthOnly(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilMode(
    IN gco3D Engine,
    IN gceSTENCIL_MODE Mode
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilWriteMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctBOOL  Front
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilCompare(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilPass(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_SetAllEarlyDepthModes(
    IN gco3D Engine,
    IN gctBOOL Disable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilDepthFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetStencilAll(
    IN gco3D Engine,
    IN gcsSTENCIL_INFO_PTR Info
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaTest(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaReference(
    IN gco3D Engine,
    IN gctUINT8 Reference
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaReferenceX(
    IN gco3D Engine,
    IN gctFIXED_POINT Reference
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaReferenceF(
    IN gco3D Engine,
    IN gctFLOAT Reference
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAntiAliasLine(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAALineWidth(
    IN gco3D Engine,
    IN gctFLOAT Width
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAALineTexSlot(
    IN gco3D Engine,
    IN gctUINT TexSlot
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT StartVertex,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gco3D_DrawPrimitivesCount(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawIndexedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT BaseVertex,
    IN gctINT StartIndex,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_DrawIndexedPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAntiAlias(
    IN gco3D Engine,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_WriteBuffer(
    IN gco3D Engine,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFragmentConfiguration(
    IN gco3D Engine,
    IN gctBOOL ColorFromStream,
    IN gctBOOL EnableFog,
    IN gctBOOL EnableSmoothPoint,
    IN gctUINT32 ClipPlanes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_EnableTextureStage(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL Enable
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTextureColorMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTextureAlphaMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFragmentColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFragmentColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFogColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetFogColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTetxureColorX(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetTetxureColorF(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetColorTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetAlphaTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_Semaphore(
    IN gco3D Engine,
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetCentroids(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctPOINTER Centroids
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_InvokeThreadWalker(
    IN gco3D Engine,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gco3D_SetLogicOp(
    IN gco3D Engine,
    IN gctUINT8 Rop
    )
{
    return gcvSTATUS_OK;
}

#endif /* gcdNULL_DRIVER < 2 */
#endif /* VIVANTE_NO_3D */

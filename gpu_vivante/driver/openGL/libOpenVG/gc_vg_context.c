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




#include "gc_vg_precomp.h"
#include <string.h>
#include <EGL/egl.h>
#include <gc_egl_common.h>

/*******************************************************************************
**
** _ConstructChipName
**
** Construct chip name string.
**
** INPUT:
**
**    ChipID
**       Chip ID.
**
**    ChipName
**       Pointer to the chip name string.
**
** OUTPUT:
**
**    Nothing.
*/

static void _ConstructChipName(
    gctUINT32 ChipID,
    gctSTRING ChipName
    )
{
    gctINT i;
    gctBOOL foundID;

    /* Constant name postifx. */
    static char postfix[] = " Graphics Engine";
    const gctINT postfixLength = 17;

    /* Append GC to the string. */
    * (gctUINT16_PTR) ChipName = gcmMAKE2CHAR('G', 'C');
    ChipName += 2;

    /* Reset the ID found flag. */
    foundID = gcvFALSE;

    /* Translate the ID. */
    for (i = 0; i < 8; i++)
    {
        /* Get the current digit. */
        gctUINT8 digit = (gctUINT8) ((ChipID >> 28) & 0xFF);

        /* Append the digit to the string. */
        if (foundID || digit)
        {
            *ChipName++ = '0' + digit;
            foundID = gcvTRUE;
        }

        /* Advance to the next digit. */
        ChipID <<= 4;
    }

    /* Add the postfix. */
    for (i = 0; i < postfixLength; i++)
    {
        *ChipName++ = postfix[i];
    }
}


/*******************************************************************************
**
** vgfFlushPipe
**
** Flush and stall the hardware if needed.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Finish
**       If not zero, makes sure the hardware is idle.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfFlushPipe(
    IN vgsCONTEXT_PTR Context,
    IN gctBOOL Finish
    )
{
    gceSTATUS status;

    do
    {
        /* Flush PE cache. */
        gcmERR_BREAK(gcoHAL_Flush(Context->hal));

        /* Need to stall the hardware as well? */
        if (Finish)
        {
            /* Make sure the hardware is idle. */
            gcmERR_BREAK(gcoHAL_Commit(Context->hal, gcvTRUE));

            /* Reset the states. */
            Context->imageDirty = vgvIMAGE_READY;
        }
        else
        {
            /* Make sure the hardware is idle. */
            gcmERR_BREAK(gcoHAL_Commit(Context->hal, gcvFALSE));

            /* Mark the states as not finished. */
            Context->imageDirty = vgvIMAGE_NOT_FINISHED;
        }
    }
    while (gcvFALSE);

    /* Return state. */
    return status;
}


/*******************************************************************************
**
** veglCreateContext
**
** Create and initialize a context object.
**
** INPUT:
**
**    Os
**       Pointer to a gcoOS object.
**
**    Hal
**       Pointer to a gcoHAL object.
**
**    SharedContext
**       Pointer to an existing OpenVG context. Any VGPath and VGImage objects
**       defined in SharedContext will be accessible from the new context, and
**       vice versa. If no sharing is desired, the value EGL_NO_CONTEXT should
**       be used.
**
** OUTPUT:
**
**    vgsCONTEXT_PTR
**       Pointer to the new context object.
*/

static void *
veglCreateContext(
    gcoOS Os,
    gcoHAL Hal,
    gctPOINTER SharedContext
    )
{
    gceSTATUS status;
    vgsCONTEXT_PTR context = gcvNULL;

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_PARAMETERS,
        "%s(0x%08X, 0x%08X, 0x%08X);\n",
        __FUNCTION__,
        Os, Hal, SharedContext
        );

    do
    {
#if gcdENABLE_VG
        gcmERR_BREAK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_VG));
#endif

        /* Allocate the context structure. */
        gcmERR_BREAK(gcoOS_Allocate(
            Os,
            gcmSIZEOF(vgsCONTEXT),
            (gctPOINTER *) &context
            ));

        /* Init context structure. */
        gcoOS_ZeroMemory(context, gcmSIZEOF(vgsCONTEXT));

        /* Set API pointers. */
        context->hal = Hal;
        context->os  = Os;

        /* Query chip identity. */
        gcmERR_BREAK(gcoHAL_QueryChipIdentity(
            Hal,
            &context->chipModel,
            &context->chipRevision,
            gcvNULL,
            gcvNULL
            ));

        /* Test chip ID. */
        if (context->chipModel == 0)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Construct chip name. */
        _ConstructChipName(context->chipModel, context->chipName);

        /* Initialize driver strings. */
        context->chipInfo.vendor     = (VGubyte*) "Vivante Corporation";
        context->chipInfo.renderer   = (VGubyte*) context->chipName;
        context->chipInfo.version    = (VGubyte*) "1.1";
        context->chipInfo.extensions = (VGubyte*) "";

        /* Query VG engine version. */
        context->vg20 = gcoHAL_IsFeatureAvailable(
            Hal, gcvFEATURE_VG20
            ) == gcvSTATUS_TRUE;

        /* Query filter support. */
        context->filterSupported = gcoHAL_IsFeatureAvailable(
            Hal, gcvFEATURE_VG_FILTER
            ) == gcvSTATUS_TRUE;

        /* Get the 3D engine pointer. */
        gcmERR_BREAK(gcoHAL_GetVGEngine(
            context->hal, &context->vg
            ));

        /* Create a signal for waiting on. */
        gcmERR_BREAK(gcoOS_CreateSignal(
            context->os, gcvTRUE, &context->waitSignal
            ));

        /* Initialize the object cache. */
        gcmERR_BREAK(vgfObjectCacheStart(
            context, (vgsCONTEXT_PTR) SharedContext
            ));

        /* Construct the path storage manager. */
        gcmERR_BREAK(vgsPATHSTORAGE_Construct(
            context,
            gcmKB2BYTES(64),
            0,
            &context->pathStorage
            ));

        /* Construct the stroke storage manager. */
        gcmERR_BREAK(vgsPATHSTORAGE_Construct(
            context,
            gcmKB2BYTES(64),
            gcmMB2BYTES(2),
            &context->strokeStorage
            ));

        /* Construct a memory manager object for ARC coordinates. */
        gcmERR_BREAK(vgsMEMORYMANAGER_Construct(
            Os,
            gcmSIZEOF(vgsARCCOORDINATES),
            100,
            &context->arcCoordinates
            ));

        /* Initialize the context states. */
        vgfSetDefaultStates(context);

        /* Create default paint object. */
        gcmERR_BREAK(vgfReferencePaint(context, &context->defaultPaint));
        context->defaultPaint->object.userValid = VG_FALSE;

        /* Set fill and stroke paints to the default paint. */
        context->strokePaint = context->fillPaint = context->defaultPaint;
        context->strokeDefaultPaint = context->fillDefaultPaint = VG_TRUE;

        /* Report new context. */
        vgmREPORT_NEW_CONTEXT(context);
    }
    while (gcvFALSE);

    /* Free the context buffer if error. */
    if (gcmIS_ERROR(status) && (context != gcvNULL))
    {
        /* Delete default paint. */
        gcmVERIFY_OK(vgfDereferenceObject(
            context, (vgsOBJECT_PTR *) &context->defaultPaint
            ));

        /* Destroy the stroke storage mamager. */
        if (context->strokeStorage != gcvNULL)
        {
            gcmVERIFY_OK(vgsPATHSTORAGE_Destroy(context->strokeStorage));
        }

        /* Destroy the path storage mamager. */
        if (context->pathStorage != gcvNULL)
        {
            gcmVERIFY_OK(vgsPATHSTORAGE_Destroy(context->pathStorage));
        }

        /* Delete object cache. */
        gcmVERIFY_OK(vgfObjectCacheStop(context));

        /* Destroy the ARC coordinates memory manager. */
        if (context->arcCoordinates != gcvNULL)
        {
            gcmVERIFY_OK(vgsMEMORYMANAGER_Destroy(context->arcCoordinates));
        }

        /* Destroy the wait signal. */
        if (context->waitSignal != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_DestroySignal(context->os, context->waitSignal));
        }

        /* Free the context. */
        gcmVERIFY_OK(gcoOS_Free(Os, context));

        /* Reset the pointer. */
        context = gcvNULL;
    }

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_PARAMETERS,
        "%s() = 0x%08X;\n",
        __FUNCTION__,
        context
        );

    return context;
}


/*******************************************************************************
**
** veglDestroyContext
**
** Destroy a context object.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
** OUTPUT:
**
**    gctBOOL
**       Not zero if successfully destroyed.
*/

static EGLBoolean
veglDestroyContext(
    void * Context
    )
{
    gceSTATUS status;
    vgsCONTEXT_PTR context = (vgsCONTEXT_PTR) Context;

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_PARAMETERS,
        "%s(0x%08X);\n",
        __FUNCTION__,
        Context
        );

    do
    {
        vgsTHREADDATA_PTR thread;

        /* Cannot be NULL. */
        if (Context == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Get the thread information. */
        thread = vgfGetThreadData(gcvFALSE);

        /* Flush the pipe. */
        gcmERR_BREAK(vgfFlushPipe(context, gcvTRUE));

        /* Delete the temporary buffer. */
        if (context->tempBuffer != gcvNULL)
        {
            gcmERR_BREAK(gcoOS_Free(
                context->os, context->tempBuffer
                ));

            context->tempBuffer = gcvNULL;
        }

        /* Reset the current context. */
        if ((thread != gcvNULL) && (thread->context == context))
        {
            gcmERR_BREAK(gcoVG_UnsetTarget(
                context->vg, context->targetImage.surface
                ));

            gcmERR_BREAK(gcoVG_UnsetMask(
                context->vg, context->maskImage.surface
                ));

            thread->context = gcvNULL;
        }

        /* Delete temporary surface. */
        if (context->tempNode != gcvNULL)
        {
            /* Schedule the current buffer for deletion. */
            gcmERR_BREAK(gcoHAL_ScheduleVideoMemory(
                context->hal, context->tempNode
                ));

            /* Reset temporary surface. */
            context->tempNode     = gcvNULL;
            context->tempPhysical = ~0;
            context->tempLogical  = gcvNULL;
            context->tempSize     = 0;
        }

        /* Release images. */
        {
            gcoSURF surface = context->targetImage.surface;
            gcmERR_BREAK(vgfReleaseImage(context, &context->targetImage));

            if (surface != gcvNULL)
            {
                /* deference the target surface */
                gcmVERIFY_OK(gcoSURF_Destroy(surface));
            }

        }
        gcmERR_BREAK(vgfReleaseImage(context, &context->maskImage));
        gcmERR_BREAK(vgfReleaseImage(context, &context->tempMaskImage));
        gcmERR_BREAK(vgfReleaseImage(context, &context->wrapperImage));
        gcmERR_BREAK(vgfReleaseImage(context, &context->tempImage));

        /* Delete default paint. */
        gcmERR_BREAK(vgfDereferenceObject(
            context, (vgsOBJECT_PTR *) &context->defaultPaint
            ));

        /* Delete the object cache. */
        gcmERR_BREAK(vgfObjectCacheStop(context));

#if gcvSTROKE_KEEP_MEMPOOL
        /* Destroy the stroke conversion object. */
        if (context->strokeConversion != gcvNULL)
        {
            gcmERR_BREAK(vgfDestroyStrokeConversion(context->strokeConversion));
            context->strokeConversion = gcvNULL;
        }
#endif

        /* Destroy the stroke storage mamager. */
        if (context->strokeStorage != gcvNULL)
        {
            gcmERR_BREAK(vgsPATHSTORAGE_Destroy(context->strokeStorage));
            context->strokeStorage = gcvNULL;
        }

        /* Destroy the path storage mamager. */
        if (context->pathStorage != gcvNULL)
        {
            gcmERR_BREAK(vgsPATHSTORAGE_Destroy(context->pathStorage));
            context->pathStorage = gcvNULL;
        }

        /* Destroy the ARC coordinates memory manager. */
        if (context->arcCoordinates != gcvNULL)
        {
            gcmERR_BREAK(vgsMEMORYMANAGER_Destroy(context->arcCoordinates));
            context->arcCoordinates = gcvNULL;
        }

        /* Destroy the wait signal. */
        if (context->waitSignal != gcvNULL)
        {
            gcmERR_BREAK(gcoOS_DestroySignal(context->os, context->waitSignal));
            context->waitSignal = gcvNULL;
        }

        /* Destroy the context object. */
        gcmERR_BREAK(gcoOS_Free(context->os, Context));
    }
    while (gcvFALSE);

    /* Return result. */
    return gcmIS_SUCCESS(status) ? EGL_TRUE : EGL_FALSE;
}


/*******************************************************************************
**
** veglSetContext
**
** Set current context to the specified one.
**
** INPUT:
**
**    Context
**       Pointer to the context to be set as current.
**
**    Draw
**       Pointer to the surface to be used for drawing.
**
**    Read
**       Pointer to the surface to be used for reading.
**
**    Depth
**       Pointer to the surface to be used as depth buffer.
**
** OUTPUT:
**
**    Nothing.
*/

static gctBOOL
veglSetContext(
    void * Context,
    gcoSURF Draw,
    gcoSURF Read,
    gcoSURF Depth
    )
{
    gceSTATUS status;
    vgsCONTEXT_PTR context = (vgsCONTEXT_PTR) Context;

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_PARAMETERS,
        "%s(0x%08X, 0x%08X, 0x%08X, 0x%08X);\n",
        __FUNCTION__,
        Context, Draw, Read, Depth
        );

    do
    {
        vgsTHREADDATA_PTR thread;
        vgsIMAGE_PTR targetImage;
        vgsIMAGE_PTR maskImage;

        /* Get the thread information. */
        thread = vgfGetThreadData(gcvTRUE);

        if (thread == gcvNULL)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;
            break;
        }

#if gcdENABLE_VG
        gcmERR_BREAK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_VG));
#endif

        /* If there was a context already set before, release it. */
        if (thread->context != gcvNULL)
        {
            /* Unset the current target. */
            gcmERR_BREAK(gcoVG_UnsetTarget(
                thread->context->vg,
                thread->context->targetImage.surface
                ));

            /* Unset the current mask. */
            gcmERR_BREAK(gcoVG_UnsetMask(
                thread->context->vg,
                thread->context->maskImage.surface
                ));

            /* Reset the context. */
            thread->context = gcvNULL;
        }

        /* Reset the context. */
        if (context == gcvNULL)
        {
            gcmASSERT(Draw  == gcvNULL);
            gcmASSERT(Read  == gcvNULL);
            gcmASSERT(Depth == gcvNULL);

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Advance the API counter. */
        vgmADVANCE_SET_CONTEXT_COUNT(context);

        /* Draw surface cannot be NULL. */
        gcmASSERT(Draw != gcvNULL);

        /* Cannot have a context already set. */
        gcmASSERT(thread->context == gcvNULL);

        /* Set the context. */
        thread->context = context;

        /* Make shortcuts to the images. */
        targetImage = &context->targetImage;
        maskImage   = &context->maskImage;

        /* Determine whether the draw surface has changed. */
        if (targetImage->surface != Draw)
        {
            gcoSURF surface = targetImage->surface;
            /* Release previously initialized target image. */
            gcmERR_BREAK(vgfReleaseImage(
                context, targetImage
                ));

            if (surface != gcvNULL)
            {
                /* deference the target surface */
                gcmVERIFY_OK(gcoSURF_Destroy(surface));
            }

            /* Destroy previously allocated mask image. */
            gcmERR_BREAK(vgfReleaseImage(
                context, maskImage
                ));

            /* Destroy previously allocated temporary mask image. */
            gcmERR_BREAK(vgfReleaseImage(
                context, &context->tempMaskImage
                ));

            /* Initialize new target image. */
			if (Draw != gcvNULL)
			{
				gcmERR_BREAK(vgfInitializeImage(
					context, targetImage, Draw
					));
			}
			else
			{
				break;
			}

            gcmERR_BREAK(gcoSURF_ReferenceSurface(Draw));

            /* Create the mask surface. */
            gcmERR_BREAK(vgfCreateImage(
                context,
                VG_A_8,
                targetImage->size.width,
                targetImage->size.height,
                vgvIMAGE_QUALITY_ALL,
                &maskImage
                ));

            /* Set default mask. */
            vgfFillColor(
                context,
                maskImage,
                0, 0,
                targetImage->size.width,
                targetImage->size.height,
                vgvFLOATCOLOR0001,
                vgvBYTECOLOR0001,
                gcvFALSE
                );

            /* By default the mask is disabled. */
            gcmERR_BREAK(gcoVG_EnableMask(
                context->vg,
                gcvFALSE
                ));
        }

        /* Set new mask. */
        gcmERR_BREAK(gcoVG_SetMask(
            context->vg,
            context->maskImage.surface
            ));

        /* Assume conformance test for 64x64 render targets. */
        context->conformance
            =  (targetImage->size.width  == 64)
            && (targetImage->size.height == 64);

        /* For the conformance test force software tesselation
           for 64x64 surfaces. */
#if vgvFORCE_SW_TS
        context->useSoftwareTS = gcvTRUE;
#elif vgvFORCE_HW_TS
        context->useSoftwareTS = gcvFALSE;
#else
        context->useSoftwareTS = context->conformance;
#endif

        /* Print target info. */
        gcmTRACE_ZONE(
            gcvLEVEL_INFO,
            gcvZONE_CONTEXT,
            "%s(%d) DRAW SURFACE SET:\n"
            "  %dx%d, %s%s R%dG%dB%dA%d (%d bits per pixel), format=%d\n",
            __FUNCTION__, __LINE__,
            targetImage->size.width,
            targetImage->size.height,
            targetImage->surfaceFormat->linear
                ? "lRGBA" : "sRGBA",
            targetImage->surfaceFormat->premultiplied
                ? "_PRE" : "_NONPRE",
            targetImage->surfaceFormat->r.width & gcvCOMPONENT_WIDTHMASK,
            targetImage->surfaceFormat->g.width & gcvCOMPONENT_WIDTHMASK,
            targetImage->surfaceFormat->b.width & gcvCOMPONENT_WIDTHMASK,
            targetImage->surfaceFormat->a.width & gcvCOMPONENT_WIDTHMASK,
            targetImage->surfaceFormat->bitsPerPixel,
            targetImage->surfaceFormat->internalFormat
            );
    }
    while (gcvFALSE);

    /* Return result. */
    return gcmIS_SUCCESS(status);
}


/*******************************************************************************
**
** veglSetBuffer
**
** Set render target (multi-buffer support).
**
** INPUT:
**
**    Draw
**       Pointer to the surface to be used for drawing.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS
veglSetBuffer(
    gcoSURF Draw
    )
{
    gceSTATUS status = gcvSTATUS_GENERIC_IO;

    vgmENTERAPI(veglSetBuffer)
    {
        /* Draw surface cannot be NULL. */
        gcmASSERT(Draw != gcvNULL);

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_PARAMETERS,
            "%s(0x%08X);\n",
            __FUNCTION__,
            Draw
            );

        /* Same as the current? */
        if (Draw == Context->targetImage.surface)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Unlock the current buffer. */
        if (Context->targetImage.buffer != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Unlock(
                Context->targetImage.surface,
                Context->targetImage.buffer
                ));

            Context->targetImage.buffer = gcvNULL;
        }

        /* Lock the new buffer. */
        gcmERR_BREAK(gcoSURF_Lock(
            Draw,
            gcvNULL,
            (gctPOINTER *) &Context->targetImage.buffer
            ));

        if (Context->targetImage.surface != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Context->targetImage.surface));
        }
        /* Set the surface object. */
        Context->targetImage.surface = Draw;

        gcmVERIFY_OK(gcoSURF_ReferenceSurface(Draw));

    }
    vgmLEAVEAPI(veglSetBuffer);

    /* Return status. */
    return status;
}


/*******************************************************************************
**
** vgGetError
**
** vgGetError returns the oldest error code provided by an API call on the
** current context since the previous call to vgGetError on that context
** (or since the creation of the context). No error is indicated by a return
** value of 0 (VG_NO_ERROR). After the call, the error code is cleared to 0.
** If no context is current at the time vgGetError is called, the error code
** VG_NO_CONTEXT_ERROR is returned. Pending error codes on existing contexts
** are not affected by the call.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    VGErrorCode
**       OpenVL error code.
*/

VG_API_CALL VGErrorCode VG_API_ENTRY vgGetError(
    void
    )
{
    vgmENTERAPI(vgGetError)
    {
        /* Set the return error. */
        error = Context->error;

        /* Reset the current error. */
        Context->error = VG_NO_ERROR;
    }
    vgmLEAVEAPI(vgGetError);

    /* Return result. */
    return error;
}


/*******************************************************************************
**
** vgGetString
**
** The vgGetString function returns information about the OpenVG implemen-
** tation, including extension information. The values returned may vary
** according to the display (e.g., the EGLDisplay when using EGL) associated
** with the current context. If no context is current, vgGetString returns
** gcvNULL. The combination of VG_VENDOR and VG_RENDERER may be used together as
** a platform identifier by applications that wish to recognize a particular
** platform and adjust their algorithms based on prior knowledge of platform
** bugs and performance characteristics.
**
** If name is VG_VENDOR, the name of company responsible for this OpenVG
** implementation is returned. This name does not change from release to
** release.
**
** If name is VG_RENDERER, the name of the renderer is returned. This name is
** typically specific to a particular configuration of a hardware platform,
** and does not change from release to release.
**
** If name is VG_VERSION, the version number of the specification implemented
** by the renderer is returned as a string in the form
** major_number.minor_number. For this specification, "1.1" is returned.
**
** If name is VG_EXTENSIONS, a space-separated list of supported extensions to
** OpenVG is returned.
**
** For other values of name, gcvNULL is returned.
**
** INPUT:
**
**    Name
**       Specifies a symbolic constant, one of:
**          VG_VENDOR
**          VG_RENDERER
**          VG_VERSION
**          VG_EXTENSIONS
**
** OUTPUT:
**
**    const VGubyte *
**       String describing the current OpenVL connection.
*/

VG_API_CALL const VGubyte * VG_API_ENTRY vgGetString(
    VGStringID Name
    )
{
    VGubyte* pointer = gcvNULL;

    vgmENTERAPI(vgGetString)
    {
        switch (Name)
        {
        case VG_VENDOR:
            pointer = Context->chipInfo.vendor;
            break;

        case VG_RENDERER:
            pointer = Context->chipInfo.renderer;
            break;

        case VG_VERSION:
            pointer = Context->chipInfo.version;
            break;

        case VG_EXTENSIONS:
            pointer = Context->chipInfo.extensions;
            break;

        default:
            pointer = gcvNULL;
        }
    }
    vgmLEAVEAPI(vgGetString);

    return pointer;
}


/*******************************************************************************
**
** vgHardwareQuery
**
** The vgHardwareQuery function returns a value indicating whether a given
** setting of a property of a type given by Key is generally accelerated in
** hardware on the currently running OpenVG implementation.
**
** The return value will be one of the values VG_HARDWARE_ACCELERATED or
** VG_HARDWARE_UNACCELERATED, taken from the VGHardwareQueryResult enumeration.
** The legal values for the setting parameter depend on the value of the key
** parameter.
**
** INPUT:
**
**    Key
**       One of:
**          VG_IMAGE_FORMAT_QUERY
**          VG_PATH_DATATYPE_QUERY
**
**    Setting
**       Vallues from VGImageFormat or VGPathDatatype enumerations depending
**       on Key value.
**
** OUTPUT:
**
**    VGHardwareQueryResult
**       VG_HARDWARE_ACCELERATED or VG_HARDWARE_UNACCELERATED value.
*/

VG_API_CALL VGHardwareQueryResult VG_API_ENTRY vgHardwareQuery(
    VGHardwareQueryType Key,
    VGint Setting
    )
{
    VGHardwareQueryResult result = VG_HARDWARE_UNACCELERATED;

    vgmENTERAPI(vgHardwareQuery)
    {
        switch (Key)
        {
        case VG_IMAGE_FORMAT_QUERY:
            if (((Setting >= VG_sRGBX_8888) && (Setting <= VG_A_4) &&
                 (Setting != VG_BW_1) && (Setting != VG_A_1)) ||
                ((Setting >= VG_sXRGB_8888) && (Setting <= VG_lARGB_8888_PRE)) ||
                ((Setting >= VG_sBGRX_8888) && (Setting <= VG_lBGRA_8888_PRE)) ||
                ((Setting >= VG_sXBGR_8888) && (Setting <= VG_lABGR_8888_PRE)))
            {
                result = VG_HARDWARE_ACCELERATED;
            }
            else
            {
                vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
            }
            break;

        case VG_PATH_DATATYPE_QUERY:
            if ((Setting >= VG_PATH_DATATYPE_S_8) &&
                (Setting <= VG_PATH_DATATYPE_F))
            {
                result = VG_HARDWARE_ACCELERATED;
            }
            else
            {
                vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
            }
            break;

        default:
            vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
        }
    }
    vgmLEAVEAPI(vgHardwareQuery);

    return result;
}


/*******************************************************************************
**
** veglFlushContext/veglFlush/veglFinish
**
** Context flushing functions.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
** OUTPUT:
**
**    Nothing.
*/

static EGLBoolean
veglFlushContext(
    void * Context
    )
{
    vgFlush();
    return gcvTRUE;
}

static void
veglFlush(
    void
    )
{
    vgFlush();
}

static void
veglFinish(
    void
    )
{
    vgFinish();
}

/*******************************************************************************
**
** veglGetClientBuffer
**
** Returns the handle to surface of the specified VG image object.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Buffer
**       Valid VG image handle.
**
** OUTPUT:
**
**    Suface handle.
*/

static gcoSURF
veglGetClientBuffer(
    void * Context,
    gctPOINTER Buffer
    )
{
    gcoSURF surface;
    vgsCONTEXT_PTR context = (vgsCONTEXT_PTR) Context;

    do
    {
        vgsIMAGE_PTR image;

        /* Check the context pointer. */
        if (context == gcvNULL)
        {
            surface = gcvNULL;
            break;
        }

        /* Validate the path object. */
        if (!vgfVerifyUserObject(context, (VGHandle) Buffer, vgvOBJECTTYPE_IMAGE))
        {
            surface = gcvNULL;
            break;
        }

        /* Cast the object. */
        image = (vgsIMAGE_PTR) Buffer;

        /* Get the surface handle. */
        surface = image->surface;

        if (gcmIS_ERROR(gcoSURF_ReferenceSurface(image->surface)))
        {
            surface = gcvNULL;
            break;
        }
    }
    while (gcvFALSE);

    /* Return the handle. */
    return surface;
}

static EGLBoolean veglQueryHWVG(void)
{
    return EGL_TRUE;
}

/*******************************************************************************
**
** vgFlush
**
** The vgFlush function ensures that all outstanding requests on the current
** context will complete in finite time. vgFlush may return prior to the actual
** completion of all requests.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgFlush(
    void
    )
{
    vgmENTERAPI(vgFlush)
    {
        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_PARAMETERS,
            "%s();\n",
            __FUNCTION__
            );

        gcmVERIFY_OK(vgfFlushPipe(Context, gcvFALSE));

        vgmADVANCE_FRAME();
    }
    vgmLEAVEAPI(vgFlush);
}


/*******************************************************************************
**
** vgFinish
**
** The vgFinish function forces all outstanding requests on the current context
** to complete, returning only when the last request has completed.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgFinish(
    void
    )
{
    vgmENTERAPI(vgFinish)
    {
        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_PARAMETERS,
            "%s();\n",
            __FUNCTION__
            );

        gcmVERIFY_OK(vgfFlushPipe(Context, gcvTRUE));
    }
    vgmLEAVEAPI(vgFinish);
}

/* Dispatch table. */
#ifdef _WIN32
VG_API_CALL
#endif
veglDISPATCH
OpenVG_DISPATCH_TABLE =
{
    /* createContext            */  veglCreateContext,
    /* destroyContext           */  veglDestroyContext,
    /* setContext               */  veglSetContext,
    /* flushContext             */  veglFlushContext,

    /* flush                    */  veglFlush,
    /* finish                   */  veglFinish,

    /* setBuffer                */  veglSetBuffer,
    /* getClientBuffer          */  veglGetClientBuffer,

    /* createImageTexture       */  gcvNULL,
    /* createImageRenderbuffer  */  gcvNULL,
    /* createImageParentImage   */  gcvNULL,
    /* bindTexImage             */  gcvNULL,

    /* profiler                 */  gcvNULL,
    /* lookup                   */  gcvNULL,

    /* queryHWVG                */  veglQueryHWVG,

    /* renderThread             */  gcvNULL,
};


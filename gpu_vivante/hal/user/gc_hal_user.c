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
**  gcoHAL object for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HAL

/*******************************************************************************
***** Version Signature *******************************************************/

#define _gcmTXT2STR(t) #t
#define gcmTXT2STR(t) _gcmTXT2STR(t)
const char * _GAL_VERSION = "\n\0$VERSION$"
                            gcmTXT2STR(gcvVERSION_MAJOR) "."
                            gcmTXT2STR(gcvVERSION_MINOR) "."
                            gcmTXT2STR(gcvVERSION_PATCH) ":"
                            gcmTXT2STR(gcvVERSION_BUILD) "$\n";

/******************************************************************************\
******************************** gcoHAL API Code ********************************
\******************************************************************************/

/*******************************************************************************
**
**  _WorkaroundForFilterBlit
**
**  Workaround for the dirty region issue of filter blit.
**  It only exists for old GC300 before 2.0.2 (included).
**
**  INPUT:
**
**      gctHAL Hal
**          Pointer to HAL.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_WorkaroundForFilterBlit(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;

    gcoSURF    srcSurf = gcvNULL;
    gcsRECT    srcRect;

    gcoSURF    dstSurf = gcvNULL;
    gcsRECT    dstRect;

    do
    {
        gcmERR_BREAK(gcoSURF_Construct(
            gcvNULL,
            256,
            256,
            1,
            gcvSURF_BITMAP,
            gcvSURF_A8R8G8B8,
            gcvPOOL_DEFAULT,
            &srcSurf
            ));

        gcmERR_BREAK(gcoSURF_Construct(
            gcvNULL,
            256,
            256,
            1,
            gcvSURF_BITMAP,
            gcvSURF_A8R8G8B8,
            gcvPOOL_DEFAULT,
            &dstSurf
            ));

        srcRect.left   = 0;
        srcRect.top    = 0;
        srcRect.right  = 64;
        srcRect.bottom = 16;

        dstRect.left   = 0;
        dstRect.top    = 0;
        dstRect.right  = 128;
        dstRect.bottom = 32;

        gcmERR_BREAK(gcoSURF_FilterBlit(
            srcSurf,
            dstSurf,
            &srcRect,
            &dstRect,
            gcvNULL
            ));

        gcmERR_BREAK(gcoSURF_Destroy(srcSurf));
        srcSurf = gcvNULL;

        gcmERR_BREAK(gcoSURF_Destroy(dstSurf));
        dstSurf = gcvNULL;
    }
    while(gcvFALSE);

    if (gcmIS_ERROR(status)) {
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_HAL,
            "Failed to workarond for GC300.");

        if (srcSurf)
        {
            gcmVERIFY_OK(gcoSURF_Destroy(srcSurf));
        }

        if (dstSurf)
        {
            gcmVERIFY_OK(gcoSURF_Destroy(dstSurf));
        }
    }

    return status;
}

/*******************************************************************************
**
**  gcoHAL_Construct
**
**  Construct a new gcoHAL object.
**
**  INPUT:
**
**      gctPOINTER Context
**          Pointer to a context that can be used by the platform specific
**          functions.
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gcoHAL * Hal
**          Pointer to a variable that will hold the gcoHAL object pointer.
*/
gceSTATUS
gcoHAL_Construct(
    IN gctPOINTER Context,
    IN gcoOS Os,
    OUT gcoHAL * Hal
    )
{
    gctBOOL created = gcvFALSE;
    gcoHAL hal = gcPLS.hal;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;
    gctBOOL separated3D = gcvFALSE;
    gctBOOL separated2D = gcvFALSE;
    gcsHAL_INTERFACE iface;
    gctINT32 i;

    gcmHEADER_ARG("Context=0x%x OS=0x%x", Context, Os);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Hal != gcvNULL);

    if (hal == gcvNULL)
    {
        /* Create the gcoHAL object. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(struct _gcoHAL),
                                  &pointer));
        hal     = pointer;
        created = gcvTRUE;

        gcmONERROR(gcoOS_ZeroMemory(hal, gcmSIZEOF(struct _gcoHAL)));

        /* Initialize the object. */
        hal->object.type = gcvOBJ_HAL;

        /* Zero the gco2D, gco3D, and gcoDUMP objects. */
        hal->dump      = gcvNULL;
        hal->reference = gcvNULL;

        /* Initialize timeOut value */
        hal->timeOut     = gcdGPU_TIMEOUT;

        /* Query the kernel version number. */
        iface.command = gcvHAL_VERSION;
        gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                       IOCTL_GCHAL_INTERFACE,
                                       &iface, gcmSIZEOF(iface),
                                       &iface, gcmSIZEOF(iface)));

        /* Test if versions match. */
        if ((iface.u.Version.major != gcvVERSION_MAJOR)
        ||  (iface.u.Version.minor != gcvVERSION_MINOR)
        ||  (iface.u.Version.patch != gcvVERSION_PATCH)
        ||  (iface.u.Version.build != gcvVERSION_BUILD)
        )
        {
            gcmPRINT("HAL user version %d.%d.%d build %u %s %s",
                     gcvVERSION_MAJOR, gcvVERSION_MINOR, gcvVERSION_PATCH,
                     gcvVERSION_BUILD, gcvVERSION_DATE, gcvVERSION_TIME);
            gcmPRINT("HAL kernel version %d.%d.%d build %u",
                     iface.u.Version.major, iface.u.Version.minor,
                     iface.u.Version.patch, iface.u.Version.build);

            gcmONERROR(gcvSTATUS_VERSION_MISMATCH);
        }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HAL,
                  "HAL user version %d.%d.%d build %u %s %s",
                  gcvVERSION_MAJOR, gcvVERSION_MINOR, gcvVERSION_PATCH,
                  gcvVERSION_BUILD, gcvVERSION_DATE, gcvVERSION_TIME);
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_HAL,
                  "HAL kernel version %d.%d.%d build %u",
                  iface.u.Version.major, iface.u.Version.minor,
                  iface.u.Version.patch, iface.u.Version.build);
#endif

        /* Query chip info */
        iface.command = gcvHAL_CHIP_INFO;
        gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                       IOCTL_GCHAL_INTERFACE,
                                       &iface, gcmSIZEOF(iface),
                                       &iface, gcmSIZEOF(iface)));

        hal->chipCount = iface.u.ChipInfo.count;

        for (i = 0; i < hal->chipCount; i++)
        {
            hal->chipTypes[i] = iface.u.ChipInfo.types[i];

            switch (hal->chipTypes[i])
            {
            case gcvHARDWARE_3D:
                separated3D = gcvTRUE;
                break;
            case gcvHARDWARE_2D:
                separated2D = gcvTRUE;
                break;
            default:
                break;
            }
        }

        hal->separated3D2D = (separated3D && separated2D);

#if VIVANTE_PROFILER
        hal->profiler.enable = 0;
#endif

        /* Create reference. */
        gcmONERROR(gcoOS_AtomConstruct(Os, &hal->reference));
    }

    /* Increment the reference. */
    gcmONERROR(gcoOS_AtomIncrement(Os, hal->reference, gcvNULL));

    /* Return pointer to the gcoHAL object. */
    *Hal = hal;

    /* Success. */
    gcmFOOTER_ARG("*Hal=0x%x", *Hal);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if ((hal != gcvNULL) && created)
    {
        if (hal->reference != gcvNULL)
        {
            /* Destroy the reference. */
            gcmVERIFY_OK(gcoOS_AtomDestroy(Os, hal->reference));
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, hal));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_Destroy
**
**  Destroy an gcoHAL object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
**
*******************************************************************************/
gceSTATUS
gcoHAL_Destroy(
    IN gcoHAL Hal
    )
{
    gctINT reference;
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x", Hal);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hal, gcvOBJ_HAL);

    /* Decrease reference count. */
    gcmONERROR(gcoOS_AtomDecrement(gcvNULL, Hal->reference, &reference));
    if (reference > 1)
    {
        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    if (Hal->reference != gcvNULL)
    {
        /* Destroy the reference. */
        gcmVERIFY_OK(gcoOS_AtomDestroy(gcvNULL, Hal->reference));
        Hal->reference = gcvNULL;
    }

    /* Destroy the gcoDUMP object if any. */
    if (Hal->dump != gcvNULL)
    {
        gcmONERROR(gcoDUMP_Destroy(Hal->dump));
        Hal->dump = gcvNULL;
    }

    /* Free the gcoHAL object. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Hal));

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
**  gcoHAL_Call
**
**  Call the kernel HAL layer.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure that defines the command to
**          be executed by the kernel HAL layer.
**
**  OUTPUT:
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure that will be filled in by
**          the kernel HAL layer.
*/
gceSTATUS
gcoHAL_Call(
    IN gcoHAL Hal,
    IN OUT gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Interface=0x%x", Interface);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Interface != gcvNULL);

    /* Call kernel service. */
    status = gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        Interface, gcmSIZEOF(gcsHAL_INTERFACE),
        Interface, gcmSIZEOF(gcsHAL_INTERFACE)
        );

    if (gcmIS_SUCCESS(status))
    {
        status = Interface->status;
    }

    if (status == gcvSTATUS_OUT_OF_MEMORY)
    {
#if gcdENABLE_VG
        gceHARDWARE_TYPE currentType;
        gcmGETCURRENTHARDWARE(currentType);
        if (currentType == gcvHARDWARE_VG)
        {
            /* Commit any command queue to memory. */
            gcmONERROR(gcoVGHARDWARE_Commit(gcvNULL, gcvTRUE));
        }
        else
#endif
        {
            /* Commit any command queue to memory. */
            gcmONERROR(gcoHARDWARE_Commit());

            /* Stall the hardware. */
            gcmONERROR(gcoHARDWARE_Stall());
        }

        /* Retry kernel call again. */
        status = gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            Interface, gcmSIZEOF(gcsHAL_INTERFACE),
            Interface, gcmSIZEOF(gcsHAL_INTERFACE)
            );

        if (gcmIS_SUCCESS(status))
        {
            status = Interface->status;
        }
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_MapMemory
**
**  Map a range of video memory into the process space.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctPHYS_ADDR Physical
**          Physical address of video memory to map.
**
**      gctSIZE_T NumberOfBytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      gctPOINTER * Logical
**          Pointer to a variable that will hold the logical address of the
**          mapped video memory.
*/
gceSTATUS
gcoHAL_MapMemory(
    IN gcoHAL Hal,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    OUT gctPOINTER * Logical
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Physical=0x%x NumberOfBytes=%lu",
                  Physical, NumberOfBytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to map the memory. */
    iface.command              = gcvHAL_MAP_MEMORY;
    iface.u.MapMemory.physical = Physical;
    iface.u.MapMemory.bytes    = NumberOfBytes;
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    /* Return logical address. */
    *Logical = iface.u.MapMemory.logical;

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x", *Logical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_UnmapMemory
**
**  Unmap a range of video memory from the process space.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctPHYS_ADDR Physical
**          Physical address of video memory to unmap.
**
**      gctSIZE_T NumberOfBytes
**          Number of bytes to unmap.
**
**      gctPOINTER Logical
**          Logical address of the video memory to unmap.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_UnmapMemory(
    IN gcoHAL Hal,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Physical=0x%x NumberOfBytes=%lu Logical=0x%x",
                  Physical, NumberOfBytes, Logical);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to unmap the memory. */
    iface.command                = gcvHAL_UNMAP_MEMORY;
    iface.u.UnmapMemory.physical = Physical;
    iface.u.UnmapMemory.bytes    = NumberOfBytes;
    iface.u.UnmapMemory.logical  = Logical;
    status = gcoHAL_Call(gcvNULL, &iface);

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_ScheduleUnmapMemory
**
**  Schedule an unmap of a buffer mapped through its physical address.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctPHYS_ADDR Physical
**          Physical address of video memory to unmap.
**
**      gctSIZE_T NumberOfBytes
**          Number of bytes to unmap.
**
**      gctPOINTER Logical
**          Logical address of the video memory to unmap.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_ScheduleUnmapMemory(
    IN gcoHAL Hal,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Physical=0x%x NumberOfBytes=%lu Logical=0x%x",
                  Physical, NumberOfBytes, Logical);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(NumberOfBytes > 0);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Schedule an event to unmap the user memory. */
    iface.command                = gcvHAL_UNMAP_MEMORY;
    iface.u.UnmapMemory.bytes    = NumberOfBytes;
    iface.u.UnmapMemory.physical = Physical;
    iface.u.UnmapMemory.logical  = Logical;
    status = gcoHAL_ScheduleEvent(gcvNULL, &iface);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_ScheduleUnmapUserMemory
**
**  Schedule an unmap of a user buffer using event mechanism.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctPOINTER Info
**          Information record returned by gcoOS_MapUserMemory.
**
**      gctSIZE_T Size
**          Size in bytes of the memory to unlock.
**
**      gctUINT32_PTR Address
**          The address returned by gcoOS_MapUserMemory.
**
**      gctPOINTER Memory
**          Pointer to memory to unlock.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_ScheduleUnmapUserMemory(
    IN gcoHAL Hal,
    IN gctPOINTER Info,
    IN gctSIZE_T Size,
    IN gctUINT32 Address,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Info=0x%x Size=%lu Address=0x%08x Memory=0x%x",
                  Info, Size, Address, Memory);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Size > 0);
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    /* Schedule an event to unmap the user memory. */
    iface.command = gcvHAL_UNMAP_USER_MEMORY;
    iface.u.UnmapUserMemory.info    = Info;
    iface.u.UnmapUserMemory.size    = Size;
    iface.u.UnmapUserMemory.address = Address;
    iface.u.UnmapUserMemory.memory  = Memory;
    status = gcoHAL_ScheduleEvent(gcvNULL, &iface);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_Commit
**
**  Commit the current command buffer to hardware and optionally wait until the
**  hardware is finished.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gctBOOL Stall
**          gcvTRUE if the thread needs to wait until the hardware has finished
**          executing the committed command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_Commit(
    IN gcoHAL Hal,
    IN gctBOOL Stall
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentHW;
#endif

    gcmHEADER_ARG("Stall=%d", Stall);

#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentHW);
    if (currentHW == gcvHARDWARE_VG)
    {
    	/* Commit the command queue. */
	    gcmONERROR(gcoVGHARDWARE_Commit(gcvNULL, Stall));

    }
    else
#endif
    {
        /* Commit the command buffer to hardware. */
        gcmONERROR(gcoHARDWARE_Commit());

        if (Stall)
        {
            /* Stall the hardware. */
            gcmONERROR(gcoHARDWARE_Stall());
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_Get2DEngine
**
**  Get the pointer to the gco2D object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gco2D * Engine
**          Pointer to a variable receiving the gco2D object pointer.
*/
gceSTATUS
gcoHAL_Get2DEngine(
    IN gcoHAL Hal,
    OUT gco2D * Engine
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    gcmONERROR(gcoOS_GetTLS(&tls));

    if (tls->engine2D == gcvNULL)
    {
        gceCHIPMODEL chipModel;
        gctUINT32 chipRevision;

        /* Construct the gco2D object. */
        gcmONERROR(gco2D_Construct(gcvNULL, &tls->engine2D));

        /* Workaround for the old 2D chip. */
        gcmONERROR(
            gcoHARDWARE_QueryChipIdentity(&chipModel,
                                          &chipRevision,
                                          gcvNULL,
                                          gcvNULL,
                                          gcvNULL,
                                          gcvNULL));

        if (gcoHARDWARE_Is2DAvailable() && (chipRevision <= 0x00004400))
        {
            _WorkaroundForFilterBlit(gcvNULL);
        }
    }

    /* Return pointer to the gco2D object. */
    *Engine = tls->engine2D;

    /* Success. */
    gcmFOOTER_ARG("*Engine=0x%x", *Engine);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !defined(VIVANTE_NO_3D)
/*******************************************************************************
**
**  gcoHAL_GetVGEngine
**
**  Get the pointer to the gcoVG object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoVG * Engine
**          Pointer to a variable receiving the gcoVG object pointer.
*/
gceSTATUS
gcoHAL_GetVGEngine(
    IN gcoHAL Hal,
    OUT gcoVG * Engine
    )
{
#if gcdENABLE_VG
    gceSTATUS status;
    gcsTLS_PTR __tls__;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    gcmONERROR(gcoOS_GetTLS(&__tls__));

    if (__tls__->engineVG == gcvNULL)
    {
        /* Construct the gcoVG object. */
        gcmONERROR(gcoVG_Construct(gcPLS.hal, &__tls__->engineVG));
    }

    /* Return pointer to the gcoVG object. */
    *Engine = __tls__->engineVG;

    /* Success. */
    gcmFOOTER_ARG("*Engine=0x%x", *Engine);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

/*******************************************************************************
**
**  gcoHAL_Get3DEngine
**
**  Get the pointer to the gco3D object.
**  Create the object, if it doesn't exist.
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
gcoHAL_Get3DEngine(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

    gcmONERROR(gcoOS_GetTLS(&tls));

    if (tls->engine3D == gcvNULL)
    {
        /* Construct the gco3D object. */
        gcmONERROR(gco3D_Construct(gcvNULL, &tls->engine3D));
    }

    /* Return pointer to the gco3D object. */
    *Engine = tls->engine3D;

    /* Success. */
    gcmFOOTER_ARG("*Engine=0x%x", *Engine);

	return gcvSTATUS_OK;

	OnError:
		 /* Return the status. */
		 gcmFOOTER();
		 return status;
	}

	/**********************************************************************
	*********
	**
	**  gcoHAL_Query3DEngine
	**
	**  Get the pointer to the gco3D object.
	**
	**  INPUT:
	**
	** 	 gcoHAL Hal
	** 		 Pointer to an gcoHAL object.
	**
	**  OUTPUT:
	**
	** 	 gco3D * Engine
	** 		 Pointer to a variable receiving the gco3D object pointer.
	*/
	gceSTATUS
	gcoHAL_Query3DEngine(
		 IN gcoHAL Hal,
		 OUT gco3D * Engine
		 )
	{
		 gceSTATUS status;
		 gcsTLS_PTR tls;

		 gcmHEADER();

		 /* Verify the arguments. */
		 gcmDEBUG_VERIFY_ARGUMENT(Engine != gcvNULL);

		 gcmONERROR(gcoOS_GetTLS(&tls));

		 /* Return pointer to the gco3D object. */
		 *Engine = tls->engine3D;

		 /* Success. */
		 gcmFOOTER_ARG("*Engine=0x%x", *Engine);
		 return gcvSTATUS_OK;

	OnError:
		 /* Return the status. */
		 gcmFOOTER();
		 return status;
	}

	/**********************************************************************
	*********
	**
	**  gcoHAL_Set3DEngine
	**
	**  Set the pointer as the gco3D object.
	**
	**  INPUT:
	**
	** 	 gcoHAL Hal
	** 		 Pointer to an gcoHAL object.
	**
	** 	 gco3D Engine
	** 		 The gco3D object that needs to be set.
	**  OUTPUT:
	**
	** 	 nothing.
	*/
	gceSTATUS
	gcoHAL_Set3DEngine(
		 IN gcoHAL Hal,
		 IN gco3D Engine
		 )
	{
		 gceSTATUS status;
		 gcsTLS_PTR tls;

		 gcmHEADER();

		 gcmONERROR(gcoOS_GetTLS(&tls));

		 /* Set the gco3D object. */
		 tls->engine3D = Engine;

		 /* Success. */
		 gcmFOOTER_NO();
		 return gcvSTATUS_OK;

	OnError:
		 /* Return the status. */
		 gcmFOOTER();
		 return status;
	}

	/**********************************************************************
	*********
	**
	**  gcoHAL_Get3DHardware
	**
	**  Get the pointer to the gcoHARDWARE object.
	**
	**  INPUT:
	**
	** 	 gcoHAL Hal
	** 		 Pointer to an gcoHAL object.
	**
	**  OUTPUT:
	**
	** 	 gco3D * Hardware
	** 		 Pointer to a variable receiving the gcoHARDWARE object pointer.
	*/
	gceSTATUS
	gcoHAL_Get3DHardware(
		 IN gcoHAL Hal,
		 OUT gcoHARDWARE * Hardware
		 )
	{
		 gceSTATUS status;
		 gcsTLS_PTR tls;

		 gcmHEADER();

		 /* Verify the arguments. */
		 gcmDEBUG_VERIFY_ARGUMENT(Hardware != gcvNULL);

		 gcmONERROR(gcoOS_GetTLS(&tls));

		 /* Return pointer to the gco3D object. */
		 *Hardware = tls->hardware;

		 /* Success. */
		 gcmFOOTER_ARG("*Hardware=0x%x", *Hardware);
		 return gcvSTATUS_OK;

	OnError:
		 /* Return the status. */
		 gcmFOOTER();
		 return status;
	}

	/**********************************************************************
	*********
	**
	**  gcoHAL_Set3DHardware
	**
	**  Set the gcoHARDWARE object.
	**
	**  INPUT:
	**
	** 	 gcoHAL Hal
	** 		 Pointer to an gcoHAL object.
	**
	** 	 gcoHARDWARE Hardware
	** 		 The gcoHARDWARE object.
	**
	**  OUTPUT:
	**
	*/
	gceSTATUS
	gcoHAL_Set3DHardware(
		 IN gcoHAL Hal,
		 IN gcoHARDWARE Hardware
		 )
	{
		 gceSTATUS status;
		 gcsTLS_PTR tls;

		 gcmHEADER();

		 gcmONERROR(gcoOS_GetTLS(&tls));

	 /* Return pointer to the gco3D object. */
		 tls->hardware = Hardware;

		 /* Success. */
		 gcmFOOTER_NO();
	    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHAL_GetDump
**
**  Get the pointer to the gcoDUMP object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoDUMP * Dump
**          Pointer to a variable receiving the gcoDUMP object pointer.
*/
gceSTATUS
gcoHAL_GetDump(
    IN gcoHAL Hal,
    OUT gcoDUMP * Dump
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Dump != gcvNULL);

    if (gcPLS.hal->dump == gcvNULL)
    {
        /* Construct the gcoDUMP object. */
        gcmONERROR(gcoDUMP_Construct(gcvNULL, gcvNULL, &gcPLS.hal->dump));
    }

    /* Return pointer to the gcoDUMP object. */
    *Dump = gcPLS.hal->dump;

    /* Success. */
    gcmFOOTER_ARG("*Dump=0x%x", *Dump);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* Schedule an event. */
gceSTATUS
gcoHAL_ScheduleEvent(
    IN gcoHAL Hal,
    IN OUT gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Interface=0x%x", Interface);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Interface != gcvNULL);

    /* Send event to hardware layer. */
    status = gcoHARDWARE_CallEvent(Interface);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_SetPowerManagementState
**
**  Set GPU to a specified power state.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceCHIPPOWERSTATE State
**          Power State.
**
*/
gceSTATUS
gcoHAL_SetPowerManagementState(
    IN gcoHAL Hal,
    IN gceCHIPPOWERSTATE State
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("State=%d", State);

    /* Call kernel API to set power management state. */
    iface.command                    = gcvHAL_SET_POWER_MANAGEMENT_STATE;
    iface.u.SetPowerManagement.state = State;
    status = gcoHAL_Call(gcvNULL, &iface);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_Compact(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;

    gcmHEADER();

    status = gcoOS_Compact(gcvNULL);

    gcmFOOTER();
    return status;
}

#if VIVANTE_PROFILER /*gcdENABLE_PROFILING*/
gceSTATUS
gcoHAL_ProfileStart(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;

    gcmHEADER();

    status = gcoOS_ProfileStart(gcvNULL);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_ProfileEnd(
    IN gcoHAL Hal,
    IN gctCONST_STRING Title
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Title=%s", Title);

    status = gcoOS_ProfileEnd(gcvNULL, Title);

    gcmFOOTER();
    return status;
}
#else
gceSTATUS
gcoHAL_ProfileStart(
    IN gcoHAL Hal
    )
{
    gcmHEADER();
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

gceSTATUS
gcoHAL_ProfileEnd(
    IN gcoHAL Hal,
    IN gctCONST_STRING Title
    )
{
    gcmHEADER_ARG("Title=%s", Title);
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}
#endif

#ifndef VIVANTE_NO_3D
/* Enable or disable depth only mode. */
gceSTATUS
gcoHAL_SetDepthOnly(
    IN gcoHAL Hal,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Enable=%d", Enable);

    /* Send the request to hardware layer. */
    status = gcoHARDWARE_SetDepthOnly(Enable);

    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

gceSTATUS
gcoHAL_DestroySurface(
    IN gcoHAL Hal,
    IN gcoSURF Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%0x", Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Destroy the surface using this gcoHAL. */
    gcmONERROR(gcoSURF_Destroy(Surface));

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
**  gcoHAL_SetTimer
**
**  Set Timer to start/stop.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gctUINT32 Timer
**          Which Timer to select for being started or stopped.
**
**      gctBOOL Start
**          Start (gcvTRUE) or Stop (gcvFALSE) request.
**
*/
gceSTATUS
gcoHAL_SetTimer(
    IN gcoHAL Hal,
    IN gctUINT32 Timer,
    IN gctBOOL Start
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Timer=0x%0x Start=0x%0x", Timer, Start);

    /* Call kernel API to map the memory. */
    iface.command             = gcvHAL_TIMESTAMP;
    iface.u.TimeStamp.timer   = Timer;
    iface.u.TimeStamp.request = Start;
    gcmONERROR(gcoHAL_ScheduleEvent(gcvNULL, &iface));

    /* Commit exisiting command buffer, to execute any scheduled timer events. */
    gcmONERROR(gcoHAL_Commit(gcvNULL, gcvFALSE));

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
**  gcoHAL_GetTimerTime
**
**  Get Time delta from a Timer in microseconds.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gctUINT32 Timer
**          Which Timer to select.
**
**      gctINT32_PTR TimeDelta
**          Pointer to a variable, which will store the result.
**
*/
gceSTATUS
gcoHAL_GetTimerTime(
    IN gcoHAL Hal,
    IN gctUINT32 Timer,
    OUT gctINT32_PTR TimeDelta
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Timer=0x%0x", Timer);

    if (TimeDelta == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Commit exisiting command buffer, to execute any scheduled timer events. */
    gcmONERROR(gcoHAL_Commit(gcvNULL, gcvTRUE));

    /* Call kernel API to map the memory. */
    iface.command             = gcvHAL_TIMESTAMP;
    iface.u.TimeStamp.timer   = Timer;
    iface.u.TimeStamp.request = 2;
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    /* Return time delta value. */
    *TimeDelta = iface.u.TimeStamp.timeDelta;

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
**  gcoHAL_SetTimeOut
**
**  set timeout value.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gctUINT32 timeOut
**          Which timeOut value to set.
**
*/
gceSTATUS
gcoHAL_SetTimeOut(
    IN gcoHAL Hal,
	IN gctUINT32 timeOut
    )
{
#if gcdGPU_TIMEOUT
    gcsHAL_INTERFACE iface;
    gceSTATUS status;
#endif
	gcmHEADER_ARG("Hal=0x%x timeOut=%d", Hal, timeOut);

#if gcdGPU_TIMEOUT
    Hal->timeOut = timeOut;
    iface.command = gcvHAL_SET_TIMEOUT;
    iface.u.SetTimeOut.timeOut = timeOut;
    status = gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                 &iface, gcmSIZEOF(iface),
                                 &iface, gcmSIZEOF(iface));
    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }
#endif
	gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoHAL_AddFrameDB(
    void
    )
{
#if gcdFRAME_DB
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;

    gcmHEADER();

    /* Test if we have outgrown our frame count. */
    if (gcPLS.hal->frameDBIndex >= gcdFRAME_DB)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /* Get frame information. */
	ioctl.command                  = gcvHAL_GET_FRAME_INFO;
	ioctl.u.GetFrameInfo.frameInfo =
	    &gcPLS.hal->frameDB[gcPLS.hal->frameDBIndex];
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &ioctl, gcmSIZEOF(ioctl),
                                   &ioctl, gcmSIZEOF(ioctl)));

    /* Increment frame index. */
    gcPLS.hal->frameDBIndex += 1;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

#if gcdFRAME_DB
static gceSTATUS
gcoHAL_PrintFrameDB(
    gctFILE File,
    gctCONST_STRING Format,
    ...
    )
{
    gctCHAR buffer[256];
    gctUINT offset = 0;
    gceSTATUS status;

    gcmHEADER_ARG("File=0x%x Format=0x%x", File, Format);

    /* Format the string. */
    gcmONERROR(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                   &offset,
                                   Format, (gctPOINTER) (&Format + 1)));

    if (File != gcvNULL)
    {
        /* Print to file. */
        gcmONERROR(gcoOS_Write(gcvNULL, File, offset, buffer));
    }
    else
    {
        /* Print to debugger. */
        gcmPRINT("%s", buffer);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#define gcmGET_COUNTER(record, field, usePrev) \
    ( record->field \
    - (usePrev ? record[-1].field : 0) \
    )

gceSTATUS
gcoHAL_DumpFrameDB(
    gctCONST_STRING Filename OPTIONAL
    )
{
#if gcdFRAME_DB
    gctINT i, j;
    gctFILE file = gcvNULL;
    gcsHAL_FRAME_INFO * record;
    gceSTATUS status;

    gcmHEADER_ARG("Filename=%s", gcmOPT_STRING(Filename));

    if (Filename != gcvNULL)
    {
        /* Open the filename. Silently ignore any errors. */
        gcoOS_Open(gcvNULL, Filename, gcvFILE_CREATETEXT, &file);
    }

    /* Loop through all records. */
    for (i = 0, record = gcPLS.hal->frameDB;
         i < gcPLS.hal->frameDBIndex;
         ++i, ++record)
    {
        const gctBOOL usePrev = (i > 0) && !gcdFRAME_DB_RESET;

        /* Print frame info. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "Frame %d: tick=%llu delta=%llu\n",
            i + 1,
            record->ticks,
            (i == 0) ? 0 : record->ticks - record[-1].ticks));

        for (j = 0; (j < 8) && (record->mcCycles[j] > 0); ++j)
        {
            /* Print HW cycles. */
            gcmONERROR(gcoHAL_PrintFrameDB(
                file,
                "  HW[%d]: cycles=%u idle=%u mcCycles=%u\n",
                j,
                record->cycles[j],
                record->idleCycles[j],
                gcmGET_COUNTER(record, mcCycles[j], (i > 0))));
        }

        for (j = 0; (j < 8) && (record->mcCycles[j] > 0); ++j)
        {
            /* Print HW bandwidth. */
            gcmONERROR(gcoHAL_PrintFrameDB(
                file,
                "  BW[%d]: readBytes=%u/%llu writeBytes=%u/%llu\n",
                j,
                record->readRequests[j],
                (gctUINT64) record->readBytes8[j] << 3,
                record->writeRequests[j],
                (gctUINT64) record->writeBytes8[j] << 3));
        }

        /* Print shader HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  SH: cycles=%u vs=%u/%u ps=%u/%u\n",
            gcmGET_COUNTER(record, shaderCycles, usePrev),
            gcmGET_COUNTER(record, vsInstructionCount, usePrev),
            gcmGET_COUNTER(record, vsTextureCount, usePrev),
            gcmGET_COUNTER(record, psInstructionCount, usePrev),
            gcmGET_COUNTER(record, psTextureCount, usePrev)));

        /* Print texture HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  TX: bi=%u tri=%u bytes=%llu hit=%u miss=%u\n",
            gcmGET_COUNTER(record, bilinearRequests, usePrev),
            gcmGET_COUNTER(record, trilinearRequests, usePrev),
            (gctUINT64) gcmGET_COUNTER(record, txBytes8, usePrev) << 3,
            gcmGET_COUNTER(record, txHitCount, usePrev),
            gcmGET_COUNTER(record, txMissCount, usePrev)));

        /* Print primitive assembly HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  PA: vertex=%u primitives=%u/-%u/@%u/#%u/%u\n",
            gcmGET_COUNTER(record, vertexCount, usePrev),
            gcmGET_COUNTER(record, primitiveCount, usePrev),
            gcmGET_COUNTER(record, rejectedPrimitives, usePrev),
            gcmGET_COUNTER(record, culledPrimitives, usePrev),
            gcmGET_COUNTER(record, clippedPrimitives, usePrev),
            gcmGET_COUNTER(record, outPrimitives, usePrev)));

        /* Print rasterizer HW counters. */
        gcmONERROR(gcoHAL_PrintFrameDB(
            file,
            "  RA: primitives=%u quads=-%u/%u/%u pixels=%u\n",
            gcmGET_COUNTER(record, inPrimitives, usePrev),
            gcmGET_COUNTER(record, culledQuadCount, usePrev),
            gcmGET_COUNTER(record, totalQuadCount, usePrev),
            gcmGET_COUNTER(record, quadCount, usePrev),
            gcmGET_COUNTER(record, totalPixelCount, usePrev)));

        for (j = 0; (j < 8) && (record->mcCycles[j] > 0); ++j)
        {
            /* Print per-pipe pixel count. */
            gcmONERROR(gcoHAL_PrintFrameDB(
                file,
                "  PE[%d]: color=-%u/+%u depth=-%u/+%u\n",
                j,
                gcmGET_COUNTER(record, colorKilled[j], usePrev),
                gcmGET_COUNTER(record, colorDrawn[j], usePrev),
                gcmGET_COUNTER(record, depthKilled[j], usePrev),
                gcmGET_COUNTER(record, depthDrawn[j], usePrev)));
        }
    }

    /* Reset frame index. */
    gcPLS.hal->frameDBIndex = 0;

    if (file != gcvNULL)
    {
        /* Close the file. */
        gcmONERROR(gcoOS_Close(gcvNULL, file));
        file = gcvNULL;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (file != gcvNULL)
    {
        /* Close the file. */
        gcmONERROR(gcoOS_Close(gcvNULL, file));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}

/*******************************************************************************
**
**  gcoHAL_GetSharedInfo
**
**  Get private data of a process by its pid.
**
**  INPUT:
**
**      gctUINT32 Pid
**          Pid of the process.
**
**      gctUINT32 DataId
**          Id of the data.
**
**      gctUINT8_PTR Data
**          Pointer to a buffer. Caller must allocate enough buffer.
**
**      gctUINT32 Bytes
**          Bytes of Data needed.
**
**      gcuVIDMEM_NODE_PTR Node
**          Node with which data is associated with.
**
**      gctUINT8_PTR NodeData,
**          Data stored in node.
**
**      gceNODE_SHARED_INFO_TYPE SharedInfoType
**          Type of data stored
**
**  Note:
**
**  Private Data for each process is a simple machnisam, by which different
**  processes can exchange a mount of information, indentified by pid and
**  data id.
**
**  Basic idea:
**  Kernel allocates a memory region for each type of data for one process.
**  Each process can update the data it stores in kernel. If other process want
**  this information, it must know the process id of the data's owner.
**
**  Restrict:
**  1) Processes must handle synchronization by themselves
**  2) Kernel will NOT notify the reader of the data if the owner of data quits.
**  3) Data size is determined by the first time it is send to kernel, and can't
**     be changed(for now).
**
*/
gceSTATUS
gcoHAL_GetSharedInfo(
    IN gctUINT32 Pid,
    IN gctUINT32 DataId,
    OUT gctUINT8_PTR Data,
    IN gctSIZE_T Bytes,
    IN gcuVIDMEM_NODE_PTR Node,
    OUT gctUINT8_PTR NodeData,
    IN gceVIDMEM_NODE_SHARED_INFO_TYPE SharedInfoType
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Pid=%i DataId=%i Data=0x%0x Bytes=%i Node=0x%0x NodeData=0x%0x SharedInfoType=%d",
                  Pid, DataId, Data, Bytes, Node, NodeData, SharedInfoType);

    iface.command             = gcvHAL_GET_SHARED_INFO;
    iface.u.GetSharedInfo.pid   = Pid;
    iface.u.GetSharedInfo.dataId = DataId;
    iface.u.GetSharedInfo.data  = Data;
    iface.u.GetSharedInfo.size  = Bytes;
    iface.u.GetSharedInfo.node  = Node;
    iface.u.GetSharedInfo.nodeData  = NodeData;
    iface.u.GetSharedInfo.infoType  = SharedInfoType;

    status = gcoHAL_Call(gcvNULL, &iface);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_SetSharedInfo
**
**  Set shared data of a process.
**
**  INPUT:
**
**      gctUINT32 DataId
**          Id of the data.
**
**      gctUINT8_PTR Data
**          Pointer to a buffer.
**
**      gctUINT32 Bytes
**          Bytes of Data needed
**
**      gcuVIDMEM_NODE_PTR Node
**          Node with which data will be associated with.
**
**      gctUINT8_PTR NodeData,
**          Data to be stored in node.
**
**      gceNODE_SHARED_INFO_TYPE SharedInfoType
**          Type of data to be stored
*/
gceSTATUS
gcoHAL_SetSharedInfo(
    IN gctUINT32 DataId,
    IN gctUINT8_PTR Data,
    IN gctSIZE_T Bytes,
    IN gcuVIDMEM_NODE_PTR Node,
    IN gctUINT8_PTR NodeData,
    IN gceVIDMEM_NODE_SHARED_INFO_TYPE SharedInfoType
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("DataId=%i Data=0x%0x Bytes=%i Node=%x NodeData=%x", DataId, Data, Bytes, Node, NodeData);

    iface.command             = gcvHAL_SET_SHARED_INFO;
    iface.u.SetSharedInfo.data  = Data;
    iface.u.SetSharedInfo.size  = Bytes;
    iface.u.SetSharedInfo.dataId   = DataId;
    iface.u.SetSharedInfo.node  = Node;
    iface.u.SetSharedInfo.nodeData  = NodeData;
    iface.u.SetSharedInfo.infoType  = SharedInfoType;

    status = gcoHAL_Call(gcvNULL, &iface);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
 *  Generic record allocation using preallocated containers.
 ******************************************************************************/

gceSTATUS
gcsCONTAINER_Construct(
    IN gcsCONTAINER_PTR Container,
    gctUINT RecordsPerContainer,
    gctUINT RecordSize
    )
{
    gcmHEADER_ARG("Container=0x%x RecordsPerContainer=%d RecordSize=%d",
        Container, RecordsPerContainer, RecordSize);

    gcmDEBUG_VERIFY_ARGUMENT(Container           != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(RecordsPerContainer != 0);
    gcmDEBUG_VERIFY_ARGUMENT(RecordSize          != 0);

    RecordSize += gcmSIZEOF(gcsCONTAINER_RECORD);

    Container->containerSize
        = gcmSIZEOF(gcsCONTAINER_LINK)
        + RecordSize * RecordsPerContainer;

    Container->recordSize  = RecordSize;
    Container->recordCount = RecordsPerContainer;

    Container->containers = gcvNULL;

    Container->freeList.next  = &Container->freeList;
    Container->freeList.prev  = &Container->freeList;
    Container->allocList.next = &Container->allocList;
    Container->allocList.prev = &Container->allocList;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcsCONTAINER_Destroy(
    IN gcsCONTAINER_PTR Container
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCONTAINER_LINK_PTR container;

    gcmHEADER_ARG("Container=0x%x", Container);
    gcmDEBUG_VERIFY_ARGUMENT(Container != gcvNULL);

    while (Container->containers != gcvNULL)
    {
        /* Get the current container. */
        container = Container->containers;

        /* Remove it from the list. */
        Container->containers = container->next;

        /* Free the current container. */
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, container));
    }

    Container->containerSize = 0;
    Container->recordSize    = 0;
    Container->recordCount   = 0;

    Container->freeList.next  = &Container->freeList;
    Container->freeList.prev  = &Container->freeList;
    Container->allocList.next = &Container->allocList;
    Container->allocList.prev = &Container->allocList;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcsCONTAINER_AllocateRecord(
    IN gcsCONTAINER_PTR Container,
    OUT gctPOINTER * Record
    )
{
    gceSTATUS status;
    gcsCONTAINER_RECORD_PTR currRecord;
    gcsCONTAINER_LINK_PTR container;
    gctUINT i;

    /* Are there free nodes available? */
    if (Container->freeList.next == &Container->freeList)
    {
        /* No, allocate an array of nodes. */
        gcmONERROR(gcoOS_Allocate(
            gcvNULL, Container->containerSize, (gctPOINTER *) &container
            ));

        /* Add to the list of containers. */
        container->next = Container->containers;
        Container->containers = container;

        /* Determine the pointer to the first layer item. */
        currRecord = (gcsCONTAINER_RECORD_PTR) (container + 1);

        /* Initialize free list. */
        for (i = 0; i < Container->recordCount; i += 1)
        {
            currRecord->prev = Container->freeList.prev;
            currRecord->next = &Container->freeList;
            Container->freeList.prev->next = currRecord;
            Container->freeList.prev       = currRecord;

            currRecord = (gcsCONTAINER_RECORD_PTR)
                ((gctUINT8_PTR) currRecord + Container->recordSize);
        }
    }

    /* Get the first free record. */
    currRecord = Container->freeList.next;

    /* Remove the record from free list. */
    currRecord->prev->next = currRecord->next;
    currRecord->next->prev = currRecord->prev;

    /* Add to allocated list. */
    currRecord->prev = &Container->allocList;
    currRecord->next = Container->allocList.next;
    Container->allocList.next->prev = currRecord;
    Container->allocList.next       = currRecord;

    /* Set return pointer. */
    * Record = currRecord + 1;

    /* Success. */
    return gcvSTATUS_OK;

    /* Return error. */
OnError:
    return status;
}

gceSTATUS
gcsCONTAINER_FreeRecord(
    IN gcsCONTAINER_PTR Container,
    IN gctPOINTER Record
    )
{
    gcsCONTAINER_RECORD_PTR currRecord;

    /* Get the record pointer */
    currRecord = (gcsCONTAINER_RECORD_PTR) Record - 1;

    /* Remove from allocated list. */
    currRecord->prev->next = currRecord->next;
    currRecord->next->prev = currRecord->prev;

    /* Add to free list. */
    currRecord->prev = &Container->freeList;
    currRecord->next = Container->freeList.next;
    Container->freeList.next->prev = currRecord;
    Container->freeList.next       = currRecord;

    /* Success. */
    return gcvSTATUS_OK;
}

gceSTATUS
gcsCONTAINER_FreeAll(
    IN gcsCONTAINER_PTR Container
    )
{
    if (Container->allocList.next != &Container->allocList)
    {
        /* Move the entire allocated list to free list. */
        Container->allocList.prev->next = Container->freeList.next;
        Container->freeList.next->prev = Container->allocList.prev;
        Container->allocList.next->prev = &Container->freeList;
        Container->freeList.next = Container->allocList.next;

        /* Reset allocated list. */
        Container->allocList.prev = &Container->allocList;
        Container->allocList.next = &Container->allocList;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

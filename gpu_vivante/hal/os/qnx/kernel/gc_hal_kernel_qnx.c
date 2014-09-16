/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/




#include "gc_hal_kernel_qnx.h"

#define _GC_OBJ_ZONE    gcvZONE_KERNEL

/******************************************************************************\
******************************* gckKERNEL API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**  gckKERNEL_QueryVideoMemory
**
**  Query the amount of video memory.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**  OUTPUT:
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure that will be filled in with
**          the memory information.
*/
gceSTATUS
gckKERNEL_QueryVideoMemory(
    IN gckKERNEL Kernel,
    OUT gcsHAL_INTERFACE * Interface
    )
{
    gckGALDEVICE device;

    gcmkHEADER_ARG("Kernel=%p", Kernel);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
    gcmkVERIFY_ARGUMENT(Interface != NULL);

    /* Extract the pointer to the gckGALDEVICE class. */
    device = (gckGALDEVICE) Kernel->context;

    /* Get internal memory size and physical address. */
    Interface->u.QueryVideoMemory.internalSize = device->internalSize;
    Interface->u.QueryVideoMemory.internalPhysical = device->internalPhysical;

    /* Get external memory size and physical address. */
    Interface->u.QueryVideoMemory.externalSize = device->externalSize;
    Interface->u.QueryVideoMemory.externalPhysical = device->externalPhysical;

    /* Get contiguous memory size and physical address. */
    Interface->u.QueryVideoMemory.contiguousSize = device->contiguousSize;
    Interface->u.QueryVideoMemory.contiguousPhysical = device->contiguousPhysical;

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckKERNEL_GetVideoMemoryPool
**
**  Get the gckVIDMEM object belonging to the specified pool.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gcePOOL Pool
**          Pool to query gckVIDMEM object for.
**
**  OUTPUT:
**
**      gckVIDMEM * VideoMemory
**          Pointer to a variable that will hold the pointer to the gckVIDMEM
**          object belonging to the requested pool.
*/
gceSTATUS
gckKERNEL_GetVideoMemoryPool(
    IN gckKERNEL Kernel,
    IN gcePOOL Pool,
    OUT gckVIDMEM * VideoMemory
    )
{
    gckGALDEVICE device;
    gckVIDMEM videoMemory;

    gcmkHEADER_ARG("Kernel=%p Pool=%d", Kernel, Pool);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
    gcmkVERIFY_ARGUMENT(VideoMemory != NULL);

    /* Extract the pointer to the gckGALDEVICE class. */
    device = (gckGALDEVICE) Kernel->context;

    /* Dispatch on pool. */
    switch (Pool)
    {
    case gcvPOOL_LOCAL_INTERNAL:
        /* Internal memory. */
        videoMemory = device->internalVidMem;
        break;

    case gcvPOOL_LOCAL_EXTERNAL:
        /* External memory. */
        videoMemory = device->externalVidMem;
        break;

    case gcvPOOL_SYSTEM:
        /* System memory. */
        videoMemory = device->contiguousVidMem;
        break;

    default:
        /* Unknown pool. */
        videoMemory = NULL;
    }

    /* Return pointer to the gckVIDMEM object. */
    *VideoMemory = videoMemory;

    /* Return status. */
    gcmkFOOTER_ARG("*VideoMemory=0x%x", *VideoMemory);
    return (videoMemory == NULL) ? gcvSTATUS_OUT_OF_MEMORY : gcvSTATUS_OK;
}
#if gcdUSE_VIDMEM_PER_PID
/*******************************************************************************
**
**  gckKERNEL_GetVideoMemoryPool
**
**  Get the gckVIDMEM object belonging to the specified pool.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gcePOOL Pool
**          Pool to query gckVIDMEM object for.
**
**  OUTPUT:
**
**      gckVIDMEM * VideoMemory
**          Pointer to a variable that will hold the pointer to the gckVIDMEM
**          object belonging to the requested pool.
*/
gceSTATUS
gckKERNEL_GetVideoMemoryPoolPid(
    IN gckKERNEL Kernel,
    IN gcePOOL Pool,
    IN gctUINT32 Pid,
    OUT gckVIDMEM * VideoMemory
    )
{
    gceSTATUS status;
    gckGALDEVICE device;
    gckVIDMEM videoMemory = gcvNULL;

    gcmkHEADER_ARG("Kernel=%p", Kernel);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

    /* Extract the pointer to the gckGALDEVICE class. */
    device = (gckGALDEVICE) Kernel->context;

    /* Acquire mutex. */
    gcmkONERROR(gckOS_AcquireMutex(device->os, device->contiguousVidMemListMutex, gcvINFINITE));

    videoMemory = device->contiguousVidMemList;

    while (videoMemory != gcvNULL)
    {
        if (videoMemory->pid == Pid)
            break;

        videoMemory = videoMemory->next;
    }

    /* Release mutex. */
    gcmkVERIFY_OK(gckOS_ReleaseMutex(device->os, device->contiguousVidMemListMutex));

OnError:
    /* Return pointer to the gckVIDMEM object. */
    if (VideoMemory != gcvNULL)
    {
        *VideoMemory = videoMemory;
        gcmkFOOTER_ARG("*VideoMemory=%p", *VideoMemory);
    }
    else
    {
        gcmkFOOTER_NO();
    }

    /* Return status. */
    return (videoMemory == gcvNULL) ? gcvSTATUS_NOT_FOUND : gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckKERNEL_GetVideoMemoryPool
**
**  Get the gckVIDMEM object belonging to the specified pool.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gcePOOL Pool
**          Pool to query gckVIDMEM object for.
**
**  OUTPUT:
**
**      gckVIDMEM * VideoMemory
**          Pointer to a variable that will hold the pointer to the gckVIDMEM
**          object belonging to the requested pool.
*/
gceSTATUS
gckKERNEL_CreateVideoMemoryPoolPid(
    IN gckKERNEL Kernel,
    IN gcePOOL Pool,
    IN gctUINT32 Pid,
    OUT gckVIDMEM * VideoMemory
    )
{
    gceSTATUS status;
    gckGALDEVICE device;
    gckVIDMEM videoMemory = gcvNULL;
    /* Allocate initial memory size. */
    gctSIZE_T bytes = gcdUSE_VIDMEM_PER_PID_SIZE;
    gctPHYS_ADDR physical_temp;
    gctUINT32 physical;
    gctPOINTER logical;

    gcmkHEADER_ARG("Kernel=%p", Kernel);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

    /* Extract the pointer to the gckGALDEVICE class. */
    device = (gckGALDEVICE) Kernel->context;

    /* Acquire mutex. */
    gcmkONERROR(gckOS_AcquireMutex(device->os, device->contiguousVidMemListMutex, gcvINFINITE));

    gcmkONERROR(gckOS_AllocateContiguous(Kernel->os,
            gcvTRUE,
            &bytes,
            &physical_temp,
            &logical));

    /* physical address is returned as 0 for user space. workaround. */
    if (physical_temp == gcvNULL)
    {
        gcmkONERROR(gckOS_GetPhysicalAddress(Kernel->os, logical, &physical));
    }

    /* Create a videoMemory node for this process. */
    gcmkONERROR(gckVIDMEM_Construct(Kernel->os,
            physical  | device->systemMemoryBaseAddress,
            bytes,
            64,
            0,
            &videoMemory
            ));

    /*videoMemory->pid = Pid;*/

    /* Append this node to the list of VidMems. */
    if (videoMemory != gcvNULL)
    {
        printf("Created VIDMEM:%p, pid:%d\n", videoMemory, Pid);
        videoMemory->next = device->contiguousVidMemList;
        device->contiguousVidMemList = videoMemory;
    }

OnError:
    /* Release mutex. */
    gcmkVERIFY_OK(gckOS_ReleaseMutex(device->os, device->contiguousVidMemListMutex));

    /* Return pointer to the gckVIDMEM object. */
    *VideoMemory = videoMemory;
    gcmkFOOTER_ARG("*VideoMemory=%p", *VideoMemory);

    /* Return status. */
    return (videoMemory == gcvNULL) ? gcvSTATUS_OUT_OF_MEMORY : gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckKERNEL_RemoveVideoMemoryPoolPid
**
**  Remove the gckVIDMEM object.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gckVIDMEM VideoMemory
**          Pointer to a variable that holds the pointer to the gckVIDMEM
**          object.
**
**  OUTPUT:
**
*/
gceSTATUS
gckKERNEL_RemoveVideoMemoryPoolPid(
    IN gckKERNEL Kernel,
    IN gckVIDMEM VideoMemory
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gckGALDEVICE device;
    gckVIDMEM prev;

    gcmkHEADER_ARG("Kernel=%p VideoMemory:%p", Kernel, VideoMemory);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
    gcmkVERIFY_ARGUMENT(VideoMemory != gcvNULL);

    /* Extract the pointer to the gckGALDEVICE class. */
    device = (gckGALDEVICE) Kernel->context;

    /* Acquire mutex. */
    gcmkVERIFY_OK(gckOS_AcquireMutex(device->os, device->contiguousVidMemListMutex, gcvINFINITE));

    if (device->contiguousVidMemList == VideoMemory)
    {
        device->contiguousVidMemList = device->contiguousVidMemList->next;
    }
    else
    {
        prev = device->contiguousVidMemList;

        while (prev != gcvNULL)
        {
            if (prev->next == VideoMemory)
            {
                prev->next = VideoMemory->next;
                break;
            }

            prev = prev->next;
        }

        status = gcvSTATUS_NOT_FOUND;
    }

    /* Release mutex. */
    gcmkVERIFY_OK(gckOS_ReleaseMutex(device->os, device->contiguousVidMemListMutex));

    /* Return status. */
    gcmkFOOTER_NO();
    return status;
}
#endif

/*******************************************************************************
**
**  gckKERNEL_MapMemory
**
**  Map video memory into the current process space.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gctPHYS_ADDR Physical
**          Physical address of video memory to map.
**
**      gctSIZE_T Bytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      gctPOINTER * Logical
**          Pointer to a variable that will hold the base address of the mapped
**          memory region.
*/
gceSTATUS
gckKERNEL_MapMemory(
    IN gckKERNEL Kernel,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Logical
    )
{
    return gckOS_MapMemory(Kernel->os, Physical, Bytes, Logical);
}

/*******************************************************************************
**
**  gckKERNEL_UnmapMemory
**
**  Unmap video memory from the current process space.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gctPHYS_ADDR Physical
**          Physical address of video memory to map.
**
**      gctSIZE_T Bytes
**          Number of bytes to map.
**
**      gctPOINTER Logical
**          Base address of the mapped memory region.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckKERNEL_UnmapMemory(
    IN gckKERNEL Kernel,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T Bytes,
    IN gctPOINTER Logical
    )
{
    return gckOS_UnmapMemory(Kernel->os, Physical, Bytes, Logical);
}

/*******************************************************************************
**
**  gckKERNEL_MapVideoMemory
**
**  Map video memory for the current process.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to map the memory into the user space.
**
**      gctUINT32 Address
**          Hardware specific memory address.
**
**      gctUINT32 Pid
**          Process ID of the current process.
**
**      gctUINT32 Bytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      gctPOINTER * Logical
**          Pointer to a variable that will hold the logical address of the
**          specified memory address.
*/
gceSTATUS
gckKERNEL_MapVideoMemory(
    IN gckKERNEL Kernel,
    IN gctBOOL InUserSpace,
    IN gctUINT32 Address,
    IN gctUINT32 Pid,
    IN gctUINT32 Bytes,
    OUT gctPOINTER * Logical
    )
{
	gctUINT32 baseAddress;
    off64_t offset;

    gcmkHEADER_ARG("Kernel=0x%x InUserSpace=%d Address=%08x",
                   Kernel, InUserSpace, Address);

    gckOS_GetBaseAddress(Kernel->os, &baseAddress);

    offset = (off64_t)(Address + baseAddress) - (off64_t)drv_mempool_get_basePAddress();

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

    if (Kernel->core == gcvCORE_VG)
    {
        *Logical = (gctPOINTER)mmap64_join(Pid, gcvNULL, Bytes,
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NOINIT,
                drv_mempool_get_fileDescriptor(), offset);
    }
    else
    {
        *Logical = (gctPOINTER)mmap64_peer(Pid, gcvNULL, Bytes,
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_NOINIT,
                drv_mempool_get_fileDescriptor(), offset);
    }

    if (*Logical == MAP_FAILED) {
        *Logical = NULL;
        gcmkFOOTER_NO();
        return gcvSTATUS_INVALID_ADDRESS;
    }

    gcmkFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckKERNEL_UnmapVideoMemory
**
**  Unmap video memory for the current process.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gctUINT32 Address
**          Hardware specific memory address.
**
**      gctUINT32 Pid
**          Process ID of the current process.
**
**      gctUINT32 Bytes
**          Number of bytes to map.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckKERNEL_UnmapVideoMemory(
    IN gckKERNEL Kernel,
    IN gctPOINTER Logical,
    IN gctUINT32 Pid,
    IN gctUINT32 Bytes
    )
{
    gcmkHEADER_ARG("Logical=0x%08x pid=%u Bytes=%u",
                   Logical, Pid, Bytes);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

    if (Kernel->core == gcvCORE_VG)
    {
        munmap(Logical, Bytes);
    }

    if (munmap_peer(Pid, Logical, Bytes) == -1)
    {
        gcmkFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ADDRESS);
        return gcvSTATUS_INVALID_ADDRESS;
    }

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckKERNEL_Notify
**
**  This function is called by clients to notify the gckKERNEL object of an event.
**
**  INPUT:
**
**      gckKERNEL Kernel
**          Pointer to an gckKERNEL object.
**
**      gceNOTIFY Notification
**          Notification event.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gckKERNEL_Notify(
    IN gckKERNEL Kernel,
    IN gceNOTIFY Notification,
    IN gctBOOL Data
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Kernel=%p Notification=%d Data=%d",
                   Kernel, Notification, Data);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);

    /* Dispatch on notifcation. */
    switch (Notification)
    {
    case gcvNOTIFY_INTERRUPT:
        /* Process the interrupt. */
#if COMMAND_PROCESSOR_VERSION > 1
        status = gckINTERRUPT_Notify(Kernel->interrupt, Data);
#else
        status = gckHARDWARE_Interrupt(Kernel->hardware, Data);
#endif
        break;

    default:
        status = gcvSTATUS_OK;
        break;
    }

    /* Success. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckKERNEL_QuerySettings(
    IN gckKERNEL Kernel,
    OUT gcsKERNEL_SETTINGS * Settings
    )
{
    gckGALDEVICE device;

    gcmkHEADER_ARG("Kernel=%p", Kernel);

    /* Verify the arguments. */
    gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
    gcmkVERIFY_ARGUMENT(Settings != gcvNULL);

    /* Extract the pointer to the gckGALDEVICE class. */
    device = (gckGALDEVICE) Kernel->context;

    /* Fill in signal. */
    Settings->signal = -1;

    /* Success. */
    gcmkFOOTER_ARG("Settings->signal=%d", Settings->signal);
    return gcvSTATUS_OK;
}


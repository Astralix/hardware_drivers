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




#include <windows.h>
#include <ceddk.h>
#include "gc_hal_kernel_ce.h"
#include "gc_hal.h"
#include "gc_hal_kernel.h"
#include "gc_hal_driver.h"

#define _GC_OBJ_ZONE    gcvZONE_KERNEL

/******************************************************************************\
******************************* gckKERNEL API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**	gckKERNEL_QueryVideoMemory
**
**	Query the amount of video memory.
**
**	INPUT:
**
**		gckKERNEL Kernel
**			Pointer to an gckKERNEL object.
**
**	OUTPUT:
**
**		gcsHAL_INTERFACE * Interface
**			Pointer to an gcsHAL_INTERFACE structure that will be filled in with
**			the memory information.
*/
gceSTATUS gckKERNEL_QueryVideoMemory(
	IN gckKERNEL Kernel,
	OUT gcsHAL_INTERFACE * Interface
	)
{
	GCHAL * gchal;

    gcmkHEADER_ARG("Kernel=%p", Kernel);

	gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_KERNEL,
			   "[ENTER] gckKERNEL_QueryVideoMemory");

	/* Verify the arguments. */
	gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmkVERIFY_ARGUMENT(Interface != gcvNULL);

	/* Extract the pointer to the CExtension class. */
	gchal = (GCHAL *) Kernel->context;

	/* Get internal memory size and physical address. */
	Interface->u.QueryVideoMemory.internalSize     =
		gchal->GetInternalSize();
	Interface->u.QueryVideoMemory.internalPhysical =
		gchal->GetInternalPhysical();

	/* Get external memory size and physical address. */
	Interface->u.QueryVideoMemory.externalSize     =
		gchal->GetExternalSize();
	Interface->u.QueryVideoMemory.externalPhysical =
		gchal->GetExternalPhysical();

	/* Get contiguous memory size and physical address. */
	Interface->u.QueryVideoMemory.contiguousSize     =
		gchal->GetContiguousSize();
	Interface->u.QueryVideoMemory.contiguousPhysical =
		gchal->GetContiguousPhysical();

	gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_KERNEL,
			   "[LEAVE] gckKERNEL_QueryVideoMemory(%u)",
			   gcvSTATUS_OK);

	/* Success. */
    gcmkFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gckKERNEL_GetVideoMemoryPool
**
**	Get the gckVIDMEM object belonging to the specified pool.
**
**	INPUT:
**
**		gckKERNEL Kernel
**			Pointer to an gckKERNEL object.
**
**		gcePOOL Pool
**			Pool to query gckVIDMEM object for.
**
**	OUTPUT:
**
**		gckVIDMEM * VideoMemory
**			Pointer to a variable that will hold the pointer to the gckVIDMEM
**			object belonging to the requested pool.
*/
gceSTATUS gckKERNEL_GetVideoMemoryPool(
	IN gckKERNEL Kernel,
	IN gcePOOL Pool,
	OUT gckVIDMEM * VideoMemory
	)
{
	GCHAL * gchal;
	gckVIDMEM videoMemory;
	gceSTATUS status;

    gcmkHEADER_ARG("Kernel=%p Pool=%d", Kernel, Pool);

	gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_KERNEL,
			   "[ENTER] gckHARDWARE_GetVideoMemoryPool");

	/* Verify the arguments. */
	gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmkVERIFY_ARGUMENT(VideoMemory != gcvNULL);

    /* Extract the pointer to the GCHAL class. */
    gchal = (GCHAL *) Kernel->context;

	/* Dispatch on pool. */
	switch (Pool)
	{
	case gcvPOOL_LOCAL_INTERNAL:
		/* Internal memory. */
		videoMemory = gchal->GetInternalHeap();
		break;

	case gcvPOOL_LOCAL_EXTERNAL:
		/* External memory. */
		videoMemory = gchal->GetExternalHeap();
		break;

	case gcvPOOL_SYSTEM:
		/* System memory. */
		videoMemory = gchal->GetContiguousHeap();
		break;

	default:
		/* Unknown pool. */
		videoMemory = gcvNULL;
		gcmkFATAL("Unknown memory pool: %u", Pool);
	}

	/* Return pointer to the gckVIDMEM object. */
	*VideoMemory = videoMemory;

	/* Determine the status. */
	status = (videoMemory == gcvNULL) ? gcvSTATUS_OUT_OF_MEMORY : gcvSTATUS_OK;

	if (gcmIS_SUCCESS(status))
	{
		gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_KERNEL,
				   "gckHARDWARE_GetVideoMemoryPool: Pool %u starts at %p",
				   Pool, videoMemory);
	}

	gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_KERNEL,
			   "[LEAVE] gckHARDWARE_GetVideoMemoryPool(%u)",
			   status);

	/* Return the status. */
    gcmkFOOTER_ARG("status=%d, *VideoMemory=%p", status, gcmOPT_VALUE(VideoMemory));
	return status;
}

/*******************************************************************************
**
**	gckKERNEL_MapMemory
**
**	Map video memory into the current process space.
**
**	INPUT:
**
**		gckKERNEL Kernel
**			Pointer to an gckKERNEL object.
**
**		gctPHYS_ADDR Physical
**			Physical address of video memory to map.
**
**		gctSIZE_T Bytes
**			Number of bytes to map.
**
**	OUTPUT:
**
**		gctPOINTER * Logical
**			Pointer to a variable that will hold the base address of the mapped
**			memory region.
*/
gceSTATUS gckKERNEL_MapMemory(
	IN gckKERNEL Kernel,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Logical
	)
{
	// Call gckOS.
	return gckOS_MapMemory(Kernel->os, Physical, Bytes, Logical);
}

/*******************************************************************************
**
**	gckKERNEL_UnmapMemory
**
**	Unmap video memory from the current process space.
**
**	INPUT:
**
**		gckKERNEL Kernel
**			Pointer to an gckKERNEL object.
**
**		gctPHYS_ADDR Physical
**			Physical address of video memory to map.
**
**		gctSIZE_T Bytes
**			Number of bytes to map.
**
**		gctPOINTER Logical
**			Base address of the mapped memory region.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gckKERNEL_UnmapMemory(
	IN gckKERNEL Kernel,
	IN gctPHYS_ADDR Physical,
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical
	)
{
	// Call gckOS.
	return gckOS_UnmapMemory(Kernel->os, Physical, Bytes, Logical);
}

gceSTATUS gckKERNEL_MapVideoMemory(
	IN gckKERNEL Kernel,
	IN gctBOOL InUserSpace,
	IN gctUINT32 Address,
	OUT gctPOINTER * Logical
	)
{
	return gckKERNEL_MapVideoMemoryEx(Kernel,
		                              gcvCORE_MAJOR,
									  InUserSpace,
									  Address,
									  Logical);
}
/*******************************************************************************
**
**	gckKERNEL_MapVideoMemory
**
**	Get the logical address for a hardware specific memory address for the
**	current process.
**
**	INPUT:
**
**		gckKERNEL Kernel
**			Pointer to an gckKERNEL object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to map the memory into the user space.
**
**		gctUINT32 Address
**			Hardware specific memory address.
**
**	OUTPUT:
**
**		gctPOINTER * Logical
**			Pointer to a variable that will hold the logical address of the
**			specified memory address.
*/
gceSTATUS gckKERNEL_MapVideoMemoryEx(
	IN gckKERNEL Kernel,
    IN gceCORE   Core,
	IN gctBOOL InUserSpace,
	IN gctUINT32 Address,
	OUT gctPOINTER * Logical
	)
{
	GCHAL * gchal;
	gcePOOL pool;
	gctUINT32 offset, base;
	gceSTATUS status;
	gctPOINTER logical;

    gcmkHEADER_ARG("Kernel=%p InUserSpace=%d Address=%08x",
                   Kernel, InUserSpace, Address);

	gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_KERNEL,
			   "[ENTER] gckKERNEL_MapVideoMemory");

	/* Verify the arguments. */
	gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmkVERIFY_ARGUMENT(Logical != gcvNULL);

	/* Extract the pointer to the GCHAL class. */
	gchal = (GCHAL *) Kernel->context;

	do
	{
#if gcdENABLE_VG
		if (Core == gcvCORE_VG)
		{
			/* Split the memory address into a pool type and offset. */
			gcmkERR_BREAK(gckVGHARDWARE_SplitMemory(Kernel->vg->hardware,
											 Address,
											 &pool,
											 &offset));
		}
		else
#endif
		{
			/* Split the memory address into a pool type and offset. */
			gcmkERR_BREAK(gckHARDWARE_SplitMemory(Kernel->hardware,
											 Address,
											 &pool,
											 &offset));
		}
		/* Dispatch on pool. */
		switch (pool)
		{
		case gcvPOOL_LOCAL_INTERNAL:
			/* Internal memory. */
			logical = gchal->GetInternalLogical();
			break;

		case gcvPOOL_LOCAL_EXTERNAL:
			/* External memory. */
			logical = gchal->GetExternalLogical();
			break;

		case gcvPOOL_SYSTEM:
			/* System memory. */
#if UNDER_CE >= 600
			if (InUserSpace)
			{
				logical = gchal->GetProcessContiguousLogical();
			}
			else
			{
				logical = gchal->GetContiguousLogical();
			}
#else
			logical = gchal->GetContiguousLogical();
#endif

#if gcdENABLE_VG
			if (Core == gcvCORE_VG)
			{
				gcmkVERIFY_OK(gckVGHARDWARE_SplitMemory(Kernel->vg->hardware,
												 gchal->GetContiguousHeap()->baseAddress,
												 &pool,
												 &base));
			}
			else
#endif
			{
				gcmkVERIFY_OK(gckHARDWARE_SplitMemory(Kernel->hardware,
												 gchal->GetContiguousHeap()->baseAddress,
												 &pool,
												 &base));
			}
			offset -= base;
			break;

		default:
			/* Invalid memory pool. */
			gcmkFATAL("Unknown memory pool: %u", pool);
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Build logical address of specified address. */
		*Logical = reinterpret_cast<gctPOINTER>
			(static_cast<gctUINT8 *>(logical) + offset);
	}
	while (gcvFALSE);

	if (gcmIS_SUCCESS(status))
	{
		gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_KERNEL,
				   "gckKERNEL_MapVideoMemory: Address 0x%08X maps to %p",
				   Address, *Logical);
	}

	gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_KERNEL,
			   "[LEAVE] gckKERNEL_MapVideoMemory(%u)",
			   status);

	/* Return the status. */
    gcmkFOOTER();
	return status;
}

/*******************************************************************************
**
**	gckKERNEL_Notify
**
**	This function iscalled by clients to notify the gckKERNEL object of an event.
**
**	INPUT:
**
**		gckKERNEL Kernel
**			Pointer to an gckKERNEL object.
**
**		gceNOTIFY Notification
**			Notification event.
**
**		gctBOOL Data
**			Optional data for notification.
**
**	OUTPUT:
**
**		Nothing.
*/
gceSTATUS gckKERNEL_Notify(
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
        /* Process interrupt. */
        status = gckHARDWARE_Interrupt(Kernel->hardware, Data);
		break;

#ifdef EMULATOR
	case gcvNOTIFY_COMMAND_QUEUE:
		// Unfreeze C-Model.
		((GCHAL*) Kernel->context)->UnFreeze();
		status = gcvSTATUS_OK;
		break;
#endif

    default:
        /* Success. */
        status = gcvSTATUS_OK;
        break;
    }

    /* Return the status. */
    gcmkFOOTER();
    return status;
}

gceSTATUS
gckKERNEL_QuerySettings(
	IN gckKERNEL Kernel,
	OUT gcsKERNEL_SETTINGS * Settings
	)
{
    gcmkHEADER_ARG("Kernel=%p", Kernel);

	/* Verify the arguments. */
	gcmkVERIFY_OBJECT(Kernel, gcvOBJ_KERNEL);
	gcmkVERIFY_ARGUMENT(Settings != gcvNULL);

	/* Success. */
    gcmkFOOTER_NO();
	return gcvSTATUS_OK;
}

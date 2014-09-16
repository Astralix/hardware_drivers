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


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

typedef struct _vgsMEMORYITEM * vgsMEMORYITEM_PTR;
typedef struct _vgsMEMORYITEM
{
	vgsMEMORYITEM_PTR		next;
}
vgsMEMORYITEM;

typedef struct _vgsMEMORYMANAGER
{
	/* HAL OS object. */
	gcoOS					os;

	/* The size of one allocation in bytes. */
	gctUINT					allocationSize;

	/* Total number of items per allocation. */
	gctUINT					itemCount;

	/* Total size of one item in bytes. */
	gctUINT					itemSize;

	/* The head of allocation list. */
	vgsMEMORYITEM_PTR		firstAllocated;

	/* The head of the free list. */
	vgsMEMORYITEM_PTR		firstFree;

#if vgvVALIDATE_MEMORY_MANAGER
	/* The number of allocated items. */
	gctINT					allocatedCount;

	/* Maximum allocated items. */
	gctUINT					maximumAllocated;

	/* Allocated buffers. */
	gctUINT					allocatedBufferCount;
#endif
}
vgsMEMORYMANAGER;


#if vgvVALIDATE_MEMORY_MANAGER
static gctBOOL _IsValidItem(
	IN vgsMEMORYMANAGER_PTR Manager,
	IN vgsMEMORYITEM_PTR Item
	)
{
	gctUINT itemCount, itemSize, itemOffset;
	gctINT itemNumber, itemAignment;
	vgsMEMORYITEM_PTR buffer;
	vgsMEMORYITEM_PTR firstItem;

	/* Get the number of items per allocation. */
	itemCount = Manager->itemCount;

	/* Get the item size. */
	itemSize = Manager->itemSize;

	/* Get the first allocated buffer. */
	buffer = Manager->firstAllocated;

	/* Go through all buffers. */
	while (buffer != gcvNULL)
	{
		/* Determine the first item. */
		firstItem = buffer + 1;

		/* Determine the offset of the item. */
		itemOffset = (gctUINT8_PTR) Item - (gctUINT8_PTR) firstItem;

		/* Determine the alignemnt. */
		itemAignment = itemOffset % itemSize;

		/* Determine the item number. */
		itemNumber = itemOffset / itemSize;

		/* Validate the item. */
		if ((itemAignment == 0) &&
			(itemNumber >= 0) &&
			((gctUINT) itemNumber < itemCount))
		{
			return gcvTRUE;
		}

		/* Advance to the next buffer. */
		buffer = buffer->next;
	}

	/* Not a valid item. */
	return gcvFALSE;
}

static gctBOOL _IsAllocatedItem(
	IN vgsMEMORYMANAGER_PTR Manager,
	IN vgsMEMORYITEM_PTR Item
	)
{
	vgsMEMORYITEM_PTR freeItem;

	/* Get first free item. */
	freeItem = Manager->firstFree;

	/* Go through all free items. */
	while (freeItem != gcvNULL)
	{
		/* A match? */
		if (freeItem == Item)
		{
			/* The item is not allocated. */
			return gcvFALSE;
		}

		/* Advance to the next item. */
		freeItem = freeItem->next;
	}

	/* The item is allocated. */
	return gcvTRUE;
}
#endif


/******************************************************************************\
****************************** Memory Manager API ******************************
\******************************************************************************/

/*******************************************************************************
**
** vgsMEMORYMANAGER_Construct
**
** Construct a manager object for same-sized items.
**
** INPUT:
**
**    Os
**       Pointer to a gcoOS object.
**
**    ItemSize
**       The size of each item in bytes.
**
**    Granularity
**       The number of items per allocation.
**
** OUTPUT:
**
**    Manager
**       Pointer to the constructed memory manager.
*/

gceSTATUS vgsMEMORYMANAGER_Construct(
    IN gcoOS Os,
	IN gctUINT ItemSize,
	IN gctUINT Granularity,
	OUT vgsMEMORYMANAGER_PTR * Manager
	)
{
	gceSTATUS status;
	vgsMEMORYMANAGER_PTR manager = gcvNULL;

    gcmHEADER_ARG("Os=0x%x ItemSize=0x%x Granularity=0x%x Manager=0x%x",
        Os, ItemSize, Granularity, Manager);

	gcmVERIFY_ARGUMENT(ItemSize > 0);
	gcmVERIFY_ARGUMENT(Granularity > 0);
	gcmVERIFY_ARGUMENT(Manager != gcvNULL);

	do
	{
		gctUINT itemSize;
		gctUINT allocationSize;

		/* Allocate the memory manager structure. */
		gcmERR_BREAK(gcoOS_Allocate(
			Os,
			gcmSIZEOF(vgsMEMORYMANAGER),
			(gctPOINTER *) &manager
			));

		/* Determine the size of the item. */
		itemSize = gcmSIZEOF(vgsMEMORYITEM) + ItemSize;

		/* Determine the allocation size. */
		allocationSize = gcmSIZEOF(vgsMEMORYITEM) + Granularity * itemSize;

		/* Initialize the structure. */
		manager->os             = Os;
		manager->allocationSize = allocationSize;
		manager->itemCount      = Granularity;
		manager->itemSize       = itemSize;
		manager->firstAllocated = gcvNULL;
		manager->firstFree      = gcvNULL;

#if vgvVALIDATE_MEMORY_MANAGER
		manager->allocatedCount       = 0;
		manager->maximumAllocated     = 0;
		manager->allocatedBufferCount = 0;
#endif

		/* Set the result pointer. */
		* Manager = manager;
	}
	while (gcvNULL);

    gcmFOOTER();

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsMEMORYMANAGER_Destroy
**
** Destruct the specified manager object.
**
** INPUT:
**
**    Manager
**       Pointer to the memory manager object.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsMEMORYMANAGER_Destroy(
	IN vgsMEMORYMANAGER_PTR Manager
	)
{
	gceSTATUS status;
	vgsMEMORYITEM_PTR current;
	vgsMEMORYITEM_PTR next;

    gcmHEADER_ARG("Manager=0x%x", Manager);

	/* Verify arguments. */
	gcmVERIFY_ARGUMENT(Manager != gcvNULL);

	/* Assume success. */
	status = gcvSTATUS_OK;

	/* Everything has to be freed. */
#if vgvVALIDATE_MEMORY_MANAGER
	gcmASSERT(Manager->allocatedCount == 0);
#endif

	/* Destroy all allocated buffers. */
	while (Manager->firstAllocated)
	{
		/* Get the current buffer. */
		current = Manager->firstAllocated;

		/* Get the next buffer. */
		next = current->next;

		/* Free the current. */
		gcmERR_BREAK(gcoOS_Free(Manager->os, current));

		/* Advance to the next one. */
		Manager->firstAllocated = next;
	}

	/* Success? */
	if (gcmIS_SUCCESS(status))
	{
		status = gcoOS_Free(Manager->os, Manager);
	}

    gcmFOOTER();
	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsMEMORYMANAGER_Allocate
**
** Allocate one item from the manager.
**
** INPUT:
**
**    Manager
**       Pointer to the memory manager object.
**
** OUTPUT:
**
**    Pointer
**       Pointer to the allocated item.
*/

gceSTATUS vgsMEMORYMANAGER_Allocate(
	IN vgsMEMORYMANAGER_PTR Manager,
	OUT gctPOINTER * Pointer
	)
{
	gceSTATUS status;
	vgsMEMORYITEM_PTR firstFree;

    gcmHEADER_ARG("Manager=0x%x Pointer=0x%x", Manager, Pointer);
	/* Verify arguments. */
	gcmVERIFY_ARGUMENT(Manager != gcvNULL);
	gcmVERIFY_ARGUMENT(Pointer != gcvNULL);

	/* Get the first free. */
	firstFree = Manager->firstFree;

	/* Are there free items? */
	if (firstFree != gcvNULL)
	{
		/* Set the result. */
		* Pointer = firstFree + 1;

		/* Remove from the free list. */
		Manager->firstFree = firstFree->next;

		/* Update allocated items count. */
#if vgvVALIDATE_MEMORY_MANAGER
		Manager->allocatedCount += 1;

		if ((gctUINT) Manager->allocatedCount > Manager->maximumAllocated)
		{
			Manager->maximumAllocated = Manager->allocatedCount;
		}
#endif

		/* Success. */
                gcmFOOTER_ARG("*Pointer=0x%x", *Pointer);
		return gcvSTATUS_OK;
	}

	/* No free items available. */
	do
	{
		vgsMEMORYITEM_PTR newBuffer;
		gctUINT i, itemCount;
		gctUINT itemSize;

		/* Allocate a new buffer. */
		gcmERR_BREAK(gcoOS_Allocate(
			Manager->os,
			Manager->allocationSize,
			(gctPOINTER) &newBuffer
			));

		/* Link in. */
		newBuffer->next = Manager->firstAllocated;
		Manager->firstAllocated = newBuffer;

		/* Set the result. */
		* Pointer = newBuffer + 2;

		/* Update allocated items count. */
#if vgvVALIDATE_MEMORY_MANAGER
		Manager->allocatedCount       += 1;
		Manager->allocatedBufferCount += 1;

		if ((gctUINT) Manager->allocatedCount > Manager->maximumAllocated)
		{
			Manager->maximumAllocated = Manager->allocatedCount;
		}
#endif

		/* Get the number of items per allocation. */
		itemCount = Manager->itemCount;

		/* Get the item size. */
		itemSize = Manager->itemSize;

		/* Determine the first free item. */
		firstFree = (vgsMEMORYITEM_PTR)
		(
			(gctUINT8_PTR) (newBuffer + 1) + itemSize
		);

		/* Populate the new buffer. */
		for (i = 1; i < itemCount; i++)
		{
			/* Add to the free item list. */
			firstFree->next = Manager->firstFree;
			Manager->firstFree = firstFree;

			/* Advance to the next item. */
			firstFree = (vgsMEMORYITEM_PTR)
			(
				(gctUINT8_PTR) firstFree + itemSize
			);
		}
	}
	while (gcvFALSE);

    gcmFOOTER();
	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsMEMORYMANAGER_Free
**
** Free previously allocated item.
**
** INPUT:
**
**    Manager
**       Pointer to the memory manager object.
**
**    Pointer
**       Pointer to the allocated item.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsMEMORYMANAGER_Free(
	IN vgsMEMORYMANAGER_PTR Manager,
	IN gctPOINTER Pointer
	)
{
	vgsMEMORYITEM_PTR item;

	/* Verify arguments. */
	gcmASSERT(Manager != gcvNULL);
	gcmASSERT(Pointer != gcvNULL);

	/* Get the item pointer. */
	item = (vgsMEMORYITEM_PTR) Pointer - 1;

	/* Update allocated items count. */
#if vgvVALIDATE_MEMORY_MANAGER
	gcmASSERT(_IsValidItem(Manager, item));
	gcmASSERT(_IsAllocatedItem(Manager, item));
	gcmASSERT(Manager->allocatedCount > 0);
	Manager->allocatedCount -= 1;
#endif

	/* Add to the free item list. */
	item->next = Manager->firstFree;
	Manager->firstFree = item;
}

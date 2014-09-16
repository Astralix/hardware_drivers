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

/* Shorthand for asserting on invalid condition during storage validation. */
#define vgmQUALIFY_STORAGE(Condition) \
{ \
	if (!(Condition)) \
	{ \
		gcmFATAL( \
			"%s(%d): STORAGE INVALID [called from %s(%d)])\n", \
			__FUNCTION__, __LINE__, CallerName, LineNumber \
			); \
		\
		return gcvSTATUS_INVALID_OBJECT; \
	} \
}

/* Free list terminator. */
#define vgvTERMINATOR \
	((vgsPATH_DATA_PTR) gcmINT2PTR(~0))

/* Minimum path data size. This is really arbitrary. */
#define vgvMINPATHSIZE \
	(gcmSIZEOF(gctUINT32) + gcmSIZEOF(gctFLOAT) * 6) * 10

/* Path storage object structure. */
typedef struct _vgsPATHSTORAGE
{
	/* System objects. */
	gcoOS					os;
	gcoHAL					hal;
	vgsCONTEXT_PTR			context;

	/* Command buffer attributes. */
	gcsCOMMAND_BUFFER_INFO	bufferInfo;

	/* Path command buffer attributes. */
	gcsPATH_BUFFER_INFO		pathInfo;

	/* The size of each allocated container. */
	gctSIZE_T				allocationSize;

	/* The maximum memory size allowed to allocate. */
	gctSIZE_T				allocationCap;

	/* The size of memory currently allocated. */
	gctSIZE_T				allocatedSize;

	/* The offset of the container structure within allocated buffer. */
	gctUINT					containerOffset;

	/* The allocated buffer list. */
	vgsCONTAINER_PTR		allocatedList;

	/* The maximum and minimum sizes of the path data buffer. */
	gctSIZE_T				minimumSize;
	gctSIZE_T				maximumSize;

	/* The list of free nodes. */
	vgsPATH_DATA_PTR		freeHead;
	vgsPATH_DATA_PTR		freeTail;

	/* Most Recently Used (MRU) list. */
	vgsPATH_DATA_PTR		mruList;

	/* The list of nodes that are freed but are still busy. */
	vgsPATH_DATA_PTR		busyHead;
	vgsPATH_DATA_PTR		busyTail;

	/* Debug info. */
#if vgvVALIDATE_PATH_STORAGE
	gctUINT					validateCounter;
	gctUINT					occupiedByHeaders;
	gctUINT					buffersAllocated;
	gctUINT					averageHeaderSize;
#endif
}
vgsPATHSTORAGE;


/*******************************************************************************
**
** _IsPathDataAllocated
**
** Verifies whether the specified path data buffer is allocated.
**
** INPUT:
**
**    PathData
**       Pointer to the buffer to be verified.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvVALIDATE_PATH_STORAGE

static gctBOOL _IsPathDataAllocated(
	IN vgsPATH_DATA_PTR PathData
	)
{
	gcmASSERT(PathData != gcvNULL);

	if (PathData->nextBusy != gcvNULL)
	{
		gcmASSERT(PathData->prevFree == gcvNULL);
		gcmASSERT(PathData->nextFree == gcvNULL);

		/* Not allocated. */
		return gcvFALSE;
	}

	if (PathData->prevFree != gcvNULL)
	{
		gcmASSERT(PathData->nextFree != gcvNULL);

		/* Not allocated. */
		return gcvFALSE;
	}

	/* Allocated. */
	return gcvTRUE;
}

#endif


/*******************************************************************************
**
** _GetSubBufferListHead / _GetSubBufferListTail
**
** Search for the head and tail of the subbuffer list.
**
** INPUT:
**
**    HeadPathData
**       Pointer to the head buffer of the path data chain.
**
**    PathData
**       Pointer to the buffer to be verified.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvVALIDATE_PATH_STORAGE

static gctBOOL _GetSubBufferListHead(
	IN vgsPATH_DATA_PTR PathData,
	OUT vgsPATH_DATA_PTR * Head
	)
{
	vgsPATH_DATA_PTR prevPathData;

	gcmASSERT(PathData != gcvNULL);

	while (gcvTRUE)
	{
		/* Get the previous subbuffer. */
		prevPathData = vgmGETPREVSUBBUFFER(PathData);

		/* Found the head? */
		if (prevPathData == gcvNULL)
		{
			break;
		}

		/* Make sure the connection is valid. */
		if (vgmGETNEXTSUBBUFFER(prevPathData) != PathData)
		{
			return gcvFALSE;
		}

		/* Advance to the previous buffer. */
		PathData = prevPathData;
	}

	/* Set the head. */
	* Head = PathData;

	/* Success. */
	return gcvTRUE;
}

static gctBOOL _GetSubBufferListTail(
	IN vgsPATH_DATA_PTR PathData,
	OUT vgsPATH_DATA_PTR * Tail
	)
{
	vgsPATH_DATA_PTR nextPathData;

	gcmASSERT(PathData != gcvNULL);

	while (gcvTRUE)
	{
		/* Get the next subbuffer. */
		nextPathData = vgmGETNEXTSUBBUFFER(PathData);

		/* Found the tail? */
		if (nextPathData == gcvNULL)
		{
			break;
		}

		/* Make sure the connection is valid. */
		if (vgmGETPREVSUBBUFFER(nextPathData) != PathData)
		{
			return gcvFALSE;
		}

		/* Advance to the next buffer. */
		PathData = nextPathData;
	}

	/* Set the tail. */
	* Tail = PathData;

	/* Success. */
	return gcvTRUE;
}

#endif


/*******************************************************************************
**
** _ValidateStorage
**
** Verify the existing structures.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    CallerName
**       Caller function name.
**
**    LineNumber
**       Caller line number.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvVALIDATE_PATH_STORAGE

gceSTATUS vgfValidateStorage(
	IN vgsPATHSTORAGE_PTR Storage,
	IN gctSTRING CallerName,
	IN gctUINT LineNumber
	)
{
	vgsPATH_DATA_PTR pathData;
	vgsCONTAINER_PTR container;

	/* Update validate counter. */
	Storage->validateCounter += 1;

	/* Break point. */
	if (Storage->validateCounter == 0)
	{
		gctBOOL breakPoint = gcvTRUE;
	}

	/* Validate the busy list. */
	pathData = Storage->busyHead;

	if (pathData != gcvNULL)
	{
		do
		{
			/* Validate reference. */
			vgmQUALIFY_STORAGE(vgmGETCMDBUFFER(pathData)->reference >= 0);

			/* Should not be free. */
			vgmQUALIFY_STORAGE(pathData->prevFree == gcvNULL);
			vgmQUALIFY_STORAGE(pathData->nextFree == gcvNULL);

			/* Validate tail. */
			if (pathData->nextBusy == vgvTERMINATOR)
			{
				vgmQUALIFY_STORAGE(pathData == Storage->busyTail);
			}

			/* Get the next buffer. */
			pathData = pathData->nextBusy;
		}
		while (pathData != vgvTERMINATOR);
	}

	/* Start with the furst container. */
	container = Storage->allocatedList;

	/* Walk through all containers. */
	while (container)
	{
		vgsPATH_DATA_PTR initialPathData;
		vgsPATH_DATA_PTR headPathData;
		vgsPATH_DATA_PTR tailPathData;
		gcsCMDBUFFER_PTR buffer;
		gctUINT bufferSize;
		gctUINT totalSize;

		/* Get the path data buffer. */
		initialPathData = (vgsPATH_DATA_PTR)
		(
			(gctUINT8_PTR) container - Storage->containerOffset
		);

		/* Reset the total size. */
		totalSize = 0;

		/* Set the initial path data. */
		pathData = initialPathData;

		/* Walk through all buffers. */
		while (pathData)
		{
			/* Get the command buffer. */
			buffer = vgmGETCMDBUFFER(pathData);

			/* Determine the buffer size. */
			bufferSize
				= buffer->bufferOffset
				+ buffer->size
				+ Storage->pathInfo.reservedForTail
				+ Storage->bufferInfo.staticTailSize;

			/* Add the buffer size. */
			totalSize += bufferSize;

			/* Make sure the path refers to the path data. */
			vgmQUALIFY_STORAGE(_GetSubBufferListHead(pathData, &headPathData));
			vgmQUALIFY_STORAGE(_GetSubBufferListTail(pathData, &tailPathData));

			if (_IsPathDataAllocated(headPathData))
			{
				/* Is this the initial buffer? */
				if (pathData->path == gcvNULL)
				{
					vgmQUALIFY_STORAGE(buffer->size == Storage->maximumSize);
					vgmQUALIFY_STORAGE(pathData == initialPathData);
					vgmQUALIFY_STORAGE(vgmGETNEXTALLOCATED(pathData) == gcvNULL);
				}
				else
				{
					gctBOOL fillBuffer, strokeBuffer;


					/* Fill path? */
					fillBuffer
						=  (pathData->path->head == headPathData)
						&& (pathData->path->tail == tailPathData);

					/* Stroke path? */
					strokeBuffer
						=  (pathData->path->stroke == headPathData);

					vgmQUALIFY_STORAGE(  fillBuffer || strokeBuffer);
					vgmQUALIFY_STORAGE(!(fillBuffer && strokeBuffer));
				}
			}

			/* Advance to the next buffer. */
			pathData = vgmGETNEXTALLOCATED(pathData);
		}

		/* Add the container size. */
		totalSize += gcmSIZEOF(vgsCONTAINER);

		/* Verify the total size. */
		vgmQUALIFY_STORAGE(totalSize == Storage->allocationSize);

		/* Advance to the next container. */
		container = container->next;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

#endif


/*******************************************************************************
**
** _AppendToNodeList
**
** Add the specified buffer to the node list.
**
** INPUT:
**
**    AddAfter
**       Pointer to the buffer after which the new buffer should be added.
**
**    PathData
**       Pointer to the path data to insert into the list.
**
** OUTPUT:
**
**    Nothing.
*/

static void _AppendToNodeList(
	IN vgsPATH_DATA_PTR AddAfter,
	IN vgsPATH_DATA_PTR PathData
	)
{
	vgsPATH_DATA_PTR addBefore;

	/* Cannot add before the first node. */
	gcmASSERT(AddAfter != gcvNULL);

	/* Create a shortcut to the next node. */
	addBefore = vgmGETNEXTALLOCATED(AddAfter);

	/* Adding to the tail? */
	if (addBefore == gcvNULL)
	{
		/* Initialize node links. */
		vgmSETPREVALLOCATED(PathData, AddAfter);
		vgmSETNEXTALLOCATED(PathData, gcvNULL);

		/* Link in. */
		vgmSETNEXTALLOCATED(AddAfter, PathData);
	}

	/* Adding in the middle. */
	else
	{
		/* Initialize node links. */
		vgmSETPREVALLOCATED(PathData, AddAfter);
		vgmSETNEXTALLOCATED(PathData, addBefore);

		/* Link in. */
		vgmSETNEXTALLOCATED(AddAfter,  PathData);
		vgmSETPREVALLOCATED(addBefore, PathData);
	}
}


/*******************************************************************************
**
** _RemoveFromNodeList
**
** Remove the specified buffer from the node list.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to be removed.
**
** OUTPUT:
**
**    Nothing.
*/

static void _RemoveFromNodeList(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	vgsPATH_DATA_PTR prev;
	vgsPATH_DATA_PTR next;

	/* Cannot remove the first node. */
	gcmASSERT(vgmGETPREVALLOCATED(PathData) != gcvNULL);

	/* Create shortcuts to the previous and next nodes. */
	prev = vgmGETPREVALLOCATED(PathData);
	next = vgmGETNEXTALLOCATED(PathData);

	/* Tail node? */
	if (next == gcvNULL)
	{
		/* Remove from the list. */
		vgmSETNEXTALLOCATED(prev, gcvNULL);
	}

	/* Node from the middle. */
	else
	{
		vgmSETNEXTALLOCATED(prev, next);
		vgmSETPREVALLOCATED(next, prev);
	}
}


/*******************************************************************************
**
** _AppendToMRUList
**
** Add the specified buffer to MRU list.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to insert into the list.
**
** OUTPUT:
**
**    Nothing.
*/

static void _AppendToMRUList(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Cannot be in the list already. */
	gcmASSERT(PathData->prevMRU == gcvNULL);
	gcmASSERT(PathData->nextMRU == gcvNULL);

	/* Empty list? */
	if (Storage->mruList == gcvNULL)
	{
		/* Initialize node links. */
		PathData->prevMRU = PathData;
		PathData->nextMRU = PathData;
	}

	/* Add to the tail. */
	else
	{
		/* Initialize node links. */
		PathData->prevMRU = Storage->mruList;
		PathData->nextMRU = Storage->mruList->nextMRU;

		/* Link in. */
		PathData->prevMRU->nextMRU = PathData;
		PathData->nextMRU->prevMRU = PathData;
	}

	/* Update the list head. */
	Storage->mruList = PathData;
}


/*******************************************************************************
**
** _RemoveFromMRUList
**
** Remove the specified buffer from MRU list.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to be removed.
**
** OUTPUT:
**
**    Nothing.
*/

static void _RemoveFromMRUList(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	if (PathData->nextMRU != gcvNULL)
	{
		/* Is this the head node? */
		if (Storage->mruList == PathData)
		{
			/* The only node? */
			if (PathData->nextMRU == PathData)
			{
				/* Reset MRU list. */
				Storage->mruList = gcvNULL;
			}
			else
			{
				/* Update MRU list. */
				Storage->mruList = PathData->prevMRU;
			}
		}

		/* Unlink. */
		PathData->prevMRU->nextMRU = PathData->nextMRU;
		PathData->nextMRU->prevMRU = PathData->prevMRU;

		/* Reset MRU links. */
		PathData->prevMRU = gcvNULL;
		PathData->nextMRU = gcvNULL;
	}
}


/*******************************************************************************
**
** _AppendToBusyList
**
** Add the specified buffer to the list of buffers that are still being
** used by the hardware.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to add to the busy list.
**
** OUTPUT:
**
**    Nothing.
*/

static void _AppendToBusyList(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Cannot be a part of the free and the busy lists. */
	gcmASSERT(PathData->prevFree == gcvNULL);
	gcmASSERT(PathData->nextFree == gcvNULL);
	gcmASSERT(PathData->nextBusy == gcvNULL);

	/* First node to add? */
	if (Storage->busyHead == gcvNULL)
	{
		/* Initialize the list pointer. */
		Storage->busyHead = Storage->busyTail = PathData;
	}

	/* Not the first, add after the tail. */
	else
	{
		/* Add after the tail. */
		Storage->busyTail->nextBusy = PathData;
		Storage->busyTail = PathData;
	}

	/* Terminate the node. */
	PathData->nextBusy = vgvTERMINATOR;
}


/*******************************************************************************
**
** _RemoveFromBusyHead
**
** Remove the head of the list of buffers that are still being used by
** the hardware.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
** OUTPUT:
**
**    Nothing.
*/

static void _RemoveBusyHead(
	IN vgsPATHSTORAGE_PTR Storage
	)
{
	/* Get the head node. */
	vgsPATH_DATA_PTR pathData = Storage->busyHead;

	/* Are there items in the list? */
	if (pathData != gcvNULL)
	{
		/* Get the next node. */
		vgsPATH_DATA_PTR nextPathData = pathData->nextBusy;

		/* Is the item also the tail of the list? */
		if (nextPathData == vgvTERMINATOR)
		{
			gcmASSERT(pathData == Storage->busyTail);

			/* Reset the list. */
			Storage->busyHead = gcvNULL;
			Storage->busyTail = gcvNULL;
		}

		/* Not the tail. */
		else
		{
			/* Update the head. */
			Storage->busyHead = nextPathData;
		}

		/* Reset the node's link. */
		pathData->nextBusy = gcvNULL;
	}
}


/*******************************************************************************
**
** _AppendToFreeList
**
** Add the specified buffer to the free list.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to add to the free list.
**
** OUTPUT:
**
**    Nothing.
*/

static void _AppendToFreeList(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Cannot be a part of the free list already. */
	gcmASSERT(PathData->prevFree == gcvNULL);
	gcmASSERT(PathData->nextFree == gcvNULL);

	/* First node to add? */
	if (Storage->freeHead == gcvNULL)
	{
		/* Terminate the links. */
		PathData->prevFree = vgvTERMINATOR;
		PathData->nextFree = vgvTERMINATOR;

		/* Initialize the list pointer. */
		Storage->freeHead = Storage->freeTail = PathData;
	}

	/* Not the first, add after the tail. */
	else
	{
		/* Initialize the new tail node. */
		PathData->prevFree = Storage->freeTail;
		PathData->nextFree = vgvTERMINATOR;

		/* Add after the tail. */
		Storage->freeTail->nextFree = PathData;
		Storage->freeTail = PathData;
	}
}


/*******************************************************************************
**
** _RemoveFromFreeList
**
** Remove the specified buffer from the free list.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to be removed from the free list.
**
** OUTPUT:
**
**    Nothing.
*/

static void _RemoveFromFreeList(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Has to be a part of the free list. */
	gcmASSERT(PathData->prevFree != gcvNULL);
	gcmASSERT(PathData->nextFree != gcvNULL);

	/* Head node? */
	if (PathData->prevFree == vgvTERMINATOR)
	{
		/* Tail node as well? */
		if (PathData->nextFree == vgvTERMINATOR)
		{
			/* Reset the list pointer. */
			Storage->freeHead = Storage->freeTail = gcvNULL;
		}

		/* No, just the head. */
		else
		{
			/* Update the head. */
			Storage->freeHead = PathData->nextFree;

			/* Terminate the next node. */
			Storage->freeHead->prevFree = vgvTERMINATOR;
		}
	}

	/* Not the head. */
	else
	{
		/* Tail node? */
		if (PathData->nextFree == vgvTERMINATOR)
		{
			/* Update the tail. */
			Storage->freeTail = PathData->prevFree;

			/* Terminate the previous node. */
			Storage->freeTail->nextFree = vgvTERMINATOR;
		}

		/* A node in the middle. */
		else
		{
			/* Remove the node from the list. */
			PathData->prevFree->nextFree = PathData->nextFree;
			PathData->nextFree->prevFree = PathData->prevFree;
		}
	}

	/* Reset free list pointers. */
	PathData->prevFree = gcvNULL;
	PathData->nextFree = gcvNULL;
}


/*******************************************************************************
**
** _CheckBusyList
**
** Scan the list of nodes that are being processed and see if any of them
** became idle.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
** OUTPUT:
**
**    Nothing.
*/

static void _CheckBusyList(
	IN vgsPATHSTORAGE_PTR Storage
	)
{
	/* Get the first node. */
	vgsPATH_DATA_PTR pathData = Storage->busyHead;

	while (gcvTRUE)
	{
		gceSTATUS status;

		/* Empty list? */
		if (pathData == gcvNULL)
		{
			break;
		}

		/* Check if the buffer has been processed. */
		status = gcoHAL_CheckCompletion(Storage->hal, &pathData->data);

		/* Break out of the loop if not. */
		if (gcmIS_ERROR(status))
		{
			break;
		}

		/* Remove the node from the busy list. */
		_RemoveBusyHead(Storage);

		/* Free the node. */
		vgsPATHSTORAGE_Free(Storage, pathData, gcvTRUE);

		/* Get the new first node of the busy list. */
		pathData = Storage->busyHead;
	}
}


/*******************************************************************************
**
** _InitializeBuffer
**
** Initializes the specified command buffer structure.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data to be initialized.
**
**    BufferAddress
**       Hardware address of the buffer.
**
**    Node
**       Memory node where the buffer resides.
**
** OUTPUT:
**
**    Nothing.
*/

static void _InitializeBuffer(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData,
	IN gctUINT32 BufferAddress,
	IN gcuVIDMEM_NODE_PTR Node
	)
{
	gcsCMDBUFFER_PTR buffer;
	gctUINT address;
    gcePOOL pool;

	/* Get a shortcut to the command buffer. */
	buffer = vgmGETCMDBUFFER(PathData);

	/* Split the address into pool and offset. */
	gcmVERIFY_OK(gcoHAL_SplitAddress(
		Storage->hal, BufferAddress, &pool, &address
		));

	/* Determine the hardware buffer address. */
	address = gcmALIGN(
		address + gcmSIZEOF(vgsPATH_DATA),
		Storage->bufferInfo.addressAlignment
		);

	/* Set the proper memory pool. */
	gcmVERIFY_OK(gcoHAL_CombineAddress(
		Storage->hal, pool, address, &address
		));

	/* Initialize gcsCMDBUFFER. */
	buffer->completion    = gcvVACANT_BUFFER;
	buffer->node          = Node;
	buffer->address       = address;
	buffer->bufferOffset  = address - BufferAddress;
	buffer->offset        = Storage->pathInfo.reservedForHead;

	vgmSETPREVSUBBUFFER(PathData, gcvNULL);
	vgmSETNEXTSUBBUFFER(PathData, gcvNULL);

	vgmSETPREVALLOCATED(PathData, gcvNULL);
	vgmSETNEXTALLOCATED(PathData, gcvNULL);

	/* Initialize vgsPATH_DATA. */
	PathData->numSegments  = 0;
	PathData->numCoords    = 0;
	PathData->nextBusy     = gcvNULL;
	PathData->prevMRU      = gcvNULL;
	PathData->nextMRU      = gcvNULL;
	PathData->prevFree     = gcvNULL;
	PathData->nextFree     = gcvNULL;
	PathData->extraManager = gcvNULL;
	PathData->extra        = gcvNULL;
	PathData->path         = gcvNULL;
}


/******************************************************************************\
************************* Memory Manager Implementation ************************
\******************************************************************************/

/*******************************************************************************
**
** vgsPATHSTORAGE_Construct
**
** Construct a memory manager object.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Granularity
**       Specifies the allocation size in bytes.
**
**    AllocationCap
**       The maximum amount of memory that can be allocated by the storage.
**       If set to 0, the allocation will continue as neccessary until there
**       is enough free memory.
*
** OUTPUT:
**
**    Storage
**       Returns the constructed memory manager object.
*/

gceSTATUS vgsPATHSTORAGE_Construct(
	IN vgsCONTEXT_PTR Context,
	IN gctSIZE_T Granularity,
	IN gctSIZE_T AllocationCap,
	OUT vgsPATHSTORAGE_PTR * Storage
	)
{
	gceSTATUS status;
	vgsPATHSTORAGE_PTR storage = gcvNULL;

    gcmHEADER_ARG("Context=0x%x Granularity=0x%x AllocationCap=0x%x Storage=0x%x",
        Context, Granularity, AllocationCap, Storage);

	/* Verify arguments. */
	gcmVERIFY_ARGUMENT(Context != gcvNULL);
	gcmVERIFY_ARGUMENT(Storage != gcvNULL);

	do
	{
		gctUINT bufferOffset;

		/* Allocate the path storage structure. */
		gcmERR_BREAK(gcoOS_Allocate(
			Context->os,
			gcmSIZEOF(vgsPATHSTORAGE),
			(gctPOINTER *) &storage
			));

		/* Initialize the structure. */
		storage->os             = Context->os;
		storage->hal            = Context->hal;
		storage->context        = Context;
		storage->allocationSize = Granularity;
		storage->allocationCap  = AllocationCap;
		storage->allocatedSize  = 0;
		storage->allocatedList  = gcvNULL;
		storage->freeHead       = gcvNULL;
		storage->freeTail       = gcvNULL;
		storage->mruList        = gcvNULL;
		storage->busyHead       = gcvNULL;
		storage->busyTail       = gcvNULL;

#if vgvVALIDATE_PATH_STORAGE
		storage->validateCounter   = 0;
		storage->occupiedByHeaders = 0;
		storage->buffersAllocated  = 0;
		storage->averageHeaderSize = 0;
#endif

		/* Query command buffer attributes. */
		gcmERR_BREAK(gcoHAL_QueryCommandBuffer(
			Context->hal, &storage->bufferInfo
			));

		/* Query path data storage attributes. */
		gcmERR_BREAK(gcoHAL_QueryPathStorage(
			Context->hal, &storage->pathInfo
			));

		/* Determine the buffer offset in an aligned container. */
		bufferOffset = gcmALIGN(
			gcmSIZEOF(vgsPATH_DATA),
			storage->bufferInfo.addressAlignment
			);

		/* Determine the minimum size of the path data buffer. */
		storage->minimumSize
			= storage->pathInfo.reservedForHead
			+ vgvMINPATHSIZE
			+ storage->pathInfo.reservedForTail;

		storage->minimumSize = gcmALIGN(
			storage->minimumSize,
			storage->bufferInfo.commandAlignment
			);

		/* Determine the initial maximum size of an allocated buffer. */
		storage->maximumSize
			= Granularity
			- bufferOffset
			- storage->pathInfo.reservedForTail
			- storage->bufferInfo.staticTailSize
			- gcmSIZEOF(vgsCONTAINER);

		/* Determine the container offset. */
		storage->containerOffset
			= Granularity
			- gcmSIZEOF(vgsCONTAINER);

		/* Set the result. */
		*Storage = storage;

        gcmFOOTER_NO();
		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (storage != gcvNULL)
	{
		gcmVERIFY_OK(gcoOS_Free(Context->os, storage));
	}

    gcmFOOTER();
	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHSTORAGE_Destroy
**
** Free all resources allocated by the memory manager object.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHSTORAGE_Destroy(
	IN vgsPATHSTORAGE_PTR Storage
	)
{
	gceSTATUS status;
	vgsCONTAINER_PTR container;

    gcmHEADER_ARG("Storage=0x%x", Storage);
	/* Verify arguments. */
	gcmVERIFY_ARGUMENT(Storage != gcvNULL);

	/* Assume success. */
	status = gcvSTATUS_OK;

	/* Free idle nodes if any. */
	_CheckBusyList(Storage);

	/* Validate the storage. */
	vgmVALIDATE_STORAGE(Storage);

	/* Destroy all allocated buffers. */
	while (Storage->allocatedList)
	{
		/* Copy the buffer pointer. */
		container = Storage->allocatedList;

#if vgvVALIDATE_PATH_STORAGE
		if (Storage->busyHead == gcvNULL)
		{
			vgsPATH_DATA_PTR pathData;
			gcsCMDBUFFER_PTR buffer;

			/* Get the path data buffer. */
			pathData = (vgsPATH_DATA_PTR)
			(
				(gctUINT8_PTR) container - Storage->containerOffset
			);

			/* Get the command buffer. */
			buffer = vgmGETCMDBUFFER(pathData);

			/* Make sure the first buffer occupies the entire allocated area. */
			gcmASSERT(buffer->size == Storage->maximumSize);

			/* Make sure the first buffer is not allocated. */
			gcmASSERT(pathData->prevFree != gcvNULL);
			gcmASSERT(pathData->nextFree != gcvNULL);
		}
#endif

		/* Free the current container. */
		gcmERR_BREAK(gcoHAL_ScheduleVideoMemory(
			Storage->hal, container->node
			));

		/* Advance to the next one. */
		Storage->allocatedList = container->next;
	}

	/* Free the object structrure. */
	if (gcmIS_SUCCESS(status))
	{
		status = gcoOS_Free(Storage->os, Storage);
	}

    gcmFOOTER();
	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHSTORAGE_Open
**
** Allocate a buffer of no less then the specified size.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    Size
**       Minimum required buffer size. If 0 is specified, the first available
**       buffer is returned regardless of its size.
**
** OUTPUT:
**
**    PathData
**       Pointer to the allocated path data buffer.
*/

gceSTATUS vgsPATHSTORAGE_Open(
	IN vgsPATHSTORAGE_PTR Storage,
	IN gctUINT Size,
	OUT vgsPATH_DATA_PTR * PathData
	)
{
	gceSTATUS status, last;
	gcuVIDMEM_NODE_PTR node = gcvNULL;

    gcmHEADER_ARG("Storage=0x%x Size=0x%x PathData=0x%x",
        Storage, Size, PathData);
	/* Verify arguments. */
	gcmVERIFY_ARGUMENT(Storage != gcvNULL);
	gcmVERIFY_ARGUMENT(PathData != gcvNULL);

	/* Add the reserved size to the request. */
	Size += Storage->pathInfo.reservedForHead;

	/* Make sure the requested size isn't too large. */
	if (Size > Storage->maximumSize)
	{
		return gcvSTATUS_OUT_OF_MEMORY;
	}

	/* See if any of busy nodes became idle. */
	_CheckBusyList(Storage);

	/* Is there a free available buffer? */
	if (Storage->freeHead != gcvNULL)
	{
		vgsPATH_DATA_PTR pathData;
		gcsCMDBUFFER_PTR buffer;

		/* Set the initial free buffer. */
		pathData = Storage->freeHead;

		do
		{
			/* Get the command buffer. */
			buffer = vgmGETCMDBUFFER(pathData);

			/* Is the buffer big enough? */
			if (buffer->size >= Size)
			{
				/* Reset the buffer. */
				gcmASSERT(buffer->completion == gcvVACANT_BUFFER);
				buffer->offset = Storage->pathInfo.reservedForHead;

				vgmSETPREVSUBBUFFER(pathData, gcvNULL);
				vgmSETNEXTSUBBUFFER(pathData, gcvNULL);

				pathData->numSegments  = 0;
				pathData->numCoords    = 0;
				pathData->extraManager = gcvNULL;
				pathData->extra        = gcvNULL;

				/* Remove the buffer from the free list. */
				_RemoveFromFreeList(Storage, pathData);

#if vgvVALIDATE_PATH_STORAGE
				Storage->occupiedByHeaders += pathData->data.data.bufferOffset;
				Storage->buffersAllocated  += 1;
				Storage->averageHeaderSize
					= Storage->occupiedByHeaders
					/ Storage->buffersAllocated;
#endif

				/* Set the result. */
				* PathData = pathData;

				/* Success. */
				return gcvSTATUS_OK;
			}

			/* Get the next free buffer. */
			pathData = pathData->nextFree;
		}
		while (pathData != vgvTERMINATOR);
	}

	/* No available buffers, allocate a new one. */
	do
	{
		gctUINT32 address;
		gctUINT8_PTR memory;
		vgsCONTAINER_PTR container;
		vgsPATH_DATA_PTR pathData;
		gcsCMDBUFFER_PTR buffer;
		gctSIZE_T newAllocatedSize;

		/* Determine new allocated size. */
		newAllocatedSize = Storage->allocatedSize + Storage->allocationSize;

		/* Did we reach the allocation cap? */
		if ((Storage->allocationCap != 0) &&
			(newAllocatedSize > Storage->allocationCap))
		{
			/* Are there MRU items? */
			if (Storage->mruList == gcvNULL)
			{
				/* No MRU list, nothing to free. */
				return gcvSTATUS_OUT_OF_MEMORY;
			}

			/* Get the least recently used (LRU) node. */
			pathData = Storage->mruList->nextMRU;

			/* Notify the path. */
			vgfFreePathDataCallback(pathData->path, pathData);

			/* Free the LRU node. */
			if (!vgsPATHSTORAGE_Free(Storage, pathData, gcvTRUE))
			{
				/* Path is still busy, flush the pipe. */
				gcmERR_BREAK(vgfFlushPipe(Storage->context, gcvTRUE));
			}

			/* Try allocating again. */
			return vgsPATHSTORAGE_Open(Storage, Size, PathData);
		}

		/* Allocate a container. */
		gcmERR_BREAK(gcoHAL_AllocateLinearVideoMemory(
			Storage->hal,
			Storage->allocationSize,
			Storage->bufferInfo.addressAlignment,
			gcvPOOL_DEFAULT,
			&node,
			&address,
			(gctPOINTER *) &memory
			));

		/* Update allocated memory size. */
		Storage->allocatedSize = newAllocatedSize;

		/* Place the container. */
		container = (vgsCONTAINER_PTR)
		(
			memory + Storage->containerOffset
		);

		/* Initialize the container. */
		container->node = node;
		container->next = Storage->allocatedList;

		vgmSET_CONTAINER_NUMBER(Storage->context, container);

		/* Place the first command buffer. */
		pathData = (vgsPATH_DATA_PTR) memory;

		/* Get the command buffer. */
		buffer = vgmGETCMDBUFFER(pathData);

		/* Initialize the path data. */
		_InitializeBuffer(Storage, pathData, address, node);

		/* Set the command buffer size. */
		buffer->size = Storage->maximumSize;

		/* Link in the buffer. */
		Storage->allocatedList = container;

#if vgvVALIDATE_PATH_STORAGE
		Storage->occupiedByHeaders += pathData->data.data.bufferOffset;
		Storage->buffersAllocated  += 1;
		Storage->averageHeaderSize
			= Storage->occupiedByHeaders
			/ Storage->buffersAllocated;
#endif

		/* Set the result. */
		* PathData = pathData;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (node != gcvNULL)
	{
		gcmCHECK_STATUS(gcoHAL_FreeVideoMemory(
			Storage->hal, node
			));
	}

    gcmFOOTER();
	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHSTORAGE_Close
**
** The closing pair for vgsPATHSTORAGE_Open call. Informs the memory manager
** of how many bytes were actually used in the open buffer so that the remaining
** space can be reclaimed back.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data buffer to be closed.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHSTORAGE_Close(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Verify arguments. */
	gcmASSERT(Storage != gcvNULL);
	gcmASSERT(PathData != gcvNULL);

	do
	{
		gcsCMDBUFFER_PTR buffer;
		gctSIZE_T dataSize;
		gctSIZE_T totalSize;

		vgsPATH_DATA_PTR newPathData;
		gcsCMDBUFFER_PTR newBuffer;
		gctUINT newBufferOffset;
		gctINT newBufferSize;

		/* Get the command buffer pointer. */
		buffer = vgmGETCMDBUFFER(PathData);

		/* Determine the final data size. */
		dataSize = gcmALIGN(
			buffer->offset + Storage->pathInfo.reservedForTail,
			Storage->bufferInfo.commandAlignment
			);

		/* Determine the totala size of the data in the buffer. */
		totalSize = dataSize + Storage->bufferInfo.staticTailSize;

		/* Determine the offset of the new buffer. */
		newBufferOffset = gcmALIGN(
			totalSize + gcmSIZEOF(vgsPATH_DATA),
			Storage->bufferInfo.addressAlignment
			);

		/* Determine the size of the new buffer. */
		newBufferSize = buffer->size - newBufferOffset;

		/* Is the buffer big enough to become a separate buffer? */
		if (newBufferSize < 0)
		{
			/* No, not big enough. */
			break;
		}

		/* Place the new path data. */
		newPathData = (vgsPATH_DATA_PTR)
		(
			(gctUINT8_PTR) buffer + buffer->bufferOffset + totalSize
		);

		/* Get the command buffer pointer. */
		newBuffer = vgmGETCMDBUFFER(newPathData);

		/* Initialize the buffer. */
		_InitializeBuffer(
			Storage,
			newPathData,
			buffer->address + totalSize,
			buffer->node
			);

		/* Set the size of the free command buffer. */
		newBuffer->size = newBufferSize;

		/* Link in. */
		_AppendToNodeList(PathData, newPathData);
		_AppendToFreeList(Storage, newPathData);

		/* Trim the used buffer. */
		buffer->size = dataSize - Storage->pathInfo.reservedForTail;
	}
	while (gcvFALSE);
}


/*******************************************************************************
**
** vgsPATHSTORAGE_Free
**
** Free a previously allocated buffer.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data buffer to be freed.
**
**    FreeSubBuffers
**       All subbuffers will be also freed if not zero.
**
** OUTPUT:
**
**    Nothing.
*/

gctBOOL vgsPATHSTORAGE_Free(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData,
	IN gctBOOL FreeSubBuffers
	)
{
	gceSTATUS status;

	vgsPATH_DATA_PTR nextSubBuffer;

	vgsPATH_DATA_PTR prev;
	vgsPATH_DATA_PTR next;

	gcsCMDBUFFER_PTR prevBuffer;
	gcsCMDBUFFER_PTR currBuffer;
	gcsCMDBUFFER_PTR nextBuffer;

	vgsPATH_DATA_PTR mergedPath;
	gcsCMDBUFFER_PTR mergedBuffer;
	gctSIZE_T mergedSize;

	/* Verify arguments. */
	gcmASSERT(Storage != gcvNULL);
	gcmASSERT(PathData != gcvNULL);
	gcmASSERT(PathData->nextBusy == gcvNULL);
	gcmASSERT(PathData->prevFree == gcvNULL);
	gcmASSERT(PathData->nextFree == gcvNULL);

	/* Remove from the MRU list. */
	_RemoveFromMRUList(Storage, PathData);

	/* Is the path still being processed? */
	status = gcoHAL_CheckCompletion(Storage->hal, &PathData->data);

	if (gcmIS_ERROR(status))
	{
		/* Add the node to the busy list. */
		_AppendToBusyList(Storage, PathData);

		/* See if any of busy nodes became idle. */
		_CheckBusyList(Storage);

		/* Done; path has not been physically freed yet. */
		return gcvFALSE;
	}

	do
	{
		/* Get the next subbuffer. */
		nextSubBuffer = vgmGETNEXTSUBBUFFER(PathData);

		/* Release completion signal. */
		gcmVERIFY_OK(gcoHAL_DeassociateCompletion(
			Storage->hal, &PathData->data
			));

		/* Free extra information. */
		if (PathData->extra != gcvNULL)
		{
			vgsMEMORYMANAGER_Free(
				PathData->extraManager,
				PathData->extra
				);
		}

#if vgvVALIDATE_PATH_STORAGE
		Storage->occupiedByHeaders -= PathData->data.data.bufferOffset;
		Storage->buffersAllocated  -= 1;
		Storage->averageHeaderSize = (Storage->buffersAllocated == 0)
			? 0
			: Storage->occupiedByHeaders / Storage->buffersAllocated;
#endif

		/* Get shortcuts to the previous and next path data buffers. */
		prev = vgmGETPREVALLOCATED(PathData);
		next = vgmGETNEXTALLOCATED(PathData);

		/* Get command buffer pointers. */
		prevBuffer = vgmGETCMDBUFFER(prev);
		currBuffer = vgmGETCMDBUFFER(PathData);
		nextBuffer = vgmGETCMDBUFFER(next);

		/* Is the previous path data buffer already free? */
		if (prev && prev->nextFree)
		{
			/* The previous path data buffer is the one that remains. */
			mergedPath = prev;
			mergedBuffer = prevBuffer;

			/* Is the next path data buffer already free? */
			if (next && next->nextFree)
			{
				/* Merge all three path data buffers into the previous. */
				mergedSize
					= prevBuffer->size
					+ currBuffer->size
					+ nextBuffer->size
					+ Storage->pathInfo.reservedForTail * 2
					+ Storage->bufferInfo.staticTailSize * 2
					+ currBuffer->bufferOffset
					+ nextBuffer->bufferOffset;

				/* Remove the next path data buffer. */
				_RemoveFromFreeList(Storage, next);
				_RemoveFromNodeList(Storage, next);
			}
			else
			{
				/* Merge the current path data buffer into the previous. */
				mergedSize
					= prevBuffer->size
					+ currBuffer->size
					+ Storage->pathInfo.reservedForTail
					+ Storage->bufferInfo.staticTailSize
					+ currBuffer->bufferOffset;
			}

			/* Delete the current path data buffer. */
			_RemoveFromNodeList(Storage, PathData);

			/* Set new size. */
			mergedBuffer->size = mergedSize;
		}
		else
		{
			/* The current path data buffer is the one that remains. */
			mergedPath = PathData;
			mergedBuffer = currBuffer;

			/* Is the next buffer already free? */
			if (next && next->nextFree)
			{
				/* Merge the next into the current. */
				mergedSize
					= currBuffer->size
					+ nextBuffer->size
					+ Storage->pathInfo.reservedForTail
					+ Storage->bufferInfo.staticTailSize
					+ nextBuffer->bufferOffset;

				/* Remove the next buffer. */
				_RemoveFromFreeList(Storage, next);
				_RemoveFromNodeList(Storage, next);

				/* Set new size. */
				mergedBuffer->size = mergedSize;
			}

			/* Add the current buffer into the free list. */
			_AppendToFreeList(Storage, mergedPath);

			/* Reset the subbuffer connections. */
			vgmSETPREVSUBBUFFER(PathData, gcvNULL);
			vgmSETNEXTSUBBUFFER(PathData, gcvNULL);
		}

		/* Advance to the next subbuffer. */
		PathData = nextSubBuffer;
	}
	while (FreeSubBuffers && (PathData != gcvNULL));

	/* Done; path has been freed. */
	return gcvTRUE;
}


/*******************************************************************************
**
** vgsPATHSTORAGE_UpdateMRU
**
** Move the path to top of MRU list.
**
** INPUT:
**
**    Storage
**       Pointer to the memory manager object.
**
**    PathData
**       Pointer to the path data buffer to be freed.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHSTORAGE_UpdateMRU(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Is it not currently the MRU? */
	if (Storage->mruList != PathData)
	{
		/* Remove from the list. */
		_RemoveFromMRUList(Storage, PathData);

		/* Add back to make the path MRU. */
		_AppendToMRUList(Storage, PathData);
	}
}

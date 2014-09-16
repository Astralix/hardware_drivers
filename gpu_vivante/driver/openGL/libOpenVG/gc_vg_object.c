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

typedef void (* vgtSET_OBJECT_LIST) (
	vgsOBJECT_LIST_PTR ListEntry
	);


/*******************************************************************************
**
** _ReferenceObjectCache
**
** Increment the reference count of the specified object cache. If object
** cache has not been created yet, it will be first created and then its
** counter incremented.
**
** INPUT:
**
**    ObjectCache
**       Pointer to the object cache pointer.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _ReferenceObjectCache(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_CACHE_PTR * ObjectCache
	)
{
	gceSTATUS status, last;
	vgsOBJECT_CACHE_PTR objectCache = gcvNULL;

	do
	{
		/* Has the object cache been created? */
		if (*ObjectCache == gcvNULL)
		{
			static vgtSET_OBJECT_LIST initList[] =
			{
				vgfSetPathObjectList,
				vgfSetImageObjectList,
				vgfSetMaskObjectList,
				vgfSetFontObjectList,
				vgfSetPaintObjectList
			};

			gctUINT i;

			/* Allocate the context structure. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				gcmSIZEOF(vgsOBJECT_CACHE),
				(gctPOINTER *) &objectCache
				));

			gcmERR_BREAK(gcoOS_ZeroMemory(
				objectCache,
				gcmSIZEOF(vgsOBJECT_CACHE)));

			/* Initialize the object. */
			objectCache->loHandle       = ~0;
			objectCache->hiHandle       =  0;
			objectCache->referenceCount =  0;

			/* Initialize object arrays. */
			for (i = 0; i < gcmCOUNTOF(initList); i++)
			{
				initList[i] (&objectCache->cache[i]);
			}

			/* Set the result pointer. */
			*ObjectCache = objectCache;
		}

		/* Increment the counter. */
		(*ObjectCache)->referenceCount++;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (objectCache != gcvNULL)
	{
		gcmCHECK_STATUS(gcoOS_Free(Context->os, objectCache));
	}

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _DereferenceObjectCache
**
** Decrement the reference count of the specified object cache. If the
** reference counter becomes zero, the object cache will be destroyed together
** with any objects that are still in the cache.
**
** INPUT:
**
**    ObjectCache
**       Pointer to the object cache pointer.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _DereferenceObjectCache(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_CACHE_PTR * ObjectCache
	)
{
	/* Define the result and assume success. */
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		vgsOBJECT_PTR object;
		vgsOBJECT_LIST_PTR objectList;

		/* Get a shortcut to the object cache. */
		vgsOBJECT_CACHE_PTR objectCache = *ObjectCache;

		/* Existing object cache? */
		if (objectCache == gcvNULL)
		{
			break;
		}

		/* Valid reference count? */
		if (objectCache->referenceCount < 1)
		{
			gcmFATAL("Invalid reference count found.\n");
			status = gcvSTATUS_INVALID_OBJECT;
			break;
		}

		/* Decrement the counter. */
		objectCache->referenceCount--;

		/* Time to destroy? */
		if (objectCache->referenceCount == 0)
		{
			gctUINT i;

			/* Delete objects that are still in the cache. */
			for (i = 0; i < vgvOBJECTTYPE_COUNT; i++)
			{
				gctUINT32 index;

				/* Get the current object list. */
				objectList = &objectCache->cache[i];

				for (index = 0; index < vgvNAMED_OBJECTS_HASH; index++)
				{
					/* Are there objects in the list? */
					if (objectList->head[index] != gcvNULL)
					{
						gcmTRACE(
							gcvLEVEL_ERROR,
							"%s (%d): object cache %d still has objects in it.\n",
							__FUNCTION__, __LINE__, i
							);

						/* Delete the objects. */
						while (objectList->head[index])
                                        	{
							/* Copy the head object. */
							object = objectList->head[index];

							/* Dereference it. */
							gcmERR_BREAK(vgfDereferenceObject(
								Context,
								&object
								));
						}
					}
				 }

				/* Error? */
				if (gcmIS_ERROR(status))
				{
					break;
				}
			}

			/* Error? */
			if (gcmIS_ERROR(status))
			{
				break;
			}

			/* Allocate the context structure. */
			gcmERR_BREAK(gcoOS_Free(
				Context->os,
				(gctPOINTER *) objectCache
				));

			/* Reset the object. */
			*ObjectCache = gcvNULL;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
************************** Object Cache Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgfObjectCacheStart
**
** Initialize the object cache for the first time during context creation.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    SharedContext
**       Pointer to the shared context.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfObjectCacheStart(
	vgsCONTEXT_PTR Context,
	vgsCONTEXT_PTR SharedContext
	)
{
	gceSTATUS status;

	/* If a shared context is specified, get the cache pointer from it. */
	if (SharedContext != gcvNULL)
	{
		Context->objectCache = SharedContext->objectCache;
	}

	/* Not specified, reset the pointer. */
	else
	{
		Context->objectCache = gcvNULL;
	}

	/* Reference the object cache. */
	status = _ReferenceObjectCache(Context, &Context->objectCache);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfObjectCacheStop
**
** Release resources associated with the object cache during context
** destruction.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfObjectCacheStop(
	vgsCONTEXT_PTR Context
	)
{
	gceSTATUS status;

	/* Dereference object cache. */
	status = _DereferenceObjectCache(
		Context,
		&Context->objectCache
		);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfObjectCacheInsert
**
** Insert an object into the object cache.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Object
**       Pointer to the object to be inserted into the cache.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfObjectCacheInsert(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	)
{
	vgsOBJECT_CACHE_PTR objectCache;
	vgsOBJECT_LIST_PTR objectList;
	vgsOBJECT_PTR head;
	gctUINT32 handle;

	gctUINT32 index;

	gcmASSERT((Context != gcvNULL) && (Object != gcvNULL));
	gcmASSERT(Context->objectCache != gcvNULL);
	gcmASSERT((Object->type >= 0) && (Object->type < vgvOBJECTTYPE_COUNT));

	/* Get a shortcut to the object cache. */
	objectCache = Context->objectCache;

	/* Get a shortcut to the object list. */
	objectList = &objectCache->cache[Object->type];

	objectList->count++;
	Object->name = objectList->count;

	if (Object->name == 0)
	{
		return gcvSTATUS_INVALID_DATA;
	}

	index = Object->name % vgvNAMED_OBJECTS_HASH;

	head = objectList->head[index];

	/* Insert to the head. */
	Object->prev = gcvNULL;

	if (head == gcvNULL)
	{
		Object->next = gcvNULL;
	}
	else
	{
		Object->next = head;
		head->prev = Object;
	}

	objectList->head[index] = Object;

	/* Update the object handle tracking variables. */
	handle = gcmPTR2INT(Object);

	if (handle < objectCache->loHandle)
	{
		objectCache->loHandle = handle;
	}

	if (handle > objectCache->hiHandle)
	{
		objectCache->hiHandle = handle;
	}

	/* Success. */
	return gcvSTATUS_OK;
}


/*******************************************************************************
**
** vgfObjectCacheRemove
**
** Remove an object from the object cache.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Object
**       Pointer to the object to be removed into the cache.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfObjectCacheRemove(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	)
{
	vgsOBJECT_LIST_PTR objectList;

	gctUINT32 index;
	gcmASSERT((Context != gcvNULL) && (Object != gcvNULL));
	gcmASSERT(Context->objectCache != gcvNULL);
	gcmASSERT((Object->type >= 0) && (Object->type < vgvOBJECTTYPE_COUNT));

	/* Get a shortcut to the object list. */
	objectList = &Context->objectCache->cache[Object->type];
	index = Object->name % vgvNAMED_OBJECTS_HASH;

	/* Remove from the chain. */
	if (Object == objectList->head[index])
	{
		objectList->head[index] = Object->next;
	}
	else
	{
		Object->prev->next = Object->next;
	}

	if (Object->next)
	{
		Object->next->prev = Object->prev;
	}

	Object->prev = Object->next = gcvNULL;

	/* Success. */
	return gcvSTATUS_OK;
}


/*******************************************************************************
**
** vgfVerifyObject
**
** Verifies whether the given object is valid within the current context.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object in question.
**
**    Type
**       Object type (set to vgvOBJECTTYPE_COUNT to accept all types).
**
** OUTPUT:
**
**    VGboolean
**       Returns VG_TRUE if the object is valid.
*/

VGboolean vgfVerifyObject(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgeOBJECTTYPE Type
	)
{
	VGboolean result;

	do
	{
		gctUINT32 handle;
		vgsOBJECT_PTR object;
		vgsOBJECT_PTR current;
		vgsOBJECT_CACHE_PTR objectCache;
		vgsOBJECT_LIST_PTR objectList;
		gctUINT32 index;

		/* Cast to handle. */
		handle = gcmPTR2INT(Object);

		/* Get the cache pointer. */
		objectCache = Context->objectCache;

		/* Is the pointer within range of allocated objects? */
		if ((handle < objectCache->loHandle) ||
			(handle > objectCache->hiHandle))
		{
			result = VG_FALSE;
			break;
		}

		/* Cast the object. */
		object = (vgsOBJECT_PTR) Object;

		/* Check the range of the type. */
		if ((object->type < 0) || (object->type >= vgvOBJECTTYPE_COUNT))
		{
			result = VG_FALSE;
			break;
		}

		/* Valid type? */
		if ((Type != vgvOBJECTTYPE_COUNT) && (object->type != Type))
		{
			result = VG_FALSE;
			break;
		}

		/* Get the proper object list. */
		objectList = &objectCache->cache[object->type];

		index = object->name % vgvNAMED_OBJECTS_HASH;
		current = objectList->head[index];

		/* Search the cache for the object. */
		while (current != gcvNULL)
		{
			/* Match? */
			if (object == current)
			{
				break;
			}

			/* Advance the current. */
			current = current->next;
		}

		/* Found a match? */
		if (current == gcvNULL)
		{
			result = VG_FALSE;
			break;
		}

		/* Move to the head of the list. */
		if (current->prev != gcvNULL)
		{
			/* Remove from the chain. */
			current->prev->next = current->next;

			if (current->next != gcvNULL)
			{
				current->next->prev = current->prev;
			}

			/* Insert to the head. */
			current->prev = gcvNULL;
			current->next = objectList->head[index];
			objectList->head[index]->prev = current;
			objectList->head[index] = current;
		}
		/* Valid object. */
		result = VG_TRUE;
	}
	while (gcvFALSE);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vgfVerifyUserObject
**
** Verifies whether the given object is a valid user object within the current
** context.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object in question.
**
**    Type
**       Object type (set to vgvOBJECTTYPE_COUNT to accept all types).
**
** OUTPUT:
**
**    VGboolean
**       Returns VG_TRUE if the object is valid.
*/

VGboolean vgfVerifyUserObject(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgeOBJECTTYPE Type
	)
{
	VGboolean result;

	do
	{
		/* Verify the object handle. */
		if (!vgfVerifyObject(Context, Object, Type))
		{
			result = VG_FALSE;
			break;
		}

		/* Check whether the object is available for the user. */
		if (!((vgsOBJECT_PTR) Object)->userValid)
		{
			result = VG_FALSE;
			break;
		}

		/* Valid user object. */
		result = VG_TRUE;
	}
	while (gcvFALSE);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vgfDereferenceObject
**
** Decrement the reference count of the given object. The object will be
** destroyed when the count reaches zero.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Object
**       Pointer to the object to be dereferenced.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfDereferenceObject(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR * Object
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	gcmASSERT(Context != gcvNULL);
	gcmASSERT(Object != gcvNULL);

	do
	{
		/* Get a shortcut to the object. */
		vgsOBJECT_PTR object = *Object;

		/* Already destroyed? */
		if (object == gcvNULL)
		{
			break;
		}

		/* Valid reference count? */
		if (object->referenceCount < 1)
		{
			gcmFATAL("Invalid reference count found.");
			status = gcvSTATUS_INVALID_OBJECT;
			break;
		}

		/* Decrement the reference count. */
		object->referenceCount--;

		/* Time to destroy? */
		if (object->referenceCount == 0)
		{
			vgsOBJECT_LIST_PTR objectList;

			/* Get a shortcut to the object list. */
			objectList = &Context->objectCache->cache[object->type];

			/* Call the destructor. */
			gcmASSERT(objectList->destructor != gcvNULL);
			gcmERR_BREAK(objectList->destructor(Context, object));

			/* Remove the object from the cache. */
			gcmERR_BREAK(vgfObjectCacheRemove(Context, object));

			/* Free the object. */
			gcmERR_BREAK(gcoOS_Free(Context->os, object));

			/* Reset the pointer. */
			*Object = gcvNULL;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

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




#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE glvZONE_TRACE


/*******************************************************************************
**
**	glfConstructNamedObjectList
**
**	Construct named object list.
**
**	INPUT:
**
**		Context
**			Pointer to the current context.
**
**		List
**			Pointer to the list to be constructed.
**
**		ObjectSize
**			The size of the user-defined object.
**
**	OUTPUT:
**
**		Nothing.
*/

gceSTATUS glfConstructNamedObjectList(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List,
	IN gctUINT32 ObjectSize
	)
{
    gcmHEADER_ARG("Context=0x%x List=0x%x ObjectSize=%u",
                    Context, List, ObjectSize);
	List->objectSize = ObjectSize;
	List->nextName = 1;
    gcmFOOTER_ARG("return=%s", "gcvSTATUS_OK");
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**	glfDestroyNamedObjectList
**
**	Free all memory associated with the specified named object list.
**
**	INPUT:
**
**		Context
**			Pointer to the current context.
**
**		List
**			Pointer to the list to be destroyed.
**
**	OUTPUT:
**
**		Nothing.
*/

gceSTATUS glfDestroyNamedObjectList(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List
	)
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x List=0x%x", Context, List);

	do
	{
		gceSTATUS last;
		gctINT i;

		/* Free all objects in the free list. */
		gcmCHECK_STATUS(glfCompactNamedObjectList(Context, List));

		/* Free all existing objects. */
		for (i = 0; i < glvNAMEDOBJECT_HASHTABLE_SIZE; i++)
		{
			glsNAMEDOBJECT_PTR wrapper = List->hashTable[i];
			while (wrapper)
			{
				glsNAMEDOBJECT_PTR temp = wrapper;
				wrapper = wrapper->next;

                gcmCHECK_STATUS((*temp->deleteObject)(Context, temp->object));
                gcmCHECK_STATUS(gcmOS_SAFE_FREE(gcvNULL, temp));
			}
		}
	}
	while (gcvFALSE);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**	glfCompactNamedObjectList
**
**	Free unused named object wrappers. This functiuon is called if the list
**	destructor is called or there is no more available memory to allocate
**	other objects.
**
**	INPUT:
**
**		Context
**			Pointer to the current context.
**
**		List
**			Pointer to the named object list.
**
**	OUTPUT:
**
**		Nothing.
*/

gceSTATUS glfCompactNamedObjectList(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List
	)
{
	gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x List=0x%x", Context, List);

	do
	{
		gceSTATUS last;

		glsNAMEDOBJECT_PTR wrapper = List->freeList;
		while (wrapper)
		{
			glsNAMEDOBJECT_PTR temp = wrapper;
			wrapper = wrapper->next;

			gcmCHECK_STATUS(gcmOS_SAFE_FREE(gcvNULL, temp));
		}

		List->freeList = gcvNULL;
	}
	while (gcvFALSE);

    gcmFOOTER();

	return status;
}


/*******************************************************************************
**
**	glfFindNamedObject
**
**	Search the named object list for the specified name.
**
**	INPUT:
**
**		List
**			Pointer to the named object list.
**
**		Name
**			Name to be searched for.
**
**	OUTPUT:
**
**		Pointer to the located wrapper; gcvNULL if not found.
*/

glsNAMEDOBJECT_PTR glfFindNamedObject(
	IN glsNAMEDOBJECTLIST_PTR List,
	IN gctUINT32 Name
	)
{
    gcmHEADER_ARG("List=0x%x Name=%u", List, Name);

	/* Allocated objects can never have zero-valued names. */
	if (Name)
	{
		/* Determine the index into the hash table. */
		gctUINT32 index = Name % glvNAMEDOBJECT_HASHTABLE_SIZE;

		/* Get a shortcut to the table entry. */
		glsNAMEDOBJECT_PTR* entry = &List->hashTable[index];

		/* Keep history. */
		glsNAMEDOBJECT_PTR prev = gcvNULL;

		/* Start from the beginning. */
		glsNAMEDOBJECT_PTR curr = *entry;

		while (curr)
		{
			if (curr->name == Name)
			{
				/* Move to the front of the list. */
				if (prev != gcvNULL)
				{
					prev->next = curr->next;

					curr->next = *entry;
					*entry = curr;
				}

                gcmFOOTER_ARG("curr=0x%x", curr);
                return curr;
            }

			/* The current becomes history. */
			prev = curr;

			/* Move to the next one. */
			curr = curr->next;
		}
	}

    gcmFOOTER_ARG("curr=0x%x", gcvNULL);
    return gcvNULL;
}


/*******************************************************************************
**
**	glfCreateNamedObject
**
**	Create a new named object wrapper. In the event when the application
**	explicitly specified the name of the object this function assumes that
**	the object does not exist.
**
**	INPUT:
**
**		Context
**			Pointer to the current context.
**
**		List
**			Pointer to the named object list.
**
**		Name
**			Name of the new object.
**
**		ObjectDestructor
**			Pointer to the object destructor.
**
**	OUTPUT:
**
**		ObjectWrapper
**			Pointer to the new wrapper.
*/

gceSTATUS glfCreateNamedObject(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List,
	IN gctUINT32 Name,
	IN glfNAMEDOBJECTDESTRUCTOR ObjectDestructor,
	OUT glsNAMEDOBJECT_PTR * ObjectWrapper
	)
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x List=0x%x Name=%u ObjectDestructor=0x%x ObjectWrapper=0x%x",
                    Context, List, Name, ObjectDestructor, ObjectWrapper);

	do
	{
		glsNAMEDOBJECT_PTR wrapper;
		glsNAMEDOBJECT_PTR* entry;
		gctUINT32 index;

		/* Are there available wrappers in the free list? */
		wrapper = List->freeList;
		if ((Name == 0) && (wrapper != gcvNULL))
		{
			/* Remove the first wrapper from the list. */
			List->freeList = wrapper->next;
		}

		/* Create a new wrapper. */
		else
		{
			/* Create a new wrapper. */
			wrapper = gcvNULL;

			/* Generate a name if not specified. */
			if (Name == 0)
			{
				if (List->nextName == 0)
				{
					/* No more names. */
					status = gcvSTATUS_OUT_OF_RESOURCES;
					break;
				}

				Name = List->nextName++;
			}
			else if (Name > List->nextName)
			{
				/* Name for the object has been explicitly specified by
				   the application and it is greater then the next automatic
				   would-be name. Update the next name with the value from
				   the application; this creates a hole in the name range. */
				List->nextName = Name + 1;
			}
			else if (List->freeList != gcvNULL)
			{
				/* Name for the object has been explicitly specified by
				   the application and it is less then or equal to the next
				   automatic would-be name.  This means the name most likely
				   lives inside the free list. */
				if (List->freeList->name == Name)
				{
					wrapper        = List->freeList;
					List->freeList = wrapper->next;
				}
				else
				{
					glsNAMEDOBJECT_PTR previous = List->freeList;

					for (wrapper = List->freeList;
						 wrapper != gcvNULL;
						 wrapper = wrapper->next)
					{
						if (wrapper->name == Name)
						{
							previous->next = wrapper->next;
							break;
						}

						previous = wrapper;
					}
				}
			}

			if (wrapper == gcvNULL)
			{
				/* Allocate wrapper and the user object. */
				gcmERR_BREAK(gcoOS_Allocate(
					gcvNULL,
					gcmALIGN(sizeof(glsNAMEDOBJECT), 4) + List->objectSize,
					(gctPOINTER) &wrapper
					));

				/* Set the objects's name. */
				wrapper->name = Name;

				/* Set the object pointer. */
				wrapper->object
					= ((gctUINT8_PTR) wrapper)
					+ gcmALIGN(sizeof(glsNAMEDOBJECT), 4);

                wrapper->referenceCount = 0;

                wrapper->listBelonged = List;
			}
		}

		/* Set the destructor. */
		wrapper->deleteObject = ObjectDestructor;

		/* Determine the hash table index. */
		index = wrapper->name % glvNAMEDOBJECT_HASHTABLE_SIZE;

		/* Shortcut to the table entry. */
		entry = &List->hashTable[index];

		/* Insert into the chain. */
		wrapper->next = *entry;
		*entry = wrapper;

        glfReferenceNamedObject(wrapper);

		/* Set the result. */
		*ObjectWrapper = wrapper;
	}
	while (gcvFALSE);

    gcmFOOTER();

	return status;
}


/*******************************************************************************
**
**	glfDeleteNamedObject
**
**	Delete an existing named object.
**
**	INPUT:
**
**		Context
**			Pointer to the current context.
**
**		List
**			Pointer to the named object list.
**
**		Name
**			Name of the new object.
**
**	OUTPUT:
**
**		Nothing.
*/

gceSTATUS glfDeleteNamedObject(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List,
	gctUINT32 Name
	)
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x List=0x%x Name=%u",
                    Context, List, Name);

	do
	{
		/* Determine the hash table index. */
		gctUINT32 index = Name % glvNAMEDOBJECT_HASHTABLE_SIZE;

		/* Shortcut to the table entry. */
		glsNAMEDOBJECT_PTR* entry = &List->hashTable[index];

		/* Keep history. */
		glsNAMEDOBJECT_PTR prev = gcvNULL;

		/* Search from the beginning. */
		glsNAMEDOBJECT_PTR curr = *entry;

		/* Search collided object list. */
		while (curr)
        {
            if (curr->name == Name)
            {
                /* Remove the wrapper from the list of allocated objects. */
                if (prev)
                {
                    prev->next = curr->next;
                }
                else
                {
                    *entry = curr->next;
                }

                glfDereferenceNamedObject(Context, curr);

                break;
            }

            /* The current becomes history. */
            prev = curr;

            /* Move to the next one. */
            curr = curr->next;
        }
	}
	while (gcvFALSE);

    gcmFOOTER();

	return status;
}

void
glfReferenceNamedObject(
    glsNAMEDOBJECT_PTR Object
    )
{
    if (!Object)
    {
        return;
    }

    Object->referenceCount++;
}

void
glfDereferenceNamedObject(
    IN glsCONTEXT_PTR Context,
    IN glsNAMEDOBJECT_PTR Object
    )
{
    glsNAMEDOBJECTLIST_PTR list;

    if (!Object || !Context)
    {
        return;
    }

    if (--Object->referenceCount == 0)
    {
        list = Object->listBelonged;

        /* Destruct the object. */
        gcmVERIFY_OK((*Object->deleteObject) (Context, Object->object));
        Object->deleteObject = gcvNULL;

        /* Reset the object. */
        gcoOS_ZeroMemory(Object->object, list->objectSize);

        /* Add to the free list. */
        Object->next = list->freeList;
        list->freeList = Object;
    }
}


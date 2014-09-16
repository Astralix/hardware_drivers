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


#include "gc_glsh_precomp.h"

GLboolean
_glshInsertObject(
	GLObjectList * List,
	GLObject Object,
	GLObjectType Type,
	GLuint Name
	)
{
	GLuint index;

	Object->name = (Name == 0) ? List->lastObject + 1 : Name;
	Object->type = Type;

	if (Object->name == 0)
	{
		return GL_FALSE;
	}

	if (Object->name > List->lastObject)
	{
		List->lastObject = Object->name;
	}

	index = Object->name % gldNAMED_OBJECTS_HASH;

	Object->prev = gcvNULL;
	Object->next = List->objects[index];

	if (Object->next != gcvNULL)
	{
		Object->next->prev = Object;
	}

	List->objects[index] = Object;

	return GL_TRUE;
}

void
_glshRemoveObject(
	GLObjectList * List,
	GLObject Object
	)
{
	GLuint index;

	index = Object->name % gldNAMED_OBJECTS_HASH;

	if (Object->prev != gcvNULL)
	{
		Object->prev->next = Object->next;
	}
	else
	{
		List->objects[index] = Object->next;
	}

	if (Object->next != gcvNULL)
	{
		Object->next->prev = Object->prev;
	}
}

GLObject
_glshFindObject(
	GLObjectList * List,
	GLuint Name
	)
{
	GLuint index;
	GLObject object;

	index = Name % gldNAMED_OBJECTS_HASH;

	for (object = List->objects[index];
		 object != gcvNULL;
		 object = object->next)
	{
		if (object->name == Name)
		{
			if (object->prev != gcvNULL)
			{
				object->prev->next = object->next;
				if (object->next != gcvNULL)
				{
					object->next->prev = object->prev;
				}

				object->prev = gcvNULL;
				object->next = List->objects[index];

				object->next->prev   = object;
				List->objects[index] = object;
			}

			return object;
		}
	}

	return gcvNULL;
}

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


#include "gc_vgsh_precomp.h"

gctBOOL
vgshInsertObject(
	_VGContext    *Context,
	_VGObject     *Object,
	_VGObjectType Type
	)
{
	VGuint index;

	gcmHEADER_ARG("Context=0x%x Object=0x%x Type=%d", Context, Object, Type);

	Object->name = Context->sharedData->objectCount + 1;
	Object->type = Type;

	if (Object->name == 0)
	{
		gcmFOOTER_ARG("return=%s", "FALSE");

		return gcvFALSE;
	}

	Context->sharedData->objectCount = Object->name;

	index = Object->name % NAMED_OBJECTS_HASH;

	Object->prev = gcvNULL;
	Object->next = Context->sharedData->namedObjects[index];

	if (Object->next != gcvNULL)
	{
		Object->next->prev = Object;
	}

	Context->sharedData->namedObjects[index] = Object;

	gcmFOOTER_ARG("return=%s", "FALSE");

	return gcvTRUE;
}

void
vgshRemoveObject(
	_VGContext   *Context,
	_VGObject    *Object
	)
{
	VGuint index;

	gcmHEADER_ARG("Context=0x%x Object=0x%x", Context, Object);

	index = Object->name % NAMED_OBJECTS_HASH;

	if (Object->prev != gcvNULL)
	{
		Object->prev->next = Object->next;
	}
	else
	{
		Context->sharedData->namedObjects[index] = Object->next;
	}

	if (Object->next != gcvNULL)
	{
		Object->next->prev = Object->prev;
	}

	gcmFOOTER_NO();
}

_VGObject *
vgshFindObject(
	_VGContext  *Context,
	VGuint      Name
	)
{
	VGuint index;
	_VGObject* object;

	gcmHEADER_ARG("Context=0x%x Name=%u", Context, Name);

	index = Name % NAMED_OBJECTS_HASH;

	for (object = Context->sharedData->namedObjects[index];
		 object != gcvNULL;
		 object = object->next)
	{
		if (object->name == (VGint) Name)
		{
			if (object->prev != gcvNULL)
			{
				object->prev->next = object->next;
				if (object->next != gcvNULL)
				{
					object->next->prev = object->prev;
				}

				object->prev = gcvNULL;
				object->next = Context->sharedData->namedObjects[index];

				object->next->prev           = object;
				Context->sharedData->namedObjects[index] = object;
			}

			gcmFOOTER_ARG("return=0x%x", object);
			return object;
		}
	}

	gcmFOOTER_ARG("return=0x%x", gcvNULL);
	return gcvNULL;
}

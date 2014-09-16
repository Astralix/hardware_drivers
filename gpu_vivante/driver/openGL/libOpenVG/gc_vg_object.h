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




#ifndef __gc_vg_object_h__
#define __gc_vg_object_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*------------------------- Object structure pointer. ------------------------*/

typedef struct _vgsOBJECT * vgsOBJECT_PTR;

/*----------------------------------------------------------------------------*/
/*----------------------- Object destructor definition. ----------------------*/

typedef gceSTATUS (* vgtOBJECTDESTRUCTOR) (
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	);

/*----------------------------------------------------------------------------*/
/*----------------------- Object structure definition. -----------------------*/

typedef struct _vgsOBJECT
{
	/* OpenVG object signature. */
	vgeOBJECTTYPE			type;

	/* Pointer to the previous and next objects in the cache. */
	vgsOBJECT_PTR			prev;
	vgsOBJECT_PTR			next;

	/* User count for object sharing. */
	VGint					referenceCount;

	/* Valid from the user space. */
	VGboolean			userValid;
	gctUINT32			name;
}
vgsOBJECT;

/*----------------------------------------------------------------------------*/
/*---------------------- Object cache type definitions. ----------------------*/

typedef struct _vgsOBJECT_LIST * vgsOBJECT_LIST_PTR;
typedef struct _vgsOBJECT_LIST
{
	/* State array. */
	vgsSTATEENTRY_PTR		stateArray;
	VGint				stateArraySize;

	/* Object destructor. */
	vgtOBJECTDESTRUCTOR		destructor;
	/* Pointer to the first object in the cache. */
	vgsOBJECT_PTR			head[vgvNAMED_OBJECTS_HASH];
	gctUINT32			count;
}
vgsOBJECT_LIST;

/*----------------------------------------------------------------------------*/
/*---------------------- Object cache header structure. ----------------------*/

typedef struct _vgsOBJECT_CACHE * vgsOBJECT_CACHE_PTR;
typedef struct _vgsOBJECT_CACHE
{
	/* The smallest and the biggest allocated handle values. */
	gctUINT32				loHandle;
	gctUINT32				hiHandle;

	/* Cache array, one entry for each object type. */
	vgsOBJECT_LIST				cache[vgvOBJECTTYPE_COUNT];

	/* User count for object cache sharing between contexts. */
	VGint					referenceCount;
}
vgsOBJECT_CACHE;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgfObjectCacheStart(
	vgsCONTEXT_PTR Context,
	vgsCONTEXT_PTR SharedContext
	);

gceSTATUS vgfObjectCacheStop(
	vgsCONTEXT_PTR Context
	);

gceSTATUS vgfObjectCacheInsert(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	);

gceSTATUS vgfObjectCacheRemove(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	);

VGboolean vgfVerifyObject(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgeOBJECTTYPE Type
	);

VGboolean vgfVerifyUserObject(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgeOBJECTTYPE Type
	);

gceSTATUS vgfDereferenceObject(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR * Object
	);

#ifdef __cplusplus
}
#endif

#endif

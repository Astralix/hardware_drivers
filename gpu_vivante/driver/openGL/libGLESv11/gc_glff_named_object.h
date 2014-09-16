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




#ifndef __gc_glff_named_object_h_
#define __gc_glff_named_object_h_

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************\
************************* Number of hash table entries. ************************
\******************************************************************************/

#define glvNAMEDOBJECT_HASHTABLE_SIZE 32


/******************************************************************************\
************************ Object destructor function type. **********************
\******************************************************************************/

typedef gceSTATUS (*glfNAMEDOBJECTDESTRUCTOR) (
	IN glsCONTEXT_PTR Context,
	IN gctPOINTER Object
	);


/******************************************************************************\
************************ Named object wrapper structure. ***********************
\******************************************************************************/

typedef struct _glsNAMEDOBJECTLIST * glsNAMEDOBJECTLIST_PTR;
typedef struct _glsNAMEDOBJECT * glsNAMEDOBJECT_PTR;
typedef struct _glsNAMEDOBJECT
{
	gctUINT32					name;
	gctPOINTER					object;
	glfNAMEDOBJECTDESTRUCTOR	deleteObject;
	glsNAMEDOBJECT_PTR			next;
	gctUINT32					referenceCount;
    glsNAMEDOBJECTLIST_PTR      listBelonged;
}
glsNAMEDOBJECT;


/******************************************************************************\
************************* Named object list structure. *************************
\******************************************************************************/

typedef struct _glsNAMEDOBJECTLIST
{
	gctUINT32					objectSize;
	gctUINT32					nextName;
	glsNAMEDOBJECT_PTR			freeList;
	glsNAMEDOBJECT_PTR			hashTable[glvNAMEDOBJECT_HASHTABLE_SIZE];
}
glsNAMEDOBJECTLIST;


/******************************************************************************\
******************************* Named object API. ******************************
\******************************************************************************/

gceSTATUS glfConstructNamedObjectList(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List,
	IN gctUINT32 ObjectSize
	);

gceSTATUS glfDestroyNamedObjectList(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List
	);

gceSTATUS glfCompactNamedObjectList(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List
	);

glsNAMEDOBJECT_PTR glfFindNamedObject(
	IN glsNAMEDOBJECTLIST_PTR List,
	IN gctUINT32 Name
	);

gceSTATUS glfCreateNamedObject(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List,
	IN gctUINT32 Name,
	IN glfNAMEDOBJECTDESTRUCTOR ObjectDestructor,
	OUT glsNAMEDOBJECT_PTR * ObjectWrapper
	);

gceSTATUS glfDeleteNamedObject(
	IN glsCONTEXT_PTR Context,
	IN glsNAMEDOBJECTLIST_PTR List,
	gctUINT32 Name
	);

void glfReferenceNamedObject(
	IN glsNAMEDOBJECT_PTR ObjectWrapper
    );

void glfDereferenceNamedObject(
    IN glsCONTEXT_PTR Context,
    IN glsNAMEDOBJECT_PTR ObjectWrapper
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_named_object_h_ */

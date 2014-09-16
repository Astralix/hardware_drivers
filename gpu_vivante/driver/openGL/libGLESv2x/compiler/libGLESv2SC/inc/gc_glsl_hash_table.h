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


#ifndef __gc_glsl_hash_table_h_
#define __gc_glsl_hash_table_h_

#include "gc_glsl_link_list.h"

typedef enum _sleHASH_TABLE_BUCKET_SIZE
{
    slvHTBS_TINY        = 53,
    slvHTBS_SMALL       = 101,
    slvHTBS_NORMAL      = 211,
    slvHTBS_LARGE       = 401,
    slvHTBS_HUGE        = 1009
}
sleHASH_TABLE_BUCKET_SIZE;

typedef gctUINT		gctHASH_VALUE;

gctHASH_VALUE
slHashString(
	IN gctCONST_STRING String
	);

/* Hash table */
typedef struct _slsHASH_TABLE
{
	slsDLINK_NODE	buckets[slvHTBS_NORMAL];
}
slsHASH_TABLE;

#define slmBUCKET_INDEX(hashValue)		((gctHASH_VALUE)(hashValue) % slvHTBS_NORMAL)

#define slsHASH_TABLE_Initialize(hashTable) \
	do \
	{ \
		gctINT i; \
		for (i = 0; i < slvHTBS_NORMAL; i++) \
		{ \
			slsDLINK_LIST_Initialize(&((hashTable)->buckets[i])); \
		} \
	} \
	while (gcvFALSE)

#define slsHASH_TABLE_Bucket(hashTable, index) \
	((hashTable)->buckets + (index))

#define slsHASH_TABLE_Insert(hashTable, hashValue, node) \
	slsDLINK_LIST_InsertFirst( \
		&((hashTable)->buckets[slmBUCKET_INDEX(hashValue)]), \
		node)

#define slsHASH_TABLE_Detach(hashTable, node) \
	slsDLINK_NODE_Detach(node)

#define FOR_EACH_HASH_BUCKET(hashTable, iter) \
	for ((iter) = (hashTable)->buckets; \
		(iter) < (hashTable)->buckets + slvHTBS_NORMAL; \
		(iter)++)

#endif /* __gc_glsl_hash_table_h_ */


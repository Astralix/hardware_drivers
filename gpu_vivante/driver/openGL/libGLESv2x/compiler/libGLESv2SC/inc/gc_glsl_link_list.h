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


#ifndef __gc_glsl_list_h_
#define __gc_glsl_list_h_

#define INVALID_ADDRESS			((void *)(long)0xcccccccc)

/* slsDLINK_NODE */
typedef struct _slsDLINK_NODE
{
	struct _slsDLINK_NODE *	prev;
	struct _slsDLINK_NODE *	next;
}
slsDLINK_NODE;

#define slsDLINK_NODE_InsertNext(thisNode, nextNode) \
	do \
	{ \
		(nextNode)->prev		= (thisNode); \
		(nextNode)->next		= (thisNode)->next; \
		(thisNode)->next->prev	= (nextNode); \
		(thisNode)->next		= (nextNode); \
	} \
	while (gcvFALSE)

#define slsDLINK_NODE_InsertPrev(thisNode, prevNode) \
	do \
	{ \
		(prevNode)->prev		= (thisNode)->prev; \
		(prevNode)->next		= (thisNode); \
		(thisNode)->prev->next	= (prevNode); \
		(thisNode)->prev		= (prevNode); \
	} \
	while (gcvFALSE)

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
#define slsDLINK_NODE_Invalid(node) \
	do \
	{ \
		(node)->prev	= (slsDLINK_NODE *)INVALID_ADDRESS; \
		(node)->next	= (slsDLINK_NODE *)INVALID_ADDRESS; \
	} \
	while (gcvFALSE)
#else
#define slsDLINK_NODE_Invalid(node) \
	do \
	{ \
	} \
	while (gcvFALSE)
#endif /* gcmIS_DEBUG(gcdDEBUG_ASSERT) */

#define slsDLINK_NODE_Detach(node) \
	do \
	{ \
		(node)->prev->next	= (node)->next; \
		(node)->next->prev	= (node)->prev; \
		slsDLINK_NODE_Invalid(node); \
	} \
	while (gcvFALSE)

#define slsDLINK_NODE_Next(node, nodeType)		((nodeType *)((node)->next))

#define slsDLINK_NODE_Prev(node, nodeType)		((nodeType *)((node)->prev))

/* slsDLINK_LIST */
typedef slsDLINK_NODE		slsDLINK_LIST;

#define slsDLINK_LIST_Invalid(list)				slsDLINK_NODE_Invalid(list)

#define slsDLINK_LIST_Initialize(list) \
	do \
	{ \
		(list)->prev = (list); \
		(list)->next = (list); \
	} \
	while (gcvFALSE)

#define slsDLINK_LIST_IsEmpty(list)	\
	((list)->next == (list))

#define slsDLINK_LIST_InsertFirst(list, firstNode) \
	slsDLINK_NODE_InsertNext((list), (firstNode))

#define slsDLINK_LIST_InsertLast(list, lastNode) \
	slsDLINK_NODE_InsertPrev((list), (lastNode))

#define slsDLINK_LIST_First(list, nodeType)	\
	slsDLINK_NODE_Next((list), nodeType)

#define slsDLINK_LIST_Last(list, nodeType) \
	slsDLINK_NODE_Prev((list), nodeType)

#define slsDLINK_LIST_DetachFirst(list, nodeType, firstNode) \
	do \
	{ \
		*(firstNode) = slsDLINK_LIST_First((list), nodeType); \
		slsDLINK_NODE_Detach((slsDLINK_NODE *)*(firstNode)); \
	} \
	while (gcvFALSE)

#define slsDLINK_LIST_DetachLast(list, nodeType, lastNode) \
	do \
	{ \
		*(lastNode) = slsDLINK_LIST_Last((list), nodeType); \
		slsDLINK_NODE_Detach((slsDLINK_NODE *)*(lastNode)); \
	} \
	while (gcvFALSE)

#define FOR_EACH_DLINK_NODE(list, nodeType, iter) \
	for ((iter) = (nodeType *)(list)->next; \
		(slsDLINK_NODE *)(iter) != (list); \
		(iter) = (nodeType *)((slsDLINK_NODE *)(iter))->next)

#define FOR_EACH_DLINK_NODE_REVERSELY(list, nodeType, iter) \
	for ((iter) = (nodeType *)(list)->prev; \
		(slsDLINK_NODE *)(iter) != (list); \
		(iter) = (nodeType *)((slsDLINK_NODE *)(iter))->prev)

#define slsDLINK_NODE_COUNT(list, count) \
    {\
        slsDLINK_NODE* iter; \
        count = 0; \
	    for ((iter) = (list)->next; (iter) != (list); (iter) = (iter)->next)\
            count ++;\
    }

/* slsSLINK_NODE */
typedef struct _slsSLINK_NODE
{
	struct _slsSLINK_NODE *	next;
}
slsSLINK_NODE;

#define slsSLINK_NODE_InsertNext(thisNode, nextNode) \
	do \
	{ \
		(nextNode)->next	= (thisNode)->next; \
		(thisNode)->next	= (nextNode); \
	} \
	while (gcvFALSE)

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
#define slsSLINK_NODE_Invalid(node) \
	do \
	{ \
		(node)->next	= (slsDLINK_NODE *)INVALID_ADDRESS; \
	} \
	while (gcvFALSE)
#else
#define slsSLINK_NODE_Invalid(node) \
	do \
	{ \
	} \
	while (gcvFALSE)
#endif /* gcmIS_DEBUG(gcdDEBUG_ASSERT) */

#define slsSLINK_NODE_Detach(thisNode, prevNode) \
	do \
	{ \
		(prevNode)->next	= (thisNode)->next; \
		slsSLINK_NODE_Invalid(thisNode); \
	} \
	while (gcvFALSE)

#define slsSLINK_NODE_Next(node, nodeType)		((nodeType *)((node)->next))

/* slsSLINK_LIST */
typedef slsSLINK_NODE		slsSLINK_LIST;

#define slsSLINK_LIST_Invalid(list)				slsSLINK_NODE_Invalid(list)

#define slsSLINK_LIST_Initialize(list) \
	do \
	{ \
		(list)->next = (list); \
	} \
	while (gcvFALSE)

#define slsSLINK_LIST_IsEmpty(list)	\
	((list)->next == (list))

#define slsSLINK_LIST_InsertFirst(list, firstNode) \
	slsSLINK_NODE_InsertNext((list), (firstNode))

#define slsSLINK_LIST_First(list, nodeType)	\
	slsSLINK_NODE_Next((list), nodeType)

#define slsSLINK_LIST_DetachFirst(list, nodeType, firstNode) \
	do \
	{ \
		*(firstNode) = slsSLINK_LIST_First((list), nodeType); \
		slsSLINK_NODE_Detach((slsDLINK_NODE *)*(firstNode), (list)); \
	} \
	while (gcvFALSE)

#define FOR_EACH_SLINK_NODE(list, nodeType, iter) \
	for ((iter) = (nodeType *)(list)->next; \
		(slsSLINK_NODE *)(iter) != (list); \
		(iter) = (nodeType *)((slsSLINK_NODE *)(iter))->next)

#endif /* __gc_glsl_list_h_ */


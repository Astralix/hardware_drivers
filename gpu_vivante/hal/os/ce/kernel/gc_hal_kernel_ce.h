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




#include "gc_hal.h"
#ifdef EMULATOR
#	define DDK
#	include <aqcm.hpp>
#	include <aqMain.hpp>
#endif

#ifdef MEMORY_STAT

typedef struct _gcsOBJ_NODE
{
	struct _gcsOBJ_NODE *	prev;

	struct _gcsOBJ_NODE *	next;

	gctUINT					id;
}
gcsOBJ_NODE;

#define gcsOBJ_NODE_InsertPrev(thisNode, prevNode) \
	do \
	{ \
		(prevNode)->prev		= (thisNode)->prev; \
		(prevNode)->next		= (thisNode); \
		(thisNode)->prev->next	= (prevNode); \
		(thisNode)->prev		= (prevNode); \
	} \
	while (gcvFALSE)


#define gcsOBJ_NODE_Detach(node) \
	do \
	{ \
		(node)->prev->next	= (node)->next; \
		(node)->next->prev	= (node)->prev; \
	} \
	while (gcvFALSE)

typedef gcsOBJ_NODE			gcsOBJ_LIST;

#define gcsOBJ_LIST_Invalid(list)				gcsOBJ_NODE_Invalid(list)

#define gcsOBJ_LIST_Initialize(list) \
	do \
	{ \
		(list)->prev	= (list); \
		(list)->next	= (list); \
		(list)->id		= 0; \
	} \
	while (gcvFALSE)

#define gcsOBJ_LIST_AddNode(list, node) \
	do \
	{ \
		(node)->id = (list)->id++; \
		gcsOBJ_NODE_InsertPrev((list), (node)); \
	} \
	while (gcvFALSE)

#define FOR_EACH_OBJ_NODE(list, iter) \
	for ((iter) = (gcsOBJ_NODE *)(list)->next; \
		(gcsOBJ_NODE *)(iter) != (list); \
		(iter) = (gcsOBJ_NODE *)((gcsOBJ_NODE *)(iter))->next)

#endif // MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT

struct gcsPHYSICAL
{
#ifdef MEMORY_STAT
	gcsOBJ_NODE				node;
#endif // MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT

	// Physical address.
	PHYSICAL_ADDRESS		physical;

	// Number of bytes.
	gctSIZE_T				bytes;

	// Mapped address of physical memory.
	gctPOINTER				logical;
#if UNDER_CE >= 600
	// Mapped address to user space.
	gctPOINTER				usrlogical;
#endif
	// Reference counter.
	gctINT					reference;
};
#if UNDER_CE >= 600
struct PROCESS_VIDEO_MAP {
	DWORD processId;
	PVOID logical;
	gctSIZE_T size;
	PROCESS_VIDEO_MAP * next;
};
#endif

#define INVALID_IRQ_NO		((DWORD)-1)

class GCHAL
{
protected:
	static gctINT	m_ReferenceCount;

public:
	static gctINT
	AddReference(
		IN LPCTSTR RegistryPath
		)
	{
		if (m_ReferenceCount++ == 0)
		{
			gcmkASSERT(m_Object == gcvNULL);
			try
			{
				m_Object = new GCHAL(RegistryPath);
			}
			catch(bool)
			{
				// Zero out reference count on error.
				m_ReferenceCount = 0;
			}
		}

		return m_ReferenceCount;
	}

	static void
	Release(
		void
		)
	{
		if (--m_ReferenceCount == 0)
		{
			gcmkASSERT(m_Object != gcvNULL);
			delete m_Object;

			m_Object = gcvNULL;
		}
	}

public:
	GCHAL(
		IN LPCTSTR RegistryPath
		);

	~GCHAL(
		void
		);

	bool
	ReadRegistry(
		IN LPCTSTR RegistryPath
		);

public:
	static GCHAL *
	GetObject(
		void
		)
	{
		return m_Object;
	}

protected:
	gckOS			m_Os;
	gckKERNEL		m_Kernels[gcdCORE_COUNT];
    gceCORE         m_CoreMapping[8];
	static GCHAL *	m_Object;

public:
	gckOS
	GetOs(
		void
		)
	{
		return m_Os;
	}

	gckKERNEL
	GetKernel(
		gceCORE Core
		)
	{
		return m_Kernels[Core];
	}

	gceCORE
	GetCore(
		gceHARDWARE_TYPE Type
		)
	{
		return m_CoreMapping[Type];
	}

protected:
	gctSIZE_T		m_SystemMemorySize;
	gctUINT32		m_SystemMemoryBaseAddress;

public:
	gctUINT32
	GetSystemBaseAddress(
		void
		)
	{
		return m_SystemMemoryBaseAddress;
	}

	bool
	AllocateMemory(
		IN gctSIZE_T Bytes,
		OUT gctPOINTER& Logical,
		OUT PHYSICAL_ADDRESS& Physical
		);

	bool FreeMemory(
		IN gctSIZE_T Bytes,
		IN gctPOINTER Logical,
		IN PHYSICAL_ADDRESS Physical
		);

#if UNDER_CE >= 600
protected:
	struct
	{
		PHYSICAL_ADDRESS physical;
		gctSIZE_T		 bytes;
		gctPOINTER       logical;
		gctPOINTER		 mapping;
	}
	m_Mappings[8];
#endif

#ifdef EMULATOR
protected:
	struct
	{
		gctUINT32	address;
		gctPOINTER	mapped;
	}
	m_LastMapping;

public:
	gctUINT32
	Read(
		gctPOINTER Logical,
		gctUINT32 Offset
		);

	void
	Write(
		gctPOINTER Logical,
		gctUINT32 Offset,
		gctUINT8 Mask,
		gctUINT32 Data
		);

	bool
	Map(
		gctUINT32& Address,
		gctPOINTER& Logical
		);
#endif

protected:
	DWORD		m_MemBases[gcdCORE_COUNT];
#ifndef EMULATOR
	gctUINT8 *	m_RegisterBases[gcdCORE_COUNT];

public:
	volatile gctUINT32 *
	GetRegisterAddress(
		gceCORE Core,
		gctUINT32 Address
		)
	{
		return (volatile gctUINT32 *) (m_RegisterBases[Core] + Address);
	}
#endif

protected:
	gcsPHYSICAL	m_Internal;
	gckVIDMEM	m_InternalHeap;

public:
	gctSIZE_T
	GetInternalSize(
		void
		)
	{
		return m_Internal.bytes;
	}

	gctPHYS_ADDR
	GetInternalPhysical(
		void
		)
	{
		return (gctPHYS_ADDR) &m_Internal;
	}

	gctPOINTER
	GetInternalLogical(
		void
		)
	{
		return m_Internal.logical;
	}

	gckVIDMEM
	GetInternalHeap(
		void
		)
	{
		return m_InternalHeap;
	}

protected:
	gcsPHYSICAL	m_External;
	gckVIDMEM	m_ExternalHeap;

public:
	gctSIZE_T
	GetExternalSize(
		void
		)
	{
		return m_External.bytes;
	}

	gctPHYS_ADDR
	GetExternalPhysical(
		void
		)
	{
		return (gctPHYS_ADDR) &m_External;
	}

	gctPOINTER
	GetExternalLogical(
		void
		)
	{
		return m_External.logical;
	}

	gckVIDMEM
	GetExternalHeap(
		void
		)
	{
		return m_ExternalHeap;
	}

protected:
	gcsPHYSICAL	m_Contiguous;
	gckVIDMEM	m_ContiguousHeap;

public:
	gctSIZE_T
	GetContiguousSize(
		void
		)
	{
		return m_Contiguous.bytes;
	}

	gctPHYS_ADDR
	GetContiguousPhysical(
		void
		)
	{
		return (gctPHYS_ADDR) &m_Contiguous;
	}

	gctPOINTER
	GetContiguousLogical(
		void
		)
	{
		return m_Contiguous.logical;
	}
#if UNDER_CE >= 600
	gctPOINTER
	GetProcessContiguousLogical(
		void
		);

	gctUINT32 GetCurrentPID();
#endif
	gckVIDMEM
	GetContiguousHeap(
		void
		)
	{
		return m_ContiguousHeap;
	}

#if UNDER_CE >= 600
protected:
	struct PROCESS
	{
		PROCESS * next;

		DWORD process;
		gcsPHYSICAL *Physical;
		gctSIZE_T bytes;

		gctPOINTER logical;
		gctINT reference;
	}
	* m_Processes;
#endif

public:
	gceSTATUS
	MapInUser(
		IN gcsPHYSICAL * Physical,
		OUT gctPOINTER * Logical
		);

	gceSTATUS
	UnmapFromUser(
		IN gcsPHYSICAL * Physical,
		IN gctPOINTER Logical
		);

protected:
	DWORD  m_InterruptIDs[gcdCORE_COUNT];
	HANDLE m_InterruptEvents[gcdCORE_COUNT];
	HANDLE m_InterruptThreads[gcdCORE_COUNT];
#ifdef EMULATOR
	HANDLE m_WaitEvent;
	HANDLE m_CModelThread;
#endif

public:
	void
	StartThreads(
		void
		);

	void
	StopThreads(
		gceCORE core
		);

#ifdef EMULATOR
	void
	WaitFunction(
		gctUINT32 Delay
		);

	void
	Interrupt(
		void
		);

	void
	UnFreeze(
		void
		)
	{
		SetEvent(m_WaitEvent);
	}

	static DWORD
	CModelLoop(
		LPVOID Parameter
		);
#endif

	static DWORD
	InterruptLoop(
		LPVOID Parameter
		);

	static DWORD
	InterruptLoop2D(
		LPVOID Parameter
		);

	static DWORD
	InterruptLoopVG(
		LPVOID Parameter
		);

#if UNDER_CE >= 600
	static PROCESS_VIDEO_MAP* m_Process_Video_Map;
#endif

protected:
	DWORD		m_BaseAddress;
public:
	gctUINT32	GetBaseAddress() const
	{
		return m_BaseAddress;
	}
};

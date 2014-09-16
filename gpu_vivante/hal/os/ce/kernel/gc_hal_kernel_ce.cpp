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




#include <windows.h>
#include <ceddk.h>
#include <devload.h>
#include <nkintr.h>
#include "gc_hal_kernel_ce.h"
#include "gc_hal_kernel.h"
#include "gc_hal_driver.h"

#if UNDER_CE >= 600
static DWORD g_ProcessID = TLS_OUT_OF_INDEXES;
static gctUINT32 g_KernelPID = 0;

gctUINT32 GCHAL::GetCurrentPID()
{
	return (gctUINT32)TlsGetValue(g_ProcessID);
}
#endif

// Reference counter.
GCHAL * GCHAL::m_Object = gcvNULL;
gctINT GCHAL::m_ReferenceCount = 0;
#if UNDER_CE >= 600
PROCESS_VIDEO_MAP* GCHAL::m_Process_Video_Map = NULL;

gctPOINTER GCHAL::GetProcessContiguousLogical(void)
{
	DWORD processId = GetCallerVMProcessId();
	PROCESS_VIDEO_MAP * pMap = GCHAL::m_Process_Video_Map;

	while (pMap != NULL)
	{
		if (pMap->processId == processId)
			break;
		else
			pMap = pMap->next;
	}
	ASSERT(pMap);

	return pMap->logical;
}
#endif

gctBOOL WINAPI
DllMain(
	HANDLE Instance,
	DWORD Reason,
	LPVOID Reserved
	)
{
	return gcvTRUE;
}

// Size of register space.
const gctSIZE_T c_RegisterSize = 256 << 10;

bool
GCHAL::AllocateMemory(
	IN gctSIZE_T Bytes,
	OUT gctPOINTER& Logical,
	OUT PHYSICAL_ADDRESS& Physical
	)
{
	// Allocate contiguous physical memory.
	gctPOINTER contiguous = AllocPhysMem(Bytes,
										 PAGE_READWRITE | PAGE_NOCACHE,
										 0,
										 0,
										 &Physical.LowPart);

	if (contiguous == gcvNULL)
	{
		// Out of memory.
		return false;
	}

	// Reset upper 32-bits of physical address.
	Physical.HighPart = 0;

#if UNDER_CE >= 600

	// Reserve the shared memory.
	Logical = VirtualAlloc(gcvNULL, Bytes, MEM_RESERVE, PAGE_NOACCESS);

	if (Logical == gcvNULL)
	{
		// Free the physical memory.
		FreePhysMem(contiguous);

		// Out of memory.
		return false;
	}

	// Copy the contiguous pages into shared memory.
	if (!VirtualCopy(Logical,
					 LPVOID(Physical.LowPart>>8),
					 Bytes,
					 PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
	{
		// Free the shared memory.
		VirtualFree(Logical, 0, MEM_RELEASE);

		// Free the contiguous memory.
		FreePhysMem(contiguous);

		// Out of memory.
		return false;
	}


	// Save mapping in table.
	for (int i = 0; i < gcmCOUNTOF(m_Mappings); ++i)
	{
		if (m_Mappings[i].logical == gcvNULL)
		{
			// Save mapping.
			m_Mappings[i].physical = Physical;
			m_Mappings[i].bytes    = Bytes;
			m_Mappings[i].logical  = contiguous;
			m_Mappings[i].mapping  = Logical;
			break;
		}

		// Make sure we don't overflow.
		gcmkASSERT(i + 1 < gcmCOUNTOF(m_Mappings));
	}
#else
	// Reserve the shared memory.
	Logical = VirtualAlloc(gcvNULL, Bytes, MEM_RESERVE, PAGE_NOACCESS);

	if (Logical == gcvNULL)
	{
		// Free the physical memory.
		FreePhysMem(contiguous);

		// Out of memory.
		return false;
	}

	// Copy the contiguous pages into shared memory.
	if (!VirtualCopy(Logical,
					 contiguous,
					 Bytes,
					 PAGE_READWRITE | PAGE_NOCACHE))
	{
		// Free the shared memory.
		VirtualFree(Logical, 0, MEM_RELEASE);

		// Free the contiguous memory.
		FreePhysMem(contiguous);

        return false;
	}

	// Don't need the original memory anymore.
	FreePhysMem(contiguous);
#endif

    gcmkASSERT(Physical.LowPart >= m_BaseAddress);
    Physical.LowPart -= m_BaseAddress;
	gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
				  "GCHAL: Allocated %u bytes of memory @ %08X(%p)",
				  Bytes,
				  Physical.LowPart,
				  Logical);

	// Success.
	return true;
}

bool GCHAL::FreeMemory(
	IN gctSIZE_T Bytes,
	IN gctPOINTER Logical,
	IN PHYSICAL_ADDRESS Physical
	)
{
#if UNDER_CE >= 600

	gcmkVERIFY(VirtualFree(Logical, Bytes, MEM_DECOMMIT));
	gcmkVERIFY(VirtualFree(Logical, 0, MEM_RELEASE));

	// Process all mappings.
	for (int i = 0; i < gcmCOUNTOF(m_Mappings); ++i)
	{
		// See if this slot has the physical address.
		if ( (m_Mappings[i].logical != gcvNULL) &&
			 (m_Mappings[i].physical.QuadPart == Physical.QuadPart) )
		{
			// Free the physical memory.
			FreePhysMem(m_Mappings[i].logical);
			m_Mappings[i].logical = gcvNULL;
			return true;
		}
	}

	// Memory not found.
	return false;
#else

    Physical.LowPart += m_BaseAddress;
	// Free the virtual memory.
	return VirtualFree(Logical, Bytes, MEM_DECOMMIT | MEM_RELEASE) != FALSE;
#endif
}

// GCHAL constructor.
GCHAL::GCHAL(
	LPCTSTR RegistryPath
	)
{
	int i;
	gceSTATUS status;
	gceHARDWARE_TYPE type;
	gckDB sharedDB = gcvNULL;

	// Zero out the heaps.
	m_Internal.bytes   = 0;
	m_InternalHeap     = gcvNULL;
	m_External.bytes   = 0;
	m_ExternalHeap     = gcvNULL;
	m_Contiguous.bytes = 0;
	m_ContiguousHeap   = gcvNULL;

	// Read parameters from the registry.
	if (!ReadRegistry(RegistryPath))
	{
		throw false;
	}

#if UNDER_CE >= 600
	for (i = 0; i < gcmCOUNTOF(m_Mappings); ++i)
	{
		m_Mappings[i].logical = gcvNULL;
	}

	// No processes attached.
	m_Processes = gcvNULL;
#endif

#ifdef EMULATOR
	// No mapped address.
	m_LastMapping.mapped = gcvNULL;

#if !defined(DYNAMIC_MEMORY_CONTROLLER)
	// Reset C-Model.
	_Memory = new AQMEMORY;
	Reset();

	// Set context for AQMEMORY.
	LocalMemory.m_Hal = this;
#else
	// Create C-Model.
	Reset();

	// Set the emulator instance.
	MemoryCtrl->m_Hal = this;
#endif

	// Create the events.
	m_WaitEvent = CreateEvent(gcvNULL, gcvFALSE, gcvFALSE, gcvNULL);

	// No threads defined.
	m_CModelThread = gcvNULL;
#else
	// Start mapping at the base address of the chip.
	PHYSICAL_ADDRESS physical;

	for (i = 0; i < gcdCORE_COUNT; i++)
	{
		if (m_MemBases[i] != 0)
		{
			physical.QuadPart = m_MemBases[i];

			// Map the registers.
#if gcdDEBUG
			m_RegisterBases[i] = (gctUINT8*) MmMapIoSpace(physical, 2<<20, gcvFALSE);
#else
			m_RegisterBases[i] = (gctUINT8*) MmMapIoSpace(physical, c_RegisterSize, gcvFALSE);
#endif

			gcmkASSERT(m_RegisterBases[i] != gcvNULL);
			gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
					   "GCHAL: Registers allocated at %p", m_RegisterBases[i]);

			// Update physical address.
			physical.QuadPart += c_RegisterSize;
		}
		else
		{
			m_RegisterBases[i] = gcvNULL;
		}
#endif
	}

	// Create interrupt event.
	for (i = 0; i < gcdCORE_COUNT; i++)
	{
		if (m_InterruptIDs[i] != INVALID_IRQ_NO)
		{
			m_InterruptEvents[i]  = CreateEvent(gcvNULL, gcvFALSE, gcvFALSE, gcvNULL);
		}

		m_InterruptThreads[i] = gcvNULL;
	}

	// Construct the gckOS object.
	gcmkVERIFY_OK(gckOS_Construct(this, &m_Os));

	if (m_InterruptIDs[gcvCORE_MAJOR] != INVALID_IRQ_NO)
	{
		// Construct the gckKERNEL object.
		gcmkVERIFY_OK(gckKERNEL_Construct(m_Os,
										  gcvCORE_MAJOR,
										  this,
										  gcvNULL,
										  &m_Kernels[gcvCORE_MAJOR]));

        sharedDB = m_Kernels[gcvCORE_MAJOR]->db;

        /* Initialize core mapping */
        for (i = 0; i < 8; i++)
        {
            m_CoreMapping[i] = gcvCORE_MAJOR;
        }

		// Start the command queue.
		gcmkVERIFY_OK(gckCOMMAND_Start(m_Kernels[gcvCORE_MAJOR]->command));
	}
	else
	{
		m_Kernels[gcvCORE_MAJOR] = gcvNULL;
	}

	if (m_InterruptIDs[gcvCORE_2D] != INVALID_IRQ_NO)
	{
		// Construct the gckKERNEL object.
		gcmkVERIFY_OK(gckKERNEL_Construct(m_Os,
										  gcvCORE_2D,
										  this,
										  sharedDB,
										  &m_Kernels[gcvCORE_2D]));

        if (sharedDB == gcvNULL) sharedDB = m_Kernels[gcvCORE_2D]->db;

        /* Verify the hardware type */
        gcmkONERROR(gckHARDWARE_GetType(m_Kernels[gcvCORE_2D]->hardware, &type));

        if (type != gcvHARDWARE_2D)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): Unexpected hardware type: %d\n",
                __FUNCTION__, __LINE__,
                type
                );

            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        /* Initialize core mapping */
        if (m_Kernels[gcvCORE_MAJOR] == gcvNULL)
        {
            for (i = 0; i < 8; i++)
            {
                m_CoreMapping[i] = gcvCORE_2D;
            }
        }
        else
        {
            m_CoreMapping[gcvHARDWARE_2D] = gcvCORE_2D;
        }

		// Start the command queue.
		gcmkVERIFY_OK(gckCOMMAND_Start(m_Kernels[gcvCORE_2D]->command));
	}
	else
	{
		m_Kernels[gcvCORE_2D] = gcvNULL;
	}

	if (m_InterruptIDs[gcvCORE_VG] != INVALID_IRQ_NO)
	{
#if gcdENABLE_VG
        gcmkONERROR(gckKERNEL_Construct(m_Os,
									    gcvCORE_VG,
										this,
            							sharedDB,
										&m_Kernels[gcvCORE_VG]));


        /* Initialize core mapping */
        if (m_Kernels[gcvCORE_MAJOR] == gcvNULL
			&& m_Kernels[gcvCORE_2D] == gcvNULL)
        {
            for (i = 0; i < 8; i++)
            {
                m_CoreMapping[i] = gcvCORE_VG;
            }
        }
        else
        {
            m_CoreMapping[gcvHARDWARE_VG] = gcvCORE_VG;
        }
#else
        m_Kernels[gcvCORE_VG] = gcvNULL;
#endif
	}
	else
	{
		m_Kernels[gcvCORE_VG] = gcvNULL;
	}

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (m_Kernels[i] != gcvNULL) break;
    }

    if (i == gcdCORE_COUNT) gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);

	gctUINT32 internalBaseAddress, internalAlignment;
	gctUINT32 externalBaseAddress, externalAlignment;
	gctUINT32 horizontalTileSize, verticalTileSize;

#if gcdENABLE_VG
    if (i == gcvCORE_VG)
    {
        // Query the ceiling of the system memory.
        gcmkONERROR(gckVGHARDWARE_QuerySystemMemory(m_Kernels[i]->vg->hardware,
													&m_SystemMemorySize,
													&m_SystemMemoryBaseAddress));

        // Query the amount of video memory.
        gcmkONERROR(gckVGHARDWARE_QueryMemory(m_Kernels[i]->vg->hardware,
											  &m_Internal.bytes,
											  &internalBaseAddress,
											  &internalAlignment,
											  &m_External.bytes,
											  &externalBaseAddress,
											  &externalAlignment,
											  &horizontalTileSize,
											  &verticalTileSize));
	}
    else
#endif
    {
		// Query the ceiling of the system memory.
		gcmkVERIFY_OK(gckHARDWARE_QuerySystemMemory(m_Kernels[i]->hardware,
													&m_SystemMemorySize,
													&m_SystemMemoryBaseAddress));

		// Query the amount of video memory.
		gcmkVERIFY_OK(gckHARDWARE_QueryMemory(m_Kernels[i]->hardware,
											  &m_Internal.bytes,
											  &internalBaseAddress,
											  &internalAlignment,
											  &m_External.bytes,
											  &externalBaseAddress,
											  &externalAlignment,
											  &horizontalTileSize,
											  &verticalTileSize));
	}

	if (m_Internal.bytes > 0)
	{
		// Create the internal memory heap.
		gceSTATUS status = gckVIDMEM_Construct(m_Os,
											   internalBaseAddress,
											   m_Internal.bytes,
											   internalAlignment,
											   0,
											   &m_InternalHeap);

		if (gcmIS_ERROR(status))
		{
			// Error, remove internal heap.
			m_Internal.bytes = 0;
		}
		else
		{
#ifdef EMULATOR
			// Allocate internal memory.
			VERIFY(AllocateMemory(m_Internal.bytes,
								  m_Internal.logical,
								  m_Internal.physical));
#else
			// Map internal memory.
			m_Internal.physical  = physical;
			m_Internal.logical   = MmMapIoSpace(physical,
											    m_Internal.bytes,
											    gcvFALSE);
			physical.QuadPart += m_Internal.bytes;
#endif
			m_Internal.reference = 1;
			gcmkASSERT(m_Internal.logical != gcvNULL);
		}
	}

	if (m_External.bytes > 0)
	{
		// Create the external memory heap.
		gceSTATUS status = gckVIDMEM_Construct(m_Os,
											 externalBaseAddress,
											 m_External.bytes,
											 externalAlignment,
											 0,
											 &m_ExternalHeap);

		if (gcmIS_ERROR(status))
		{
			// Error, remove external heap.
			m_External.bytes = 0;
		}
		else
		{
#ifdef EMULATOR
			// Allocate external memory.
			VERIFY(AllocateMemory(m_External.bytes,
								  m_External.logical,
								  m_External.physical));
#else
			// Map external memory.
			m_External.physical  = physical;
			m_External.logical   = MmMapIoSpace(physical,
												m_External.bytes,
												gcvFALSE);
#endif
			m_External.reference = 1;
			gcmkASSERT(m_External.logical != gcvNULL);
		}
	}

	// Try to allocate contiguous memory, starting at the specified value,
	// decreasing 4MB each time until we proceed.
	for (; m_Contiguous.bytes > 0; m_Contiguous.bytes -= 4 << 20)
	{
		// Allocate contiguous memory. */
		if (AllocateMemory(m_Contiguous.bytes,
						   m_Contiguous.logical,
						   m_Contiguous.physical))
		{
			m_Contiguous.reference = 1;

			/* Construct the contiguous memory heap. */
			gceSTATUS status = gckVIDMEM_Construct(m_Os,
												 m_Contiguous.physical.LowPart |
												 m_SystemMemoryBaseAddress,
												 m_Contiguous.bytes,
												 64,
												 0,
												 &m_ContiguousHeap);

			if (gcmIS_SUCCESS(status))
			{
				/* Success, abort loop. */
				break;
			}

			/* Free allocated contiguous memory. */
			FreeMemory(m_Contiguous.bytes, m_Contiguous.logical, m_Contiguous.physical);
		}
	}

#ifndef EMULATOR
	// Enable the interrupts.
    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (m_InterruptIDs[i] != INVALID_IRQ_NO)
		{
			InterruptInitialize(m_InterruptIDs[i], m_InterruptEvents[i], gcvNULL, 0);
		}
    }

	// Start the interrupt threads.
	StartThreads();

	// Clear any lingering interrupts.
    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (m_InterruptIDs[i] != INVALID_IRQ_NO)
		{
			InterruptDone(m_InterruptIDs[i]);
		}
	}
#endif

	return;

OnError:
	throw false;
}

// GCHAL destructor.
GCHAL::~GCHAL(
	void
	)
{
	int i;

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (m_Kernels[i] != gcvNULL)
		{
			if (i != gcvCORE_VG)
			{
				/* Switch to ON power state. */
				gcmkVERIFY_OK(
					gckHARDWARE_SetPowerManagementState(m_Kernels[i]->hardware,
														gcvPOWER_ON));
			}

#ifndef EMULATOR
			// Disable the interrupt.
			InterruptDisable(m_InterruptIDs[i]);
#endif

			// Stop the threads.
			StopThreads((gceCORE)i);

			// Delete the events.
			CloseHandle(m_InterruptEvents[i]);

			// Destruct the gckKERNEL object.
			gckKERNEL_Destroy(m_Kernels[i]);
		}
	}

	if (m_Internal.bytes > 0)
	{
		// Destroy the internal heap.
		gcmkVERIFY_OK(gckVIDMEM_Destroy(m_InternalHeap));

#ifdef EMULATOR
		// Free the internal memory.
		gcmkVERIFY(FreeMemory(m_Internal.bytes,
							 m_Internal.logical,
							 m_Internal.physical));
#else
		// Unmap the internal memory.
		MmUnmapIoSpace(m_Internal.logical, m_Internal.bytes);
#endif
	}

	if (m_External.bytes > 0)
	{
		// Destroy the external heap.
		gcmkVERIFY_OK(gckVIDMEM_Destroy(m_ExternalHeap));

#ifdef EMULATOR
		// Free the external memory.
		gcmkVERIFY(FreeMemory(m_External.bytes,
							 m_External.logical,
							 m_External.physical));
#else
		// Unmap the external memory.
		MmUnmapIoSpace(m_External.logical, m_External.bytes);
#endif
	}

	if (m_Contiguous.bytes > 0)
	{
		// Destroy the contiguous heap.
		gcmkVERIFY_OK(gckVIDMEM_Destroy(m_ContiguousHeap));

		// Free the contiguous memory.
		gcmkVERIFY(FreeMemory(m_Contiguous.bytes,
							 m_Contiguous.logical,
							 m_Contiguous.physical));
	}

#ifdef EMULATOR
	CloseHandle(m_WaitEvent);

	// Delete C-Model.
	Quit();

#if !defined(DYNAMIC_MEMORY_CONTROLLER)
	delete _Memory;
#endif

	// Unmap any mapped memory.
	if (m_LastMapping.mapped != gcvNULL)
	{
		gcmkVERIFY(DeleteStaticMapping(m_LastMapping.mapped, PAGE_SIZE));
	}
#else
    for (i = 0; i < gcdCORE_COUNT; i++)
    {
		if (m_RegisterBases[i] != gcvNULL)
		{
			// Unmap the register space.
			MmUnmapIoSpace(m_RegisterBases[i], c_RegisterSize);
		}
	}
#endif

	// Destruct the gckOS object.
	gckOS_Destroy(m_Os);
}

extern "C" DWORD
GPU_Init(
	LPCTSTR registryPath
	)
{
	DWORD ret;

#if UNDER_CE >= 600
	g_ProcessID = TlsAlloc();
	gckOS_GetCurrentProcessID(&g_KernelPID);
	gcmkVERIFY(TlsSetValue(g_ProcessID, (LPVOID)g_KernelPID));
#endif

	ret = GCHAL::AddReference(registryPath);

	return ret;
}

extern "C" gctBOOL
GPU_Deinit(
	DWORD channelInterruptLoop
	)
{
#if UNDER_CE >= 600
	TlsFree(g_ProcessID);
    g_ProcessID = TLS_OUT_OF_INDEXES;
	g_KernelPID = 0;
#endif

	GCHAL::Release();
	return gcvTRUE;
}


extern "C" DWORD
GPU_Open(
	DWORD Reference,
	DWORD access,
	DWORD shareMode
	)
{
	int i;

	if ((Reference == IOCTL_GCHAL_KERNEL_INTERFACE)
		&& (access == IOCTL_GCHAL_KERNEL_INTERFACE)
		&& (shareMode == IOCTL_GCHAL_KERNEL_INTERFACE))
	{
		GCHAL *gchal = GCHAL::GetObject();
		if (gchal == gcvNULL)
		{
			return (DWORD)INVALID_HANDLE_VALUE;
		}
		else if (gchal->GetKernel(gcvCORE_MAJOR) == gcvNULL
				 && gchal->GetKernel(gcvCORE_2D) == gcvNULL
				 && gchal->GetKernel(gcvCORE_VG) == gcvNULL)
		{
			return (DWORD)INVALID_HANDLE_VALUE;
		}
	}
#if UNDER_CE >= 600
	else
	{
		DWORD processId = GetCallerVMProcessId();
		PROCESS_VIDEO_MAP * pMap = GCHAL::m_Process_Video_Map;

		while (pMap != NULL)
		{
			if (pMap->processId == processId)
				break;
			else
				pMap = pMap->next;
		}

		if(pMap == NULL)
		{
			pMap = new PROCESS_VIDEO_MAP;
			pMap->processId = processId;
			pMap->size = GCHAL::GetObject()->GetContiguousSize();

			pMap->logical = VirtualAllocEx(
									  HANDLE(processId),
									  NULL,
									  pMap->size,
									  MEM_RESERVE,
									  PAGE_READWRITE | PAGE_NOCACHE
										);
			gcmkASSERT(pMap->logical);

			gcmkVERIFY(VirtualCopyEx(
								HANDLE(processId),
								pMap->logical,
								GetCurrentProcess(),
								GCHAL::GetObject()->GetContiguousLogical(),
								pMap->size,
								PAGE_READWRITE | PAGE_NOCACHE
							));

			pMap->next = GCHAL::m_Process_Video_Map;
			GCHAL::m_Process_Video_Map = pMap;
		}
		else
		{
			gcmkTRACE(gcvLEVEL_ERROR,
				"GPU_Open: warning -- mapping info existed for process: %08x\n", processId);
		}
	}

	gctUINT32 processId;
	gckOS_GetCurrentProcessID(&processId);
	gcmkVERIFY(TlsSetValue(g_ProcessID, (LPVOID)processId));
#endif

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (GCHAL::GetObject()->GetKernel((gceCORE)i) != gcvNULL)
        {
			if (gcmIS_ERROR(gckKERNEL_AttachProcess(GCHAL::GetObject()->GetKernel((gceCORE)i),
												    gcvTRUE)))
			{
				gcmkTRACE(gcvLEVEL_ERROR, "Process attach failed\n");
			}
		}
	}

	return -1;
}

#ifdef MEMORY_STAT
extern "C" void
OnProcessExit(
	IN gckOS Os
	);
#endif // MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT

extern "C" gctBOOL
GPU_Close(
	DWORD Reference
	)
{
	int i;

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (GCHAL::GetObject()->GetKernel((gceCORE)i) != gcvNULL)
        {
			if (gcmIS_ERROR(gckKERNEL_AttachProcess(GCHAL::GetObject()->GetKernel((gceCORE)i),
													gcvFALSE)))
			{
				gcmkTRACE(gcvLEVEL_ERROR, "Process detatch failed\n");
			}
		}
	}

#if UNDER_CE >= 600
	if (Reference != IOCTL_GCHAL_KERNEL_INTERFACE)
	{
		DWORD processId = GetCallerVMProcessId();
		PROCESS_VIDEO_MAP * pMap = GCHAL::m_Process_Video_Map;
		PROCESS_VIDEO_MAP * pre = pMap;

		while (pMap != NULL)
		{
			if (pMap->processId == processId)
				break;
			else
			{
				pre = pMap;
				pMap = pMap->next;
			}
		}

		if (pMap != NULL)
		{
			if (pMap == GCHAL::m_Process_Video_Map)
				GCHAL::m_Process_Video_Map = pMap->next;
			else
			{
				ASSERT(pre != NULL);
				pre->next = pMap->next;
			}
			gcmkVERIFY(VirtualFreeEx(HANDLE(pMap->processId),
										pMap->logical,
										pMap->size,
										MEM_DECOMMIT));

			gcmkVERIFY(VirtualFreeEx(HANDLE(pMap->processId),
										pMap->logical,
										0,
										MEM_RELEASE));
			delete pMap;
		}
		else
		{
			gcmkTRACE(gcvLEVEL_ERROR,
				"GPU_Close: warning -- mapping info missed for process: %08x\n", processId);
		}
	}
#endif

#ifdef MEMORY_STAT
	OnProcessExit(GCHAL::GetObject()->GetOs());
#endif // MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT
// MEMORY_STAT

#if UNDER_CE >= 600
	gcmkVERIFY(TlsSetValue(g_ProcessID, 0));
#endif

	return gcvTRUE;
}

extern "C" DWORD
GPU_Read(
	DWORD Reference,
	LPVOID buffer,
	DWORD numRead
	)
{
	return -1;
}

extern "C" DWORD
GPU_Write(
	DWORD Reference,
	LPCVOID buffer,
	DWORD numBytes
	)
{
	return -1;
}

extern "C" gctBOOL
GPU_IOControl(
	DWORD Reference,
	DWORD IoControlCode,
	PBYTE InputBuffer,
	DWORD InputBufferSize,
	PBYTE OutputBuffer,
	DWORD OutputBufferSize,
	PDWORD ReturnedBytes
	)
{
	gceSTATUS status;

	/* Make sure it is an gcsHAL_INTERFACE call. */
	if ( ((IoControlCode != IOCTL_GCHAL_INTERFACE) &&
		 (IoControlCode != IOCTL_GCHAL_KERNEL_INTERFACE)) ||
		 (InputBufferSize != sizeof(gcsHAL_INTERFACE)) ||
		 (OutputBufferSize != sizeof(gcsHAL_INTERFACE)) )
	{
		/* Error. */
		return gcvFALSE;
	}

#if UNDER_CE >= 600
	gctUINT32 processId;

	gckOS_GetCurrentProcessID(&processId);

	if (IoControlCode == IOCTL_GCHAL_KERNEL_INTERFACE)
	{
		gcmkVERIFY(TlsSetValue(g_ProcessID, (LPVOID)g_KernelPID));
	}
	else
	{
		gcmkVERIFY(TlsSetValue(g_ProcessID, (LPVOID)processId));
	}
#endif

	gcsHAL_INTERFACE * iface = (gcsHAL_INTERFACE *) InputBuffer;

    if (iface->command == gcvHAL_CHIP_INFO)
    {
        int i, count = 0;

        for (i = 0; i < gcdCORE_COUNT; i++)
        {
            if (GCHAL::GetObject()->GetKernel((gceCORE)i) != gcvNULL)
            {
#if gcdENABLE_VG
                if (i == gcvCORE_VG)
                {
                    iface->u.ChipInfo.types[count] = gcvHARDWARE_VG;
                }
                else
#endif
                {
                    gcmkVERIFY_OK(gckHARDWARE_GetType(GCHAL::GetObject()->GetKernel((gceCORE)i)->hardware,
                                                      &iface->u.ChipInfo.types[count]));
                }

                count++;
            }
        }

        iface->u.ChipInfo.count = count;
        iface->status = gcvSTATUS_OK;
    }
    else
    {
        if (iface->hardwareType < 0 || iface->hardwareType > 7)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): unknown hardwareType %d\n",
                __FUNCTION__, __LINE__,
                iface->hardwareType
                );

            iface->status = gcvSTATUS_INVALID_ARGUMENT;
        }
		else
		{
#if gcdENABLE_VG
			if (GCHAL::GetObject()->GetCore(iface->hardwareType) == gcvCORE_VG)
			{
				status = gckVGKERNEL_Dispatch(GCHAL::GetObject()->GetKernel(gcvCORE_VG),
											  (IoControlCode == IOCTL_GCHAL_INTERFACE),
											  iface);
			}
			else
#endif
			{
				/* Dispatch the command to the kernel. */
				status = gckKERNEL_Dispatch(GCHAL::GetObject()->GetKernel(
												GCHAL::GetObject()->GetCore(iface->hardwareType)),
											(IoControlCode == IOCTL_GCHAL_INTERFACE),
											iface);
			}

			if (gcmIS_ERROR(status)) iface->status = status;
		}
    }

#if UNDER_CE >= 600
	if ((IoControlCode == IOCTL_GCHAL_KERNEL_INTERFACE)
		&& (processId != g_KernelPID))
	{
		gcmkVERIFY(TlsSetValue(g_ProcessID, (LPVOID)processId));
	}
#endif

	/* Success. */
	return gcvTRUE;
}

extern "C" void
GPU_PowerDown(
	DWORD hContext
	)
{
    for (int i = 0; i < gcdCORE_COUNT; i++)
    {
        if (GCHAL::GetObject()->GetKernel((gceCORE)i) != gcvNULL)
        {
#if gcdENABLE_VG
            if (i != gcvCORE_VG)
#endif
            {
				if (gcmIS_ERROR(gckHARDWARE_SetPowerManagementState(
						GCHAL::GetObject()->GetKernel((gceCORE)i)->hardware,
						gcvPOWER_OFF)))
				{
					gcmkTRACE(gcvLEVEL_ERROR, "Power off failed\n");
				}
			}
		}
	}
}

extern "C" void
GPU_PowerUp(
	DWORD hContext
	)
{
    for (int i = 0; i < gcdCORE_COUNT; i++)
    {
        if (GCHAL::GetObject()->GetKernel((gceCORE)i) != gcvNULL)
        {
#if gcdENABLE_VG
            if (i != gcvCORE_VG)
#endif
            {
				if (gcmIS_ERROR(gckHARDWARE_SetPowerManagementState(
						GCHAL::GetObject()->GetKernel((gceCORE)i)->hardware,
						gcvPOWER_ON)))
				{
					gcmkTRACE(gcvLEVEL_ERROR, "Power on failed\n");
				}
			}
		}
	}
}

extern "C" DWORD
GPU_Seek(
	DWORD hOpenContext,
	long Amount,
	WORD Type
	)
{
	return -1;
}

void
GCHAL::StartThreads(
	void
	)
{
#ifdef EMULATOR
	if (m_CModelThread == gcvNULL)
	{
		// Create C-Model thread.
		m_CModelThread = CreateThread(gcvNULL, 0, CModelLoop, this, 0, gcvNULL);
	}
#endif

    for (int i = 0; i < gcdCORE_COUNT; i++)
    {
		if (m_InterruptIDs[i] != INVALID_IRQ_NO
			&& m_InterruptThreads[i] == gcvNULL)
		{
			switch (i)
			{
			case gcvCORE_MAJOR:
				// Create interrupt thread.
				m_InterruptThreads[i] = CreateThread(gcvNULL, 0, InterruptLoop, this, 0, gcvNULL);
				break;

			case gcvCORE_2D:
				// Create interrupt thread.
				m_InterruptThreads[i] = CreateThread(gcvNULL, 0, InterruptLoop2D, this, 0, gcvNULL);
				break;

			case gcvCORE_VG:
				// Create interrupt thread for VG.
				m_InterruptThreads[i] = CreateThread(gcvNULL, 0, InterruptLoopVG, this, 0, gcvNULL);
				break;

			default:
				break;
			}

			if (m_InterruptThreads[i] != gcvNULL)
			{
				// Boost the thread priority.
				SetThreadPriority(m_InterruptThreads[i], THREAD_PRIORITY_HIGHEST);
			}
		}
	}
}

void
GCHAL::StopThreads(
	gceCORE Core
	)
{
#ifdef EMULATOR
	if (m_CModelThread != gcvNULL)
	{
		// Terminate C-Model thread.
		TerminateThread(m_CModelThread, 0);
		m_CModelThread = gcvNULL;
	}
#endif

	if (m_InterruptThreads[Core] != gcvNULL)
	{
		// Terminate niterrupt thread.
		TerminateThread(m_InterruptThreads[Core], 0);
		m_InterruptThreads[Core] = gcvNULL;
	}
}

DWORD
GCHAL::InterruptLoop(
	LPVOID Context
	)
{
    gceSTATUS status;
	GCHAL* gchal = (GCHAL*) Context;

	// Wait for interrupt event from C-Model.
	while (WaitForSingleObject(gchal->m_InterruptEvents[gcvCORE_MAJOR],
							   INFINITE) == WAIT_OBJECT_0)
	{
		gcmkVERIFY_OK(gckOS_SuspendInterruptEx(gchal->m_Os, gcvCORE_MAJOR));

        status = gckKERNEL_Notify(gchal->m_Kernels[gcvCORE_MAJOR], gcvNOTIFY_INTERRUPT, gcvTRUE);

		gcmkVERIFY_OK(gckOS_ResumeInterruptEx(gchal->m_Os, gcvCORE_MAJOR));

		// Notify HAL of interrupt.
		if (gcmIS_SUCCESS(status))
		{
			gckKERNEL_Notify(gchal->m_Kernels[gcvCORE_MAJOR], gcvNOTIFY_INTERRUPT, gcvFALSE);
		}

#ifndef EMULATOR
		// Interrupt handled.
		InterruptDone(gchal->m_InterruptIDs[gcvCORE_MAJOR]);
#endif
	}

	// Success.
	return STATUS_SUCCESS;
}

DWORD
GCHAL::InterruptLoop2D(
	LPVOID Context
	)
{
    gceSTATUS status;
	GCHAL* gchal = (GCHAL*) Context;

	// Wait for interrupt event from C-Model.
	while (WaitForSingleObject(gchal->m_InterruptEvents[gcvCORE_2D],
							   INFINITE) == WAIT_OBJECT_0)
	{
		gcmkVERIFY_OK(gckOS_SuspendInterruptEx(gchal->m_Os, gcvCORE_2D));

        status = gckKERNEL_Notify(gchal->m_Kernels[gcvCORE_2D], gcvNOTIFY_INTERRUPT, gcvTRUE);

		gcmkVERIFY_OK(gckOS_ResumeInterruptEx(gchal->m_Os, gcvCORE_2D));

		// Notify HAL of interrupt.
		if (gcmIS_SUCCESS(status))
		{
			gckKERNEL_Notify(gchal->m_Kernels[gcvCORE_2D], gcvNOTIFY_INTERRUPT, gcvFALSE);
		}

#ifndef EMULATOR
		// Interrupt handled.
		InterruptDone(gchal->m_InterruptIDs[gcvCORE_2D]);
#endif
	}

	// Success.
	return STATUS_SUCCESS;
}


DWORD
GCHAL::InterruptLoopVG(
	LPVOID Context
	)
{
	GCHAL* gchal = (GCHAL*) Context;

	// Wait for interrupt event from C-Model.
	while (WaitForSingleObject(gchal->m_InterruptEvents[gcvCORE_VG],
							   INFINITE) == WAIT_OBJECT_0)
	{
		gcmkVERIFY_OK(gckOS_SuspendInterruptEx(gchal->m_Os, gcvCORE_VG));

#if gcdENABLE_VG
		gcmkVERIFY_OK(gckVGINTERRUPT_Enque(gchal->m_Kernels[gcvCORE_VG]->vg->interrupt));
#endif

		gcmkVERIFY_OK(gckOS_ResumeInterruptEx(gchal->m_Os, gcvCORE_VG));

#ifndef EMULATOR
		// Interrupt handled.
		InterruptDone(gchal->m_InterruptIDs[gcvCORE_VG]);
#endif
	}

	// Success.
	return STATUS_SUCCESS;
}

bool
GCHAL::ReadRegistry(
	LPCTSTR RegistryPath
	)
{
	int i;

	// Open the device key.
	HKEY key = OpenDeviceKey(RegistryPath);

	// Initialize all parameters
	for (i = 0; i < gcdCORE_COUNT; i++)
	{
		m_InterruptIDs[i]	= INVALID_IRQ_NO;
		m_MemBases[i]		= 0;
	}

	// Query the SYSINTR value.
	DWORD valueType, valueSize = sizeof(m_InterruptIDs[gcvCORE_MAJOR]);
	DWORD irq;

	if (ERROR_SUCCESS != RegQueryValueEx(key,
										 DEVLOAD_SYSINTR_VALNAME,
										 gcvNULL,
										 &valueType,
										 (LPBYTE) &m_InterruptIDs[gcvCORE_MAJOR],
										 &valueSize))
	{
		RETAILMSG(gcvTRUE,
				  (TEXT("%s: failed to read %s from registry. Try to read %s\r\n"),
				  TEXT("GCHAL::ReadRegistry"),
				  DEVLOAD_SYSINTR_VALNAME, DEVLOAD_IRQ_VALNAME));

		if (ERROR_SUCCESS != RegQueryValueEx(key,
											 DEVLOAD_IRQ_VALNAME,
											 gcvNULL,
											 &valueType,
											 (LPBYTE) &irq,
											 &valueSize))
		{
			RETAILMSG(gcvTRUE,
					  (TEXT("%s: failed to read %s from registry.\r\n"),
					  TEXT("GCHAL::ReadRegistry"),
					  DEVLOAD_IRQ_VALNAME));
		}
		else
		{
			if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
								 &irq,
								 valueSize,
								 &m_InterruptIDs[gcvCORE_MAJOR],
								 valueSize,
								 NULL))
			{
				RETAILMSG(gcvTRUE,
						  (TEXT("%s: ERROR: failed to request sysintr. (%s=%d)\r\n"),
						  TEXT("GCHAL::ReadRegistry"),
						  DEVLOAD_IRQ_VALNAME, irq));

				return false;
			}
		}
	}

	if (ERROR_SUCCESS != RegQueryValueEx(key,
										 TEXT("Irq2D"),
										 gcvNULL,
										 &valueType,
										 (LPBYTE) &irq,
										 &valueSize))
	{
		RETAILMSG(gcvTRUE,
				  (TEXT("%s: failed to read %s from registry.\r\n"),
				  TEXT("GCHAL::ReadRegistry"),
				  TEXT("Irq2D")));
	}
	else
	{
		if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
							 &irq,
							 valueSize,
							 &m_InterruptIDs[gcvCORE_2D],
							 valueSize,
							 NULL))
		{
			RETAILMSG(gcvTRUE,
					  (TEXT("%s: ERROR: failed to request sysintr. (%s=%d)\r\n"),
					  TEXT("GCHAL::ReadRegistry"),
					  TEXT("Irq2D"), irq));

			return false;
		}
	}

	if (ERROR_SUCCESS != RegQueryValueEx(key,
										 TEXT("IrqVG"),
										 gcvNULL,
										 &valueType,
										 (LPBYTE) &irq,
										 &valueSize))
	{
		RETAILMSG(gcvTRUE,
				  (TEXT("%s: failed to read %s from registry.\r\n"),
				  TEXT("GCHAL::ReadRegistry"),
				  TEXT("IrqVG")));
	}
	else
	{
		if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
							 &irq,
							 valueSize,
							 &m_InterruptIDs[gcvCORE_VG],
							 valueSize,
							 NULL))
		{
			RETAILMSG(gcvTRUE,
					  (TEXT("%s: ERROR: failed to request sysintr. (%s=%d)\r\n"),
					  TEXT("GCHAL::ReadRegistry"),
					  TEXT("IrqVG"), irq));

			return false;
		}
	}

	if (m_InterruptIDs[gcvCORE_MAJOR] == INVALID_IRQ_NO
		&& m_InterruptIDs[gcvCORE_2D] == INVALID_IRQ_NO
		&& m_InterruptIDs[gcvCORE_VG] == INVALID_IRQ_NO)
	{
		RETAILMSG(gcvTRUE,
				  (TEXT("%s: failed to read any irq from registry.\r\n"),
				  TEXT("GCHAL::ReadRegistry")));

		return false;
	}

    for (i = 0; i < gcdCORE_COUNT; i++)
	{
		RETAILMSG(gcvTRUE,
				 (TEXT("%s: Irqs[%d] = %d\r\n"),
				  TEXT("GCHAL::ReadRegistry"),
				  i, m_InterruptIDs[i]));
	}

	// Query the MEMBASE values.
	if (m_InterruptIDs[gcvCORE_MAJOR] != INVALID_IRQ_NO)
	{
		valueSize = sizeof(m_MemBases[gcvCORE_MAJOR]);

		if (ERROR_SUCCESS != RegQueryValueEx(key,
											 DEVLOAD_MEMBASE_VALNAME,
											 gcvNULL,
											 &valueType,
											 (LPBYTE) &m_MemBases[gcvCORE_MAJOR],
											 &valueSize))
		{
			RETAILMSG(gcvTRUE,
					  (TEXT("%s: ERROR: failed to read %s from registry.\r\n"),
					  TEXT("GCHAL::ReadRegistry"),
					  DEVLOAD_MEMBASE_VALNAME));

			return false;
		}
	}

	if (m_InterruptIDs[gcvCORE_2D] != INVALID_IRQ_NO)
	{
		valueSize = sizeof(m_MemBases[gcvCORE_2D]);

		if (ERROR_SUCCESS != RegQueryValueEx(key,
											 TEXT("MemBase2D"),
											 gcvNULL,
											 &valueType,
											 (LPBYTE) &m_MemBases[gcvCORE_2D],
											 &valueSize))
		{
			RETAILMSG(gcvTRUE,
					  (TEXT("%s: ERROR: failed to read %s from registry.\r\n"),
					  TEXT("GCHAL::ReadRegistry"),
					  TEXT("MemBase2D")));

			return false;
		}
	}

	if (m_InterruptIDs[gcvCORE_VG] != INVALID_IRQ_NO)
	{
		valueSize = sizeof(m_MemBases[gcvCORE_VG]);

		if (ERROR_SUCCESS != RegQueryValueEx(key,
											 TEXT("MemBaseVG"),
											 gcvNULL,
											 &valueType,
											 (LPBYTE) &m_MemBases[gcvCORE_VG],
											 &valueSize))
		{
			RETAILMSG(gcvTRUE,
					  (TEXT("%s: ERROR: failed to read %s from registry.\r\n"),
					  TEXT("GCHAL::ReadRegistry"),
					  TEXT("MemBaseVG")));

			return false;
		}
	}

    for (i = 0; i < gcdCORE_COUNT; i++)
	{
		RETAILMSG(gcvTRUE,
				 (TEXT("%s: MemBases[%d] = 0x%x\r\n"),
				  TEXT("GCHAL::ReadRegistry"),
				  i, m_MemBases[i]));
	}

	// Query the CONTIGUOUS value.
	valueSize = sizeof(m_Contiguous.bytes);
	if (ERROR_SUCCESS != RegQueryValueEx(key,
										 TEXT("Contiguous"),
										 gcvNULL,
										 &valueType,
										 (LPBYTE) &m_Contiguous.bytes,
										 &valueSize))
	{
		// Set default contiguous memory size.
		m_Contiguous.bytes = 16 << 20;
	}

	// Query the BaseAddress value.
	valueSize = sizeof(m_BaseAddress);
	if (ERROR_SUCCESS != RegQueryValueEx(key,
										 TEXT("BaseAddress"),
										 gcvNULL,
										 &valueType,
										 (LPBYTE) &m_BaseAddress,
										 &valueSize))
	{
		// Set default BaseAddress value.
		m_BaseAddress = 0;
	}


	// Close the device key.
	RegCloseKey(key);

	// Return error status.
	return true;
}

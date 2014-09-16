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




#ifndef __gc_vg_memory_manager_h__
#define __gc_vg_memory_manager_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*--------------------- Memory manager structure pointer. --------------------*/

typedef struct _vgsMEMORYMANAGER * vgsMEMORYMANAGER_PTR;

/*----------------------------------------------------------------------------*/
/*--------------------------- List management API. ---------------------------*/

gceSTATUS vgsMEMORYMANAGER_Construct(
    IN gcoOS Os,
	IN gctUINT ItemSize,
	IN gctUINT Granularity,
	OUT vgsMEMORYMANAGER_PTR * Manager
	);

gceSTATUS vgsMEMORYMANAGER_Destroy(
	IN vgsMEMORYMANAGER_PTR Manager
	);

gceSTATUS vgsMEMORYMANAGER_Allocate(
	IN vgsMEMORYMANAGER_PTR Manager,
	OUT gctPOINTER * Pointer
	);

void vgsMEMORYMANAGER_Free(
	IN vgsMEMORYMANAGER_PTR Manager,
	IN gctPOINTER Pointer
	);

#ifdef __cplusplus
}
#endif

#endif

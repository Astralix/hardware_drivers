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




#ifndef __gc_vg_path_transform_h__
#define __gc_vg_path_transform_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*-------------------- Coordinate access function types. ---------------------*/

/* Segment transformation function parameters. */
#define vgmTRANSFORMPARAMETERS \
	vgsPATHWALKER_PTR Destination, \
	vgsPATHWALKER_PTR Source, \
	vgsMATRIX_PTR Transform

/* Segment transformation function type. */
typedef gceSTATUS (* vgtTRANSFORMFUNCTION) (
	vgmTRANSFORMPARAMETERS
	);

/*----------------------------------------------------------------------------*/
/*-------------------------- Coordinate access API. --------------------------*/

void vgfGetTransformArray(
	vgtTRANSFORMFUNCTION ** Array,
	gctUINT_PTR Count
	);

#ifdef __cplusplus
}
#endif

#endif

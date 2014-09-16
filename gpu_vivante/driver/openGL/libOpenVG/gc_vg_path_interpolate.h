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




#ifndef __gc_vg_path_interpolate_h__
#define __gc_vg_path_interpolate_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*---------------------- Interpolation function type. ------------------------*/

/* Segment normalization function parameters. */
#define vgmINTERPOLATEPARAMETERS \
	vgsPATHWALKER_PTR Destination, \
	gctFLOAT_PTR StartCoordinates, \
	gctFLOAT_PTR EndCoordinates, \
	gctFLOAT Amount

typedef gceSTATUS (* vgtINTERPOLATEFUNCTION) (
	vgmINTERPOLATEPARAMETERS
	);

/*----------------------------------------------------------------------------*/
/*---------------------------- Interpolation API. ----------------------------*/

void vgfGetInterpolationArray(
	vgtINTERPOLATEFUNCTION ** Array,
	gctUINT_PTR Count
	);

#ifdef __cplusplus
}
#endif

#endif

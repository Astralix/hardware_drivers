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

/*=======================================================================*/
/* Wrapper macros.*/

#define vgvSL_CLOSE_PATH			VG_CLOSE_PATH
#define vgvSL_MOVE_TO				VG_MOVE_TO
#define vgvSL_LINE_TO				VG_LINE_TO
#define vgvSL_HLINE_TO				VG_HLINE_TO
#define vgvSL_VLINE_TO				VG_VLINE_TO
#define vgvSL_QUAD_TO				VG_QUAD_TO
#define vgvSL_CUBIC_TO				VG_CUBIC_TO
#define vgvSL_SQUAD_TO				VG_SQUAD_TO
#define vgvSL_SCUBIC_TO				VG_SCUBIC_TO
#define vgvSL_SCCWARC_TO			VG_SCCWARC_TO
#define vgvSL_SCWARC_TO				VG_SCWARC_TO
#define vgvSL_LCCWARC_TO			VG_LCCWARC_TO
#define vgvSL_LCWARC_TO				VG_LCWARC_TO

#define vgvSL_PATH_DATATYPE_S_8		VG_PATH_DATATYPE_S_8
#define vgvSL_PATH_DATATYPE_S_16	VG_PATH_DATATYPE_S_16
#define vgvSL_PATH_DATATYPE_S_32	VG_PATH_DATATYPE_S_32
#define vgvSL_PATH_DATATYPE_F		VG_PATH_DATATYPE_F

#define VGSL_GETPATHSCALE(PATH)				(PATH)->scale
#define VGSL_GETPATHBIAS(PATH)				(PATH)->bias
#define VGSL_GETPATHDATATYPE(PATH)			(PATH)->datatype
#define VGSL_GETPATHDATA(PATH)				(PATH)->data
#define VGSL_GETPATHDATAPOINTER(PATH)		(PATH)->data.items
#define VGSL_GETPATHSEGMENTS(PATH)			(PATH)->segments
#define VGSL_GETPATHSEGMENTSSIZE(PATH)		(PATH)->segments.size
#define VGSL_GETPATHSEGMENTSPOINTER(PATH)	(PATH)->segments.items
#define VGSL_GETPATHFILLBUFFER(PATH)		(PATH)->scanLineFillBuffer
#define VGSL_GETPATHSTROKEBUFFER(PATH)		(PATH)->scanLineStrokeBuffer

#define VGSL_GETCONTEXTFILLRULE(CONTEXT)		(CONTEXT)->fillRule
#define VGSL_GETCONTEXTSTOKELINEWIDTH(CONTEXT)	(CONTEXT)->strokeLineWidth
#define VGSL_GETCONTEXTSTOKECAPSTYLE(CONTEXT)	(CONTEXT)->strokeCapStyle
#define VGSL_GETCONTEXTSTOKEJOINSTYLE(CONTEXT)	(CONTEXT)->strokeJoinStyle
#define VGSL_GETCONTEXTSTOKEMITERLIMIT(CONTEXT)	(CONTEXT)->strokeMiterLimit
#define VGSL_GETCONTEXTSTOKEDASHPHASE(CONTEXT)	(CONTEXT)->strokeDashPhase
#define VGSL_GETCONTEXTSTOKEDASHPHASERESET(CONTEXT)	(CONTEXT)->strokeDashPhaseReset
#define VGSL_GETCONTEXTSTOKEDASHPATTERN(CONTEXT)		(CONTEXT)->strokeDashPattern
#define VGSL_GETCONTEXTSTOKEDASHPATTERNSIZE(CONTEXT)	(CONTEXT)->strokeDashPattern.size
#define VGSL_GETCONTEXTSTOKEDASHPATTERNPOINTER(CONTEXT)	(CONTEXT)->strokeDashPattern.items

/*=======================================================================*/
/* Redefine gcdUSE_VG to 0 to use old driver.*/

#undef gcdUSE_VG
#define gcdUSE_VG				0

/*=======================================================================*/
/* Include scan line tessellation header and code.*/

#if USE_SCAN_LINE

#include "gc_vgsh_scanline.h"
#include "gc_vgsh_scanline.c"
#endif

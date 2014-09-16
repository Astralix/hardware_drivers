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




#include "gc_vg_precomp.h"

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/* Validation macros. */
#if vgvVALIDATE_PATH_WALKER

#	define vgmVERIFY_READER(Walker) \
		gcmASSERT((Walker != gcvNULL) && Walker->reader)

#	define vgmVERIFY_WRITER(Walker) \
		gcmASSERT((Walker != gcvNULL) && !Walker->reader)

#else

#	define vgmVERIFY_READER(Walker)
#	define vgmVERIFY_WRITER(Walker)

#endif

/* Maximum command sizes. */
#define vgvMAXCOMMANDSIZE_INT8	vgmCOMMANDSIZE(6, gctUINT8)
#define vgvMAXCOMMANDSIZE_INT16	vgmCOMMANDSIZE(6, gctUINT16)
#define vgvMAXCOMMANDSIZE_INT32	vgmCOMMANDSIZE(6, gctUINT32)
#define vgvMAXCOMMANDSIZE_FLOAT	vgmCOMMANDSIZE(6, gctFLOAT)

/* Command sizes with VG_PATH_DATATYPE_S_8 coordinates. */
static gctUINT _commandSize_INT8[] =
{
	vgmCOMMANDSIZE(0, gctINT8),              /*   0: gcvVGCMD_END             */
	vgmCOMMANDSIZE(0, gctINT8),              /*   1: gcvVGCMD_CLOSE           */
	vgmCOMMANDSIZE(2, gctINT8),              /*   2: gcvVGCMD_MOVE            */
	vgmCOMMANDSIZE(2, gctINT8),              /*   3: gcvVGCMD_MOVE_REL        */
	vgmCOMMANDSIZE(2, gctINT8),              /*   4: gcvVGCMD_LINE            */
	vgmCOMMANDSIZE(2, gctINT8),              /*   5: gcvVGCMD_LINE_REL        */
	vgmCOMMANDSIZE(4, gctINT8),              /*   6: gcvVGCMD_QUAD            */
	vgmCOMMANDSIZE(4, gctINT8),              /*   7: gcvVGCMD_QUAD_REL        */
	vgmCOMMANDSIZE(6, gctINT8),              /*   8: gcvVGCMD_CUBIC           */
	vgmCOMMANDSIZE(6, gctINT8),              /*   9: gcvVGCMD_CUBIC_REL       */
	vgmCOMMANDSIZE(0, gctINT8),              /*  10: gcvVGCMD_BREAK           */
	0,                                       /*  11: gcvVGCMD_HLINE           */
	0,                                       /*  12: gcvVGCMD_HLINE_REL       */
	0,                                       /*  13: gcvVGCMD_VLINE           */
	0,                                       /*  14: gcvVGCMD_VLINE_REL       */
	0,                                       /*  15: gcvVGCMD_SQUAD           */
	0,                                       /*  16: gcvVGCMD_SQUAD_REL       */
	0,                                       /*  17: gcvVGCMD_SCUBIC          */
	0,                                       /*  18: gcvVGCMD_SCUBIC_REL      */
	0,                                       /*  19: gcvVGCMD_SCCWARC         */
	0,                                       /*  20: gcvVGCMD_SCCWARC_REL     */
	0,                                       /*  21: gcvVGCMD_SCWARC          */
	0,                                       /*  22: gcvVGCMD_SCWARC_REL      */
	0,                                       /*  23: gcvVGCMD_LCCWARC         */
	0,                                       /*  24: gcvVGCMD_LCCWARC_REL     */
	0,                                       /*  25: gcvVGCMD_LCWARC          */
	0,                                       /*  26: gcvVGCMD_LCWARC_REL      */
	0,                                       /*  27: **** R E S E R V E D *****/
	0,                                       /*  28: **** R E S E R V E D *****/
	0,                                       /*  29: **** R E S E R V E D *****/
	0,                                       /*  30: **** R E S E R V E D *****/
	0,                                       /*  31: **** R E S E R V E D *****/

	0,                                       /*  32: ***** I N V A L I D ******/
	0,                                       /*  33: ***** I N V A L I D ******/
	0,                                       /*  34: ***** I N V A L I D ******/
	0,                                       /*  35: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT8),              /*  36: gcvVGCMD_HLINE_EMUL      */
	vgmCOMMANDSIZE(2, gctINT8),              /*  37: gcvVGCMD_HLINE_EMUL_REL  */
	0,                                       /*  38: ***** I N V A L I D ******/
	0,                                       /*  39: ***** I N V A L I D ******/
	0,                                       /*  40: ***** I N V A L I D ******/
	0,                                       /*  41: ***** I N V A L I D ******/
	0,                                       /*  42: ***** I N V A L I D ******/
	0,                                       /*  43: ***** I N V A L I D ******/
	0,                                       /*  44: ***** I N V A L I D ******/
	0,                                       /*  45: ***** I N V A L I D ******/
	0,                                       /*  46: ***** I N V A L I D ******/
	0,                                       /*  47: ***** I N V A L I D ******/
	0,                                       /*  48: ***** I N V A L I D ******/
	0,                                       /*  49: ***** I N V A L I D ******/
	0,                                       /*  50: ***** I N V A L I D ******/
	0,                                       /*  51: ***** I N V A L I D ******/
	0,                                       /*  52: ***** I N V A L I D ******/
	0,                                       /*  53: ***** I N V A L I D ******/
	0,                                       /*  54: ***** I N V A L I D ******/
	0,                                       /*  55: ***** I N V A L I D ******/
	0,                                       /*  56: ***** I N V A L I D ******/
	0,                                       /*  57: ***** I N V A L I D ******/
	0,                                       /*  58: ***** I N V A L I D ******/
	0,                                       /*  59: ***** I N V A L I D ******/
	0,                                       /*  60: ***** I N V A L I D ******/
	0,                                       /*  61: ***** I N V A L I D ******/
	0,                                       /*  62: ***** I N V A L I D ******/
	0,                                       /*  63: ***** I N V A L I D ******/

	0,                                       /*  64: ***** I N V A L I D ******/
	0,                                       /*  65: ***** I N V A L I D ******/
	0,                                       /*  66: ***** I N V A L I D ******/
	0,                                       /*  67: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT8),              /*  68: gcvVGCMD_VLINE_EMUL      */
	vgmCOMMANDSIZE(2, gctINT8),              /*  69: gcvVGCMD_VLINE_EMUL_REL  */
	0,                                       /*  70: ***** I N V A L I D ******/
	0,                                       /*  71: ***** I N V A L I D ******/
	0,                                       /*  72: ***** I N V A L I D ******/
	0,                                       /*  73: ***** I N V A L I D ******/
	0,                                       /*  74: ***** I N V A L I D ******/
	0,                                       /*  75: ***** I N V A L I D ******/
	0,                                       /*  76: ***** I N V A L I D ******/
	0,                                       /*  77: ***** I N V A L I D ******/
	0,                                       /*  78: ***** I N V A L I D ******/
	0,                                       /*  79: ***** I N V A L I D ******/
	0,                                       /*  80: ***** I N V A L I D ******/
	0,                                       /*  81: ***** I N V A L I D ******/
	0,                                       /*  82: ***** I N V A L I D ******/
	0,                                       /*  83: ***** I N V A L I D ******/
	0,                                       /*  84: ***** I N V A L I D ******/
	0,                                       /*  85: ***** I N V A L I D ******/
	0,                                       /*  86: ***** I N V A L I D ******/
	0,                                       /*  87: ***** I N V A L I D ******/
	0,                                       /*  88: ***** I N V A L I D ******/
	0,                                       /*  89: ***** I N V A L I D ******/
	0,                                       /*  90: ***** I N V A L I D ******/
	0,                                       /*  91: ***** I N V A L I D ******/
	0,                                       /*  92: ***** I N V A L I D ******/
	0,                                       /*  93: ***** I N V A L I D ******/
	0,                                       /*  94: ***** I N V A L I D ******/
	0,                                       /*  95: ***** I N V A L I D ******/

	0,                                       /*  96: ***** I N V A L I D ******/
	0,                                       /*  97: ***** I N V A L I D ******/
	0,                                       /*  98: ***** I N V A L I D ******/
	0,                                       /*  99: ***** I N V A L I D ******/
	0,                                       /* 100: ***** I N V A L I D ******/
	0,                                       /* 101: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(4, gctINT8),              /* 102: gcvVGCMD_SQUAD_EMUL      */
	vgmCOMMANDSIZE(4, gctINT8),              /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
	vgmCOMMANDSIZE(6, gctINT8),              /* 104: gcvVGCMD_SCUBIC_EMUL     */
	vgmCOMMANDSIZE(6, gctINT8),              /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
	0,                                       /* 106: ***** I N V A L I D ******/
	0,                                       /* 107: ***** I N V A L I D ******/
	0,                                       /* 108: ***** I N V A L I D ******/
	0,                                       /* 109: ***** I N V A L I D ******/
	0,                                       /* 110: ***** I N V A L I D ******/
	0,                                       /* 111: ***** I N V A L I D ******/
	0,                                       /* 112: ***** I N V A L I D ******/
	0,                                       /* 113: ***** I N V A L I D ******/
	0,                                       /* 114: ***** I N V A L I D ******/
	0,                                       /* 115: ***** I N V A L I D ******/
	0,                                       /* 116: ***** I N V A L I D ******/
	0,                                       /* 117: ***** I N V A L I D ******/
	0,                                       /* 118: ***** I N V A L I D ******/
	0,                                       /* 119: ***** I N V A L I D ******/
	0,                                       /* 120: ***** I N V A L I D ******/
	0,                                       /* 121: ***** I N V A L I D ******/
	0,                                       /* 122: ***** I N V A L I D ******/
	0,                                       /* 123: ***** I N V A L I D ******/
	0,                                       /* 124: ***** I N V A L I D ******/
	0,                                       /* 125: ***** I N V A L I D ******/
	0,                                       /* 126: ***** I N V A L I D ******/
	0,                                       /* 127: ***** I N V A L I D ******/

	0,                                       /* 128: ***** I N V A L I D ******/
	0,                                       /* 129: ***** I N V A L I D ******/
	0,                                       /* 130: ***** I N V A L I D ******/
	0,                                       /* 131: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT8),              /* 132: gcvVGCMD_ARC_LINE        */
	vgmCOMMANDSIZE(2, gctINT8),              /* 133: gcvVGCMD_ARC_LINE_REL    */
	vgmCOMMANDSIZE(4, gctINT8),              /* 134: gcvVGCMD_ARC_QUAD        */
	vgmCOMMANDSIZE(4, gctINT8)               /* 135: gcvVGCMD_ARC_QUAD_REL    */
};

/* Command sizes with VG_PATH_DATATYPE_S_16 coordinates. */
static gctUINT _commandSize_INT16[] =
{
	vgmCOMMANDSIZE(0, gctINT16),             /*   0: gcvVGCMD_END             */
	vgmCOMMANDSIZE(0, gctINT16),             /*   1: gcvVGCMD_CLOSE           */
	vgmCOMMANDSIZE(2, gctINT16),             /*   2: gcvVGCMD_MOVE            */
	vgmCOMMANDSIZE(2, gctINT16),             /*   3: gcvVGCMD_MOVE_REL        */
	vgmCOMMANDSIZE(2, gctINT16),             /*   4: gcvVGCMD_LINE            */
	vgmCOMMANDSIZE(2, gctINT16),             /*   5: gcvVGCMD_LINE_REL        */
	vgmCOMMANDSIZE(4, gctINT16),             /*   6: gcvVGCMD_QUAD            */
	vgmCOMMANDSIZE(4, gctINT16),             /*   7: gcvVGCMD_QUAD_REL        */
	vgmCOMMANDSIZE(6, gctINT16),             /*   8: gcvVGCMD_CUBIC           */
	vgmCOMMANDSIZE(6, gctINT16),             /*   9: gcvVGCMD_CUBIC_REL       */
	vgmCOMMANDSIZE(0, gctINT16),             /*  10: gcvVGCMD_BREAK           */
	0,                                       /*  11: gcvVGCMD_HLINE           */
	0,                                       /*  12: gcvVGCMD_HLINE_REL       */
	0,                                       /*  13: gcvVGCMD_VLINE           */
	0,                                       /*  14: gcvVGCMD_VLINE_REL       */
	0,                                       /*  15: gcvVGCMD_SQUAD           */
	0,                                       /*  16: gcvVGCMD_SQUAD_REL       */
	0,                                       /*  17: gcvVGCMD_SCUBIC          */
	0,                                       /*  18: gcvVGCMD_SCUBIC_REL      */
	0,                                       /*  19: gcvVGCMD_SCCWARC         */
	0,                                       /*  20: gcvVGCMD_SCCWARC_REL     */
	0,                                       /*  21: gcvVGCMD_SCWARC          */
	0,                                       /*  22: gcvVGCMD_SCWARC_REL      */
	0,                                       /*  23: gcvVGCMD_LCCWARC         */
	0,                                       /*  24: gcvVGCMD_LCCWARC_REL     */
	0,                                       /*  25: gcvVGCMD_LCWARC          */
	0,                                       /*  26: gcvVGCMD_LCWARC_REL      */
	0,                                       /*  27: **** R E S E R V E D *****/
	0,                                       /*  28: **** R E S E R V E D *****/
	0,                                       /*  29: **** R E S E R V E D *****/
	0,                                       /*  30: **** R E S E R V E D *****/
	0,                                       /*  31: **** R E S E R V E D *****/

	0,                                       /*  32: ***** I N V A L I D ******/
	0,                                       /*  33: ***** I N V A L I D ******/
	0,                                       /*  34: ***** I N V A L I D ******/
	0,                                       /*  35: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT16),             /*  36: gcvVGCMD_HLINE_EMUL      */
	vgmCOMMANDSIZE(2, gctINT16),             /*  37: gcvVGCMD_HLINE_EMUL_REL  */
	0,                                       /*  38: ***** I N V A L I D ******/
	0,                                       /*  39: ***** I N V A L I D ******/
	0,                                       /*  40: ***** I N V A L I D ******/
	0,                                       /*  41: ***** I N V A L I D ******/
	0,                                       /*  42: ***** I N V A L I D ******/
	0,                                       /*  43: ***** I N V A L I D ******/
	0,                                       /*  44: ***** I N V A L I D ******/
	0,                                       /*  45: ***** I N V A L I D ******/
	0,                                       /*  46: ***** I N V A L I D ******/
	0,                                       /*  47: ***** I N V A L I D ******/
	0,                                       /*  48: ***** I N V A L I D ******/
	0,                                       /*  49: ***** I N V A L I D ******/
	0,                                       /*  50: ***** I N V A L I D ******/
	0,                                       /*  51: ***** I N V A L I D ******/
	0,                                       /*  52: ***** I N V A L I D ******/
	0,                                       /*  53: ***** I N V A L I D ******/
	0,                                       /*  54: ***** I N V A L I D ******/
	0,                                       /*  55: ***** I N V A L I D ******/
	0,                                       /*  56: ***** I N V A L I D ******/
	0,                                       /*  57: ***** I N V A L I D ******/
	0,                                       /*  58: ***** I N V A L I D ******/
	0,                                       /*  59: ***** I N V A L I D ******/
	0,                                       /*  60: ***** I N V A L I D ******/
	0,                                       /*  61: ***** I N V A L I D ******/
	0,                                       /*  62: ***** I N V A L I D ******/
	0,                                       /*  63: ***** I N V A L I D ******/

	0,                                       /*  64: ***** I N V A L I D ******/
	0,                                       /*  65: ***** I N V A L I D ******/
	0,                                       /*  66: ***** I N V A L I D ******/
	0,                                       /*  67: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT16),             /*  68: gcvVGCMD_VLINE_EMUL      */
	vgmCOMMANDSIZE(2, gctINT16),             /*  69: gcvVGCMD_VLINE_EMUL_REL  */
	0,                                       /*  70: ***** I N V A L I D ******/
	0,                                       /*  71: ***** I N V A L I D ******/
	0,                                       /*  72: ***** I N V A L I D ******/
	0,                                       /*  73: ***** I N V A L I D ******/
	0,                                       /*  74: ***** I N V A L I D ******/
	0,                                       /*  75: ***** I N V A L I D ******/
	0,                                       /*  76: ***** I N V A L I D ******/
	0,                                       /*  77: ***** I N V A L I D ******/
	0,                                       /*  78: ***** I N V A L I D ******/
	0,                                       /*  79: ***** I N V A L I D ******/
	0,                                       /*  80: ***** I N V A L I D ******/
	0,                                       /*  81: ***** I N V A L I D ******/
	0,                                       /*  82: ***** I N V A L I D ******/
	0,                                       /*  83: ***** I N V A L I D ******/
	0,                                       /*  84: ***** I N V A L I D ******/
	0,                                       /*  85: ***** I N V A L I D ******/
	0,                                       /*  86: ***** I N V A L I D ******/
	0,                                       /*  87: ***** I N V A L I D ******/
	0,                                       /*  88: ***** I N V A L I D ******/
	0,                                       /*  89: ***** I N V A L I D ******/
	0,                                       /*  90: ***** I N V A L I D ******/
	0,                                       /*  91: ***** I N V A L I D ******/
	0,                                       /*  92: ***** I N V A L I D ******/
	0,                                       /*  93: ***** I N V A L I D ******/
	0,                                       /*  94: ***** I N V A L I D ******/
	0,                                       /*  95: ***** I N V A L I D ******/

	0,                                       /*  96: ***** I N V A L I D ******/
	0,                                       /*  97: ***** I N V A L I D ******/
	0,                                       /*  98: ***** I N V A L I D ******/
	0,                                       /*  99: ***** I N V A L I D ******/
	0,                                       /* 100: ***** I N V A L I D ******/
	0,                                       /* 101: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(4, gctINT16),             /* 102: gcvVGCMD_SQUAD_EMUL      */
	vgmCOMMANDSIZE(4, gctINT16),             /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
	vgmCOMMANDSIZE(6, gctINT16),             /* 104: gcvVGCMD_SCUBIC_EMUL     */
	vgmCOMMANDSIZE(6, gctINT16),             /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
	0,                                       /* 106: ***** I N V A L I D ******/
	0,                                       /* 107: ***** I N V A L I D ******/
	0,                                       /* 108: ***** I N V A L I D ******/
	0,                                       /* 109: ***** I N V A L I D ******/
	0,                                       /* 110: ***** I N V A L I D ******/
	0,                                       /* 111: ***** I N V A L I D ******/
	0,                                       /* 112: ***** I N V A L I D ******/
	0,                                       /* 113: ***** I N V A L I D ******/
	0,                                       /* 114: ***** I N V A L I D ******/
	0,                                       /* 115: ***** I N V A L I D ******/
	0,                                       /* 116: ***** I N V A L I D ******/
	0,                                       /* 117: ***** I N V A L I D ******/
	0,                                       /* 118: ***** I N V A L I D ******/
	0,                                       /* 119: ***** I N V A L I D ******/
	0,                                       /* 120: ***** I N V A L I D ******/
	0,                                       /* 121: ***** I N V A L I D ******/
	0,                                       /* 122: ***** I N V A L I D ******/
	0,                                       /* 123: ***** I N V A L I D ******/
	0,                                       /* 124: ***** I N V A L I D ******/
	0,                                       /* 125: ***** I N V A L I D ******/
	0,                                       /* 126: ***** I N V A L I D ******/
	0,                                       /* 127: ***** I N V A L I D ******/

	0,                                       /* 128: ***** I N V A L I D ******/
	0,                                       /* 129: ***** I N V A L I D ******/
	0,                                       /* 130: ***** I N V A L I D ******/
	0,                                       /* 131: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT16),             /* 132: gcvVGCMD_ARC_LINE        */
	vgmCOMMANDSIZE(2, gctINT16),             /* 133: gcvVGCMD_ARC_LINE_REL    */
	vgmCOMMANDSIZE(4, gctINT16),             /* 134: gcvVGCMD_ARC_QUAD        */
	vgmCOMMANDSIZE(4, gctINT16)              /* 135: gcvVGCMD_ARC_QUAD_REL    */
};

/* Command sizes with PATH_DATATYPE_S_32 and PATH_DATATYPE_F coordinates. */
static gctUINT _commandSize_INT32[] =
{
	vgmCOMMANDSIZE(0, gctINT32),             /*   0: gcvVGCMD_END             */
	vgmCOMMANDSIZE(0, gctINT32),             /*   1: gcvVGCMD_CLOSE           */
	vgmCOMMANDSIZE(2, gctINT32),             /*   2: gcvVGCMD_MOVE            */
	vgmCOMMANDSIZE(2, gctINT32),             /*   3: gcvVGCMD_MOVE_REL        */
	vgmCOMMANDSIZE(2, gctINT32),             /*   4: gcvVGCMD_LINE            */
	vgmCOMMANDSIZE(2, gctINT32),             /*   5: gcvVGCMD_LINE_REL        */
	vgmCOMMANDSIZE(4, gctINT32),             /*   6: gcvVGCMD_QUAD            */
	vgmCOMMANDSIZE(4, gctINT32),             /*   7: gcvVGCMD_QUAD_REL        */
	vgmCOMMANDSIZE(6, gctINT32),             /*   8: gcvVGCMD_CUBIC           */
	vgmCOMMANDSIZE(6, gctINT32),             /*   9: gcvVGCMD_CUBIC_REL       */
	vgmCOMMANDSIZE(0, gctINT32),             /*  10: gcvVGCMD_BREAK           */
	0,                                       /*  11: gcvVGCMD_HLINE           */
	0,                                       /*  12: gcvVGCMD_HLINE_REL       */
	0,                                       /*  13: gcvVGCMD_VLINE           */
	0,                                       /*  14: gcvVGCMD_VLINE_REL       */
	0,                                       /*  15: gcvVGCMD_SQUAD           */
	0,                                       /*  16: gcvVGCMD_SQUAD_REL       */
	0,                                       /*  17: gcvVGCMD_SCUBIC          */
	0,                                       /*  18: gcvVGCMD_SCUBIC_REL      */
	0,                                       /*  19: gcvVGCMD_SCCWARC         */
	0,                                       /*  20: gcvVGCMD_SCCWARC_REL     */
	0,                                       /*  21: gcvVGCMD_SCWARC          */
	0,                                       /*  22: gcvVGCMD_SCWARC_REL      */
	0,                                       /*  23: gcvVGCMD_LCCWARC         */
	0,                                       /*  24: gcvVGCMD_LCCWARC_REL     */
	0,                                       /*  25: gcvVGCMD_LCWARC          */
	0,                                       /*  26: gcvVGCMD_LCWARC_REL      */
	0,                                       /*  27: **** R E S E R V E D *****/
	0,                                       /*  28: **** R E S E R V E D *****/
	0,                                       /*  29: **** R E S E R V E D *****/
	0,                                       /*  30: **** R E S E R V E D *****/
	0,                                       /*  31: **** R E S E R V E D *****/

	0,                                       /*  32: ***** I N V A L I D ******/
	0,                                       /*  33: ***** I N V A L I D ******/
	0,                                       /*  34: ***** I N V A L I D ******/
	0,                                       /*  35: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT32),             /*  36: gcvVGCMD_HLINE_EMUL      */
	vgmCOMMANDSIZE(2, gctINT32),             /*  37: gcvVGCMD_HLINE_EMUL_REL  */
	0,                                       /*  38: ***** I N V A L I D ******/
	0,                                       /*  39: ***** I N V A L I D ******/
	0,                                       /*  40: ***** I N V A L I D ******/
	0,                                       /*  41: ***** I N V A L I D ******/
	0,                                       /*  42: ***** I N V A L I D ******/
	0,                                       /*  43: ***** I N V A L I D ******/
	0,                                       /*  44: ***** I N V A L I D ******/
	0,                                       /*  45: ***** I N V A L I D ******/
	0,                                       /*  46: ***** I N V A L I D ******/
	0,                                       /*  47: ***** I N V A L I D ******/
	0,                                       /*  48: ***** I N V A L I D ******/
	0,                                       /*  49: ***** I N V A L I D ******/
	0,                                       /*  50: ***** I N V A L I D ******/
	0,                                       /*  51: ***** I N V A L I D ******/
	0,                                       /*  52: ***** I N V A L I D ******/
	0,                                       /*  53: ***** I N V A L I D ******/
	0,                                       /*  54: ***** I N V A L I D ******/
	0,                                       /*  55: ***** I N V A L I D ******/
	0,                                       /*  56: ***** I N V A L I D ******/
	0,                                       /*  57: ***** I N V A L I D ******/
	0,                                       /*  58: ***** I N V A L I D ******/
	0,                                       /*  59: ***** I N V A L I D ******/
	0,                                       /*  60: ***** I N V A L I D ******/
	0,                                       /*  61: ***** I N V A L I D ******/
	0,                                       /*  62: ***** I N V A L I D ******/
	0,                                       /*  63: ***** I N V A L I D ******/

	0,                                       /*  64: ***** I N V A L I D ******/
	0,                                       /*  65: ***** I N V A L I D ******/
	0,                                       /*  66: ***** I N V A L I D ******/
	0,                                       /*  67: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT32),             /*  68: gcvVGCMD_VLINE_EMUL      */
	vgmCOMMANDSIZE(2, gctINT32),             /*  69: gcvVGCMD_VLINE_EMUL_REL  */
	0,                                       /*  70: ***** I N V A L I D ******/
	0,                                       /*  71: ***** I N V A L I D ******/
	0,                                       /*  72: ***** I N V A L I D ******/
	0,                                       /*  73: ***** I N V A L I D ******/
	0,                                       /*  74: ***** I N V A L I D ******/
	0,                                       /*  75: ***** I N V A L I D ******/
	0,                                       /*  76: ***** I N V A L I D ******/
	0,                                       /*  77: ***** I N V A L I D ******/
	0,                                       /*  78: ***** I N V A L I D ******/
	0,                                       /*  79: ***** I N V A L I D ******/
	0,                                       /*  80: ***** I N V A L I D ******/
	0,                                       /*  81: ***** I N V A L I D ******/
	0,                                       /*  82: ***** I N V A L I D ******/
	0,                                       /*  83: ***** I N V A L I D ******/
	0,                                       /*  84: ***** I N V A L I D ******/
	0,                                       /*  85: ***** I N V A L I D ******/
	0,                                       /*  86: ***** I N V A L I D ******/
	0,                                       /*  87: ***** I N V A L I D ******/
	0,                                       /*  88: ***** I N V A L I D ******/
	0,                                       /*  89: ***** I N V A L I D ******/
	0,                                       /*  90: ***** I N V A L I D ******/
	0,                                       /*  91: ***** I N V A L I D ******/
	0,                                       /*  92: ***** I N V A L I D ******/
	0,                                       /*  93: ***** I N V A L I D ******/
	0,                                       /*  94: ***** I N V A L I D ******/
	0,                                       /*  95: ***** I N V A L I D ******/

	0,                                       /*  96: ***** I N V A L I D ******/
	0,                                       /*  97: ***** I N V A L I D ******/
	0,                                       /*  98: ***** I N V A L I D ******/
	0,                                       /*  99: ***** I N V A L I D ******/
	0,                                       /* 100: ***** I N V A L I D ******/
	0,                                       /* 101: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(4, gctINT32),             /* 102: gcvVGCMD_SQUAD_EMUL      */
	vgmCOMMANDSIZE(4, gctINT32),             /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
	vgmCOMMANDSIZE(6, gctINT32),             /* 104: gcvVGCMD_SCUBIC_EMUL     */
	vgmCOMMANDSIZE(6, gctINT32),             /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
	0,                                       /* 106: ***** I N V A L I D ******/
	0,                                       /* 107: ***** I N V A L I D ******/
	0,                                       /* 108: ***** I N V A L I D ******/
	0,                                       /* 109: ***** I N V A L I D ******/
	0,                                       /* 110: ***** I N V A L I D ******/
	0,                                       /* 111: ***** I N V A L I D ******/
	0,                                       /* 112: ***** I N V A L I D ******/
	0,                                       /* 113: ***** I N V A L I D ******/
	0,                                       /* 114: ***** I N V A L I D ******/
	0,                                       /* 115: ***** I N V A L I D ******/
	0,                                       /* 116: ***** I N V A L I D ******/
	0,                                       /* 117: ***** I N V A L I D ******/
	0,                                       /* 118: ***** I N V A L I D ******/
	0,                                       /* 119: ***** I N V A L I D ******/
	0,                                       /* 120: ***** I N V A L I D ******/
	0,                                       /* 121: ***** I N V A L I D ******/
	0,                                       /* 122: ***** I N V A L I D ******/
	0,                                       /* 123: ***** I N V A L I D ******/
	0,                                       /* 124: ***** I N V A L I D ******/
	0,                                       /* 125: ***** I N V A L I D ******/
	0,                                       /* 126: ***** I N V A L I D ******/
	0,                                       /* 127: ***** I N V A L I D ******/

	0,                                       /* 128: ***** I N V A L I D ******/
	0,                                       /* 129: ***** I N V A L I D ******/
	0,                                       /* 130: ***** I N V A L I D ******/
	0,                                       /* 131: ***** I N V A L I D ******/
	vgmCOMMANDSIZE(2, gctINT32),             /* 132: gcvVGCMD_ARC_LINE        */
	vgmCOMMANDSIZE(2, gctINT32),             /* 133: gcvVGCMD_ARC_LINE_REL    */
	vgmCOMMANDSIZE(4, gctINT32),             /* 134: gcvVGCMD_ARC_QUAD        */
	vgmCOMMANDSIZE(4, gctINT32)              /* 135: gcvVGCMD_ARC_QUAD_REL    */
};

/* Command size array pointers. */
static gctUINT_PTR _commandSize[] =
{
	_commandSize_INT8,				/* gcePATHTYPE_INT8   */
	_commandSize_INT16,				/* gcePATHTYPE_INT16  */
	_commandSize_INT32,				/* gcePATHTYPE_INT32  */
	_commandSize_INT32				/* gcePATHTYPE_FLOAT  */
};


/*******************************************************************************
**
** _CopyCoordinate
**
** Copy the current coordinate from the source to the destination buffer.
**
** ASSUMPTION: the source and destination are of the same format
**             and have the same bias and scale values.
**
** INPUT:
**
**    Source
**       Pointer to the source walker.
**
**    Destination
**       Pointer to the destination walker.
**
** OUTPUT:
**
**    Nothing.
*/

static void _CopyCoordinate_S_8(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	/* Get the source coordinate. */
	gctINT8 coordinate = * (gctINT8_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT8_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);
}

static void _CopyCoordinate_S_16(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	/* Get the source coordinate. */
	gctINT16 coordinate = * (gctINT16_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT16_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);
}

static void _CopyCoordinate_S_32(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	/* Get the source coordinate. */
	gctINT32 coordinate = * (gctINT32_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT32_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);
}

static void _CopyCoordinate_F(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	/* Get the source coordinate. */
	gctFLOAT coordinate = * (gctFLOAT_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctFLOAT_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);
}

static vgtCOPYCOORDINATE _copyCoordinate[] =
{
	_CopyCoordinate_S_8,					/* VG_PATH_DATATYPE_S_8  */
	_CopyCoordinate_S_16,					/* VG_PATH_DATATYPE_S_16 */
	_CopyCoordinate_S_32,					/* VG_PATH_DATATYPE_S_32 */
	_CopyCoordinate_F						/* VG_PATH_DATATYPE_F    */
};


/*******************************************************************************
**
** _ZeroCoordinate
**
** Set the current coordinate to zero.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker.
**
** OUTPUT:
**
**    Nothing.
*/

static void _ZeroCoordinate_S_8(
	vgsPATHWALKER_PTR Destination
	)
{
	/* Set the destination coordinate. */
	* ((gctINT8_PTR) Destination->coordinate) = 0;

	/* Advance to the next coordinate. */
	Destination->coordinate += sizeof(gctINT8);
}

static void _ZeroCoordinate_S_16(
	vgsPATHWALKER_PTR Destination
	)
{
	/* Set the destination coordinate. */
	* ((gctINT16_PTR) Destination->coordinate) = 0;

	/* Advance to the next coordinate. */
	Destination->coordinate += sizeof(gctINT16);
}

static void _ZeroCoordinate_S_32(
	vgsPATHWALKER_PTR Destination
	)
{
	/* Set the destination coordinate. */
	* ((gctINT32_PTR) Destination->coordinate) = 0;

	/* Advance to the next coordinate. */
	Destination->coordinate += sizeof(gctINT32);
}

static void _ZeroCoordinate_F(
	vgsPATHWALKER_PTR Destination
	)
{
	/* Set the destination coordinate. */
	* ((gctFLOAT_PTR) Destination->coordinate) = 0.0f;

	/* Advance to the next coordinate. */
	Destination->coordinate += sizeof(gctFLOAT);
}

static vgtZEROCOORDINATE _zeroCoordinate[] =
{
	_ZeroCoordinate_S_8,		/* VG_PATH_DATATYPE_S_8  */
	_ZeroCoordinate_S_16,		/* VG_PATH_DATATYPE_S_16 */
	_ZeroCoordinate_S_32,		/* VG_PATH_DATATYPE_S_32 */
	_ZeroCoordinate_F			/* VG_PATH_DATATYPE_F    */
};


/*******************************************************************************
**
** _ReadSegment
**
** Parse the current segment.
**
** INPUT:
**
**    Source
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

static vgmINLINE void _ReadSegment(
	vgsPATHWALKER_PTR Source
	)
{
	static gctBOOL _relative[] =
	{
		gcvFALSE,                            /*   0: gcvVGCMD_END             */
		gcvFALSE,                            /*   1: gcvVGCMD_CLOSE           */
		gcvFALSE,                            /*   2: gcvVGCMD_MOVE            */
		gcvTRUE,                             /*   3: gcvVGCMD_MOVE_REL        */
		gcvFALSE,                            /*   4: gcvVGCMD_LINE            */
		gcvTRUE,                             /*   5: gcvVGCMD_LINE_REL        */
		gcvFALSE,                            /*   6: gcvVGCMD_QUAD            */
		gcvTRUE,                             /*   7: gcvVGCMD_QUAD_REL        */
		gcvFALSE,                            /*   8: gcvVGCMD_CUBIC           */
		gcvTRUE,                             /*   9: gcvVGCMD_CUBIC_REL       */
		gcvFALSE,                            /*  10: gcvVGCMD_BREAK           */
		~0,                                  /*  11: gcvVGCMD_HLINE           */
		~0,                                  /*  12: gcvVGCMD_HLINE_REL       */
		~0,                                  /*  13: gcvVGCMD_VLINE           */
		~0,                                  /*  14: gcvVGCMD_VLINE_REL       */
		~0,                                  /*  15: gcvVGCMD_SQUAD           */
		~0,                                  /*  16: gcvVGCMD_SQUAD_REL       */
		~0,                                  /*  17: gcvVGCMD_SCUBIC          */
		~0,                                  /*  18: gcvVGCMD_SCUBIC_REL      */
		~0,                                  /*  19: gcvVGCMD_SCCWARC         */
		~0,                                  /*  20: gcvVGCMD_SCCWARC_REL     */
		~0,                                  /*  21: gcvVGCMD_SCWARC          */
		~0,                                  /*  22: gcvVGCMD_SCWARC_REL      */
		~0,                                  /*  23: gcvVGCMD_LCCWARC         */
		~0,                                  /*  24: gcvVGCMD_LCCWARC_REL     */
		~0,                                  /*  25: gcvVGCMD_LCWARC          */
		~0,                                  /*  26: gcvVGCMD_LCWARC_REL      */
		~0,                                  /*  27: **** R E S E R V E D *****/
		~0,                                  /*  28: **** R E S E R V E D *****/
		~0,                                  /*  29: **** R E S E R V E D *****/
		~0,                                  /*  30: **** R E S E R V E D *****/
		~0,                                  /*  31: **** R E S E R V E D *****/

		~0,                                  /*  32: ***** I N V A L I D ******/
		~0,                                  /*  33: ***** I N V A L I D ******/
		~0,                                  /*  34: ***** I N V A L I D ******/
		~0,                                  /*  35: ***** I N V A L I D ******/
		gcvFALSE,                            /*  36: gcvVGCMD_HLINE_EMUL      */
		gcvTRUE,                             /*  37: gcvVGCMD_HLINE_EMUL_REL  */
		~0,                                  /*  38: ***** I N V A L I D ******/
		~0,                                  /*  39: ***** I N V A L I D ******/
		~0,                                  /*  40: ***** I N V A L I D ******/
		~0,                                  /*  41: ***** I N V A L I D ******/
		~0,                                  /*  42: ***** I N V A L I D ******/
		~0,                                  /*  43: ***** I N V A L I D ******/
		~0,                                  /*  44: ***** I N V A L I D ******/
		~0,                                  /*  45: ***** I N V A L I D ******/
		~0,                                  /*  46: ***** I N V A L I D ******/
		~0,                                  /*  47: ***** I N V A L I D ******/
		~0,                                  /*  48: ***** I N V A L I D ******/
		~0,                                  /*  49: ***** I N V A L I D ******/
		~0,                                  /*  50: ***** I N V A L I D ******/
		~0,                                  /*  51: ***** I N V A L I D ******/
		~0,                                  /*  52: ***** I N V A L I D ******/
		~0,                                  /*  53: ***** I N V A L I D ******/
		~0,                                  /*  54: ***** I N V A L I D ******/
		~0,                                  /*  55: ***** I N V A L I D ******/
		~0,                                  /*  56: ***** I N V A L I D ******/
		~0,                                  /*  57: ***** I N V A L I D ******/
		~0,                                  /*  58: ***** I N V A L I D ******/
		~0,                                  /*  59: ***** I N V A L I D ******/
		~0,                                  /*  60: ***** I N V A L I D ******/
		~0,                                  /*  61: ***** I N V A L I D ******/
		~0,                                  /*  62: ***** I N V A L I D ******/
		~0,                                  /*  63: ***** I N V A L I D ******/

		~0,                                  /*  64: ***** I N V A L I D ******/
		~0,                                  /*  65: ***** I N V A L I D ******/
		~0,                                  /*  66: ***** I N V A L I D ******/
		~0,                                  /*  67: ***** I N V A L I D ******/
		gcvFALSE,                            /*  68: gcvVGCMD_VLINE_EMUL      */
		gcvTRUE,                             /*  69: gcvVGCMD_VLINE_EMUL_REL  */
		~0,                                  /*  70: ***** I N V A L I D ******/
		~0,                                  /*  71: ***** I N V A L I D ******/
		~0,                                  /*  72: ***** I N V A L I D ******/
		~0,                                  /*  73: ***** I N V A L I D ******/
		~0,                                  /*  74: ***** I N V A L I D ******/
		~0,                                  /*  75: ***** I N V A L I D ******/
		~0,                                  /*  76: ***** I N V A L I D ******/
		~0,                                  /*  77: ***** I N V A L I D ******/
		~0,                                  /*  78: ***** I N V A L I D ******/
		~0,                                  /*  79: ***** I N V A L I D ******/
		~0,                                  /*  80: ***** I N V A L I D ******/
		~0,                                  /*  81: ***** I N V A L I D ******/
		~0,                                  /*  82: ***** I N V A L I D ******/
		~0,                                  /*  83: ***** I N V A L I D ******/
		~0,                                  /*  84: ***** I N V A L I D ******/
		~0,                                  /*  85: ***** I N V A L I D ******/
		~0,                                  /*  86: ***** I N V A L I D ******/
		~0,                                  /*  87: ***** I N V A L I D ******/
		~0,                                  /*  88: ***** I N V A L I D ******/
		~0,                                  /*  89: ***** I N V A L I D ******/
		~0,                                  /*  90: ***** I N V A L I D ******/
		~0,                                  /*  91: ***** I N V A L I D ******/
		~0,                                  /*  92: ***** I N V A L I D ******/
		~0,                                  /*  93: ***** I N V A L I D ******/
		~0,                                  /*  94: ***** I N V A L I D ******/
		~0,                                  /*  95: ***** I N V A L I D ******/

		~0,                                  /*  96: ***** I N V A L I D ******/
		~0,                                  /*  97: ***** I N V A L I D ******/
		~0,                                  /*  98: ***** I N V A L I D ******/
		~0,                                  /*  99: ***** I N V A L I D ******/
		~0,                                  /* 100: ***** I N V A L I D ******/
		~0,                                  /* 101: ***** I N V A L I D ******/
		gcvFALSE,                            /* 102: gcvVGCMD_SQUAD_EMUL      */
		gcvTRUE,                             /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
		gcvFALSE,                            /* 104: gcvVGCMD_SCUBIC_EMUL     */
		gcvTRUE,                             /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
		~0,                                  /* 106: ***** I N V A L I D ******/
		~0,                                  /* 107: ***** I N V A L I D ******/
		~0,                                  /* 108: ***** I N V A L I D ******/
		~0,                                  /* 109: ***** I N V A L I D ******/
		~0,                                  /* 110: ***** I N V A L I D ******/
		~0,                                  /* 111: ***** I N V A L I D ******/
		~0,                                  /* 112: ***** I N V A L I D ******/
		~0,                                  /* 113: ***** I N V A L I D ******/
		~0,                                  /* 114: ***** I N V A L I D ******/
		~0,                                  /* 115: ***** I N V A L I D ******/
		~0,                                  /* 116: ***** I N V A L I D ******/
		~0,                                  /* 117: ***** I N V A L I D ******/
		~0,                                  /* 118: ***** I N V A L I D ******/
		~0,                                  /* 119: ***** I N V A L I D ******/
		~0,                                  /* 120: ***** I N V A L I D ******/
		~0,                                  /* 121: ***** I N V A L I D ******/
		~0,                                  /* 122: ***** I N V A L I D ******/
		~0,                                  /* 123: ***** I N V A L I D ******/
		~0,                                  /* 124: ***** I N V A L I D ******/
		~0,                                  /* 125: ***** I N V A L I D ******/
		~0,                                  /* 126: ***** I N V A L I D ******/
		~0,                                  /* 127: ***** I N V A L I D ******/

		~0,                                  /* 128: ***** I N V A L I D ******/
		~0,                                  /* 129: ***** I N V A L I D ******/
		~0,                                  /* 130: ***** I N V A L I D ******/
		~0,                                  /* 131: ***** I N V A L I D ******/
		gcvFALSE,                            /* 132: gcvVGCMD_ARC_LINE        */
		gcvTRUE,                             /* 133: gcvVGCMD_ARC_LINE_REL    */
		gcvFALSE,                            /* 134: gcvVGCMD_ARC_QUAD        */
		gcvTRUE,                             /* 135: gcvVGCMD_ARC_QUAD_REL    */
	};

	gctUINT segmentOffset;
	gctUINT8_PTR currData;
	gceVGCMD command;
	gctUINT commandSize;
	gctUINT dataTypeSize;

	/* Verify the walker. */
	vgmVERIFY_READER(Source);

	/* Make a shortcut to the segment offset. */
	segmentOffset = Source->segmentOffset;

	/* Make a shortcut to the data buffer pointer. */
	currData = Source->currData;

	/* Get the command. */
	command = currData[segmentOffset];

	/* Get the command and data sizes. */
	commandSize = Source->commandSizeArray[command];
	dataTypeSize = Source->dataTypeSize;

	/* Determine relative flag. */
	gcmASSERT(gcmIS_VALID_INDEX(command, _relative));
	Source->relative = _relative[command];
	gcmASSERT(Source->relative != ~0);

	/* Set the command. */
	Source->command = command;

	/* Is there data in the command? */
	if (commandSize == dataTypeSize)
	{
		/* Set the segment size. */
		Source->segmentSize = gcmSIZEOF(gctUINT8);
	}
	else
	{
		/* Determine the segment size. */
		Source->segmentSize
			= commandSize
			- (segmentOffset & Source->dataAlignment);

		/* Determine the coordinate pointer. */
		Source->coordinate
			= currData
			+ (segmentOffset & Source->dataMask)
			+ dataTypeSize;
	}
}


/*******************************************************************************
**
** _InitializeReadBuffer
**
** Initialize the reader parameters for the current buffer.
**
** INPUT:
**
**    Source
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

static vgmINLINE void _InitializeReadBuffer(
	vgsPATHWALKER_PTR Source
	)
{
	gcePATHTYPE dataType;
	gctUINT dataTypeSize;
	gctUINT dataAlignment;

	/* Verify the walker. */
	vgmVERIFY_READER(Source);

	/* Determine the beginning of the command buffer. */
	Source->currData
		= (gctUINT8_PTR) Source->currBuffer
		+ Source->currBuffer->bufferOffset;

	/* Reset the segment offset. */
	Source->segmentOffset = Source->reservedForHead;

	/* Set the walking boundary. */
	if (Source->currBuffer == Source->lastBuffer)
	{
		Source->currEndOffset = Source->lastEndOffset;
	}
	else
	{
		Source->currEndOffset = Source->currBuffer->offset;
	}

	/* Reset skipped counts. */
	Source->numSegments = 0;
	Source->numCoords   = 0;

	/* Get the buffer data type. */
	dataType = Source->currPathData->data.dataType;

	/* Set the reader and writer functions. */
	Source->get  = Source->getArray[dataType];
	Source->set  = Source->setArray[dataType];
	Source->zero = _zeroCoordinate[dataType];

	/* Determine the data type size, alignment and mask. */
	dataTypeSize  = vgfGetPathDataSize(dataType);
	dataAlignment = dataTypeSize - 1;

	Source->dataTypeSize  =  dataTypeSize;
	Source->dataAlignment =  dataAlignment;
	Source->dataMask      = ~dataAlignment;

	/* Set the command sizes for the buffer. */
	Source->commandSizeArray = _commandSize[dataType];

	/* Parse the segment. */
	_ReadSegment(Source);
}


/*******************************************************************************
**
** _AppendSubpathToChain
**
** Attach the specified subpath to the chain.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
**    PathData
**       Pointer to the subpath to append.
**
** OUTPUT:
**
**    Nothing.
*/

static void _AppendSubpathToChain(
	vgsPATHWALKER_PTR Destination,
	vgsPATH_DATA_PTR PathData
	)
{
	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Attach to the chain. */
	if (Destination->head == gcvNULL)
	{
		/* Set the  */
		Destination->head =
		Destination->tail = PathData;
	}
	else
	{
		vgmSETNEXTSUBBUFFER(Destination->tail, PathData);
		vgmSETPREVSUBBUFFER(PathData, Destination->tail);

		Destination->tail = PathData;
	}
}


/*******************************************************************************
**
** _AppendSubpathToPath
**
** Attach the accumulated chain of subpaths to the specified path.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure that contains the accumulated chain.
**
**    Path
**       Pointer to the path to append the chain to.
**
** OUTPUT:
**
**    Nothing.
*/

static void _AppendSubpathToPath(
	vgsPATHWALKER_PTR Destination,
	vgsPATH_PTR Path
	)
{
	vgsPATH_DATA_PTR newPathData;

	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Get shortcuts to the new path data. */
	newPathData = Destination->head;

	if (Path->head == gcvNULL)
	{
		/* Set the head. */
		Path->head = newPathData;
	}
	else
	{
		/* Link to the tail. */
		vgmSETNEXTSUBBUFFER(Path->tail, newPathData);
		vgmSETPREVSUBBUFFER(newPathData, Path->tail);
	}

	/* Set new tail. */
	Path->tail = Destination->tail;

	/* Update path counts. */
	Path->numSegments += Destination->numSegments;
	Path->numCoords   += Destination->numCoords;
}


/*******************************************************************************
**
** _RemoveSubpathFromPath
**
** Reverse the effect of _AppendSubpathToPath.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure that contains the accumulated chain.
**
**    Path
**       Pointer to the path to append the chain to.
**
** OUTPUT:
**
**    Nothing.
*/

static void _RemoveSubpathFromPath(
	vgsPATHWALKER_PTR Destination,
	vgsPATH_PTR Path
	)
{
	vgsPATH_DATA_PTR newPathData;

	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Cannot be empty. */
	gcmASSERT(Path->head != gcvNULL);

	/* Get shortcuts to the new path data. */
	newPathData = Destination->head;
	gcmASSERT(newPathData != gcvNULL);

	/* Was there anything in the path before? */
	if (Path->head == newPathData)
	{
		/* Tails better match as well. */
		gcmASSERT(Path->tail == Destination->tail);

		/* Reset the list. */
		Path->head =
		Path->tail = gcvNULL;

		/* Reset the path counts. */
		Path->numSegments = 0;
		Path->numCoords   = 0;
	}
	else
	{
		/* Get the previous. */
		vgsPATH_DATA_PTR prev = vgmGETPREVSUBBUFFER(newPathData);
		gcmASSERT(prev != gcvNULL);

		/* Break the link. */
		vgmSETNEXTSUBBUFFER(prev, gcvNULL);

		/* Set new tail. */
		Path->tail = prev;

		/* Update path counts. */
		Path->numSegments -= Destination->numSegments;
		Path->numCoords   -= Destination->numCoords;
	}
}


/*******************************************************************************
**
** _InitializeWriter
**
** Initialize the data format dependent writer parameters.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
**    DataType
**       Data type for the path.
**
** OUTPUT:
**
**    Nothing.
*/

static void _InitializeWriter(
	vgsPATHWALKER_PTR Destination,
	gcePATHTYPE DataType
	)
{
	gcePATHTYPE dataType;
	gctUINT dataTypeSize;
	gctUINT dataAlignment;

	/* Determine the data format. */
	dataType = (DataType == gcePATHTYPE_UNKNOWN)
		? Destination->path->halDataType
		: DataType;

	/* Determine the data type size, alignment and mask. */
	dataTypeSize  = vgfGetPathDataSize(dataType);
	dataAlignment = dataTypeSize - 1;

	Destination->forcedDataType =  DataType;
	Destination->dataType       =  dataType;
	Destination->dataTypeSize   =  dataTypeSize;
	Destination->dataAlignment  =  dataAlignment;
	Destination->dataMask       = ~dataAlignment;

	/* Determine the function pointers. */
	Destination->set  = Destination->setArray[dataType];
	Destination->zero = _zeroCoordinate[dataType];

	/* Set the data size array for the buffer. */
	Destination->commandSizeArray = _commandSize[dataType];
}


/*******************************************************************************
**
** _OpenStorage
**
** Open a new path data buffer for writing.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
**    MinimumSize
**       Minimum required buffer size. If 0 is specified, the first available
**       buffer is returned regardless of its size.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _OpenStorage(
	vgsPATHWALKER_PTR Destination,
	gctUINT MinimumSize
	)
{
	gceSTATUS status;

	do
	{
		vgsPATH_DATA_PTR pathData;
		gcsCMDBUFFER_PTR buffer;

		/* Allocate command buffer. */
		gcmERR_BREAK(vgsPATHSTORAGE_Open(
			Destination->storage, MinimumSize, &pathData
			));

		/* Attach to the chain. */
		_AppendSubpathToChain(Destination, pathData);

		/* Get the data header. */
		buffer = vgmGETCMDBUFFER(pathData);

		/* Set the buffer type. */
		pathData->data.dataType = Destination->dataType;

		/* Set the owner. */
		pathData->path = Destination->path;

		/* Initialize the walker. */
		Destination->currPathData = pathData;
		Destination->currBuffer   = buffer;
		Destination->available    = buffer->size - Destination->reservedForHead;

		/* Determine the beginning of the command buffer. */
		Destination->currData = (gctUINT8_PTR) buffer + buffer->bufferOffset;

		/* Set the initial offset. */
		Destination->segmentOffset = Destination->reservedForHead;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _CloseSubpath
**
** Close the current subpath if any.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

static vgsPATH_DATA_PTR _CloseSubpath(
	vgsPATHWALKER_PTR Destination
	)
{
	vgsPATH_DATA_PTR pathData;

	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Get the buffer pointer. */
	pathData = Destination->currPathData;

	if (pathData != gcvNULL)
	{
		/* Empty path? */
		if (Destination->segmentOffset == Destination->reservedForHead)
		{
			/* Remove from the chain. */
			if (Destination->head == pathData)
			{
				Destination->head =
				Destination->tail = gcvNULL;
			}
			else
			{
				/* Get the previous buffer. */
				vgsPATH_DATA_PTR prev = vgmGETPREVSUBBUFFER(pathData);

				/* Break the link. */
				vgmSETNEXTSUBBUFFER(prev, gcvNULL);
				Destination->tail = prev;
			}

			/* Make sure no extra was allocated. */
			gcmASSERT(pathData->extra == gcvNULL);

			/* Free the buffer. */
			vgsPATHSTORAGE_Free(Destination->storage, pathData, gcvTRUE);

			/* The path was empty. */
			pathData = gcvNULL;
		}
		else
		{
			/* Set the final size. */
			Destination->currBuffer->offset = Destination->segmentOffset;

			/* Close the buffer. */
			vgsPATHSTORAGE_Close(Destination->storage, pathData);
		}

		/* Reset the current buffer. */
		Destination->currPathData = gcvNULL;
		Destination->available    = -1;
	}

	/* Return the path pointer. */
	return pathData;
}


/******************************************************************************\
******************************** Path walker API *******************************
\******************************************************************************/

/*******************************************************************************
**
** vgsPATHWALKER_InitializeImport
**
** Initialize path segment walker parameters for reading.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Storage
**       Pointer to the storage manager.
**
**    Source
**       Pointer to the source walker to initialize.
**
**    DestinationPath
**       Pointer to the destination path object.
**
**    Data
**       Pointer to the coordinate data to import.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_InitializeImport(
	vgsCONTEXT_PTR Context,
	vgsPATHSTORAGE_PTR Storage,
	vgsPATHWALKER_PTR Source,
	vgsPATH_PTR DestinationPath,
	gctCONST_POINTER Data
	)
{
	gcePATHTYPE dataType;

#if vgvVALIDATE_PATH_WALKER
	/* Singnal reading operation. */
	Source->reader = gcvTRUE;
#endif

	/* Store the object pointers. */
	Source->context        = Context;
	Source->storage        = Storage;
	Source->arcCoordinates = Context->arcCoordinates;
	Source->vg             = Context->vg;

	/* Set the bias and scale values. */
	Source->bias  = DestinationPath->bias;
	Source->scale = DestinationPath->scale;

	/* Get the datatype. */
	dataType = DestinationPath->halDataType;

	/* Set the reader/copier functions. */
	Source->get     = DestinationPath->getArray[dataType];
	Source->getcopy = DestinationPath->getcopyArray[dataType];
	Source->copy    = _copyCoordinate[dataType];

	/* Set initial coordinate pointer. */
	Source->coordinate = (gctUINT8_PTR) Data;
}


/*******************************************************************************
**
** vgsPATHWALKER_InitializeReader
**
** Initialize path segment walker parameters for reading.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Storage
**       Pointer to the storage manager.
**
**    Source
**       Pointer to the source walker to initialize.
**
**    ControlCoordinates
**       Pointer to the control coordinates.
**
**    Path
**       Pointer to the path object.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_InitializeReader(
	vgsCONTEXT_PTR Context,
	vgsPATHSTORAGE_PTR Storage,
	vgsPATHWALKER_PTR Source,
	vgsCONTROL_COORD_PTR ControlCoordinates,
	vgsPATH_PTR Path
	)
{
	/* There must be data in the source. */
	gcmASSERT(Path->numCoords > 0);

#if vgvVALIDATE_PATH_WALKER
	/* Singnal reading operation. */
	Source->reader = gcvTRUE;
#endif

	/* Store the object pointers. */
	Source->path           = Path;
	Source->context        = Context;
	Source->storage        = Storage;
	Source->arcCoordinates = Context->arcCoordinates;
	Source->vg             = Context->vg;

	/* Set the pointer to the control coordinates. */
	Source->coords = ControlCoordinates;

	/* Set the size of the head reserve. */
	Source->reservedForHead = Path->pathInfo.reservedForHead;

	/* Set the bias and scale values. */
	Source->bias  = Path->bias;
	Source->scale = Path->scale;

	/* Set the get and set function arrays. */
	Source->getArray = Path->getArray;
	Source->setArray = Path->setArray;

	/* Initialize buffer positions. */
	Source->currPathData = Path->head;
	Source->currBuffer   = vgmGETCMDBUFFER(Path->head);
	Source->lastBuffer   = vgmGETCMDBUFFER(Path->tail);

	/* Store the last buffer's end. */
	Source->lastEndOffset = Source->lastBuffer->offset;

	/* Initialize walking for the current buffer. */
	_InitializeReadBuffer(Source);
}


/*******************************************************************************
**
** vgsPATHWALKER_InitializeWriter
**
** Initialize path segment walker parameters for writing.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Storage
**       Pointer to the storage manager.
**
**    Destination
**       Pointer to the destination walker to initialize.
**
**    Path
**       Pointer to the path object.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_InitializeWriter(
	vgsCONTEXT_PTR Context,
	vgsPATHSTORAGE_PTR Storage,
	vgsPATHWALKER_PTR Destination,
	vgsPATH_PTR Path
	)
{
#if vgvVALIDATE_PATH_WALKER
	/* Singnal reading operation. */
	Destination->reader = gcvFALSE;
#endif

	/* Store the object pointers. */
	Destination->path           = Path;
	Destination->context        = Context;
	Destination->storage        = Storage;
	Destination->arcCoordinates = Context->arcCoordinates;
	Destination->vg             = Context->vg;

	/* Set the pointer to the control coordinates. */
	Destination->coords = &Path->coords;

	/* Set the size of the head reserve. */
	Destination->reservedForHead = Path->pathInfo.reservedForHead;

	/* Set the bias and scale values. */
	Destination->bias  = Path->bias;
	Destination->scale = Path->scale;

	/* Set the get function array. */
	Destination->setArray = Path->setArray;

	/* Reset the current buffer pointer. */
	Destination->currPathData = gcvNULL;

	/* Reset written data counts. */
	Destination->numSegments = 0;
	Destination->numCoords   = 0;

	/* Float format is not forced by default. */
	Destination->forcedDataType = gcePATHTYPE_UNKNOWN;

	/* Reset the available space counter. */
	Destination->available = -1;

	/* Reset the buffer chain. */
	Destination->head = gcvNULL;
	Destination->tail = gcvNULL;
}


/*******************************************************************************
**
** vgsPATHWALKER_NextSegment
**
** Skip to the next segment.
**
** INPUT:
**
**    Source
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHWALKER_NextSegment(
	vgsPATHWALKER_PTR Source
	)
{
	/* Verify the walker. */
	vgmVERIFY_READER(Source);

	/* Advance to the next segment. */
	Source->segmentOffset += Source->segmentSize;
	gcmASSERT(Source->segmentOffset <= Source->currEndOffset);

	/* Is this the end of the current buffer? */
	if (Source->segmentOffset == Source->currEndOffset)
	{
		return vgsPATHWALKER_NextBuffer(Source);
	}

	/* Update skipped counts. */
	Source->numSegments += 1;
	Source->numCoords   += vgfGetSegmentDataCount(Source->command);

	/* Parse the segment. */
	_ReadSegment(Source);

	/* Success. */
	return gcvSTATUS_OK;
}


/*******************************************************************************
**
** vgsPATHWALKER_NextBuffer
**
** Skip to the next buffer.
**
** INPUT:
**
**    Source
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHWALKER_NextBuffer(
	vgsPATHWALKER_PTR Source
	)
{
	/* Verify the walker. */
	vgmVERIFY_READER(Source);

	/* Is this the last buffer? */
	if (Source->currBuffer == Source->lastBuffer)
	{
		return gcvSTATUS_NO_MORE_DATA;
	}

	/* Advance to the next buffer. */
	Source->currPathData = vgmGETNEXTSUBBUFFER(Source->currPathData);
	Source->currBuffer   = vgmGETCMDBUFFER(Source->currPathData);

	/* Initialize walking for the current buffer. */
	_InitializeReadBuffer(Source);

	/* Success. */
	return gcvSTATUS_OK;
}


/*******************************************************************************
**
** vgsPATHWALKER_SeekToSegment
**
** Seek to the specified segment number.
**
** INPUT:
**
**    Source
**       Pointer to the walker structure.
**
**    Segment
**       The number of the segment to set the current position to.
**
** OUTPUT:
**
**    Nothing.
*/

gctBOOL vgsPATHWALKER_SeekToSegment(
	vgsPATHWALKER_PTR Source,
	gctUINT Segment
	)
{
	/* Verify the walker. */
	vgmVERIFY_READER(Source);

	/* Skip buffers until the starting segment is found. */
	while (Segment >= Source->currPathData->numSegments)
	{
		/* Subtract the current number of segments. */
		Segment -= Source->currPathData->numSegments;

		/* Advance to the next buffer. */
		if (vgsPATHWALKER_NextBuffer(Source) == gcvSTATUS_NO_MORE_DATA)
		{
			return gcvFALSE;
		}
	}

	/* Skip until positioned on the starting segment. */
	while (Segment)
	{
		/* Skip one segment. */
		Segment -= 1;

		/* Advance to the next segment. */
		if (vgsPATHWALKER_NextSegment(Source) == gcvSTATUS_NO_MORE_DATA)
		{
			return gcvFALSE;
		}
	}

	/* Success. */
	return gcvTRUE;
}


/*******************************************************************************
**
** vgsPATHWALKER_SeekToEnd
**
** Seek to the end of the current buffer so that the following call to
** vgsPATHWALKER_NextSegment would return the first segment of the next buffer.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_SeekToEnd(
	vgsPATHWALKER_PTR Destination
	)
{
	Destination->segmentSize   = 0;
	Destination->segmentOffset = Destination->currEndOffset;
}


/*******************************************************************************
**
** vgsPATHWALKER_StartSubpath
**
** Close the current subpath if any and open a new one for writing.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
**    MinimumSize
**       Minimum required buffer size. If 0 is specified, the first available
**       buffer is returned regardless of its size.
**
**    DataType
**       Data type for the path.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHWALKER_StartSubpath(
	vgsPATHWALKER_PTR Destination,
	gctUINT MinimumSize,
	gcePATHTYPE DataType
	)
{
	gceSTATUS status;

	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Close the current subpath. */
	_CloseSubpath(Destination);

	/* Initialize the walker. */
	_InitializeWriter(Destination, DataType);

	/* Open the path for writing. */
	status = _OpenStorage(Destination, MinimumSize);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHWALKER_CloseSubpath
**
** Close the current subpath if any.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

vgsPATH_DATA_PTR vgsPATHWALKER_CloseSubpath(
	vgsPATHWALKER_PTR Destination
	)
{
	vgsPATH_DATA_PTR pathData;

	/* Reset float force. */
	Destination->forcedDataType = gcePATHTYPE_UNKNOWN;

	/* Close the current path. */
	pathData = _CloseSubpath(Destination);

	/* Return the path pointer. */
	return pathData;
}


/*******************************************************************************
**
** vgsPATHWALKER_DoneWriting
**
** Closes the buffer for writing.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHWALKER_DoneWriting(
	vgsPATHWALKER_PTR Destination
	)
{
	gceSTATUS status;
	vgsPATH_DATA_PTR newPathData;

	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Close the subpath. */
	vgsPATHWALKER_CloseSubpath(Destination);

	/* Get the shortcut to the new path data. */
	newPathData = Destination->head;

	/* Attach to the path. */
	if (newPathData == gcvNULL)
	{
		/* No change. */
		status = gcvSTATUS_OK;
	}
	else
	{
		/* Get the path. */
		vgsPATH_PTR path = Destination->path;

		/* Append the chain to the path. */
		_AppendSubpathToPath(Destination, path);

		/* Finalize the path structure. */
		status = gcoVG_FinalizePath(
			Destination->vg, &path->head->data
			);

		/* Failed? */
		if (gcmIS_ERROR(status))
		{
			/* Roll back. */
			_RemoveSubpathFromPath(Destination, path);
		}

		/* Validate the storage. */
		vgmVALIDATE_STORAGE(Destination->storage);
	}

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHWALKER_WriteCommand
**
** Add a new segment command to the data buffer.
**
** INPUT:
**
**    Destination
**       Pointer to the walker structure.
**
**    Command
**       Command to add.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHWALKER_WriteCommand(
	vgsPATHWALKER_PTR Destination,
	gceVGCMD Command
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		gctUINT available;
		gctUINT commandSize;
		gctUINT segmentOffset;
		gctUINT8_PTR currData;
		gctUINT dataTypeSize;
		gctUINT coordinateCount;
		gctUINT segmentSize;
		gctUINT coordinateOffset;

		/* Verify the walker. */
		vgmVERIFY_WRITER(Destination);

		/* Get the number of available bytes in the destination. */
		available = Destination->available;

		/* Nothing allocated yet? */
		if (available == -1)
		{
			/* Initialize the walker. */
			_InitializeWriter(Destination, Destination->forcedDataType);

			/* Get the command size. */
			commandSize = Destination->commandSizeArray[Command];

			/* Open the path for writing. */
			gcmERR_BREAK(_OpenStorage(Destination, commandSize));
		}
		else
		{
			/* Get the command size. */
			commandSize = Destination->commandSizeArray[Command];

			/* Is there enough space in the destination? */
			if (available < commandSize)
			{
				/* Close the current subpath. */
				_CloseSubpath(Destination);

				/* Open the path for writing. */
				gcmERR_BREAK(_OpenStorage(Destination, commandSize));

				/* Should be enough now. */
				gcmASSERT(Destination->available >= (gctINT) commandSize);
			}
		}

		/* Make a shortcut to the segment offset. */
		segmentOffset = Destination->segmentOffset;

		/* Make a shortcut to the data buffer pointer. */
		currData = Destination->currData;

		/* Set the command. */
		currData[segmentOffset] = Command;

		/* Get the size of the data type. */
		dataTypeSize = Destination->dataTypeSize;

		/* Is there data in the command? */
		if (commandSize == dataTypeSize)
		{
			/* Advance the offset. */
			Destination->segmentOffset += gcmSIZEOF(gctUINT8);

			/* Update available buffer counter. */
			Destination->available -= gcmSIZEOF(gctUINT8);
		}
		else
		{
			/* Determine the segment size. */
			segmentSize
				= commandSize
				- (segmentOffset & Destination->dataAlignment);

			/* Determine the coordinate offset. */
			coordinateOffset
				= (segmentOffset & Destination->dataMask)
				+ dataTypeSize;

			/* Fill the space between the command and data. */
#if vgvVALIDATE_PATH_WALKER
			{
				gctINT fillCount = coordinateOffset - segmentOffset - 1;
				gcmASSERT(fillCount >= 0);

				if (fillCount > 0)
				{
					gctUINT8_PTR filler = currData + segmentOffset + 1;

					while (fillCount--)
					{
						*filler++ = 0xEE;
					}
				}
			}
#endif

			/* Advance the offset. */
			Destination->segmentOffset += segmentSize;

			/* Determine the coordinate pointer. */
			Destination->coordinate = currData + coordinateOffset;

			/* Update available buffer counter. */
			Destination->available -= segmentSize;
		}

		/* Determine the number of coordinates in the command. */
		coordinateCount =
		Destination->coordinateCount = vgfGetSegmentDataCount(Command);

		/* Upate the command and coordinate counters. */
		Destination->currPathData->numSegments += 1;
		Destination->currPathData->numCoords   += coordinateCount;
		Destination->numSegments               += 1;
		Destination->numCoords                 += coordinateCount;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHWALKER_GetCopyBlockSize
**
** Determines the maximum size of path data in bytes that can be copied from
** the specified source to the specified existing destination buffer.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    Source
**       Pointer to the source walker structure.
**
** OUTPUT:
**
**    Size
**       The size of data block.
**
**    SegmentCount
**       The number of segments contained within the data block.
**
**    CoordinateCount
**       The number of coordinates contained within the data block.
*/

gceSTATUS vgsPATHWALKER_GetCopyBlockSize(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctUINT * Size,
	gctUINT * SegmentCount,
	gctUINT * CoordinateCount
	)
{
	gceSTATUS status;

	do
	{
		gctUINT destinationDataSize;
		gctUINT sourceDataSize;
		gctUINT newSize;
		gctUINT blockSize;
		gctUINT segmentCount;
		gctUINT coordinateCount;
		gctUINT segmentSize;
		gctUINT segmentOffset;
		gctUINT8_PTR currSegment;
		gctUINT8_PTR lastSegment;
		gceVGCMD command;
		gctUINT commandSize;

		/* Verify the walkers. */
		vgmVERIFY_WRITER(Destination);
		vgmVERIFY_READER(Source);

		/* Is the destination completely occupied? */
		if ((gctINT) Source->segmentSize > Destination->available)
		{
			/* Yes, start a new subpath. */
			gcmERR_BREAK(vgsPATHWALKER_StartSubpath(
				Destination, Source->segmentSize, gcePATHTYPE_UNKNOWN
				));
		}

		/* Get the available destination size. */
		destinationDataSize = Destination->available;

		/* Determine the size of the source data. */
		sourceDataSize = Source->currEndOffset - Source->segmentOffset;

		/* If the source size is less or equal to the size of available destination,
		   use the size of the source. */
		if (sourceDataSize <= destinationDataSize)
		{
			/* Set the size of data to be copied equal to the size of the source. */
			*Size
				= sourceDataSize;

			/* Determine the number of the segments and coordinates to copy. */
			*SegmentCount
				= Source->currPathData->numSegments		/* Segments in the buffer. */
				- Source->numSegments;					/* Segments already walked. */

			*CoordinateCount
				= Source->currPathData->numCoords		/* Coords in the buffer. */
				- Source->numCoords;					/* Coords already walked. */

			/* Success. */
			status = gcvSTATUS_OK;
			break;
		}

		/* Set initial parameters. */
		segmentSize     =
		newSize         = Source->segmentSize;
		blockSize       = 0;
		segmentCount    = 0;
		coordinateCount = 0;

		/* Set initial and last segments. */
		segmentOffset = Source->segmentOffset;
		currSegment = Source->currData + segmentOffset;
		lastSegment = Source->currData + Source->currEndOffset;

		/* Walk the data. */
		do
		{
			/* Doesn't fit? */
			if (newSize > destinationDataSize)
			{
				/* Found the maximum source size that fits, now exit. */
				break;
			}

			/* Update source counts. */
			blockSize        = newSize;
			segmentCount    += 1;
			coordinateCount += vgfGetSegmentDataCount(Source->command);

			/* Advance to the next segment. */
			currSegment += segmentSize;

			/* End of buffer? */
			if (currSegment == lastSegment)
			{
				break;
			}

			/* Advance the segment offset. */
			segmentOffset += segmentSize;

			/* Get the command. */
			command = *currSegment;

			/* Get command size. */
			commandSize = Source->commandSizeArray[command];

			/* Determine the next segment size. */
			segmentSize
				= commandSize
				- (segmentOffset & Source->dataAlignment);

			/* Determine the new aligned size. */
			newSize = blockSize + segmentSize;
		}
		while (gcvTRUE);

		/* Return results. */
		*Size            = blockSize;
		*SegmentCount    = segmentCount;
		*CoordinateCount = coordinateCount;

		/* Success. */
		status = gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHWALKER_CopyBlock
**
** Copy a block of path data from the source to the destination and update both
** walkers with the new position.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    Source
**       Pointer to the source walker structure.
**
**    Size
**       The number of bytes to copy. The size is assumed to be segment aligned.
**
**    SegmentCount
**       The number of segments contained within the area to be copied.
**
**    CoordinateCount
**       The number of coordinates contained within the area to be copied.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgsPATHWALKER_CopyBlock(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctUINT Size,
	gctUINT SegmentCount,
	gctUINT CoordinateCount
	)
{
	gceSTATUS status;
	gctPOINTER source;
	gctPOINTER destination;

	/* Verify the walkers. */
	vgmVERIFY_WRITER(Destination);
	vgmVERIFY_READER(Source);

	/* The destination has to have enough space. */
	gcmASSERT((gctINT) Size <= Destination->available);

	/* Determine the source and destination locations. */
	source      = Source->currData      + Source->segmentOffset;
	destination = Destination->currData + Destination->segmentOffset;

	/* Copy data. */
	gcmVERIFY_OK(gcoOS_MemCopy(destination, source, Size));

	/* Advance the destination. */
	Destination->segmentOffset             += Size;
	Destination->available                 -= Size;
	Destination->currPathData->numSegments += SegmentCount;
	Destination->currPathData->numCoords   += CoordinateCount;
	Destination->numSegments               += SegmentCount;
	Destination->numCoords                 += CoordinateCount;

	/* Advance the source. */
	Source->segmentSize = Size;
	status = vgsPATHWALKER_NextSegment(Source);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgsPATHWALKER_AppendData
**
** Append the path data accumulated in the specified walker.
**
** INPUT:
**
**    Destination
**       Pointer to the destination path walker.
**
**    Source
**       Pointer to the walker structure containing the data to append.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_AppendData(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctUINT SegmentCount,
	gctUINT CoordinateCount
	)
{
	vgsPATH_DATA_PTR newPathData;

	/* Verify the walkers. */
	vgmVERIFY_WRITER(Destination);
	vgmVERIFY_WRITER(Source);

	/* Verify if the destination is currently open. */
	if (Destination->currPathData != gcvNULL)
	{
		/* Having more then one buffers open at the same time may increase
		   memory consumption, therefore, it is best to avoid it. */
		gcmTRACE(
			gcvLEVEL_WARNING,
			"%s (%d): The destination path is currently open.\n",
			__FUNCTION__, __LINE__
			);

		/* Close the current subpath. */
		vgsPATHWALKER_CloseSubpath(Destination);
	}

	/* Close the specified subpath. */
	newPathData = vgsPATHWALKER_CloseSubpath(Source);

	/* Attach to the chain. */
	if (newPathData != gcvNULL)
	{
		_AppendSubpathToChain(Destination, newPathData);

		/* Update segment and coordinate counts. */
		Destination->numSegments += SegmentCount;
		Destination->numCoords   += CoordinateCount;
	}
}


/*******************************************************************************
**
** vgsPATHWALKER_ReplaceData
**
** Replace the current path data buffer with a new one.
**
** INPUT:
**
**    Destination
**       Pointer to the destination path walker.
**
**    Source
**       Pointer to the walker structure containing the data to append.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_ReplaceData(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source
	)
{
	vgsPATH_DATA_PTR newPathData;

	/* Verify the walkers. */
	vgmVERIFY_READER(Destination);
	vgmVERIFY_WRITER(Source);

	/* Close the source path. */
	newPathData = vgsPATHWALKER_CloseSubpath(Source);

	/* Was there anything in the path? */
	if (newPathData != gcvNULL)
	{
		vgsPATH_PTR path;
		vgsPATH_DATA_PTR currPathData;
		vgsPATH_DATA_PTR prevPathData;
		vgsPATH_DATA_PTR nextPathData;

		/* Get a shortcut to the path. */
		path = Destination->path;

		/* Get the current buffer. */
		gcmASSERT(Destination->currPathData != gcvNULL);
		currPathData = Destination->currPathData;

		/* Seek to the end of the buffer. */
		vgsPATHWALKER_SeekToEnd(Destination);

		/* First in the chain? */
		if (path->head == currPathData)
		{
			/* Set the new head. */
			path->head = newPathData;

			/* Also the last one? */
			if (path->tail == currPathData)
			{
				/* Set the new tail. */
				path->tail = newPathData;

				/* Set the current equal to the last. */
				Destination->currBuffer = Destination->lastBuffer;
			}
			else
			{
				/* Get the next subbuffer. */
				nextPathData = vgmGETNEXTSUBBUFFER(currPathData);

				/* Link in. */
				vgmSETNEXTSUBBUFFER(newPathData,  nextPathData);
				vgmSETPREVSUBBUFFER(nextPathData, newPathData);
			}
		}
		else
		{
			/* Get the previous subbuffer. */
			prevPathData = vgmGETPREVSUBBUFFER(currPathData);

			/* Last in the chain? */
			if (path->tail == currPathData)
			{
				/* Set the new tail. */
				path->tail = newPathData;

				/* Link in. */
				vgmSETPREVSUBBUFFER(newPathData,  prevPathData);
				vgmSETNEXTSUBBUFFER(prevPathData, newPathData);

				/* Set the current equal to the last. */
				Destination->currBuffer = Destination->lastBuffer;
			}
			else
			{
				/* Get the next subbuffer. */
				nextPathData = vgmGETNEXTSUBBUFFER(currPathData);

				/* Link in. */
				vgmSETPREVSUBBUFFER(newPathData,  prevPathData);
				vgmSETNEXTSUBBUFFER(newPathData,  nextPathData);
				vgmSETPREVSUBBUFFER(nextPathData, newPathData);
				vgmSETNEXTSUBBUFFER(prevPathData, newPathData);
			}
		}

		/* Set new current path data. */
		Destination->currPathData = newPathData;

		/* Free the old subbuffer. */
		vgsPATHSTORAGE_Free(Destination->storage, currPathData, gcvFALSE);
	}
}


/*******************************************************************************
**
** vgsPATHWALKER_Rollback
**
** Free the buffers allocated from the mark.
**
** INPUT:
**
**    Destination
**       Pointer to the destination path walker.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPATHWALKER_Rollback(
	vgsPATHWALKER_PTR Destination
	)
{
	vgsPATH_DATA_PTR head;

	/* Verify the walker. */
	vgmVERIFY_WRITER(Destination);

	/* Get the head buffer. */
	head = Destination->head;

	/* Anything was allocated? */
	if (head != gcvNULL)
	{
		/* Free the buffer. */
		vgsPATHSTORAGE_Free(Destination->storage, head, gcvTRUE);
	}

	/* Validate the storage. */
	vgmVALIDATE_STORAGE(Destination->storage);
}

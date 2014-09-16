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

/* Segment normalization function name. */
#define vgmNORMAL(Command) \
	_Normalize_ ## Command

/* Segment transformation function definition. */
#define vgmDEFINENORMALIZE(Command) \
	static void vgmNORMAL(Command) ( \
		vgmNORMALIZEPARAMETERS \
		)


/******************************************************************************\
********************************* Invalid Entry ********************************
\******************************************************************************/

vgmDEFINENORMALIZE(Invalid)
{
	/* This function should never be called. */
	gcmASSERT(gcvFALSE);
}


/******************************************************************************\
******************************** gcvVGCMD_CLOSE ********************************
\******************************************************************************/

vgmDEFINENORMALIZE(gcvVGCMD_CLOSE)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Update the control coordinates. */
	coords->lastX = coords->startX;
	coords->lastY = coords->startY;

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_CLOSE;
	* Command = gcvVGCMD_CLOSE;
}


/******************************************************************************\
********************************* gcvVGCMD_MOVE ********************************
\******************************************************************************/

vgmDEFINENORMALIZE(gcvVGCMD_MOVE)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	Coordinates[0] = Source->get(Source);
	Coordinates[1] = Source->get(Source);

	/* Update the control coordinates. */
	coords->startX = Coordinates[0];
	coords->startY = Coordinates[1];
	coords->lastX  = Coordinates[0];
	coords->lastY  = Coordinates[1];

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_MOVE;
	* Command = gcvVGCMD_MOVE;
}

vgmDEFINENORMALIZE(gcvVGCMD_MOVE_REL)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	Coordinates[0] = Source->get(Source) + coords->lastX;
	Coordinates[1] = Source->get(Source) + coords->lastY;

	/* Update the control coordinates. */
	coords->startX = Coordinates[0];
	coords->startY = Coordinates[1];
	coords->lastX  = Coordinates[0];
	coords->lastY  = Coordinates[1];

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_MOVE;
	* Command = gcvVGCMD_MOVE;
}


/******************************************************************************\
******************************** gcvVGCMD_LINE *********************************
\******************************************************************************/

vgmDEFINENORMALIZE(gcvVGCMD_LINE)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	Coordinates[0] = Source->get(Source);
	Coordinates[1] = Source->get(Source);

	/* Update the control coordinates. */
	coords->lastX = Coordinates[0];
	coords->lastY = Coordinates[1];

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_LINE;
	* Command = gcvVGCMD_LINE;
}

vgmDEFINENORMALIZE(gcvVGCMD_LINE_REL)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	Coordinates[0] = Source->get(Source) + coords->lastX;
	Coordinates[1] = Source->get(Source) + coords->lastY;

	/* Update the control coordinates. */
	coords->lastX = Coordinates[0];
	coords->lastY = Coordinates[1];

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_LINE;
	* Command = gcvVGCMD_LINE;
}


/******************************************************************************\
********************************* gcvVGCMD_QUAD ********************************
\******************************************************************************/

vgmDEFINENORMALIZE(gcvVGCMD_QUAD)
{
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;
	gctFLOAT tempX, tempY;

	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	controlX = Source->get(Source);
	controlY = Source->get(Source);
	quadToX  = Source->get(Source);
	quadToY  = Source->get(Source);

	/* Compute intermediate values. */
	tempX = controlX * 2.0f;
	tempY = controlY * 2.0f;

	/* Determine cubic control points. */
	Coordinates[0] = (1.0f / 3.0f) * (tempX + coords->lastX);
	Coordinates[1] = (1.0f / 3.0f) * (tempY + coords->lastY);
	Coordinates[2] = (1.0f / 3.0f) * (tempX + quadToX);
	Coordinates[3] = (1.0f / 3.0f) * (tempY + quadToY);
	Coordinates[4] = quadToX;
	Coordinates[5] = quadToY;

	/* Update the control coordinates. */
	coords->lastX = quadToX;
	coords->lastY = quadToY;

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_CUBIC;
	* Command = gcvVGCMD_CUBIC;
}

vgmDEFINENORMALIZE(gcvVGCMD_QUAD_REL)
{
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;
	gctFLOAT tempX, tempY;

	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	controlX = Source->get(Source) + coords->lastX;
	controlY = Source->get(Source) + coords->lastY;
	quadToX  = Source->get(Source) + coords->lastX;
	quadToY  = Source->get(Source) + coords->lastY;

	/* Compute intermediate values. */
	tempX = controlX * 2.0f;
	tempY = controlY * 2.0f;

	/* Determine cubic control points. */
	Coordinates[0] = (1.0f / 3.0f) * (tempX + coords->lastX);
	Coordinates[1] = (1.0f / 3.0f) * (tempY + coords->lastY);
	Coordinates[2] = (1.0f / 3.0f) * (tempX + quadToX);
	Coordinates[3] = (1.0f / 3.0f) * (tempY + quadToY);
	Coordinates[4] = quadToX;
	Coordinates[5] = quadToY;

	/* Update the control coordinates. */
	coords->lastX = quadToX;
	coords->lastY = quadToY;

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_CUBIC;
	* Command = gcvVGCMD_CUBIC;
}


/******************************************************************************\
******************************** gcvVGCMD_CUBIC ********************************
\******************************************************************************/

vgmDEFINENORMALIZE(gcvVGCMD_CUBIC)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	Coordinates[0] = Source->get(Source);
	Coordinates[1] = Source->get(Source);
	Coordinates[2] = Source->get(Source);
	Coordinates[3] = Source->get(Source);
	Coordinates[4] = Source->get(Source);
	Coordinates[5] = Source->get(Source);

	/* Update the control coordinates. */
	coords->lastX = Coordinates[4];
	coords->lastY = Coordinates[5];

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_CUBIC;
	* Command = gcvVGCMD_CUBIC;
}

vgmDEFINENORMALIZE(gcvVGCMD_CUBIC_REL)
{
	/* Get a shortcut to the control coordinates. */
	vgsCONTROL_COORD_PTR coords = Source->coords;

	/* Retrieve the source coordinates. */
	Coordinates[0] = Source->get(Source) + coords->lastX;
	Coordinates[1] = Source->get(Source) + coords->lastY;
	Coordinates[2] = Source->get(Source) + coords->lastX;
	Coordinates[3] = Source->get(Source) + coords->lastY;
	Coordinates[4] = Source->get(Source) + coords->lastX;
	Coordinates[5] = Source->get(Source) + coords->lastY;

	/* Update the control coordinates. */
	coords->lastX = Coordinates[4];
	coords->lastY = Coordinates[5];

	/* Set the normalized commands. */
	* Group   = gcvVGCMD_CUBIC;
	* Command = gcvVGCMD_CUBIC;
}


/******************************************************************************\
********************************* gcvVGCMD_ARC *********************************
\******************************************************************************/

vgmDEFINENORMALIZE(gcvVGCMD_ARC)
{
	vgsCONTROL_COORD_PTR coords;
	vgsARCCOORDINATES_PTR arcCoords;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Get the ARC coordinates. */
	arcCoords = (vgsARCCOORDINATES_PTR) Source->currPathData->extra;
	gcmASSERT(arcCoords != gcvNULL);

	/* Retrieve the source coordinates. */
	Coordinates[0] = arcCoords->horRadius;
	Coordinates[1] = arcCoords->verRadius;
	Coordinates[2] = arcCoords->rotAngle;
	Coordinates[3] = arcCoords->endX;
	Coordinates[4] = arcCoords->endY;

	/* Update the control coordinates. */
	coords->lastX = Coordinates[3];
	coords->lastY = Coordinates[4];

	/* Set the normalized commands. */
	* Group = gcvVGCMD_ARC_MOD;

	if (arcCoords->large)
	{
		if (arcCoords->counterClockwise)
		{
			* Command = gcvVGCMD_LCCWARC;
		}
		else
		{
			* Command = gcvVGCMD_LCWARC;
		}
	}
	else
	{
		if (arcCoords->counterClockwise)
		{
			* Command = gcvVGCMD_SCCWARC;
		}
		else
		{
			* Command = gcvVGCMD_SCWARC;
		}
	}

	/* Skip the ARC path in the source. */
	vgsPATHWALKER_SeekToEnd(Source);
}

vgmDEFINENORMALIZE(gcvVGCMD_ARC_REL)
{
	vgsCONTROL_COORD_PTR coords;
	vgsARCCOORDINATES_PTR arcCoords;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Get the ARC coordinates. */
	arcCoords = (vgsARCCOORDINATES_PTR) Source->currPathData->extra;
	gcmASSERT(arcCoords != gcvNULL);

	/* Retrieve the source coordinates. */
	Coordinates[0] = arcCoords->horRadius;
	Coordinates[1] = arcCoords->verRadius;
	Coordinates[2] = arcCoords->rotAngle;
	Coordinates[3] = arcCoords->endX + coords->lastX;
	Coordinates[4] = arcCoords->endY + coords->lastY;

	/* Update the control coordinates. */
	coords->lastX = Coordinates[3];
	coords->lastY = Coordinates[4];

	/* Set the normalized commands. */
	* Group = gcvVGCMD_ARC_MOD;

	if (arcCoords->large)
	{
		if (arcCoords->counterClockwise)
		{
			* Command = gcvVGCMD_LCCWARC;
		}
		else
		{
			* Command = gcvVGCMD_LCWARC;
		}
	}
	else
	{
		if (arcCoords->counterClockwise)
		{
			* Command = gcvVGCMD_SCCWARC;
		}
		else
		{
			* Command = gcvVGCMD_SCWARC;
		}
	}

	/* Skip the ARC path in the source. */
	vgsPATHWALKER_SeekToEnd(Source);
}


/*******************************************************************************
**
** vgfGetNormalizationArray
**
** Returns the pointer to the array of coordinate normalization functions.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Array
**       Pointer to the function array.
**
**    Count
**       Pointer to the number of functions in the array.
*/

void vgfGetNormalizationArray(
	vgtNORMALIZEFUNCTION ** Array,
	gctUINT_PTR Count
	)
{
	static vgtNORMALIZEFUNCTION _normalizeSegment[] =
	{
		vgmNORMAL(Invalid),                  /*   0: gcvVGCMD_END             */
		vgmNORMAL(gcvVGCMD_CLOSE),           /*   1: gcvVGCMD_CLOSE           */
		vgmNORMAL(gcvVGCMD_MOVE),            /*   2: gcvVGCMD_MOVE            */
		vgmNORMAL(gcvVGCMD_MOVE_REL),        /*   3: gcvVGCMD_MOVE_REL        */
		vgmNORMAL(gcvVGCMD_LINE),            /*   4: gcvVGCMD_LINE            */
		vgmNORMAL(gcvVGCMD_LINE_REL),        /*   5: gcvVGCMD_LINE_REL        */
		vgmNORMAL(gcvVGCMD_QUAD),            /*   6: gcvVGCMD_QUAD            */
		vgmNORMAL(gcvVGCMD_QUAD_REL),        /*   7: gcvVGCMD_QUAD_REL        */
		vgmNORMAL(gcvVGCMD_CUBIC),           /*   8: gcvVGCMD_CUBIC           */
		vgmNORMAL(gcvVGCMD_CUBIC_REL),       /*   9: gcvVGCMD_CUBIC_REL       */
		vgmNORMAL(Invalid),                  /*  10: gcvVGCMD_BREAK           */
		vgmNORMAL(Invalid),                  /*  11: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  12: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  13: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  14: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  15: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  16: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  17: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  18: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  19: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  20: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  21: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  22: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  23: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  24: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  25: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  26: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  27: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  28: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  29: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  30: **** R E S E R V E D *****/
		vgmNORMAL(Invalid),                  /*  31: **** R E S E R V E D *****/

		vgmNORMAL(Invalid),                  /*  32: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  33: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  34: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  35: ***** I N V A L I D ******/
		vgmNORMAL(gcvVGCMD_LINE),            /*  36: gcvVGCMD_HLINE_EMUL      */
		vgmNORMAL(gcvVGCMD_LINE_REL),        /*  37: gcvVGCMD_HLINE_EMUL_REL  */
		vgmNORMAL(Invalid),                  /*  38: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  39: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  40: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  41: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  42: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  43: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  44: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  45: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  46: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  47: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  48: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  49: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  50: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  51: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  52: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  53: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  54: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  55: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  56: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  57: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  58: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  59: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  60: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  61: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  62: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  63: ***** I N V A L I D ******/

		vgmNORMAL(Invalid),                  /*  64: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  65: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  66: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  67: ***** I N V A L I D ******/
		vgmNORMAL(gcvVGCMD_LINE),            /*  68: gcvVGCMD_VLINE_EMUL      */
		vgmNORMAL(gcvVGCMD_LINE_REL),        /*  69: gcvVGCMD_VLINE_EMUL_REL  */
		vgmNORMAL(Invalid),                  /*  70: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  71: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  72: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  73: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  74: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  75: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  76: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  77: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  78: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  79: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  80: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  81: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  82: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  83: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  84: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  85: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  86: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  87: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  88: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  89: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  90: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  91: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  92: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  93: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  94: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  95: ***** I N V A L I D ******/

		vgmNORMAL(Invalid),                  /*  96: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  97: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  98: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /*  99: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 100: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 101: ***** I N V A L I D ******/
		vgmNORMAL(gcvVGCMD_QUAD),            /* 102: gcvVGCMD_SQUAD_EMUL      */
		vgmNORMAL(gcvVGCMD_QUAD_REL),        /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
		vgmNORMAL(gcvVGCMD_CUBIC),           /* 104: gcvVGCMD_SCUBIC_EMUL     */
		vgmNORMAL(gcvVGCMD_CUBIC_REL),       /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
		vgmNORMAL(Invalid),                  /* 106: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 107: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 108: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 109: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 110: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 111: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 112: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 113: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 114: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 115: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 116: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 117: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 118: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 119: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 120: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 121: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 122: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 123: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 124: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 125: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 126: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 127: ***** I N V A L I D ******/

		vgmNORMAL(Invalid),                  /* 128: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 129: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 130: ***** I N V A L I D ******/
		vgmNORMAL(Invalid),                  /* 131: ***** I N V A L I D ******/
		vgmNORMAL(gcvVGCMD_ARC),             /* 132: gcvVGCMD_ARC_LINE        */
		vgmNORMAL(gcvVGCMD_ARC_REL),         /* 133: gcvVGCMD_ARC_LINE_REL    */
		vgmNORMAL(gcvVGCMD_ARC),             /* 134: gcvVGCMD_ARC_QUAD        */
		vgmNORMAL(gcvVGCMD_ARC_REL)          /* 135: gcvVGCMD_ARC_QUAD_REL    */
	};

	* Array = _normalizeSegment;
	* Count = gcmCOUNTOF(_normalizeSegment);
}

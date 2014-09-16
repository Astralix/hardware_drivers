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
#include "gc_vgsh_dump.h"
#include "gc_hal_user.h"

gctUINT32 ZoneMask;
gctUINT32 DebugLevel;

#ifdef OVG_DBGOUT
static void
_Print(
	IN char * Message,
	IN gctARGUMENTS Arguments
	)
{
	char buffer[256];
	gctUINT32 offset = 0;

	gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer), &offset,
						Message, Arguments);

	gcmPRINT(buffer);
}
#endif

void OVG_DebugTrace(
	IN gctUINT32 Level,
	IN char* Message,
	...
	)
{
#ifdef OVG_DBGOUT
    gctARGUMENTS arguments;
	/* Verify that the debug level is valid. */
	if ((int) Level > DebugLevel)
	{
		return;
	}

    gcmARGUMENTS_START(arguments, Message);
    _Print(Message, arguments);
    gcmARGUMENTS_END(arguments);
#endif
}


void OVG_DebugTraceZone(
	IN gctUINT32 Level,
	IN gctUINT32 Zone,
	IN char * Message,
	...
	)
{
#ifdef OVG_DBGOUT
    gctARGUMENTS arguments;
	/* Verify that the debug level and zone are valid. */
	if (((int) Level > DebugLevel) || !(ZoneMask & Zone))
	{
		return;
	}

    gcmARGUMENTS_START(arguments, Message);
    _Print(Message, arguments);
    gcmARGUMENTS_END(arguments);
#endif
}

void OVG_SetDebugLevel(
	IN gctUINT32 Level
	)
{
	DebugLevel = Level;
}

void OVG_SetDebugZone(
	IN gctUINT32 Zone
	)
{
	ZoneMask = Zone;
}

#ifdef OVG_DBGOUT
gctINT32 GetTime(void)
{
	return gcoOS_GetTicks();
}
#endif




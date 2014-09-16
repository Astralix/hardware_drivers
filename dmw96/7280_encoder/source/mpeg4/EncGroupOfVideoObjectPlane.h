/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncGroupOfVideoObjectPlane.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_GROUP_OF_VIDEO_OBJECT_PLANE_H__
#define __ENC_GROUP_OF_VIDEO_OBJECT_PLANE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncUserData.h"
#include "encputbits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
	true_e	header;
	i32	timeCodeHours;
	i32	timeCodeMinutes;
	i32	timeCodeSecond;
	i32	closedGov;
	i32	brokenLink;
	userData_s userData;
} govop_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncGoVopHdr(stream_s *, govop_s *);
void EncGoVopInit(govop_s *govop);

#endif

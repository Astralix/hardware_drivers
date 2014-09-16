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
--  $RCSfile: EncVisualObjectSequence.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_VISUAL_OBJECT_SEQUENCE_H__
#define __ENC_VISUAL_OBJECT_SEQUENCE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "encputbits.h"
#include "EncUserData.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
	true_e	header;		/* ENCHW_YES/ENCHW_NO    */
	i32	profile;	/* Profile and level (Table G-1) */
	userData_s userData;
} vos_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncVosInit(vos_s *vos);
void EncVosHrd(stream_s *, vos_s *);
void EncVosEndHrd(stream_s *, vos_s *);

#endif

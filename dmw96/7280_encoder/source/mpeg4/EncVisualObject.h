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
--  $RCSfile: EncVisualObject.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_VISUAL_OBJECT_H__
#define __ENC_VISUAL_OBJECT_H__

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
typedef enum {
	VIDEO_ID         = 1,
	STILL_TEXTURE_ID = 2,
	MESH_ID          = 3,
	FBA_ID           = 4,
	D3_MESH_ID       = 5
} visualObjectType_e;

typedef struct {
	true_e	videoSignalType;		/* ENCHW_YES/ENCHW_NO    */
	i32	videoFormat;			/* Table 6-6 */
	i32	videoRange;
	true_e	colourDescription;		/* ENCHW_YES/ENCHW_NO    */
	i32	colourPrimaries;		/* Table 6-7 */
	i32	transferCharacteristics;	/* Table 6-8 */
	i32	matrixCoefficients;		/* Table 6-9 */
} videoSignalType_s;

typedef struct {
	true_e	header;
	true_e	isVisualObjectIdentifier;	/* ENCHW_YES/ENCHW_NO    */
	i32	visualObjectVerid;		/* Table 6-4 */
	i32	visualObjectPriority;		/* [1,2...7] */
	visualObjectType_e visualObjectType;	/* Table 6-5 */
	videoSignalType_s videoSignalType;
	userData_s userData;
} visob_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncVisObInit(visob_s *);
void EncVisObHdr(stream_s *, visob_s *);

#endif

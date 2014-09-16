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


#include "gc_glsl_common.h"

#define HASH_MULTIPLIER		31		/* the multiplier of Hash */

gctHASH_VALUE
slHashString(
	IN gctCONST_STRING String
	)
{
	gctHASH_VALUE		hashValue = 0;
	gctCONST_CHAR_PTR	ch;

	for (ch = String; *ch != '\0'; ch++)
	{
		hashValue = HASH_MULTIPLIER * hashValue + *ch;
	}

	return hashValue;
}

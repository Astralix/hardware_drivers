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


#ifndef __gc_vgsh_debug_h_
#define __gc_vgsh_debug_h_

#ifdef __cplusplus
extern "C" {
#endif

VG_API_CALL void OVGFrameDump(gctFILE *pFile, int flag);
VG_API_CALL void OVGAvgFrameDump(gctFILE *pFile);
VG_API_CALL void OVGSetNonCache(void);

#ifdef __cplusplus
}
#endif

#endif /* __gc_vgsh_debug_h_ */

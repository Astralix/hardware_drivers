//
//  wiExternal.h -- Skeleton for external code to be include in WiSE build
//  Copyright 2005 DSP Group, Inc. All rights reserved.
//

#ifndef __WIEXTERNAL_H
#define __WIEXTERNAL_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=============================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=============================================================================
int      wiExternal(void);
wiStatus wiExternal_Init(void);
wiStatus wiExternal_Close(void);
//-----------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-----------------------------------------------------------------------------
//--- End of File -------------------------------------------------------------

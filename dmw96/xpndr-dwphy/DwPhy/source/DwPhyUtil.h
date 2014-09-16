//
// DwPhyUtil.h -- Driver/Interface Functions for DW52 (not included in driver)
// Copyright 2007 DSP Group, Inc. All rights reserved.
//

#ifndef __DWPHYUTIL_H
#define __DWPHYUTIL_H

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// ================================================================================================
// GENERIC INTEGER DATA TYPES
// ...these may be defined elsewhere in other projects, so beware
// ================================================================================================
#ifndef ANDROID
#ifndef __int8_t_defined
typedef   signed char   int8_t;
typedef   signed short  int16_t;
typedef   signed int    int32_t;
#endif

#ifndef __uint32_t_defined
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#endif
#endif

// ================================================================================================
// Utility Functions
// ================================================================================================
char *   DwPhyUtil_RadioName   (unsigned int PartID);
char *   DwPhyUtil_BasebandName(unsigned int PartID);
int32_t  ReadMib( uint16_t MibObjectID, void *pMibValue );
int32_t  WriteMib( uint16_t MibObjectID, void *pMibValue );

// ================================================================================================
// OS Wrapper Style Functions (these must match what is used internally by the WiFi driver)
// ================================================================================================
uint32_t OS_RegRead   ( uint32_t Addr                 );
void     OS_RegWrite  ( uint32_t Addr, uint32_t Value );
uint8_t  OS_RegReadBB ( uint32_t Addr                 );
void     OS_RegWriteBB( uint32_t Addr, uint8_t Value  );
void     OS_Delay     ( uint32_t microsecond );

uint32_t DwPhyEnableFn( void *pDevInfo, uint32_t Enable );

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

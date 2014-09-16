//
//  Wise.h -- Wireless Simulation Environment
//  Copyright 2001-2003 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.

#ifndef __WISE_H
#define __WISE_H

/////////////////////////////////////////////////////////////////////////////////////////
//
//  PREVENT C++ NAME MANGLING
//
//  Although C++ code may be included in the library when it is built, the interface
//  for this library requires that all functions are exported as standard 'C' functions
//
#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//
//  VERSION CODES
//
//  The primary WiSE version is the date code applied to the archive. The X.XX.XX version
//  style is used for releases, but is subject to manual update. The same is true for the
//  the BUILD_VERSION code which is intended to reflect the date code format. The latter
//  can be used in compiler defines to select compatibility with different versions of
//  WiSE. Versions prior to 090313B (3.05) do not supply this definition.
//
#define WISE_VERSION_STRING "WiSE 4.04.1"
#define WISE_BUILD_VERSION  0x110922A
        
/////////////////////////////////////////////////////////////////////////////////////////
//
//  CONDITIONAL BUILDS
//
#if defined(WISE_VCS_BUILD_MOJAVE) /////////////////  DW52 RTL Verification
    #define WISE_BUILD_INCLUDE_DW52
#endif

#if defined(WISE_VCS_BUILD_NEVADA) /////////////////  DMW96 RTL Verification
    #define WISE_BUILD_INCLUDE_NEVADAPHY
#endif

#if defined(WISE_BUILD_WISEMEX) ////////////////////  Matlab Library (wiseMex)

    #define WISE_BUILD_INCLUDE_CALCEVM
    #define WISE_BUILD_INCLUDE_NEVADAPHY
    #define WISE_BUILD_INCLUDE_TAMALEPHY
	#define WISE_BUILD_INCLUDE_PHY11N

#endif
    
#if defined(WISE_BUILD_STANDARD) ///////////////////  Standard Console Application

    #define WISE_BUILD_INCLUDE_CALCEVM
    #define WISE_BUILD_INCLUDE_NEVADAPHY
    #define WISE_BUILD_INCLUDE_TAMALEPHY
	#define WISE_BUILD_INCLUDE_PHY11N

#endif

/////////////////////////////////////////////////////////////////////////////////////////
//
//  PHY MODEL SPECIFIC HEADERS
//
#if defined (WISE_BUILD_INCLUDE_CALCEVM) ////////////////  CalcEVM (OFDM EVM Calculator)
    #include "CalcEVM.h"
    #include "CalcEVM_OFDM.h"
#endif

#if defined (WISE_BUILD_INCLUDE_DW52) ///////////////////  DW52 (Mojave Baseband)
    #include "DW52.h"
    #include "Mojave.h"
    #include "MojaveTest.h"
    #include "RF52.h"
#endif

#if defined (WISE_BUILD_INCLUDE_NEVADAPHY) //////////////  NevadaPhy (DMW96, Nevada)
    #include "NevadaPHY.h"
#endif

#if defined (WISE_BUILD_INCLUDE_TAMALEPHY) //////////////  TamalePhy (Tamale Baeband)
    #include "TamalePHY.h"
#endif

#if defined (WISE_BUILD_INCLUDE_PHY11N) /////////////////  Phy11n (802.11n 20 MHz MIMO)
    #include "Phy11n.h"
#endif

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __WISE_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

/*
    Phy11nTest.h -- Phy11n Test Routines
    Copyright 2006 DSP Group, Inc. All rights reserved.
*/

#ifndef __PHY11NTEST_H
#define __PHY11NTEST_H

#include "wiType.h"
#include "wiMatrix.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=============================================================================
//  TEST PARAMETER STRUCTURE
//=============================================================================
typedef struct
{
    int H52_Length;      // Length of LastH, which is the number of subcarriers to consider
    wiCMat H52[52];      // Latest channel estimates at the Rx, to be used for beamforming, etc.
    wiCMat *Steering;    // Steering matrices for beamforming
    int Steering_Length; // Length of Steering, which is the number of subcarriers for beamforming
    int Sounding;        // 0: Data packet; 1: Sounding packet;
}  Phy11nTest_State_t;

//=============================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=============================================================================
wiStatus Phy11nTest_Init(void);
wiStatus Phy11nTest_Close(void);
Phy11nTest_State_t* Phy11nTest_State(void);
//-----------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-----------------------------------------------------------------------------
//--- End of File -------------------------------------------------------------
// Revised 2010-10-13 x.yu: Syntax changes to be compatible with current WISE

//
//  Mojave_DigitalFrontEnd.h -- RX Front End for Mojave
//  Copyright 2002-2003, Bermai, 2005 DSP Group, Inc. All rights reserved.
//

#ifndef __MOJAVE_DIGITALFRONTEND_H
#define __MOJAVE_DIGITALFRONTEND_H

#include "wiType.h"
#include "Mojave.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
void aMojave_DFT_pilot( wiWORD xR[], wiWORD xI[], wiWORD *pR, wiWORD *pI );

void aMojave_DFS( wiBoolean DFSEnB, Mojave_aSigState SigIn, Mojave_aSigState *SigOut, Mojave_RX_State *pRX, MojaveRegisters *pReg );

void aMojave_DigitalFrontEnd( wiBoolean DFEEnB, wiBoolean DFERestart, wiBoolean DFELgSigAFE,
                              wiWORD  DFEIn0R,  wiWORD  DFEIn0I,  wiWORD  DFEIn1R,  wiWORD  DFEIn1I,
                              wiWORD *DFEOut0R, wiWORD *DFEOut0I, wiWORD *DFEOut1R, wiWORD *DFEOut1I,
                              wiUWORD *DFELNAGain, wiUWORD *DFEAGain,
                              wiUWORD *DFERSSI0, wiUWORD *DFERSSI1, wiUWORD *DFECCA,
                              Mojave_RX_State *pRX, MojaveRegisters *pReg, wiBoolean CSCCK, 
                              wiBoolean RestartRX, Mojave_aSigState *aSigOut, Mojave_aSigState aSigIn);
 
void aMojave_PhaseCorrect( wiUWORD PathSel, int Nk, wiWORD cSP, wiWORD cCP, wiWORD cCPP, Mojave_RX_State *pRX,
                           wiWORD *v0R, wiWORD *v0I, wiWORD *v1R, wiWORD *v1I,
                           wiWORD *u0R, wiWORD *u0I, wiWORD *u1R, wiWORD *u1I );

void aMojave_PLL( wiWORD I1[], wiWORD Q1[], wiWORD H2[], wiWORD DI[], wiWORD DQ[], int iFlag, wiWORD *AdvDly, 
                  wiWORD *_cSP, wiWORD *_cCP, Mojave_RX_State *pRX, MojaveRegisters *pReg );

void aMojave_WeightedPilotPhaseTrack( int FInit, wiUWORD PathSel, 
                                      wiWORD v0R[],   wiWORD v0I[], wiWORD v1R[],   wiWORD v1I[],
                                      wiWORD h0R[],   wiWORD h0I[], wiWORD h1R[],   wiWORD h1I[],
                                      wiWORD cSP, wiWORD AdvDly, wiWORD *cCPP, MojaveRegisters *pReg );
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------


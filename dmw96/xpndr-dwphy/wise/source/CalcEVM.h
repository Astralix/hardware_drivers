//
//  CalcEVM.h -- PHY for OFDM EVM Calculation
//  Copyright 2006-2007 DSP Group, Inc. All rights reserved.
//

#ifndef __CALCEVM_H
#define __CALCEVM_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  CalcEVM PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiBoolean UseMojaveRX; // use Mojave RX for EVM calculator
    wiBoolean UseMojaveTX; // 1: use DW52-Mojave TX, otherwise using floating point TX;  

    struct {
        wiInt      N_SYM;      // Number of symbols in EVM
        wiComplex  h[64];      // Estimated channel response
        wiComplex  *x;         // 64 subcarriers by N_SYM symbols  //equalized symbols
        wiComplex  *u;         // 64 subcarriers by N_SYM symbols  //phase corrector output   
        wiReal     cSe2[64];   // Se2 by subcarrier
        wiReal     nSe2[1367]; // Se2 by symbol #
        wiReal     Se2;        // Sum of squared errors
        wiReal     EVM;        // Measured EVM (linear)
        wiReal     EVMdB;      // Measured EVM [dB]
        wiReal     cCFO;       // Coarse estimated CFO (fraction of channel)
        wiReal     fCFO;       // Fine estimated CFO
        wiReal     W_EVMdB;    // temp; added 11/29/06 weighted EVM
    } EVM;                           

    struct
    {
        int        Ns;
        wiComplex *s[1]; // radio output, channel input signal
    } TX;

}  CalcEVM_State_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus CalcEVM(int command, void *pdata);
CalcEVM_State_t *CalcEVM_State(void);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------


//--< Phy11nTest.c >-------------------------------------------------------------------------------
//=================================================================================================
//
//  Phy11n Specific Test Routines
//  Copyright 2006 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "Phy11nTest.h"
#include "wiChanl.h"
#include "wiParse.h"
#include "wiPHY.h"
#include "wiTest.h"
#include "wiTGnChanl.h"
#include "wiUtil.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean           Verbose = 0;
static wiStatus            Status  = 0;
static Phy11nTest_State_t *State   = 0;
static wiTest_State_t     *pState  = 0;
static FILE               *OUTF    = 0;
static FILE               *LOGF    = 0;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (WICHK(arg) <0 ) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((Status=WICHK(arg)) <=0 ) return Status;

static wiStatus Phy11nTest_ParseLine(wiString text);

// ================================================================================================
// FUNCTION  : Phy11nTest_Init
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11nTest_Init(void)
{
    if (State) return WI_ERROR_MODULE_ALREADY_INIT;

    WICALLOC (State, 1, Phy11nTest_State_t );
    State->H52_Length=0;

    XSTATUS(wiParse_AddMethod(Phy11nTest_ParseLine));
    return WI_SUCCESS;
}
// end of Phy11nTest_Init()

// ================================================================================================
// FUNCTION  : Phy11nTest_Close
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11nTest_Close(void)
{
    int k=0;
    if (!State) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);
    
    // Free the memory allocated to State->H52
    for (k=0; k < State->H52_Length; k++)
        wiCMatFree(&(State->H52[k]));
    State->H52_Length=0;

    // Free the memory allocated to State
    WIFREE( State );
    XSTATUS(wiParse_RemoveMethod(Phy11nTest_ParseLine));
    return WI_SUCCESS;
}
// end of Phy11nTest_Close()

// ================================================================================================
// FUNCTION : Phy11nTest_State
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Phy11nTest_State_t* Phy11nTest_State(void)
{
    if (!State) STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);
    return State;
}
// end of Phy11nTest_State()


// ================================================================================================
// FUNCTION  : Phy11nTest_PER_vs_SNR_BF()
// ------------------------------------------------------------------------------------------------
// Purpose   : Single packet transmit/receive
// Parameters: none
// ================================================================================================
wiStatus Phy11nTest_PER_vs_SNR_BF(void)
{
    wiComplex **R; int N;
    wiChanl_State_t *pChanl;
    wiReal  SNR;

    wiTGnChanl_State_t *TGnState = wiTGnChanl_State();

    XSTATUS( wiTest_StartTest("wiTest_PER_vs_SNR()") );

    pState = wiTest_State();

    wiTestWr("\n");
    wiTestWr(" Eb/No(dB)  # Errors     BER     # Failures     PER    \n");
    wiTestWr(" ---------  --------  ---------  ----------  --------- \n");

    for (SNR=pState->MinSNR; SNR<=pState->MaxSNR; SNR+=pState->StepSNR)
    {
        XSTATUS( wiTest_ClearCounters() );
        while ((pState->PacketErrorCount<pState->MinFail) || (pState->PacketCount < pState->MinPacketCount))
        {
            State->Sounding = 1;  // This is a sounding packet
            // TX/RX Sequence to obtain CSI
            if (Verbose) printf("-------> Transmitting the sounding packet.\n");
            XSTATUS( wiTest_TxPacket(0, SNR, pState->DataRate, pState->Length, pState->DataType, &N, &R, NULL) );          
			pChanl = wiChanl_State();
            if (Verbose) printf("-------> Receiving the sounding packet.\n");
            XSTATUS( wiPHY_RX(NULL, &(pState->RxData[wiUtil_ThreadIndex()]), N, R) );
            
            TGnState->CSI.Hold = 1; // Hold On; Use the existing FilterStates to generate the channel <<< add flag to hold channel >>>

            State->Sounding = 0;
            // TX/RX Sequence for PER measurement
            if (Verbose) printf("-------> TxRx the Data packet.\n");
            XSTATUS( wiTest_TxRxPacket(0, SNR, pState->DataRate, pState->Length) );
            if (pState->PacketCount >= pState->MaxPackets) break;

            TGnState->CSI.Hold = 0; // Hold Off; Generate a whole new channel <<< clear flag to hold channel >>>
        }
        wiPrintf(" Eb/No=% 5.1f: BER = %8.2e, PER = %8.2e\n", SNR, pState->BER, pState->PER);
        wiTestWr(" % 8.2f  % 8d  % 9.2e % 10d  % 9.2e\n", SNR, pState->BitErrorCount, pState->BER, pState->PacketErrorCount, pState->PER);
        if (pState->PER < pState->MinPER) break;
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of Phy11nTest_PER_vs_SNR_BF()

// ================================================================================================
// FUNCTION  : Phy11nTest_ParseLine
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static wiStatus Phy11nTest_ParseLine(wiString text)
{
    wiStatus status;

    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;

    XSTATUS(status = wiParse_Command(text,"Phy11nTest_PER_vs_SNR_BF()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(Phy11nTest_PER_vs_SNR_BF());
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of Phy11nTest_ParseLine()

/***************************************************************************************/
/*=== END OF SOURCE CODE ==============================================================*/
/***************************************************************************************/



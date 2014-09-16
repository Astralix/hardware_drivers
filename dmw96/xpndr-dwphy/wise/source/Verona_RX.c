//--< Verona_RX.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Verona - Baseband Receiver Model
//  Copyright 2001-2003 Bermai, 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <memory.h>
#include "wiMAC.h"
#include "wiRnd.h"
#include "wiUtil.h"
#include "Verona.h"

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

#define ClockRegister(arg)       {(arg).Q=(arg).D;};
#define ClockDelayLine(SR,value) {(SR).word = ((SR).word << 1) | ((value)&1);};

#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#endif

// ================================================================================================
// FUNCTION  : Verona_RX_ClockGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate clocks for the baseband receiver. Internally, the module operates
//             an 880 MHz clock and produces {11,22,44,20,40,80} MHz divided clocks. 
//
//             When called, the master clock is run until at least one positive edge occurs on one
//             of the generated clocks. The generated clocks are used to time cycle-accurate 
//             modules. Clock edge indicators are stored in the Verona RX state.
//
// Parameters: pRX   - Receiver state structure
//             Reset - Reset and synchronize clocks (active high)
// ================================================================================================
void Verona_RX_ClockGenerator(Verona_RxState_t *pRX, wiBoolean Reset)
{
    const wiUWORD InitClockGate40 = {0xAAAAA}; // 80->40 MHz clock gating cadence
    const wiUWORD InitClockGate44 = {0xAAB55}; // 80->44 MHz clock gating cadence

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Reset clock - use to set the clock generator state
    //
    //  The initial phase is specified as an integer 0-79 that moves the clock generator the
    //  specified number of cycles before continuing. If a negative value is supplied, the 
    //  phase is randomly generated.
    //
    if (Reset)
    {
        unsigned int k, InitialPhase;

        pRX->clock.posedgeCLK80MHz = 1;
        pRX->clock.clockGate40 = InitClockGate40;
        pRX->clock.clockGate44 = InitClockGate44;
        pRX->clock.clock40 = pRX->clock.clock44 = 3;

        if (pRX->clock.InitialPhase < 0)
            InitialPhase = (unsigned int)(80.0 * wiRnd_UniformSample());
        else InitialPhase = pRX->clock.InitialPhase;

        for (k=0; k<InitialPhase; k++) // advance clock generator specified periods
            Verona_RX_ClockGenerator(pRX, 0);

        // clear array indices
        pRX->k80 = pRX->k40 = pRX->k20 = pRX->k44 = pRX->k22 = pRX->k11 = 0;
    }
    pRX->clock.posedgeCLK40MHz = pRX->clock.clockGate40.b19;
    pRX->clock.posedgeCLK44MHz = pRX->clock.clockGate44.b19;
    
    pRX->clock.clockGate40.w20 = (pRX->clock.clockGate40.w19 << 1) | pRX->clock.clockGate40.b19;
    pRX->clock.clockGate44.w20 = (pRX->clock.clockGate44.w19 << 1) | pRX->clock.clockGate44.b19;

    if (pRX->clock.posedgeCLK40MHz) pRX->clock.clock40++;
    if (pRX->clock.posedgeCLK44MHz) pRX->clock.clock44++;

    // Indicate a positive edge for the divided clocks
    //
    pRX->clock.posedgeCLK20MHz = pRX->clock.posedgeCLK40MHz & (pRX->clock.clock40 % 2 ? 0:1);
    pRX->clock.posedgeCLK22MHz = pRX->clock.posedgeCLK44MHz & (pRX->clock.clock44 % 2 ? 0:1);
    pRX->clock.posedgeCLK11MHz = pRX->clock.posedgeCLK22MHz & (pRX->clock.clock44 % 4 ? 0:1);

    // Increment sample counters
    // 
    if (pRX->clock.posedgeCLK80MHz) pRX->k80++;
    if (pRX->clock.posedgeCLK40MHz) pRX->k40++;
    if (pRX->clock.posedgeCLK20MHz) pRX->k20++;
    if (pRX->clock.posedgeCLK44MHz) pRX->k44++;
    if (pRX->clock.posedgeCLK22MHz) pRX->k22++;
    if (pRX->clock.posedgeCLK11MHz) pRX->k11++;
}
// end of Verona_RX_ClockGenerator()

// ================================================================================================
// FUNCTION : Verona_RX_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_RX_Init(void)
{
    const int PSK[2]     = {-1,1};
    const int QAM16[4]   = {-3,-1,3,1};
    const int QAM64[8]   = {-7,-5,-1,-3, 7, 5, 1, 3};
      
    wiUWORD X, Y, A;

    Verona_RxState_t     *pRX  = Verona_RxState();
	Verona_LookupTable_t *pLut = Verona_LookupTable();
   
    // ------------------
    // Default Parameters
    // ------------------
    if (!pRX->aLength)   pRX->aLength   = 16+8*65535+6+287; // last term fills out interger # symbols @ 72  287
    if (!pRX->rLength)   pRX->rLength   = 2*65536;
    if (!pRX->ADC.nEff)  pRX->ADC.nEff  = 9;   // 9 bit effective ADC resolution   
    if (!pRX->ADC.Vpp)   pRX->ADC.Vpp   = 1.0; // 1.2Vpp full-scale input
    if (!pRX->ADC.Offset)pRX->ADC.Offset= 6;   // 6 LSB offset

    pRX->NonRTL.MaxSamplesCFO = 256;

    // Enable Model I/O and Trace variables by default
    //
    pRX->EnableModelIO = 1;
    pRX->EnableTrace   = 1;

    XSTATUS(Verona_RX_AllocateMemory());

    // ----------------------------------------------
    // Build the hard decision demapper look-up table   
    // ----------------------------------------------
    for (Y.word=0; Y.word<32; Y.word++)
    {
        for (X.word=0; X.word<16; X.word++)
        {
            A.w9 = (Y.w5<<4) | X.w4;
            switch (Y.w5)
            {
                case 0xD: case 0xF: case 0x10:                       pLut->DemodROM[A.w9].word = PSK[X.w1];   break;
                case 0x5: case 0x7: case 0x11: case 0x12:            pLut->DemodROM[A.w9].word = PSK[X.w1];   break;
                case 0x9: case 0xB: case 0x13: case 0x14:            pLut->DemodROM[A.w9].word = QAM16[X.w2]; break;
                case 0x1: case 0x3: case 0x15: case 0x16: case 0x17: pLut->DemodROM[A.w9].word = QAM64[X.w3]; break;
                default :                                            pLut->DemodROM[A.w9].word = 0;
            }
        }
    }    
    //{{1, 26}, {1,52}, {1, 78}, {1,104}, {1,156}, {1, 208}, {1, 234},{1,260}};

    return WI_SUCCESS;
}
// end of Verona_RX_Init()

// ================================================================================================
// FUNCTION  : Verona_RX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Reset model state in anticipation of an RX operation
// Parameters: none
// ================================================================================================
wiStatus Verona_RX_Restart(void)
{
    int i;

    Verona_RxState_t  *pRX  = Verona_RxState();
    Verona_bRxState_t *pRXb = Verona_bRxState();

    // Call the RX1 module local restart
    //
    XSTATUS( Verona_RX1_Restart() );

    // Compute timing for MAC_TX_EN signal
    //
    if (pRX->TX.Usek2Delay)
    {
        for (i=0; i<4; i++)
            pRX->TX.k2[i] = pRX->TX.kTxDone[i] + pRX->TX.k2Delay;
    }
    // Clear the Exception Flags
    //
    pRX->RTL_Exception.word = 0;
  
    // Initialize PHY_RX_D (the upper bits are set to create mismatch in wiTest if the data is
    // not written by an actual packet receiption.
    //
    pRX->N_PHY_RX_WR = 0;
    for (i=0; i<16; i++)
        pRX->PHY_RX_D[i].word = 0xFFFF0000 | i;

    // Clear the traceH arrays
    //
    if (pRX->EnableTrace)
    {
        for (i=0; i<2; i++)
        {
            if (pRX->traceH2[i])    memset(pRX->traceH2[i],    0, pRX->N20*sizeof(wiWORD));
            if (pRX->traceHReal[i]) memset(pRX->traceHReal[i], 0, pRX->N20*sizeof(wiWORD));
            if (pRX->traceHImag[i]) memset(pRX->traceHImag[i], 0, pRX->N20*sizeof(wiWORD));
        }
        if (pRX->traceRxClock)   memset(pRX->traceRxClock,   0, pRX->N80*sizeof(wiWORD));
        if (pRX->traceRxState)   memset(pRX->traceRxState,   0, pRX->N80*sizeof(wiWORD));
        if (pRX->traceRx)        memset(pRX->traceRx,        0, pRX->N80*sizeof(wiWORD));
        if (pRX->traceRX2)       memset(pRX->traceRX2,       0, pRX->N20*sizeof(wiWORD));
        if (pRX->traceDFE)       memset(pRX->traceDFE,       0, pRX->N20*sizeof(wiWORD));
        if (pRXb->traceControl)  memset(pRXb->traceControl,  0, pRX->N22*sizeof(wiWORD));
    }

    return WI_SUCCESS;
}
// end of Verona_RX_Restart()

// ================================================================================================
// FUNCTION  : Verona_RX_EndOfWaveform()
// ------------------------------------------------------------------------------------------------
// Purpose   : Tasks that need to run once the sample-by-sample waveform processing is complete
// ================================================================================================
wiStatus Verona_RX_EndOfWaveform(void)
{
    wiWORD Zero = {0};

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Update DCX Acculator Registers
    //
    Verona_DCX(-2, Zero, NULL, 0, Verona_RxState(), Verona_Registers()); // Load values from DCX accumulators

    return WI_SUCCESS;
}
// end of Verona_RX_EndOfWaveform()

// ================================================================================================
// FUNCTION : Verona_RX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_RX_FreeMemory(Verona_RxState_t *pRX, wiBoolean ClearPointerOnly)
{
    #define FREE(ptr)                                        \
    {                                                        \
        if (!ClearPointerOnly && (ptr != NULL)) wiFree(ptr); \
        ptr = NULL;                                          \
    }

    int n;

    FREE( pRX->PHY_RX_D );
    FREE( pRX->cSP  );
    FREE( pRX->cCP  );
    FREE( pRX->kFFT );

    FREE( pRX->a     ); pRX->Na = 0;
    FREE( pRX->b     ); pRX->Nb = 0;
    FREE( pRX->Lc    ); pRX->Nc = 0;
    FREE( pRX->Lp    ); pRX->Np = 0; 
    FREE( pRX->cA    );
    FREE( pRX->cB    );
    FREE( pRX->dReal );
    FREE( pRX->dImag );
    FREE( pRX->DReal );
    FREE( pRX->DImag );
    FREE( pRX->H2    ); pRX->Nd = 0;

    FREE( pRX->DGain    );
    FREE( pRX->AdvDly   );
    FREE( pRX->AGain    );
    FREE( pRX->DFEState );
    FREE( pRX->DFEFlags );
    FREE( pRX->aSigOut  );
    FREE( pRX->bSigOut  );

    FREE( pRX->traceRxClock   );
    FREE( pRX->traceRxState   );
    FREE( pRX->traceRxControl );
    FREE( pRX->traceRadioIO   );
    FREE( pRX->traceDataConv  );
    FREE( pRX->traceDFE       );
    FREE( pRX->traceAGC       );
    FREE( pRX->traceRSSI      );
    FREE( pRX->traceDFS       );
    FREE( pRX->traceSigDet    );
    FREE( pRX->traceFrameSync );
    FREE( pRX->traceRX2       );
    FREE( pRX->traceRx        );
    FREE( pRX->traceImgDet    );
    FREE( pRX->traceIQCal     );

    FREE( pRX->VitDec.DB_l    );
    FREE( pRX->VitDec.DB_u    );

    for (n=0; n<2; n++)
    {
        FREE( pRX->pReal[n] ); FREE( pRX->pImag[n] );
        FREE( pRX->qReal[n] ); FREE( pRX->qImag[n] );
        FREE( pRX->cReal[n] ); FREE( pRX->cImag[n] );
        FREE( pRX->tReal[n] ); FREE( pRX->tImag[n] );
        FREE( pRX->rReal[n] ); FREE( pRX->rImag[n] );
        FREE( pRX->sReal[n] ); FREE( pRX->sImag[n] );
        FREE( pRX->uReal[n] ); FREE( pRX->uImag[n] );
        FREE( pRX->vReal[n] ); FREE( pRX->vImag[n] );
        FREE( pRX->wReal[n] ); FREE( pRX->wImag[n] );
        FREE( pRX->xReal[n] ); FREE( pRX->xImag[n] );
        FREE( pRX->yReal[n] ); FREE( pRX->yImag[n] );
        FREE( pRX->zReal[n] ); FREE( pRX->zImag[n] );
        FREE( pRX->traceH2[n] );
        FREE( pRX->traceHReal[n] );
        FREE( pRX->traceHImag[n] );
    }
    #undef FREE
    return WI_SUCCESS;
}
// end of Verona_RX_FreeMemory()

// ================================================================================================
// FUNCTION : Verona_RX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_RX_AllocateMemory(void)
{
    int n;
    Verona_RxState_t *pRX = Verona_RxState();    

    int N80 = pRX->N80 = pRX->rLength;
    int N40 = pRX->N40 = N80/2 + 1;
    int N20 = pRX->N20 = N80/4 + 2 + (VERONA_FFT_DELAY + VERONA_PLL_DELAY);
    int N44 =           (N80*11/10)/2+1;
    int N22 = pRX->N22 = N44/2 + 1;

    Verona_RX_FreeMemory(pRX, 0);

    pRX->PHY_RX_D = (wiUWORD *)wiCalloc(65536 + 8,        sizeof(wiUWORD));
    pRX->cSP      = (wiWORD  *)wiCalloc(VERONA_MAX_N_SYM, sizeof(wiWORD));
    pRX->cCP      = (wiWORD  *)wiCalloc(VERONA_MAX_N_SYM, sizeof(wiWORD));
    pRX->kFFT     = (wiInt   *)wiCalloc(VERONA_MAX_N_SYM, sizeof(wiInt));
    
    if (!pRX->PHY_RX_D || !pRX->cSP || !pRX->cCP || !pRX->kFFT)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    pRX->Na = pRX->aLength + 288;
    pRX->Nb = pRX->Na + 288;
    pRX->Np = 2 * pRX->aLength + 384 + 96*3;  // 96*3 account for L-SIG and HT-SIG for Mixed Mode. The aLength is based on 65535. It's enough to cover all case including 6Mbps
    pRX->Nc = 2 * pRX->aLength + 384 + 48*3;  // 48*3 account for L-SIG and HT-SIG for Mixed Mode. 384 gives enough margin for the highest rate
    pRX->Nd = N20;

    pRX->Nr = N40-1;
    pRX->Nx = N20-1;
    pRX->Ns = N20-1;

    pRX->a     = (wiUWORD   *)wiCalloc(pRX->Na, sizeof(wiUWORD));       
    pRX->b     = (wiUWORD   *)wiCalloc(pRX->Nb, sizeof(wiUWORD));
    pRX->Lp    = (wiUWORD   *)wiCalloc(pRX->Np, sizeof(wiUWORD));
    pRX->Lc    = (wiUWORD   *)wiCalloc(pRX->Nc, sizeof(wiUWORD));
    pRX->cA    = (wiUWORD   *)wiCalloc(pRX->Nb, sizeof(wiUWORD));
    pRX->cB    = (wiUWORD   *)wiCalloc(pRX->Nb, sizeof(wiUWORD));
    pRX->H2    = (wiWORD    *)wiCalloc(pRX->Nd, sizeof(wiWORD ));
    pRX->dReal = (wiWORD    *)wiCalloc(pRX->Nd, sizeof(wiWORD ));
    pRX->dImag = (wiWORD    *)wiCalloc(pRX->Nd, sizeof(wiWORD ));
    pRX->DReal = (wiWORD    *)wiCalloc(pRX->Nd, sizeof(wiWORD ));
    pRX->DImag = (wiWORD    *)wiCalloc(pRX->Nd, sizeof(wiWORD ));
    pRX->DGain    = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
    pRX->AdvDly   = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));

    for (n=0; n<2; n++)
    {
        pRX->pReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->pImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->qReal[n] = (wiUWORD*)wiCalloc(N40, sizeof(wiUWORD));
        pRX->qImag[n] = (wiUWORD*)wiCalloc(N40, sizeof(wiUWORD));
        pRX->rReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->rImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->tReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->tImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->cReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->cImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->sReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->sImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->uReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->uImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->vReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->vImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->wReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->wImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->xReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->xImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->yReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->yImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        pRX->zReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
        pRX->zImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD ));
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Check for memory allocation faults
    //
    if (!pRX->a  || !pRX->b  || !pRX->Lc || !pRX->Lp || !pRX->cA || !pRX->cB ||  !pRX->dReal || !pRX->dImag)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    for (n=0; n<2; n++)
    {
        if (!pRX->rReal[n] || !pRX->rImag[n] || !pRX->zReal[n] || !pRX->zImag[n] 
        ||  !pRX->yReal[n] || !pRX->yImag[n] || !pRX->wReal[n] || !pRX->wImag[n] 
        ||  !pRX->xReal[n] || !pRX->xImag[n] || !pRX->vReal[n] || !pRX->vImag[n] 
        ||  !pRX->uReal[n] || !pRX->uImag[n] || !pRX->qReal[n] || !pRX->qImag[n] 
        ||  !pRX->cReal[n] || !pRX->cImag[n] || !pRX->tReal[n] || !pRX->tImag[n])
            return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Trace Variables - Conditional Allocation
    //
    if (pRX->EnableTrace)
    {
        pRX->AGain         = (wiUWORD *)wiCalloc(N20, sizeof(wiUWORD));
        pRX->DFEState      = (wiUWORD *)wiCalloc(N20, sizeof(wiUWORD));
        pRX->DFEFlags      = (wiUWORD *)wiCalloc(N20, sizeof(wiUWORD));
        pRX->aSigOut       = (Verona_aSigState_t        *)wiCalloc(N20, sizeof(Verona_aSigState_t        ));
        pRX->bSigOut       = (Verona_bSigState_t        *)wiCalloc(N22, sizeof(Verona_bSigState_t        ));
        pRX->traceRxClock  = (Verona_traceRxClockType   *)wiCalloc(N80, sizeof(Verona_traceRxClockType   ));
        pRX->traceRxState  = (Verona_traceRxStateType   *)wiCalloc(N80, sizeof(Verona_traceRxStateType   ));
        pRX->traceRxControl= (Verona_traceRxControlType *)wiCalloc(N80, sizeof(Verona_traceRxControlType ));
        pRX->traceRadioIO  = (Verona_RadioDigitalIO_t   *)wiCalloc(N80, sizeof(Verona_RadioDigitalIO_t   ));
        pRX->traceDataConv = (Verona_DataConvCtrlState_t*)wiCalloc(N80, sizeof(Verona_DataConvCtrlState_t));
        pRX->traceDFE      = (Verona_traceDFEType       *)wiCalloc(N20, sizeof(Verona_traceDFEType       ));
        pRX->traceAGC      = (Verona_traceAGCType       *)wiCalloc(N20, sizeof(Verona_traceAGCType       ));
        pRX->traceRSSI     = (Verona_traceRSSIType      *)wiCalloc(N20, sizeof(Verona_traceRSSIType      ));
        pRX->traceDFS      = (Verona_traceDFSType       *)wiCalloc(N20, sizeof(Verona_traceDFSType       ));
        pRX->traceSigDet   = (Verona_traceSigDetType    *)wiCalloc(N20, sizeof(Verona_traceSigDetType    ));
        pRX->traceFrameSync= (Verona_traceFrameSyncType *)wiCalloc(N20, sizeof(Verona_traceFrameSyncType ));
        pRX->traceRX2      = (Verona_traceRX2Type       *)wiCalloc(N20, sizeof(Verona_traceRX2Type       ));
        pRX->traceRx       = (Verona_traceRxType        *)wiCalloc(N80, sizeof(Verona_traceRxType        ));
        pRX->traceImgDet   = (Verona_traceImgDetType    *)wiCalloc(N20, sizeof(Verona_traceImgDetType    ));
        pRX->traceIQCal    = (Verona_traceIQCalType     *)wiCalloc(N20, sizeof(Verona_traceIQCalType     ));
        
        for (n=0; n<2; n++)
        {
            pRX->traceH2[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
            pRX->traceHReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
            pRX->traceHImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));
        }

        if (!pRX->AGain          || !pRX->DFEState      || !pRX->DFEFlags      || !pRX->aSigOut
        ||  !pRX->bSigOut        || !pRX->traceRxClock  || !pRX->traceRxState  || !pRX->traceRxControl 
        ||  !pRX->traceRadioIO   || !pRX->traceDataConv || !pRX->traceDFE      || !pRX->traceAGC
        ||  !pRX->traceAGC       || !pRX->traceRSSI     || !pRX->traceDFS      || !pRX->traceSigDet
        ||  !pRX->traceFrameSync || !pRX->traceRX2      || !pRX->traceRx
        ||  !pRX->traceH2[0]     || !pRX->traceH2[1]    || !pRX->traceHReal[0] || !pRX->traceHReal[1] 
        ||  !pRX->traceHImag[0]  || !pRX->traceHImag[1] || !pRX->traceImgDet   || !pRX->traceIQCal)
        {
            wiUtil_WriteLogFile("Verona_RX_AllocateMemory: N80 = %u\n",N80);
            return STATUS(WI_ERROR_MEMORY_ALLOCATION);
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Allocate RX memory in the Verona_b module
    //
    XSTATUS( Verona_bRX_AllocateMemory() );

    return WI_SUCCESS;
}
// end of Verona_RX_AllocateMemory()

// ================================================================================================
// FUNCTION : Verona_RX_NuisanceFilter()
// ------------------------------------------------------------------------------------------------
// Purpose  : Look for "nuisance" packets: unicast packets with non-matching STA address
// Notes    : RTL: Need to adjust for 4 octet delimiter offset for HT A-MPDU 
// ================================================================================================
void Verona_RX_NuisanceFilter( wiBoolean CheckNuisance, wiBoolean MAC_NOFILTER, 
                               Verona_CtrlState_t *SigOut, Verona_RxState_t *pRX, VeronaRegisters_t *pReg)
{
    wiBitDelayLine *pDataValid = &(pRX->NuisanceFilterDataValid);
	
    SigOut->NuisancePacket = 0; // default result
     
    if (pReg->AddrFilter.b0 && CheckNuisance && !MAC_NOFILTER)
    {        
        wiBoolean DataReady;
		wiUWORD bRate;
		wiUInt dAgg = (pRX->PHY_RX_D[2].b7 && pRX->PHY_RX_D[7].b7 && pRX->PHY_RX_D[6].b5) ? 4: 0; // 4 octet offset for HT A-MPDU 

		if (!pRX->N_PHY_RX_WR) pDataValid->word = 0;

		ClockDelayLine(*pDataValid, pRX->N_PHY_RX_WR == (18+ dAgg) ? 1:0); // 8 Byte PHY Control Header + First 10 Bytes of MAC Header  (18 + dAgg) + RTLDelay 
        bRate.w3= pRX->PHY_RX_D[2].w7>>4 & 0x7;
		DataReady = (!pRX->PHY_RX_D[2].b7 && pRX->PHY_RX_D[7].b7) && ((bRate.w3 == 0 && pDataValid->delay_0T && !pDataValid->delay_1T) 
			|| ((bRate.w3 == 1 ||bRate.w3 == 5) && pDataValid->delay_8T && !pDataValid->delay_9T)
			|| ((bRate.w3 == 6 ||bRate.w3 == 2) && pDataValid->delay_6T && !pDataValid->delay_7T)
			|| ((bRate.w3 == 7 ||bRate.w3 == 3) && pDataValid->delay_13T && !pDataValid->delay_14T));
			
        if (DataReady)
		{   
            wiUWORD   ModType, Mode;
            wiBoolean ValidRate;
            wiUWORD   Protocol  = {(pRX->PHY_RX_D[8+dAgg].w8 >> 0) & 0x3}; // bits [1:0]
            wiUWORD   FrameType = {(pRX->PHY_RX_D[8+dAgg].w8 >> 2) & 0x3}; // bits [3:2]
            wiUWORD  *Address1  = pRX->PHY_RX_D + 12+dAgg;                 // first byte of Address 1
            wiUWORD   RSSI      = { max(pRX->PHY_RX_D[0].w8, pRX->PHY_RX_D[1].w8) };
            wiBoolean GroupAddr = Address1[0].b0;              // bit 0 -- Broadcast/Multicast
            wiBoolean ValidRSSI = RSSI.w8 >= pReg->minRSSI.w8; // receiver power >= threshold
            wiBoolean ValidType = (FrameType.w2==2) || pReg->FilterAllTypes.b0;

            wiBoolean AddrMatch = (   (Address1[0].w8 == pReg->STA0.w8)
                                   && (Address1[1].w8 == pReg->STA1.w8)
                                   && (Address1[2].w8 == pReg->STA2.w8)
                                   && (Address1[3].w8 == pReg->STA3.w8)
                                   && (Address1[4].w8 == pReg->STA4.w8)
                                   && (Address1[5].w8 == pReg->STA5.w8) ) ? 1:0;

            Mode.word = pRX->PHY_RX_D[7].b7 ? (pRX->PHY_RX_D[2].b7 ? 3: 1): 0;  // 0: Legacy OFDM  1: DSSS/CCK  3: HT OFDM
			
            if (Mode.w2 == 3)
			{
                switch (pRX->PHY_RX_D[2].w7)  // Set ModType to align with legacy OFDM
                {
                    case 0:                 ModType.word = 3; break;
                    case 1: case 2:         ModType.word = 1; break;
                    case 3: case 4:         ModType.word = 2; break;
                    case 5: case 6: case 7: ModType.word = 0; break;
                    default:STATUS(WI_ERROR_UNDEFINED_CASE); return;
                }
			}
            else
                ModType.word = (pRX->PHY_RX_D[2].w8 >> 6) & 0x3 ; // bits [7:6]
           

            ValidRate = ((Mode.w2 == 1) || (pReg->FilterBPSK.b0  && (ModType.w2 == 3))
                                        || (pReg->FilterQPSK.b0  && (ModType.w2 == 1))
                                        || (pReg->Filter16QAM.b0 && (ModType.w2 == 2))
                                        || (pReg->Filter64QAM.b0 && (ModType.w2 == 0)) ) ? 1:0;

            SigOut->NuisancePacket = ( (Protocol.w2==0) && ValidType && !GroupAddr && !AddrMatch && ValidRate && ValidRSSI) ? 1:0;		
        }
    }

	/////////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Non-RTL. To model AddrFilter for OFDM 
	if(pRX->Nuisance.Enabled && pRX->k80 == pRX->Nuisance.kNuisance ) SigOut->NuisancePacket = 1;  
}
// end of Verona_RX_NuisanceFilter

// ================================================================================================
// FUNCTION : Verona_RX_Header()
// ------------------------------------------------------------------------------------------------
// Purpose  : Construct PHY header for RX packets
// ================================================================================================
wiStatus Verona_RX_Header(Verona_CtrlState_t outReg, wiUInt AntSel, wiUWORD RSSI0, wiUWORD RSSI1)
{
    wiUWORD RATE, LENGTH;

    Verona_RxState_t  *pRX  = Verona_RxState();
    Verona_bRxState_t *pRXb = Verona_bRxState();
    VeronaRegisters_t *pReg = Verona_Registers();

    if (!outReg.TurnOnCCK) pReg->CFO = pRX->CFO; // record CFO on a valid OFDM header decode

    // -----------------------------
    // Multiplex RATE, LENGTH values
    // -----------------------------
    RATE.word   = outReg.TurnOnCCK ? pRXb->bRATE.w4       : pRX->RX_RATE.w7;
    LENGTH.word = outReg.TurnOnCCK ? pRXb->LengthPSDU.w12 : pRX->RX_LENGTH.w16; 

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Construct the PHY Header to the MAC
    //
    pRX->PHY_RX_D[0].word = pReg->RSSIdB.b0 ? (RSSI0.w8*3/4) : RSSI0.w8;
    pRX->PHY_RX_D[1].word = pReg->RSSIdB.b0 ? (RSSI1.w8*3/4) : RSSI1.w8;

    if (pRX->RX_nMode) // 802.11n ///////////////////////////////////////////////////////
    {
        pRX->PHY_RX_D[2].b7   = 1;
        pRX->PHY_RX_D[2].w7   = RATE.w7;
        pRX->PHY_RX_D[3].word = LENGTH.w16>>8;
        pRX->PHY_RX_D[4].word = LENGTH.w8;        
        pRX->PHY_RX_D[5].word = pRX->Greenfield<<3;                
        pRX->PHY_RX_D[6].word = pRX->Aggregation<<5; 
        pRX->PHY_RX_D[7].word = 0x80; // set MSB for HT packets         
    }
    else //////////////// 802.11a/b/g ///////////////////////////////////////////////////
    {
        pRX->PHY_RX_D[2].word = (RATE.w4<<4) | (LENGTH.w12>>8);
        pRX->PHY_RX_D[3].word = LENGTH.w8;

        pRX->PHY_RX_D[4].word = pReg->RxFaultCount.w8;
        pRX->PHY_RX_D[5].word = pReg->RestartCount.w8;
        pRX->PHY_RX_D[6].word = outReg.TurnOnCCK ? 0:pReg->CFO.w8;

        pRX->PHY_RX_D[7].b7   = outReg.TurnOnCCK;                // set MSB for DSSS/CCK packets
        pRX->PHY_RX_D[7].b6   = pReg->detRadar.b0;               // DFS radar detect
        pRX->PHY_RX_D[7].b5   = AntSel;                          // antenna selection for switched antenna diversity
        pRX->PHY_RX_D[7].w5   = min(31, pReg->NuisanceCount.w8); // nuisance filter count   
    } 

    // Store Header in Read-Only Registers //////////////////////////////////////////////
    //
    pReg->RxHeader1.word = pRX->PHY_RX_D[0].w8;
    pReg->RxHeader2.word = pRX->PHY_RX_D[1].w8;
    pReg->RxHeader3.word = pRX->PHY_RX_D[2].w8;
    pReg->RxHeader4.word = pRX->PHY_RX_D[3].w8;
    pReg->RxHeader5.word = pRX->PHY_RX_D[4].w8;
    pReg->RxHeader6.word = pRX->PHY_RX_D[5].w8;
    pReg->RxHeader7.word = pRX->PHY_RX_D[6].w8;
    pReg->RxHeader8.word = pRX->PHY_RX_D[7].w8;

    return WI_SUCCESS;
}
// end of Verona_RX_Header()


// ================================================================================================
// FUNCTION : Verona_RX_ERROR()
// ------------------------------------------------------------------------------------------------
// Purpose  : Generate the PHY_RX_ERROR signal (for unsupported/invalid packets)
// ================================================================================================
void Verona_RX_ERROR( Verona_CtrlState_t *inReg, Verona_CtrlState_t outReg, wiUWORD PktTime)
{
    VeronaRegisters_t *pReg = Verona_Registers();
    Verona_RxState_t *pRX = Verona_RxState();

    wiUReg *pRxEndTimer = &(pRX->RX_ERROR.RxEndTimer);

    pRxEndTimer->Q.word = pRxEndTimer->D.w10;
    pRxEndTimer->D.word = (pRxEndTimer->Q.w10 == 0) ? 0 : (pRxEndTimer->Q.w10 - 1);

    switch (outReg.State)
    {
        case 0: 
        case 1:
            pRX->RX_ERROR.ShortTime = 0;
            pRxEndTimer->D.word = 0;
            break;

        case 8:
            pRX->RX_ERROR.ShortTime = 0;
            break;

        case 24:
        case 25:
            pRX->RX_ERROR.ShortTime = (PktTime.w16 == 0) ? 1 : 0;
            pRxEndTimer->D.word = (pReg->ExtendRxError.w8 << 2) | 3;
            break;

        case 26:
            if (outReg.PHY_RX_ERROR)
                pRxEndTimer->D.word = ((pRX->RX_ERROR.ShortTime ? pReg->ExtendRxError1.w8 : pReg->ExtendRxError.w8) << 2) | 3;
            else
                pRxEndTimer->D.word = 0;
            break;

        case 28:
            pRxEndTimer->D.word = 8; // 100 ns pulse
            break;
    }
    inReg->PHY_RX_ERROR = ((outReg.State != 28) && (pRxEndTimer->Q.w10 != 0)) ? 1:0;
}
// end of Verona_RX_ERROR()


// ================================================================================================
// FUNCTION : Verona_RX_CCA()
// ------------------------------------------------------------------------------------------------
// Purpose  : Generate PHY_CCA
// ================================================================================================
void Verona_RX_CCA( Verona_CtrlState_t *inReg, Verona_CtrlState_t outReg, wiUWORD PktTime, 
                    wiUWORD RSSI, wiBoolean PHY_RX_EN, wiBoolean ValidRSSI )
{
    wiBoolean CS, ED;
    wiBoolean PowerCCA, SigDetCCA, SyncCCA, HeaderCCA, DataCCA, GreenfieldCCA;
    wiBoolean NuisanceCCA, SleepCCA, RxEndCCA, PacketCCA, ExtendCCA;

    VeronaRegisters_t *pReg = Verona_Registers();
    Verona_RX_CCA_t *pState = &(Verona_RxState()->CCA);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  ExtendCCA is used to adjust the CCA falling edge on Greenfield packets using only
    //  signal power to be aligned with the falling edge of PHY_RX_EN/PHY_RX_ERROR for 
    //  other formats.
    //
    pState->ExtendCCATimer.Q.word = pState->ExtendCCATimer.D.w10;
    ExtendCCA = (pState->ExtendCCATimer.Q.w10 != 0) ? 1:0;

    switch (outReg.State)
    {
        case 0: // clear timer (just to put into known state)
        case 1:
            pState->ExtendCCATimer.D.word = 0;
            break;

        case 26: // maintain setting (needed when entering from state 29)
            pState->ExtendCCATimer.D.word = pState->ExtendCCATimer.Q.w10; // maintain setting
            break;

        case 29: // load timer for HT-GF with UnsupportedGF = 0
            if (pState->dGreenfieldCCA)
                pState->ExtendCCATimer.D.word = pReg->ExtendBusyGF.w8 << 2;
            else
                pState->ExtendCCATimer.D.word = 0;
            break;

        default: // count down timer
            pState->ExtendCCATimer.D.word = (pState->ExtendCCATimer.Q.w10 == 0) ? 0 : (pState->ExtendCCATimer.Q.w10 - 1);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  GreenfieldCCA is busy when a signal indicated as Greenfield format is present
    //  above a predefined signal threshold.
    //
    GreenfieldCCA  = (outReg.State == 28 && RSSI.w8 > pReg->ThCCA_GF1.w8)
                  || (outReg.State == 29 && RSSI.w8 > pReg->ThCCA_GF2.w8 && pState->dGreenfieldCCA);
    pState->dGreenfieldCCA = GreenfieldCCA;
    
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  SigDetCCA is busy after carrier sense when the received signal exceeds a programmed
    //  threshold. Once the signal exceeds the threshold, CCA is held until frame
    //  synchronization is complete.
    //
    //  
    CS = (outReg.State == 9 || outReg.State == 10); // carrier sense
    ED = RSSI.w8 > pReg->ThCCA_SigDet.w8;           // energy detect (power threshold)

    SigDetCCA = (CS & ED) || (CS & pState->dSigDetCCA); // busy on carrier sense (qualified RSSI)
    pState->dSigDetCCA = SigDetCCA;                     // register delayed SigDetCCA

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  PowerCCA is busy for any RSSI above a programmable threshold. This is intended to
    //  block transmission in the presence of an unidentified strong signal or for the case
    //  where the preamble is missed. It is specifically required by 802.11 section 17.3.10.5.
    //  
    PowerCCA = (((RSSI.w8 > pReg->ThCCA_RSSI1.w8) && ValidRSSI)
             || ((RSSI.w8 > pReg->ThCCA_RSSI2.w8) && pState->dPowerCCA)
             || (!ValidRSSI && pState->dPowerCCA)) ? 1:0;
    pState->dPowerCCA = PowerCCA & !outReg.PHY_Sleep;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  SyncCCA is busy after frame synchronization (SFD found for DSSS preamble and PeakFound
    //  for OFDM. It is a stronger indicator than carrier sense because the threshold for
    //  synchronization is higher.
    //  
    SyncCCA = outReg.State == 11 ? 1:0;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  HeaderCCA is busy following a valid PLCP header until the PktTime is loaded with the
    //  packet duration. For HT-MF packets, HeaderCCA is asserted after receiving a valid
    //  L-SIG and may be de-asserted if the HT-SIG is invalid.
    //
    HeaderCCA = (outReg.State == 12 || outReg.State == 13 || outReg.State == 20 
              || outReg.State == 21 || outReg.State == 22 || outReg.State == 23
              || outReg.State == 24) ? 1:0; 

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  DataCCA is asserted during the PSDU portion of a valid packet
    //
    DataCCA = (outReg.State == 13 || outReg.State == 14 
            || outReg.State == 25 || outReg.State == 26) ? 1:0;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  NuisanceCCA is asserted when the PHY is in standby mode due to a nuisance packet.
    //
    NuisanceCCA = (outReg.State == 6 || outReg.State == 7 || outReg.State == 53 || outReg.State == 54) ? 1:0;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  SleepCCA is asserted whenever the PHY is in sleep/standby and cannot evaluate CCA
    //
    SleepCCA = (outReg.State == 0 || outReg.State ==  1 || outReg.State == 2 
             || outReg.State == 3 || outReg.State == 62 || outReg.State == 63) ? 1:0;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  RxEndCCA is asserted whenever PHY_RX_EN or PHY_RX_ERROR is asserted. It is used to
    //  keep the falling edge of PHY_CCA aligned with these two signals.
    //
    RxEndCCA = outReg.PHY_RX_ERROR || PHY_RX_EN;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  PacketCCA is asserted for the packet duration (indicated by PktTime) following a valid
    //  PLCP header or Data field. It is latched on either signal and held while PktTime is
    //  non-zero so that CCA can remain active on a Step-Up/Down restart event.
    //
    PacketCCA = DataCCA || (HeaderCCA && pReg->SelectCCA.b3) || (pState->dPacketCCA && PktTime.w16 != 0) ? 1:0;
    pState->dPacketCCA = PacketCCA;
    
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  PHY_CCA is a programmable combination of events.
    //
    inReg->GreenfieldCCA = GreenfieldCCA;
    inReg->PHY_CCA =  (pReg->SelectCCA.b0 && PowerCCA     )
                   || (pReg->SelectCCA.b1 && SigDetCCA    )
                   || (pReg->SelectCCA.b2 && SyncCCA      )
                   || (pReg->SelectCCA.b3 && HeaderCCA    )
                   || (pReg->SelectCCA.b4 && DataCCA      )
                   || (pReg->SelectCCA.b5 && GreenfieldCCA)
                   || (pReg->SelectCCA.b6 && RxEndCCA     )
                   || (pReg->SelectCCA.b7 && SleepCCA     ) 
                   || ExtendCCA || NuisanceCCA || PacketCCA ? 1:0;
}
// end of Verona_RX_CCA()


// ================================================================================================
// FUNCTION : Verona_RX()
// ------------------------------------------------------------------------------------------------
// Purpose  : Front-end top for OFDM Baseband (ADC output to FFT input) with cycle
//            accurate timing. For Verona, this module contains controls from the top
//            level as well as all DSSS/CCK RX processing.
// Timing   : This function runs on the 80 MHz clock
// ================================================================================================
wiStatus Verona_RX( wiBoolean DW_PHYEnB, Verona_RadioDigitalIO_t *Radio, 
                    Verona_DataConvCtrlState_t *DataConvIn, Verona_DataConvCtrlState_t DataConvOut,
                    Verona_CtrlState_t *inReg, Verona_CtrlState_t outReg )
{
    const wiBoolean MAC_RSTB = 1; // PHY reset

    wiBoolean dMAC_TX_EN = 0;       // registered version of MAC_TX_EN
    wiBoolean TxDone     = 0;       // TX Control: TX Complete
    wiBoolean AntSel = 0;
    wiBoolean TxEnB = 1;
    wiBoolean ClearDCX;
    wiBoolean SquelchRF;
    
    Verona_RxState_t  *pRX  = Verona_RxState();
    VeronaRegisters_t *pReg = Verona_Registers();
    Verona_RxTop_t    *p    = &(pRX->RxTop);

    Verona_aSigState_t aSigOut, *aSigIn = &(p->aSigIn);
    Verona_bSigState_t bSigOut, *bSigIn = &(p->bSigIn);

    // ------------------------
    // Clock Explicit Registers
    // ------------------------
    if (pRX->clock.posedgeCLK80MHz) 
    {
        ClockRegister(p->PktTime);
        ClockRegister(p->SqlTime);
        ClockRegister(p->phase80);
		ClockDelayLine(p->dPacketAbort, p->PacketAbort);
        p->d2PHY_DFS_INT = p->dPHY_DFS_INT;
        p->dPHY_DFS_INT   = p->PHY_DFS_INT;
    }
    if (pRX->clock.posedgeCLK40MHz)
    {
        ClockRegister(p->bInR);
        ClockRegister(p->bInI);
        p->dSquelchRX = p->SquelchRX;
    }
    if (pRX->clock.posedgeCLK20MHz) p->aSigOut = p->aSigIn; // these registers exist in the OFDM DFE
    if (pRX->clock.posedgeCLK22MHz) p->bSigOut = p->bSigIn; // these registers exist in the CCK FrontCtrl

    aSigOut = p->aSigOut;
    bSigOut = p->bSigOut;

    // ----------------------------
    // RX and Top-Level PHY Control
    // ----------------------------
    if (pRX->clock.posedgeCLK80MHz)
    {
        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Combination Logic
        //
        wiBoolean SigDet       = (outReg.TurnOnOFDM  && aSigOut.SigDet)  || (outReg.TurnOnCCK   && bSigOut.SigDet  ) ? 1:0;
        wiBoolean AGCdone      = (pReg->RXMode.b0 && aSigOut.AGCdone)    || (pReg->RXMode.b1 && bSigOut.AGCdone    ) ? 1:0;
        wiBoolean SyncFound    = (pReg->RXMode.b0 && aSigOut.SyncFound)  || (pReg->RXMode.b1 && bSigOut.SyncFound  ) ? 1:0;
        wiBoolean HeaderValid  = (pReg->RXMode.b0 && aSigOut.HeaderValid)|| (pReg->RXMode.b1 && bSigOut.HeaderValid) ? 1:0;
        wiBoolean RxDone       = (pReg->RXMode.b0 && aSigOut.RxDone)     || (pReg->RXMode.b1 && bSigOut.RxDone     ) ? 1:0;
        wiBoolean NewSig       = pReg->StepUpRestart.b0   && ((pReg->RXMode.b0 && aSigOut.StepUp)   || (pReg->RXMode.b1 && bSigOut.StepUp)) && (!p->RxAggregate || !pReg->NoStepUpAgg.b0);
        wiBoolean LostSig      = pReg->StepDownRestart.b0 && ((pReg->RXMode.b0 && aSigOut.StepDown) || (pReg->RXMode.b1 && bSigOut.StepDown));
        wiBoolean detRestart   = NewSig || LostSig || pReg->ForceRestart.b0; // detect restart condition
        wiBoolean Unsupported  = (outReg.TurnOnOFDM  && pRX->Unsupported) ? 1:0;      // Unsupported valid header
        
        if (pReg->sPHYEnB.b0) DW_PHYEnB = pReg->PHYEnB.b0; // DW_PHYEnB register override

        AntSel = pReg->RXMode.b0 ? aSigOut.AntSel : bSigOut.AntSel;

        if (aSigOut.RxDone) aSigIn->RxDone = 0; // OFDM RxDone is an 80 MHz pulse (implemented in RX2)

        ///////////////////////////////////////////////////////////////////////////////////////////
        //
        //  Default Counter/Timer Inputs
        //
        //  In addition to the explicit state variable, two counters are used to sequence events.
        //  PktTime is a microsecond count-down timer used to sequence startup events and to time 
        //  packet duration during receive. Counter phase80 is a modulo-80 counter used to specify
        //  ticks for PktTime as well as finer timing within the controller. Both counters may be
        //  reset or loaded in different control states below. Implemented here is the default
        //  operation if nothing in the current state is specified.
        //
        //  An additional timer, SqlTime, provides 12.5 ns timing for RX squelch operations that
        //  occur on gain changes.
        //
        p->phase80.D.word = (p->phase80.Q.w7 + 1) % 80;   // update modulo-80 count

        if (p->PktTime.Q.w16 && (p->phase80.Q.w7==79)) 
                p->PktTime.D.word = p->PktTime.Q.w16 - 1; // decrement 1us timer

        if (p->SqlTime.Q.w6) 
                p->SqlTime.D.word = p->SqlTime.Q.w6 - 1;  // decrement 80 MHz squelch counter

        ///////////////////////////////////////////////////////////////////////////////////////////
        //
        //  TX Timing Model
        //
        //  Transmit in this model is handled outside this top-level control. To simulated control
        //  behavior for transmit in the top level, timing for the handshake signals MAC_TX_EN and
        //  TxDone is specified below.
        //
        if (pRX->TX.Enabled)
        {
            dMAC_TX_EN = p->MAC_TX_EN; // register delay

            p->MAC_TX_EN  = ((pRX->k80 >= pRX->TX.k1[0] && pRX->k80 < pRX->TX.k2[0])
                          || (pRX->k80 >= pRX->TX.k1[1] && pRX->k80 < pRX->TX.k2[1])
                          || (pRX->k80 >= pRX->TX.k1[2] && pRX->k80 < pRX->TX.k2[2])
                          || (pRX->k80 >= pRX->TX.k1[3] && pRX->k80 < pRX->TX.k2[3])) ? 1:0;

            TxDone        = ((pRX->k80 == pRX->TX.kTxDone[0]) || (pRX->k80 == pRX->TX.kTxDone[1]) 
                          || (pRX->k80 == pRX->TX.kTxDone[2]) || (pRX->k80 == pRX->TX.kTxDone[3])) ? 1:0;

            if (p->MAC_TX_EN && !dMAC_TX_EN)
                Verona_DvState()->LastTxStart = pRX->k80; // record the last assertion of MAC_TX_EN
        }

        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Top-Level Baseband Controller State Machine
        //
        switch (outReg.State)
        {
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Sleep and Sleep-to-RX Startup
            //  =============================
            //
            case 0: // PHY Sleep ------------------------------------------------------------------

                p->BGEnB           = 1; // Data converter bandgap disabled
                p->ADCEnB          = 1; // ADCs disabled
                p->DACEnB          = 1; // DACs disabled
                p->PAEnB           = 1; // Power amplifier disabled
                p->AntTxEnB        = 0; // RF switch in TX position
                p->DFEEnB          = 1; // baseband front-end disabled
                p->ModemSleep      = 1; // baseband DSP disabled
                p->SquelchRX       = 1; // clear RX input at DCX
                p->PacketAbort     = 0; // clear packet abort
                p->DFERestart      = 0; // clear DFE restart signal
                p->RadioSleep      = 1; // radio sleep 
                p->RadioStandby    = 1; // lesser included condition of sleep
                p->RadioTX         = 0; // default active state is RX
                p->RestartRX       = 0; // clear packet RX restart
                p->CheckRIFS       = 0; // clear carrier sense at RIFS
                p->HoldGain        = 0; // clear HoldGain for nuisance RIFS
                p->RxAggregate     = 0; // clear the RX A-MPDU flag
				p->phase80.D.word  = 0; // clear phase counter
                p->PktTime.D.word  = 0; // clear packet timer
                p->SqlTime.D.word  = 0; // clear squelch counter
                inReg->PHY_Sleep= 1; // signal PHY in sleep mode
                inReg->UpdateAGC= 0; // clear HT-MF gain update

                inReg->State    = 1; 
                break;

            case 1: // PHY Sleep: Wait for Enable -------------------------------------------------

                p->phase80.D.word = 0;               // hold phase counter inactive
                if (!DW_PHYEnB) inReg->State = 2; // startup on DW_PHYEnB --> 0
                break;

            case 2: // PHY Startup: Initiate Wakeup -----------------------------------------------

                p->PktTime.D.word   = (pReg->WakeupTimeH.w8 << 8) | (pReg->WakeupTimeL.w8);
                p->RadioSleep       = 0; // bring radio out of sleep mode
                inReg->PHY_Sleep = 0; // indicate PHY is no longer in sleep mode
                inReg->State   = 3;
                break;

            case 3: // PHY Startup: Sleep-to-RX Sequencing ----------------------------------------

                p->RadioStandby = (p->PktTime.Q.w16 > pReg->DelayStdby.w8) ? 1:0; // Radio Standby->RX
                p->AntTxEnB     = (p->PktTime.Q.w16 > pReg->DelayStdby.w8) ? 0:1; // RF switch to RX (1)  
                p->BGEnB        = (p->PktTime.Q.w16 > pReg->DelayBGEnB.w8) ? 1:0; // Data converter bandgap enable
                p->ADCEnB       = (p->PktTime.Q.w16 > pReg->DelayADC1.w8 ) ? 1:0; // RX ADC Startup
                p->ModemSleep   = (p->PktTime.Q.w16 > pReg->DelayModem.w8) ? 1:0; // Baseband Startup (backoff for FESP)
                p->SquelchRX    = (p->PktTime.Q.w16 >=pReg->DelayModem.w8) ? 1:0; // Baseband Initialization (clear inputs)
                p->DFEEnB       = (p->PktTime.Q.w16 > pReg->DelayDFE.w8  ) ? 1:0; // Start post-FESP processing
                inReg->State = (p->PktTime.Q.w16 > 0                  ) ? 3:8; // advance when timer reaches 0
                
                if (DW_PHYEnB) inReg->State = 62; // shutdown
                break;

            case 4: inReg->State = 0; break; // unused state

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Standby/Sleep During a Nuisance Packet
            //  ======================================
            //
            case 5: // Reduce power during nuisance packet ----------------------------------------
            
                p->PacketAbort    = 1; // set packet abort flag
                inReg->State   = 6;
                break;

            case 6: // Reduce power during nuisance packet ----------------------------------------
			{
				wiBoolean RIFSEn = outReg.TurnOnOFDM && pReg->RxRIFS.b0 && pRX->RX_nMode; 
                p->PktTime.D.word = p->PktTime.Q.w16 + (RIFSEn? 0 : pReg->TimeExtend.w8); // increase time for IFS+ACK          
                pReg->NuisanceCount.word = min(255, pReg->NuisanceCount.w8 + 1);// increment nuisance counter
				p->PacketAbort  =  0; // clear packet abort flag
                
				if(RIFSEn)	inReg->State   = 53; // Nuisance packet with RIFS support
				else		inReg->State   =  7; // select power-down/up sequence
                break;
			}

            case 7: // PHY Startup: Nuisance Standby-to-RX Sequencing -----------------------------
                                                      
                p->RadioStandby = (p->PktTime.Q.w16 > pReg->DelayStdby.w8) ? 1:0; // Radio Standby->RX
                p->ADCEnB       = (p->PktTime.Q.w16 > pReg->DelayADC2.w8 ) ? 1:0; // RX ADC Startup
                p->ModemSleep   = (p->PktTime.Q.w16 > pReg->DelayModem.w8) ? 1:0; // Baseband Startup (backoff for FESP)
                p->SquelchRX    = (p->PktTime.Q.w16 >=pReg->DelayModem.w8) ? 1:0; // Baseband Initialization (clear inputs)
                p->DFEEnB       = (p->PktTime.Q.w16 > pReg->DelayDFE.w8  ) ? 1:0; // Start post-FESP processing

                     if (DW_PHYEnB     ) inReg->State = 62; // shutdown
                else if (!p->PktTime.Q.w16) inReg->State =  8; // advance to RX idle when timer reaches 0
                break;

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Receive: Idle and Active Demodulation/Decoding
            //  ==============================================
            //
            case 8: // RX: Idle - Wait for Signal Detect/Carrier Sense ----------------------------
            
                p->DFERestart = 1; // maintain DFE restart signal for RX-to-RX
                p->RestartRX  = 0; // clear RX packet restart
                p->CheckRIFS  = 0; // clear any residual carrier sense at RIFS
				p->HoldGain   = 0; // clear HoldGain signal after nuisance RIFS detection
                p->SquelchRX  = 0; // clear any residual squelch
                p->RxAggregate= 0; // clear any aggregation flag

                     if (DW_PHYEnB ) inReg->State = 62; // shutdown
                else if (dMAC_TX_EN) inReg->State = 40; // start transmit
                else if (SigDet    ) inReg->State =  9; // signal detect
                break;

            case 9: // RX: Automatic Gain Control (AGC) -------------------------------------------

                p->DFERestart = 0; // clear DFE restart

                     if (DW_PHYEnB ) inReg->State = 62; // shutdown
                else if (dMAC_TX_EN) inReg->State = 40; // start transmit
                else if (AGCdone   ) inReg->State = 10; // AGC complete (RSSI valid)
                break;

            case 10: // RX: Preamble Processing ---------------------------------------------------

                aSigIn->RxFault     = 0; // clear the RX2_SIGNAL fault flag (RX2)
                aSigIn->HeaderValid = 0; // clear the valid header flag (RX2)
                aSigIn->ChEstDone   = 0; // clear the HT ChEst flag (RX2)

                     if (DW_PHYEnB  ) inReg->State = 62; // shutdown
                else if (dMAC_TX_EN ) inReg->State = 40; // start transmit
                else if (SyncFound  ) inReg->State = 11; // sync found
                else if (!SigDet    ) inReg->State =  8; // false detect: sync timeout
                else if (detRestart ) inReg->State = 33; // new signal detect
                break;

            case 11: // RX: Frame Synchronized ----------------------------------------------------

                     if (DW_PHYEnB  ) inReg->State = 62; // shutdown
                else if (dMAC_TX_EN ) inReg->State = 40; // start transmit
                else if (HeaderValid) inReg->State = 12; // valid header
                else if (!SyncFound || aSigOut.RxFault) inReg->State = 31; // bad packet: bad CRC/SIGNAL
                else if (detRestart && !pReg->NoStepAfterSync.b0) inReg->State = 32; // new signal detect
                break;

            case 12: // RX: Valid Header ----------------------------------------------------------
            {
                wiBoolean RateIs6Mbps = (outReg.TurnOnOFDM && pReg->RXMode.b2 && pRX->RX_RATE.w7 == 0xD);

                // Load packet duration from the PLCP header
                //
                p->PktTime.D.word = outReg.TurnOnCCK ? Verona_bRxState()->bPacketDuration.w16 : pRX->aPacketDuration.w16;
                p->phase80.D.word = pReg->PktTimePhase80.w7;

                aSigIn->HeaderValid = 0;

                     if (Unsupported) inReg->State = 28; // HT-GF
                else if (RateIs6Mbps) inReg->State = 20; // L-OFDM 6 Mbps or HT-MF
                else    {             inReg->State = 13; // L-OFDM or DSSS/CCK
                    Verona_RX_Header(outReg, AntSel, p->RSSI0, p->RSSI1); 
                };
                break;
            }
            case 13: // RX: Receive PSDU ----------------------------------------------------------

                     if (DW_PHYEnB)                              inReg->State = 61; // shutdown
                else if (dMAC_TX_EN)                             inReg->State = 39; // abort RX to start transmit
                else if (outReg.NuisancePacket)                  inReg->State =  5; // nuisance packet...go to standby
                else if (p->PktTime.Q.w16 <=4 )                     inReg->State = 14; // near end of packet
                else if (detRestart && !pReg->NoStepAfterHdr.b0) inReg->State = 32; // new signal detect
                else if (RxDone   )                              inReg->State = 15; // unexpected finish - restart 
                break;
                
            case 14: // RX: Near End of PSDU, Lock Out Restart Events -----------------------------

                p->CheckRIFS = (outReg.TurnOnOFDM && pReg->RxRIFS.b0 && pRX->RX_nMode) ? 1:0;
                pRX->PacketType = outReg.TurnOnCCK ? 2:1; // record packet type                            

                     if (DW_PHYEnB )         inReg->State = 61; // shutdown
                else if (dMAC_TX_EN )        inReg->State = 39; // abort RX to start transmit                   
                else if (p->PktTime.Q.w16 == 0) inReg->State = 15; // end of packet at antenna                 
                else if (RxDone   )          inReg->State = 15; // unexpected finish - restart 
                break;
               
            case 15: // RX: Finish demod and transfer to MAC --------------------------------------

                if (!outReg.CheckRIFS || aSigOut.TimeoutRIFS) p->DFERestart = 1; // restart if not RIFS [!Mealy]
                p->PktTime.D.word = 0; // clear packet timer (for unexpected transition from 14->15 on RxDone)

                     if (DW_PHYEnB)  inReg->State = 61; // shutdown
                else if (dMAC_TX_EN) inReg->State = 39; // abort RX to start transmit
                else if (RxDone   )  inReg->State =  8; // end of packet. For HT OFDM, normal RIFS window ends before RxDone
			    break;

            case 16: inReg->State = 0; break; // unused state
            case 17: inReg->State = 0; break; // unused state
            case 18: inReg->State = 0; break; // unused state
            case 19: inReg->State = 0; break; // unused state
            
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Active Receive for High Throughput OFDM
            //  =======================================
            //
            case 20: // RX: 6 Mbps, Check for Rotation --------------------------------------------
            {
                wiBoolean PacketIsHT_MF = pRX->RxRot6MbpsValid &&  pRX->RxRot6Mbps;
                wiBoolean PacketIs6Mbps = pRX->RxRot6MbpsValid && !pRX->RxRot6Mbps;

                     if (DW_PHYEnB)                               inReg->State = 62; // shutdown    
                else if (dMAC_TX_EN)                              inReg->State = 40; // start transmit
                else if (PacketIsHT_MF) {                         inReg->State = 21; // HT-OFDM (MF)
                    p->PktTime.D.word = 4;
                    p->phase80.D.word = 40;
                }
                else if (PacketIs6Mbps) {                         inReg->State = 13; // L-OFDM, 6 Mbps
                    Verona_RX_Header(outReg, AntSel, p->RSSI0, p->RSSI1);
                }
                else if (detRestart && !pReg->NoStepAfterSync.b0) inReg->State = 32; // Abort/Restart
                break;
            }
            case 21: // RX: HT-STF Processing

                inReg->UpdateAGC = (p->PktTime.Q.w16 == 3) ? 1:0;  // request AGC gain update

                     if (DW_PHYEnB  )        inReg->State = 62; // shutdown    
                else if (dMAC_TX_EN )        inReg->State = 40; // start transmit
                else if (p->PktTime.Q.w16 == 0) inReg->State = 22; // HT-STF complete
                break;

            case 22: // RX: Mixed-Mode HT-SIG Decoding --------------------------------------------

                     if (DW_PHYEnB)                               inReg->State = 62; // shutdown
                else if (dMAC_TX_EN)                              inReg->State = 40; // start transmit
                else if (HeaderValid && !Unsupported)             inReg->State = 23; // HT-MF supported format
                else if (HeaderValid &&  Unsupported)             inReg->State = 24; // unsupported, but valid
                else if (aSigOut.RxFault)                         inReg->State = 31; // bad packet: bad CRC/SIGNAL
                else if (detRestart && !pReg->NoStepAfterSync.b0) inReg->State = 32; // new signal detect

                if (HeaderValid)
                {
                    p->PktTime.D.word = pRX->aPacketDuration.w16;      // load packet duration from PLCP header,  
                    p->phase80.D.word = pReg->PktTimePhase80.w7;

                    if (!Unsupported) Verona_RX_Header(outReg, AntSel, p->RSSI0, p->RSSI1);
                }
                break;            
                              
            case 23: // RX: Mixed Mode HT Channel Estimation --------------------------------------

                p->RxAggregate = pRX->Aggregation; // set aggregation flag based on PLCP header

                     if (DW_PHYEnB  )                             inReg->State = 62; // shutdown
                else if (dMAC_TX_EN )                             inReg->State = 40; // start transmit
                else if (aSigOut.ChEstDone)                       inReg->State = 13; // HT MM PSDU Processing               
                else if (detRestart && !pReg->NoStepAfterSync.b0) inReg->State = 32; // new signal detect
                break;
            
            /////////////////////////////////////////////////////////////////////////////
            //                
            //  Unsupported/Greenfield OFDM Packets
            //  ===================================
            //                
            case 24: // RX: Unsupported Format ----------------------------------------------------
			
                p->CheckRIFS = (pReg->RxRIFS.b0 && p->PktTime.Q.w16 == 0) ? 1:0; // for HT-GF, single symbol

					 if (DW_PHYEnB)                inReg->State = 62; // shutdown
				else if (pReg->FilterUndecoded.b0) inReg->State =  6; // Treat as nuisance packet
				else			                   inReg->State = 25; // wait for end of packet
                break;	

            case 25: // RX: Unsupported Format: Wait for the End of Packet ------------------------

                     if (DW_PHYEnB)                              inReg->State = 62; // shutdown
                else if (p->PktTime.Q.w16 <= 4)                     inReg->State = 26; // near end of packet
                else if (detRestart && !pReg->NoStepAfterHdr.b0) inReg->State = 32; // restart
                break;
                
            case 26: // RX: End of Unsupported Packet ---------------------------------------------

				p->CheckRIFS = (pReg->RxRIFS.b0 && pRX->RX_nMode) ? 1:0;

                     if (DW_PHYEnB)                     inReg->State = 62; // shutdown
                else if (p->PktTime.Q.w16==0 && p->CheckRIFS) inReg->State = 27; // end of packet at antenna with RIFS
				else if (p->PktTime.Q.w16==0)              inReg->State =  8; // end of packet at antenna
                break;
			

            case 27: // Wait for carrier sense in RIFS or timeout ---------------------------------
                     if (DW_PHYEnB)                              inReg->State = 62; // shutdown
                else if (aSigOut.RIFSDet || aSigOut.TimeoutRIFS) inReg->State =  8; // restart if not RIFS; 
                break;

            /////////////////////////////////////////////////////////////////////////////

            case 28: // HT-Greenfield

                     if (DW_PHYEnB)  inReg->State = 62; // shutdown
                else                 inReg->State = pReg->UnsupportedGF.b0 ? 24 : 29; 
                break;

            case 29: // Wait for end of packet denoted by RSSI < -72 dBm --------------------------

                     if (DW_PHYEnB)             inReg->State = 62; // shutdown
                else if (!outReg.GreenfieldCCA) inReg->State =  8; // end of packet at antenna
                else if (p->PktTime.Q.w16 <= 4)    inReg->State = 26; // predicted end of packet
                else if (detRestart && !pReg->NoStepAfterHdr.b0) inReg->State = 32; // new signal detect

                if (!outReg.GreenfieldCCA) p->PktTime.D.word = 0; // clear counter on !GreenfieldCCA
                break;

            case 30: inReg->State = 0; break; // unused state

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Receiver Restart on a Fault or Step-Up/Down
            //  ===========================================
            //
            case 31: // RX: PLCP Header Error (Parity/CRC Error = RxFault)

                p->PktTime.D.word = 0; // reset packet timer
                pReg->RxFault.word  = 1; // set the RxFault status bit
                pReg->RxFaultCount.word = min(255, pReg->RxFaultCount.w8 + 1);   // update fault counter

                inReg->State = 33;
                break;

            case 32: // RX: Abort packet before restart (enter here after valid header) -----------

                if ((NewSig  && !pReg->KeepCCA_New.b0 ) 
                ||  (LostSig && !pReg->KeepCCA_Lost.b0)) p->PktTime.D.word = 0; // reset packet timer (to clear old CCA)

                pReg->RestartCount.word = min(255, pReg->RestartCount.w8 + 1);

                p->PacketAbort  =  1;
                inReg->State = 33;
                break;
            
            case 33: // RX: Signal abort/restart on new signal detect or lost signal --------------

                p->PacketAbort = 0;                   // clear the abort strobe (from state 32)
                p->RestartRX   = 1;                   // flag restart (on new signal detect or lost signal)
                
                if (dMAC_TX_EN) inReg->State = 40; // start TX
                else            inReg->State = 34; // continue
                break;

            case 34: // RX: Wait for end of abort/restart -----------------------------------------

                p->RestartRX = 1;                          // hold restart signal

                     if (DW_PHYEnB )            inReg->State = 62; // shutdown PHY
                else if (dMAC_TX_EN)            inReg->State = 40; // start transmit
                else if (!SigDet && !SyncFound) inReg->State =  8; // wait for old signal detect to clear
                break;

            case 35: inReg->State = 0; break; // unused state 
            case 36: inReg->State = 0; break; // unused state 
            case 37: inReg->State = 0; break; // unused state 
            case 38: inReg->State = 0; break; // unused state

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Active Transmit (describes top-level control but no functional TX)
            //  ==================================================================
            //
            case 39: // Abort RX to Start TX ------------------------------------------------------

                pReg->RxAbortTx.word =  1; // set the RxAbort on Tx status bit
                p->DFERestart           =  0; // clear restart from state 34
                p->PacketAbort          =  1; // signal an RX packet abort
                inReg->State         = 40;
                break;

            case 40: // RX-to-TX transition (Entry for normal TX) ---------------------------------

                p->DFERestart       = 0; // clear any DFE restart signal
                p->CheckRIFS        = 0; // clear any RIFS enable signal
				p->HoldGain         = 0; // Reset HoldGain (not straightly nessary)
                p->RestartRX        = 0; // clear any restart signal (states 33, 34)
                p->DFEEnB           = 1; // disable RX
                p->ADCEnB           = 1; // disable ADCs
                p->DACEnB           = 0; // enabled DACs
                p->AntTxEnB         = 0; // RF switch to TX position
                p->RadioTX          = 1; // set radio to transmit
                p->PacketAbort      = 0; // clear the abort flag (from state 39)
                inReg->UpdateAGC = 0; // clear any residual HT-MF gain update

                p->phase80.D.word = pReg->DelayPA.b0 ? 40:0;                 // Timer = 0.5us * DelayPA
                p->PktTime.D.word =(pReg->DelayPA.w4>>1) + pReg->DelayPA.b0; //   "       "       "

                inReg->State   = 41;
                break;

            case 41: // start TX but delay PA enable ----------------------------------------------

                if (pRX->k80 == pRX->TX.kTxFault) pReg->TxFault.word = 1; // TxFault timing

                     if ( DW_PHYEnB    ) inReg->State = 50; // abort TX and shutdown
                else if (!dMAC_TX_EN   ) inReg->State = 45; // requested TX abort
                else if (TxDone        ) inReg->State = 43; // TxFault
                else if (!p->PktTime.Q.w16) inReg->State = 42; // enable PA and continue with TX
                break;

            case 42: // active TX -----------------------------------------------------------------

                p->PAEnB          = 0;                      // enable power amp
                p->phase80.D.word = 0;                      // clear the phase counter
                p->PktTime.D.word = pReg->TxRxTime.w4;      // load delay for TX-to-RX phase (state 44)

                if (pRX->k80 == pRX->TX.kTxFault) pReg->TxFault.word = 1; // TxFault timing

                     if (DW_PHYEnB  ) inReg->State = 50; // abort TX and shutdown
                else if (TxDone     ) inReg->State = 43; // TX returns control to RX (normal or TxFault)
                else if (!dMAC_TX_EN) inReg->State = 46; // requested TX abort
                break;

            case 43: // wait for MAC to acknowledge end-of-transmission ---------------------------

                p->RadioTX = 0; // change radio mode to RX
                p->PAEnB   = 1; // Power amplifier disabled

                p->phase80.D.word = 0;                      // clear the phase counter
                p->PktTime.D.word = pReg->TxRxTime.w4;      // load delay for TX-to-RX phase (state 44)

                     if (!dMAC_TX_EN) inReg->State = 44; // normal end of TX
                else if (DW_PHYEnB )  inReg->State = 50; // shutdown PHY
                break;

            case 44: // TX-to-RX transition -------------------------------------------------------

                p->RadioTX  = 0; // change radio mode to RX
                p->DACEnB   = 1; // disable DACs
                p->PAEnB    = 1; // Power amplifier disabled

                p->AntTxEnB = (p->PktTime.Q.w4 > pReg->DelayRFSW.w4 ) ? 0:1; // delay antenna switch to RX
                p->ADCEnB   = (p->PktTime.Q.w4 > pReg->DelayADC2.w8 ) ? 1:0; // RX ADC Startup
                p->DFEEnB   = (p->PktTime.Q.w4 > pReg->DelayDFE.w8  ) ? 1:0; // Start post-FESP processing
                
                     if (DW_PHYEnB    ) inReg->State = 50; // shutdown
                else if (dMAC_TX_EN   ) inReg->State = 40; // start transmit
                else if (!p->PktTime.Q.w4) inReg->State =  8; // advance to RX idle when timer reaches 0
                break;

            case 45: // request TX abort - (same as state 46, but entry from state 41) ------------

                p->PAEnB = 1;                             // disable or keep disabled the PA
                p->phase80.D.word = 0;                    // reset phase counter 
                p->PktTime.D.word = pReg->TxRxTime.w4;    // load delay for TX-to-RX phase (state 44)
                pReg->TxAbort.word = 1;                // indicate TX abort

                     if (TxDone   ) inReg->State = 43; // TX returns control to RX
                else if (DW_PHYEnB) inReg->State = 50; // shutdown PHY
                break;
            
            case 46: // request TX abort - wait for return of control from TX state machine -------

                p->phase80.D.word = 0;                    // reset phase counter 
                p->PktTime.D.word = pReg->TxRxTime.w4;    // load delay for TX-to-RX phase (state 7)
                pReg->TxAbort.word = 1;                // indicate TX abort

                     if (TxDone   ) inReg->State = 43; // TX returns control to RX
                else if (DW_PHYEnB) inReg->State = 50; // shutdown PHY
                break;

            case 47: inReg->State = 0; break; // unused state
            case 48: inReg->State = 0; break; // unused state
            case 49: inReg->State = 0; break; // unused state

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Shutdown from TX/RX to Sleep
            //  ============================
            //
           case 50: // Shutdown from TX or TX-RX Transition ---------------------------------------

                p->phase80.D.word = 0; // clear phase counter
                p->PktTime.D.word = 0; // clear timer
                p->RadioTX    = 0;     // put radio into RX mode
                p->PAEnB      = 1;     // Power amplifier disabled
                p->AntTxEnB   = 0;     // RF switch to TX position (hold position for shutdown from TX)
                p->BGEnB      = 1;     // disable data converter bandgap (full shutdown)
                p->DACEnB     = 1;     // disable DACs
                p->ADCEnB     = 1;     // disable ADCs
                p->DFEEnB     = 1;     // shutdown baseband DFE/FrontCtrl
                p->ModemSleep = 1;     // shutdown all baesband FESP
                p->SquelchRX  = 1;     // clear all RX inputs (at DCX)

                pReg->TxSleep.b0 = 1;
                inReg->State = 51; 
                break;

            case 51: // pass-thru radio RX during shutdown ----------------------------------------

                if (p->phase80.Q.w8 == 4*pReg->DelayState51.w4) inReg->State = 62;
                break;

            case 52: inReg->State = 0; break; // unused state

            /////////////////////////////////////////////////////////////////////////////
            //
            //  RIFS Enabled Nuisance Packet Standby
            //  ====================================
            //
            case 53: // PHY Startup: Standby-to-RX Sequencing for RIFS after Nuisance -------------
								
				p->HoldGain     = 1; // HoldGain if DFE is reset
				p->RadioStandby = (p->PktTime.Q.w16 > (pReg->DelayStdby.w8 + 4)) ? 1:0; // Radio Standby->RX
                p->ADCEnB       = (p->PktTime.Q.w16 > (pReg->DelayADC2.w8  + 4)) ? 1:0; // RX ADC Startup
                p->ModemSleep   = (p->PktTime.Q.w16 > (pReg->DelayModem.w8 + 4)) ? 1:0; // Baseband Startup (backoff for FESP)
                p->SquelchRX    = (p->PktTime.Q.w16 >=(pReg->DelayModem.w8 + 4)) ? 1:0; // Baseband Initialization (clear inputs)
                p->DFEEnB       = (p->PktTime.Q.w16 > (pReg->DelayDFE.w8   + 4)) ? 1:0; // Start post-FESP processing
						
				     if (DW_PHYEnB            ) inReg->State = 62; // shutdown
                else if (p->PktTime.Q.w16 <= 4) inReg->State = 54;				
				break;					
			
			case 54: // Forced wake up if not done in state 53 ------------------------------------ 
				
				p->CheckRIFS    = 1; //Enable SD if DFE is not reset for short packet
								
					 if (DW_PHYEnB     ) inReg->State = 62; // shutdown
				else if (!p->PktTime.Q.w16) inReg->State = 55;
				break;

            case 55: // Wait for carrier sense in RIFS or timeout ---------------------------------
                 
                     if (DW_PHYEnB)								 inReg->State = 62; // shutdown
                else if (aSigOut.RIFSDet || aSigOut.TimeoutRIFS) inReg->State =  8; // restart if not RIFS; 
			    break;

            case 56: inReg->State = 0; break; // unused state
            case 57: inReg->State = 0; break; // unused state
            case 58: inReg->State = 0; break; // unused state
            case 59: inReg->State = 0; break; // unused state
            case 60: inReg->State = 0; break; // unused state

            case 61: // Abort RX and Shutdown -----------------------------------------------------

                p->DFERestart   =  0; // clear restart from state 34
                p->PacketAbort  =  1;
                inReg->State = 62;
                break;
                
            case 62: // RX-to-Standby Transition --------------------------------------------------

                p->phase80.D.word   = 0; // reset phase counter
                p->PktTime.D.word   = 0; // clear timer
                p->PacketAbort      = 0; // clear packet abort from state 61
                p->RestartRX        = 0; // clear any restart signal (states 33, 34)
                p->DFERestart       = 0; // clear any DFE restart signal
                p->AntTxEnB         = 0; // RF switch to TX position (hold position for shutdown from TX)
                p->BGEnB            = 1; // disable data converter bandgap (full shutdown)
                p->DACEnB           = 1; // disable DACs
                p->ADCEnB           = 1; // disable ADCs
                p->DFEEnB           = 1; // shutdown baseband DFE/FrontCtrl
                p->HoldGain         = 0; // reset HoldGain 
				p->ModemSleep       = 0; // enable clocks (to allow AGAIN to be re-initialized)
                p->SquelchRX        = 1; // clear all RX inputs (at DCX)
                p->RadioStandby     = 1; // put radio in standby mode
                inReg->UpdateAGC = 0; // clear any residual HT-MF gain update

                inReg->State  = 63;
                break;

            case 63: // Shutdown: Wait for RX-to-Standby Transition -------------------------------

                p->RadioSleep   = (p->phase80.Q.w8 < 4*pReg->DelaySleep.w4   ) ?  0:1; // Standby-to-Sleep
                inReg->State = (p->phase80.Q.w8 < 4*pReg->DelayShutdown.w4) ? 63:0; // Sleep-to-Shutdown
                break;
    
            default: inReg->State = 0; // for unused states and on RESET --------------------------
        }       
    }
    
    // ------------------------------------
    // Clear RX Fault Counters If Requested
    // ------------------------------------
    if (pReg->ClrNow.b0 || (pReg->ClrOnHdr.b0 && outReg.State==15) || (pReg->ClrOnWake.b0 && outReg.State==3))
    {
        pReg->RxFaultCount.word  = 0;
        pReg->RestartCount.word  = 0;
        pReg->NuisanceCount.word = 0;
    }
    // -----------------
    // RX Initialization
    // -----------------
    if (p->DFEEnB || p->RestartRX || p->DFERestart)
    {
        inReg->TurnOnOFDM = pReg->RXMode.b0;
        inReg->TurnOnCCK  = pReg->RXMode.b1;
    }

	if (p->HoldGain) inReg-> TurnOnCCK = 0;
    if (outReg.State == 10) pRX->RX_nMode = 0; // reset (needed for HT-OFDM to CCK);

    if (!pRX->k80) 
    {
        pRX->dx = pRX->dv = pRX->du = pRX->dc = 0;
        pRX->PacketType = 0; // clear packet type
    }
    // -------------------------------------------
    // Register Control Signals as Required by RTL
    // -------------------------------------------
    inReg->RestartRX  = p->RestartRX;
    inReg->DFERestart = p->DFERestart;
	inReg->CheckRIFS  = p->CheckRIFS;
	inReg->HoldGain   = p->HoldGain;

    // ---------------
    // Squelch Signals
    // ---------------
    ClearDCX  = (p->SqlTime.Q.w6>4*pReg->DelayClearDCX.w4 || p->SquelchRX  ) ? 1:0; // reset DCX accumulator
    SquelchRF = (p->SqlTime.Q.w6>4*pReg->DelaySquelchRF.w4              ) ? 1:0; // enable squelch with RF switch

    // ---------------------------------------------
    // Common/OFDM Receiver Processing at 20, 40 MHz
    // ---------------------------------------------
    if (pRX->clock.posedgeCLK40MHz)
    {
        // ---------------------
        // Common/OFDM Front End
        // ---------------------
        Verona_RX1( p->DFEEnB, p->d2LgSigAFE, &p->LNAGainOFDM, &p->AGainOFDM, &p->RSSI0OFDM, &p->RSSI1OFDM,
                    bSigOut.CSCCK, outReg.bPathMux, &p->bInR, &p->bInI, outReg.RestartRX, outReg.DFERestart, 
                    outReg.CheckRIFS, outReg.HoldGain, outReg.UpdateAGC, ClearDCX, p->dSquelchRX,
                    aSigIn, aSigOut, pRX, pReg);

        if (pRX->clock.posedgeCLK20MHz && outReg.TurnOnOFDM)
        {
            XSTATUS( Verona_RX2(aSigIn, aSigOut, outReg, pRX, pReg) );
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  DSSS/CCK Receiver
    //
    //  Processing occurs on 40 and 44 MHz clocks.
    //  Signal bRXEnB is the enable for "b" DSP/FrontCtrl
    //
    inReg->bRXEnB = p->DFEEnB || !(pReg->RXMode.b1 && outReg.TurnOnCCK);

    if ((pRX->clock.posedgeCLK40MHz || pRX->clock.posedgeCLK44MHz) && pReg->RXMode.b1)
    {
        Verona_bRxState_t *pRXb = Verona_bRxState();

        Verona_bRX_top( pRX, pRXb, pReg, outReg.bRXEnB, &(p->bInR.Q), &(p->bInI.Q), 
                        &p->AGainCCK, &p->LNAGainCCK, p->d2LgSigAFE, &p->RSSICCK, 
                        aSigOut.Run11b, outReg.RestartRX, bSigIn, bSigOut );
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Top-Level Processes at 20 MHz
    //
    if (pRX->clock.posedgeCLK20MHz)
    {
        ClockRegister(p->aValidPacketRSSI);
        p->aValidPacketRSSI.D.word = aSigOut.ValidRSSI & aSigOut.AGCdone & (outReg.State != 21);

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Double Buffer LgSigAFE
        //
        //  These are hardware registers at 20 MHz to avoid meta-stability because
        //  the signal is source from the radio on a 20 MHz clock not phase-aligned
        //  with the baseband.
        //
        p->d2LgSigAFE = p->dLgSigAFE;
        p->dLgSigAFE  = Radio->LGSIGAFE;

        pReg->LgSigAFE.word = p->d2LgSigAFE & 1; // read-only register bit

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  OFDM vs DSSS/CCK Mode Arbitration
        //
        //  If both OFDM and DSSS/CCK are enabled, the OFDM digital front end processes
        //  the packet following carrier sense from either receiver. At the end of the
        //  time-out window for the OFDM short preamble, the DFE decides one of
        //
        //     (a) OFDM preamble validated: assert PeakFound
        //     (b) Not OFDM but DSSS carrier sense: assert Run11b
        //     (c) Not OFDM and not DSSS, DFE resets to RX idle
        //
        if ((pReg->RXMode.w2==3) && (outReg.State==10 || outReg.State==11))
        {
            if (aSigOut.Run11b)    inReg->TurnOnOFDM = 0; // turn off OFDM DSP
            if (aSigOut.PeakFound) inReg->TurnOnCCK  = 0; // turn off CCK DSP
        }
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  bPathMux
        //
        //  When both receive paths are enabled, the multiplexer selects path 0 until
        //  Run11b is asserted by the DFE indicating the preamble is not an OFDM type.
        //  At this point, the multiplexer selects the stronger of the two receive paths.
        //
        //  When a single antenna is active, the multiplexer selects the active path.
        //
        if (outReg.TurnOnOFDM && aSigOut.Run11b && pReg->PathSel.w2==3 && outReg.State == 10)
        {
           inReg->bPathMux = (p->RSSI1OFDM.w8 > p->RSSI0OFDM.w8) ? 1:0;
        }
        else if (!aSigOut.Run11b)
        {
            inReg->bPathMux = pReg->PathSel.b0 ? 0:1;
        }            
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Receive Signal Strength Indicator (RSSI)
    //
    //  The RSSI value can be sourced from either the DSSS based AGC for RXMode = 2 or from
    //  the OFDM AGC for other RXMode values. In addition to the multiplixer, the logical
    //  below indicates when the value reported to the MAC in the PHY header is registerd.
    //  
    //  RSSI without the 0 or 1 suffix is the stronger of the two signals used for CCA
    //
    if (pReg->RXMode.w3 == 2) // 802.11b only
    {
        if (outReg.State==10 && bSigOut.SyncFound) // record RSSI after SFD
        { 
            p->RSSI0.word = p->RSSICCK.w8;
            p->RSSI1.word = 0;
        }
        p->RSSI.word = (outReg.State > 1) ? p->RSSICCK.w8 : 0;
    }
    else // 802.11a/g/n
    { 
        if (p->aValidPacketRSSI.D.b0 && !p->aValidPacketRSSI.Q.b0) // record RSSI when valid after AGC
        { 
            p->RSSI0.word = p->RSSI0OFDM.w8;
            p->RSSI1.word = p->RSSI1OFDM.w8;
        }
        p->RSSI.word = (outReg.State > 1) ? max(p->RSSI0OFDM.w8, p->RSSI1OFDM.w8) : 0;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Nuisance Packet Filter
    //
    //  Check for nuisance packet filters during PSDU processing (state 13). The actual check
    //  occurs when the destination address in the MAC header has been decoded, but this
    //  timing is handled internal to the function. MAC_NOFILTER is a modeled signal driven
    //  from the MAC.
    //
    Verona_RX_NuisanceFilter(outReg.State==13, pRX->MAC_NOFILTER, inReg, pRX, pReg);

    ////////////////////////////////////////////////////////////////////
    //
    // Clear RX_EN on PacketAbort or power-down(Modeling PHY_RX_EN. Non-RTL) 
    //
    if (p->PacketAbort || DW_PHYEnB || p->MAC_TX_EN)
		aSigIn->RX_EN = 0;
	
    if ((!p->dPacketAbort.delay_2T &&  p->dPacketAbort.delay_1T) || DW_PHYEnB || p->MAC_TX_EN)    
        bSigIn->RX_EN = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Baseband Digital Interface I/O Model
    //
    //  Control signals driven from the top level controller are modelled here. This model is
    //  incomplete in that it does not consider the register programming interface or normal
    //  TX/RX MAC/PHY interfaces which are implied by operations in Verona_TX and Verona_RX.
    //
    //  Signals here are bit/cycle-accurate. Many of these signals are not required for normal
    //  operation of the system model. For that case, a subset of these signals following
    //  simplified logic is implemented to improve model runtime.
    //
    if (pRX->EnableModelIO)
    {     
        if (pRX->clock.posedgeCLK80MHz) 
        {
            ClockRegister(p->Signal1);
            ClockRegister(p->Signal2);
        }

        // Radio Reset
        Radio->RESETB = MAC_RSTB & pReg->RESETB.b0;

        // Transmit Enable _____________________________________________________
        TxEnB = (outReg.State==40 || outReg.State==41 || (outReg.State==42 && dMAC_TX_EN)) ? 0:1;

        // Radio Mode Control __________________________________________________
             if (p->RadioSleep  ) Radio->MC = (pReg->RadioMC.w8 >> 6) & 0x3; // bits [7:6]: Sleep
        else if (p->RadioStandby) Radio->MC = (pReg->RadioMC.w8 >> 4) & 0x3; // bits [5:4]: Standby
        else if (p->RadioTX     ) Radio->MC = (pReg->RadioMC.w8 >> 0) & 0x3; // bits [1:0]: Transmit
        else                   Radio->MC = (pReg->RadioMC.w8 >> 2) & 0x3; // bits [3:2]: Receive

        // Data Converters _____________________________________________________
        {
            wiBoolean sA = pReg->PathSel.b0;
            wiBoolean sB = pReg->PathSel.b1;

                 if (!p->ADCEnB & sA) DataConvIn->ADCAOPM = (pReg->ADCAOPM.w8 >> 0) & 0x3; // operational
            else if (!p->BGEnB  & sA) DataConvIn->ADCAOPM = (pReg->ADCAOPM.w8 >> 2) & 0x3; // sleep/standby
            else if (!p->BGEnB  &!sA) DataConvIn->ADCAOPM = (pReg->ADCAOPM.w8 >> 6) & 0x3; // VCMRX
            else                      DataConvIn->ADCAOPM = (pReg->ADCAOPM.w8 >> 4) & 0x3; // power down

                 if (!p->ADCEnB & sB) DataConvIn->ADCBOPM = (pReg->ADCBOPM.w8 >> 0) & 0x3; // operational
            else if (!p->BGEnB  & sB) DataConvIn->ADCBOPM = (pReg->ADCBOPM.w8 >> 2) & 0x3; // sleep/standby
            else if (!p->BGEnB  &!sB) DataConvIn->ADCBOPM = (pReg->ADCBOPM.w8 >> 6) & 0x3; // VCMRX
            else                      DataConvIn->ADCBOPM = (pReg->ADCBOPM.w8 >> 4) & 0x3; // power down

                 if (!p->DACEnB )     DataConvIn->DACOPM  = (pReg->DACOPM.w6  >> 0) & 0x3; // operational
            else if (!p->BGEnB  )     DataConvIn->DACOPM  = (pReg->DACOPM.w6  >> 2) & 0x3; // sleep/standby
            else                      DataConvIn->DACOPM  = (pReg->DACOPM.w6  >> 4) & 0x3; // power down

            DataConvIn->DACOPM = (DataConvIn->DACOPM << 1) | (DataConvIn->DACOPM & 1);  // 2b DACOPM to 3b OPM mapping

            switch (outReg.State)
            {
                case  3 : DataConvIn->ADCFCAL = pReg->ADCFCAL.b0 || (pReg->ADCFCAL1.b0 && (p->PktTime.Q.w16==pReg->DelayFCAL1.w4)); break; // wake from sleep
                case  7 :                                                                                                               // nuisance standby to RX
                case 44 :                                                                                                               // TX oto RX
                case 53 : DataConvIn->ADCFCAL = pReg->ADCFCAL.b0 || (pReg->ADCFCAL2.b0 && (p->PktTime.Q.w16==pReg->DelayFCAL2.w4)); break; // nuisance with RIFS support to RX
                default : DataConvIn->ADCFCAL = pReg->ADCFCAL.b0;
            }

            DataConvIn->ADCARDY = (DataConvOut.ADCAOPM == 3) ? 1:0;
            DataConvIn->ADCBRDY = (DataConvOut.ADCBOPM == 3) ? 1:0;

            switch (pReg->PPSEL_MODE.w2)
            {
                case 0: DataConvIn->PP_SELA = DataConvOut.ADCARDY;  
                        DataConvIn->PP_SELB = DataConvOut.ADCBRDY; break;
                case 1: DataConvIn->PP_SELA = DataConvIn->PP_SELB = (DataConvOut.ADCARDY || DataConvOut.ADCBRDY); break;
                case 2: DataConvIn->PP_SELA = DataConvIn->PP_SELB = 0; break;
                case 3: DataConvIn->PP_SELA = DataConvIn->PP_SELB = 1; break;
            }
            switch (pReg->VCMEN.w2)
            {
                case 0: DataConvIn->VCMEN = p->ADCEnB ? 0:1; break;
                case 1: DataConvIn->VCMEN = p->BGEnB  ? 0:1; break;
                case 2: DataConvIn->VCMEN = 0; break;
                case 3: DataConvIn->VCMEN = 1; break;
            }
        }
        // External LNA and Power Amplifier Enables ____________________________
        {
            wiBoolean c = (p->RadioSleep || p->PAEnB) ? 1:0;
            wiBoolean d = (p->RadioSleep || (p->RadioStandby && pReg->LNAEnB.b1) || (p->RadioTX && pReg->LNAEnB.b0)) ? 1:0;
            wiBoolean SignalMux[16] = {0, 1, c, c^1, d, d^1, p->Signal1.Q.b0, p->Signal2.Q.b0, 
                                       Radio->LNAGAIN2, Radio->LNAGAIN2&(!d), pReg->Event1.b0, pReg->Event2.b0, 0, 0, 0, 0};

            Radio->PA24   = SignalMux[ pReg->sPA24.w4   ];
            Radio->PA50   = SignalMux[ pReg->sPA50.w4   ];
            Radio->LNA24A = SignalMux[ pReg->sLNA24A.w4 ];
            Radio->LNA24B = SignalMux[ pReg->sLNA24B.w4 ];
            Radio->LNA50A = SignalMux[ pReg->sLNA50A.w4 ];
            Radio->LNA50B = SignalMux[ pReg->sLNA50B.w4 ];
        }
        // RF Switches _________________________________________________________
        {
            wiUWORD s;  s.b1=AntSel; s.b0=(p->AntTxEnB && !SquelchRF);
            
            Radio->SW1  =                         (pReg->RFSW1.w4      >> s.w2) & 1;
            Radio->SW1B =                         (pReg->RFSW1.w8 >> 4 >> s.w2) & 1;
            Radio->SW2  = !pReg->LNAxxBOnSW2.b0? ((pReg->RFSW2.w4      >> s.w2) & 1) : Radio->LNA24B;
            Radio->SW2B = !pReg->LNAxxBOnSW2.b0? ((pReg->RFSW2.w8 >> 4 >> s.w2) & 1) : Radio->LNA50B;
        }
        // Radio Gain Control Signals __________________________________________
        {
            if (p->RadioTX) // transmit mode selection
            {
                wiUWORD TXPWRLVL = pReg->TxHeader1;     // RTL: TXPWRLVL is first byte on MAC_TX_D; output from TX Control
                Radio->LNAGAIN2  = pReg->TxLNAGAIN2.b0; // Register bit
                Radio->LNAGAIN   = TXPWRLVL.b5; // TXPWRLVL[5]
                Radio->AGAIN     = TXPWRLVL.w5; // TXPWRLVL[4:0]
            }
            else // receive/sleep mode selection
            {
                Radio->LNAGAIN2 = p->RadioSleep ? 0 : (pReg->RXMode.b0 ? p->LNAGainOFDM.b1:p->LNAGainCCK.b1 );
                Radio->LNAGAIN  = p->RadioSleep ? 0 : (pReg->RXMode.b0 ? p->LNAGainOFDM.b0:p->LNAGainCCK.b0 );
                Radio->AGAIN    = p->RadioSleep ? 0 : (pReg->RXMode.b0 ? p->AGainOFDM.w6  :p->AGainCCK.w6   );
            }
        }
	    //////////////////////////////////////////////////////////////////////////////////////////
        //
	    //  Generate PHY_RX_EN
        // 
	    //  For OFDM, PHY_RX_EN is based on 20 MHz clk; for CCK, PHY_RX_EN is based on 22 MHz clk.
	    //  Adjusted for 80 MHz clk to lineup with RTL Top-Level signal
        //
	    if (pRX->clock.posedgeCLK80MHz)	
	    {	
            wiBitDelayLine dPacketAbort;
            dPacketAbort.word = (p->dPacketAbort.word << 1) | p->PacketAbort;
		    // OFDM
		    ClockDelayLine(p->DaPSDU_EN, aSigIn->RX_EN);

            // Assertion for non-6Mbps Legacy packet
			if (pRX->LegacyPacket && pRX->RX_RATE.w7 != 0xD && p->DaPSDU_EN.delay_1T && !p->DaPSDU_EN.delay_2T)
				p->aPSDU_EN = 1; // for rising edge
			
			// Assertion for 6Mbps Legacy packet
			if (pRX->LegacyPacket && pRX->RX_RATE.w7 == 0xD && p->DaPSDU_EN.delay_1T && !p->DaPSDU_EN.delay_2T)
				p->aPSDU_EN = 1; // for rising edge
			
			// Assertion for HT supported packet
		    if (pRX->HtHeaderValid  && p->DaPSDU_EN.delay_2T && !p->DaPSDU_EN.delay_3T)  // HT supported packet
				p->aPSDU_EN = 1; // for rising edge
			
			// Deassertion for normal supported packet or PacketAbort
		    if ((!p->DaPSDU_EN.delay_3T &&  p->DaPSDU_EN.delay_4T) || (!dPacketAbort.delay_4T &&  dPacketAbort.delay_3T))
				p->aPSDU_EN = 0; // for falling edge or fault/unsupported cases
			
		    // DSSS/CCK
		    ClockDelayLine(p->DbPSDU_EN, bSigOut.RX_EN);                      // bSigOut has one 22M clk delay
		    if ( p->DbPSDU_EN.delay_1T && !p->DbPSDU_EN.delay_2T)  p->bPSDU_EN = 1; // for rising edge
		    if ((!p->DbPSDU_EN.delay_1T &&  p->DbPSDU_EN.delay_2T) || (!dPacketAbort.delay_4T &&  dPacketAbort.delay_3T))
				p->bPSDU_EN = 0; // for falling edge
						    
		    // Top level PHY_RX_EN MUX
		    p->PHY_RX_EN = ((p->aPSDU_EN && !p->bPSDU_EN) || (p->bPSDU_EN && !p->aPSDU_EN)) ? 1 : 0;
	    }
        // Generate signals PHY_CCA and PHY_RX_ERROR ___________________________
        //
        Verona_RX_ERROR( inReg, outReg, p->PktTime.Q );
        Verona_RX_CCA  ( inReg, outReg, p->PktTime.Q, p->RSSI, p->PHY_RX_EN, aSigOut.ValidRSSI );

        // Abort/Fault Based Interrupts //////////////////////////////////////
        //
        if (pReg->ClearInt.b0)
        {
            pReg->Event2.word    = 0;
            pReg->Event1.word    = 0;
            pReg->RxFault.word   = 0;
            pReg->RxAbortTx.word = 0;
            pReg->TxSleep.word   = 0;
            pReg->TxAbort.word   = 0;
            pReg->TxFault.word   = 0;
        }

        p->PHY_DFS_INT = ((pReg->IntTxFault.b0   && pReg->TxFault.b0)
                       || (pReg->IntTxAbort.b0   && pReg->TxAbort.b0)
                       || (pReg->IntTxSleep.b0   && pReg->TxSleep.b0)
                       || (pReg->IntRxAbortTx.b0 && pReg->RxAbortTx.b0)
                       || (pReg->IntRxFault.b0   && pReg->RxFault.b0)
                       || (pReg->IntEvent1.b0    && pReg->Event1.b0)
                       || (pReg->IntEvent2.b0    && pReg->Event2.b0)
                       || (pReg->DFSIntDet.b0    && pReg->detRadar.b0)
                       || 0 ) ? 1:0; // last item is to include FIFO interrupt not modelled here

        pReg->Interrupt.word = p->d2PHY_DFS_INT ? 1:0;
    
        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  General Purpose Signal and Event Trigger Defintions
        //
        //  A macro defintion is used to defined the 16-to-1 multiplexer which is replicated
        //  for both Signal 1 and Signal2.
        //
        #define VERONA_SELECTSIGNAL( SelectSignal, Signal )                                                 \
        {                                                                                                   \
            switch (SelectSignal)                                                                           \
            {                                                                                               \
                case  0: (Signal) = 0; break;                                                               \
                case  1: (Signal) = DW_PHYEnB; break;                                                       \
                case  2: (Signal) = p->MAC_TX_EN; break;                                                       \
                case  3: (Signal) = p->PHY_RX_EN; break;                                                       \
                case  4: (Signal) = outReg.PHY_CCA; break;                                                  \
                case  5: (Signal) = p->d2PHY_DFS_INT; break;                                                   \
                case  6: (Signal) = p->d2LgSigAFE; break;                                                   \
                case  7: (Signal) = outReg.State == pReg->TriggerState1.w6 ? 1:0; break;                    \
                case  8: (Signal) = outReg.State == pReg->TriggerState2.w6 ? 1:0; break;                    \
                case  9: (Signal) = aSigOut.ValidRSSI && (p->RSSI.w8 >= pReg->TriggerRSSI.w8) ? 1:0; break; \
                case 10: (Signal) = pReg->CalDone.b0; break;                                                \
                case 11: (Signal) = pReg->ImgDetDone.b0; break;                                             \
                case 12: (Signal) = Radio->MC & 1; break;                                                   \
                case 13: (Signal) = outReg.bPathMux; break;                                                 \
                case 14: (Signal) = DataConvOut.ADCARDY; break;                                             \
                case 15: (Signal) = DataConvOut.ADCBRDY; break;                                             \
            }                                                                                               \
        } // end of VERONA_SELECTSIGNAL() //////////////////////////////////////////////////////////////////

        VERONA_SELECTSIGNAL(pReg->SelectSignal1.w4, p->Signal1.D.word);
        VERONA_SELECTSIGNAL(pReg->SelectSignal2.w4, p->Signal2.D.word);

        pReg->Signal1.word = p->Signal1.Q.b0;
        pReg->Signal2.word = p->Signal2.Q.b0;

        switch (pReg->SelectTrigger1.w2)
        {
            case 0: pReg->Trigger1.b0 = p->Signal1.Q.b0;                   break;
            case 1: pReg->Trigger1.b0 =                   p->Signal2.Q.b0; break;
            case 2: pReg->Trigger1.b0 = p->Signal1.Q.b0 & p->Signal2.Q.b0; break;
            case 3: pReg->Trigger1.b0 = p->Signal1.Q.b0 | p->Signal2.Q.b0; break;
        }
        switch (pReg->SelectTrigger2.w2)
        {
            case 0: pReg->Trigger2.b0 = p->Signal1.Q.b0;                   break;
            case 1: pReg->Trigger2.b0 =                   p->Signal2.Q.b0; break;
            case 2: pReg->Trigger2.b0 = p->Signal1.Q.b0 & p->Signal2.Q.b0; break;
            case 3: pReg->Trigger2.b0 = p->Signal1.Q.b0 | p->Signal2.Q.b0; break;
        }
        pReg->Event1.b0 = pReg->Event1.b0 | pReg->Trigger1.b0;
        pReg->Event2.b0 = pReg->Event2.b0 | pReg->Trigger2.b0;                    
    }
    else // simple I/O for system simulations ////////////////////////////////////////////////
    {
        Radio->MC           = (p->RadioSleep || p->RadioStandby) ? 0:3; // Sleep:Receive switching
        Radio->SW2          = AntSel;                             // follow AntSel
        DataConvIn->ADCAOPM = 3;                                  // ADCA enabled
        DataConvIn->ADCBOPM = 3;                                  // ADCB enabled

        Radio->LNAGAIN2 = p->RadioSleep ? 0 : (pReg->RXMode.b0 ? p->LNAGainOFDM.b1:p->LNAGainCCK.b1 );
        Radio->LNAGAIN  = p->RadioSleep ? 0 : (pReg->RXMode.b0 ? p->LNAGainOFDM.b0:p->LNAGainCCK.b0 );
        Radio->AGAIN    = p->RadioSleep ? 0 : (pReg->RXMode.b0 ? p->AGainOFDM.w6  :p->AGainCCK.w6   );
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Load Squelch Timer on Gain/Mode Changes
    //
    //  The squelch timer is primarily used to block DCX output following either a gain
    //  change or a mode change. The idea is that large "garbage" transient signals can
    //  occur at these points and it is preferable to feed a zero-signal to subsequent
    //  baseband DSP as opposed to large transient signals.
    //
    //  The code operates in RX mode and looks for a change on any analog gain line or
    //  de-activation of DFE (DFEEnB = 1). Typical programming causes a transition from
    //  either standby or TX into RX prior to activation of the digital front end.
    //
    if (!p->RadioSleep && !p->RadioTX)
    {
        inReg->dLNAGain2 = Radio->LNAGAIN2; // register gain state for edge detector below
        inReg->dLNAGain  = Radio->LNAGAIN;
        inReg->dAGain    = Radio->AGAIN;

        if (p->DFEEnB 
        || (Radio->LNAGAIN2 != outReg.dLNAGain2) 
        || (Radio->LNAGAIN  != outReg.dLNAGain) 
        || (Radio->AGAIN    != outReg.dAGain) )
            p->SqlTime.D.word = pReg->SquelchTime.w6; // set time on gain change or DFEEnB->0
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Frame Counters
    //
    //  These counters are used to record the number of good and bad packets in a waveform.
    //  This function is not implemented in RTL but is included here to support architecture
    //  test functions.
    //
    {
        p->dRxDone = p->RxDone;

        p->RxDone  =  (pReg->RXMode.b0 && aSigOut.RxDone) 
                   || (pReg->RXMode.b1 && bSigOut.RxDone) ? 1:0;

        if (p->RxDone && !p->dRxDone && pRX->N_PHY_RX_WR > 8)
        {
            wiBoolean ValidFCS = wiMAC_ValidFCS(pRX->N_PHY_RX_WR - 8, pRX->PHY_RX_D + 8);
            
            pRX->FrameCount.Total++;
            pRX->FrameCount.ValidFCS  += (ValidFCS ? 1:0);
            pRX->FrameCount.BadFCS    += (ValidFCS ? 0:1);
            pRX->FrameCount.FrameCheck = (pRX->FrameCount.FrameCheck << 1) | 1;
            pRX->FrameCount.FrameValid = (pRX->FrameCount.FrameValid << 1) | (ValidFCS? 1:0);

            if(pRX->EnableTrace)
                pRX->traceRx[pRX->k80].FCS = ValidFCS ? 1 : -1;
        }
        else if(pRX->EnableTrace) pRX->traceRx[pRX->k80].FCS = 0;
    }        
    ///////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Trace Signals
    //
    //  Signals output when EnableTrace is selected are used for examination of internal
    //  control signals and HDL verification. They are simply storage arrays to output these
    //  internal signals and do not affect normal operation. They may be disabled to reduce
    //  memory requirements and runtime during system simulations.
    //
    if (pRX->EnableTrace)
    {
        unsigned int k = pRX->k80;

        pReg->TopState.word = outReg.State;
        pReg->DW_PHYEnB.word = DW_PHYEnB;

        if (pRX->clock.posedgeCLK20MHz) pRX->aSigOut[pRX->k20] = aSigOut;
        if (pRX->clock.posedgeCLK22MHz) pRX->bSigOut[pRX->k22] = bSigOut;

        pRX->traceRxClock[k].traceValid       = 1;
        pRX->traceRxClock[k].posedgeCLK20MHz  = pRX->clock.posedgeCLK20MHz;
        pRX->traceRxClock[k].posedgeCLK40MHz  = pRX->clock.posedgeCLK40MHz;
        pRX->traceRxClock[k].posedgeCLK80MHz  = pRX->clock.posedgeCLK80MHz;
        pRX->traceRxClock[k].posedgeCLK11MHz  = pRX->clock.posedgeCLK11MHz;
        pRX->traceRxClock[k].posedgeCLK22MHz  = pRX->clock.posedgeCLK22MHz;
        pRX->traceRxClock[k].posedgeCLK44MHz  = pRX->clock.posedgeCLK44MHz;

        pRX->traceRxState[k].traceValid       = 1;
        pRX->traceRxState[k].posedgeCLK20MHz  = pRX->clock.posedgeCLK20MHz;
        pRX->traceRxState[k].posedgeCLK22MHz  = pRX->clock.posedgeCLK22MHz;
        pRX->traceRxState[k].State            = outReg.State;
        pRX->traceRxState[k].PktTime          = p->PktTime.Q.w16;
        pRX->traceRxState[k].phase80          = p->phase80.Q.w7;

        pRX->traceRxControl[k].DW_PHYEnB      = DW_PHYEnB;
        pRX->traceRxControl[k].PHY_Sleep      = outReg.PHY_Sleep;
        pRX->traceRxControl[k].MAC_TX_EN      = p->MAC_TX_EN;
        pRX->traceRxControl[k].PHY_TX_DONE    = 0; // not used
        pRX->traceRxControl[k].PHY_CCA        = outReg.PHY_CCA;
        pRX->traceRxControl[k].PHY_RX_EN      = p->PHY_RX_EN;
        pRX->traceRxControl[k].PHY_RX_ABORT   = 0; // not used
        pRX->traceRxControl[k].TxEnB          = TxEnB;
        pRX->traceRxControl[k].TxDone         = TxDone;
        pRX->traceRxControl[k].RadioSleep     = p->RadioSleep;
        pRX->traceRxControl[k].RadioStandby   = p->RadioStandby;
        pRX->traceRxControl[k].RadioTX        = p->RadioTX;
        pRX->traceRxControl[k].PAEnB          = p->PAEnB;
        pRX->traceRxControl[k].AntTxEnB       = p->AntTxEnB;
        pRX->traceRxControl[k].BGEnB          = p->BGEnB;
        pRX->traceRxControl[k].DACEnB         = p->DACEnB;
        pRX->traceRxControl[k].ADCEnB         = p->ADCEnB;
        pRX->traceRxControl[k].ModemSleep     = p->ModemSleep;
        pRX->traceRxControl[k].PacketAbort    = p->PacketAbort;
        pRX->traceRxControl[k].RestartRX      = p->RestartRX;
        pRX->traceRxControl[k].DFEEnB         = p->DFEEnB;
        pRX->traceRxControl[k].DFERestart     = p->DFERestart;
        pRX->traceRxControl[k].TurnOnOFDM     = outReg.TurnOnOFDM;
        pRX->traceRxControl[k].TurnOnCCK      = outReg.TurnOnCCK;
        pRX->traceRxControl[k].NuisancePacket = outReg.NuisancePacket;
        pRX->traceRxControl[k].ClearDCX       = ClearDCX;
        pRX->traceRxControl[k].bPathMux       = outReg.bPathMux;
        pRX->traceRxControl[k].PHY_RX_ERROR   = outReg.PHY_RX_ERROR;
        pRX->traceRxControl[k].PHY_DFS_INT    = p->d2PHY_DFS_INT;
        pRX->traceRxControl[k].posedgeCLK40MHz= pRX->clock.posedgeCLK40MHz;
        pRX->traceRxControl[k].posedgeCLK11MHz= pRX->clock.posedgeCLK11MHz;      

        pRX->traceRx[k].traceValid = 1;
        
        pRX->traceRx[k].Marker1 = ( k == pRX->Trace.kMarker[1]) ? 1:0;
        pRX->traceRx[k].Marker2 = ( k == pRX->Trace.kMarker[2]) ? 1:0;
        pRX->traceRx[k].Marker3 = ( k == pRX->Trace.kMarker[3]) ? 1:0;
        pRX->traceRx[k].Marker4 = ( k == pRX->Trace.kMarker[4]) ? 1:0;
        
        pRX->traceRx[k].Window1 = ((k >= pRX->Trace.kStart[1]) && (k <= pRX->Trace.kEnd[1])) ? 1:0;
        pRX->traceRx[k].Window2 = ((k >= pRX->Trace.kStart[2]) && (k <= pRX->Trace.kEnd[2])) ? 1:0;
        pRX->traceRx[k].Window3 = ((k >= pRX->Trace.kStart[3]) && (k <= pRX->Trace.kEnd[3])) ? 1:0;
        pRX->traceRx[k].Window4 = ((k >= pRX->Trace.kStart[4]) && (k <= pRX->Trace.kEnd[4])) ? 1:0;
        pRX->traceRx[k].Window5 = ((k >= pRX->Trace.kStart[5]) && (k <= pRX->Trace.kEnd[5])) ? 1:0;
        pRX->traceRx[k].Window6 = ((k >= pRX->Trace.kStart[6]) && (k <= pRX->Trace.kEnd[6])) ? 1:0;
        pRX->traceRx[k].Window7 = ((k >= pRX->Trace.kStart[7]) && (k <= pRX->Trace.kEnd[7])) ? 1:0;
        pRX->traceRx[k].Window8 = ((k >= pRX->Trace.kStart[8]) && (k <= pRX->Trace.kEnd[8])) ? 1:0;

        pRX->traceRx[k].Window  = ( pRX->traceRx[k].Window1 || pRX->traceRx[k].Window2
                                 || pRX->traceRx[k].Window3 || pRX->traceRx[k].Window4 ) ? 1:0;
    }
    return WI_SUCCESS;
}
// end of Verona_RX()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

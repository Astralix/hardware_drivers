//--< Verona_b.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Verona - DSSS/CCK (11b) Baseband
//  Copyright 2001-2003 Bermai, 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "wiRnd.h"
#include "wiUtil.h"
#include "wiParse.h"
#include "Verona.h"

//=================================================================================================
//--  INTERNAL ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

//=================================================================================================
//--  OTHER MACRO DEFINITIONS
//=================================================================================================
#define SQR(arg) ((arg)*(arg))
#define ClockRegister(arg) {(arg).Q=(arg).D;};
#define ResetRegister(arg) {(arg).Q.word=(arg).D.word=0;};
#define ClockDelayLine(SR,value) {(SR).word = ((SR).word << 1) | ((value)&1);};
#define PriorityEncoder(X) ((unsigned)floor(log((double)X+0.5)/log(2.0)))
#define limit(x,llim,ulim) (min(max((llim),(x)),(ulim)))

// ================================================================================================
// FUNCTION : Verona_b_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_b_Init()
{
	Verona_LookupTable_t *pLut = Verona_LookupTable();

    Verona_bRxState()->N44  = 0;

    // ------------------------------------------
    // Build CCK Correlation Sequences
    //    "p" terms are phases with units of pi/2
    // ------------------------------------------
    {
        const int cR[4] = {1, 0, -1, 0}; // coordinates for phase shifts
        const int cI[4] = {0, 1, 0, -1}; // as {1, j,-1,-j}
        int k,p,p2,p3,p4,d2,d3;

        for (k=0; k<4; k++) // table for CCK at 5.5Mbps
        {
            d2 = (k >> 1) & 0x01;
            d3 = (k >> 0) & 0x01;

            p2 = 2*d2 + 1;
            p3 = 0;
            p4 = 2*d3;

            p = (p2+p3+p4+0)%4;  pLut->CC55Real[k][0].word = cR[p];  pLut->CC55Imag[k][0].word = cI[p]; 
            p = (   p3+p4+0)%4;  pLut->CC55Real[k][1].word = cR[p];  pLut->CC55Imag[k][1].word = cI[p]; 
            p = (p2   +p4+0)%4;  pLut->CC55Real[k][2].word = cR[p];  pLut->CC55Imag[k][2].word = cI[p]; 
            p = (      p4+2)%4;  pLut->CC55Real[k][3].word = cR[p];  pLut->CC55Imag[k][3].word = cI[p]; 
            p = (p2+p3   +0)%4;  pLut->CC55Real[k][4].word = cR[p];  pLut->CC55Imag[k][4].word = cI[p]; 
            p = (   p3   +0)%4;  pLut->CC55Real[k][5].word = cR[p];  pLut->CC55Imag[k][5].word = cI[p]; 
            p = (p2      +2)%4;  pLut->CC55Real[k][6].word = cR[p];  pLut->CC55Imag[k][6].word = cI[p]; 
            p = (         0)%4;  pLut->CC55Real[k][7].word = cR[p];  pLut->CC55Imag[k][7].word = cI[p]; 
        }
        for (k=0; k<64; k++) // table for CCK at 11Mbps
        {
            p2 = (k>>4) & 0x03;
            p3 = (k>>2) & 0x03;
            p4 = (k>>0) & 0x03;

            p = (p2+p3+p4+0)%4; pLut->CC110Real[k][0].word = cR[p];  pLut->CC110Imag[k][0].word = cI[p];
            p = (   p3+p4+0)%4; pLut->CC110Real[k][1].word = cR[p];  pLut->CC110Imag[k][1].word = cI[p];
            p = (p2   +p4+0)%4; pLut->CC110Real[k][2].word = cR[p];  pLut->CC110Imag[k][2].word = cI[p];
            p = (      p4+2)%4; pLut->CC110Real[k][3].word = cR[p];  pLut->CC110Imag[k][3].word = cI[p]; 
            p = (p2+p3   +0)%4; pLut->CC110Real[k][4].word = cR[p];  pLut->CC110Imag[k][4].word = cI[p]; 
            p = (   p3   +0)%4; pLut->CC110Real[k][5].word = cR[p];  pLut->CC110Imag[k][5].word = cI[p];  
            p = (p2      +2)%4; pLut->CC110Real[k][6].word = cR[p];  pLut->CC110Imag[k][6].word = cI[p];  
            p = (         0)%4; pLut->CC110Real[k][7].word = cR[p];  pLut->CC110Imag[k][7].word = cI[p];  
        }
    }
    return WI_SUCCESS;
}
// end of Verona_b_Init()

// ================================================================================================
// FUNCTION : Verona_bTX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// Parameters: Na -- Number of serial bits
//             Nc -- Number of samples at Baud rate (11 MHz)
//             M  -- Oversample rate at DAC output
// ================================================================================================
wiStatus Verona_bTX_AllocateMemory(int Na, int Nc, int M)
{
    Verona_TxState_t *pTX = Verona_TxState();

    // Determine if a new memory allocation is required
    //
    if ((pTX->LastAllocation.Fn      != (wiFunction)Verona_bTX_AllocateMemory) ||
        (pTX->LastAllocation.Size[0] != Na) ||
        (pTX->LastAllocation.Size[1] != Nc) ||
        (pTX->LastAllocation.Size[2] != M ) ||
        (pTX->LastAllocation.Size[3] != 0 ) ||
        (pTX->LastAllocation.Size[4] != 0 ) ||
        (pTX->LastAllocation.Size[5] != 0 ) ||
        (pTX->LastAllocation.Size[6] != (int)pTX->bPadFront) ||
        (pTX->LastAllocation.Size[7] != (int)pTX->bPadBack)
        )
    {
        pTX->LastAllocation.Fn      = (wiFunction)Verona_bTX_AllocateMemory;
        pTX->LastAllocation.Size[0] = Na;
        pTX->LastAllocation.Size[1] = Nc;
        pTX->LastAllocation.Size[2] = M;
        pTX->LastAllocation.Size[3] = 0;
        pTX->LastAllocation.Size[4] = 0;
        pTX->LastAllocation.Size[5] = 0;
        pTX->LastAllocation.Size[6] = pTX->bPadFront;
        pTX->LastAllocation.Size[7] = pTX->bPadBack;
    }
    else 
    {
        return WI_SUCCESS;
    }
    Verona_TX_FreeMemory(pTX, 0);

    pTX->Na = Na;                                 // number of serial bits
    pTX->Nb = Na;                                 // number of scrambled bits
    pTX->Nc = Nc + pTX->bPadFront + pTX->bPadBack;// add padding samples at 11 MHz
    pTX->Nv = 4*pTX->Nc + 12 + 9;                 // 44 MHz samples, add for filter and delays
    pTX->Nx = 10*(pTX->Nv+10)/11;                 // 40 MHz samples (+10 to cover delays, CYA)
    pTX->Ny = 2*pTX->Nx;                          // 80 MHz samples
    pTX->Nz =   pTX->Ny;                          // Mixer output
    pTX->Nd = M*pTX->Nz;                          // M times oversampled DAC output
    
    pTX->a     = (wiUWORD  *)wiCalloc(pTX->Na, sizeof(wiUWORD));
    pTX->b     = (wiUWORD  *)wiCalloc(pTX->Nb, sizeof(wiUWORD));
    pTX->cReal = (wiUWORD  *)wiCalloc(pTX->Nc, sizeof(wiUWORD));
    pTX->cImag = (wiUWORD  *)wiCalloc(pTX->Nc, sizeof(wiUWORD));
    pTX->vReal = (wiWORD   *)wiCalloc(pTX->Nv, sizeof(wiWORD));
    pTX->vImag = (wiWORD   *)wiCalloc(pTX->Nv, sizeof(wiWORD));
    pTX->xReal = (wiWORD   *)wiCalloc(pTX->Nx, sizeof(wiWORD));
    pTX->xImag = (wiWORD   *)wiCalloc(pTX->Nx, sizeof(wiWORD));
    pTX->yReal = (wiWORD   *)wiCalloc(pTX->Ny, sizeof(wiWORD));
    pTX->yImag = (wiWORD   *)wiCalloc(pTX->Ny, sizeof(wiWORD));
    pTX->zReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD));
    pTX->zImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD));
    pTX->qReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->qImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->rReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->rImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->sReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->sImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->uReal = (wiUWORD  *)wiCalloc(pTX->Nz, sizeof(wiUWORD));
    pTX->uImag = (wiUWORD  *)wiCalloc(pTX->Nz, sizeof(wiUWORD));
    pTX->d     = (wiComplex*)wiCalloc(pTX->Nd, sizeof(wiComplex));
    pTX->traceTx = (Verona_traceTxType  *)wiCalloc(pTX->Nz, sizeof(Verona_traceTxType));

    if (!pTX->a || !pTX->b || !pTX->cReal || !pTX->cImag || !pTX->vReal || !pTX->vImag || !pTX->xReal || !pTX->xImag 
        || !pTX->yReal || !pTX->yImag || !pTX->zReal || !pTX->zImag || !pTX->uReal || !pTX->uImag || !pTX->d
        || !pTX->qReal || !pTX->qImag || !pTX->rReal || !pTX->rImag || !pTX->sReal || !pTX->sImag 
        || !pTX->traceTx)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    return WI_SUCCESS;
}
// end of bVerona_bTX_AllocateMemory()

// ================================================================================================
// FUNCTION : Verona_bRX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_bRX_AllocateMemory(void)
{
    Verona_bRxState_t *pbRX = Verona_bRxState();
    int N80 = (pbRX->N44 * 80/44) + 2; // +2 to cover rounding errors

    Verona_bRX_FreeMemory(pbRX, 0);

    pbRX->N40 = pbRX->N44*40/44+2; // all counts use +2: +1 for rounding and +1 for the case
    pbRX->N22 = pbRX->N44/2 + 2;   // ...where the initial clock index is 1 instead of 0.
    pbRX->N11 = pbRX->N44/4 + 2;   // ...due to new clocking scheme as of WiSE.060306a
    pbRX->Na  = 8*4095 + 192;
    pbRX->Nb  = pbRX->Na;
    
    pbRX->a    =(wiUWORD *)wiCalloc(pbRX->Na,  sizeof(wiUWORD));
    pbRX->b    =(wiUWORD *)wiCalloc(pbRX->Nb,  sizeof(wiUWORD));
    pbRX->xReal=(wiWORD  *)wiCalloc(pbRX->N44, sizeof(wiWORD));
    pbRX->xImag=(wiWORD  *)wiCalloc(pbRX->N44, sizeof(wiWORD));
    pbRX->yReal=(wiWORD  *)wiCalloc(pbRX->N44, sizeof(wiWORD));
    pbRX->yImag=(wiWORD  *)wiCalloc(pbRX->N44, sizeof(wiWORD));
    pbRX->sReal=(wiWORD  *)wiCalloc(pbRX->N44, sizeof(wiWORD));
    pbRX->sImag=(wiWORD  *)wiCalloc(pbRX->N44, sizeof(wiWORD));
    pbRX->zReal=(wiWORD  *)wiCalloc(pbRX->N22, sizeof(wiWORD));
    pbRX->zImag=(wiWORD  *)wiCalloc(pbRX->N22, sizeof(wiWORD));
    pbRX->rReal=(wiWORD  *)wiCalloc(pbRX->N22, sizeof(wiWORD));
    pbRX->rImag=(wiWORD  *)wiCalloc(pbRX->N22, sizeof(wiWORD));
    pbRX->vReal=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->vImag=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->uReal=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->uImag=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->wReal=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->wImag=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->pReal=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->pImag=(wiWORD  *)wiCalloc(pbRX->N11, sizeof(wiWORD));
    pbRX->tReal=(wiWORD  *)wiCalloc(pbRX->N11+14, sizeof(wiWORD));
    pbRX->tImag=(wiWORD  *)wiCalloc(pbRX->N11+14, sizeof(wiWORD));
    pbRX->EDOut=(wiUWORD *)wiCalloc(pbRX->N22, sizeof(wiUWORD));
    pbRX->CQOut=(wiUWORD *)wiCalloc(pbRX->N22, sizeof(wiUWORD));

    if (Verona_RxState()->EnableTrace)
    {
        pbRX->traceCCK     = (Verona_b_traceCCKType     *)wiCalloc(pbRX->N22, sizeof(Verona_b_traceCCKType)     );
        pbRX->traceState   = (Verona_b_traceStateType   *)wiCalloc(pbRX->N22, sizeof(Verona_b_traceStateType)   );
        pbRX->traceControl = (Verona_b_traceControlType *)wiCalloc(pbRX->N22, sizeof(Verona_b_traceControlType) );
        pbRX->traceFrontReg= (Verona_b_FrontCtrlRegType *)wiCalloc(pbRX->N22, sizeof(Verona_b_FrontCtrlRegType) );
        pbRX->traceDPLL    = (Verona_b_traceDPLLType    *)wiCalloc(pbRX->N22, sizeof(Verona_b_traceDPLLType)    );
        pbRX->traceBarker  = (Verona_b_traceBarkerType  *)wiCalloc(pbRX->N22, sizeof(Verona_b_traceBarkerType)  );
        pbRX->traceDSSS    = (Verona_b_traceDSSSType    *)wiCalloc(pbRX->N22, sizeof(Verona_b_traceDSSSType)    );
        pbRX->traceCCKInput= (Verona_b_traceCCKFBFType  *)wiCalloc( 65536,  sizeof(Verona_b_traceCCKFBFType)  );
        pbRX->traceCCKTVMF = (Verona_b_traceCCKTVMFType *)wiCalloc( 65536,  sizeof(Verona_b_traceCCKTVMFType) );
        pbRX->traceCCKFWT  = (Verona_b_traceCCKFWTType  *)wiCalloc(262144,  sizeof(Verona_b_traceCCKTVMFType) );
        pbRX->traceResample= (Verona_b_traceResampleType*)wiCalloc(N80,     sizeof(Verona_b_traceResampleType));
        pbRX->k80          = (unsigned int              *)wiCalloc(pbRX->N22, sizeof(unsigned int)              );

        if ( !pbRX->traceCCK     || !pbRX->traceState  || !pbRX->traceControl || !pbRX->traceFrontReg 
          || !pbRX->traceDPLL    || !pbRX->traceBarker || !pbRX->traceDSSS    || !pbRX->traceCCKInput 
          || !pbRX->traceCCKTVMF || !pbRX->traceCCKFWT || !pbRX->traceResample ) 
        {
            wiUtil_WriteLogFile("Verona_bRX_AllocateMemory: N44 = %u, N80 = %u\n",pbRX->N44, N80);
            return STATUS(WI_ERROR_MEMORY_ALLOCATION);
        }
    }
    // ---------------------------------
    // Check for memory allocation fault
    // ---------------------------------
    if ( !pbRX->a     || !pbRX->b
      || !pbRX->xReal || !pbRX->xImag || !pbRX->yReal || !pbRX->yImag
      || !pbRX->sReal || !pbRX->sImag || !pbRX->zReal || !pbRX->zImag
      || !pbRX->vReal || !pbRX->vImag || !pbRX->uReal || !pbRX->uImag
      || !pbRX->tReal || !pbRX->tImag || !pbRX->wReal || !pbRX->wImag
      || !pbRX->pReal || !pbRX->pImag || !pbRX->rReal || !pbRX->rImag
      || !pbRX->EDOut || !pbRX->CQOut )
    {
        wiUtil_WriteLogFile("Verona_bRX_AllocateMemory: N44 = %u, N80 = %u\n",pbRX->N44, N80);
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    }
    return WI_SUCCESS;
}
// end of Verona_bRX_AllocateMemory()

// ================================================================================================
// FUNCTION : Verona_bRX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_bRX_FreeMemory(Verona_bRxState_t *pbRX, wiBoolean ClearPointerOnly)
{
    #define FREE(ptr)                                        \
    {                                                        \
        if (!ClearPointerOnly && (ptr != NULL)) wiFree(ptr); \
        ptr = NULL;                                          \
    }

    FREE(pbRX->a); pbRX->Na = 0;
    FREE(pbRX->b); pbRX->Nb = 0;

    FREE(pbRX->xReal);  FREE(pbRX->xImag);
    FREE(pbRX->yReal);  FREE(pbRX->yImag);
    FREE(pbRX->sReal);  FREE(pbRX->sImag);
    FREE(pbRX->zReal);  FREE(pbRX->zImag);
    FREE(pbRX->vReal);  FREE(pbRX->vImag);
    FREE(pbRX->uReal);  FREE(pbRX->uImag);
    FREE(pbRX->tReal);  FREE(pbRX->tImag);
    FREE(pbRX->wReal);  FREE(pbRX->wImag);
    FREE(pbRX->pReal);  FREE(pbRX->pImag);
    FREE(pbRX->rReal);  FREE(pbRX->rImag);

    FREE(pbRX->EDOut);
    FREE(pbRX->CQOut);

    FREE(pbRX->traceCCK);
    FREE(pbRX->traceState);
    FREE(pbRX->traceControl);
    FREE(pbRX->traceFrontReg);
    FREE(pbRX->traceDPLL);
    FREE(pbRX->traceBarker);
    FREE(pbRX->traceDSSS);
    FREE(pbRX->traceCCKInput);
    FREE(pbRX->traceCCKTVMF);
    FREE(pbRX->traceCCKFWT);
    FREE(pbRX->traceResample);
    FREE(pbRX->k80);
    
    #undef FREE
    return WI_SUCCESS;
}
// end of Verona_bRX_FreeMemory()

// ================================================================================================
// FUNCTION  : Verona_bTX_DataGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate bit sequence from PSDU bytes, preamble length, and data rate
// Parameters: RATE    - RATE field from PHY header
//             NumByte - PSDU length in bytes
//             PSDU    - PHY Service Data Unit (Data Bytes)
//             a       - serialized bit stream
//             ModSel  - ModSelectionBit (0=CCK, 1=PBCC)
// ================================================================================================
void Verona_bTX_DataGenerator(wiUWORD RATE, int NumByte, wiUWORD *PSDU, wiUWORD *a, wiBoolean ModSel)
{
    int b,i,k,n,r;
    Verona_TxState_t *pTX = Verona_TxState();

    wiUWORD SIGNAL  = {0};
    wiUWORD SERVICE = {0x04};
    wiUWORD LENGTH  = {0};   // length of PSDU in microseconds
    wiUWORD HEADER;          // PLCP Header without CRC: SIGNAL+SERVICE+LENGTH
    wiUWORD Z;               // shift register for CRC calculation
    wiUWORD SFD;             // start frame delimiter

    SFD.word = RATE.b2 ? 0x05CF : 0xF3A0;

    // ----------
    // SYNC Field
    // ----------
    if (RATE.b2) for (i=0; i< 56; i++) (a++)->word = 0;
    else         for (i=0; i<128; i++) (a++)->word = 1;

    // ---------------------------
    // Start Frame Delimiter (SFD)
    // ---------------------------
    if (pTX->UseForcedbSFD) SFD.word = pTX->ForcedbSFD.w16;
    for (i=0; i<16; i++)
        (a++)->word = (SFD.word >> i) & 1;

    // -----------------------------------
    // SIGNAL Field (Data Rate (100 kbps))
    // -----------------------------------
    switch (RATE.w2)
    {
        case 0: SIGNAL.word = 0x0A; break;
        case 1: SIGNAL.word = 0x14; break;
        case 2: SIGNAL.word = 0x37; break;
        case 3: SIGNAL.word = 0x6E; break;
    }
    // -------------
    // SERVICE Field
    // -------------
    r = (8*NumByte)%11;
    SERVICE.word = 0x00;
    SERVICE.b2 = 1;                               // locked clocks
    SERVICE.b3 = ModSel? 1:0;                     // WiSE ONLY --- Set to 0 in RTL
    SERVICE.b7 = (RATE.w2==3 && r>0 && r<4)? 1:0; // length extension bit

    // ----------------------------------------------------
    // LENGTH Field (Time Duration of PSDU in microseconds)
    // ----------------------------------------------------
    n = 8*NumByte; // number of bits in PSDU
    switch (RATE.w2)
    {
        case 0: LENGTH.word =   n;   break;
        case 1: LENGTH.word =   n/2; break;
        case 2: LENGTH.word = 2*n/11 + ((2*n)%11!=0 ? 1:0); break;
        case 3: LENGTH.word =   n/11 + (n%11!=0 ? 1:0);     break;
    }
    
    if (pTX->UseForcedbSIGNAL ) SIGNAL.word  = pTX->ForcedbSIGNAL.w8;
    if (pTX->UseForcedbSERVICE) SERVICE.word = pTX->ForcedbSERVICE.w8;
    if (pTX->UseForcedbLENGTH ) LENGTH.word  = pTX->ForcedbLENGTH.w16;
    
    HEADER.word = (LENGTH.w16 << 16) | (SERVICE.w8 << 8) | (SIGNAL.w8);
    for (i=0; i<32; i++)
        (a++)->word = (HEADER.w32>>i) & 1;

    // -----------------
    // CRC Field (16bit)
    // -----------------
    Z.word = 0xFFFF;
    for (i=0; i<32; i++)
    {
        b = ((HEADER.w32>>i) &1) ^ Z.b15;
        Z.word = (Z.w16<<1) & 0xFFFF; // 16 bit shift register
        Z.b12  = (b ^ Z.b12);
        Z.b5   = (b ^ Z.b5);
        Z.b0   =  b;
    }
    if (pTX->UseForcedbCRC) Z.word = pTX->ForcedbCRC.w16;
    for (i=15; i>=0 ; i--)
        (a++)->word = ((Z.w16>>i) & 1) ^ 1;

    // ----------
    // PSDU Field
    // ----------
    for (i=0; i<NumByte; i++) {
        for (k=0; k<8; k++)
            (a++)->word = (PSDU[i].word >> k) & 1;   // LSB first
    }
}
// end of Verona_bTX_DataGenerator()

// ================================================================================================
// FUNCTION  : Verona_bTX_Scrambler()
// ------------------------------------------------------------------------------------------------
// Purpose   : TX Scrambler for the data bit stream
// Parameters: RATE - rate field from PHY header
//             Na   - number of bits (input and output)
//             a    - input serial bits
//             b    - output serial bits
// ================================================================================================
void Verona_bTX_Scrambler(wiUWORD RATE, int Na, wiUWORD *a, wiUWORD *b)
{
    int k; wiUWORD X;

    X.word = RATE.b2 ? 0x6C : 0x1B; // initialize scrambler based on preamble type

    for (k=0; k<Na; k++)
    {
        b[k].word = a[k].b0 ^ (X.b3) ^ (X.b6);
        X.word = ((X.w7<<1) & 0x7E) | b[k].b0;
    }
}
// end of Verona_bTX_Scrambler()

// ================================================================================================
// FUNCTION  : Verona_bTX_DBPSK()
// ------------------------------------------------------------------------------------------------
// Purpose   : DBPSK Modulation
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void Verona_bTX_DBPSK(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    const wiUInt Barker[11] = {0,1,0,0,1,0,0,0,1,1,1}; // Barker sequence in multiples of pi
    int i,j,k; // i=symbol index, j=chip index, k=sample index

    for (i=k=0; i<N; i++)
    {
        pp->b1 = pp->b1 ^ b[i].b0; // differential phase modulation: 180 degree shift

        // spreading by Barker sequence: p = pp + (Barker<<1)
        // mapping to constellation: 0=(+1+j), 1=(-1+j), 2=(-1-j), 3=(+1-j)
        for (j=0; j<11; j++, k++)
        {
            cReal[k].word = 1 ^ Barker[j] ^ pp->b1 ^ pp->b0;
            cImag[k].word = 1 ^ Barker[j] ^ pp->b1;
        }
    }
}
// end of Verona_bTX_DBPSK()

// ================================================================================================
// FUNCTION  : Verona_b_DQPSK_Encoding()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return DQPSK Encoding phase change (units of pi/2) for two data bits
// Parameters: b0 -- two-bit LSB (first bit in time)
//             b1 -- two-bit MSB (second bit in time)
// ================================================================================================
__inline int Verona_b_DQPSK_Encoding(wiUWORD b0, wiUWORD b1)
{
    const int PhaseChange[4] = {0,1,3,2};
    return PhaseChange[b0.b0<<1 | b1.b0];
}

// ================================================================================================
// FUNCTION  : Verona_bTX_DQPSK()
// ------------------------------------------------------------------------------------------------
// Purpose   : DQPSK Modulation
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void Verona_bTX_DQPSK(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    const wiUInt Barker[11] = {0,1,0,0,1,0,0,0,1,1,1}; // Barker sequence in multiples of pi
    int i, j, k, n; // i=symbol index, j=chip index, k=sample index, n=bit index, p=chip phase

    for (i=k=n=0; i<N; i++, n+=2)
    {
        pp->w2 += Verona_b_DQPSK_Encoding(b[n], b[n+1]); // differential phase modulation

        // spreading by Barker sequence: p = pp + (Barker<<1)
        // mapping to constellation: 0=(+1+j), 1=(-1+j), 2=(-1-j), 3=(+1-j)
        for (j=0; j<11; j++, k++)
        {
            cReal[k].word = 1 ^ Barker[j] ^ pp->b1 ^ pp->b0;
            cImag[k].word = 1 ^ Barker[j] ^ pp->b1;
        }
    }
}
// end of Verona_bTX_DQPSK()

// ================================================================================================
// FUNCTION  : Verona_bTX_CCK()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Codeword Generation
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void Verona_bTX_CCK(wiUWORD p1, wiUWORD p2, wiUWORD p3, wiUWORD p4, wiUWORD *cReal, wiUWORD *cImag)
{
    wiUWORD p[8];   // phases for each of 8 chips in a code
    int k;

    p[0].w2 = (p1.w2 + p2.w2 + p3.w2 + p4.w2    );
    p[1].w2 = (p1.w2         + p3.w2 + p4.w2    );
    p[2].w2 = (p1.w2 + p2.w2         + p4.w2    );
    p[3].w2 = (p1.w2                 + p4.w2 + 2);
    p[4].w2 = (p1.w2 + p2.w2 + p3.w2            );
    p[5].w2 = (p1.w2         + p3.w2            );
    p[6].w2 = (p1.w2 + p2.w2                 + 2);
    p[7].w2 = (p1.w2                            );

    // QPSK Modulation: x ~ p*(pi/2) + pi/4
    // p = {0,1,2,3} --> x = [{(+1+j), (-1+j), (-1-j), (+1-j)} + 1]/2
    for (k=0; k<8; k++)
    {
        cReal[k].word = 1 ^ p[k].b1 ^ p[k].b0;
        cImag[k].word = 1 ^ p[k].b1;
    }
}
// end of Verona_bTX_CCK()

// ================================================================================================
// FUNCTION  : Verona_bTX_CCK55()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Modulation at 5.5 Mbps
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void Verona_bTX_CCK55(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    int i;            // symbol index
    int k;            // sample index
    int n;            // bit index
    wiUWORD p2,p3,p4;   // component phases in multiples of pi/2

    for (i=k=n=0; i<N; i++, n+=4, k+=8)
    {
        pp->w2+= Verona_b_DQPSK_Encoding(b[n], b[n+1]); // DQPSK for common phase
        pp->b1 = pp->b1 ^ (i&1);                       // reverse phase on odd symbols

        p2.w2 = b[n+2].b0? 3:1;
        p3.w2 = 0;
        p4.w2 = b[n+3].b0? 2:0;

        Verona_bTX_CCK(*pp, p2, p3, p4, cReal+k, cImag+k);
    }
}
// end of Verona_bTX_CCK55

// ================================================================================================
// FUNCTION  : Verona_bTX_CCK110()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Modulation at 11 Mbps
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void Verona_bTX_CCK110(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    int i;            // symbol index
    int k;            // sample index
    int n;            // bit index
    wiUWORD p2,p3,p4;      // component phases in multiples of pi/2

    for (i=k=n=0; i<N; i++, n+=8, k+=8)
    {
        pp->w2+= Verona_b_DQPSK_Encoding(b[n], b[n+1]); // DQPSK for common phase
        pp->b1 = pp->b1 ^ (i&1);                       // reverse phase on odd symbols

        p2.w2 = b[n+2].b0<<1 | b[n+3].b0;
        p3.w2 = b[n+4].b0<<1 | b[n+5].b0;
        p4.w2 = b[n+6].b0<<1 | b[n+7].b0;

        Verona_bTX_CCK(*pp, p2, p3, p4, cReal+k, cImag+k);
    }
}
// end of Verona_bTX_CCK110

// ================================================================================================
// FUNCTION  : Verona_bTX_Modulation
// ------------------------------------------------------------------------------------------------
// Purpose   : DSSS/CCK Modulator
// Parameters: RATE    -- RATE field from PHY header
//             NumByte -- Length of PSDU (Bytes)
//             b       -- Serial bit stream input
//             cReal   -- Output transmit signal, real, 11 MHz
//             cImag   -- Output transmit signal, imag, 11 MHz
// ================================================================================================
void Verona_bTX_Modulation(wiUWORD RATE, int NumByte, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag)
{
    wiUWORD phase = {0}; // 2-bit symbol phase {0->pi/4, 1->3pi/4, 2->5pi/4, 3->7pi/4}

    // ---------------------
    // Modulation: Preamble
    // ---------------------
    Verona_bTX_DBPSK((RATE.b2? 72:144), b, cReal, cImag, &phase);
    b     +=     RATE.b2? 72:144;
    cReal += 11*(RATE.b2? 72:144);
    cImag += 11*(RATE.b2? 72:144);

    // -------------------
    // Modulation: Header
    // -------------------
    if (RATE.b2) Verona_bTX_DQPSK(24, b, cReal, cImag, &phase); // short preamble
    else         Verona_bTX_DBPSK(48, b, cReal, cImag, &phase); // long preamble

    b += 48;
    cReal += 11 *(RATE.b2? 24:48);
    cImag += 11 *(RATE.b2? 24:48);

    // -----------------
    // Modulation: PSDU
    // -----------------
    switch (RATE.w2)
    {
        case 0: Verona_bTX_DBPSK (8*NumByte,   b, cReal, cImag, &phase); break;
        case 1: Verona_bTX_DQPSK (8*NumByte/2, b, cReal, cImag, &phase); break;
        case 2: Verona_bTX_CCK55 (8*NumByte/4, b, cReal, cImag, &phase); break;
        case 3: Verona_bTX_CCK110(8*NumByte/8, b, cReal, cImag, &phase); break;
    }
}
// end of Verona_bTX_Modulation()

// ================================================================================================
// FUNCTION  : Verona_bTX_PulseFilter_FIR
// ------------------------------------------------------------------------------------------------
// Purpose   : Pulse Filter FIR Element. Filter is implemented as a lookup table using
//             the relavent binary inputs to form the address/index.
//             Impulse response at 44 MHz: h={-4,-9,0,41,115,191,224,191,115,41,0,-9,-4}
// Parameters: Nc -- Number of input samples
//             c  -- Binary input array corresponding to +/- pulses (use zero outside c)
//             x  -- Upsampled output, 9 bit
// ================================================================================================
void Verona_bTX_PulseFilter_FIR(int Nc, wiUWORD *c, wiWORD *x)
{
    const int h0[] = {-4, 115, 115, -4};
    const int h1[] = {-9, 191,  41,  0};
    const int h2[] = { 0, 224,   0,  0}; // filter taps: primary phase
    const int h3[] = {41, 191,  -9,  0};

    wiWORD v[4] = {{0}};  // FIR output accumulators
    wiWORD d[4] = {{0}};  // filter delay line
    int Nx = 4*Nc + 12; // number of non-zero output terms
    int i,k;

    // ------------------------
    // for loop on 44 MHz clock
    // ------------------------
    for (k = 0; k < Nx; k++)
    {
        // --------------------
        // Processing at 11 MHz
        // --------------------
        if (k%4 == 0)
        {
            for (i=0; i<4; i++) v[i].word = 0;             // clear accumulators
            d[3]=d[2]; d[2]=d[1]; d[1]=d[0];              // clock delay line
            d[0].word = (k<4*Nc)? (c[k/4].b0? 1:-1) : 0;  // load delay line input {+1,-1} during packet

            for (i=0; i<4; i++)
            {
                v[0].word = v[0].w9 + (h0[i] * d[i].w2);
                v[1].word = v[1].w9 + (h1[i] * d[i].w2);
                v[2].word = v[2].w9 + (h2[i] * d[i].w2);
                v[3].word = v[3].w9 + (h3[i] * d[i].w2);
            }
        }
        // ---------------------------------------
        // Select Output Term at 44 MHz Clock Rate
        // ---------------------------------------
        switch (k % 4)
        {
            case 0: x[k].word = v[0].w9; break;
            case 1: x[k].word = v[1].w9; break;
            case 2: x[k].word = v[2].w9; break;
            case 3: x[k].word = v[3].w9; break;
        }
    }
}
// end of Verona_bTX_PulseFilter_FIR()

// ================================================================================================
// FUNCTION  : Verona_bTX_PulseFilter
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit spectrum shaping and upsample from 11 to 44 MS/s
// Parameters: Nc    -- Number of input samples
//             cReal -- Filter input,  1-bit, real, 11 MHz
//             cImag -- Filter input,  1-bit, imag, 11 MHz
//             xReal -- Filter output, 9-bit, real, 44 MHz
//             xImag -- Filter output, 9-bit, imag, 44 MHz
// ================================================================================================
void Verona_bTX_PulseFilter(int Nc, wiUWORD *cReal, wiUWORD *cImag, wiWORD *xReal, wiWORD *xImag)
{
    Verona_bTX_PulseFilter_FIR(Nc, cReal, xReal);
    Verona_bTX_PulseFilter_FIR(Nc, cImag, xImag);
}
// end of Verona_bTX_PulseFilter()

// ================================================================================================
// FUNCTION  : Verona_bTX_Resample_Core
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit signal sample rate conversion from 44 to 40 MS/s
// Parameters: N     -- Number of input samples
//             x     -- Input signal,  9-bit, 44 MHz
//             y     -- Output signal, 8-bit, 40 MHz
// ================================================================================================
void Verona_bTX_Resample_Core(int N, wiWORD *x, wiWORD *y, int bResamplePhase)
{
    wiUWORD clockGate40 = {0xAAAAA}; // 80->40 MHz clock gating cadence
    wiUWORD clockGate44 = {0xAAB55}; // 80->44 MHz clock gating cadence
    wiBoolean posedgeCLK44MHz, posedgeCLK40MHz; // clock edges

    wiWORD q,r;                                  // intermediate result
    wiReg c = {{9},{9}};                         // counter (with initial value)
    wiReg d1={{0}}, d2={{0}}, d3={{0}}, z={{0}}; // input/output registers

    int k, k44 = 0; // counter on 44 MHz clock

    // -----------------------
    // Set Initial Clock Phase
    // -----------------------
    for (k=0; k<bResamplePhase; k++)
    {
        posedgeCLK40MHz = clockGate40.b19;

        clockGate44.w20 = (clockGate44.w19 << 1) | clockGate44.b19;
        clockGate40.w20 = (clockGate40.w19 << 1) | clockGate40.b19;

        if (posedgeCLK40MHz)
        {
            ClockRegister(c);
            c.D.word = (c.Q.word+1)%10;
        }
    }
    // -----------------------------------------------
    // Clock Generator at 80 MHz for N cycles @ 44 MHz
    // -----------------------------------------------
    while (k44 < N)
    {
        posedgeCLK44MHz = clockGate44.b19;
        posedgeCLK40MHz = clockGate40.b19;

        clockGate44.w20 = (clockGate44.w19 << 1) | clockGate44.b19;
        clockGate40.w20 = (clockGate40.w19 << 1) | clockGate40.b19;

        if (posedgeCLK44MHz || posedgeCLK40MHz) // process only on relevent clocks
        {  
            // Clock registers
            if (posedgeCLK44MHz)
            {
                ClockRegister(d3); ClockRegister(d2); ClockRegister(d1);
                d3.D = d2.Q;       d2.D = d1.Q;       d1.D.word = x[k44].w9; // delay line
                k44++;
            }
            if (posedgeCLK40MHz) 
            {
                ClockRegister(c);
                ClockRegister(z);
            }

            // linear interpolator with cycle delay mux
            if (c.Q.word<4) q.word = (c.Q.word*d1.Q.w9 + (10-c.Q.word)*d2.Q.w9)/4;
            else            q.word = (c.Q.word*d2.Q.w9 + (10-c.Q.word)*d3.Q.w9)/4;

            // Divide by 8 with rounding
            r.word = (q.b10==0 && q.b2==1)? 1 : (q.b10==1 && (q.w3>=1 || q.w3<=-4))? -1 : 0;
            z.D.word = q.w11/8 + r.w2;

            // increment phase counter
            c.D.word = (c.Q.word+1)%10;

            // Output Results @ 40 MHz
            if (posedgeCLK40MHz)
            {
                y->word = z.Q.w8;
                y++;
            }
        }
    }
}
// end of Verona_bTX_Resample_Core()

// ================================================================================================
// FUNCTION  : Verona_bTX_Resample
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit signal sample rate conversion from 44 to 40 MS/s
// Parameters: Nc    -- Number of input samples
//             xReal -- Input,  9-bit, real, 44 MHz
//             xImag -- Input,  9-bit, imag, 44 MHz
//             yReal -- Output, 8-bit, real, 40 MHz
//             yImag -- Output, 8-bit, imag, 40 MHz
//             phase -- Relative phase of 40/44 MHz clocks
// ================================================================================================
void Verona_bTX_Resample(int N, wiWORD *xReal, wiWORD *xImag, wiWORD *yReal, wiWORD *yImag, wiInt bResamplePhase)
{
    // If a negative bResamplePhase value is input, generate a random phase
    //
    if (bResamplePhase < 0)
        bResamplePhase = (int)(20.0 * wiRnd_UniformSample());

    Verona_bTX_Resample_Core(N, xReal, yReal, bResamplePhase);
    Verona_bTX_Resample_Core(N, xImag, yImag, bResamplePhase);
}
// end of Verona_bTX_Resample()

// ================================================================================================
// FUNCTION  : Verona_bTX_UpsampleFIR
// ------------------------------------------------------------------------------------------------
// Purpose   : Upsample Filter: 40 MHz --> 80 MHz
//             Filter Response: c = {-3, 0, 20, 34, 20, 0, -3}
// Parameters: Ny -- Number of output samples
//             x  -- Input,   8-bit, 40 MHz
//             y  -- Output, 11-bit, 80 MHz
// ================================================================================================
void Verona_bTX_UpsampleFIR(int Ny, wiWORD *x, wiWORD *y)
{
    const int c0[5] = { 0,  0, 34,  0}; // filter taps: main phase
    const int c1[5] = {-3, 20, 20, -3}; // filter taps: intermediate phase
    wiWORD d[4] = {{0}};                // filter delay line
    wiWORD u[2] = {{0}};                // filter accumulators/output terms
    wiWORD z[2] = {{0}};                // filter accumulators/output terms
    wiWORD r[2] = {{0}};                // remainder for rounding in output divider
    int i,k;

    // ------------------------
    // for loop on 80 MHz clock
    // ------------------------
    for (k=0; k<Ny; k++, y++)
    {
        // --------------------
        // Processing at 40 MHz
        // --------------------
        if (k % 2 == 0)
        {
            u[0].word = u[1].word = 0;       // clear FIR accumulators
            d[3]=d[2]; d[2]=d[1]; d[1]=d[0]; // clock shift register
            d[0].word = x->w8;               // register input

            for (i=0; i<4; i++)               // FIR multiply+accumulate
            {
                u[0].word = u[0].w13 + c0[i]*d[i].w8; // u is 13bits because
                u[1].word = u[1].w13 + c1[i]*d[i].w8; // 1+ceil(log2(241*10/32*(3+20+20+3)) = 13
            }

            // Compute residual term required for rounding in divider
            r[0].word = (u[0].b12==0 && u[0].b3==1)? 1 : (u[0].b12==1 && (u[0].w4>=1 || u[0].w4<=-8))? -1 : 0;
            r[1].word = (u[1].b12==0 && u[1].b3==1)? 1 : (u[1].b12==1 && (u[1].w4>=1 || u[1].w4<=-8))? -1 : 0;

           // Divide-by-16 with rounding
           z[0].word = u[0].w13/16   + r[0].w2;
           z[1].word = u[1].w13/16   + r[1].w2;
            
            x++; // clock input at 40 MHz
        }
        // ----------------------------------------------------------------------------
        // Processing at 80 MHz
        // The required resolution for z is 9-bit, but it is expanded to match the OFDM
        // Upmixer input resolution
        // ----------------------------------------------------------------------------
        switch (k%2)
        {
            case 0: y->word = z[0].w11; break;
            case 1: y->word = z[1].w11; break;
        }
    }
}
// end of Verona_bTX_UpsampleFIR()

// ================================================================================================
// FUNCTION  : Verona_bTX_Upsample
// ------------------------------------------------------------------------------------------------
// Purpose   : Upsample: 40 MHz --> 80 MHz
// Parameters: Nx -- Number of input samples
//             xReal  -- Input, real, 8-bit, 40 MHz
//             xImag  -- Input, real, 8-bit, 40 MHz
//             yReal  -- Output,real,11-bit, 80 MHz
//             yImag  -- Output,imag,11-bit, 80 MHz
// ================================================================================================
void Verona_bTX_Upsample(int Nx, wiWORD *xReal, wiWORD *xImag, wiWORD *yReal, wiWORD *yImag)
{
    Verona_bTX_UpsampleFIR(Nx, xReal, yReal);
    Verona_bTX_UpsampleFIR(Nx, xImag, yImag);
}
// end of Verona_bTX_Upsample()

// ================================================================================================
// FUNCTION  : Verona_bTX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level DSSS/CCK TX routine
// Parameters: RATE     -- Data rate field from PHY Header
//             LENGTH   -- Number of PSDU bytes (in MAC_TX_D)
//             MAC_TX_D -- PSDU transferred from MAC (not including header)
//             Nx       -- Number of transmit samples (by reference)
//             x        -- Transmit signal array/DAC output (by reference)
//             M        -- Oversample rate for analog signal
// ================================================================================================
wiStatus Verona_bTX(wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M) 
{
    int NsPSDU, Na, Nc, dv;

    Verona_TxState_t *pTX  = Verona_TxState();
    VeronaRegisters_t *pReg = Verona_Registers();

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!MAC_TX_D)                 return STATUS(WI_ERROR_PARAMETER3);
    if (!Nx)                       return STATUS(WI_ERROR_PARAMETER4);
    if (!x)                        return STATUS(WI_ERROR_PARAMETER5);
    if (InvalidRange(M, 1, 40000)) return STATUS(WI_ERROR_PARAMETER6);

    // -------------------------------------------------
    // Calculate Data Rate/Preamble Dependent Parameters
    // -------------------------------------------------
    switch (RATE.w4)
    {
        case 0: NsPSDU = 11*(8*LENGTH.w12)/1; break;
        case 1: NsPSDU = 11*(8*LENGTH.w12)/2; break;
        case 2: NsPSDU =  8*(8*LENGTH.w12)/4; break;
        case 3: NsPSDU =  8*(8*LENGTH.w12)/8; break;
        case 5: NsPSDU = 11*(8*LENGTH.w12)/2; break;
        case 6: NsPSDU =  8*(8*LENGTH.w12)/4; break;
        case 7: NsPSDU =  8*(8*LENGTH.w12)/8; break;
        default:
            pReg->TxFault.word = 1;
            STATUS(WI_ERROR_UNDEFINED_CASE);
            return WI_SUCCESS;
    }

    if (LENGTH.w12 == 0) pReg->TxFault.word = 1;

    // -----------------------------------------------------
    // Calculate Timers/Signal Durations and Allocate Memory
    // -----------------------------------------------------
    Na = 8*(LENGTH.w12) + (RATE.b2? 120:192);          // number of bits including preamble
    Nc = NsPSDU + (RATE.b2? 11*(72+24) : 11*(144+48)); // number of samples at 11 MHz
    
    pTX->PacketDuration = (double)Nc / 11.0e6;

    XSTATUS(Verona_bTX_AllocateMemory(Na, Nc, M) );

    // ---------------------------------------------------------------
    // Generate bit sequence from PSDU bytes, preamble type, data rate
    // ---------------------------------------------------------------
    Verona_bTX_DataGenerator(RATE, LENGTH.w12, MAC_TX_D, pTX->a, pTX->SetPBCC);
    Verona_bTX_Scrambler    (RATE, Na, pTX->a, pTX->b);
    Verona_bTX_Modulation   (RATE, LENGTH.w12, pTX->b, pTX->cReal, pTX->cImag);

    // -----------------------------
    // Transmit pulse-shaping filter
    // -----------------------------
    dv = 4*pTX->bPadFront + 9; // +9 accounts for pipeline delays
    pTX->OfsEOP = M*(15*8 + 18 + 80*(Nc + pTX->bPadFront))/11; // approximate end-of-packet
    Verona_bTX_PulseFilter(Nc, pTX->cReal, pTX->cImag, pTX->vReal+dv, pTX->vImag+dv);

    // -------------------
    // Resample and Output
    // -------------------
    Verona_bTX_Resample(pTX->Nv, pTX->vReal, pTX->vImag, pTX->xReal, pTX->xImag, pTX->bResamplePhase);
    Verona_bTX_Upsample(pTX->Ny, pTX->xReal, pTX->xImag, pTX->yReal, pTX->yImag);
    Verona_Upmix       (pTX->Ny, pTX->yReal, pTX->yImag, pTX->zReal, pTX->zImag);
    Verona_TxIQSource  (pTX->Nz, pTX->zReal, pTX->zImag, pTX->qReal, pTX->qImag);
    Verona_TxIQ        (pTX->Nz, pTX->qReal, pTX->qImag, pTX->rReal, pTX->rImag);
    Verona_Prd         (pTX->Nz, pTX->rReal, pTX->rImag, pTX->uReal, pTX->uImag);
    Verona_DAC         (pTX->Nz, pTX->uReal, pTX->uImag, pTX->d, M);
 
    *Nx = pTX->Nd;
    *x  = pTX->d;

    return WI_SUCCESS;
}
// end of Verona_bTX()

// ================================================================================================
// FUNCTION  : Verona_bTX_ResampleToDAC()
// ------------------------------------------------------------------------------------------------
// Purpose   : Resample and apply subsequent DSP to the DAC. This function is used to post-process
//             an existing packet waveform after changing bResamplePhase.
// ================================================================================================
wiStatus Verona_bTX_ResampleToDAC(void)
{
    Verona_TxState_t  *pTX  = Verona_TxState();

    int M = pTX->Nd / pTX->Nz;

    if (!pTX->vReal) return STATUS(WI_ERROR); // no data in arrays

    Verona_bTX_Resample(pTX->Nv, pTX->vReal, pTX->vImag, pTX->xReal, pTX->xImag, pTX->bResamplePhase);
    Verona_bTX_Upsample(pTX->Ny, pTX->xReal, pTX->xImag, pTX->yReal, pTX->yImag);
    Verona_Upmix       (pTX->Ny, pTX->yReal, pTX->yImag, pTX->zReal, pTX->zImag);
    Verona_TxIQSource  (pTX->Nz, pTX->zReal, pTX->zImag, pTX->qReal, pTX->qImag);
    Verona_TxIQ        (pTX->Nz, pTX->qReal, pTX->qImag, pTX->rReal, pTX->rImag);
    Verona_Prd         (pTX->Nz, pTX->rReal, pTX->rImag, pTX->uReal, pTX->uImag);
    Verona_DAC         (pTX->Nz, pTX->uReal, pTX->uImag, pTX->d, M);
 
    return WI_SUCCESS;
}
// end of Verona_bTX_ResampleToDAC()

// ================================================================================================
// FUNCTION  : Verona_bRX_Filter
// ------------------------------------------------------------------------------------------------
// Purpose   : DSSS/CCK Receive Channel Select/Anti-Alias Filter
// Parameters: n -- filter number (model input, specifies module instance)
//             x -- input,  10 bit
//             y -- output, 11 bit
// ================================================================================================
void Verona_bRX_Filter(int n, wiWORD *x, wiWORD *y)
{
    const int h[14]={-1,-2,-2, 1, 6, 12, 16, 16, 12, 6, 1,-2,-2,-1};

    wiWORD *d = &(Verona_bRxState()->dRxFilter[n][0]);
    int i;

    // --------------------
    // Clock the Delay Line
    // --------------------
    for (i=15; i>0; i--) 
        d[i] = d[i-1];
    d[0].word = x->w10;

    // ----------------------
    // FIR Filter Computation
    // ----------------------
    y->word = 0;
    for (i=0; i<14; i++)
        y->word = y->w17 + h[i]*d[i+2].w10;

    y->word = y->w17/32;
    y->word = limit(y->word, -1023, 1023);
}   
// end of Verona_bRX_Filter()

// ================================================================================================
// FUNCTION  : Verona_bRX_DigitalGain
// ------------------------------------------------------------------------------------------------
// Purpose   : Digital Gain Stage: x = q * 2^(DGain/4), Gain Range: 1/16 to 13.45
// Parameters: qReal -- Input, real, 11-bit
//             qImag -- Input, imag, 11-bit
//             xReal -- Input, real,  8-bit
//             xImag -- Input, imag,  8-bit
//             DGain -- Gain, (1.5dB steps), signed, 5-bit
// ================================================================================================
void Verona_bRX_DigitalGain(wiWORD *qReal, wiWORD *qImag, wiWORD *xReal, wiWORD *xImag, wiWORD DGain)
{
    const wiUWORD AA[4] = {{0x20}, {0x26}, {0x2D}, {0x36}}; // multipliers for 32 * 2^({0,1,2,3}/4)

    wiUWORD A, u; // gain
    wiWORD  p, q; // intermediate calculations
    unsigned   n; // shift term

    // ---------------------
    // Select the multiplier
    // ---------------------
    u.word = DGain.w5 + 16; // convert to unsigned
    A = AA[u.w2];           // gain term = 32 * 2^({0,1,2,3}/4)
    n = 12 - (u.w6>>2);     // shift term: 2^(3-n)

    // ----------
    // Gain Stage
    // ----------
    p.word = ((qReal->w11 * A.w6) << 3) >> n; // multiplier output
    q.word = ((qImag->w11 * A.w6) << 3) >> n; // multiplier output

    // ----------------------------------
    // Limit Output Range to [-127, +127]
    // ----------------------------------
    xReal->word = limit(p.w16,-127,127);
    xImag->word = limit(q.w16,-127,127);

}
// end of Verona_bRX_DigitalGain

// ================================================================================================
// FUNCTION  : Verona_bRX_Interpolate()
// ------------------------------------------------------------------------------------------------
// Purpose   : Resample from 40 MHz to 44 MHz with DPLL NCO
// Parameters: LPFOutR    -- input from LPF output, real, 8b, 40MHz
//             LPFOutI    -- input from LPF output, imag, 8b, 40MHz
//             IntpOutR   -- output, real, 8b, 44MHz
//             IntpOutI   -- output, imag, 8b, 44MHz
//             cSP        -- sample phase correction from DPLL (implied input register)
//             PLLEnB     -- enable control from the DPLL
//             TransCCK   -- indicate buffering for transition from DSSS to CCK
//             INTERPinit -- initialize resampler (at reset and end of every packet)
//             IntpFreeze -- indicates interpolator output is stopped (frozen)
// ================================================================================================
void Verona_bRX_Interpolate(wiWORD *LPFOutR, wiWORD *LPFOutI, wiWORD *IntpOutR, wiWORD *IntpOutI,
                            wiWORD cSP, wiBoolean PLLEnB, wiBoolean TransCCK, wiBoolean INTERPinit,
                            wiBoolean *IntpFreeze, Verona_RxState_t *pRX)
{
    const wiUInt LUT32div11[16] = {0, 3, 6, 9, 12, 15, 17, 20, 23, 26, 29}; // = round(index*32/11)

    Verona_bRX_Interpolate_t *pState = &(Verona_bRxState()->Interpolate);

    // ---------------
    // Clock Registers
    // ---------------
    if (pRX->clock.posedgeCLK40MHz)
    {
        ClockRegister(pState->dLPFOutR); ClockRegister(pState->d2LPFOutR);
        ClockRegister(pState->dLPFOutI); ClockRegister(pState->d2LPFOutI);
    }
    if (pRX->clock.posedgeCLK44MHz)
    {
        ClockRegister(pState->p);
        ClockRegister(pState->n);
        ClockRegister(pState->yR);
        ClockRegister(pState->yI);
        ClockRegister(pState->NCO);
        ClockDelayLine(pState->dTransCCK, TransCCK);
        ClockDelayLine(pState->dIntpFreeze, 0);
    }
    // -----------------------------------------------
    // Initialization 
    // (occurs at RX startup and after each packet RX)
    // -----------------------------------------------
    if (INTERPinit)
    {
        pState->m.word = 1;
        pState->k44n.word = 0;
        pState->k44p.word = 0;
        pState->p.D.word = pState->p.Q.word = 0;
        pState->n.D.word = pState->n.Q.word = 0;
        
        pState->yR.D.word = pState->yR.Q.word = 0;
        pState->yI.D.word = pState->yI.Q.word = 0;

        pState->dIntpFreeze.word = 0;
        pState->dTransCCK.word = 0;
        pState->NCO.D.word = 0;

        pState->b44p.w5 = 10;
        pState->b44n.w7 = 0;

        return;
    }
    // -----------------------------
    // Buffer Input Signal at 40 MHz
    // -----------------------------
    if (pRX->clock.posedgeCLK40MHz)
    {
        pState->dLPFOutR.D.word = LPFOutR->w8;
        pState->dLPFOutI.D.word = LPFOutI->w8;
        pState->d2LPFOutR.D = pState->dLPFOutR.Q;
        pState->d2LPFOutI.D = pState->dLPFOutI.Q;

        pState->m.w7++; // increment RAM input address (modulo-128 counter) - free running 40 MHz clock

        pState->RAM[pState->m.w7].xReal  =  pState->dLPFOutR.Q.w8;
        pState->RAM[pState->m.w7].dxReal = pState->d2LPFOutR.Q.w8;
        pState->RAM[pState->m.w7].xImag  =  pState->dLPFOutI.Q.w8;
        pState->RAM[pState->m.w7].dxImag = pState->d2LPFOutI.Q.w8;
    }
    // -------------------------------------------
    // Oscillator (NCO) and Interpolator at 44 MHz
    // -------------------------------------------
    if (pRX->clock.posedgeCLK44MHz)
    {
        wiUWORD b44; // NCO for sample delay
        wiUWORD k44;
        wiWORD  a44; // phase adjustment to 44 MHz sample timing
        wiWORD  d;

        // NCO for phase error between TX and RX clocks
        if (PLLEnB)
        {
            a44.word = 29;
        }
        else
        {
            // -----------------------------------------------------
            // Output Freeze for either TransCCK or Buffer Underflow
            // -----------------------------------------------------
            if (pState->dTransCCK.delay_1T || pState->dIntpFreeze.delay_1T)
            {
                pState->b44n.w7 += (pState->b44p.w5 >= 1) ?  1:0;
                pState->b44p.w5 += (pState->b44p.w5 >= 1) ? -1:10;
            }
            // ------------------------------------------------------------
            // Phase Correction From DPLL and Output Freeze/TransCCK Events
            // ------------------------------------------------------------
            b44.w13 = (pState->b44n.w7<<5) + LUT32div11[pState->b44p.w4]; // b44 = 32*b44n + round(32*b44p/11)
            a44.w14 = b44.w13 + cSP.w14;                     // total phase adjustment

            pState->dIntpFreeze.delay_0T = a44.w14 < 0 ? 1:0; // flag DPLL increase in NCO (--> buffer underflow)
        }
        // ------------------------------
        // Free-Running 44 MHz Oscillator
        // ------------------------------
        pState->k44n.w7 += (pState->k44p.w5 >= 1) ?  1 : 0;
        pState->k44p.w5 += (pState->k44p.w5 >= 1) ? -1 : 10;
        k44.w13 = (pState->k44n.w7 << 5) + LUT32div11[pState->k44p.w4]; // k44 = 32*k44n + round(32*k44p/11)

        // ----------------------------------------
        // Combine All Oscillator/Phase Corrections
        // ----------------------------------------
        d.w15 = k44.w13 - a44.w14;         // combine free-running counter with adjustment
        pState->NCO.D.w15 = (d.w15 + 4096) % 4096; // modulo-(32x128)

        pState->n.D.w8 = pState->NCO.Q.w13 >> 5; // integer part (RAM address)
        pState->p.D.w3 = pState->NCO.Q.w5  >> 2; // fractional part (interpolator weight)

        // -------------------------------------------------------------------
        // Linear Interpolation y[k] = ((8-p)*x[k-1] + p*x[k])/8
        // n.Q models RAM read delay, p.Q is an actual register delay to match
        // -------------------------------------------------------------------
        pState->yR.D.word = (pState->RAM[pState->n.Q.w8].dxReal * (8-(signed)pState->p.Q.w3) 
                          + (pState->RAM[pState->n.Q.w8].xReal  *    (signed)pState->p.Q.w3)) / 8;
        pState->yI.D.word = (pState->RAM[pState->n.Q.w8].dxImag * (8-(signed)pState->p.Q.w3) 
                          + (pState->RAM[pState->n.Q.w8].xImag  *    (signed)pState->p.Q.w3)) / 8;
    }
    // -------------
    // Trace Signals
    // -------------
    if (Verona_RxState()->EnableTrace)
    {
        Verona_bRxState_t *pbRX = Verona_bRxState();

        pbRX->traceResample[pRX->k80].posedgeCLK40MHz = pRX->clock.posedgeCLK40MHz;
        pbRX->traceResample[pRX->k80].posedgeCLK44MHz = pRX->clock.posedgeCLK44MHz;
        pbRX->traceResample[pRX->k80].posedgeCLK22MHz = pRX->clock.posedgeCLK22MHz;
        pbRX->traceResample[pRX->k80].posedgeCLK11MHz = pRX->clock.posedgeCLK11MHz;
        pbRX->traceResample[pRX->k80].INTERPinit      = INTERPinit;
        pbRX->traceResample[pRX->k80].m               = pState->m.w7;
        pbRX->traceResample[pRX->k80].NCO             = pState->NCO.Q.w13;
    }
    // ------------
    // Wire Outputs
    // ------------
    IntpOutR->word = pState->yR.Q.w8;
    IntpOutI->word = pState->yI.Q.w8;
    *IntpFreeze    = pState->dIntpFreeze.delay_3T;
}
// end of Verona_bRX_Interpolate()

// ================================================================================================
// FUNCTION  : Verona_b_BarkerCorrelator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Cross correlation of the input with the Barker Sequence
// Parameters: BCInR22 -- BC input,  real,  8-bit, 22 MHz
//             BCInI22 -- BC input,  imag,  8-bit, 22 MHz
//             BCInR11 -- BC input,  real,  8-bit, 11 MHz
//             BCInI11 -- BC input,  imag,  8-bit, 11 MHZ
//             BCOutR  -- BC output, real,  9-bit
//             BCOutI  -- BC output, imag,  9-bit
//             BCMode  -- input sample selection {0=22MHz, 1=11MHz}
//             BCEnB   -- correlator enable (active low)
// ================================================================================================
void Verona_b_BarkerCorrelator( wiWORD *BCInR22, wiWORD *BCInI22, wiWORD *BCInR11, wiWORD *BCInI11,
                                wiWORD *BCOutR, wiWORD *BCOutI, int BCMode, int BCEnB )
{
    const int Barker[11] = {0,0,0,1,1,1,0,1,1,0,1}; // Time-reversed Barker sequence

    Verona_bBarkerCorrelator_t *pState = &(Verona_bRxState()->BarkerCorrelator);
    wiWORD *dR = pState->dR;
    wiWORD *dI = pState->dI;

    wiWORD aReal, aImag;                 // accumulators for correlator
    int     i, j;

    // ------------------------
    // clock explicit registers
    // ------------------------
    ClockRegister(pState->rReal); 
    ClockRegister(pState->rImag);
    ClockRegister(pState->RegBCEnB);
    pState->RegBCEnB.D.word = BCEnB;
    pState->TFF = !pState->TFF;

    // --------------------------
    // initialize/clear registers
    // --------------------------
    if (pState->RegBCEnB.Q.b0 && !BCEnB) {
        for (i=0; i<25; i++)   dR[i].word=dI[i].word=0;
        pState->TFF = 0;
    }
    if (pState->RegBCEnB.Q.b0) pState->counter.word = 0; // clear counter

    // -----------------------------
    // Barker Correlator Calculation
    // -----------------------------
    if (!pState->RegBCEnB.Q.b0)
    {
        for (i=24; i>0; i--) dR[i] = dR[i-1]; // clock delay lines
        for (i=24; i>0; i--) dI[i] = dI[i-1]; //   "     "     "

        dR[0].word = BCMode ? BCInR11->w8 : BCInR22->w8; // select input
        dI[0].word = BCMode ? BCInI11->w8 : BCInI22->w8; //   "      "
        aReal.word = aImag.word = 0;                  // clear accumulators

        if (pState->counter.w5>=22 && (!BCMode || pState->TFF)) // wait until delay line has been filled
        {                                                       //   before starting correlator output
            for (i=0; i<11; i++) 
            {
                j = 2*(i+1);
                aReal.word = aReal.w11 + (Barker[i] ? dR[j].w8 : -dR[j].w8);
                aImag.word = aImag.w11 + (Barker[i] ? dI[j].w8 : -dI[j].w8);
            }
            pState->rReal.D.word = limit(aReal.w11 >> 1, -255, 255); // ~divide-by-2 and limit to 9 bits
            pState->rImag.D.word = limit(aImag.w11 >> 1, -255, 255);
        }
        pState->counter.word = (pState->counter.w5==31)? 31:pState->counter.w5+1; // increment counter to 31
    }
    // -----------------------------
    // output registered correlation
    // -----------------------------
    BCOutR->word = pState->rReal.Q.w9;
    BCOutI->word = pState->rImag.Q.w9;
}
// end of Verona_b_BarkerCorrelator()

// ================================================================================================
// FUNCTION  : Verona_b_EnergyDetect()
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate the signal energy (power)
// Parameters: zReal     -- input, real, 8-bit, 22 MHz
//             zImag     -- input, imag, 8-bit, 22 MHz
//             EDEnB     -- energy detect enable (active low)
//             EDwindow  -- log2(number of symbols) for measurement window
//             EDOut     -- energy detect output, power of 2^(1/4)
// ================================================================================================
void Verona_b_EnergyDetect(wiWORD *EDInR, wiWORD *EDInI, int EDEnB, wiUWORD EDwindow, wiUReg *EDOut)
{
    Verona_bEnergyDetect_t *pState = &(Verona_bRxState()->EnergyDetect);

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(pState->dEDEnB);
    ClockRegister(pState->zReal);
    ClockRegister(pState->zImag);
    ClockRegister(pState->acc);
    ClockRegister(pState->count);
    ClockRegister(pState->EDdone);
    ClockRegister(pState->FirstMeasure)

    // ---------------
    // Register Inputs
    // ---------------
    pState->dEDEnB.D.word = EDEnB;
    pState->zReal.D.word  = EDEnB ? pState->zReal.Q.word : (EDInR->w8/2);
    pState->zImag.D.word  = EDEnB ? pState->zImag.Q.word : (EDInI->w8/2);

    // -------
    // Counter
    // -------
    pState->EDdone.D.word = pState->count.Q.w8 == (unsigned)22*(1 << EDwindow.w2);
    pState->count.D.word  = (pState->dEDEnB.Q.b0 || EDEnB || pState->EDdone.D.word) ? 0 : pState->count.Q.w8+1;
    pState->FirstMeasure.D.word = EDEnB? 1 : (pState->EDdone.D.word? 0:pState->FirstMeasure.Q.b0);

    // -----------------
    // Power Measurement
    // -----------------
    if (pState->dEDEnB.Q.b0 || EDEnB || pState->EDdone.D.b0) 
        pState->acc.D.w20 = 0; // initialization
    else 
        pState->acc.D.w20 = pState->acc.Q.w20 + (pState->zReal.Q.w7)*(pState->zReal.Q.w7) 
                                              + (pState->zImag.Q.w7)*(pState->zImag.Q.w7);

    // -------------------------------------------------------------
    // Convert Power Measurement to 2^(1/4) Format
    // With N symbols (22 samples/symbol) accumulated, tED = acc/16N
    // EDOut = (int)(4*log2(tED)+0.5-1.8); -1.8 for 4log2(16/22)
    // -------------------------------------------------------------
    if (EDEnB || !(pState->FirstMeasure.Q.b0 || pState->EDdone.D.b0))
    {
        EDOut->D = EDOut->Q;
    }
    else
    {
        wiUWORD tED, sig, exp;
        wiWORD  tail;
        int i;

        tED.word = pState->acc.Q.w20 >> (EDwindow.w2 + 4);
        tED.word = tED.b13 ? 8191 : tED.w13; // limit range to 13 bits (theoretical = 13.46b)

        if (tED.w13 < 2) // negative log output
            EDOut->D.w6 = 0;
        else
        {
            // ED->w6 = (int)(4.0*log((double)tED.w13)/log(2.0)+0.5-1.8);
            //          0.5 for rounding, -1.8 for 4*log2(16/22) correction
            sig.word = 0;  exp.word = 0;  tail.word = 0; 
            for (i=12; i>=0; i--) 
            { 
                if ((tED.w13 >> i) == 1) 
                { 
                    exp.w4 = i; 
                    sig.w5 = (i < 4) ? tED.w13<<(4-i) : tED.w13>>(i-4);
                    tail.w4 = (int)(4.0*log((double)sig.w5/16.0)/log(2.0) + 0.7) - 2;
                    if (sig.w5 == 20 || sig.w5 == 28) tail.w4++; // compensation
                    break; 
                } 
            } 
            EDOut->D.w6 = (tED.w13<2)? 0 : (unsigned)((signed)(exp.w4 << 2) + tail.w4); 
        }
    }
}
// end of Verona_b_EnergyDetect()

// ================================================================================================
// FUNCTION  : Verona_b_CorrelationQuality()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: BCOutR   -- Barker correlator output, real, input, 9b
//             BCOutI   -- Barker correlator output, imag, input, 9b
//             CQEnB    -- enable CQ module
//             CSEnB    -- carrier sensecarrier sense enable (active low)
//             CQwindow -- number of symbols for correlation quality, CSEnB=0
//             SSwindow -- number of symbols for correlation quality, CSEnB=1
//             CQOut    -- CQ output value, 13b, registered
//             CQPeak   -- position of peak value, registered
// ================================================================================================
void Verona_b_CorrelationQuality( wiWORD BCOutR, wiWORD BCOutI, wiBoolean CQEnB, wiBoolean CSEnB, 
                                  wiUWORD CQwindow, wiUWORD SSwindow, wiUReg *CQOut, wiUReg *CQPeak )
{
    Verona_bCorrelationQuality_t *pState = &(Verona_bRxState()->CorrelationQuality);
    wiUWORD *cqRAM = pState->cqRAM;

    wiUWORD newPeak, x2, y, z, s;

    // ---------------------------------------------
    // (WiSE) return to avoid unnecessary processing
    // ---------------------------------------------
    if (CQEnB && !pState->state.Q.w2 && !CQOut->D.word) return; 

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(pState->dBCOutR);
    ClockRegister(pState->dBCOutI);
    ClockRegister(pState->RdAdr);
    ClockRegister(pState->WrAdr);
    ClockRegister(pState->WrEnB);
    ClockRegister(pState->RAMInit);
    ClockRegister(pState->CQClr);
    ClockRegister(pState->OutEnB);
    ClockRegister(pState->state);

    // ----------
    // Controller
    // ----------
    switch (pState->state.Q.w2)
    {
        case 0: // idle
            pState->OutEnB.D.word  = 1;
            if (!CQEnB) pState->state.D.word = 1;
            break;

        case 1: // initialization
            pState->OutEnB.D.word= 0;
            pState->state.D.word = (pState->RdAdr.Q.w5 == 21) ? 2:1;
            break;

        case 2: // operational
            pState->OutEnB.D.word = 0;
            if (!CQEnB) { pState->state.D.word = 2; } // OutEnB.D.word = 0; }
            else        { pState->state.D.word = 3; } // OutEnB.D.word = 1; }
            break;

        case 3: // output result
            pState->OutEnB.D.word = 1;
            pState->state.D.word  = 0;
            break;
    }
    pState->WrAdr.D.word  = pState->state.Q.w2 ? (pState->WrAdr.Q.w5 + 1)%22 : 21;
    pState->RdAdr.D.word  = pState->state.Q.w2 ? (pState->RdAdr.Q.w5 + 1)%22 :  0;
    pState->WrEnB.D.word  = pState->state.Q.w2 ? 0:1;
    pState->RAMInit.D.word= pState->state.Q.b1 ? 0:1; // initialize in states 0,1
    pState->CQClr.D.word  = pState->state.Q.w2 ? 0:1;

    // -----------
    // Wire Inputs
    // -----------
    if (pState->state.Q.w2) {
        pState->dBCOutR.D.word = BCOutR.w9 >> 2;
        pState->dBCOutI.D.word = BCOutI.w9 >> 2;
    }
    // -----------
    // Calculation
    // -----------
    x2.word = SQR(pState->dBCOutR.Q.w7) + SQR(pState->dBCOutI.Q.w7);            // x2 = ||BCOut||^2
    y.word  = pState->RAMInit.Q.b0 ? 0 : cqRAM[(pState->RdAdr.Q.w5+21)%22].w16; // retrieve accumulator into y
    y.word  = y.w16 + x2.w14;                                 // y += x2
    s.word  = !CSEnB ? CQwindow.w2 : SSwindow.w3;             // s = log2(number of symbols)
    z.word  = y.w16 >> s.w3;                                  // z = y/(2^s)

    if (!pState->WrEnB.Q.b0) cqRAM[pState->WrAdr.Q.w5].word = y.w16;           // store accumulator

    // -----------
    // Peak Search
    // -----------
    newPeak.b0 = (CQOut->Q.w16 < z.w13) ? 1:0;

         if (pState->CQClr.Q.b0)                 CQOut->D.word = 0;            // clear output
    else if (!pState->OutEnB.Q.b0 && newPeak.b0) CQOut->D.word = z.w13;        // update output
    else                                         CQOut->D.word = CQOut->Q.w13; // hold output

    if (!pState->OutEnB.Q.b0 && newPeak.b0) CQPeak->D.word = pState->WrAdr.Q.w5;
}
// end of Verona_b_CorrelationQuality()

// ================================================================================================
// FUNCTION  : Verona_b_AGC()
// ------------------------------------------------------------------------------------------------
// Purpose   : Receiver Automatic Gain Control (runs at 44 MHz)
// Parameters: AGCEnB        -- enable signal for AGC operation (active low)
//             AGCInitGains  -- initialize gain settings (before start of packet)
//             EDOut         -- measured energy, power of 2^(1/4) 
//             AGCbounded    -- {1=attenuation only, 0=gain up or down}
//             LgSigAFE      -- large signal detector from the radio
//             LNAGain       -- LNA gain {0=low,1=high}, b0=Radio, b1=External (output)
//             AGain         -- SGA gain, linear-in-dB with 1.5 dB step (output, 6b)
//             DGain         -- "b" baseband gain, 1.5 dB step, (signed output, 6b)
//             LargeSignal   -- indicates a large signal adjustment (output)
//             GainUpdate    -- indicate whether a gain update occurred (output)
//             AGCAbsEnergy  -- absolute energy at gain update (output, 8b)
//             AGCAbsPwr     -- same as AbsEnergy but continually updated (output, 8b)
//             pReg          -- Pointer to modeled register file (model input)
// ================================================================================================
void Verona_b_AGC( wiBoolean AGCEnB, wiBoolean AGCInitGains, wiUWORD EDOut, wiBoolean AGCbounded, 
                   wiBoolean LgSigAFE, wiUReg *LNAGain, wiUReg *AGain, wiReg *DGain, wiUReg *LargeSignal, 
                   wiUReg *GainUpdate, wiUWORD *AGCAbsEnergy, wiUReg *AGCAbsPwr, VeronaRegisters_t *pReg )
{
    Verona_bAGC_t *pState = &(Verona_bRxState()->AGC);
    wiBoolean UpdateGain = (!AGCEnB && pState->dAGCEnB && !AGCInitGains)? 1:0;

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(pState->GainLimit);
    ClockRegister(pState->XGain);
    ClockRegister(pState->negGain);

    // ---------------
    // Initialize Gain
    // ---------------
    if (AGCInitGains)
    {
        DGain->D.word            = 0;
        pState->XGain.D.word     = pReg->bInitAGain.w6;
        AGain->D.word            = pReg->bInitAGain.w6;
        LNAGain->D.word          = 3; // b0=Interface LNAGain, b1=Interface LNAGain2
        pState->GainLimit.D.word = 0;
        pState->negGain.D.word   = 0;
        LargeSignal->D.word      = 0;
        GainUpdate->D.word       = 0;
        pState->UpdateCount.word = 0;
        AGCAbsPwr->D.word        = 0;
    }
    // --------------------------------------
    // Absolute Power (Always on for 802.11b)
    // --------------------------------------
    if (!pReg->RXMode.b0)
    {
        wiWORD  tAbsEnergy;  // intermediate AbsEnergy calculation
        
        // --------------------------------------------------------------------
        // Calculate Absolute Energy from Relative Energy Measurement (EDOut)
        // Note that energy has units of 0.75 dB while gain has units of 1.5 dB
        // --------------------------------------------------------------------
        tAbsEnergy.w9   = (signed  )(EDOut.w6 + 2*pState->negGain.Q.w8) - pReg->bPwr100dBm.w5;
        AGCAbsPwr->D.w8 = (unsigned)max(0, tAbsEnergy.w9); // limit to non-negative value

        if (UpdateGain) AGCAbsEnergy->w8 = AGCAbsPwr->D.w8; // update AbsEnergy only on gain update
    }
    else AGCAbsEnergy->word = 0;

    // -----------
    // Update Gain
    // -----------
    if (UpdateGain)
    {
        wiWORD  gainError = {0};          // negative measured gain error in linear region
        wiWORD  gainUpdate = {0};         // gain update in linear region
        wiWORD  AGain1;                   // intermediate AGain value
        wiUWORD AGain2;                   //      "         "     "
        wiWORD  DGain1;                   // intermediate DGain value
        wiWORD  XGain1, XGain2;           // intermediate XGain values
        wiWORD  dXGain1, dXGain2, dXGain3;// gain change calculations
        wiUWORD LNAGain1, LNAGain2;       // intermediate LNAGain value

        wiBoolean validLgSigAFE=0;        // flag a large signal detect from the LgSigAFE signal
        wiBoolean validLgSigDFE=0;        // flag a large signal detect using ThSigLarge
        wiBoolean switchLNA;              // flag to switch radio LNA to low gain state
        wiBoolean switchLNA2;             // flag to switch external LNA to low gain state

        // check for update count limit
        pState->GainLimit.D.word = (pState->UpdateCount.w4 == pReg->bMaxUpdateCount.w4) ? 1:0; 
        pState->UpdateCount.w4++;   // increment gain update counter

        // ------------------
        // Flag Large Signals
        // ------------------
        validLgSigAFE = (!pReg->RXMode.b0 && LgSigAFE && LNAGain->Q.b0) ? 1:0; // radio large signal detector
        validLgSigDFE = (EDOut.w6>pReg->bThSigLarge.w6)? 1:0;                  // baseband (ED)

        // ---------------------
        // Calculate Gain Update
        // ---------------------
        if (validLgSigAFE)
        {
            LNAGain1.w2 = 0;          // switch LNAs to low gain state
            XGain1.w7   = pState->XGain.Q.w7; // no change in AGain
        }
        else if (validLgSigDFE)
        {  
            LNAGain1.w2 = LNAGain->Q.w2;                                    // no change to LNAGain
            XGain1.w7   = pState->XGain.Q.w7 - (signed)pReg->bStepLarge.w5; // reduce gain by StepLarge
        }
        else
        {  
            LNAGain1.w2   = LNAGain->Q.w2;                                       // no change to LNAGain
            gainError.w5  = ((signed)pReg->bRefPwr.w6 - (signed)EDOut.w6)/2;     // measured gain error
            gainUpdate.w5 = (AGCbounded && gainError.w5 > 0) ? 0 : gainError.w5; // gain correction
            XGain1.w7     = pState->XGain.Q.w7 + gainUpdate.w5;                  // updated AGain
        }
        // -------------------------------------------
        // Check for LNA Switching During XGain Update
        // -------------------------------------------
        dXGain1.w7 = max(0, (signed)pReg->bInitAGain.w6 - XGain1.w7); // net change in XGain (limit to attenuation)
        switchLNA  = (LNAGain1.b0 && (dXGain1.w7 > (signed)pReg->bThSwitchLNA.w6 )) ? 1:0;
        switchLNA2 = (LNAGain1.b1 && (dXGain1.w7 > (signed)pReg->bThSwitchLNA2.w6)) ? 1:0;

        if (switchLNA && switchLNA2) {
            LNAGain2.b1 = 0;
            LNAGain2.b0 = 0;                             // switch LNA to low gain state
            XGain2.w7   = XGain1.w7 + pReg->bStepLNA2.w5 + pReg->bStepLNA.w5; // adjust AGain
        }
        else if (switchLNA2) {
            LNAGain2.b1 = 0;
            LNAGain2.b0 = LNAGain1.b0;                   // switch LNA to low gain state
            XGain2.w7   = XGain1.w7 + pReg->bStepLNA2.w5;// adjust AGain
        }
        else if (switchLNA) {
            LNAGain2.b1 = LNAGain1.b1;
            LNAGain2.b0 = 0;                             // switch LNA to low gain state
            XGain2.w7   = XGain1.w7 + pReg->bStepLNA.w5; // adjust AGain
        }
        else  {
            LNAGain2.w2 = LNAGain1.w2;
            XGain2.w7   = XGain1.w7;
        }
        dXGain2.w7 = (signed)pReg->bInitAGain.w6 - XGain2.w7;     // net change in XGain
        dXGain3.w7 = dXGain2.w7 - (signed)pReg->DAmpGainRange.w4; // ...residual after DGain

        // --------------------------------------------
        // Partition Gain Stages: XGain = AGain + DGain
        // --------------------------------------------
        if (pReg->RXMode.w2 == 2) // 11b mode: CCK AGC Only
        {                         // ----------------------
            if (dXGain3.w7 >= 0)
            {  // XGain covers entire DGain range and part of AGain
                AGain1.w7 =  (signed)pReg->bInitAGain.w6 - dXGain3.w7;
                DGain1.w5 = -(signed)pReg->DAmpGainRange.w4;
            }
            else if (dXGain2.w7 > 0)
            {  // XGain covered by DGain alone (no change to AGain)
                AGain1.w7 = (signed)pReg->bInitAGain.w6;
                DGain1.w5 = -dXGain2.w7;
            }
            else
            {  // Gain increase - adjust AGain only
                AGain1.w7 = XGain2.w7;
                DGain1.w5 = 0;
            }
        }     // --------------------------------------
        else  // 11g mode: OFDM AGC + CCK AGC for DGain
        {     // --------------------------------------
            if (dXGain3.w7 >= 0)
                DGain1.w5 = -(signed)pReg->DAmpGainRange.w4;
            else
                DGain1.w5 = -dXGain2.w7;

            AGain1.w7 = (signed)pReg->bInitAGain.w6;
        }
        AGain2.word = limit(AGain1.w7, 0, 63);

        // ------------------------------------------------------
        // Check for Attenuation Limit and Register Gain Settings
        // ------------------------------------------------------
        AGain->D.word = pState->GainLimit.Q.b0 ? AGain->Q.w6 : AGain2.w6;
        DGain->D.word = pState->GainLimit.Q.b0 ? DGain->Q.w5 : DGain1.w5;
        pState->XGain.D.word  = XGain2.w7;
        LNAGain->D.w2         = LNAGain2.w2;
        pState->negGain.D.w8 = dXGain2.w7 + (signed)(LNAGain2.b1 ? 0 : pReg->bStepLNA2.w5) 
                                          + (signed)(LNAGain2.b0 ? 0 : pReg->bStepLNA.w5);
        LargeSignal->D.b0 = (validLgSigAFE || validLgSigDFE || switchLNA || switchLNA2) && !pState->GainLimit.Q.b0;
        GainUpdate->D.b0  = (LargeSignal->D.b0 || (gainUpdate.w5 != 0)) && !pState->GainLimit.Q.b0;
    }
    pState->dAGCEnB = AGCEnB; // 1T delayed version of AGCEnB
}
// end of Verona_b_AGC()

// ================================================================================================
// FUNCTION  : Verona_b_EstimateCFO
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate Carrier Frequency Offset in the Received Signal
// Parameters: CFOeEnB       -- enable CFO estimation
//             CFOeGet       -- get CFO estimate
//             BCOutR        -- Barker correlator output, real, 9 bit
//             BCOutI        -- Barker correlator output, imag, 9 bit
//             CFO           -- estimated CFO = 8192/11MHz * fo
// ================================================================================================
void Verona_b_EstimateCFO( wiBoolean CFOeEnB, wiBoolean CFOeGet, wiWORD BCOutR, wiWORD BCOutI, 
                           wiReg *CFO, VeronaRegisters_t *pReg )
{
    const wiUInt cfoTable[64] = {  0,  4,  7, 11, 15, 18, 22, 26, 29, 32, 36, 39, 43, 46, 49, 52,
                                  55, 58, 61, 64, 66, 69, 71, 74, 76, 79, 81, 83, 85, 87, 89, 91,
                                  93, 95, 97, 98,100,102,103,105,106,108,109,110,112,113,114,115,
                                 116,118,119,120,121,122,123,124,125,126,126,127,128,129,130,130 };
    
    Verona_bEstimateCFO_t *pState = &(Verona_bRxState()->EstimateCFO);

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(pState->AcmAxR);
    ClockRegister(pState->AcmAxI);
    ClockRegister(pState->ratio);

    // ------------------------------------
    // Initialization (Estimation Disabled)
    // ------------------------------------
    if (CFOeEnB == 1)
    {
        pState->count = 0;
        pState->AcmAxR.D.word = pState->AcmAxI.D.word = 0;
        pState->addr = 0;
    }
    // -----------------
    // Estimator Enabled
    // -----------------
    if (CFOeEnB == 0)
    {
        wiWORD axR, axI;         // autocorrelation for current and delayed input
        wiWORD dBCOutR, dBCOutI; // delayed input (from RAM)

        // ---------------------------
        // Input Delay Line: RAM Model
        // ---------------------------
        dBCOutR.word = pState->BCOutR_RAM[pState->addr].w8;
        dBCOutI.word = pState->BCOutI_RAM[pState->addr].w8;

        pState->BCOutR_RAM[pState->addr].word = BCOutR.w9 >> 1;
        pState->BCOutI_RAM[pState->addr].word = BCOutI.w9 >> 1;
        pState->addr = (pState->addr + 1) % 22;

        // ---------------------------------
        // Autocorrelation with lag 22/22MHz
        // ---------------------------------
        if (pState->count >= 22)
        {
            wiUWORD aR, aI;

            // autocorrelation
            axR.word = ((BCOutR.w9>>1) * dBCOutR.w8 + (BCOutI.w9>>1) * dBCOutI.w8) >> 4;
            axI.word = ((BCOutI.w9>>1) * dBCOutR.w8 - (BCOutR.w9>>1) * dBCOutI.w8) >> 4;

            // magnitude of quadrature components
            aR.word = abs(axR.w12);
            aI.word = abs(axI.w12);

            // accumulate large magnitude samples
            if (!pReg->bCFOQual.b0 || (aR.w11+aI.w11 > 64) || (aR.w11 > 45) || (aI.w11 > 45))
            {
                // accumulate autocorrelation (keep real term positive)
                if (axR.w12 > 0)
                {
                    pState->AcmAxR.D.word = pState->AcmAxR.Q.w20 + axR.w12;
                    pState->AcmAxI.D.word = pState->AcmAxI.Q.w20 + axI.w12;
                }
                else if (axR.w12 < 0)
                {
                    pState->AcmAxR.D.word = pState->AcmAxR.Q.w20 - axR.w12;
                    pState->AcmAxI.D.word = pState->AcmAxI.Q.w20 - axI.w12;
                }
            }
        }

        // ------------
        // Estimate CFO
        // ------------
        if (CFOeGet)
        {
            int k;
            wiWORD xxReal,xxImag; // sum of autocorrelation samples (reduced resolution)
            wiUWORD arg;          // argument to the atan lookup table = abs(ratio)

            for (k=0; (pState->AcmAxR.Q.w20>>k) >= 16; k++) // compute order of AcmAxR
                ;
            // ------------------------------------------------------------------
            // reduce resolution of divider input (includes limiter on Imag term)
            // ------------------------------------------------------------------
            xxReal.word = pState->AcmAxR.Q.w20 >> k;
            xxImag.word = (k > 4) ? (pState->AcmAxI.Q.w20 >> (k-5)) : (pState->AcmAxI.Q.w20 << (5-k));
            xxImag.word = limit(xxImag.w20, -1024,1023); // limit to 11bits

            pState->ratio.D.word = xxReal.w5 ? (xxImag.w11/xxReal.w5) : 0; // RTL: Divider
            pState->ratio.D.word = limit(pState->ratio.D.w11, -59, 59); // limit size of atan table

            // Compute CFO = round{4096/pi/11 * atan(ratio/32)}
            arg.word = abs(pState->ratio.Q.w7);
            CFO->D.word = (pState->ratio.Q.b6? -1:1) * cfoTable[arg.w6];

            // Disable CFO estimate/correction
            // This is a non-RTL feature included for architecture testing
            //
            if (Verona_RxState()->NonRTL.Enabled && Verona_RxState()->NonRTL.DisableCFO)
                CFO->D.word = 0;
        }
        else
            CFO->D.word = 0; // clear output until estimate is requested

        pState->count++; // increment sample counter
    }
}
// end of Verona_b_EstimateCFO()

// ================================================================================================
// FUNCTION  : Verona_b_CorrectCFO()
// ------------------------------------------------------------------------------------------------
// Purpose   : CFO Correction
// Parameters: CFOcEnB    -- enable correction (active low)
//             TransCCK   -- hold the oscillator phase (due to TransCCK)
//             dDeciFreeze-- hold the oscillator phase (due to DeciFreeze)
//             CFO        -- CFO value from estimation, 8b
//             cCP        -- carrier phase correction term from DPLL, 6b
//             CFOcInR    -- data input,  real, 8b
//             CFOcInI    -- data input,  imag, 8b
//             CFOcOutR   -- data output, real, 8b
//             CFOcOutI   -- data output, imag, 8b
//             posedgeCLK11MHz -- rising edge of 11 MHz clock
// ================================================================================================
void Verona_b_CorrectCFO( wiBoolean CFOcEnB, wiBoolean TransCCK, wiBitDelayLine dDeciFreeze, wiWORD CFO, wiWORD cCP, 
                          wiWORD CFOcInR, wiWORD CFOcInI, wiWORD *CFOcOutR, wiWORD *CFOcOutI, wiBoolean posedgeCLK11MHz )
{
    const wiInt sinTable[64] = {  0, -3, -6, -9,-12,-15,-18,-20,-23,-25,-27,-28,-30,-31,-31,-32,
                                -32,-32,-31,-31,-30,-28,-27,-25,-23,-20,-18,-15,-12, -9, -6, -3,
                                  0,  3,  6,  9, 12, 15, 18, 20, 23, 25, 27, 28, 30, 31, 31, 32,
                                 32, 32, 31, 31, 30, 28, 27, 25, 23, 20, 18, 15, 12,  9,  6,  3 };
    const wiInt cosTable[64] = {-32,-32,-31,-31,-30,-28,-27,-25,-23,-20,-18,-15,-12, -9, -6, -3,
                                  0,  3,  6,  9, 12, 15, 18, 20, 23, 25, 27, 28, 30, 31, 31, 32,
                                 32, 32, 31, 31, 30, 28, 27, 25, 23, 20, 18, 15, 12,  9,  6,  3,
                                  0, -3, -6, -9,-12,-15,-18,-20,-23,-25,-27,-28,-30,-31,-31,-32 };

    Verona_bCorrectCFO_t *pState = &(Verona_bRxState()->CorrectCFO);

    wiWORD c,s,p;

    // ---------------
    // Clock Registers
    // ---------------
    if (posedgeCLK11MHz)
    {
        wiWORD *q = &(pState->q);

        ClockRegister(pState->xR); ClockRegister(pState->xI);
        ClockRegister(pState->yR); ClockRegister(pState->yI);
        ClockRegister(pState->dcCP);
        ClockDelayLine(pState->dTransCCK, TransCCK);

        // -----------
        // Wire Inputs
        // -----------
        pState->xR.D.word = CFOcInR.w8;
        pState->xI.D.word = CFOcInI.w8;

        pState->xR.D.word = pState->dTransCCK.delay_2T ? pState->xR.Q.w8 : CFOcInR.w8;
        pState->xI.D.word = pState->dTransCCK.delay_2T ? pState->xI.Q.w8 : CFOcInI.w8;

        pState->dcCP.D.word = cCP.w6;

        if (CFOcEnB)
        {
            pState->yR.D.word = pState->yI.D.word = 0;
            q->word = 0;
            pState->count = 0;
        }
        else if (!pState->dTransCCK.delay_3T && !dDeciFreeze.delay_1T) // correction enabled -------
        {
            q->w13 = (pState->count < 2) ? q->w13 : (q->w13 + CFO.w8); // 4096 -> pi, allow wrap-around
            p.w6   = (q->w13 >> 7) + pState->dcCP.Q.w6;                // 32 -> pi, allow wrap-around

            c.word = cosTable[p.w6 + 32];                // cosine/sine lookup
            s.word = sinTable[p.w6 + 32];                // (c + j*s) = exp{j*pi*p/32}

            pState->yR.D.word = (pState->xR.Q.w8 * c.w7 + pState->xI.Q.w8 * s.w7) / 32; // no bitshift allowed for divider
            pState->yI.D.word = (pState->xI.Q.w8 * c.w7 - pState->xR.Q.w8 * s.w7) / 32;

            pState->count++;
        }
    }
    // ------------
    // Wire Outputs
    // ------------
    CFOcOutR->word = pState->yR.Q.w8;
    CFOcOutI->word = pState->yI.Q.w8;
}
//end of Verona_b_CorrectCFO()

// ================================================================================================
// FUNCTION  : Verona_b_ChanEst()
// ------------------------------------------------------------------------------------------------
// Purpose   : Channel Estimation
// Parameters: CEEnB      -- channel estimation enable (active low)
//             CEMode     -- channel estimation state (0=coarse, 1=fine)
//             GetCE      -- force completion of channel estimation
//             BCOutR     -- Barker correlator output, real, 9b, 11MHz
//             BCOutI     -- Barker correlator output, imag, 9b, 11MHz
//             DMOut      -- demodulator output,1b, 1 or 2 element arrays for DBPSK,DQPSK
//             DMOutValid -- indicate data at DMOut is valid
//             DQPSK      -- modulation type (0=DBPSK, 1=DQPSK)
//             CEDone     -- channel estimation complete
//             RX         -- bVerona RX data structure
//             pReg       -- configuration registers
// ================================================================================================
void Verona_b_ChanEst( wiBoolean CEEnB, wiBoolean CEMode, wiBoolean GetCE, wiWORD BCOutR, wiWORD BCOutI,
                       wiUWORD DMOut, wiBoolean DMOutValid, wiBoolean DBPSK, wiBoolean *CEDone,
                       Verona_bRxState_t *RX, VeronaRegisters_t *pReg )
{
    Verona_bChanEst_t *pState = &(Verona_bRxState()->ChanEst);

    // --------------
    // Initialization
    // --------------
    if (CEEnB)
    {
        pState->WrAdr.word = 0;
        pState->RdAdr.word = 0;
        pState->count = 0;
        pState->SymBufCount = 0;
        pState->NSym = 0;
        pState->CNTDM = 0;
        pState->CNTdelay = 0;
        pState->CNTstart = 0;
        *CEDone = 0; // added 2005-07-21 19:42 (otherwise holds over from previous packet)
    }
    // -------------------------
    // Active Channel Estimation
    // -------------------------
    if (!CEEnB)
    {
        int i;
        wiWORD *aReal   = pState->aReal;
        wiWORD *aImag   = pState->aImag;
        wiWORD *hReal   = pState->hReal;
        wiWORD *hImag   = pState->hImag;
        wiWORD *dBCOutR = pState->dBCOutR;
        wiWORD *dBCOutI = pState->dBCOutI;

        // count symbol decisions and activate CEDone
        if (pState->CNTstart) pState->CNTdelay++;
        if (DMOutValid) pState->CNTDM++;
        if (pState->CNTDM == (CEMode == 0 ? 1<<pReg->CENSym1.w3 : (1<<pReg->CENSym2.w3))) pState->CNTstart = 1;
        if (GetCE == 1 && DMOutValid==1) pState->CNTstart = 1;
        if (pState->CNTdelay == 23 + (pState->save_Nh / 4)) // delay at T=1/11MHz
        {
            pState->CNTstart = 0;
            pState->CNTdelay = 0;
            *CEDone  = 1;

            RX->Nh = pState->save_Nh;
            RX->Np = pState->save_Np;
            RX->Np2= pState->save_Np2;
            for (i=0; i<22; i++)
            {
                RX->hReal[i].word = hReal[i].w9;
                RX->hImag[i].word = hImag[i].w9;
            }
        }
        // -------------------------------------------
        // Buffer the Barker Correlator Output Samples
        // -------------------------------------------
        dBCOutR[pState->WrAdr.w5].word = BCOutR.w9;
        dBCOutI[pState->WrAdr.w5].word = BCOutI.w9;
        pState->WrAdr.w5++;
        pState->count++;

        // ----------------------------------------------------
        // Buffer the Demodulator Output and Type (DBPSK/DQPSK)
        // ----------------------------------------------------
        if (DMOutValid == 1)
        {
            pState->dMod  [pState->SymBufCount].word = DBPSK;
            pState->dDMOut[pState->SymBufCount].word = DMOut.w2;
            pState->SymBufCount++;
        }
        // --------------------
        // Error/Range Checking
        // --------------------
        if (pState->count>32 || pState->SymBufCount>2) { // check for buffer overrun (model only)
            wiPrintf("*** Verona_b_ChanEst ERROR: count=%d, SymBufCount=%d\n",pState->count,pState->SymBufCount);
            WICHK(WI_ERROR_RANGE); // place an entry in the log file
            exit(1);
        }
        // ------------------------------
        // Channel Estimation Calculation
        // ------------------------------
        if (pState->count >= 22 && pState->SymBufCount > 0)
        {
            int cReal, cImag; // QPSK constellation coordinates

            // ------------------------------------------------
            // Update Phase Modulation Using Demodulator Output
            // Phase Mapping: 0=pi/4, 1=3pi/4, 2=5pi/4, 3=7pi/4
            // ------------------------------------------------
            if (pState->NSym == 0) 
            {  
                pState->phase.word = 0;                       // initial phase
                for (i=0; i<22; i++) 
                    aReal[i].word = aImag[i].word = 0; // clear the accumulators
            }
            else if (pState->dMod[0].b0)   
            {  
                if (pState->dDMOut[0].b0) pState->phase.w2 += 2; // DBPSK
            }
            else {  
                switch (pState->dDMOut[0].w2)            // DQPSK (gray coded)
                {
                    case 0: pState->phase.w2 += 0; break;
                    case 1: pState->phase.w2 += 1; break;
                    case 2: pState->phase.w2 += 3; break;
                    case 3: pState->phase.w2 += 2; break;
                }
            }
            // --------------------------------------
            // Map Phase to Cartesian Coordinates
            // 0=(+1+j), 1=(-1+j), 2=(-1-j), 3=(+1-j)
            // --------------------------------------
            cReal = pState->phase.b0 ^ pState->phase.b1 ? -1:1;
            cImag = pState->phase.b1                    ? -1:1;
            
            // -------------------------------------------------------
            // Accumulate Correlations After Removing Phase Modulation
            // -------------------------------------------------------
            for (i=0; i<22; i++)
            {
                unsigned addr = (pState->RdAdr.w5 + i) % 32;
                aReal[i].w16 += (dBCOutR[addr].w9*cReal + dBCOutI[addr].w9*cImag);
                aImag[i].w16 += (dBCOutI[addr].w9*cReal - dBCOutR[addr].w9*cImag);
            }
            // ---------------------------
            // Update Counters and Buffers
            // ---------------------------
            pState->RdAdr.w5 += 11;       // move BCOut buffer read address forward 1 symbol (11 chips)
            pState->count    -= 11;       // decrement count of BCOUt buffered samples
            pState->dDMOut[0] = pState->dDMOut[1];// reduce FIFO
            pState->dMod[0]   = pState->dMod[1];  // reduce FIFO
            pState->SymBufCount--;        // decrement symbol buffer counter
            pState->NSym++;               // increment symbol counter

            // --------------------------------------------------------------------------------
            // Correction Filtering
            // Reduce error due to difference between delta function and Barker autocorrelation
            // --------------------------------------------------------------------------------
            if ( (CEMode == 0 && pState->NSym == (unsigned)(1<<pReg->CENSym1.w3)) 
              || (CEMode == 1 && pState->NSym == (unsigned)(1<<pReg->CENSym2.w3) && !GetCE) 
              || (GetCE && pState->CNTdelay == 0) )
            {
                const int g[29] = {2,0,2,0,5,0,6,0,7,0,7,0,8,0,37,0,8,0,7,0,7,0,6,0,5,0,2,0,2};
                const int Ng = 29; // filter length
                const int g0 = 14; // center tap position (group delay)

                wiWORD xR[50]={{0}}, xI[50]={{0}}; // shift register for filter (unused terms set to zero)
                wiWORD yR[22], yI[22];             // filter output before scaling
                int i,j,k,m,n;
                
                // ---------------------------------------------------------------------------------
                // Scale Accumulated Values -- net scaling is 4*sqrt(2) including Barker Correlator
                // ...note values are offset in xR/xI by g0, the correction filter center tap offset
                // ---------------------------------------------------------------------------------
                for (i=0; i<22; i++)
                {
                    xR[g0+i].word = aReal[i].w16 * 16 / ((int)pState->NSym*2*11);
                    xI[g0+i].word = aImag[i].w16 * 16 / ((int)pState->NSym*2*11);
                }
                // -----------------------------------------------------------------
                // Apply Correction Filter -- net scaling after divider is 4*sqrt(2)
                // -----------------------------------------------------------------
                for (i=0; i<22; i++)
                {
                    yR[i].word = yI[i].word = 0;
                    for (k=0; k<Ng; k++)
                    {
                        j = (Ng - 1) - k + i;
                        yR[i].word += (g[k] * xR[j].w10);
                        yI[i].word += (g[k] * xI[j].w10);
                    }
                    hReal[i].word = limit(yR[i].w16 / 64, -255,255);
                    hImag[i].word = limit(yI[i].w16 / 64, -255,255);
                }
                hReal[21].word = hImag[21].word = 0; // clear the end tap

                // -----------------------------------------------------------------------------
                // Remove Unreliable Terms
                // Approximate magnitude with the sum of magnitudes of the quadrature components
                // The "peak" magnitude is computed from the center of the filter
                // -----------------------------------------------------------------------------
                {
                    wiUWORD peak, mag, Th1, Th2;

                    peak.word = abs(hReal[10].w9) + abs(hImag[10].w9);
                    Th1.word = peak.w9 >> (CEMode==0 ? pReg->hThreshold1.w3 : pReg->hThreshold3.w3);
                    Th2.word = peak.w9 >> pReg->hThreshold2.w3;

                    n = m = 22;
                    for (k=0; k<22; k++)
                    {
                        mag.word = abs(hReal[k].w9) + abs(hImag[k].w9);

                        if (mag.w9 <= Th1.w9) 
                            hReal[k].word = hImag[k].word = 0;
                        else 
                            n=min(n,k); // flag first term >Th1

                        if (mag.w9 > Th2.w9) m = min(m,k); // flag first term >Th2
                    }
                }
                // shift length 22-n term response to start at tap 0
                for (i=n,j=0; i<22; i++,j++)  
                {
                    hReal[j].word = hReal[i].w9;
                    hImag[j].word = hImag[i].w9;
                }
                for (; j<22; j++) 
                    hReal[j].word = hImag[j].word = 0; // clean up end of response (C only)
                pState->save_Nh  = 22 - n; // adjusted filter length
                pState->save_Np  = 10 - n; // adjusted RAKE receiver filter delay (precursors)
                pState->save_Np2 = 10 - m; // adjusted CCK receiver filter delay (precursors)

                if (CEMode == 0) // save coarse estimate
                {
                    for (i=0; i<22; i++)
                    {
                        RX->hcReal[i].word = hReal[i].word;
                        RX->hcImag[i].word = hImag[i].word;
                        RX->Nhc = pState->save_Nh;
                        RX->Npc = pState->save_Np;
                        RX->Np2c = pState->save_Np2;
                    }
                }
            }
        }
    }     
}
// end of Verona_b_ChanEst()

// ================================================================================================
// FUNCTION  : Verona_bRX_SP()
// ------------------------------------------------------------------------------------------------
// Purpose   : FIFO between CFO correction output and CCK demodulator input
// Parameters: uReal, uImag -- CFO correction output, real, imag, 8-bit
//             wRptr, wIptr -- pointers to last output sample from FIFO
//             Np2          -- number of extra precursor terms used for CCK demodulation
//             SPEnB        -- enable FIFO
//             TransCCK     -- input hold signal to reduce buffer delay
//             dDeciFreeze  -- input hold signal from decimator
// ================================================================================================
void Verona_bRX_SP(wiWORD *uReal, wiWORD *uImag, wiWORD **wRptr, wiWORD **wIptr, int Np2, 
                   wiBoolean SPEnB, wiBoolean TransCCK, wiBitDelayLine dDeciFreeze)
{
    Verona_bRxState_t *pbRX = Verona_bRxState();
    Verona_bRX_SP_t   *pState = &(pbRX->RX_SP);

    ClockDelayLine(pState->dTransCCK, TransCCK);

    // -------------------
    // Initialize CCK FIFO
    // -------------------
    if (SPEnB) 
    {
        pState->wrptr = 11+8; // delay set by CMF(11) + DSSS Symbol Period-Demodulation Delay(8)
        pState->rdptr = 0;
    }
    // ----------------------------------------------------------------
    // Adjust Buffer Depth for CCK Precursors
    // ...move CCK input back to include Np2 additional precursor terms
    // ----------------------------------------------------------------
    if (pState->dTransCCK.delay_0T && !pState->dTransCCK.delay_1T) // update at start of CCK demodulation
        pState->rdptr -= Np2; 

    // -------------------------------------------------
    // Write Input to FIFO Except During TransCCK period
    // -------------------------------------------------
    if (!pState->dTransCCK.delay_4T && !dDeciFreeze.delay_2T)
    {
        pbRX->wReal[pState->wrptr].word = uReal->w8;
        pbRX->wImag[pState->wrptr].word = uImag->w8;
        pState->wrptr++;
    }
    // --------------------
    // FIFO Output Pointers
    // --------------------
    *wRptr = pbRX->wReal + pState->rdptr;
    *wIptr = pbRX->wImag + pState->rdptr;
    pState->rdptr++;
}
// end of Verona_bRX_SP()

// ================================================================================================
// FUNCTION  : Verona_bRX_CMF()
// ------------------------------------------------------------------------------------------------
// Purpose   : Channel Matched Filter (for DSSS Demodulator)
// Parameters: CMFEnB      -- Enable CMF
//             DMSym       -- DSSS symbol strobe
//             dDeciFreeze -- 11 MHz sample stream hold
//             uReal       -- CMF input, real, 8 bit (7 bit used)
//             uImag       -- CMF input, imag, 8 bit (7 bit used)
//             zReal       -- CMF output, real, 9 bit
//             zImag       -- CMF output, imag, 9 bit
//             Nh          -- length of channel response
//             hReal       -- estimated channel response, real terms, 8 bit
//             hImag       -- estimated channel response, imag terms, 8 bit
// ================================================================================================
void Verona_bRX_CMF( wiBoolean CMFEnB, wiBitDelayLine DMSym, wiBitDelayLine dDeciFreeze, 
                     wiWORD *uReal, wiWORD *uImag, wiWORD **SSMatOutR, wiWORD **SSMatOutI, 
                     int Nh, wiWORD *hReal, wiWORD *hImag )
{
    Verona_bRX_CMF_t *pState = &(Verona_bRxState()->CMF);

    wiWORD *zReal = pState->zReal;
    wiWORD *zImag = pState->zImag;

    if (CMFEnB) 
    {
        pState->Nhs=0; 
    }
    else
    {
        int i, j, k;

        wiWORD *dR = pState->dR;
        wiWORD *dI = pState->dI;
        wiWORD *hR = pState->hR;
        wiWORD *hI = pState->hI;

        if (pState->dCMFEnB) // initialize on first sample
        {
            for (i=0; i<23; i++) dR[i].word = dI[i].word = 0;
            for (i=0; i<15; i++) zReal[i].word = zImag[i].word = 0;
        }
        if (DMSym.delay_0T) // update channel response
        {
            for (k=0; k<Nh; k++)
            {
                hR[k].word = hReal[k].w9 >> 1;
                hI[k].word = hImag[k].w9 >> 1;
                pState->Nhs = Nh;
            }
            for (; k<22; k++) hR[k].word = hI[k].word = 0;
        }
        // -----------------------------------------------------
        // Input Signal Delay Line (With Shift as Divide-by-Two)
        // -----------------------------------------------------
        if (!dDeciFreeze.delay_2T)
        {
            for (i=22; i>0; i--) dR[i] = dR[i-1];   dR[0].word = uReal->w8 >> 1;
            for (i=22; i>0; i--) dI[i] = dI[i-1];   dI[0].word = uImag->w8 >> 1;
        }
        // ------------------------
        // Output Signal Delay Line
        // ------------------------
        if (!dDeciFreeze.delay_3T)
        {
            for (i=20; i>0; i--) zReal[i] = zReal[i-1];
            for (i=20; i>0; i--) zImag[i] = zImag[i-1];
        }
        // ----------------------------------------------------------
        // Channel Matched Filter: z(t) = w(t) * conj[h(-t)]
        // Note there is an implicit delay at the front of the filter
        // ----------------------------------------------------------
        zReal->word = zImag->word = 0;
        for (k=0, j=pState->Nhs; k<pState->Nhs; k++, j--)
        {
            zReal[0].w18 += (hR[k].w8 * dR[j].w7) + (hI[k].w8 * dI[j].w7);
            zImag[0].w18 += (hR[k].w8 * dI[j].w7) - (hI[k].w8 * dR[j].w7);
        }
        zReal->word = zReal->w18 >> 5;
        zImag->word = zImag->w18 >> 5;

        zReal->word = limit(zReal->w13, -255, 255);
        zImag->word = limit(zImag->w13, -255, 255);
    }
    // --------------------------------
    // Wire Outputs With Register Delay
    // --------------------------------
    *SSMatOutR = zReal + 1;
    *SSMatOutI = zImag + 1;

    pState->dCMFEnB = CMFEnB;
}
// end of Verona_bRX_CMF()

// ================================================================================================
// FUNCTION  : Verona_bRX_DSSS()
// ------------------------------------------------------------------------------------------------
// Purpose   : DSSS Demodulator (using channel matched filter output)
// Parameters: DMEnB         -- enable demodulator
//             CFOOutR       -- CFO correction output, real, 8b
//             CFOOutI       -- CFO correction output, imag, 8b
//             CMFOutR       -- CMF output, real, 9b
//             CMFOutI       -- CMF output, imag, 9b
//             RBCOutR       -- rake receiver, Barker correlator output, real, 12b
//             RBCOutI       -- rake receiver, Barker correlator output, imag, 12b
//             SSCorOutValid -- rake receiver correlator output valid
//             SSSliOutR     -- demodulated symbol (slicer output), real
//             SSSliOutI     -- demodulated symbol (slicer output), imag
//             DMOut         --
//             DMSym   -- trigger DSSS symbol demodulation
//             CEDone  -- channel estimation complete (used on first CEDone)
//             DBPSK   -- modulation type (1=DBPSK, 0=DQPSK)
//
// ================================================================================================
void Verona_bRX_DSSS( wiBoolean DMEnB, wiWORD *CFOOutR, wiWORD *CFOOutI, wiWORD *CMFOutR, wiWORD *CMFOutI,
                      wiReg *RBCOutR, wiReg *RBCOutI,
                      wiUReg *SSCorOutValid,
                      wiUReg  *SSSliOutR, wiUReg *SSSliOutI,
                      wiUReg *DMOut, // symbol decision (1 or 2 bit output)
                      wiUReg *DMOutValid, // symbol decision valid
                      wiBoolean DBPSK,  // modulation = {1=DBPSK, 0=DQPSK}
                      wiBoolean CEDone,
                      wiBoolean HDREnB,
                      wiBitDelayLine DMSym,
                      wiWORD *BFReal, wiWORD *BFImag )
{
    const int BS[11] = {-1,-1,-1, 1, 1, 1,-1, 1, 1,-1, 1}; // Barker sequence
    int n;
    unsigned qReal, qImag;           // quantized correlator output   
    wiWORD   yReal, yImag;           // detector intermediate variable

    Verona_bRX_DSSS_t *pState = &(Verona_bRxState()->RX_DSSS);

    ClockRegister(pState->CBCOutR);
    ClockRegister(pState->CBCOutI);
    ClockRegister(pState->state);

    if (DMEnB) ResetRegister(pState->state); // initialize demodulator
    DMOutValid->D.word = 0;          // default output unless set below
    SSCorOutValid->D.word = 0;       // default output unless set below

    // ----------
    // Controller
    // ----------
    switch (pState->state.Q.word)
    {
        case 0:
            pState->CBCEnB = 1;
            pState->RBCEnB = 1;
            pState->pp1Real.word = pState->pp1Imag.word = 1;
            pState->pp2Real.word = pState->pp2Imag.word = 1;
            pState->NSymRake = 0;
            pState->NSymHDR = 0;
            pState->CBCOutR.D.word = 0;
            pState->CBCOutI.D.word = 0;
            RBCOutR->D.word = RBCOutI->D.word = 0;
            pState->CBCEnB = 1;
            pState->RBCEnB = 1;
            if (!DMEnB) pState->state.D.word = 1;
            break;
        case 1:
            pState->CBCEnB = 0; // correlation demod correlator enabled
            pState->RBCEnB = 1; // rake Barker correlator disabled
            if (CEDone) pState->state.D.word = 2; // first channel estimation done
            break;
        case 2:
            if (DMSym.delay_1T) pState->state.D.word = 3; // first channel estimation loaded
            break;
        case 3:
            if (DMSym.delay_2T) pState->state.D.word = 4; // delay due to CMF complete
            break;
        case 4:
            pState->RBCEnB = 0;                          // enable rake Barker correlator
            if (DMSym.delay_3T) pState->state.D.word = 5; // first symbol demod used to setup previous symbol qReal,Imag
            break;
        case 5:
            pState->CBCEnB = 1; // disable correlation demod
            break;
        default:
            pState->state.D.word = 0;
    }
    // --------------------------------
    // Noncoherent Correlation Receiver
    // --------------------------------
    if (!pState->CBCEnB)
    {
        if (DMSym.delay_11T)
        {
            wiWORD acmR = {0}, acmI = {0};
            for (n=0; n<11; n++) {
                acmR.w12 += BS[n] * (CFOOutR-n-2)->w8;
                acmI.w12 += BS[n] * (CFOOutI-n-2)->w8;
            }
            pState->CBCOutR.D.word = acmR.w12 >> 4;
            pState->CBCOutI.D.word = acmI.w12 >> 4;
        }
        if (DMSym.delay_12T)
        {
            // phase difference
            yReal.w16 = (pState->CBCOutR.Q.w8 * pState->pp1Real.w8) + (pState->CBCOutI.Q.w8 * pState->pp1Imag.w8);

            // decode bit
            DMOut->D.word = (yReal.w16 >= 0) ? 0:1;

            // save the phase of current symbol
            pState->pp1Real.word = pState->CBCOutR.Q.w8;
            pState->pp1Imag.word = pState->CBCOutI.Q.w8;

            DMOutValid->D.word = 1;
        }
    }
    // -------------
    // RAKE Receiver
    // -------------
    if (!pState->RBCEnB)
    {
        // correlation with Barker sequence
        if (DMSym.delay_12T)
        {
            wiWORD acmR = {0}, acmI = {0};
            for (n=0; n<11; n++)
            {
                acmR.w13 += BS[n] * CMFOutR[n+1].w9;
                acmI.w13 += BS[n] * CMFOutI[n+1].w9;
            }
            RBCOutR->D.word = acmR.w13 >> 1;
            RBCOutI->D.word = acmI.w13 >> 1;
            SSCorOutValid->D.word = 1;
        }
        if (DMSym.delay_13T)
        {
            if (DBPSK) // D-BPSK
            {
                // quantize chip coordinates (to +1+j or -1-j)
                qReal = qImag = (RBCOutR->Q.w12 + RBCOutI->Q.w12 >= 0) ? 1:0; 

                // phase difference
                yReal.w3 = ((pState->pp2Real.b0 ^ qReal) ? -1:1) + ((pState->pp2Imag.b0 ^ qImag) ? -1:1);

                // decode bit
                if (pState->NSymRake>0) DMOut->D.word = (yReal.w3 >= 0) ? 0:1;
            }
            else   // D-QPSK
            {
                // quantize phase
                if      (RBCOutR->Q.w12>=0 && RBCOutI->Q.w12>=0) { qReal= 1; qImag= 1;}
                else if (RBCOutR->Q.w12< 0 && RBCOutI->Q.w12>=0) { qReal= 0; qImag= 1;}
                else if (RBCOutR->Q.w12< 0 && RBCOutI->Q.w12< 0) { qReal= 0; qImag= 0;}
                else                                             { qReal= 1; qImag= 0;}

                // phase difference
                yReal.word = (pState->pp2Real.b0 ^ qReal ? -1:1) + (pState->pp2Imag.b0 ^ qImag ? -1:1);
                yImag.word = (pState->pp2Real.b0 ^ qImag ? -1:1) - (pState->pp2Imag.b0 ^ qReal ? -1:1);

                // decode bit
                DMOut->D.b0 = (yReal.w3+yImag.w3 >= 0) ? 0:1;
                DMOut->D.b1 = (yReal.w3-yImag.w3 >= 0) ? 0:1;
            }
            if (pState->NSymRake>0) DMOutValid->D.word = 1; // first symbol used only to get phase for diff. demod

            SSSliOutR->D.b0 = pState->pp2Real.b0 = qReal;
            SSSliOutI->D.b0 = pState->pp2Imag.b0 = qImag;

            if (!HDREnB) pState->NSymHDR++;
            pState->NSymRake++;

            // feedback filter input
            if (!HDREnB && (pState->NSymHDR > 44 || (!DBPSK && pState->NSymHDR > 20)))
            {
                for (n=21; n>11; n--)
                {
                    BFReal[n] = BFReal[n-11];
                    BFImag[n] = BFImag[n-11];
                }
                for (n=1; n<=11; n++)
                {
                    BFReal[n].word = qReal ? BS[n-1] : -BS[n-1];
                    BFImag[n].word = qImag ? BS[n-1] : -BS[n-1];
                }
            }
        }
    }
    // ----------------------------------
    // Save Output Data in RX State Array
    // ----------------------------------
    if (DMOutValid->D.b0)
    {
        Verona_bRxState_t *pbRX = Verona_bRxState();
        pbRX->b[pbRX->db++].word = DMOut->D.b0;
        if (!DBPSK) pbRX->b[pbRX->db++].word = DMOut->D.b1;
    }
}
// end of Verona_bRX_DSSS()

// ================================================================================================
// FUNCTION  : Verona_bRX_CCK()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Demodulator
// Parameters: 
// ================================================================================================
void Verona_bRX_CCK(wiWORD *wR, wiWORD *wI,
                    wiUWORD *cckOut, wiUWORD *cckOutValid,
                    int DataRate,   // data rate
                    wiWORD *CCKMatOutR,// correlator input (TVMF output)
                    wiWORD *CCKMatOutI,
                    wiWORD *CCKWalOutR,// correlator output
                    wiWORD *CCKWalOutI,
                    wiWORD *pccReal[], wiWORD *pccImag[],
                    wiWORD *hReal, wiWORD *hImag,
                    int Nh, int Np, int Np2,
                    wiWORD *BFReal,   // feedback filter input
                    wiWORD *BFImag,
                    wiUWORD SSSliOutR, wiUWORD SSSliOutI,
                    wiBoolean *cckInit   // initialize header decoder (self-clearing)
                    )
{
    int   i, j, k, l, n, m;

    wiWORD    vReal[8], vImag[8];     // ISI
    wiWORD    yReal[8], yImag[8];     // ISI cancelled observation samples
    wiWORD    xcReal[64], xcImag[64]; // correlator output
    wiWORD    met[64];       // metric to compare
    int       max_met = 0;   // maximum metric
    wiUWORD   maxPtr = {0};  // index to the maximum metric
    wiBoolean max_qReal = 0, max_qImag = 0;// quantized correlation at the maximum
    wiBoolean qcReal,qcImag;      // quantized correlator output
    wiWORD    ycReal,ycImag;      // phase difference of two successive symbols

    int Nc;          // number of correlator terms (4 and 64 for 5.5 and 11 Mbps, respectively)

    wiWORD *zReal = CCKMatOutR, *zImag = CCKMatOutI; // rename TVMF outputs

    Verona_bRxState_t *pbRX = Verona_bRxState();
    Verona_bRX_CCK_t  *pState = &(pbRX->RX_CCK);
    wiUWORD           *ICI    = pState->ICI;
    wiWORD           (*ccReal)[8] = pState->ccReal;
    wiWORD           (*ccImag)[8] = pState->ccImag;

    cckOutValid->word = 0;
    
    // ------------------------------------------------
    // Per-Packet Initialization Using Channel Estimate
    // ------------------------------------------------
    if (*cckInit)
    {
        wiWORD uReal,uImag; // ISI cancelled noiseless samples
    	Verona_LookupTable_t *pLut = Verona_LookupTable();

        pState->ppReal = SSSliOutR.b0;
        pState->ppImag = SSSliOutI.b0;

        Nc                      = (DataRate == 55)? 4 : 64;                           // number of correlator terms
        ccReal = pState->ccReal = (DataRate == 55)? pLut->CC55Real : pLut->CC110Real; // cross-correlation sequence table
        ccImag = pState->ccImag = (DataRate == 55)? pLut->CC55Imag : pLut->CC110Imag;

        // -----------------------------------------
        // Inter-Chip Interference (ICI) Calculation
        // -----------------------------------------
        for (k=0; k<Nc; k++)
        {
            ICI[k].word = 0;
            for (m=0; m<8; m++)
            {
                uReal.word = uImag.word = 0;
                for (i=0; i<8; i++)
                {
                    j = (Np - Np2) + m - i;
                    if (j>=0 && j<Nh)
                    {
                        uReal.w12 += hReal[j].w9*ccReal[k][i].w2 - hImag[j].w9*ccImag[k][i].w2;
                        uImag.w12 += hImag[j].w9*ccReal[k][i].w2 + hReal[j].w9*ccImag[k][i].w2;
                    }
                }
                ICI[k].w13 += (uReal.w10*uReal.w10 + uImag.w10*uImag.w10)*2/8/8/2;
            }
        }
        pState->NSym.word = 0;
        *cckInit = 0;
    }
    // ------------------------------------------------------------------------------------
    // Handle Unassigned Pointer
    // There is a one clock period window in the control during which this routine could be 
    // called without cckInit as the result of a restart event. Because the behavior is not
    // determinental to the functionality (it is only a problem because ccReal/ccImage are
    // NULL pointers, we just make dummy assignments here
    // ------------------------------------------------------------------------------------
    if (!ccReal)
    {
    	Verona_LookupTable_t *pLut = Verona_LookupTable();
        ccReal = pLut->CC110Real; 
        ccImag = pLut->CC110Real; // purposely pick the wrong one so this cannot be successfully decoded
        wiPrintf(" ### Null pointer forced to valid value in %s at line %d\n",__FILE__,__LINE__);
    }
    // ---------------------------------------------
    // ISI Cancellation via Decision Feedback Filter
    // ---------------------------------------------
    for (n=0; n<8; n++)
    {
        vReal[n].word = vImag[n].word = 0;
        for (l=1; l<12+Np2-n; l++)
        {
            m = (Np-Np2)+l+n;
            vReal[n].w11 += BFReal[l].w2*hReal[m].w9 - BFImag[l].w2*hImag[m].w9;
            vImag[n].w11 += BFImag[l].w2*hReal[m].w9 + BFReal[l].w2*hImag[m].w9;
        }
        vReal[n].w9 = vReal[n].w11 / 4; // divide-by-4 to account for scaling of (hReal,hImag)
        vImag[n].w9 = vImag[n].w11 / 4;

        yReal[n].w8 = (wR[n-7].w8 - vReal[n].w9) / 2; // simple scaling for resolution reduction
        yImag[n].w8 = (wI[n-7].w8 - vImag[n].w9) / 2;
    }
    // ---------------------------
    // Time-Varying Matched Filter
    // ---------------------------
    for (n=0; n<8; n++)
    {
        zReal[n].word = zImag[n].word = 0;
        for (l=0; l<8; l++)
        {
            i=l-n+Np-Np2;
            if (i>=0 && i<Nh)
            {
                zReal[n].w16 += yReal[l].w8*hReal[i].w9 + yImag[l].w8*hImag[i].w9;
                zImag[n].w16 += yImag[l].w8*hReal[i].w9 - yReal[l].w8*hImag[i].w9;
            }
        }
        zReal[n].w13 = zReal[n].w16 / 8; // simple scaling for resolution reduction
        zImag[n].w13 = zImag[n].w16 / 8;

        // limiter
        zReal[n].w12 = limit(zReal[n].w13, -2047,2047);
        zImag[n].w12 = limit(zImag[n].w13, -2047,2047);
    }
    //wiPrintf("CCK: CCKMatOutR=["); for (n=0; n<8; n++) wiPrintf("%5d",zReal[n].w12); wiPrintf("]\n");

    // --------------------------------
    // find the most likely code
    // based on the Euclidean distance
    // --------------------------------
    Nc = (DataRate == 55) ? 4:64; // number of correlator sequences

    // -----------------------------------------
    // Cross-Correlation With Transmit Sequences
    // -----------------------------------------
    for (k=0; k<Nc; k++)
    {
        xcReal[k].word = xcImag[k].word = 0;
        for (n=0; n<8; n++)
        {
            xcReal[k].w14 += zReal[n].w12*ccReal[k][n].w2 + zImag[n].w12*ccImag[k][n].w2;
            xcImag[k].w14 += zImag[n].w12*ccReal[k][n].w2 - zReal[n].w12*ccImag[k][n].w2;
        }
    }
    // --------------------------------
    // Metric Aritration - Pick Largest
    // --------------------------------
    for (k=0; k<Nc; k++)
    {
        // quantize angle (1 for >= 0, 0 for < 0)
        qcReal = 1 ^ xcReal[k].b13;
        qcImag = 1 ^ xcImag[k].b13;

        // real part of correlation and ICI cancellation
        met[k].w15 = (xcReal[k].w14*(qcReal? 1:-1) + xcImag[k].w14*(qcImag? 1:-1)) - (signed)ICI[k].w13;

        // maximum picking
        if (!k || met[k].w15 > max_met)
        {
            maxPtr.word = k;
            max_met   = met[k].w15;
            max_qReal = qcReal;
            max_qImag = qcImag;
        }
    }
    // feedback filter input
    for (n=11+Np2; n>8; n--)
    {
        BFReal[n].w2 = BFReal[n-8].w2;
        BFImag[n].w2 = BFImag[n-8].w2;
    }
    for (n=1; n<=8 && n<12+Np2; n++)
    {
        BFReal[n].w2 = ccReal[maxPtr.w6][8-n].w2*(max_qReal? 1:-1) - ccImag[maxPtr.w6][8-n].w2*(max_qImag? 1:-1);
        BFImag[n].w2 = ccImag[maxPtr.w6][8-n].w2*(max_qReal? 1:-1) + ccReal[maxPtr.w6][8-n].w2*(max_qImag? 1:-1);
    }

    // phase difference between two symbols
    ycReal.w3 = (max_qReal? 1:-1)*(pState->ppReal? 1:-1) + (max_qImag? 1:-1)*(pState->ppImag? 1:-1);
    ycImag.w3 = (max_qImag? 1:-1)*(pState->ppReal? 1:-1) - (max_qReal? 1:-1)*(pState->ppImag? 1:-1);

    // -----------
    // Bit Decoder
    // -----------
    cckOut->word = 0;
    cckOutValid->word = 1;

    cckOut->b0 = ((ycReal.w3 + ycImag.w3 > 0)? 0:1) ^ pState->NSym.b0;
    cckOut->b1 = ((ycReal.w3 - ycImag.w3 > 0)? 0:1) ^ pState->NSym.b0;

    switch (DataRate)
    {
        case  55:
            cckOut->b2 = maxPtr.b1;
            cckOut->b3 = maxPtr.b0;
            break;

        case 110:
            cckOut->b2 = maxPtr.b5;
            cckOut->b3 = maxPtr.b4;
            cckOut->b4 = maxPtr.b3;
            cckOut->b5 = maxPtr.b2;
            cckOut->b6 = maxPtr.b1;
            cckOut->b7 = maxPtr.b0;
            break;
    }
    // serialize the output into the "b" array
    for (n=0; n<(DataRate==55 ? 4:8); n++)
        pbRX->b[pbRX->db++].word = (cckOut->w8>>n) & 1;

    // save the current phase
    pState->ppReal = max_qReal;
    pState->ppImag = max_qImag;

    CCKWalOutR->word = xcReal[maxPtr.w6].w14 >> 3; // 12bits are used to share carrier
    CCKWalOutI->word = xcImag[maxPtr.w6].w14 >> 3; // phase detector with DSSS signal.

    *pccReal = ccReal[maxPtr.w6];
    *pccImag = ccImag[maxPtr.w6];

    // -------------
    // Trace Signals
    // -------------
    if (Verona_RxState()->EnableTrace)
    {
        for (n=0; n<8; n++)
        {
            k = 8*pState->NSym.word + n;
            pbRX->traceCCKInput[k].wReal = wR[n-7].w8;
            pbRX->traceCCKInput[k].wImag = wI[n-7].w8;
            pbRX->traceCCKInput[k].yReal = yReal[n].w8;
            pbRX->traceCCKInput[k].yImag = yImag[n].w8;
            pbRX->traceCCKTVMF[k].zReal  = zReal[n].w12;
            pbRX->traceCCKTVMF[k].zImag  = zImag[n].w12;
            pbRX->traceCCKTVMF[k].NSym   = pState->NSym.w8; // mod-256 value of symbol counter
        }
        for (n=0; n<Nc; n++)
        {
            k = Nc*pState->NSym.word + n;
            pbRX->traceCCKFWT[k].xcReal = xcReal[n].w14;
            pbRX->traceCCKFWT[k].met    = met[n].w15;
            pbRX->traceCCKFWT[k].Nc64   = Nc == 64 ? 1:0;
        }
    }
    pState->NSym.word++;
}
// end of Verona_bRX_CCK()

// ================================================================================================
// FUNCTION  : Verona_bRX_SFD()
// ------------------------------------------------------------------------------------------------
// Purpose   : Find start frame delimitor (SFD) and determine preamble type
// Parameters: SFDEnB      -- enabled SFD
//             DesOut      -- descrambler output (always single bit, 1Mbps mode)
//             DesOutValid -- data on DesOut is valid (strobe)
//             Preamble    -- preamble type (out) (0=long, 1=short)
//             SFDone      -- SFD found
// ================================================================================================
void Verona_bRX_SFD(wiBoolean SFDEnB, wiUWORD DesOut, wiBoolean DesOutValid, 
                    wiUReg *Preamble, wiUReg *SFDDone)
{
    const wiUWORD longSFD  = {0x05CF}; // 0000,0101,1100,1111
    const wiUWORD shortSFD = {0xF3A0}; // 1111,0011,1010,0000

    Verona_bRX_SFD_t *pState = &(Verona_bRxState()->RX_SFD);

    if (SFDEnB)             // initialize when not enabled
    {
        pState->NSym.word = 0;
        SFDDone->D.word = 0;
        pState->d.word = 0;
    }
    else
    {
        if (pState->NSym.w5 == 16) // check for SFD after loading 16 symbols
        {
            Verona_bRxState_t *pbRX = Verona_bRxState();

            wiBoolean matchLong  = (pState->d.w16 ==  longSFD.w16) ? 1:0;
            wiBoolean matchShort = (pState->d.w16 == shortSFD.w16) ? 1:0;

            SFDDone->D.word  = (matchShort | matchLong) ? 1:0;
            Preamble->D.word =  matchShort;

            pbRX->da = 0; // reset descrambler output array so it starts with the
            pbRX->db = 0; // ...PLCP header bits
        }
        if (DesOutValid) // shift in a new bit
        {
            pState->d.w16 = (pState->d.w15 << 1) | DesOut.b0;
            if (pState->NSym.w5 < 16) pState->NSym.w5++;
        }
    }
}
// end of Verona_bRX_SFD()

// ================================================================================================
// FUNCTION  : Verona_bRX_Header()
// ------------------------------------------------------------------------------------------------
// Purpose   : Decode the packet header
// Parameters: a             -- input, bit stream
//             shortPreamble -- indicates preamble {0=long, 1=short} if SFDDone=1
//             DataRate      -- data rate (100 kbps)
//             Modulation    -- modulation type: {0=CCK, 1=PBCC-not supported by Verona}
//             Length        -- packet length (Bytes)
//             LengthUSEC    -- packet duration (microseconds)
//             CRCpass       -- CRC {0=fail, 1=pass}
//             HDRInit       -- initialize SFD search
//             HDRDone       -- flag end of header
// ================================================================================================
void Verona_bRX_Header(wiBoolean HDREnB, wiBoolean DBPSK, wiBoolean shortPreamble, wiBoolean SSOutValid, 
                       wiUWORD *a, wiBitDelayLine *HDRDone, Verona_bRxState_t *RX)
{
    int *pNbit = &(RX->RX_Header.Nbit);

    if (HDREnB)
    {
        *pNbit = 0;
        HDRDone->word = (HDRDone->word << 1) | 0;
    }
    else
    {
        if (SSOutValid) (*pNbit) += (DBPSK ? 1:2); // increment data bit counter

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Extract Header Information
        //
        //  The header is protected by a 16-bit CRC which must show no errors.
        //  Additionally, the PSDU length (bytes) is checked for range errors. The
        //  valid range is for a PSDU length from 1-4095 bytes. Also, the duration
        //  field is limited to 32.767 ms (~4095 bytes at 1 Mbps).
        //
        if (*pNbit == 48)
        {
            wiBoolean ValidLength;
            wiUWORD X = {0};          // PLCP Header without CRC (bit order reversed)
            wiUWORD M = {0xFFFF}, pM; // register for CRC calculation
            int i, c, b;

            for (i=31; i>=0; i--) X.word = (X.word<<1) | a[i].b0;

            RX->bDataRate.word   = X.w8;        // data rate (100kbps units)
            RX->bModulation.word = X.b11;       // modulation select
            RX->bSERVICE.word    = X.w16 >> 8;  // SERVICE field
            RX->bLENGTH.word     = X.w32 >> 16; // PSDU duration (micorseconds)

            Verona_Registers()->RX_SERVICE.word = RX->bSERVICE.w8;

            switch (RX->bDataRate.w8) // compute LENGTH (bytes)
            {
                case  10: RX->LengthPSDU.word = RX->bLENGTH.w16/8; break;
                case  20: RX->LengthPSDU.word = RX->bLENGTH.w16/4; break;
                case  55: RX->LengthPSDU.word = RX->bLENGTH.w16*11/16; break;
                case 110: RX->LengthPSDU.word = RX->bLENGTH.w16*11/8 - RX->a[15].b0; break;
                default : RX->LengthPSDU.word = 1; break; // prevent ValidLength error for unsupported modes
            }
            ValidLength = ((RX->LengthPSDU.w17 > 0) && !RX->bLENGTH.b15) ? 1:0;

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Treat a PSDU length > 4095 as unsupported but valid (pending CRC)
            //  
            //  This is necessary because the balance of the baseband cannot process
            //  DSSS/CCK packets longer than 4095. However, it is possible to see a valid
            //  header longer than this. Specifically, the 802.11 standard allows an MPDU
            //  with 8192 bytes even though the actual MAC frame is limited < 4095 bytes
            //  elsewhere.
            //
            //  A specific consideration is that Wi-Fi certification includes a test with
            //  a faked PLCP header to verify CCA follows the PLCP indicated length. This
            //  test could imply a PSDU length > 4095 bytes though the commentator does
            //  not have details about the test.
            //
            if (RX->LengthPSDU.w17 > 4095) RX->bDataRate.word = 0xFF;

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Data rate (bRATE) encoding for the PHY RX Header
            //
            switch (RX->bDataRate.w8) // 4 bit RATE encoding
            {
                case  10: RX->bRATE.word = shortPreamble ? 0x0 : 0x0; break;
                case  20: RX->bRATE.word = shortPreamble ? 0x5 : 0x1; break;
                case  55: RX->bRATE.word = shortPreamble ? 0x6 : 0x2; break;
                case 110: RX->bRATE.word = shortPreamble ? 0x7 : 0x3; break;
                default:  RX->bRATE.word = 0xF;
            }

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Compute and Check CRC
            //
            //  The conditions for CRCpass are different in the Verona baseband. Here the
            //  length is subject to contraints to avoid zero duration packets or 
            //
            for (i=0; i<48; i++)
            {
                b = a[i].b0 ^ (i < 32 ? 0:1);
                c = b ^ M.b15;
                pM.word = M.w15 << 1;
                pM.b12  = c ^ M.b11;
                pM.b5   = c ^ M.b4;
                pM.b0   = c;
                M = pM;
            }
            RX->CRCpass = (!(M.w16 || X.b31) && ValidLength && RX->bLENGTH.w16) ? 1:0;

            if (RX->CRCpass)
                RX->bPacketDuration.word = max(0, RX->bLENGTH.w16 - 3 + 1); // [RTL] adjust this for actual delay

            HDRDone->word = (HDRDone->word << 1) | 1;
        }
        else HDRDone->word = (HDRDone->word << 1) | 0;
    }
}
// end of Verona_bRX_Header()

// ================================================================================================
// FUNCTION  : Verona_bRX_PSDU()
// ------------------------------------------------------------------------------------------------
// Purpose   : Flag end of PSDU decoding
// Parameters: PSDUEnB       -- enable PSDU-to-MAC/PHY interface mapping (active low)
//             DesOutValid   -- valid data at descrambler output
//             PSDUDone      -- flag end of PSDU                  
// ================================================================================================
void Verona_bRX_PSDU( wiBoolean PSDUEnB, wiBoolean DesOutValid, wiBitDelayLine *PSDUDone, 
                      Verona_RxState_t *RX)
{
    wiUWORD *pBitCount = &(Verona_bRxState()->RX_PSDU.BitCount);

    if (PSDUEnB)
    {
        pBitCount->word   = 0;
        PSDUDone->word = 0;
    }
    else
    {
        wiBoolean done = 0;

        if (DesOutValid) 
        {
            unsigned int n;
            Verona_bRxState_t *pbRX = Verona_bRxState();

            switch (pbRX->bDataRate.w8)
            {
                case  10: pBitCount->w15 += 1; break; //  1 Mbps: 1 bit/11T
                case  20: pBitCount->w15 += 2; break; //  2 Mbps: 2 bit/11T
                case  55: pBitCount->w15 += 4; break; //5.5 Mbps: 4 bit/8T
                case 110: pBitCount->w15 += 8; break; // 11 Mbps: 8 bit/8T
            }
            n = pBitCount->w15 >> 3; // number of bytes

            if (!pBitCount->w3) // modulo-8 subcount for bytes
            {
                int k;
                RX->PHY_RX_D[n+7].word = 0;
                for (k=0; k<8; k++)
                    RX->PHY_RX_D[n+7].word = RX->PHY_RX_D[n+7].w8 | (pbRX->a[48+8*(n-1)+k].b0 << k);

                RX->N_PHY_RX_WR = 8 + n; // number of bytes transfered on PHY_RX_D
            }
            done = (n == pbRX->LengthPSDU.w12) ? 1:0; // end of packet?
            if (done) RX->RTL_Exception.DecodedDSSS_CCK = 1; // flag completion of DSSS/CCK decode
            if (done) RX->RTL_Exception.PacketCount = min(3, RX->RTL_Exception.PacketCount+1);
        }
        PSDUDone->word = (PSDUDone->word << 1) | done;
    }
}
// end of Verona_bRX_PSDU()

// ================================================================================================
// FUNCTION  : Verona_bRX_DPLL()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: PLLEnB
//             PSDUEnB
//             ...
//             SSOutValid - SSCorOutValid delayed 1T@11MHz
//
// Notes     : Timing for DPLL updates is referenced to the positive edge of either
//             SSCorOutValid or cckWalOutValid which are referenced in the top level to
//             DMSym and cckSym.
// ================================================================================================
void Verona_bRX_DPLL(wiBoolean PLLEnB, wiWORD *SSMatOutR, wiWORD *SSMatOutI, wiBitDelayLine SSCorOutValid, 
                     wiWORD SSCorOutR, wiWORD SSCorOutI, wiBitDelayLine cckWalOutValid, wiWORD *CCKMatOutR, 
                     wiWORD *CCKMatOutI, wiWORD CCKWalOutR, wiWORD CCKWalOutI, wiWORD ccReal[], wiWORD ccImag[],
                     wiWORD *cCP, wiWORD *cSP, VeronaRegisters_t *pReg)
{
    Verona_bRX_DPLL_t *pState = &(Verona_bRxState()->RX_DPLL);
    
    wiWORD *cV = &(pState->cV); // vco for carrier
    wiWORD *cI = &(pState->cI); // integral path memory for carrier

    wiWORD *sV = &(pState->sV); // vco for sample timing
    wiWORD *sI = &(pState->sI); // integral path memory for sample timing

    // ---------------------------------------
    // Initialize Loop Filter and VCO Memories
    // ---------------------------------------
    if (PLLEnB)
    {
        cCP->word = cV->word = cI->word = 0;
        cSP->word = sV->word = sI->word = 0;
        return;
    }
    // =========================================================================
    // CARRIER PHASE LOCKED LOOP
    // =========================================================================
    if (SSCorOutValid.delay_11T || cckWalOutValid.delay_5T)
    {
        wiWORD  eCP;          // estimate of carrier phase error
        wiWORD  cP;             // proportional path for carrier
        wiWORD  cF;             // loop filter output for carrier
        wiWORD  pcReal,pcImag;// phase difference for carrier phase detector
        wiWORD  rReal, rImag; // selected correlator output
        int     qReal, qImag; // quantized coordinates for sample phae rotation (hard decode)
        wiUWORD aC, bC;       // loop filter proportional, integtral gains

        wiBoolean UseDSSS = SSCorOutValid.delay_11T; // DSSS or CCK?

        // ------------------------
        // Select Loop Filter Gains
        // ------------------------
        if (UseDSSS)
        {
            aC.word = pReg->aC1.w3;
            bC.word = pReg->bC1.w3;
        }
        else   // CCK
        {
            aC.word = pReg->aC2.w3;
            bC.word = pReg->bC2.w3;
        }
        // ----------------------------
        // Carrier Phase Error Detector
        // ----------------------------
        rReal = UseDSSS? SSCorOutR : CCKWalOutR; // replace second term with CCK outputs
        rImag = UseDSSS? SSCorOutI : CCKWalOutI;

        qReal = (rReal.w12 < 0)? -1:1; // QPSK demapper (coordinate quantizer)
        qImag = (rImag.w12 < 0)? -1:1; // (matched to RTL, better with BPSK to use SSSliOut, need to check) !!!!!!!!

        pcReal.w13 = qReal * rReal.w12 + qImag * rImag.w12; // x * conj(q)
        pcImag.w13 = qReal * rImag.w12 - qImag * rReal.w12;

        eCP.word = pcReal.w13 ? (8*pcImag.w13/pcReal.w13) : 0; // RTL: Divider
        eCP.word = limit(eCP.w16, -7, 7);;

        // -----------------------------------------
        // Carrier PLL : Loop Filter and Digital VCO
        // -----------------------------------------
        cP.w13 = (eCP.w4 << 9) >> aC.w3;
        cI->w14 = cI->w13 + ((eCP.w4 << 9) >> (2 + bC.w3));
        cI->w13 = limit(cI->w14, -4095, 4095);
        cF.w14 = cP.w13 + cI->w13;
        cF.w13 = limit(cF.w14, -4095, 4095);
        cV->w15 = cV->w15 + cF.w13;      // This implements wrap-around MSB.
    }
    // =========================================================================
    // SAMPLE TIMING PHASE LOCKED LOOP
    // =========================================================================
    if (SSCorOutValid.delay_18T || cckWalOutValid.delay_18T)
    {
        wiWORD  eSP;     // estimate of sampling phase error
        wiWORD  sJ;        // integral path
        wiWORD  sP;      // proportional path for sampling
        wiWORD  sF;      // loop filter output for sampling
        wiWORD  ps1Real, ps1Imag, ps2Real, ps2Imag;   // early/late correlations
        wiUWORD div, mag;// normalization factor of sampling phase estimate
        wiUWORD aS, bS;  // loop filter proportional, integtral gains
        int     n;

        const int BSe[12] = {-1,-2,-2, 0, 2, 2, 0, 0, 2, 0, 0, 1}; // interpolated Barker sequence
        wiBoolean UseDSSS = SSCorOutValid.delay_18T; // DSSS or CCK?

        // ------------------------
        // Select Loop Filter Gains
        // ------------------------
        if (UseDSSS)
        {
            aS.word = pReg->aS1.w3;
            bS.word = pReg->bS1.w3;
        }
        else   // CCK
        {
            aS.word = pReg->aS2.w3;
            bS.word = pReg->bS2.w3;
        }
        // ---------------------------
        // Timing Phase Error Detector
        // ---------------------------
        if (UseDSSS)
        {
            ps1Real.w13 = 0;
            ps1Imag.w13 = 0;
            ps2Real.w13 = 0;
            ps2Imag.w13 = 0;

            SSMatOutR += 5; SSMatOutI += 5; // delay to align DPLL with CMF output

            for (n=0; n<12; n++)
            {
                ps1Real.w13 += (BSe[n] * SSMatOutR[n+1].w9) / 2;
                ps1Imag.w13 += (BSe[n] * SSMatOutI[n+1].w9) / 2;

                ps2Real.w13 += (BSe[n] * SSMatOutR[n].w9) / 2;
                ps2Imag.w13 += (BSe[n] * SSMatOutI[n].w9) / 2;
            }
            ps1Real.w12 = ps1Real.w13 >> 1;
            ps1Imag.w12 = ps1Imag.w13 >> 1;
            ps2Real.w12 = ps2Real.w13 >> 1;
            ps2Imag.w12 = ps2Imag.w13 >> 1;

            mag.w13 = abs(SSCorOutR.w12) + abs(SSCorOutI.w12);
        }
        else
        {

            ps1Real.w15 = CCKMatOutR[7].w12*ccReal[7].w2 / 2 + CCKMatOutI[7].w12*ccImag[7].w2 / 2;
            ps1Imag.w15 = CCKMatOutI[7].w12*ccReal[7].w2 / 2 - CCKMatOutR[7].w12*ccImag[7].w2 / 2;
            ps2Real.w15 = CCKMatOutR[0].w12*ccReal[0].w2 / 2 + CCKMatOutI[0].w12*ccImag[0].w2 / 2;
            ps2Imag.w15 = CCKMatOutI[0].w12*ccReal[0].w2 / 2 - CCKMatOutR[0].w12*ccImag[0].w2 / 2;
    
            for (n=0; n<7; n++)
            {
                ps1Real.w15 += CCKMatOutR[n  ].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 + CCKMatOutI[n  ].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
                ps1Imag.w15 += CCKMatOutI[n  ].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 - CCKMatOutR[n  ].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
                ps2Real.w15 += CCKMatOutR[n+1].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 + CCKMatOutI[n+1].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
                ps2Imag.w15 += CCKMatOutI[n+1].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 - CCKMatOutR[n+1].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
            }
            ps1Real.w12 = ps1Real.w15 >> 3;
            ps1Imag.w12 = ps1Imag.w15 >> 3;
            ps2Real.w12 = ps2Real.w15 >> 3;
            ps2Imag.w12 = ps2Imag.w15 >> 3;

            mag.w13 = abs(CCKWalOutR.w12) + abs(CCKWalOutI.w12);
        }

        // Combine "Early","Late","Nominal" correlator terms
        eSP.w13 =(abs(ps1Real.w12) + abs(ps1Imag.w12));
        eSP.w13-=(abs(ps2Real.w12) + abs(ps2Imag.w12));
        eSP.w19 = 64*eSP.w13;

        if (mag.w13) 
        {
            div.w13 = 1 << PriorityEncoder(mag.w13);
            eSP.w19 = eSP.w19/((int)div.w13); // RTL: Divider; 090203: add (int) cast per XYU
        }
        else eSP.w19 = 0;
        eSP.w7 = limit(eSP.w19, -63, 63);

        // ---------------------------
        // Loop Filter and Digital VCO
        // ---------------------------
        sP.w16  = (eSP.w7 << 9) >>  aS.w3;
        sJ.w14  = (eSP.w7 << 9) >> (bS.w3 + 2);
        sI->w17 = sI->w16 + sJ.w14;
        sI->w16 = limit(sI->w17, -32767, 32767);
        sF.w17  = sP.w16 + sI->w16;
        sF.w16  = limit(sF.w17,-32767, 32767);
        sV->w27 = sV->w27 + sF.w16;
    }
    cCP->word = cV->w15 >>  9;
    cSP->word = sV->w27 >> 13;
}
// end of Verona_bRX_DPLL()

// ================================================================================================
// FUNCTION  : Verona_bRX_Descrambler()
// ------------------------------------------------------------------------------------------------
// Purpose   : RX Descrambler for the data bit stream
// Parameters: n - number of bits to process
//             b - input bit stream
//             a - output bit stream (descrambled)
// ================================================================================================
void Verona_bRX_Descrambler(wiUWORD SSOut, wiBoolean SSOutValid, wiUWORD cckOut, wiBoolean cckOutValid,
                            wiBoolean DBPSK, wiUReg *DesOut, wiUReg *DesOutValid, Verona_bRxState_t *RX, wiUInt FrontState)
{
    if (FrontState == 17 || FrontState == 20) 
        RX->Descrambler.S.word = 0; // clear scrambler state on FrontCtrl state 17,20 (AGC done)

    if (SSOutValid || cckOutValid)
    {
        int k, b, c, n, y; 
        wiUWORD X, Y = {0};
        wiUWORD *S = &(RX->Descrambler.S);

        X.word = SSOutValid ? SSOut.w2 : cckOut.w8;
        n = SSOutValid ? (DBPSK ? 1:2) : (RX->bDataRate.w8 == 110 ? 8:4);

        for (k=0; k<n; k++)
        {
            b = (X.w8 >> k) & 1;      // input bit
            c = S->b3 ^ S->b6;        // scrambler value
            y = b^c;                  // output bit
            S->w7 = (S->w6 << 1) | b; // update state of shift register

            Y.word = Y.word | y << k; // output word
            RX->a[RX->da++].word = y; // output bit in serial array
        }
        DesOut->D.word = Y.w8;        // output word to SFD/Header modules
        DesOutValid->D.word = 1;
    }
    else DesOutValid->D.word = 0;
}
// end of Verona_bRX_Descrambler()

// ================================================================================================
// FUNCTION  : Verona_b_Downsample()
// ------------------------------------------------------------------------------------------------
// Purpose   : Downsample from 44 to 22 and 11 MHz. In RTL this function is "Decim"
//             This function operates on the 44 MHz clock
// Parameters: posedgeCLK22MHz -- input, 22 MHz clock
//             posedgeCLK11MHz -- input, 11 MHz clock
//             DeciOutEnB      -- enable DeciOut and response to IntpFreeze and SymPhase
//             IntpFreeze      -- output stop from Interpolator
//             SymPhase        -- phase of 11 MHz samples downsampled from 22 MHz
//             IntpOutR        -- input,  real, 8-bit, 44 MHz
//             IntpOutI        -- input,  imag, 8-bit, 44 MHz
//             Deci22OutR      -- output, real, 8-bit, 22 MHz
//             Deci22OutI      -- output, imag, 8-bit, 22 MHz
//             DeciOutR        -- output, real, 8-bit, 11 MHz
//             DeciOutI        -- output, imag, 8-bit, 11 MHz
//             DeciFreeze      -- output, DeciOutR/I output stop
// ================================================================================================
void Verona_b_Downsample( wiBoolean posedgeCLK22MHz, wiBoolean posedgeCLK11MHz, wiUInt DeciOutEnB,
                          wiBoolean IntpFreeze, wiUInt SymPhase, wiWORD IntpOutR, wiWORD IntpOutI, 
                          wiWORD *Deci22OutR, wiWORD *Deci22OutI, wiWORD *DeciOutR, wiWORD *DeciOutI, 
                          wiUReg *DeciFreeze)
{
    Verona_b_Downsample_t *pState = &(Verona_bRxState()->Downsample);

    //if (posedgeCLK44MHz) implied by function call
    {
        ClockRegister(pState->d44R); 
        ClockRegister(pState->d44I);
        ClockRegister(pState->s22);
    }
    if (posedgeCLK22MHz) 
    {
        ClockRegister(pState->d22R[0]); 
        ClockRegister(pState->d22I[0]);
        ClockRegister(pState->d22R[1]); 
        ClockRegister(pState->d22I[1]);
        ClockRegister(pState->Deci22Freeze);
        ClockRegister(pState->s11);
    }
    if (posedgeCLK11MHz) 
    {
        ClockRegister(pState->d11R); 
        ClockRegister(pState->d11I);
    }
    pState->d44R.D    = IntpOutR;
    pState->d44I.D    = IntpOutI;

    pState->d22R[0].D = pState->s22.Q.b0 ? IntpOutR : pState->d44R.Q;
    pState->d22I[0].D = pState->s22.Q.b0 ? IntpOutI : pState->d44I.Q;
    
    pState->d22R[1].D = pState->d22R[0].Q;
    pState->d22I[1].D = pState->d22I[0].Q;

    pState->d11R.D    = pState->s11.Q.b0 ? pState->d22R[0].Q : pState->d22R[1].Q;
    pState->d11I.D    = pState->s11.Q.b0 ? pState->d22I[0].Q : pState->d22I[1].Q;

    if (DeciOutEnB)
    {
        pState->s11.D.word = 0;
        pState->s22.D.word = 0;
        pState->Deci22Freeze.D.word = 0;
        DeciFreeze->D.word = 0;
    }
    else
    {
        switch (pState->s22.Q.w2)
        {
            case 0: pState->Deci22Freeze.D.word = 0; if (IntpFreeze) {pState->s22.D.word = 1;} break;
            case 1: pState->Deci22Freeze.D.word = 0; if (IntpFreeze) {pState->Deci22Freeze.D.word = 1; pState->s22.D.word = 3;} break;
            case 2: break; // unused
            case 3: pState->Deci22Freeze.D.word = 1; pState->s22.D.word = 0; break;
        }
        switch (pState->s11.Q.word)
        {
            case 0: DeciFreeze->D.word = 0; if (SymPhase || pState->Deci22Freeze.Q.b0) pState->s11.D.word = 1; break;
            case 1: DeciFreeze->D.word = 0; if (pState->Deci22Freeze.Q.b0) {DeciFreeze->D.word = 1; pState->s11.D.word = 3;} break;
            case 2: DeciFreeze->D.word = 0; if (pState->Deci22Freeze.Q.b0) pState->s11.D.word = 1; break;
            case 3: DeciFreeze->D.word = 1; pState->s11.D.word = 2; break;
        }
    }
    // ------------
    // Wire Outputs
    // ------------
    Deci22OutR->word = pState->d22R[0].Q.w8;
    Deci22OutI->word = pState->d22I[0].Q.w8;

    DeciOutR->word = DeciOutEnB ? 0 : pState->d11R.Q.w8;
    DeciOutI->word = DeciOutEnB ? 0 : pState->d11I.Q.w8;
}
// end of Verona_b_Downsample()

// ================================================================================================
// FUNCTION  : Verona_bRX_top()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level DSSS/CCK receiver and front-end control
// Parameters: bRXEnB      -- enable DSSS/CCK receiver
//             qReal,qImag -- input signal (from OFDM downmix output)
//             bAGain      -- AGain from bAGC
//             bLNAGain    -- LNA gain from bAGC
//             LgSigAFE    -- large signal detect from radio
//             RSSICCK     -- RSSI reported from energy detect
//             Run11b      --
// ================================================================================================
void Verona_bRX_top( Verona_RxState_t *pRX, Verona_bRxState_t *RX, VeronaRegisters_t *Reg, 
					 wiBoolean bRXEnB, wiWORD *qReal, wiWORD *qImag, 
					 wiUWORD *bAGain, wiUWORD *bLNAGain, wiBoolean  LgSigAFE, wiUWORD *RSSICCK, 
					 wiBoolean Run11b, wiBoolean Restart, 
                     Verona_bSigState_t *bSigOut, Verona_bSigState_t bSigIn)
{
    Verona_bRxState_t *pbRX = Verona_bRxState();
    Verona_bRxTop_t   *pRxTop = &(pbRX->RxTop);

    Verona_b_FrontCtrlRegType *inReg = &(pRxTop->inReg);
    Verona_b_FrontCtrlRegType outReg;

    // -----------------------------------------------------
    // Local Copies of Clock Edges (renamed for readability)
    // -----------------------------------------------------
    wiBoolean posedgeCLK40MHz = pRX->clock.posedgeCLK40MHz; // 40 MHz clock edge
    wiBoolean posedgeCLK44MHz = pRX->clock.posedgeCLK44MHz; // 44 MHz clock edge
    wiBoolean posedgeCLK22MHz = pRX->clock.posedgeCLK22MHz; // 22 MHz clock edge
    wiBoolean posedgeCLK11MHz = pRX->clock.posedgeCLK11MHz; // 11 MHz clock edge

    unsigned int k40 = pRX->k40;
    unsigned int k44 = pRX->k44;
    unsigned int k22 = pRX->k22;
    unsigned int k11 = pRX->k11;

    // --------------------------------------------
    // Detect Signal Power Increase/Decreases
    // (run only in 11b mode and after AGC is done)
    // --------------------------------------------
    if (!Reg->RXMode.b0 && bSigIn.ValidRSSI && Reg->bPwrStepDet.b0)
    {
        unsigned StepDnTh = (pRxTop->AbsEnergyOld.w8 < 2*Reg->bThStepDown.w4)? 0 : pRxTop->AbsEnergyOld.w8 - 2*Reg->bThStepDown.w4;
        bSigOut->StepUp   =((pRxTop->AbsPwr.Q.w8 > pRxTop->AbsEnergyOld.w8+2*Reg->bThStepUp.w4) && (pRxTop->AbsEnergyOld.w8<Reg->bThStepUpRefPwr.w8))? 1:0;
        bSigOut->StepDown = (pRxTop->AbsPwr.Q.w8 < StepDnTh)? 1:0;
    }
    else bSigOut->StepUp = bSigOut->StepDown = 0;
 
    // ---------------
    // Clock Registers
    // ---------------
    if (posedgeCLK40MHz)
    {
    }
    if (posedgeCLK44MHz)
    {
        ClockRegister(pRxTop->LNAGain);     // AGC
        ClockRegister(pRxTop->AGain);       // AGC
        ClockRegister(pRxTop->DGain);       // AGC
        ClockRegister(pRxTop->GainUpdate);  // AGC
        ClockRegister(pRxTop->LargeSignal); // AGC
        ClockRegister(pRxTop->AbsPwr);      // AGC
        ClockRegister(pRxTop->cSP);         // DPLL/Resample
        ClockRegister(pRxTop->Phase44);     // clock phase
        ClockDelayLine(pRxTop->posedge_SSCorOutValid,   pRxTop->SSCorOutValid.D.b0  & posedgeCLK11MHz);
        ClockDelayLine(pRxTop->posedge_cckWalOutValid,  pRxTop->cckWalOutValid.D.b0 & posedgeCLK11MHz);
        pRxTop->Phase44.D.w2 = posedgeCLK11MHz ? 1 : pRxTop->Phase44.Q.w2 + 1; // phase of 44 MHz clock wrt 11 MHz
    }
    if (posedgeCLK22MHz)
    {
        ClockRegister(pRxTop->state);
        ClockRegister(pRxTop->count);
        ClockRegister(pRxTop->EDOut);       // EnergyDetect
        ClockRegister(pRxTop->CQOut);       // CorrelationQuality
        ClockRegister(pRxTop->CQPeak);      // CorrelationQuality
        ClockRegister(pRxTop->CFO);         // EstimateCFO
        ClockRegister(pRxTop->SFDcount);    // SFD timeout
        ClockRegister(pRxTop->LgSigDetAFE); // AFE Large signal detector
        pRxTop->outReg = pRxTop->inReg;
    }
    outReg = pRxTop->outReg; // shorten packed register name

    if (posedgeCLK11MHz)
    {
        ClockRegister(pRxTop->SSOut);
        ClockRegister(pRxTop->SSOutValid);
        ClockRegister(pRxTop->SSCorOutR);
        ClockRegister(pRxTop->SSCorOutI);
        ClockRegister(pRxTop->SSCorOutValid);
        ClockRegister(pRxTop->SSSliOutR);
        ClockRegister(pRxTop->SSSliOutI);
        ClockRegister(pRxTop->cckOut);
        ClockRegister(pRxTop->cckWalOutValid);
        ClockRegister(pRxTop->cckOutValid);
        ClockRegister(pRxTop->DesOut);
        ClockRegister(pRxTop->DesOutValid);
        ClockRegister(pRxTop->SymCount);       // DSSS symbol boundary counter
        ClockRegister(pRxTop->SymCountCCK);    // CCK  symbol boundary counter
        ClockRegister(pRxTop->LengthCount);    // PSDU timer for unsupported modulation (PBCC)
        ClockRegister(pRxTop->Preamble);
        ClockRegister(pRxTop->SFDDone);
        ClockRegister(pRxTop->DeciFreeze);
        ClockDelayLine(pRxTop->dDeciFreeze, pRxTop->DeciFreeze.Q.b0);

        // ---------------------------------------------
        // Conditionally Clock Symbol Marker Delay Lines
        // ...stop if DeciFreeze pauses demodulation
        // ---------------------------------------------
        if (!pRxTop->dDeciFreeze.delay_0T ) ClockDelayLine(pRxTop->DMSym, outReg.SymCMF);
        ClockDelayLine(pRxTop->cckSym, outReg.SymCCK);
    }
    // -----------
    // Wire Output
    // -----------
    bAGain->word    = pRxTop->AGain.Q.w6;
    bLNAGain->word  = pRxTop->LNAGain.Q.w2; // b0=Radio LNA, b1=External LNA


    // ------------------------------------------
    // Increment State Machine Counters (default)
    // ------------------------------------------
    if (posedgeCLK22MHz)
    {
        pRxTop->count.D.word = pRxTop->count.Q.w11 + 1;
        pRxTop->SFDcount.D.word = pRxTop->SFDCntEnB? 0:(pRxTop->SFDcount.Q.w13+1);
    }
    // -------------
    // Signal Detect
    // -------------
    {
        pRxTop->pCCAED = (pRxTop->AbsPwr.Q.w8 >=  Reg->EDthreshold.w6) && !Reg->RXMode.b0; // compare only for 11b
        pRxTop->pCCACS = (pRxTop->CQOut.Q.w13 >= (Reg->CQthreshold.w2<<(Reg->CQthreshold.w6>>2)));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    ////
    ////  FRONT END CONTROL STATE MACHINES ( "FrontCtrl" )
    ////
    ////  Note that the switch-case structure is overclocked to provide pseudo-concurrent
    ////  signal updates from the function calls below. However, the state and counter
    ////  registers are clocked at 22 MHz, so that is the execution speed of the controller
    ////
    if (posedgeCLK22MHz || (posedgeCLK44MHz && pRxTop->state.Q.w6<24))   // run at 22MHz clock
    {
        switch (pRxTop->state.Q.w6)
        {
            case 0:   // initial state ------------------------------------------------------------

                bSigOut->SigDet      = 0;
                bSigOut->AGCdone     = 0;
                bSigOut->ValidRSSI   = 0;
                bSigOut->SyncFound   = 0;
                bSigOut->HeaderValid = 0;
                bSigOut->RxDone      = 0;
                bSigOut->CSCCK       = 0;

                inReg->BCEnB      = 1; // disable Barker correlator
                inReg->EDEnB      = 1; // disable energy detect
                inReg->CQEnB      = 1; // disable correlation quality
                inReg->AGCEnB     = 1; // initialization: AGC
                inReg->AGCInit    = 1;
                inReg->AGCbounded = 0;
                inReg->ArmLgSigDet= 0;
                pRxTop->AbsEnergy.w8     = 0;
                pRxTop->AbsEnergyOld.w8  = 0;
                
                inReg->CEEnB   = 1;    // disable channel estimation
                inReg->CEMode  = 0;
                inReg->CEGet   = 0;
                pRxTop->CEDone = 0;
                pRxTop->Preamble.D.word = 0;
                
                inReg->CFOeEnB = 1;   // initialize CFO correction
                inReg->CFOcEnB = 1;
                inReg->CFOeGet = 0;
                
                inReg->SPEnB    = 1;  // demodulator symbol timing
                inReg->DMEnB    = 1;  // disable DSSS demodulator
                inReg->CMFEnB   = 1;  // disable CMF
                inReg->SymGo    = 0;  // disable DSSS/CCK symbol counters
                inReg->SymGoCCK = 0;
                inReg->SymPhase = 0;  // initialize symbol sync
                
                inReg->SFDEnB = 1;    // disable SFD search
                pRxTop->SFDCntEnB = 1;
                
                inReg->PLLEnB   = 1;   // disable DPLL
                inReg->HDREnB   = 1;   // disable header decoder
                inReg->PSDUEnB  = 1;   // disable PSDU decoder
                inReg->TransCCK = 0;
                pRxTop->cckInit = 0;
                pRxTop->INTERPinit = 1;      // initialize interpolator
                inReg->DeciOutEnB  = 1;

                pRxTop->CCKWalOutR.word = 0;
                pRxTop->CCKWalOutI.word = 0;
                pRxTop->count.D.word = 0; 

                // initial displacements of various memories (for simulation purposes)
                RX->da = 0; // descrambled bits
                RX->db = 0; // detected bits

                if (posedgeCLK22MHz) pRxTop->state.D.word = bRXEnB? 0:63; // hold control in state 0 (bRXEnB=1 and EnableTrace=1)
                break;

            case 63:  // receiver startup ---------------------------------------------------------

                inReg->AGCInit = 0;
                if (!pRxTop->count.Q.w11) // compute counter limits: in RTL these are combinational logic; they are placed here
                {                 // to reduce simulation overhead; the registers do not change in WiSE during a packet
                    pRxTop->CQWinCount  = 22 * (1 << Reg->CQwindow.w2);
                    pRxTop->EDWinCount  = 22 * (1 << Reg->EDwindow.w2);
                    pRxTop->SSWinCount  = 22 *((1 << Reg->SSwindow.w3)  + 1); // 22*(+1) accounts for Barker Correlator delay
                    pRxTop->SFDWinCount =(22 *       Reg->SFDWindow.w8) - 32;
                }
                if (pRxTop->count.Q.w11 >= Reg->INITdelay.w8 - 2 && !pRxTop->Phase44.Q.b1) inReg->BCEnB  = 0;
                if (pRxTop->count.Q.w11 >= Reg->INITdelay.w8 - 1 &&  pRxTop->Phase44.Q.b1)
                {
                    inReg->EDEnB  = Reg->RXMode.b0; // enable for 11b but not 11g mode
                    inReg->BCEnB  = 0;              // enable Barker correlator
                    inReg->BCMode = 0;              // select 22 MS/s input
                    inReg->CSEnB  = 0;              // configure CQ module for carrier sense
                    inReg->DBPSK  = 1;              // DBPSK (1 Mbps) modulation during preamble
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 1;
                }
                break;

            case 1: // estimate energy and correlation quality ------------------------------------

                if (pRxTop->count.Q.w11 == 22)                       inReg->CQEnB = 0; // enable CQ after Barker Correlator delay line is filled
                if (pRxTop->count.Q.w11 > (pRxTop->CQWinCount + 21)) inReg->CQEnB = 1; // correlation-based carrier sense complete
                if (pRxTop->count.Q.w11 >  pRxTop->EDWinCount)       inReg->EDEnB = 1; // energy based signal-detect complete
                if (pRxTop->count.Q.w11 > (pRxTop->EDWinCount + 1) 
                && (pRxTop->count.Q.w11 > (pRxTop->CQWinCount + 24))) // wait for CQ/ED outputs to become valid
                    pRxTop->state.D.word = 2;
                break;

            case 2:   // carrier sensing
                pRxTop->count.D.word = 0;
                if ( (pRxTop->pCCACS && pRxTop->pCCAED) 
                  || (pRxTop->pCCACS && Reg->CSMode.b0) 
                  || (pRxTop->pCCAED && Reg->CSMode.b1) )
                {
                    bSigOut->CSCCK = 1; // set carrier sense flag
                    inReg->BCEnB   = 1; // disable Barker correlator
                    inReg->CSEnB   = 1; // carrier sense complete; configure CQ for symbol sync
                    pRxTop->state.D.word  = 4; // proceed to AGC
                }
                else if (Reg->bSwDEn.b0 && (Reg->RXMode.w2 == 2))
                {
                    bSigOut->AntSel = bSigIn.AntSel^1; // switch antennas
                    pRxTop->state.D.word = 3; // wait for antenna switch propagation
                }
                else
                {
                    inReg->EDEnB  = Reg->RXMode.b0; // enable for 11b but not 11g mode
                    pRxTop->state.D.word = 1;              // return to carrier sense phase
                }
                break;

            case 3:   // antenna switching --------------------------------------------------------

                if (pRxTop->count.Q.w11 == 2*Reg->bSwDDelay.w5 - 1)
                {
                    inReg->BCEnB  = 0; 
                    inReg->BCMode = 0;      
                    inReg->CSEnB  = 0;
                    inReg->EDEnB  = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 1;
                }
                break;

            case 4:   // wait before AGC starts ---------------------------------------------------

                if (Reg->RXMode.w2 == 3) // [802.11g]
                {
                    pRxTop->count.D.word = 0;  // reset counter (assume ED input is invalid until Run11b)
                    if (posedgeCLK22MHz && Run11b) pRxTop->state.D.word = 7; // force bAGCdelay before start of AGC updates
                }
                else // [802.11b]
                {
                    inReg->AGCbounded = Reg->bAGCBounded4.b0;
                    if (pRxTop->count.Q.w11 == Reg->bWaitAGC.w6 - 1)
                    {
                        inReg->EDEnB  = 0;
                        pRxTop->count.D.word = 0;
                        pRxTop->state.D.word = 5;
                    }
                }
                bSigOut->SigDet = 1;
                break;

            case 5:   // estimate signal power ----------------------------------------------------

                inReg->ArmLgSigDet= 1; // arm the large signal detector
                if ((pRxTop->count.Q.w11 == pRxTop->EDWinCount + 1) || pRxTop->LgSigDetAFE.Q.b0)
                {
                    inReg->EDEnB     = 1;
                    inReg->AGCEnB    = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 6;
                }
                break;

            case 6:   // branch according to AGC result -------------------------------------------

                inReg->AGCEnB = 1;
                inReg->AGCbounded  = 0;
                inReg->ArmLgSigDet = 0; // reset the large signal detector
                pRxTop->count.D.word = 0;
                if      (pRxTop->LargeSignal.Q.b0) pRxTop->state.D.word = 7;
                else if (pRxTop->GainUpdate.Q.b0)  pRxTop->state.D.word = 8;
                else                               pRxTop->state.D.word = 9;
                break;

            case 7:   // wait until signal is settled ---------------------------------------------

                if (pRxTop->count.Q.w11 == Reg->bAGCdelay.w6-1)
                {
                    inReg->EDEnB = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 5;
                }
                break;

            case 8: // wait until signal is settled -----------------------------------------------

                if (pRxTop->count.Q.w11 == Reg->bAGCdelay.w6-1)
                {
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 9;
                }
                break;

            case 9: // check for antenna switch ---------------------------------------------------

                pRxTop->AbsEnergyOld.w8 = pRxTop->AbsEnergy.w8;
                if ((Reg->bSwDEn.b0 && Reg->RXMode.w2 == 2) && (pRxTop->AbsEnergy.w8 < Reg->bSwDTh.w8))
                {
                    bSigOut->AntSel = bSigIn.AntSel^1; // switch antenna
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 10;
                }
                else
                {
                    inReg->EDEnB = Reg->RXMode.b0; // re-enable power meter (ED) in 11b mode
                    pRxTop->state.D.word = 17;
                }
                break;

            case 10: // antenna switching ---------------------------------------------------------

                if (pRxTop->count.Q.w11 == 2*Reg->bSwDDelay.w5)
                {
                    inReg->EDEnB  = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 11;
                }
                break;

            case 11: // estimate signal power -----------------------------------------------------

                inReg->ArmLgSigDet = 1; // arm the large signal detector
                if (pRxTop->count.Q.w11 == pRxTop->EDWinCount + 1)
                {
                    inReg->EDEnB  = 1;
                    inReg->AGCEnB = 0;
                    inReg->AGCbounded = 1;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 12;
                }
                break;

            case 12: // branch according to AGC result --------------------------------------------

                inReg->AGCEnB = 1;
                inReg->AGCbounded  = 0;
                inReg->ArmLgSigDet = 0; // reset the large signal detector
                pRxTop->count.D.word = 0;
                     if (pRxTop->LargeSignal.Q.b0) pRxTop->state.D.word = 13;
                else if (pRxTop->GainUpdate.Q.b0)  pRxTop->state.D.word = 16;
                else                               pRxTop->state.D.word = 18;
                break;

            case 13: // wait until signal is settled ----------------------------------------------

                if (pRxTop->count.Q.w11 == Reg->bAGCdelay.w6 - 1)
                {
                    inReg->EDEnB  = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 14;
                }
                break;

            case 14: // estimate signal power -----------------------------------------------------

                inReg->ArmLgSigDet= 1; // arm the large signal detector
                if (pRxTop->count.Q.w11 > pRxTop->EDWinCount + 1)
                {
                    inReg->EDEnB = 1;
                    inReg->AGCEnB = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 15;
                }
                break;

            case 15: // branch according to AGC result --------------------------------------------

                inReg->ArmLgSigDet = 0; // reset the large signal detector
                inReg->AGCEnB = 1;
                pRxTop->count.D.word = 0;
                if     (pRxTop->LargeSignal.Q.b0) pRxTop->state.D.word = 13;
                else if (pRxTop->GainUpdate.Q.b0) pRxTop->state.D.word = 16;
                else {                            pRxTop->state.D.word = 17;
                    pRxTop->AbsEnergyOld.w8 = pRxTop->AbsEnergy.w8;
                }
                break;

            case 16: // wait until signal is settled ----------------------------------------------

                if (pRxTop->count.Q.w11 == Reg->bAGCdelay.w6 + pRxTop->EDWinCount + 1)
                {
                    pRxTop->AbsEnergyOld.w8 = pRxTop->AbsEnergy.w8;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 17;
                }
                break;

            case 17: // end of AGC ----------------------------------------------------------------

                inReg->EDEnB = Reg->RXMode.b0; // enable for 11b, disable for 11g
                bSigOut->AGCdone = 1;         // signal AGC (and antenna selection) complete
                RX->Nh = 0;                   // clear so CMF output is zero until CEDone
                pRxTop->state.D.word = 20;    // continue preamble processing
                break;

            case 18: // check for antenna switch --------------------------------------------------

                pRxTop->count.D.word = 0;
                if (pRxTop->AbsEnergy.w8 < pRxTop->AbsEnergyOld.w8)
                {
                    bSigOut->AntSel = bSigIn.AntSel ^ 1; // switch antenna
                    pRxTop->state.D.word = 19;
                }
                else
                {
                    pRxTop->AbsEnergyOld.w8 = pRxTop->AbsEnergy.w8;
                    pRxTop->state.D.word = 17;
                }
                break;

            case 19: // antenna switching ---------------------------------------------------------

                if (pRxTop->count.Q.w11 == 2*Reg->bSwDDelay.w5 - 1)
                {
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 17;
                }
                break;

            case 20: // start symbol sync and CFO estimation --------------------------------------

                inReg->BCEnB  = 0;
                inReg->BCMode = 0;
                pRxTop->count.D.word = 0;
                pRxTop->state.D.word = 21;
                break;

            case 21: // wait for the first valid correlation --------------------------------------

                if (pRxTop->count.Q.w11 == 23) // delay of 24T to match BC with 22 MS/s input
                {
                    inReg->CQEnB  = 0;
                    inReg->CFOeEnB= 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 22;
                }
                break;

            case 22: // wait until symbol sync and CFO estimation are finished --------------------

                if (pRxTop->count.Q.w11 == pRxTop->SSWinCount - 1)
                {
                    inReg->BCEnB   = 1;
                    inReg->CQEnB   = 1;
                    inReg->CFOeGet = 1;
                }
                if (pRxTop->count.Q.w11 == pRxTop->SSWinCount + 2)
                {
                    inReg->DeciOutEnB = 0;
                    if (pRxTop->CQPeak.Q.b0 ^ pRxTop->Phase44.Q.b1)
                    {
                        inReg->SymPhase = 1;
                        pRxTop->count.D.word = 0;
                        pRxTop->state.D.word = 23;
                    }
                    else
                    {
                        inReg->SymPhase = 0;
                        pRxTop->count.D.word = 0;
                        pRxTop->state.D.word = 23;
                    }
                }
                if (pRxTop->count.Q.w11 + 22 >= pRxTop->EDWinCount) bSigOut->ValidRSSI = Reg->RXMode.b0 ^ 1; // set in 802.11b mode
                break;

            case 23: // wait for the begining of precursors ---------------------------------------

                inReg->CFOeGet = 0;
                inReg->CFOcEnB = 0;
                inReg->CFOeEnB = 1;
                if (pRxTop->count.Q.w11 == (pRxTop->CQPeak.Q.w5 + 3) % 22)
                {
                    if (pRxTop->Phase44.Q.w2 == 3)
                    {
                        inReg->BCEnB = 0;   // start correlation at first precursor
                        inReg->BCMode = 1;   // correlation for channel estimation purpose
                        pRxTop->count.D.word = 0;
                        pRxTop->state.D.word = 24;
                    }
                    else pRxTop->count.D.word = pRxTop->count.Q.word;
                }
                break;

            case 24: // wait until the peak and first valid correlation ---------------------------

                if (pRxTop->count.Q.w11 == 18) {
                    inReg->DMEnB = 0;
                    inReg->SymGo = 1; // start counter for DMSym strobe (mine is delayed 22T_22 from Simon's)
                }
                if (pRxTop->count.Q.w11 == 20)
                {
                    inReg->SFDEnB = 0; // start SFD search at first precursor
                    inReg->SPEnB = 0;  // enable CCK FIFO
                    pRxTop->SFDCntEnB   = 0;  // enable SFD search timeout counter
                }
                if (pRxTop->count.Q.w11 == 22)
                {
                    inReg->CEEnB = 0;
                    inReg->CEMode = 0;
                    inReg->CMFEnB = 0; // will output zero until response is updated (okay to start early in WiSE)
                    pRxTop->state.D.word = 25;
                }
                break;

            case 25: // wait until first channel estimation is finished ---------------------------
                     // ...this is expected to be done before the SFD
                if (pRxTop->CEDone == 1)
                {
                    pRxTop->CEDone = 0;       // clear CEDone so it is not picked up entering state 26
                    inReg->BCEnB = 0;   // continue second channel estimation
                    inReg->CEMode = 1;
                    inReg->PLLEnB = 0;
                    pRxTop->state.D.word = 26;
                }
                break;

            case 26: // wait until SFD is found or channel estimation is ended --------------------
                if (pRxTop->SFDcount.Q.w13 > pRxTop->SFDWinCount)
                {
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 0;
                }
                else if (pRxTop->CEDone && pRxTop->SFDDone.Q.b0)
                {
                    inReg->CEEnB = 1;
                    inReg->SFDEnB = 1;
                    inReg->BCEnB = 1;
                    inReg->DBPSK = pRxTop->Preamble.Q.b0 ? 0:1;
                    inReg->HDREnB = 0;
                    bSigOut->SyncFound = 1; // signal valid SFD
                    pRxTop->state.D.word = 31;
                }
                else if (pRxTop->SFDDone.Q.b0)
                {
                    inReg->SFDEnB = 1;
                    pRxTop->count.D.word = 0;
                    inReg->HDREnB = 0;
                    inReg->DBPSK = pRxTop->Preamble.Q.b0 ? 0:1;
                    bSigOut->SyncFound = 1; // signal valid SFD
                    pRxTop->state.D.word = 28; 
                }
                else if (pRxTop->CEDone)
                {
                    inReg->CEEnB = 1;
                    inReg->BCEnB = 1;
                    pRxTop->state.D.word = 27;
                }
                break;

            case 27: // wait until SFD is found ---------------------------------------------------

                if (pRxTop->SFDcount.Q.w13 > pRxTop->SFDWinCount)
                {
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 0;
                }
                else if (pRxTop->SFDDone.Q.b0 == 1)
                {
                    inReg->SFDEnB = 1;
                    inReg->HDREnB = 0;
                    inReg->DBPSK = pRxTop->Preamble.Q.b0 ? 0:1;
                    bSigOut->SyncFound = 1; // signal valid SFD
                    pRxTop->state.D.word = 31; 
                }
                break;

            case 28: // start HDR decoding --------------------------------------------------------

                pRxTop->SFDCntEnB = 1;
                pRxTop->state.D.word = 29;
                break;

            case 29: // wait until channel estimation is finished ---------------------------------

                if (pRxTop->CEDone == 1)
                {
                    inReg->BCEnB = 1;
                    inReg->CEEnB = 1;
                    pRxTop->state.D.word = 32;
                }
                else if (pRxTop->count.Q.w11 == 22*Reg->SymHDRCE.w6-1)
                {
                    inReg->CEGet = 1;
                    pRxTop->state.D.word = 30;
                }
                pRX->N_PHY_RX_WR = 0; // clear counter for PHY_RX_D writes (WiSE)
                break;

            case 30: // wait until forced ending of channel estimation ----------------------------

                if (pRxTop->CEDone == 1)
                {
                    inReg->BCEnB = 1;
                    inReg->CEEnB = 1;
                    inReg->CEGet = 0;
                    pRxTop->state.D.word = 32;
                }
                break;

            case 31: // end of preamble -----------------------------------------------------------

                pRxTop->SFDCntEnB = 1;
                pRxTop->state.D.word = 32;
                pRX->N_PHY_RX_WR = 0; // clear counter for PHY_RX_D writes (WiSE)
                break;

            case 32: // decode header -------------------------------------------------------------

                if (pRxTop->HDRDone.delay_2T == 1)
                {
                    if (RX->CRCpass) // PLCP Valid: CRC correct
                    {
                        if ((RX->bDataRate.w8 == 10 || RX->bDataRate.w8 == 20)) // DSSS (1,2 Mbps)
                        {
                            inReg->PSDUEnB = 0;
                            inReg->DBPSK   = (RX->bDataRate.w8 == 10) ? 1:0;
                            pRxTop->state.D.word  = 34;
                        }
                        else if ((RX->bDataRate.w8 == 55 || RX->bDataRate.w8 == 110) && !RX->bModulation.b0) // CCK (5.5, 11 Mbps)
                        {
                            inReg->PSDUEnB = 0;
                            pRxTop->cckInit       = 1;
                            pRxTop->state.D.word  = 33;
                        }
                        else 
                        {
                            pRxTop->state.D.word = 40; // unsupported modulation
                        }
                        inReg->HDREnB = 1;         // disable header processing
                        bSigOut->HeaderValid = 1; // signal valid PLCP header
                    }
                    else // PLCP decoding error: CRC failed
                    {
                        pRxTop->state.D.word = 0;
                    }
                }
                bSigOut->CSCCK = 0; // clear carrier sense signal (prevent false detect on 2nd packet in 11g)
                pRxTop->count.D.word = 0;   // clear counter
                break;

            case 33: // transition from DSSS to CCK demodulation ----------------------------------

                inReg->TransCCK= 1;
                inReg->DMEnB   = 1;
                inReg->SymGo   = 0; // stop DSSS symbol counter
                if (pRxTop->count.Q.w11 ==  3) inReg->CMFEnB = 1; // delayed to flush SSMatOutX delay lines for last DPLL update
                if (pRxTop->count.Q.w11 == 12) inReg->SymGoCCK = 1;
                if (pRxTop->count.Q.w11 == (unsigned int)(11 + RX->Np2)*2) // assert TransCCK for (11+Np2)*T11
                {
                    inReg->TransCCK = 0;
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 34;
                }
                break;

            case 34: // decode PSDU ---------------------------------------------------------------

                if (pRxTop->PSDUDone.delay_2T)
                {
                    inReg->DMEnB = 1;
                    inReg->PSDUEnB = 1;
                    inReg->SymGo = 0;
                    inReg->SymGoCCK = 0;
                    bSigOut->SigDet = 0;
                    bSigOut->SyncFound = 0;
                    bSigOut->HeaderValid = 0;
                    pRxTop->state.D.word = 35;
                    //wiPrintf("\n ======================================= PSDU DONE ==========================================\n\n");
                    //wiPrintf("PSDU Done: SymPhase=%d, cSP =%4d %c\n",outReg.SymPhase,cSP.word,(outReg.SymPhase&&cSP.word<-32 || !outReg.SymPhase&&abs(cSP.word)<8)? '-':'<');
                }
                pRxTop->count.D.word   = 0; // clear general counter
                break;

            case 35: // end of packet -------------------------------------------------------------

                bSigOut->RxDone = 1; // signal end of PSDU to top-level control
                inReg->CMFEnB = 1;
                if (pRxTop->count.Q.w11 == 11) // 0.5 us delay at end-of-packet
                {
                    if (pRX->NonRTL.Enabled && pRX->NonRTL.MojaveFrontCtrlEOP)
                        pRxTop->state.D.word =  0; // return to top of control (Mojave convention)
                    else
                        pRxTop->state.D.word = 36;
                }
                break;

            case 36: // re-initialize at end of DSSS/CCK packet (similar to state 0) --------------

                bSigOut->SigDet      = 0;
                bSigOut->AGCdone     = 0;
                bSigOut->ValidRSSI   = 0;
                bSigOut->SyncFound   = 0;
                bSigOut->HeaderValid = 0;
                bSigOut->RxDone      = 0;
                bSigOut->CSCCK       = 0;

                inReg->BCEnB      = 1; // disable Barker correlator
                inReg->EDEnB      = 1; // disable energy detect
                inReg->CQEnB      = 1; // disable correlation quality
                inReg->AGCEnB     = 1; // initialization: AGC
                inReg->AGCInit    = 1;
                inReg->AGCbounded = 0;
                inReg->ArmLgSigDet= 0;
                pRxTop->AbsEnergy.w8     = 0;
                pRxTop->AbsEnergyOld.w8  = 0;
                
                inReg->CEEnB  = 1;    // disable channel estimation
                inReg->CEMode = 0;
                inReg->CEGet  = 0;
                pRxTop->CEDone = 0;
                pRxTop->Preamble.D.word = 0;
                
                inReg->CFOeEnB = 1;   // initialize CFO correction
                inReg->CFOcEnB = 1;
                inReg->CFOeGet = 0;
                
                inReg->SPEnB    = 1;  // demodulator symbol timing
                inReg->DMEnB    = 1;  // disable DSSS demodulator
                inReg->CMFEnB   = 1;  // disable CMF
                inReg->SymGo    = 0;  // disable DSSS/CCK symbol counters
                inReg->SymGoCCK = 0;
                inReg->SymPhase = 0;  // initialize symbol sync
                
                inReg->SFDEnB = 1;    // disable SFD search
                pRxTop->SFDCntEnB = 1;
                
                inReg->PLLEnB  = 1;   // disable DPLL
                inReg->HDREnB  = 1;   // disable header decoder
                inReg->PSDUEnB = 1;   // disable PSDU decoder
                inReg->TransCCK = 0;
                pRxTop->cckInit = 0;
                pRxTop->INTERPinit = 1;      // initialize interpolator
                inReg->DeciOutEnB = 1;

                pRxTop->CCKWalOutR.word = 0;
                pRxTop->CCKWalOutI.word = 0;
                pRxTop->count.D.word = 0; 

                // initial displacements of various memories (for simulation purposes)
                RX->da = 0; // descrambled bits
                RX->db = 0; // detected bits

                if (posedgeCLK22MHz) pRxTop->state.D.word = 37;
                break;

            case 37:  // receiver restart (similar to state 1) ------------------------------------

                inReg->AGCInit = 0;
                if (pRxTop->count.Q.w11 >= Reg->bRestartDelay.w8 - 2 && !pRxTop->Phase44.Q.b1) inReg->BCEnB  = 0;
                if (pRxTop->count.Q.w11 >= Reg->bRestartDelay.w8 - 1 &&  pRxTop->Phase44.Q.b1)
                {
                    inReg->EDEnB  = Reg->RXMode.b0; // enable for 11b but not 11g mode
                    inReg->BCEnB  = 0;              // enable Barker correlator
                    inReg->BCMode = 0;              // select 22 MS/s input
                    inReg->CSEnB  = 0;              // configure CQ module for carrier sense
                    inReg->DBPSK  = 1;              // DBPSK (1 Mbps) modulation during preamble
                    pRxTop->count.D.word = 0;
                    pRxTop->state.D.word = 1;
                }
                break;

            case 40: // wait until the end of the packet, for LenUSEC usec ------------------------

                pRxTop->count.D.word  = 0; // clear general counter
                inReg->DMEnB   = 1; // disable DSSS demodulator
                inReg->CMFEnB  = 1; // disable CMF
                inReg->PLLEnB  = 1; // disable PLL
                inReg->CFOcEnB = 1; // disable offset correction
                inReg->DeciOutEnB = 1; // disabled decimator output
                bSigOut->SigDet = 0;
                if (pRxTop->LengthCount.Q.w16 >= pbRX->bLENGTH.w16) pRxTop->state.D.word = 35; // end of unsupported packet
                break;

            default: pRxTop->state.D.word = 0; // -------------------------------------------------
        }
        // end of state machine

        RSSICCK->w8 = pRxTop->AbsEnergyOld.w8;
    }
    // end of 22 MHz clocked operation //////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////
    //
	//  PHY_RX_EN Signal for DSSS/CCK
    //
    //  The signal is generated based on PSDUEnB with delays to align with the existing
    //  RTL timing. The signal RX_EN is mapped to PHY_RX_EN in the top level.
    //
	if (posedgeCLK22MHz)
	{
		ClockDelayLine(pRxTop->Delay2En, pRxTop->DelayEn.delay_31T);
		ClockDelayLine(pRxTop->DelayEn, !inReg->PSDUEnB); 	
		
		if (pRxTop->DelayEn.delay_7T && !pRxTop->DelayEn.delay_8T)	// 7 clk delay. RTL: PSDUEnB_init = 6 for assertion			
			bSigOut->RX_EN = 1;
		
		if ((RX->bDataRate.w8 == 10 || RX->bDataRate.w8 == 20)) // DSSS (1, 2 Mbps) TDecode =2.42us, Delay: 1.58us 
		{
			if (pRxTop->Delay2En.delay_4T && !pRxTop->Delay2En.delay_3T) // 35 clk delay. RTL: PSDUEnB_init = 34 for deassertion to get 4us timing. 		
				bSigOut->RX_EN = 0; 
		}
		else if ((RX->bDataRate.w8 == 55 || RX->bDataRate.w8 == 110) && !RX->bModulation.b0) // CCK (5.5, 11 Mbps)	TDecode =3.82us, Delay 0.18us  
		{
            if (pRxTop->DelayEn.delay_5T && !pRxTop->DelayEn.delay_4T)	//  4 clk dealy. RTL: PSDUEnB_init = 3 for deassertion to get 4us timing		
				bSigOut->RX_EN = 0; 
		}
	}
    // --------------------------------------
    // State Machine Reset/Restart Conditions
    // --------------------------------------
    if (posedgeCLK22MHz)
    {
        // ---------------------
        // State Machine Restart
        // ---------------------
        pRxTop->ResetState = (Restart && !pRxTop->dRestart) ? 1 : 0;
        pRxTop->dRestart = Restart;

        // -------------------------------------------
        // State Machine Reset on bRXEnB
        // ...configure state and signal when inactive
        // -------------------------------------------
        if (bRXEnB)
        {
            // initialize antenna select
            bSigOut->AntSel = Reg->bSwDEn.b0 ? (Reg->bSwDSave.b0 ? bSigIn.AntSel : Reg->bSwDInit.b0) : 0;

            pRxTop->state.D.word    = 0; // reset the controller state
            RSSICCK->word   = 0; // RSSI
            bSigOut->CSCCK  = 0; // carrier sense from 11b modem
            bSigOut->RxDone = 0; // clear RxDone
            bSigOut->SigDet = 0; // clear signal detect
            if (!Verona_RxState()->EnableTrace) return; // skip subsequent processing to speed up simulation
        }
    }
    if (pRxTop->ResetState) pRxTop->state.D.word = 0;

    if (k44 >= RX->N44)  // abort if at end of input array (to avoid memory fault)
    { 
        bSigOut->RxDone = 1; 
        pRX->RTL_Exception.kLimit_FrontCtrl = 1; 
        return; 
    }

    // --------------------------------------------------------------------------------
    //
    // CALL RECEIVER MODULE FUNCTIONS
    //
    // --------------------------------------------------------------------------------
    if (posedgeCLK40MHz)
    {
        Verona_bRX_Filter( 0, qReal, RX->xReal + k40 ); // filter #0: real
        Verona_bRX_Filter( 1, qImag, RX->xImag + k40 ); // filter #1: imag
        Verona_bRX_DigitalGain( RX->xReal+k40, RX->xImag+k40, RX->yReal+k40, RX->yImag+k40, pRxTop->DGain.Q );
    }
    if (posedgeCLK40MHz || posedgeCLK44MHz)
    {
        Verona_bRX_Interpolate( RX->yReal+k40, RX->yImag+k40, RX->sReal+k44, RX->sImag+k44, pRxTop->cSP.Q, 
			                    outReg.PLLEnB, outReg.TransCCK, pRxTop->INTERPinit, &pRxTop->IntpFreeze, pRX );

        if (posedgeCLK40MHz && posedgeCLK44MHz) pRxTop->INTERPinit = 0;
    }
    if (!posedgeCLK44MHz) return; // skip the rest of the function except for 11/22/44 MHz clock edges

    // ----------------------------------------------------------------------
    //
    // 44 MHz Clock Domain
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK44MHz)
    {
        Verona_b_AGC( outReg.AGCEnB, outReg.AGCInit, pRxTop->EDOut.Q, outReg.AGCbounded, 
			          pRxTop->LgSigDetAFE.Q.b0, &pRxTop->LNAGain, &pRxTop->AGain, 
                      &pRxTop->DGain, &pRxTop->LargeSignal, &pRxTop->GainUpdate, 
					  &pRxTop->AbsEnergy, &pRxTop->AbsPwr, Reg );

        Verona_b_Downsample( posedgeCLK22MHz, posedgeCLK11MHz, inReg->CFOcEnB, pRxTop->IntpFreeze, 
			                 outReg.SymPhase, RX->sReal[k44], RX->sImag[k44],
                             RX->zReal+k22, RX->zImag+k22, RX->vReal+k11, RX->vImag+k11, 
							 &pRxTop->DeciFreeze );

        Verona_bRX_DPLL( outReg.PLLEnB, pRxTop->SSMatOutR, pRxTop->SSMatOutI, pRxTop->posedge_SSCorOutValid, 
			             pRxTop->SSCorOutR.Q, pRxTop->SSCorOutI.Q, pRxTop->posedge_cckWalOutValid, 
                         pRxTop->CCKMatOutR, pRxTop->CCKMatOutI, pRxTop->CCKWalOutR, pRxTop->CCKWalOutI, 
                         pRxTop->ccReal, pRxTop->ccImag, &pRxTop->cCP, &pRxTop->cSP.D, Reg );
    }
    // ----------------------------------------------------------------------
    //
    // 22 MHz Clock Domain
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK22MHz)
    {
        Verona_b_CorrectCFO( outReg.CFOcEnB, outReg.TransCCK, pRxTop->dDeciFreeze, pRxTop->CFO.Q, pRxTop->cCP, 
			                 RX->vReal[k11], RX->vImag[k11], RX->uReal+k11, RX->uImag+k11, posedgeCLK11MHz );

        Verona_b_BarkerCorrelator( RX->zReal+k22, RX->zImag+k22, RX->uReal+k11, RX->uImag+k11,
                                   RX->rReal+k22, RX->rImag+k22, outReg.BCMode, outReg.BCEnB );

        Verona_b_EnergyDetect( RX->zReal+k22, RX->zImag+k22, outReg.EDEnB, Reg->EDwindow, &pRxTop->EDOut );

        Verona_b_CorrelationQuality( RX->rReal[k22], RX->rImag[k22], outReg.CQEnB, outReg.CSEnB, 
			                         Reg->CQwindow, Reg->SSwindow, &pRxTop->CQOut, &pRxTop->CQPeak );

        Verona_b_EstimateCFO(outReg.CFOeEnB, outReg.CFOeGet, RX->rReal[k22], RX->rImag[k22], &pRxTop->CFO, Reg);
    }
    // ----------------------------------------------------------------------
    //
    // 11 MHz Clock Domain
    // ...(k11>22) insures that the CMF does not look before array element 0
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK11MHz && (k11 > 22))
    {
        wiWORD *wR, *wI;

        Verona_b_ChanEst( outReg.CEEnB, outReg.CEMode, outReg.CEGet, RX->rReal[k22], RX->rImag[k22], 
			              pRxTop->SSOut.Q, pRxTop->SSOutValid.Q.b0, outReg.DBPSK, &pRxTop->CEDone, RX, Reg);

        Verona_bRX_CMF( outReg.CMFEnB, pRxTop->DMSym, pRxTop->dDeciFreeze, RX->uReal+k11, RX->uImag+k11, 
			            &pRxTop->SSMatOutR, &pRxTop->SSMatOutI, RX->Nh, RX->hReal, RX->hImag );

        Verona_bRX_DSSS( outReg.DMEnB, RX->uReal+k11, RX->uImag+k11, pRxTop->SSMatOutR, pRxTop->SSMatOutI, 
			             &pRxTop->SSCorOutR, &pRxTop->SSCorOutI, &pRxTop->SSCorOutValid, 
                         &pRxTop->SSSliOutR, &pRxTop->SSSliOutI, &pRxTop->SSOut, &pRxTop->SSOutValid, 
                         outReg.DBPSK, pRxTop->CEDone, outReg.HDREnB, pRxTop->DMSym, 
                         pRxTop->BFReal, pRxTop->BFImag );

        Verona_bRX_SP( RX->uReal+k11, RX->uImag+k11, &wR, &wI, RX->Np2, 
			           outReg.SPEnB, outReg.TransCCK, pRxTop->dDeciFreeze );

        if (!outReg.PSDUEnB && pRxTop->cckSym.delay_10T)
        {
            Verona_bRX_CCK( wR-10,wI-10, &(pRxTop->cckOut.D), &(pRxTop->cckWalOutValid.D), 
                            RX->bDataRate.w8, pRxTop->CCKMatOutR, pRxTop->CCKMatOutI, 
                            &pRxTop->CCKWalOutR, &pRxTop->CCKWalOutI, &pRxTop->ccReal, &pRxTop->ccImag, 
							RX->hReal, RX->hImag, RX->Nh, RX->Np, RX->Np2, pRxTop->BFReal, pRxTop->BFImag, 
							pRxTop->SSSliOutR.Q, pRxTop->SSSliOutI.Q, &pRxTop->cckInit );
        }
        else pRxTop->cckWalOutValid.D.word = 0;

        pRxTop->cckOutValid.D = pRxTop->cckWalOutValid.Q; // approximate timing for cckOutValid (cckWalOutValid is cycle accurate)

        Verona_bRX_Descrambler( pRxTop->SSOut.Q, pRxTop->SSOutValid.Q.b0, pRxTop->cckOut.Q, pRxTop->cckOutValid.Q.b0, 
			                    outReg.DBPSK, &pRxTop->DesOut, &pRxTop->DesOutValid, RX, pRxTop->state.Q.w6 );

        Verona_bRX_SFD (outReg.SFDEnB, pRxTop->DesOut.Q, pRxTop->DesOutValid.Q.b0, &pRxTop->Preamble, &pRxTop->SFDDone);

        Verona_bRX_Header( outReg.HDREnB, outReg.DBPSK, pRxTop->Preamble.Q.b0, 
			               pRxTop->SSOutValid.Q.b0, RX->a, &pRxTop->HDRDone, RX );

        Verona_bRX_PSDU(outReg.PSDUEnB, pRxTop->DesOutValid.Q.b0, &pRxTop->PSDUDone, pRX);
    }
    // -----------------------
    // Receive Symbol Counters
    // -----------------------
    if (posedgeCLK11MHz)
    {
        inReg->SymCMF = outReg.SymGo    && (pRxTop->SymCount.Q.w11    ==  0)? 1:0; // setup flag for DSSS symbol at CMF output
        inReg->SymCCK = outReg.SymGoCCK && (pRxTop->SymCountCCK.Q.w11 ==  0)? 1:0; // setup flag for CCK symbol at CMF output
    }
    pRxTop->SymCount.D.word    = (outReg.SymGo    && !pRxTop->dDeciFreeze.delay_0T )? ((pRxTop->SymCount.Q.w4    + 1) % 11) :10;
    pRxTop->SymCountCCK.D.word = (outReg.SymGoCCK && !pRxTop->dDeciFreeze.delay_19T)? ((pRxTop->SymCountCCK.Q.w4 + 1) %  8) : 7;
    if (pRxTop->state.Q.w6 == 40) {
        if (!pRxTop->SymCount.Q.w4) pRxTop->LengthCount.D.word = pRxTop->LengthCount.Q.w16 + 1; // 1 microsecond counter for unsupported packet types
    }
    else pRxTop->LengthCount.D.word = 2; // offset to reduce error due to DSSS decoding delay

    // ---------------------------------------
    // Qualify/Latch AFE Large Signal Detector
    // ---------------------------------------
    pRxTop->LgSigDetAFE.D.word = outReg.ArmLgSigDet 
                               & (LgSigAFE || pRxTop->LgSigDetAFE.Q.b0) 
                               & pRxTop->LNAGain.Q.b0 & Reg->bLgSigAFEValid.b0;

    // ----------------------------------------------------------------------
    //
    // Trace Signals
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK22MHz && Verona_RxState()->EnableTrace)
    {
        // CCK Baseband Interface
        //
        RX->traceCCK[k22].bRXEnB   = bRXEnB;
        RX->traceCCK[k22].Run11b   = Run11b;
        RX->traceCCK[k22].LgSigAFE = LgSigAFE;
        RX->traceCCK[k22].bRXDone  = bSigOut->RxDone;
        RX->traceCCK[k22].AntSel   = bSigOut->AntSel;
        RX->traceCCK[k22].CSCCK    = bSigIn.CSCCK;
        RX->traceCCK[k22].Restart  = Restart;
        RX->traceCCK[k22].RSSICCK  = RSSICCK->w8;
        RX->traceCCK[k22].LNAGain  = bLNAGain->b0;
        RX->traceCCK[k22].AGain    = bAGain->w6;
        RX->traceCCK[k22].DGain    = pRxTop->DGain.Q.w5;
        RX->traceCCK[k22].LNAGain2 = bLNAGain->b1;
        //
        // Baseband State
        //
        RX->traceState[k22].State     = pRxTop->state.Q.w6;
        RX->traceState[k22].Counter   = pRxTop->count.Q.w11;
        RX->traceState[k22].CQPeak    = pRxTop->CQPeak.Q.w5;
        //
        // Control Signals
        //
        RX->traceControl[k22].EDEnB      = outReg.EDEnB;
        RX->traceControl[k22].traceValid = 1;
        RX->traceControl[k22].CQEnB      = outReg.CQEnB;
        RX->traceControl[k22].CSEnB      = outReg.CSEnB;
        RX->traceControl[k22].LargeSignal= pRxTop->LargeSignal.Q.b0;
        RX->traceControl[k22].GainUpdate = pRxTop->GainUpdate.Q.b0;
        RX->traceControl[k22].CEEnB      = outReg.CEEnB;
        RX->traceControl[k22].CEDone     = pRxTop->CEDone;
        RX->traceControl[k22].CEMode     = outReg.CEMode;
        RX->traceControl[k22].CEGet      = outReg.CEGet;
        RX->traceControl[k22].CFOeEnB    = outReg.CFOeEnB;
        RX->traceControl[k22].CFOeGet    = outReg.CFOeGet;
        RX->traceControl[k22].CFOcEnB    = outReg.CFOcEnB;
        RX->traceControl[k22].SFDEnB     = outReg.SFDEnB;
        RX->traceControl[k22].SFDDone    = pRxTop->SFDDone.Q.b0;
        RX->traceControl[k22].SymPhase   = outReg.SymPhase;
        RX->traceControl[k22].Preamble   = pRxTop->Preamble.Q.b0;
        RX->traceControl[k22].HDREnB     = outReg.HDREnB;
        RX->traceControl[k22].HDRDone    = pRxTop->HDRDone.delay_0T;
        RX->traceControl[k22].PSDUEnB    = outReg.PSDUEnB;
        RX->traceControl[k22].PSDUDone   = pRxTop->PSDUDone.delay_1T;
        RX->traceControl[k22].TransCCK   = outReg.TransCCK;
        RX->traceControl[k22].DeciFreeze = pRxTop->DeciFreeze.Q.b0;

        RX->traceFrontReg[k22].word      = outReg.word;
        //
        // DPLL and Interpolator
        //
        RX->traceDPLL[k22].PLLEnB     = outReg.PLLEnB;
        RX->traceDPLL[k22].IntpFreeze = pRxTop->IntpFreeze;
        RX->traceDPLL[k22].InterpInit = pRxTop->INTERPinit;
        RX->traceDPLL[k22].cCP        = pRxTop->cCP.w6;
        RX->traceDPLL[k22].cSP        = pRxTop->cSP.D.w14;
        //
        // Barker Correlator
        //
        RX->traceBarker[k22].BCEnB    = outReg.BCEnB;
        RX->traceBarker[k22].BCMode   = outReg.BCMode;
        RX->traceBarker[k22].BCOutR   = RX->rReal[k22].w10;
        RX->traceBarker[k22].BCOutI   = RX->rImag[k22].w10;
        //
        // DSSS Demodulator
        //
        RX->traceDSSS[k22].DMSym      = pRxTop->DMSym.delay_0T;
        RX->traceDSSS[k22].SSOutValid = pRxTop->SSOutValid.Q.b0;
        RX->traceDSSS[k22].SSOut      = pRxTop->SSOut.Q.w2;
        RX->traceDSSS[k22].SSSliOutR  = pRxTop->SSSliOutR.Q.b0;
        RX->traceDSSS[k22].SSSliOutI  = pRxTop->SSSliOutI.Q.b0;
        RX->traceDSSS[k22].SSCorOutR  = pRxTop->SSCorOutR.Q.w12;
        RX->traceDSSS[k22].SSCorOutI  = pRxTop->SSCorOutI.Q.w12;
        if (k11>22)
        {
            RX->tReal[k11].word = pRxTop->SSMatOutR ? pRxTop->SSMatOutR[0].word : 0;
            RX->tImag[k11].word = pRxTop->SSMatOutI ? pRxTop->SSMatOutI[0].word : 0;
        }
        //
        // Descrambler
        //
        RX->traceBarker[k22].DesOutValid = pRxTop->DesOutValid.Q.b0;
        RX->traceBarker[k22].DesOut      = pRxTop->DesOut.Q.w8;
        //
        // Non-packed Signals
        //
        RX->EDOut[k22].word = pRxTop->EDOut.Q.w6;
        RX->CQOut[k22].word = pRxTop->CQOut.Q.word;
        RX->CQPeak = pRxTop->CQPeak.Q;

        RX->k80[k22] = pRX->k80;
    }
}
// end of Verona_bRX_top()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------


//--< bMojave.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  Mojave - DSSS/CCK (11b) Baseband
//           
//  Developed by Younggyun Kim and Barrett Brickner
//  Copyright 2001-2003 Bermai, 2005-2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "Mojave.h"

//=================================================================================================
//--  MODULE LOCAL PARAMETERS
//=================================================================================================
static wiBoolean Verbose    = 0;
static bMojave_RX_State bRX = {0};   // receiver state

static wiWORD  CC55Real[ 4][8],  CC55Imag[ 4][8]; // CCK cross-correlation arrays
static wiWORD CC110Real[64][8], CC110Imag[64][8]; // ...defined in bMojave_PowerOn()

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
// FUNCTION : bMojave_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus bMojave_ParseLine(wiString text)
{
    wiStatus status;

    status = WICHK(wiParse_Boolean(text,"Mojave.EnableTraceCCK", &(bRX.EnableTraceCCK)));
    if (status==WI_SUCCESS)
    {
        XSTATUS(bMojave_RX_AllocateMemory()); // make sure trace arrays are allocated
        return WI_SUCCESS;
    }
    else if (status<0) return status;
     
    return WI_WARN_PARSE_FAILED;
}
// end of bMojave_ParseLine()

// ================================================================================================
// FUNCTION : bMojave_StatePointer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus bMojave_StatePointer(bMojave_RX_State **pbRX)
{
    if (pbRX) *pbRX = &bRX;
    return WI_SUCCESS;
}
// end of bMojave_StatePointer()

// ================================================================================================
// FUNCTION : bMojave_PowerOn()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus bMojave_PowerOn()
{
    if (Verbose) wiPrintf(" bMojave_PowerOn()\n");

    bRX.N44  = 0;
    XSTATUS(bMojave_RX_AllocateMemory());

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
            d2 = (k>>1) & 0x01;
            d3 = (k>>0) & 0x01;

            p2 = 2*d2 + 1;
            p3 = 0;
            p4 = 2*d3;

            p = (p2+p3+p4+0)%4; CC55Real[k][0].word = cR[p]; CC55Imag[k][0].word = cI[p]; 
            p = (   p3+p4+0)%4; CC55Real[k][1].word = cR[p]; CC55Imag[k][1].word = cI[p]; 
            p = (p2   +p4+0)%4; CC55Real[k][2].word = cR[p]; CC55Imag[k][2].word = cI[p]; 
            p = (      p4+2)%4; CC55Real[k][3].word = cR[p]; CC55Imag[k][3].word = cI[p]; 
            p = (p2+p3   +0)%4; CC55Real[k][4].word = cR[p]; CC55Imag[k][4].word = cI[p]; 
            p = (   p3   +0)%4; CC55Real[k][5].word = cR[p]; CC55Imag[k][5].word = cI[p]; 
            p = (p2      +2)%4; CC55Real[k][6].word = cR[p]; CC55Imag[k][6].word = cI[p]; 
            p = (         0)%4; CC55Real[k][7].word = cR[p]; CC55Imag[k][7].word = cI[p]; 
        }
        for (k=0; k<64; k++) // table for CCK at 11Mbps
        {
            p2 = (k>>4) & 0x03;
            p3 = (k>>2) & 0x03;
            p4 = (k>>0) & 0x03;

            p = (p2+p3+p4+0)%4; CC110Real[k][0].word = cR[p]; CC110Imag[k][0].word = cI[p];
            p = (   p3+p4+0)%4; CC110Real[k][1].word = cR[p]; CC110Imag[k][1].word = cI[p];
            p = (p2   +p4+0)%4; CC110Real[k][2].word = cR[p]; CC110Imag[k][2].word = cI[p];
            p = (      p4+2)%4; CC110Real[k][3].word = cR[p]; CC110Imag[k][3].word = cI[p]; 
            p = (p2+p3   +0)%4; CC110Real[k][4].word = cR[p]; CC110Imag[k][4].word = cI[p]; 
            p = (   p3   +0)%4; CC110Real[k][5].word = cR[p]; CC110Imag[k][5].word = cI[p];  
            p = (p2      +2)%4; CC110Real[k][6].word = cR[p]; CC110Imag[k][6].word = cI[p];  
            p = (         0)%4; CC110Real[k][7].word = cR[p]; CC110Imag[k][7].word = cI[p];  
        }
    }
    return WI_SUCCESS;
}
// end of bMojave_PowerOn()

// ================================================================================================
// FUNCTION : bMojave_RX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus bMojave_RX_AllocateMemory(void)
{
    int N80 = (bRX.N44 * 80/44) + 2; // +2 to cover rounding errors

    if (Verbose) wiPrintf(" bMojave_RX_AllocateMemory(): N44=%d\n",bRX.N44);

    bMojave_RX_FreeMemory();

    bRX.N40 = bRX.N44*40/44+2; // all counts use +2: +1 for rounding and +1 for the case
    bRX.N22 = bRX.N44/2 + 2;   // ...where the initial clock index is 1 instead of 0.
    bRX.N11 = bRX.N44/4 + 2;   // ...due to new clocking scheme as of WiSE.060306a
    bRX.Na  = 8*4095 + 192;
    bRX.Nb  = bRX.Na;
    
    bRX.a    =(wiUWORD *)wiCalloc(bRX.Na,  sizeof(wiUWORD));
    bRX.b    =(wiUWORD *)wiCalloc(bRX.Nb,  sizeof(wiUWORD));
    bRX.xReal=(wiWORD  *)wiCalloc(bRX.N44, sizeof(wiWORD));
    bRX.xImag=(wiWORD  *)wiCalloc(bRX.N44, sizeof(wiWORD));
    bRX.yReal=(wiWORD  *)wiCalloc(bRX.N44, sizeof(wiWORD));
    bRX.yImag=(wiWORD  *)wiCalloc(bRX.N44, sizeof(wiWORD));
    bRX.sReal=(wiWORD  *)wiCalloc(bRX.N44, sizeof(wiWORD));
    bRX.sImag=(wiWORD  *)wiCalloc(bRX.N44, sizeof(wiWORD));
    bRX.zReal=(wiWORD  *)wiCalloc(bRX.N22, sizeof(wiWORD));
    bRX.zImag=(wiWORD  *)wiCalloc(bRX.N22, sizeof(wiWORD));
    bRX.rReal=(wiWORD  *)wiCalloc(bRX.N22, sizeof(wiWORD));
    bRX.rImag=(wiWORD  *)wiCalloc(bRX.N22, sizeof(wiWORD));
    bRX.vReal=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.vImag=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.uReal=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.uImag=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.wReal=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.wImag=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.pReal=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.pImag=(wiWORD  *)wiCalloc(bRX.N11, sizeof(wiWORD));
    bRX.tReal=(wiWORD  *)wiCalloc(bRX.N11+14, sizeof(wiWORD));
    bRX.tImag=(wiWORD  *)wiCalloc(bRX.N11+14, sizeof(wiWORD));
    bRX.State=(wiUWORD *)wiCalloc(bRX.N22, sizeof(wiUWORD));
    bRX.EDOut=(wiUWORD *)wiCalloc(bRX.N22, sizeof(wiUWORD));
    bRX.CQOut=(wiUWORD *)wiCalloc(bRX.N22, sizeof(wiUWORD));

    if (bRX.EnableTraceCCK)
    {
        bRX.traceCCK     = (bMojave_traceCCKType     *)wiCalloc(bRX.N22, sizeof(bMojave_traceCCKType)     );
        bRX.traceState   = (bMojave_traceStateType   *)wiCalloc(bRX.N22, sizeof(bMojave_traceStateType)   );
        bRX.traceControl = (bMojave_traceControlType *)wiCalloc(bRX.N22, sizeof(bMojave_traceControlType) );
        bRX.traceFrontReg= (bMojave_FrontCtrlRegType *)wiCalloc(bRX.N22, sizeof(bMojave_FrontCtrlRegType) );
        bRX.traceDPLL    = (bMojave_traceDPLLType    *)wiCalloc(bRX.N22, sizeof(bMojave_traceDPLLType)    );
        bRX.traceBarker  = (bMojave_traceBarkerType  *)wiCalloc(bRX.N22, sizeof(bMojave_traceBarkerType)  );
        bRX.traceDSSS    = (bMojave_traceDSSSType    *)wiCalloc(bRX.N22, sizeof(bMojave_traceDSSSType)    );
        bRX.traceCCKInput= (bMojave_traceCCKFBFType  *)wiCalloc( 65536,  sizeof(bMojave_traceCCKFBFType)  );
        bRX.traceCCKTVMF = (bMojave_traceCCKTVMFType *)wiCalloc( 65536,  sizeof(bMojave_traceCCKTVMFType) );
        bRX.traceCCKFWT  = (bMojave_traceCCKFWTType  *)wiCalloc(262144,  sizeof(bMojave_traceCCKTVMFType) );
        bRX.traceResample= (bMojave_traceResampleType*)wiCalloc(N80,     sizeof(bMojave_traceResampleType));
        bRX.k80          = (unsigned int             *)wiCalloc(bRX.N22, sizeof(unsigned int)             );
        if (!bRX.traceCCK || !bRX.traceState || !bRX.traceControl || !bRX.traceFrontReg || !bRX.traceDPLL || !bRX.traceBarker 
            || !bRX.traceDSSS ||!bRX.traceCCKInput || !bRX.traceCCKTVMF || !bRX.traceCCKFWT || !bRX.traceResample) 
            return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    }
    // ---------------------------------
    // Check for memory allocation fault
    // ---------------------------------
    if (   !bRX.a     || !bRX.b
        || !bRX.xReal || !bRX.xImag
        || !bRX.yReal || !bRX.yImag
        || !bRX.sReal || !bRX.sImag
        || !bRX.zReal || !bRX.zImag
        || !bRX.vReal || !bRX.vImag
        || !bRX.uReal || !bRX.uImag
        || !bRX.tReal || !bRX.tImag
        || !bRX.wReal || !bRX.wImag
        || !bRX.pReal || !bRX.pImag
        || !bRX.rReal || !bRX.rImag
        || !bRX.State
        || !bRX.EDOut || !bRX.CQOut )
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    return WI_SUCCESS;
}
// end of bMojave_RX_AllocateMemory()

// ================================================================================================
// FUNCTION : bMojave_RX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus bMojave_RX_FreeMemory(void)
{
    if (Verbose) wiPrintf(" bMojave_RX_FreeMemory()\n");

    if (bRX.a)   wiFree(bRX.a);      bRX.a   = NULL;      bRX.Na = 0;
    if (bRX.b)   wiFree(bRX.b);      bRX.b   = NULL;      bRX.Nb = 0;

    if (bRX.xReal) wiFree(bRX.xReal);  bRX.xReal = NULL;
    if (bRX.xImag) wiFree(bRX.xImag);  bRX.xImag = NULL;
    if (bRX.yReal) wiFree(bRX.yReal);  bRX.yReal = NULL;
    if (bRX.yImag) wiFree(bRX.yImag);  bRX.yImag = NULL;
    if (bRX.sReal) wiFree(bRX.sReal);  bRX.sReal = NULL;
    if (bRX.sImag) wiFree(bRX.sImag);  bRX.sImag = NULL;
    if (bRX.zReal) wiFree(bRX.zReal);  bRX.zReal = NULL;
    if (bRX.zImag) wiFree(bRX.zImag);  bRX.zImag = NULL;
    if (bRX.vReal) wiFree(bRX.vReal);  bRX.vReal = NULL;
    if (bRX.vImag) wiFree(bRX.vImag);  bRX.vImag = NULL;
    if (bRX.uReal) wiFree(bRX.uReal);  bRX.uReal = NULL;
    if (bRX.uImag) wiFree(bRX.uImag);  bRX.uImag = NULL;
    if (bRX.tReal) wiFree(bRX.tReal);  bRX.tReal = NULL;
    if (bRX.tImag) wiFree(bRX.tImag);  bRX.tImag = NULL;
    if (bRX.wReal) wiFree(bRX.wReal);  bRX.wReal = NULL;
    if (bRX.wImag) wiFree(bRX.wImag);  bRX.wImag = NULL;
    if (bRX.pReal) wiFree(bRX.pReal);  bRX.pReal = NULL;
    if (bRX.pImag) wiFree(bRX.pImag);  bRX.pImag = NULL;
    if (bRX.rReal) wiFree(bRX.rReal);  bRX.rReal = NULL;
    if (bRX.rImag) wiFree(bRX.rImag);  bRX.rImag = NULL;

    if (bRX.State) wiFree(bRX.State);  bRX.State = NULL;
    if (bRX.EDOut) wiFree(bRX.EDOut);  bRX.EDOut = NULL;
    if (bRX.CQOut) wiFree(bRX.CQOut);  bRX.CQOut = NULL;

    if (bRX.traceCCK)     wiFree(bRX.traceCCK);     bRX.traceCCK     = NULL;
    if (bRX.traceState)   wiFree(bRX.traceState);   bRX.traceState   = NULL;
    if (bRX.traceControl) wiFree(bRX.traceControl); bRX.traceControl = NULL;
    if (bRX.traceFrontReg)wiFree(bRX.traceFrontReg);bRX.traceFrontReg= NULL;
    if (bRX.traceDPLL)    wiFree(bRX.traceDPLL);    bRX.traceDPLL    = NULL;
    if (bRX.traceBarker)  wiFree(bRX.traceBarker);  bRX.traceBarker  = NULL;
    if (bRX.traceDSSS)    wiFree(bRX.traceDSSS);    bRX.traceDSSS    = NULL;
    if (bRX.traceCCKInput)wiFree(bRX.traceCCKInput);bRX.traceCCKInput= NULL;
    if (bRX.traceCCKTVMF) wiFree(bRX.traceCCKTVMF); bRX.traceCCKTVMF = NULL;
    if (bRX.traceCCKFWT)  wiFree(bRX.traceCCKFWT);  bRX.traceCCKFWT  = NULL;
    if (bRX.traceResample)wiFree(bRX.traceResample);bRX.traceResample= NULL;
    if (bRX.k80)          wiFree(bRX.k80);          bRX.k80          = NULL;
    
    return WI_SUCCESS;
}
// end of bMojave_RX_FreeMemory()

// ================================================================================================
// FUNCTION  : bMojave_TX_DataGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate bit sequence from PSDU bytes, preamble length, and data rate
// Parameters: RATE    - RATE field from PHY header
//             NumByte - PSDU length in bytes
//             PSDU    - PHY Service Data Unit (Data Bytes)
//             a       - serialized bit stream
//             pTX     - Pointer to the TX State structure
// ================================================================================================
void bMojave_TX_DataGenerator(wiUWORD RATE, int NumByte, wiUWORD *PSDU, wiUWORD *a, Mojave_TX_State *pTX)
{
    int b,i,k,n,r;

    wiUWORD SFD = {RATE.b2? 0x05CF:0xF3A0};
    wiUWORD SIGNAL;
    wiUWORD SERVICE = {0x04};
    wiUWORD LENGTH; // length of PSDU in microseconds
    wiUWORD HEADER; // PLCP Header without CRC: SIGNAL+SERVICE+LENGTH
    wiUWORD Z;      // shift register for CRC calculation
    wiBoolean ModSel = pTX->SetPBCC; // (0=CCK, 1=PBCC)

    // ----------
    // SYNC Field
    // ----------
    if (RATE.b2) for (i=0; i< 56; i++) (a++)->word = 0;
    else        for (i=0; i<128; i++) (a++)->word = 1;

    // ---------------------------
    // Start Frame Delimiter (SFD)
    // ---------------------------
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
    HEADER.word = (LENGTH.w16 << 16) | (SERVICE.w8 << 8) | (SIGNAL.w8);
    for (i=0; i<32; i++)
        (a++)->word = (HEADER.w32>>i) & 1;

    // Forced Header
    // -------------
    if (pTX->UseForcedbHeader)
        HEADER.word = pTX->ForcedbHeader.w32;

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
// end of bMojave_TX_DataGenerator()

// ================================================================================================
// FUNCTION  : bMojave_TX_Scrambler()
// ------------------------------------------------------------------------------------------------
// Purpose   : TX Scrambler for the data bit stream
// Parameters: RATE - rate field from PHY header
//             Na   - number of bits (input and output)
//             a    - input serial bits
//             b    - output serial bits
// ================================================================================================
void bMojave_TX_Scrambler(wiUWORD RATE, int Na, wiUWORD *a, wiUWORD *b)
{
    wiUWORD X = {RATE.b2? 0x6C:0x1B}; // initialize scrambler based on preamble type
    int k;

    for (k=0; k<Na; k++)
    {
        b[k].word = a[k].b0 ^ (X.b3) ^ (X.b6);
        X.word = ((X.w7<<1) & 0x7E) | b[k].b0;
    }
}
// end of bMojave_TX_Scrambler()

// ================================================================================================
// FUNCTION  : bMojave_TX_DBPSK()
// ------------------------------------------------------------------------------------------------
// Purpose   : DBPSK Modulation
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void bMojave_TX_DBPSK(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    const wiUWORD Barker[11] = {0,1,0,0,1,0,0,0,1,1,1}; // Barker sequence in multiples of pi
    int i,j,k; // i=symbol index, j=chip index, k=sample index

    for (i=k=0; i<N; i++)
    {
        pp->b1 = pp->b1 ^ b[i].b0; // differential phase modulation: 180 degree shift

        // spreading by Barker sequence: p = pp + (Barker<<1)
        // mapping to constellation: 0=(+1+j), 1=(-1+j), 2=(-1-j), 3=(+1-j)
        for (j=0; j<11; j++, k++)
        {
            cReal[k].word = 1 ^ Barker[j].b0 ^ pp->b1 ^ pp->b0;
            cImag[k].word = 1 ^ Barker[j].b0 ^ pp->b1;
        }
    }
}
// end of bMojave_TX_DBPSK()

// ================================================================================================
// FUNCTION  : bMojave_DQPSK_Encoding()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return DQPSK Encoding phase change (units of pi/2) for two data bits
// Parameters: b0 -- two-bit LSB (first bit in time)
//             b1 -- two-bit MSB (second bit in time)
// ================================================================================================
__inline int bMojave_DQPSK_Encoding(wiUWORD b0, wiUWORD b1)
{
    const int PhaseChange[4] = {0,1,3,2};
    return PhaseChange[b0.b0<<1 | b1.b0];
}

// ================================================================================================
// FUNCTION  : bMojave_TX_DQPSK()
// ------------------------------------------------------------------------------------------------
// Purpose   : DQPSK Modulation
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void bMojave_TX_DQPSK(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    const wiUWORD Barker[11] = {0,1,0,0,1,0,0,0,1,1,1}; // Barker sequence in multiples of pi
    int i,j,k,n; // i=symbol index, j=chip index, k=sample index, n=bit index, p=chip phase

    for (i=k=n=0; i<N; i++, n+=2)
    {
        pp->w2 += bMojave_DQPSK_Encoding(b[n], b[n+1]); // differential phase modulation

        // spreading by Barker sequence: p = pp + (Barker<<1)
        // mapping to constellation: 0=(+1+j), 1=(-1+j), 2=(-1-j), 3=(+1-j)
        for (j=0; j<11; j++, k++)
        {
            cReal[k].word = 1 ^ Barker[j].b0 ^ pp->b1 ^ pp->b0;
            cImag[k].word = 1 ^ Barker[j].b0 ^ pp->b1;
        }
    }
}
// end of bMojave_TX_DQPSK()

// ================================================================================================
// FUNCTION  : bMojave_TX_CCK()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Codeword Generation
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void bMojave_TX_CCK(wiUWORD p1, wiUWORD p2, wiUWORD p3, wiUWORD p4, wiUWORD *cReal, wiUWORD *cImag)
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
// end of bMojave_TX_CCK()

// ================================================================================================
// FUNCTION  : bMojave_TX_CCK55()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Modulation at 5.5 Mbps
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void bMojave_TX_CCK55(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    int i;            // symbol index
    int k;            // sample index
    int n;            // bit index
    wiUWORD p2,p3,p4;   // component phases in multiples of pi/2

    for (i=k=n=0; i<N; i++, n+=4, k+=8)
    {
        pp->w2+= bMojave_DQPSK_Encoding(b[n], b[n+1]); // DQPSK for common phase
        pp->b1 = pp->b1 ^ (i&1);                       // reverse phase on odd symbols

        p2.w2 = b[n+2].b0? 3:1;
        p3.w2 = 0;
        p4.w2 = b[n+3].b0? 2:0;

        bMojave_TX_CCK(*pp, p2, p3, p4, cReal+k, cImag+k);
    }
}
// end of bMojave_TX_CCK55

// ================================================================================================
// FUNCTION  : bMojave_TX_CCK110()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Modulation at 11 Mbps
// Parameters: N     -- number of symbols to generate
//             b     -- input serial bit stream
//             cReal -- transmit signal output, 1-bit, real, 11 MHz
//             cImag -- transmit signal output, 1-bit, imag, 11 MHz
//             pp    -- phase of previous symbol in quadrant
// ================================================================================================
void bMojave_TX_CCK110(int N, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag, wiUWORD *pp)
{
    int i;            // symbol index
    int k;            // sample index
    int n;            // bit index
    wiUWORD p2,p3,p4;      // component phases in multiples of pi/2

    for (i=k=n=0; i<N; i++, n+=8, k+=8)
    {
        pp->w2+= bMojave_DQPSK_Encoding(b[n], b[n+1]); // DQPSK for common phase
        pp->b1 = pp->b1 ^ (i&1);                       // reverse phase on odd symbols

        p2.w2 = b[n+2].b0<<1 | b[n+3].b0;
        p3.w2 = b[n+4].b0<<1 | b[n+5].b0;
        p4.w2 = b[n+6].b0<<1 | b[n+7].b0;

        bMojave_TX_CCK(*pp, p2, p3, p4, cReal+k, cImag+k);
    }
}
// end of bMojave_TX_CCK110

// ================================================================================================
// FUNCTION  : bMojave_TX_Modulation
// ------------------------------------------------------------------------------------------------
// Purpose   : DSSS/CCK Modulator
// Parameters: RATE    -- RATE field from PHY header
//             NumByte -- Length of PSDU (Bytes)
//             b       -- Serial bit stream input
//             cReal   -- Output transmit signal, real, 11 MHz
//             cImag   -- Output transmit signal, imag, 11 MHz
// ================================================================================================
void bMojave_TX_Modulation(wiUWORD RATE, int NumByte, wiUWORD *b, wiUWORD *cReal, wiUWORD *cImag)
{
    wiUWORD phase = {0}; // 2-bit symbol phase {0->pi/4, 1->3pi/4, 2->5pi/4, 3->7pi/4}

    // ---------------------
    // Modulation: Preamble
    // ---------------------
    bMojave_TX_DBPSK((RATE.b2? 72:144), b, cReal, cImag, &phase);
    b     +=     RATE.b2? 72:144;
    cReal += 11*(RATE.b2? 72:144);
    cImag += 11*(RATE.b2? 72:144);

    // -------------------
    // Modulation: Header
    // -------------------
    if (RATE.b2) bMojave_TX_DQPSK(24, b, cReal, cImag, &phase); // short preamble
    else        bMojave_TX_DBPSK(48, b, cReal, cImag, &phase); // long preamble

    b += 48;
    cReal += 11 *(RATE.b2? 24:48);
    cImag += 11 *(RATE.b2? 24:48);

    // -----------------
    // Modulation: PSDU
    // -----------------
    switch (RATE.w2)
    {
        case 0: bMojave_TX_DBPSK (8*NumByte,   b, cReal, cImag, &phase); break;
        case 1: bMojave_TX_DQPSK (8*NumByte/2, b, cReal, cImag, &phase); break;
        case 2: bMojave_TX_CCK55 (8*NumByte/4, b, cReal, cImag, &phase); break;
        case 3: bMojave_TX_CCK110(8*NumByte/8, b, cReal, cImag, &phase); break;
    }
}
// end of bMojave_TX_Modulation()

// ================================================================================================
// FUNCTION  : bMojave_TX_PulseFilter_FIR
// ------------------------------------------------------------------------------------------------
// Purpose   : Pulse Filter FIR Element. Filter is implemented as a lookup table using
//             the relavent binary inputs to form the address/index.
//             Impulse response at 44 MHz: h={-4,-9,0,41,115,191,224,191,115,41,0,-9,-4}
// Parameters: Nc -- Number of input samples
//             c  -- Binary input array corresponding to +/- pulses (use zero outside c)
//             x  -- Upsampled output, 9 bit
// ================================================================================================
void bMojave_TX_PulseFilter_FIR(int Nc, wiUWORD *c, wiWORD *x)
{
    const int h0[] = {-4, 115, 115, -4};
    const int h1[] = {-9, 191,  41,  0};
    const int h2[] = { 0, 224,   0,  0}; // filter taps: primary phase
    const int h3[] = {41, 191,  -9,  0};

    wiWORD v[4] = {0};  // FIR output accumulators
    wiWORD d[4] = {0};  // filter delay line
    int Nx = 4*Nc + 12; // number of non-zero output terms
    int i,k;

    // ------------------------
    // for loop on 44 MHz clock
    // ------------------------
    for (k=0; k<Nx; k++)
    {
        // --------------------
        // Processing at 11 MHz
        // --------------------
        if (k%4==0)
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
        switch (k%4)
        {
            case 0: x[k].word = v[0].w9; break;
            case 1: x[k].word = v[1].w9; break;
            case 2: x[k].word = v[2].w9; break;
            case 3: x[k].word = v[3].w9; break;
        }
    }
}
// end of bMojave_TX_PulseFilter_FIR()

// ================================================================================================
// FUNCTION  : bMojave_TX_PulseFilter
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit spectrum shaping and upsample from 11 to 44 MS/s
// Parameters: Nc    -- Number of input samples
//             cReal -- Filter input,  1-bit, real, 11 MHz
//             cImag -- Filter input,  1-bit, imag, 11 MHz
//             xReal -- Filter output, 9-bit, real, 44 MHz
//             xImag -- Filter output, 9-bit, imag, 44 MHz
// ================================================================================================
void bMojave_TX_PulseFilter(int Nc, wiUWORD *cReal, wiUWORD *cImag, wiWORD *xReal, wiWORD *xImag)
{
    bMojave_TX_PulseFilter_FIR(Nc, cReal, xReal);
    bMojave_TX_PulseFilter_FIR(Nc, cImag, xImag);
}
// end of bMojave_TX_PulseFilter()

// ================================================================================================
// FUNCTION  : bMojave_TX_Resample_Core
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit signal sample rate conversion from 44 to 40 MS/s
// Parameters: N     -- Number of input samples
//             x     -- Input signal,  9-bit, 44 MHz
//             y     -- Output signal, 8-bit, 40 MHz
// ================================================================================================
void bMojave_TX_Resample_Core(int N, wiWORD *x, wiWORD *y, int bResamplePhase)
{
    wiUWORD clockGate40 = {0xAAAAA}; // 80->40 MHz clock gating cadence
    wiUWORD clockGate44 = {0xAAB55}; // 80->44 MHz clock gating cadence
    wiBoolean posedgeCLK44MHz, posedgeCLK40MHz; // clock edges

    wiWORD q,r;                          // intermediate result
    wiReg c = {9,9};                     // counter (with initial value)
    wiReg d1={0}, d2={0}, d3={0}, z={0}; // input/output registers

    int k, k44 = 0; // counter on 44 MHz clock

    // -----------------------
    // Set Initial Clock Phase
    // -----------------------
    for (k=0; k<bResamplePhase; k++)
    {
        posedgeCLK40MHz = clockGate40.b19;

        clockGate44.w20 = (clockGate44.w19 << 1) | clockGate44.b19;
        clockGate40.w20 = (clockGate40.w19 << 1) | clockGate40.b19;

        // NEW CODE TO FIX PHASE PROBLEM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        //    
        #if defined(MOJAVE_VERSION_1B)

            if (posedgeCLK40MHz)
            {
                ClockRegister(c);
                c.D.word = (c.Q.word+1)%10;
            }

        #endif // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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
            else           q.word = (c.Q.word*d2.Q.w9 + (10-c.Q.word)*d3.Q.w9)/4;

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
// end of bMojave_TX_Resample_Core()

// ================================================================================================
// FUNCTION  : bMojave_TX_Resample
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit signal sample rate conversion from 44 to 40 MS/s
// Parameters: Nc    -- Number of input samples
//             xReal -- Input,  9-bit, real, 44 MHz
//             xImag -- Input,  9-bit, imag, 44 MHz
//             yReal -- Output, 8-bit, real, 40 MHz
//             yImag -- Output, 8-bit, imag, 40 MHz
//             phase -- Relative phase of 40/44 MHz clocks
// ================================================================================================
void bMojave_TX_Resample(int N, wiWORD *xReal, wiWORD *xImag, wiWORD *yReal, wiWORD *yImag, wiInt bResamplePhase)
{
    bMojave_TX_Resample_Core(N, xReal, yReal, bResamplePhase);
    bMojave_TX_Resample_Core(N, xImag, yImag, bResamplePhase);
}
// end of bMojave_TX_Resample()

// ================================================================================================
// FUNCTION  : bMojave_TX_UpsampleFIR
// ------------------------------------------------------------------------------------------------
// Purpose   : Upsample Filter: 40 MHz --> 80 MHz
//             Filter Response: c = {-3, 0, 20, 34, 20, 0, -3}
// Parameters: Ny -- Number of output samples
//             x  -- Input,   8-bit, 40 MHz
//             y  -- Output, 11-bit, 80 MHz
// ================================================================================================
void bMojave_TX_UpsampleFIR(int Ny, wiWORD *x, wiWORD *y)
{
    const int c0[5] = { 0,  0, 34,  0}; // filter taps: main phase
    const int c1[5] = {-3, 20, 20, -3}; // filter taps: intermediate phase
    wiWORD d[4] = {0};                  // filter delay line
    wiWORD u[2] = {0};                  // filter accumulators/output terms
    wiWORD z[2] = {0};                  // filter accumulators/output terms
    wiWORD r[2] = {0};                  // remainder for rounding in output divider
    int i,k;

    // ------------------------
    // for loop on 80 MHz clock
    // ------------------------
    for (k=0; k<Ny; k++, y++)
    {
        // --------------------
        // Processing at 40 MHz
        // --------------------
        if (k%2==0)
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
// end of bMojave_TX_UpsampleFIR()

// ================================================================================================
// FUNCTION  : bMojave_TX_Upsample
// ------------------------------------------------------------------------------------------------
// Purpose   : Upsample: 40 MHz --> 80 MHz
// Parameters: Nx -- Number of input samples
//             xReal  -- Input, real, 8-bit, 40 MHz
//             xImag  -- Input, real, 8-bit, 40 MHz
//             yReal  -- Output,real,11-bit, 80 MHz
//             yImag  -- Output,imag,11-bit, 80 MHz
// ================================================================================================
void bMojave_TX_Upsample(int Nx, wiWORD *xReal, wiWORD *xImag, wiWORD *yReal, wiWORD *yImag)
{
    bMojave_TX_UpsampleFIR(Nx, xReal, yReal);
    bMojave_TX_UpsampleFIR(Nx, xImag, yImag);
}
// end of bMojave_TX_Upsample()

// ================================================================================================
// FUNCTION  : bMojave_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level DSSS/CCK TX routine
// Parameters: RATE     -- Data rate field from PHY Header
//             LENGTH   -- Number of PSDU bytes (in MAC_TX_D)
//             MAC_TX_D -- PSDU transferred from MAC (not including header)
//             Nx       -- Number of transmit samples (by reference)
//             x        -- Transmit signal array/DAC output (by reference)
//             M        -- Oversample rate for analog signal
//             pReg     -- Point to configuration registers
// ================================================================================================
wiStatus bMojave_TX ( wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, 
                      int M, MojaveRegisters *pReg, Mojave_TX_State *pTX )
{
    int NsPSDU, Na, Nc, dv;

    if (Verbose) wiPrintf(" aMojave_TX(0x%X,%d,*,*,%d,*,*)\n",RATE.w4,RATE.w12,M);

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!MAC_TX_D)                 return STATUS(WI_ERROR_PARAMETER3);
    if (!Nx)                       return STATUS(WI_ERROR_PARAMETER4);
    if (!x)                        return STATUS(WI_ERROR_PARAMETER5);
    if (InvalidRange(M, 1, 40000)) return STATUS(WI_ERROR_PARAMETER6); /* CHECK */

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
        default: return STATUS(WI_ERROR_UNDEFINED_CASE); // set TxFault in Hardware
    }
    // -----------------------------------------------------
    // Calculate Timers/Signal Durations and Allocate Memory
    // -----------------------------------------------------
    Na = 8*(LENGTH.w12) + (RATE.b2? 120:192);          // number of bits including preamble
    Nc = NsPSDU + (RATE.b2? 11*(72+24) : 11*(144+48)); // number of samples at 11 MHz

    XSTATUS(bMojave_TX_AllocateMemory(Na, Nc, M) );

    // ---------------------------------------------------------------
    // Generate bit sequence from PSDU bytes, preamble type, data rate
    // ---------------------------------------------------------------
    bMojave_TX_DataGenerator(RATE, LENGTH.w12, MAC_TX_D, pTX->a, pTX);
    bMojave_TX_Scrambler    (RATE, Na, pTX->a, pTX->b);
    bMojave_TX_Modulation   (RATE, LENGTH.w12, pTX->b, pTX->cReal, pTX->cImag);

    // -----------------------------
    // Transmit pulse-shaping filter
    // -----------------------------
    dv = 4*pTX->bPadFront + 9; // +9 accounts for pipeline delays
    pTX->OfsEOP = M*(15*8 + 18 + 80*(Nc + pTX->bPadFront))/11; // approximate end-of-packet
    bMojave_TX_PulseFilter(Nc, pTX->cReal, pTX->cImag, pTX->vReal+dv, pTX->vImag+dv);

    // -------------------
    // Resample and Output
    // -------------------
    bMojave_TX_Resample(pTX->Nv, pTX->vReal, pTX->vImag, pTX->xReal, pTX->xImag, pTX->bResamplePhase);
    bMojave_TX_Upsample(pTX->Ny, pTX->xReal, pTX->xImag, pTX->yReal, pTX->yImag);
    Mojave_Upmix       (pTX->Ny, pTX->yReal, pTX->yImag, pTX->zReal, pTX->zImag);
    Mojave_Prd         (pTX->Nz, pTX->zReal, pTX->zImag, pTX->uReal, pTX->uImag, pReg->PrdDACSrcR, pReg->PrdDACSrcI);
    Mojave_DAC         (pTX->Nz, pTX->uReal, pTX->uImag, pTX->d, M);
 
    *Nx = pTX->Nd;
    *x  = pTX->d;

    return WI_SUCCESS;
}
// end of bMojave_TX()

// ================================================================================================
// FUNCTION  : bMojave_RX_Filter
// ------------------------------------------------------------------------------------------------
// Purpose   : DSSS/CCK Receive Channel Select/Anti-Alias Filter
// Parameters: n -- filter number (model input, specifies module instance)
//             x -- input,  10 bit
//             y -- output, 11 bit
// ================================================================================================
void bMojave_RX_Filter(int n, wiWORD *x, wiWORD *y)
{
    const int h[14]={-1,-2,-2, 1, 6, 12, 16, 16, 12, 6, 1,-2,-2,-1};
    static wiWORD d[2][16]; // input delay line
    int i;

    // --------------------
    // Clock the Delay Line
    // --------------------
    for (i=15; i>0; i--) d[n][i] = d[n][i-1];
        d[n][0].word = x->w10;

    // ----------------------
    // FIR Filter Computation
    // ----------------------
    y->word = 0;
    for (i=0; i<14; i++)
        y->word = y->w17 + h[i]*d[n][i+2].w10;

    y->word = y->w17/32;
    y->word = limit(y->word, -1023, 1023);
}   
// end of bMojave_RX_Filter()

// ================================================================================================
// FUNCTION  : bMojave_RX_DigitalGain
// ------------------------------------------------------------------------------------------------
// Purpose   : Digital Gain Stage: x = q * 2^(DGain/4), Gain Range: 1/16 to 13.45
// Parameters: qReal -- Input, real, 11-bit
//             qImag -- Input, imag, 11-bit
//             xReal -- Input, real,  8-bit
//             xImag -- Input, imag,  8-bit
//             DGain -- Gain, (1.5dB steps), signed, 5-bit
// ================================================================================================
void bMojave_RX_DigitalGain(wiWORD *qReal, wiWORD *qImag, wiWORD *xReal, wiWORD *xImag, wiWORD DGain)
{
    const wiUWORD AA[4] = {0x20, 0x26, 0x2D, 0x36}; // multipliers for 32 * 2^({0,1,2,3}/4)

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
// end of bMojave_RX_DigitalGain

// ================================================================================================
// FUNCTION  : bMojave_RX_Interpolate()
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
void bMojave_RX_Interpolate(wiWORD *LPFOutR, wiWORD *LPFOutI, wiWORD *IntpOutR, wiWORD *IntpOutI,
                            wiWORD cSP, wiBoolean PLLEnB, wiBoolean TransCCK, wiBoolean INTERPinit,
                            wiBoolean *IntpFreeze, Mojave_RX_State *pRX)
{
    const  wiUWORD LUT32div11[16] = {0, 3, 6, 9, 12, 15, 17, 20, 23, 26, 29}; // = round(index*32/11)
    static wiUWORD m;  // counter for RAM write address (40 MHz sample index)

    static wiUWORD   b44n;   // integer part    of 11/32 "buffer" phase counter
    static wiUWORD   b44p;   // fractional part of 11/32 "buffer" phase counter

    static wiUWORD   k44n;   // integer part    of 11/32 phase counter for free-running 44 MHz clock
    static wiUWORD   k44p;   // fractional part of 11/32 phase counter for free-running 44 MHz clock

    static wiUReg n;      // integer part of sampling error
    static wiUReg p;      // fractional part of sampling error [0, 1)*N

    static wiReg  NCO;   // net NCO
    static wiReg  dLPFOutR,  dLPFOutI;  // input register (1T delayed)
    static wiReg  d2LPFOutR, d2LPFOutI; // input register (2T delayed)
    static wiReg  yR, yI;               // output registers

    static wiBitDelayLine dTransCCK;    // TransCCK signal delay line
    static wiBitDelayLine dIntpFreeze;  // FIFO Underflow delay line (for Interpolator Freeze)

    static struct RAM_128x32 { // 128x32, 2-port RAM
        int  xReal:8;           // with data word divided into four 8-bit Bytes
        int dxReal:8;
        int  xImag:8;
        int dxImag:8;
    } RAM[128];

    // ---------------
    // Clock Registers
    // ---------------
    if (pRX->clock.posedgeCLK40MHz)
    {
        ClockRegister(dLPFOutR); ClockRegister(d2LPFOutR);
        ClockRegister(dLPFOutI); ClockRegister(d2LPFOutI);
    }
    if (pRX->clock.posedgeCLK44MHz)
    {
        ClockRegister(p);
        ClockRegister(n);
        ClockRegister(yR);
        ClockRegister(yI);
        ClockRegister(NCO);
        ClockDelayLine(dTransCCK, TransCCK);
        ClockDelayLine(dIntpFreeze, 0);
    }
    // -----------------------------------------------
    // Initialization 
    // (occurs at RX startup and after each packet RX)
    // -----------------------------------------------
    if (INTERPinit)
    {
        m.word = 1;
        k44n.word = 0;
        k44p.word = 0;
        p.D.word = p.Q.word = n.D.word = n.Q.word = 0;
        
        yR.D.word = yR.Q.word = 0;
        yI.D.word = yI.Q.word = 0;

        dIntpFreeze.word = 0;
        dTransCCK.word = 0;
        NCO.D.word = 0;

        b44p.w5 = 10;
        b44n.w7 = 0;

        return;
    }
    // -----------------------------
    // Buffer Input Signal at 40 MHz
    // -----------------------------
    if (pRX->clock.posedgeCLK40MHz)
    {
        dLPFOutR.D.word = LPFOutR->w8;
        dLPFOutI.D.word = LPFOutI->w8;
        d2LPFOutR.D = dLPFOutR.Q;
        d2LPFOutI.D = dLPFOutI.Q;

        m.w7++; // increment RAM input address (modulo-128 counter) - free running 40 MHz clock

        RAM[m.w7].xReal  =  dLPFOutR.Q.w8;
        RAM[m.w7].dxReal = d2LPFOutR.Q.w8;
        RAM[m.w7].xImag  =  dLPFOutI.Q.w8;
        RAM[m.w7].dxImag = d2LPFOutI.Q.w8;
    }
    // -------------------------------------------
    // Oscillator (NCO) and Interpolator at 44 MHz
    // -------------------------------------------
    if (pRX->clock.posedgeCLK44MHz)
    {
        wiUWORD b44; // NCO for sample delay
        wiUWORD k44;
        wiWORD  a44; // phase adjustment to 44MHz sample timing
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
            if (dTransCCK.delay_1T || dIntpFreeze.delay_1T)
            {
                b44n.w7 += (b44p.w5>=1)?  1:0;
                b44p.w5 += (b44p.w5>=1)? -1:10;
            }
            // ------------------------------------------------------------
            // Phase Correction From DPLL and Output Freeze/TransCCK Events
            // ------------------------------------------------------------
            b44.w13 = (b44n.w7<<5) + LUT32div11[b44p.w4].w5; // b44 = 32*b44n + round(32*b44p/11)
            a44.w14 = b44.w13 + cSP.w14;                     // total phase adjustment

            dIntpFreeze.delay_0T = a44.w14<0? 1:0; // flag DPLL increase in NCO (--> buffer underflow)
        }
        // ------------------------------
        // Free-Running 44 MHz Oscillator
        // ------------------------------
        k44n.w7 += (k44p.w5>=1)?  1:0;
        k44p.w5 += (k44p.w5>=1)? -1:10;
        k44.w13 = (k44n.w7<<5) + LUT32div11[k44p.w4].w5; // k44 = 32*k44n + round(32*k44p/11)

        // ----------------------------------------
        // Combine All Oscillator/Phase Corrections
        // ----------------------------------------
        d.w15 = k44.w13 - a44.w14;         // combine free-running counter with adjustment
        NCO.D.w15 = (d.w15 + 4096) % 4096; // modulo-(32x128)

        n.D.w8 = NCO.Q.w13 >> 5; // integer part (RAM address)
        p.D.w3 = NCO.Q.w5  >> 2; // fractional part (interpolator weight)

        // -------------------------------------------------------------------
        // Linear Interpolation y[k] = ((8-p)*x[k-1] + p*x[k])/8
        // n.Q models RAM read delay, p.Q is an actual register delay to match
        // -------------------------------------------------------------------
        yR.D.word =   (RAM[n.Q.w8].dxReal * (8-(signed)p.Q.w3) + (RAM[n.Q.w8].xReal * (signed)p.Q.w3))/8;
        yI.D.word = (RAM[n.Q.w8].dxImag * (8-(signed)p.Q.w3) + (RAM[n.Q.w8].xImag * (signed)p.Q.w3))/8;
    }
    // -------------
    // Trace Signals
    // -------------
    if (bRX.EnableTraceCCK)
    {
        bRX.traceResample[pRX->k80].posedgeCLK40MHz = pRX->clock.posedgeCLK40MHz;
        bRX.traceResample[pRX->k80].posedgeCLK44MHz = pRX->clock.posedgeCLK44MHz;
        bRX.traceResample[pRX->k80].posedgeCLK22MHz = pRX->clock.posedgeCLK22MHz;
        bRX.traceResample[pRX->k80].posedgeCLK11MHz = pRX->clock.posedgeCLK11MHz;
        bRX.traceResample[pRX->k80].INTERPinit      = INTERPinit;
        bRX.traceResample[pRX->k80].m               = m.w7;
        bRX.traceResample[pRX->k80].NCO             = NCO.Q.w13;
    }
    // ------------
    // Wire Outputs
    // ------------
    IntpOutR->word = yR.Q.w8;
    IntpOutI->word = yI.Q.w8;
    *IntpFreeze    = dIntpFreeze.delay_3T;
}
// end of bMojave_RX_Interpolate()

// ================================================================================================
// FUNCTION  : bMojave_BarkerCorrelator()
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
void bMojave_BarkerCorrelator(wiWORD *BCInR22, wiWORD *BCInI22, wiWORD *BCInR11, wiWORD *BCInI11,
                                        wiWORD *BCOutR, wiWORD *BCOutI, int BCMode, int BCEnB)
{
    const int Barker[11] = {0,0,0,1,1,1,0,1,1,0,1}; // Time-reversed Barker sequence
    static wiUReg  RegBCEnB;             // BCEnB register
    static wiReg   rReal={0}, rImag={0}; // correlator output register
    static wiWORD  dR[25], dI[25];       // input delay line
    static wiUWORD counter;                // sample counter at 22 MHz
    static wiBoolean TFF = 0;
    wiWORD aReal, aImag;                 // accumulators for correlator
    int     i, j;

    // ------------------------
    // clock explicit registers
    // ------------------------
    ClockRegister(rReal); 
    ClockRegister(rImag);
    ClockRegister(RegBCEnB);
    RegBCEnB.D.word = BCEnB;
    TFF = !TFF;

    // --------------------------
    // initialize/clear registers
    // --------------------------
    if (RegBCEnB.Q.b0 && !BCEnB) {
        for (i=0;i<25;i++)   dR[i].word=dI[i].word=0;
        TFF = 0;
    }
    if (RegBCEnB.Q.b0) counter.word = 0; // clear counter

    // -----------------------------
    // Barker Correlator Calculation
    // -----------------------------
    if (!RegBCEnB.Q.b0)
    {
        for (i=24; i>0; i--) dR[i]=dR[i-1]; // clock delay lines
        for (i=24; i>0; i--) dI[i]=dI[i-1]; //   "     "     "

        dR[0].word = BCMode? BCInR11->w8:BCInR22->w8; // select input
        dI[0].word = BCMode? BCInI11->w8:BCInI22->w8; //   "      "
        aReal.word = aImag.word = 0;                  // clear accumulators

        if (counter.w5>=22 && (!BCMode || TFF)) // wait until delay line has been filled
        {                                      //   before starting correlator output
            for (i=0; i<11; i++) 
            {
                j = 2*(i+1);
                aReal.word = aReal.w11 + (Barker[i]? dR[j].w8:-dR[j].w8);
                aImag.word = aImag.w11 + (Barker[i]? dI[j].w8:-dI[j].w8);
            }
            rReal.D.word = limit(aReal.w11 >> 1, -255,255); // ~divide-by-2 and limit to 9 bits
            rImag.D.word = limit(aImag.w11 >> 1, -255,255);
        }
        counter.word = (counter.w5==31)? 31:counter.w5+1; // increment counter to 31
    }
    // -----------------------------
    // output registered correlation
    // -----------------------------
    BCOutR->word = rReal.Q.w9;
    BCOutI->word = rImag.Q.w9;
}
// end of bMojave_BarkerCorrelator()

// ================================================================================================
// FUNCTION  : bMojave_EnergyDetect()
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate the signal energy (power)
// Parameters: zReal     -- input, real, 8-bit, 22 MHz
//             zImag     -- input, imag, 8-bit, 22 MHz
//             EDEnB     -- energy detect enable (active low)
//             EDwindow  -- log2(number of symbols) for measurement window
//             EDOut     -- energy detect output, power of 2^(1/4)
// ================================================================================================
void bMojave_EnergyDetect(wiWORD *EDInR, wiWORD *EDInI, int EDEnB, wiUWORD EDwindow, wiUReg *EDOut)
{
    static wiUReg dEDEnB;
    static wiReg  zReal, zImag;
    static wiUReg acc;
    static wiUReg count;
    static wiUReg EDdone;
    static wiUReg FirstMeasure;

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(dEDEnB);
    ClockRegister(zReal);
    ClockRegister(zImag);
    ClockRegister(acc);
    ClockRegister(count);
    ClockRegister(EDdone);
    ClockRegister(FirstMeasure)

    // ---------------
    // Register Inputs
    // ---------------
    dEDEnB.D.word = EDEnB;
    zReal.D.word  = EDEnB? zReal.Q.word : (EDInR->w8/2);
    zImag.D.word  = EDEnB? zImag.Q.word : (EDInI->w8/2);

    // -------
    // Counter
    // -------
    EDdone.D.word = count.Q.w8 == (unsigned)22*(1<<EDwindow.w2);
    count.D.word  = (dEDEnB.Q.b0 || EDEnB || EDdone.D.word)? 0 : count.Q.w8+1;
    FirstMeasure.D.word = EDEnB? 1 : (EDdone.D.word? 0:FirstMeasure.Q.b0);

    // -----------------
    // Power Measurement
    // -----------------
    if (dEDEnB.Q.b0 || EDEnB || EDdone.D.b0) 
        acc.D.w20 = 0; // initialization
    else 
        acc.D.w20 = acc.Q.w20 + (zReal.Q.w7)*(zReal.Q.w7) + (zImag.Q.w7)*(zImag.Q.w7);

    // -------------------------------------------------------------
    // Convert Power Measurement to 2^(1/4) Format
    // With N symbols (22 samples/symbol) accumulated, tED = acc/16N
    // EDOut = (int)(4*log2(tED)+0.5-1.8); -1.8 for 4log2(16/22)
    // -------------------------------------------------------------
    if (EDEnB || !(FirstMeasure.Q.b0 || EDdone.D.b0))
    {
        EDOut->D = EDOut->Q;
    }
    else
    {
        wiUWORD tED, sig, exp;
        wiWORD  tail;
        int i;

        tED.word = acc.Q.w20>>(EDwindow.w2 + 4);
        tED.word = tED.b13? 8191:tED.w13; // limit range to 13 bits (theoretical = 13.46b)

        if (tED.w13<2) // negative log output
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
                    sig.w5 = (i<4)? tED.w13<<(4-i) : tED.w13>>(i-4);
                    tail.w4=(int)(4.0*log((double)sig.w5/16.0)/log(2.0)+0.7)-2;
                    if (sig.w5 == 20 || sig.w5 == 28) tail.w4++; // compensation
                    break; 
                } 
            } 
            EDOut->D.w6 = (tED.w13<2)? 0 : (unsigned)((signed)(exp.w4 << 2) + tail.w4); 
        }
    }
}
// end of bMojave_EnergyDetect()

// ================================================================================================
// FUNCTION  : bMojave_CorrelationQuality()
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
void bMojave_CorrelationQuality(wiWORD BCOutR, wiWORD BCOutI, wiBoolean CQEnB, wiBoolean CSEnB, 
                                wiUWORD CQwindow, wiUWORD SSwindow, wiUReg *CQOut, wiUReg *CQPeak)
{
    wiUWORD newPeak, x2, y, z, s;

    static wiReg   dBCOutR, dBCOutI;
    static wiUReg  RdAdr, WrAdr, WrEnB, RAMInit, CQClr, OutEnB, state;
    static wiUWORD cqRAM[32]; // RAM - only 22 of 32 addresses used

    // ---------------------------------------------
    // (WiSE) return to avoid unnecessary processing
    // ---------------------------------------------
    if (CQEnB && !state.Q.w2 && !CQOut->D.word) return; 

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(dBCOutR);
    ClockRegister(dBCOutI);
    ClockRegister(RdAdr);
    ClockRegister(WrAdr);
    ClockRegister(WrEnB);
    ClockRegister(RAMInit);
    ClockRegister(CQClr);
    ClockRegister(OutEnB);
    ClockRegister(state);

    // ----------
    // Controller
    // ----------
    switch (state.Q.w2)
    {
        case 0: // idle
            OutEnB.D.word  = 1;
            if (!CQEnB) state.D.word = 1;
            break;

        case 1: // initialization
            OutEnB.D.word= 0;
            state.D.word = (RdAdr.Q.w5==21)? 2:1;
            break;

        case 2: // operational
            OutEnB.D.word = 0;
            if (!CQEnB) { state.D.word = 2; } // OutEnB.D.word = 0; }
            else       { state.D.word = 3; } // OutEnB.D.word = 1; }
            break;

        case 3: // output result
            OutEnB.D.word= 1;
            state.D.word = 0;
            break;
    }
    WrAdr.D.word  = state.Q.w2? (WrAdr.Q.w5+1)%22 : 21;
    RdAdr.D.word  = state.Q.w2? (RdAdr.Q.w5+1)%22 :  0;
    WrEnB.D.word  = state.Q.w2? 0:1;
    RAMInit.D.word= state.Q.b1? 0:1; // initialize in states 0,1
    CQClr.D.word  = state.Q.w2? 0:1;

    // -----------
    // Wire Inputs
    // -----------
    if (state.Q.w2) {
        dBCOutR.D.word = BCOutR.w9 >> 2;
        dBCOutI.D.word = BCOutI.w9 >> 2;
    }
    // -----------
    // Calculation
    // -----------
    x2.word = SQR(dBCOutR.Q.w7) + SQR(dBCOutI.Q.w7);           // x2 = ||BCOut||^2
    y.word  = RAMInit.Q.b0? 0 : cqRAM[(RdAdr.Q.w5+21)%22].w16; // retrieve accumulator into y
    y.word  = y.w16 + x2.w14;                                  // y += x2
    s.word = !CSEnB? CQwindow.w2 : SSwindow.w3;                // s = log2(number of symbols)
    z.word = y.w16 >> s.w3;                                    // z = y/(2^s)

    if (!WrEnB.Q.b0) cqRAM[WrAdr.Q.w5].word = y.w16;            // store accumulator

    // -----------
    // Peak Search
    // -----------
    newPeak.b0 = (CQOut->Q.w16 < z.w13)? 1:0;

        if (CQClr.Q.b0)                 CQOut->D.word = 0;            // clear output
    else if (!OutEnB.Q.b0 && newPeak.b0) CQOut->D.word = z.w13;        // update output
    else                                CQOut->D.word = CQOut->Q.w13; // hold output

    if (!OutEnB.Q.b0 && newPeak.b0) CQPeak->D.word = WrAdr.Q.w5;
}
// end of bMojave_CorrelationQuality()

// ================================================================================================
// FUNCTION  : bMojave_AGC()
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
void bMojave_AGC(wiBoolean AGCEnB, wiBoolean AGCInitGains, wiUWORD EDOut, wiBoolean AGCbounded, 
                 wiBoolean LgSigAFE, wiUReg *LNAGain, wiUReg *AGain, wiReg *DGain, wiUReg *LargeSignal, 
                 wiUReg *GainUpdate, wiUWORD *AGCAbsEnergy, wiUReg *AGCAbsPwr, MojaveRegisters *pReg)
{
    static wiUReg    GainLimit;   // flag for maximum attenuation
    static wiReg     XGain;       // internal AGain+DGain setting
    static wiReg     negGain;     // negative gain used to adjust measured energy
    static wiBoolean dAGCEnB;     // delayed version of AGCEnB
    static wiUWORD   UpdateCount; // gain update counter (4-bit)
    
    wiBoolean UpdateGain = (!AGCEnB && dAGCEnB && !AGCInitGains)? 1:0;

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(GainLimit);
    ClockRegister(XGain);
    ClockRegister(negGain);

    // ---------------
    // Initialize Gain
    // ---------------
    if (AGCInitGains)
    {
        DGain->D.word      = 0;
        XGain.D.word       = pReg->bInitAGain.w6;
        AGain->D.word      = pReg->bInitAGain.w6;
        LNAGain->D.word    = 3; // b0=Interface LNAGain, b1=Interface LNAGain2
        GainLimit.D.word   = 0;
        negGain.D.word     = 0;
        LargeSignal->D.word= 0;
        GainUpdate->D.word = 0;
        UpdateCount.word   = 0;
        AGCAbsPwr->D.word  = 0;
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
        tAbsEnergy.w9   = (signed  )(EDOut.w6 + 2*negGain.Q.w8) - pReg->bPwr100dBm.w5;
        AGCAbsPwr->D.w8 = (unsigned)max(0, tAbsEnergy.w9); // limit to non-negative value

        if (UpdateGain) AGCAbsEnergy->w8 = AGCAbsPwr->D.w8; // update AbsEnergy only on gain update
    }
    else AGCAbsEnergy->word = 0;

    // -----------
    // Update Gain
    // -----------
    if (UpdateGain)
    {
        wiWORD  gainError;                // negative measured gain error in linear region
        wiWORD  gainUpdate;               // gain update in linear region
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

        GainLimit.D.word = (UpdateCount.w4==pReg->bMaxUpdateCount.w4)? 1:0; // check for update count limit
        UpdateCount.w4++;                 // increment gain update counter

        // ------------------
        // Flag Large Signals
        // ------------------
        validLgSigAFE = (!pReg->RXMode.b0 && LgSigAFE && LNAGain->Q.b0)? 1:0; // radio large signal detector
        validLgSigDFE = (EDOut.w6>pReg->bThSigLarge.w6)? 1:0;                 // baseband (ED)

        // ---------------------
        // Calculate Gain Update
        // ---------------------
        if (validLgSigAFE)
        {
            LNAGain1.w2 = 0;          // switch LNAs to low gain state
            XGain1.w7   = XGain.Q.w7; // no change in AGain
        }
        else if (validLgSigDFE)
        {  
            LNAGain1.w2 = LNAGain->Q.w2;                            // no change to LNAGain
            XGain1.w7   = XGain.Q.w7 - (signed)pReg->bStepLarge.w5; // reduce gain by StepLarge
        }
        else
        {  
            LNAGain1.w2   = LNAGain->Q.w2;                                   // no change to LNAGain
            gainError.w5  = ((signed)pReg->bRefPwr.w6 - (signed)EDOut.w6)/2; // measured gain error
            gainUpdate.w5 = (AGCbounded && gainError.w5>0)? 0:gainError.w5;  // gain correction
            XGain1.w7     = XGain.Q.w7 + gainUpdate.w5;                      // updated AGain
        }
        // -------------------------------------------
        // Check for LNA Switching During XGain Update
        // -------------------------------------------
        dXGain1.w7 = max(0, (signed)pReg->bInitAGain.w6 - XGain1.w7); // net change in XGain (limit to attenuation)
        switchLNA  = (LNAGain1.b0 && (dXGain1.w7>(signed)pReg->bThSwitchLNA.w6 ))? 1:0;
        switchLNA2 = (LNAGain1.b1 && (dXGain1.w7>(signed)pReg->bThSwitchLNA2.w6))? 1:0;

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
        if (pReg->RXMode.w2==2) // 11b mode: CCK AGC Only
        {                      // ----------------------
            if (dXGain3.w7>=0)
            {  // XGain covers entire DGain range and part of AGain
                AGain1.w7 = (signed)pReg->bInitAGain.w6 - dXGain3.w7;
                DGain1.w5 = -(signed)pReg->DAmpGainRange.w4;
            }
            else if (dXGain2.w7>0)
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
            if (dXGain3.w7>=0)
                DGain1.w5 = -(signed)pReg->DAmpGainRange.w4;
            else
                DGain1.w5 = -dXGain2.w7;

            AGain1.w7 = (signed)pReg->bInitAGain.w6;
        }
        AGain2.word = limit(AGain1.w7, 0,63);

        // ------------------------------------------------------
        // Check for Attenuation Limit and Register Gain Settings
        // ------------------------------------------------------
        AGain->D.word = GainLimit.Q.b0? AGain->Q.w6:AGain2.w6;
        DGain->D.word = GainLimit.Q.b0? DGain->Q.w5:DGain1.w5;
        XGain.D.word  = XGain2.w7;
        LNAGain->D.w2 = LNAGain2.w2;
        negGain.D.w8 = dXGain2.w7 + (signed)(LNAGain2.b1? 0:pReg->bStepLNA2.w5) + (signed)(LNAGain2.b0? 0:pReg->bStepLNA.w5);
        LargeSignal->D.b0 = (validLgSigAFE || validLgSigDFE || switchLNA || switchLNA2) && !GainLimit.Q.b0;
        GainUpdate->D.b0  = (LargeSignal->D.b0 || (gainUpdate.w5!=0)) && !GainLimit.Q.b0;
    }
    dAGCEnB = AGCEnB; // 1T delayed version of AGCEnB
}
// end of bMojave_AGC()

// ================================================================================================
// FUNCTION  : bMojave_EstimateCFO
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate Carrier Frequency Offset in the Received Signal
// Parameters: CFOeEnB       -- enable CFO estimation
//             CFOeGet       -- get CFO estimate
//             BCOutR        -- Barker correlator output, real, 9 bit
//             BCOutI        -- Barker correlator output, imag, 9 bit
//             CFO           -- estimated CFO = 8192/11MHz * fo
// ================================================================================================
void bMojave_EstimateCFO(wiBoolean CFOeEnB, wiBoolean CFOeGet, wiWORD BCOutR, wiWORD BCOutI, 
                         wiReg *CFO, MojaveRegisters *pReg)
{
    const wiWORD cfoTable[64] = {  0,  4,  7, 11, 15, 18, 22, 26, 29, 32, 36, 39, 43, 46, 49, 52,
                                            55, 58, 61, 64, 66, 69, 71, 74, 76, 79, 81, 83, 85, 87, 89, 91,
                                            93, 95, 97, 98,100,102,103,105,106,108,109,110,112,113,114,115,
                                116,118,119,120,121,122,123,124,125,126,126,127,128,129,130,130 };
    
    static wiWORD BCOutR_RAM[22], BCOutI_RAM[22]; // RAM - delay line for BCOut
    static wiReg AcmAxR, AcmAxI;                  // autocorrelator accumulator
    static wiReg ratio;                            // ratio used to calculate angle
    static wiUInt addr;                           // RAM address
    static int count;                               // number of input samples

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(AcmAxR);
    ClockRegister(AcmAxI);
    ClockRegister(ratio);

    // ------------------------------------
    // Initialization (Estimation Disabled)
    // ------------------------------------
    if (CFOeEnB == 1)
    {
        count = 0;
        AcmAxR.D.word = AcmAxI.D.word = 0;
        addr = 0;
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
        dBCOutR.word = BCOutR_RAM[addr].w8;
        dBCOutI.word = BCOutI_RAM[addr].w8;

        BCOutR_RAM[addr].word = BCOutR.w9>>1;
        BCOutI_RAM[addr].word = BCOutI.w9>>1;
        addr = (addr+1)%22;

        // ---------------------------------
        // Autocorrelation with lag 22/22MHz
        // ---------------------------------
        if (count>=22)
        {
            wiUWORD aR, aI;

            // autocorrelation
            axR.word = ((BCOutR.w9>>1)* dBCOutR.w8 + (BCOutI.w9>>1) * dBCOutI.w8) >> 4;
            axI.word = ((BCOutI.w9>>1)* dBCOutR.w8 - (BCOutR.w9>>1) * dBCOutI.w8) >> 4;

            // magnitude of quadrature components
            aR.word = abs(axR.w12);
            aI.word = abs(axI.w12);

            // accumulate large magnitude samples
            if (!pReg->bCFOQual.b0 || (aR.w11+aI.w11 > 64) || (aR.w11 > 45) || (aI.w11 > 45))
            {
                // accumulate autocorrelation (keep real term positive)
                if (axR.w12>0)
                {
                    AcmAxR.D.word = AcmAxR.Q.w20 + axR.w12;
                    AcmAxI.D.word = AcmAxI.Q.w20 + axI.w12;
                }
                else if (axR.w12<0)
                {
                    AcmAxR.D.word = AcmAxR.Q.w20 - axR.w12;
                    AcmAxI.D.word = AcmAxI.Q.w20 - axI.w12;
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

            for (k=0; (AcmAxR.Q.w20>>k)>=16; k++) // compute order of AcmAxR
                ;
            // ------------------------------------------------------------------
            // reduce resolution of divider input (includes limiter on Imag term)
            // ------------------------------------------------------------------
            xxReal.word = AcmAxR.Q.w20 >> k;
            xxImag.word = (k>4)? (AcmAxI.Q.w20>>(k-5)):(AcmAxI.Q.w20<<(5-k));
            xxImag.word = limit(xxImag.w20, -1024,1023); // limit to 11bits

            ratio.D.word = xxReal.w5? (xxImag.w11/xxReal.w5):0;
            ratio.D.word = limit(ratio.D.w11, -59,59); // limit size of atan table

            // Compute CFO = round{4096/pi/11 * atan(ratio/32)}
            arg.word = abs(ratio.Q.w7);
            CFO->D.word = (ratio.Q.b6? -1:1) * cfoTable[arg.w6].w8;
        }
        else
            CFO->D.word = 0; // clear output until estimate is requested

        count++; // increment sample counter
    }
}
// end of bMojave_EstimateCFO()

// ================================================================================================
// FUNCTION  : bMojave_CorrectCFO()
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
void bMojave_CorrectCFO(wiBoolean CFOcEnB, wiBoolean TransCCK, wiBitDelayLine dDeciFreeze, wiWORD CFO, wiWORD cCP, 
                                wiWORD CFOcInR, wiWORD CFOcInI, wiWORD *CFOcOutR, wiWORD *CFOcOutI, wiBoolean posedgeCLK11MHz)
{
    const wiWORD sinTable[64] = {  0, -3, -6, -9,-12,-15,-18,-20,-23,-25,-27,-28,-30,-31,-31,-32,
                                -32,-32,-31,-31,-30,-28,-27,-25,-23,-20,-18,-15,-12, -9, -6, -3,
                                  0,  3,  6,  9, 12, 15, 18, 20, 23, 25, 27, 28, 30, 31, 31, 32,
                                            32, 32, 31, 31, 30, 28, 27, 25, 23, 20, 18, 15, 12,  9,  6,  3};
    const wiWORD cosTable[64] = {-32,-32,-31,-31,-30,-28,-27,-25,-23,-20,-18,-15,-12, -9, -6, -3,
                                  0,  3,  6,  9, 12, 15, 18, 20, 23, 25, 27, 28, 30, 31, 31, 32,
                                            32, 32, 31, 31, 30, 28, 27, 25, 23, 20, 18, 15, 12,  9,  6,  3,
                                  0, -3, -6, -9,-12,-15,-18,-20,-23,-25,-27,-28,-30,-31,-31,-32};

    static wiBitDelayLine dTransCCK;
    static wiReg xR, xI, yR, yI, dcCP;
    static unsigned count;
    static wiWORD q;
    wiWORD   c,s,p;

    // ---------------
    // Clock Registers
    // ---------------
    if (posedgeCLK11MHz)
    {
        ClockRegister(xR); ClockRegister(xI);
        ClockRegister(yR); ClockRegister(yI);
        ClockRegister(dcCP);
        ClockDelayLine(dTransCCK, TransCCK);

        // -----------
        // Wire Inputs
        // -----------
        xR.D.word = CFOcInR.w8;
        xI.D.word = CFOcInI.w8;

        xR.D.word = dTransCCK.delay_2T? xR.Q.w8:CFOcInR.w8;
        xI.D.word = dTransCCK.delay_2T? xI.Q.w8:CFOcInI.w8;

        dcCP.D.word = cCP.w6;

        if (CFOcEnB)
        {
            yR.D.word = yI.D.word = 0;
            q.word = 0;
            count = 0;
        }
        else if (!dTransCCK.delay_3T && !dDeciFreeze.delay_1T) // correction enabled -------
        {
            q.w13 = (count<2)? q.w13:(q.w13+CFO.w8);      // 4096 -> pi, allow wrap-around
            p.w6  = (q.w13>>7) + dcCP.Q.w6;                // 32 -> pi, allow wrap-around

            c.word = cosTable[p.w6+32].w7;                // cosine/sine lookup
            s.word = sinTable[p.w6+32].w7;                // (c + j*s) = exp{j*pi*p/32}

            yR.D.word = (xR.Q.w8*c.w7 + xI.Q.w8*s.w7)/32; // no bitshift allowed for divider
            yI.D.word = (xI.Q.w8*c.w7 - xR.Q.w8*s.w7)/32;

            count++;
        }
    }
    // ------------
    // Wire Outputs
    // ------------
    CFOcOutR->word = yR.Q.w8;
    CFOcOutI->word = yI.Q.w8;
}
//end of bMojave_CorrectCFO()

// ================================================================================================
// FUNCTION  : bMojave_ChanEst()
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
//             RX         -- bMojave RX data structure
//             pReg       -- configuration registers
// ================================================================================================
void bMojave_ChanEst(wiBoolean CEEnB, wiBoolean CEMode, wiBoolean GetCE, wiWORD BCOutR, wiWORD BCOutI,
                            wiUWORD DMOut, wiBoolean DMOutValid, wiBoolean DBPSK, wiBoolean *CEDone,
                            bMojave_RX_State *RX, MojaveRegisters *pReg)
{
    static wiUWORD phase = {0};            // modulated symbol phase

    static wiWORD  dBCOutR[32],dBCOutI[32];// Barker correlator samples (RAM)
    static wiUWORD WrAdr, RdAdr;           // address for dBCOut RAM

    static wiUWORD dDMOut[4];   // buffer for demodulator output (symbol decisions, 2b wide)
    static wiUWORD dMod[4];     // buffer for modulation type
    static wiUInt  SymBufCount; // number of symbols in the buffers
    static wiUInt  NSym;        // number of symbols summed in correlation accumulators
    static wiUInt  count;       // counter for the number of buffered BCOut symbols

    static wiWORD aReal[22], aImag[22]; // correlator sum after phase correction
    static wiWORD hReal[22], hImag[22]; // channel estimate after correction filtering

    static int CNTDM=0;         // count symbol decisions
    static int CNTdelay=0;      // delay count
    static int CNTstart=0;      // start counting
    static int save_Nh,save_Np,save_Np2;

    // --------------
    // Initialization
    // --------------
    if (CEEnB)
    {
        WrAdr.word = RdAdr.word = 0;
        count = 0;
        SymBufCount = 0;
        NSym = 0;
        CNTDM = 0;
        CNTdelay = 0;
        CNTstart = 0;
        *CEDone = 0; // added 2005-07-21 19:42 (otherwise holds over from previous packet)
    }
    // -------------------------
    // Active Channel Estimation
    // -------------------------
    if (!CEEnB)
    {
        int i;

        // count symbol decisions and activate CEDone
        if (CNTstart)   CNTdelay++;
        if (DMOutValid) CNTDM++;
        if (CNTDM==(CEMode==0? 1<<pReg->CENSym1.w3 : (1<<pReg->CENSym2.w3))) CNTstart = 1;
        if (GetCE == 1 && DMOutValid==1) CNTstart = 1;
        if (CNTdelay==23+(save_Nh/4)) // delay at T=1/11MHz
        {
            CNTstart = 0;
            CNTdelay = 0;
            *CEDone  = 1;

            RX->Nh = save_Nh;
            RX->Np = save_Np;
            RX->Np2= save_Np2;
            for (i=0; i<22; i++)
            {
                RX->hReal[i].word = hReal[i].w9;
                RX->hImag[i].word = hImag[i].w9;
            }
        }
        // -------------------------------------------
        // Buffer the Barker Correlator Output Samples
        // -------------------------------------------
        dBCOutR[WrAdr.w5].word = BCOutR.w9;
        dBCOutI[WrAdr.w5].word = BCOutI.w9;
        WrAdr.w5++; count++;

        // ----------------------------------------------------
        // Buffer the Demodulator Output and Type (DBPSK/DQPSK)
        // ----------------------------------------------------
        if (DMOutValid==1)
        {
            dMod  [SymBufCount].word = DBPSK;
            dDMOut[SymBufCount].word = DMOut.w2;
            SymBufCount++;
        }
        // --------------------
        // Error/Range Checking
        // --------------------
        if (count>32 || SymBufCount>2) { // check for buffer overrun (model only)
            wiPrintf("*** bMojave_ChanEst ERROR: count=%d, SymBufCount=%d\n",count,SymBufCount);
            WICHK(WI_ERROR_RANGE); // place an entry in the log file
            exit(1);
        }
        // ------------------------------
        // Channel Estimation Calculation
        // ------------------------------
        if (count>=22 && SymBufCount>0)
        {
            int cReal, cImag; // QPSK constellation coordinates

            // ------------------------------------------------
            // Update Phase Modulation Using Demodulator Output
            // Phase Mapping: 0=pi/4, 1=3pi/4, 2=5pi/4, 3=7pi/4
            // ------------------------------------------------
            if (NSym==0) 
            {  
                phase.word = 0;                       // initial phase
                for (i=0; i<22; i++) 
                    aReal[i].word = aImag[i].word = 0; // clear the accumulators
            }
            else if (dMod[0].b0)   
            {  
                if (dDMOut[0].b0) phase.w2 += 2; // DBPSK
            }
            else {  
                switch (dDMOut[0].w2)            // DQPSK (gray coded)
                {
                    case 0: phase.w2 += 0; break;
                    case 1: phase.w2 += 1; break;
                    case 2: phase.w2 += 3; break;
                    case 3: phase.w2 += 2; break;
                }
            }
            // --------------------------------------
            // Map Phase to Cartesian Coordinates
            // 0=(+1+j), 1=(-1+j), 2=(-1-j), 3=(+1-j)
            // --------------------------------------
            cReal = phase.b0^phase.b1 ? -1:1;
            cImag = phase.b1          ? -1:1;
            
            // -------------------------------------------------------
            // Accumulate Correlations After Removing Phase Modulation
            // -------------------------------------------------------
            for (i=0; i<22; i++)
            {
                unsigned addr = (RdAdr.w5 + i) % 32;
                aReal[i].w16 += (dBCOutR[addr].w9*cReal + dBCOutI[addr].w9*cImag);
                aImag[i].w16 += (dBCOutI[addr].w9*cReal - dBCOutR[addr].w9*cImag);
            }
            // ---------------------------
            // Update Counters and Buffers
            // ---------------------------
            RdAdr.w5 += 11;       // move BCOut buffer read address forward 1 symbol (11 chips)
            count    -= 11;       // decrement count of BCOUt buffered samples
            dDMOut[0] = dDMOut[1];// reduce FIFO
            dMod[0]   = dMod[1];  // reduce FIFO
            SymBufCount--;        // decrement symbol buffer counter
            NSym++;               // increment symbol counter

            // --------------------------------------------------------------------------------
            // Correction Filtering
            // Reduce error due to difference between delta function and Barker autocorrelation
            // --------------------------------------------------------------------------------
          if ((CEMode==0 && NSym==(unsigned)(1<<pReg->CENSym1.w3)) || (CEMode==1 && NSym==(unsigned)(1<<pReg->CENSym2.w3) && !GetCE) || (GetCE && CNTdelay==0))
            {
                const int g[29] = {2,0,2,0,5,0,6,0,7,0,7,0,8,0,37,0,8,0,7,0,7,0,6,0,5,0,2,0,2};
                const int Ng = 29; // filter length
                const int g0 = 14; // center tap position (group delay)

                wiWORD xR[50]={0}, xI[50]={0}; // shift register for filter (unused terms set to zero)
                wiWORD yR[22], yI[22];         // filter output before scaling
                int i,j,k,m,n;
                
                // ---------------------------------------------------------------------------------
                // Scale Accumulated Values -- net scaling is 4*sqrt(2) including Barker Correlator
                // ...note values are offset in xR/xI by g0, the correction filter center tap offset
                // ---------------------------------------------------------------------------------
                for (i=0; i<22; i++)
                {
                    xR[g0+i].word = aReal[i].w16 * 16 / ((int)NSym*2*11);
                    xI[g0+i].word = aImag[i].w16 * 16 / ((int)NSym*2*11);
                }
                // -----------------------------------------------------------------
                // Apply Correction Filter -- net scaling after divider is 4*sqrt(2)
                // -----------------------------------------------------------------
/**
wiPrintf("\n");
wiPrintf("xR=["); for (i=0; i<22; i++) wiPrintf("%4d",xR[g0+i].word); wiPrintf("], NSym=%d\n",NSym);
wiPrintf("xI=["); for (i=0; i<22; i++) wiPrintf("%4d",xI[g0+i].word); wiPrintf("]\n");
/**/
                for (i=0; i<22; i++)
                {
                    yR[i].word = yI[i].word = 0;
                    for (k=0; k<Ng; k++)
                    {
                        j = (Ng-1)-k+i;
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
                    Th1.word = peak.w9 >> (CEMode==0? pReg->hThreshold1.w3:pReg->hThreshold3.w3);
                    Th2.word = peak.w9 >> pReg->hThreshold2.w3;

                    n=m=22;
                    for (k=0; k<22; k++)
                    {
                        mag.word = abs(hReal[k].w9) + abs(hImag[k].w9);

                        if (mag.w9 <= Th1.w9) 
                            hReal[k].word = hImag[k].word = 0;
                        else 
                            n=min(n,k); // flag first term >Th1

                        if (mag.w9 > Th2.w9) m = min(m,k); // flag first term >Th2
                    }
/**
wiPrintf("|h|=["); for (k=0; k<22; k++) wiPrintf("%4.0f",sqrt(SQR(hReal[k].w9)+SQR(hImag[k].w9))); wiPrintf("]\n");
wiPrintf("     "); for (k=0; k<10; k++) wiPrintf("    "); wiPrintf("   ^\n");
/**/
                }
                // shift length 22-n term response to start at tap 0
                for (i=n,j=0; i<22; i++,j++)  
                {
                    hReal[j].word = hReal[i].w9;
                    hImag[j].word = hImag[i].w9;
                }
                for (; j<22; j++) 
                    hReal[j].word = hImag[j].word = 0; // clean up end of response (C only)
                save_Nh  = 22 - n; // adjusted filter length
                save_Np  = 10 - n; // adjusted RAKE receiver filter delay (precursors)
                save_Np2 = 10 - m; // adjusted CCK receiver filter delay (precursors)

/**
wiPrintf("ChanEst: Nh=%2d, Np=%2d, Np2=%d\n",save_Nh,save_Np,save_Np2);
wiPrintf("         hReal = ["); for (i=0; i<save_Nh; i++) wiPrintf("%4d",hReal[i]); wiPrintf("]\n");
wiPrintf("         hImag = ["); for (i=0; i<save_Nh; i++) wiPrintf("%4d",hImag[i]); wiPrintf("]\n");
/**/
                if (CEMode==0) // save coarse estimate
                {
                    for (i=0; i<22; i++)
                    {
                        RX->hcReal[i].word = hReal[i].word;
                        RX->hcImag[i].word = hImag[i].word;
                        RX->Nhc = save_Nh;
                        RX->Npc = save_Np;
                        RX->Np2c = save_Np2;
                    }
                }
            }
        }
    }     
}
// end of bMojave_ChanEst()

// ================================================================================================
// FUNCTION  : bMojave_RX_SP()
// ------------------------------------------------------------------------------------------------
// Purpose   : FIFO between CFO correction output and CCK demodulator input
// Parameters: uReal, uImag -- CFO correction output, real, imag, 8-bit
//             wRptr, wIptr -- pointers to last output sample from FIFO
//             Np2          -- number of extra precursor terms used for CCK demodulation
//             SPEnB        -- enable FIFO
//             TransCCK     -- input hold signal to reduce buffer delay
//             dDeciFreeze  -- input hold signal from decimator
// ================================================================================================
void bMojave_RX_SP(wiWORD *uReal, wiWORD *uImag, wiWORD **wRptr, wiWORD **wIptr, int Np2, 
                   wiBoolean SPEnB, wiBoolean TransCCK, wiBitDelayLine dDeciFreeze)
 {
    static wiUInt wrptr, rdptr;       // RAM write/read address pointers
    static wiBitDelayLine dTransCCK;  // TransCCK signal delay line (11 MHz)

    ClockDelayLine(dTransCCK, TransCCK);

    // -------------------
    // Initialize CCK FIFO
    // -------------------
    if (SPEnB) 
    {
        wrptr = 11+8; // delay set by CMF(11) + DSSS Symbol Period-Demodulation Delay(8)
        rdptr = 0;
    }
    // ----------------------------------------------------------------
    // Adjust Buffer Depth for CCK Precursors
    // ...move CCK input back to include Np2 additional precursor terms
    // ----------------------------------------------------------------
    if (dTransCCK.delay_0T && !dTransCCK.delay_1T) // update at start of CCK demodulation
        rdptr -= Np2; 

    // -------------------------------------------------
    // Write Input to FIFO Except During TransCCK period
    // -------------------------------------------------
    if (!dTransCCK.delay_4T && !dDeciFreeze.delay_2T)
    {
        bRX.wReal[wrptr].word = uReal->w8;
        bRX.wImag[wrptr].word = uImag->w8;
        wrptr++;
    }
    // --------------------
    // FIFO Output Pointers
    // --------------------
    *wRptr = bRX.wReal + rdptr;
    *wIptr = bRX.wImag + rdptr;
    rdptr++;
}
// end of bMojave_RX_SP()

// ================================================================================================
// FUNCTION  : bMojave_RX_CMF()
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
void bMojave_RX_CMF(wiBoolean CMFEnB, wiBitDelayLine DMSym, wiBitDelayLine dDeciFreeze, wiWORD *uReal, wiWORD *uImag,
                    wiWORD **SSMatOutR, wiWORD **SSMatOutI, int Nh, wiWORD *hReal, wiWORD *hImag)
{
    static wiBoolean dCMFEnB;     // delayed CMFEnB signal
    static wiWORD dR[23], dI[23]; // filter delay line
    static wiWORD hR[22], hI[22]; // channel response
    static int Nhs;               // channel length
    static wiWORD zReal[21], zImag[21]; // delay line for CMF outputs

    if (CMFEnB) 
    {
        Nhs=0; 
    }
    else
    {
        int i,j,k;

        if (dCMFEnB)        // initialize on first sample
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
                Nhs = Nh;
            }
            for (; k<22; k++) hR[k].word = hI[k].word = 0;
        }
        // -----------------------------------------------------
        // Input Signal Delay Line (With Shift as Divide-by-Two)
        // -----------------------------------------------------
        if (!dDeciFreeze.delay_2T)
        {
            for (i=22; i>0; i--) dR[i] = dR[i-1]; dR[0].word = uReal->w8 >> 1;
            for (i=22; i>0; i--) dI[i] = dI[i-1]; dI[0].word = uImag->w8 >> 1;
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
        for (k=0, j=Nhs; k<Nhs; k++, j--)
        {
            zReal[0].w18 += hR[k].w8*dR[j].w7 + hI[k].w8*dI[j].w7;
            zImag[0].w18 += hR[k].w8*dI[j].w7 - hI[k].w8*dR[j].w7;
        }
        zReal->word = zReal->w18>>5;
        zImag->word = zImag->w18>>5;

        zReal->word = limit(zReal->w13, -255,255);
        zImag->word = limit(zImag->w13, -255,255);

        /*** ALIGNMENT TEST CODE ***
        {
            int kpk=0, h2, h2pk=-1;
            for (k=0; k<Nhs; k++) {
                h2 = SQR(hR[k].w9)+SQR(hI[k].w9);
                if (h2>h2pk) {
                    h2pk = h2;
                    kpk = k;
                }
            }
            for (k=0, j=-Nhs; k<Nhs; k++, j++) {if (k==kpk) wiPrintf("CMF:: %4d -->%4d....................",uReal[j].w8,uReal->w9);}
            wiPrintf("CMF dR=["); 
            for (k=0, j=Nhs; k<Nhs; k++, j--) {
                if (k==kpk) wiPrintf("<<"); 
                wiPrintf("%4d",dR[j].w8); 
                if (k==kpk) wiPrintf(">>");
            } 
            wiPrintf("] --> %4d\n",zReal->w9);
            //      wiPrintf("|h|      =["); for (k=0, j=-Nhs; k<Nhs; k++, j++) wiPrintf("%4.0f",sqrt(SQR(hR[k].w9)+SQR(hI[k].w9))); wiPrintf("]\n");
        }
        /**/
    }
    // --------------------------------
    // Wire Outputs With Register Delay
    // --------------------------------
    *SSMatOutR = zReal+1;
    *SSMatOutI = zImag+1;

    dCMFEnB = CMFEnB;
}
// end of bMojave_RX_CMF()

// ================================================================================================
// FUNCTION  : bMojave_RX_DSSS()
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
void bMojave_RX_DSSS(wiBoolean DMEnB, wiWORD *CFOOutR, wiWORD *CFOOutI, wiWORD *CMFOutR, wiWORD *CMFOutI,
                            wiReg *RBCOutR, wiReg *RBCOutI,
                    wiUReg *SSCorOutValid,
                    wiUReg  *SSSliOutR, wiUReg *SSSliOutI,
                    wiUReg *DMOut, // symbol decision (1 or 2 bit output)
                    wiUReg *DMOutValid, // symbol decision valid
                    wiBoolean DBPSK,  // modulation = {1=DBPSK, 0=DQPSK}
                    wiBoolean CEDone,
                    wiBoolean HDREnB,
                    wiBitDelayLine DMSym,
                    wiWORD *BFReal, wiWORD *BFImag
                    )
{
    const int BS[11] = {-1,-1,-1, 1, 1, 1,-1, 1, 1,-1, 1}; // Barker sequence
    int n;
    unsigned qReal, qImag;           // quantized correlator output   
    wiWORD   yReal, yImag;           // detector intermediate variable

    static int NSymRake;               // number of symbols processed by RAKE receiver
    static int NSymHDR;               // number of header symbols processed
    static wiWORD  pp1Real, pp1Imag;
    static wiUWORD pp2Real, pp2Imag;

    static wiReg CBCOutR, CBCOutI;
    static wiBoolean CBCEnB, RBCEnB;
    static wiUReg state;

    ClockRegister(CBCOutR);
    ClockRegister(CBCOutI);
    ClockRegister(state);

    if (DMEnB) ResetRegister(state); // initialize demodulator
    DMOutValid->D.word = 0;         // default output unless set below
    SSCorOutValid->D.word = 0;      // default output unless set below

    // ----------
    // Controller
    // ----------
    switch (state.Q.word)
    {
        case 0:
            CBCEnB = 1;
            RBCEnB = 1;
            pp1Real.word = pp1Imag.word = 1;
            pp2Real.word = pp2Imag.word = 1;
            NSymRake = 0;
            NSymHDR = 0;
            CBCOutR.D.word = CBCOutI.D.word = 0;
            RBCOutR->D.word = RBCOutI->D.word = 0;
            CBCEnB = RBCEnB = 1;
            if (!DMEnB) state.D.word = 1;
            break;
        case 1:
            CBCEnB = 0; // correlation demod correlator enabled
            RBCEnB = 1; // rake Barker correlator disabled
            if (CEDone) state.D.word = 2; // first channel estimation done
            break;
        case 2:
            if (DMSym.delay_1T) state.D.word = 3; // first channel estimation loaded
            break;
        case 3:
            if (DMSym.delay_2T) state.D.word = 4; // delay due to CMF complete
            break;
        case 4:
            RBCEnB = 0;                          // enable rake Barker correlator
            if (DMSym.delay_3T) state.D.word = 5; // first symbol demod used to setup previous symbol qReal,Imag
            break;
        case 5:
            CBCEnB = 1; // disable correlation demod
            break;
        default:
            state.D.word = 0;
    }
    // --------------------------------
    // Noncoherent Correlation Receiver
    // --------------------------------
    if (!CBCEnB)
    {
        if (DMSym.delay_11T)
        {
            wiWORD acmR = {0}, acmI = {0};
            for (n=0; n<11; n++) {
                acmR.w12 += BS[n] * (CFOOutR-n-2)->w8;
                acmI.w12 += BS[n] * (CFOOutI-n-2)->w8;
            }
            CBCOutR.D.word = acmR.w12 >> 4;
            CBCOutI.D.word = acmI.w12 >> 4;
//         wiPrintf("DSSS CFOOutR=["); for (n=0; n<11; n++) wiPrintf(" %5d",(CFOOutR-n-2)->w8); wiPrintf("], |CBCOut| =%4d\n",(int)sqrt(acmR.w12*acmR.w12 + acmI.w12*acmI.w12));
        }
        /**
        { // continuous correlation used for alignment experiments
            wiWORD acmR = {0}, acmI = {0};
            for (n=0; n<11; n++) {
                acmR.w12 += BS[n] * (CFOOutR-n-2)->w8;
                acmI.w12 += BS[n] * (CFOOutI-n-2)->w8;
            }
            wiPrintf("|CBCOut| =%4d%c\n",(int)sqrt(acmR.w12*acmR.w12 + acmI.w12*acmI.w12),DMSym.delay_11T? '<':' ');
        } /* end of experimental code
        */
        if (DMSym.delay_12T)
        {
            // phase difference
            yReal.w16 = CBCOutR.Q.w8*pp1Real.w8 + CBCOutI.Q.w8*pp1Imag.w8;

            // decode bit
            DMOut->D.word = (yReal.w16>=0)? 0:1;
//         wiPrintf("DSSS CBCOut =(%3d,%3d) * conj{(%3d,%4d) =%4d  --> d=%d\n",CBCOutR.Q.w8,CBCOutI.Q.w8,pp1Real.w8,pp1Imag.w8,yReal.w16,DMOut->D.word);

//wiPrintf("%5d %5d  %5d %5d  %2d\n",pp1Real.word,pp1Imag.word,CBCOutR.Q.w8,CBCOutI.Q.w8,DMOut->D.word);

            // save the phase of current symbol
            pp1Real.word = CBCOutR.Q.w8;
            pp1Imag.word = CBCOutI.Q.w8;

            DMOutValid->D.word = 1;
        }
    }
    // -------------
    // RAKE Receiver
    // -------------
    if (!RBCEnB)
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
            RBCOutR->D.word = acmR.w13>>1;
            RBCOutI->D.word = acmI.w13>>1;
            SSCorOutValid->D.word = 1;
//         wiPrintf("DSSS CMFOutR = ["); for (n=0; n<11; n++) wiPrintf("%4d",CMFOutR[n+1].w9); wiPrintf("], RBCOut=(%4d,%4d)\n",RBCOutR->D.word,RBCOutI->D.word);
        }
        /**
        {  // continuous correlation used for alignment experiments
            wiWORD acmR = {0}, acmI = {0};
            for (n=0; n<11; n++) {
                acmR.w13 += BS[n] * CMFOutR[n+1].w9;
                acmI.w13 += BS[n] * CMFOutI[n+1].w9;
            }
            wiPrintf("|RBCOut| = |(%4d,%4d)| = %4d%c\n",acmR.w13>>1,acmI.w13>>1,(int)sqrt(SQR(acmR.w13)+SQR(acmI.w13)),DMSym.delay_12T? '<':' ');
        } /* end of experimental code 
        */
        if (DMSym.delay_13T)
        {
            if (DBPSK) // D-BPSK
            {
                // quantize chip coordinates (to +1+j or -1-j)
                qReal = qImag = (RBCOutR->Q.w12 + RBCOutI->Q.w12>=0)? 1:0; 

                // phase difference
                yReal.w3 = ((pp2Real.b0^qReal)? -1:1) + ((pp2Imag.b0^qImag)? -1:1);

                // decode bit
                if (NSymRake>0) DMOut->D.word = (yReal.w3>=0)? 0:1;
            }
            else   // D-QPSK
            {
                // quantize phase
                if     (RBCOutR->Q.w12>=0 && RBCOutI->Q.w12>=0) { qReal= 1; qImag= 1;}
                else if (RBCOutR->Q.w12< 0 && RBCOutI->Q.w12>=0) { qReal= 0; qImag= 1;}
                else if (RBCOutR->Q.w12< 0 && RBCOutI->Q.w12< 0) { qReal= 0; qImag= 0;}
                else                                            { qReal= 1; qImag= 0;}

                // phase difference
                yReal.word = (pp2Real.b0^qReal ? -1:1) + (pp2Imag.b0^qImag ? -1:1);
                yImag.word = (pp2Real.b0^qImag ? -1:1) - (pp2Imag.b0^qReal ? -1:1);

                // decode bit
                DMOut->D.b0 = (yReal.w3+yImag.w3>=0)? 0:1;
                DMOut->D.b1 = (yReal.w3-yImag.w3>=0)? 0:1;
//wiPrintf("%3d %2d  %5d %5d  %2d\n",pp2Real.word,pp2Imag.word,RBCOutR->Q.w12,RBCOutI->Q.w12);
            }
            if (NSymRake>0) DMOutValid->D.word = 1; // first symbol used only to get phase for diff. demod

            SSSliOutR->D.b0 = pp2Real.b0 = qReal;
            SSSliOutI->D.b0 = pp2Imag.b0 = qImag;

            if (!HDREnB) NSymHDR++;
            NSymRake++;

            // feedback filter input
            if (!HDREnB && (NSymHDR>44 || (!DBPSK && NSymHDR>20)))
            {
                for (n=21; n>11; n--)
                {
                    BFReal[n] = BFReal[n-11];
                    BFImag[n] = BFImag[n-11];
                }
                for (n=1; n<=11; n++)
                {
                    BFReal[n].word = qReal? BS[n-1]:-BS[n-1];
                    BFImag[n].word = qImag? BS[n-1]:-BS[n-1];
                }
            }
        }
    }
    // ----------------------------------
    // Save Output Data in RX State Array
    // ----------------------------------
    if (DMOutValid->D.b0)
    {
        bRX.b[bRX.db++].word = DMOut->D.b0;
        if (!DBPSK) bRX.b[bRX.db++].word = DMOut->D.b1;
    }
}
// end of bMojave_RX_DSSS()

// ================================================================================================
// FUNCTION  : bMojave_RX_CCK()
// ------------------------------------------------------------------------------------------------
// Purpose   : CCK Demodulator
// Parameters: 
// ================================================================================================
void bMojave_RX_CCK(wiWORD *wR, wiWORD *wI,
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
    int   i,j,k,l,n,m;

    wiWORD  vReal[8],vImag[8];      // ISI
    wiWORD  yReal[8],yImag[8];      // ISI cancelled observation samples
    wiWORD  xcReal[64],xcImag[64];   // correlator output
    wiWORD  met[64];      // metric to compare
    int     max_met;   // maximum metric
    wiUWORD maxPtr;  // index to the maximum metric
    wiBoolean max_qReal,max_qImag;// quantized correlation at the maximum
    wiBoolean qcReal,qcImag;      // quantized correlator output
    wiWORD  ycReal,ycImag;      // phase difference of two successive symbols

    static wiUWORD ICI[64];    // ICI terms (shared for 5.5/11 Mbps)
    static wiWORD   (*ccReal)[8],(*ccImag)[8];   // pointer to complementary code table
    static wiUWORD NSym; // symbol counter (really only need LSB for odd/even symbol)
    static wiBoolean ppReal, ppImag;
    int Nc;          // number of correlator terms (4 and 64 for 5.5 and 11 Mbps, respectively)

    wiWORD *zReal = CCKMatOutR, *zImag = CCKMatOutI; // rename TVMF outputs

    cckOutValid->word = 0;
    
    // ------------------------------------------------
    // Per-Packet Initialization Using Channel Estimate
    // ------------------------------------------------
    if (*cckInit)
    {
        wiWORD uReal,uImag; // ISI cancelled noiseless samples

        ppReal = SSSliOutR.b0;
        ppImag = SSSliOutI.b0;

        Nc     = (DataRate==55)? 4:64;               // number of correlator terms
        ccReal = (DataRate==55)? CC55Real:CC110Real; // cross-correlation sequence table
        ccImag = (DataRate==55)? CC55Imag:CC110Imag;

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
                    j = (Np-Np2)+m-i;
                    if (j>=0 && j<Nh)
                    {
                        uReal.w12 += hReal[j].w9*ccReal[k][i].w2 - hImag[j].w9*ccImag[k][i].w2;
                        uImag.w12 += hImag[j].w9*ccReal[k][i].w2 + hReal[j].w9*ccImag[k][i].w2;
                    }
                }
                ICI[k].w13 += (uReal.w10*uReal.w10 + uImag.w10*uImag.w10)*2/8/8/2;
            }
        }
        NSym.word = 0;
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
        ccReal = CC110Real; 
        ccImag = CC110Real; // purposely pick the wrong one so this cannot be successfully decoded
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
        vReal[n].w9 = vReal[n].w11/4; // divide-by-4 to account for scaling of (hReal,hImag)
        vImag[n].w9 = vImag[n].w11/4;

        yReal[n].w8 = (wR[n-7].w8 - vReal[n].w9)/2; // simple scaling for resolution reduction
        yImag[n].w8 = (wI[n-7].w8 - vImag[n].w9)/2;
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
        zReal[n].w13 = zReal[n].w16/8; // simple scaling for resolution reduction
        zImag[n].w13 = zImag[n].w16/8;

        // limiter
        zReal[n].w12 = limit(zReal[n].w13, -2047,2047);
        zImag[n].w12 = limit(zImag[n].w13, -2047,2047);
    }
    //wiPrintf("CCK: CCKMatOutR=["); for (n=0; n<8; n++) wiPrintf("%5d",zReal[n].w12); wiPrintf("]\n");

    // --------------------------------
    // find the most likely code
    // based on the Euclidean distance
    // --------------------------------
    Nc = (DataRate==55)? 4:64; // number of correlator sequences

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
        if (!k || met[k].w15>max_met)
        {
            maxPtr.word = k;
            max_met   = met[k].w15;
            max_qReal = qcReal;
            max_qImag = qcImag;
        }
    }
    /**
    {  // Pick Second Largest ['C' Diagnostic Only]
        int max_met2 = -65536;
        double dmindB;
        
        for (k=0; k<Nc; k++)
        {
            if (k == maxPtr.word) continue;
            if (met[k].w15>max_met2) max_met2 = met[k].w15;
        }
        dmindB = 10*log10((double)max_met/(double)abs(max_met2));
        {
            wiPrintf("CCK: w=["); for (n=0; n<8; n++) wiPrintf("%4d",wR[n-7].w8); 
            wiPrintf("], Sym=%2d, max_met=%5d(%5d) dmin=%1.1fdB\n",NSym.word,max_met,max_met2,dmindB);
        }
    }
    /**/
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
    ycReal.w3 = (max_qReal? 1:-1)*(ppReal? 1:-1) + (max_qImag? 1:-1)*(ppImag? 1:-1);
    ycImag.w3 = (max_qImag? 1:-1)*(ppReal? 1:-1) - (max_qReal? 1:-1)*(ppImag? 1:-1);

    // -----------
    // Bit Decoder
    // -----------
    cckOut->word = 0;
    cckOutValid->word = 1;

    cckOut->b0 = ((ycReal.w3+ycImag.w3>0)? 0:1) ^ NSym.b0;
    cckOut->b1 = ((ycReal.w3-ycImag.w3>0)? 0:1) ^ NSym.b0;

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
    for (n=0; n<(DataRate==55? 4:8); n++)
        bRX.b[bRX.db++].word = (cckOut->w8>>n) & 1;

    // save the current phase
    ppReal = max_qReal;
    ppImag = max_qImag;

    CCKWalOutR->word = xcReal[maxPtr.w6].w14>>3; // 12bits are used to share carrier
    CCKWalOutI->word = xcImag[maxPtr.w6].w14>>3; // phase detector with DSSS signal.

    *pccReal = ccReal[maxPtr.w6];
    *pccImag = ccImag[maxPtr.w6];

    // -------------
    // Trace Signals
    // -------------
    if (bRX.EnableTraceCCK)
    {
        for (n=0; n<8; n++)
        {
            k = 8*NSym.word + n;
            bRX.traceCCKInput[k].wReal = wR[n-7].w8;
            bRX.traceCCKInput[k].wImag = wI[n-7].w8;
            bRX.traceCCKInput[k].yReal = yReal[n].w8;
            bRX.traceCCKInput[k].yImag = yImag[n].w8;
            bRX.traceCCKTVMF[k].zReal  = zReal[n].w12;
            bRX.traceCCKTVMF[k].zImag  = zImag[n].w12;
            bRX.traceCCKTVMF[k].NSym   = NSym.w8; // mod-256 value of symbol counter
        }
        for (n=0; n<Nc; n++)
        {
            k = Nc*NSym.word + n;
            bRX.traceCCKFWT[k].xcReal = xcReal[n].w14;
            bRX.traceCCKFWT[k].met    = met[n].w15;
            bRX.traceCCKFWT[k].Nc64   = Nc==64? 1:0;
        }
    }
    NSym.word++;
}
// end of bMojave_RX_CCK()

// ================================================================================================
// FUNCTION  : bMojave_RX_SFD()
// ------------------------------------------------------------------------------------------------
// Purpose   : Find start frame delimitor (SFD) and determine preamble type
// Parameters: SFDEnB      -- enabled SFD
//             DesOut      -- descrambler output (always single bit, 1Mbps mode)
//             DesOutValid -- data on DesOut is valid (strobe)
//             Preamble    -- preamble type (out) (0=long, 1=short)
//             SFDone      -- SFD found
// ================================================================================================
void bMojave_RX_SFD(wiBoolean SFDEnB, wiUWORD DesOut, wiBoolean DesOutValid, 
                    wiUReg *Preamble, wiUReg *SFDDone)
{
    const wiUWORD longSFD  = {0x05CF}; // 0000,0101,1100,1111
    const wiUWORD shortSFD = {0xF3A0}; // 1111,0011,1010,0000
    
    static wiUWORD NSym;      // number of symbols examined
    static wiUWORD d;       // shift register delay line

    if (SFDEnB)              // initialize when not enabled
    {
        NSym.word = 0;
        SFDDone->D.word = 0;
        d.word = 0;
    }
    else
    {
        if (NSym.w5 == 16) // check for SFD after loading 16 symbols
        {
            wiBoolean matchLong  = (d.w16== longSFD.w16)? 1:0;
            wiBoolean matchShort = (d.w16==shortSFD.w16)? 1:0;


            SFDDone->D.word  =(matchShort|matchLong)? 1:0;
            Preamble->D.word = matchShort;

            bRX.da = 0; // reset descrambler output array so it starts with the
            bRX.db = 0; // ...PLCP header bits
        }
        if (DesOutValid) // shift in a new bit
        {
            d.w16 = (d.w15 << 1) | DesOut.b0;
            if (NSym.w5 < 16) NSym.w5++;
        }
    }
}
// end of bMojave_RX_SFD()

// ================================================================================================
// FUNCTION  : bMojave_RX_Header()
// ------------------------------------------------------------------------------------------------
// Purpose   : Decode the packet header
// Parameters: a             -- input, bit stream
//             shortPreamble -- indicates preamble {0=long, 1=short} if SFDDone=1
//             DataRate      -- data rate (100 kbps)
//             Modulation    -- modulation type: {0=CCK, 1=PBCC-not supported by Mojave}
//             Length        -- packet length (Bytes)
//             LengthUSEC    -- packet duration (microseconds)
//             CRCpass       -- CRC {0=fail, 1=pass}
//             HDRInit       -- initialize SFD search
//             HDRDone       -- flag end of header
// ================================================================================================
void bMojave_RX_Header(wiBoolean HDREnB, wiBoolean DBPSK, wiBoolean shortPreamble, wiBoolean SSOutValid, 
                       wiUWORD *a, wiBitDelayLine *HDRDone, bMojave_RX_State *RX)
{
    static int Nbit; // data bit counter

    if (HDREnB)
    {
        Nbit = 0;
        HDRDone->word = (HDRDone->word << 1) | 0;
    }
    else
    {
        if (SSOutValid) Nbit += DBPSK? 1:2; // increment data bit counter

        // --------------------------
        // Extract Header Information
        // --------------------------
        if (Nbit==48)
        {
            int i,c,b;
            wiUWORD X = {0};          // PLCP Header without CRC (bit order reversed)
            wiUWORD M = {0xFFFF}, pM; // register for CRC calculation

            for (i=31; i>=0; i--) X.word = (X.word<<1) | a[i].b0;

            // -----------------------
            // Cyclic Redundancy Check
            // -----------------------
            for (i=0; i<48; i++)
            {
                b = a[i].b0 ^ (i<32? 0:1);
                c = b ^ M.b15;
                pM.word = M.w15 << 1;
                pM.b12  = c ^ M.b11;
                pM.b5   = c ^ M.b4;
                pM.b0   = c;
                M = pM;
            }
            RX->CRCpass = (M.w16 || X.b31)? 0:1;

            if (RX->CRCpass)
            {
                RX->bDataRate.word   = X.w8;      // data rate (100kbps units)
                RX->bModulation.word = X.b11;     // modulation select
                RX->bSERVICE.word    = X.w16>>8;  // SERVICE field
                RX->bLENGTH.word     = X.w32>>16; // PSDU duration (micorseconds)

                switch (bRX.bDataRate.w8) // 4 bit RATE encoding
                {
                    case  10: bRX.bRATE.word = shortPreamble? 0x0:0x0; break;
                    case  20: bRX.bRATE.word = shortPreamble? 0x5:0x1; break;
                    case  55: bRX.bRATE.word = shortPreamble? 0x6:0x2; break;
                    case 110: bRX.bRATE.word = shortPreamble? 0x7:0x3; break;
                    default:  bRX.bRATE.word = 0xF;
                }
                switch (RX->bDataRate.w8) // compute LENGTH (bytes)
                {
                    case  10: RX->LengthPSDU.word = RX->bLENGTH.w16/8; break;
                    case  20: RX->LengthPSDU.word = RX->bLENGTH.w16/4; break;
                    case  55: RX->LengthPSDU.word = RX->bLENGTH.w16*11/16; break;
                    case 110: RX->LengthPSDU.word = RX->bLENGTH.w16*11/8 - RX->a[15].b0; break;
                }
                RX->bPacketDuration.word = max(0, RX->bLENGTH.w16-3+1); // [ROSS] adjust for decoding delay
            }
            HDRDone->word = (HDRDone->word << 1) | 1;
        }
        else HDRDone->word = (HDRDone->word << 1) | 0;
    }
}
// end of bMojave_RX_Header()

// ================================================================================================
// FUNCTION  : bMojave_RX_PSDU()
// ------------------------------------------------------------------------------------------------
// Purpose   : Flag end of PSDU decoding
// Parameters: PSDUEnB       -- enable PSDU-to-MAC/PHY interface mapping (active low)
//             DesOutValid   -- valid data at descrambler output
//             PSDUDone      -- flag end of PSDU                  
// ================================================================================================
void bMojave_RX_PSDU(wiBoolean PSDUEnB, wiBoolean DesOutValid, wiBitDelayLine *PSDUDone, 
                            Mojave_RX_State *RX, MojaveRegisters *pReg)
{
    static wiUWORD bitCount; // number of bits received in PSDU

    if (PSDUEnB)
    {
        bitCount.word   = 0;
        PSDUDone->word = 0;
    }
    else
    {
        wiBoolean done=0;
        if (DesOutValid) 
        {
            unsigned int n;
            switch (bRX.bDataRate.w8)
            {
                case  10: bitCount.w15 += 1; break; //  1 Mbps: 1 bit/11T
                case  20: bitCount.w15 += 2; break; //  2 Mbps: 2 bit/11T
                case  55: bitCount.w15 += 4; break; //5.5 Mbps: 4 bit/8T
                case 110: bitCount.w15 += 8; break; // 11 Mbps: 8 bit/8T
            }
            n = bitCount.w15>>3; // number of bytes

            if (!bitCount.w3) // modulo-8 subcount for bytes
            {
                int k;
                RX->PHY_RX_D[n+7].word = 0;
                for (k=0; k<8; k++)
                    RX->PHY_RX_D[n+7].word = RX->PHY_RX_D[n+7].w8 | (bRX.a[48+8*(n-1)+k].b0 << k);

                RX->N_PHY_RX_WR = 8 + n; // number of bytes transfered on PHY_RX_D
            }
            done = (n==bRX.LengthPSDU.w12)? 1:0; // end of packet?
            if (done) RX->RTL_Exception.DecodedDSSS_CCK = 1; // flag completion of DSSS/CCK decode
            if (done) RX->RTL_Exception.PacketCount = min(3, RX->RTL_Exception.PacketCount+1);
        }
        PSDUDone->word = (PSDUDone->word << 1) | done;
    }
}
// end of bMojave_RX_PSDU()

// ================================================================================================
// FUNCTION  : bMojave_RX_DPLL()
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
void bMojave_RX_DPLL(wiBoolean PLLEnB, wiWORD *SSMatOutR, wiWORD *SSMatOutI, wiBitDelayLine SSCorOutValid, wiWORD SSCorOutR, wiWORD SSCorOutI, 
                            wiBitDelayLine cckWalOutValid, wiWORD *CCKMatOutR, wiWORD *CCKMatOutI, wiWORD CCKWalOutR, wiWORD CCKWalOutI, wiWORD ccReal[], wiWORD ccImag[],
                            wiWORD *cCP, wiWORD *cSP, MojaveRegisters *pReg)
{
    static wiWORD   cV; // vco for carrier
    static wiWORD   cI; // integral path memory for carrier

    static wiWORD  sV; // vco for sample timing
    static wiWORD   sI; // integral path memory for sampling

    // ---------------------------------------
    // Initialize Loop Filter and VCO Memories
    // ---------------------------------------
    if (PLLEnB)
    {
        cCP->word = cV.word = cI.word = 0;
        cSP->word = sV.word = sI.word = 0;
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

        eCP.word = pcReal.w13? (8*pcImag.w13/pcReal.w13) : 0;
        eCP.word = limit(eCP.w16, -7,7);;

        // -----------------------------------------
        // Carrier PLL : Loop Filter and Digital VCO
        // -----------------------------------------
        cP.w13 = (eCP.w4<<9)>>aC.w3;
        cI.w14 = cI.w13 + ((eCP.w4<<9)>>(2+bC.w3));
        cI.w13 = limit(cI.w14, -4095,4095);
        cF.w14 = cP.w13 + cI.w13;
        cF.w13 = limit(cF.w14, -4095,4095);
        cV.w15 = cV.w15 + cF.w13;      // This implements wrap-around MSB.

//      wiPrintf("DPLL, DSSS=%d, r=(%4d,%4d), q-(%2d,%2d), pc=(%3d,%3d), eCP=%3d, cV=%5d, cCP=%3d,\n",UseDSSS,rReal,rImag,qReal,qImag,pcReal.w13,pcImag.w13,eCP.word,cV.w15,cV.w15>>9);
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
        wiWORD  ps1Real,ps1Imag,ps2Real,ps2Imag;   // early/late correlations
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

//         wiPrintf("DPLL SSMatOutR=["); for (n=0; n<12; n++) wiPrintf("%4d",SSMatOutR[n+1].w9); wiPrintf("]\n");
//         wiPrintf("DPLL SSMatOutR=["); for (n=0; n<12; n++) wiPrintf("%4d",SSMatOutR[n].w9); wiPrintf("]\n");
            for (n=0; n<12; n++)
            {
                ps1Real.w13 += (BSe[n] * SSMatOutR[n+1].w9)/2;
                ps1Imag.w13 += (BSe[n] * SSMatOutI[n+1].w9)/2;

                ps2Real.w13 += (BSe[n] * SSMatOutR[n].w9)/2;
                ps2Imag.w13 += (BSe[n] * SSMatOutI[n].w9)/2;
            }
            ps1Real.w12 = ps1Real.w13>>1;
            ps1Imag.w12 = ps1Imag.w13>>1;
            ps2Real.w12 = ps2Real.w13>>1;
            ps2Imag.w12 = ps2Imag.w13>>1;

            mag.w13 = abs(SSCorOutR.w12) + abs(SSCorOutI.w12);
        }
        else
        {
//         wiPrintf("DPLL CCKMatOutR=["); for (n=0; n<8; n++) wiPrintf("%5d",CCKMatOutR[n  ].w12); wiPrintf("]\n");
//         wiPrintf("DPLL CCKMatOutR=["); for (n=0; n<8; n++) wiPrintf("%5d",CCKMatOutR[n+1].w12); wiPrintf("]\n");

            ps1Real.w15 = CCKMatOutR[7].w12*ccReal[7].w2/2 + CCKMatOutI[7].w12*ccImag[7].w2/2;
            ps1Imag.w15 = CCKMatOutI[7].w12*ccReal[7].w2/2 - CCKMatOutR[7].w12*ccImag[7].w2/2;
            ps2Real.w15 = CCKMatOutR[0].w12*ccReal[0].w2/2 + CCKMatOutI[0].w12*ccImag[0].w2/2;
            ps2Imag.w15 = CCKMatOutI[0].w12*ccReal[0].w2/2 - CCKMatOutR[0].w12*ccImag[0].w2/2;
    
//wiPrintf("DPLL: CCKMatOutR =["); for (n=0; n<8; n++) wiPrintf("%4d",CCKMatOutR[n].w12); wiPrintf("]\n");

            for (n=0;n<7;n++)
            {
                ps1Real.w15 += CCKMatOutR[n  ].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 + CCKMatOutI[n  ].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
                ps1Imag.w15 += CCKMatOutI[n  ].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 - CCKMatOutR[n  ].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
                ps2Real.w15 += CCKMatOutR[n+1].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 + CCKMatOutI[n+1].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
                ps2Imag.w15 += CCKMatOutI[n+1].w12*(ccReal[n].w2+ccReal[n+1].w2)/2 - CCKMatOutR[n+1].w12*(ccImag[n].w2+ccImag[n+1].w2)/2;
            }
            ps1Real.w12 = ps1Real.w15>>3;
            ps1Imag.w12 = ps1Imag.w15>>3;
            ps2Real.w12 = ps2Real.w15>>3;
            ps2Imag.w12 = ps2Imag.w15>>3;

            mag.w13 = abs(CCKWalOutR.w12)+abs(CCKWalOutI.w12);
        }

        // Combine "Early","Late","Nominal" correlator terms
        eSP.w13 =(abs(ps1Real.w12)+abs(ps1Imag.w12));
        eSP.w13-=(abs(ps2Real.w12)+abs(ps2Imag.w12));
        eSP.w19 = 64*eSP.w13;

        if (mag.w13) {
            div.w13 = 1 << PriorityEncoder(mag.w13);
            eSP.w19 = eSP.w19/div.w13;
        }
        else eSP.w19 = 0;
        eSP.w7 = limit(eSP.w19,-63,63);

        // ---------------------------
        // Loop Filter and Digital VCO
        // ---------------------------
        sP.w16 = (eSP.w7<<9)>> aS.w3;
        sJ.w14 = (eSP.w7<<9)>>(bS.w3+2);
        sI.w17 = sI.w16 + sJ.w14;
        sI.w16 = limit(sI.w17, -32767,32767);
        sF.w17 = sP.w16 + sI.w16;
        sF.w16 = limit(sF.w17,-32767,32767);
        sV.w27 = sV.w27 + sF.w16;

//wiPrintf("DPLL, DSSS=%d, |ps1|=%4d, |ps2|=%4d, mag=%4d, eSP=%3d, sJ=%3d, sI=%3d, sF=%5d, sV=%6d, cSP = %3d\n",UseDSSS,abs(ps1Real.w12)+abs(ps1Imag.w12),abs(ps2Real.w12)+abs(ps2Imag.w12),abs(CCKWalOutR.w12)+abs(CCKWalOutI.w12),eSP.w7,sJ.w14,sI.w17,sF.w16,sV.w27,cSP->word);
    /**
        wiPrintf("  RX DPLL: cCP = %3d(%3d,%4d,%4d,%6d)[%5d,%5d] %s\n",cCP->w6,eCP.w4,cI.w14,cF.w14,cV.w15,rReal.w12,rImag.w12,UseDSSS? "DSSS":"CCK");
        wiPrintf("  RX DPLL: cSP = %6d, ps1Real=%4d, k11=%d (%s)\n\n",cSP->w14, ps1Real.w12, bRX.k11, UseDSSS? "DSSS":"CCK");
    /**/
    }
    cCP->word = cV.w15 >>  9;
    cSP->word = sV.w27 >> 13;
}
// end of bMojave_RX_DPLL()

// ================================================================================================
// FUNCTION  : bMojave_RX_Descrambler()
// ------------------------------------------------------------------------------------------------
// Purpose   : RX Descrambler for the data bit stream
// Parameters: n - number of bits to process
//             b - input bit stream
//             a - output bit stream (descrambled)
// ================================================================================================
void bMojave_RX_Descrambler(wiUWORD SSOut, wiBoolean SSOutValid, wiUWORD cckOut, wiBoolean cckOutValid,
                            wiBoolean DBPSK, wiUReg *DesOut, wiUReg *DesOutValid, bMojave_RX_State *RX, wiUInt FrontState)
{
    static wiUWORD S = {0}; // scrambler shift register (7-bit)

    if (FrontState==17 || FrontState==20) S.word = 0; // clear scrambler state on FrontCtrl state 17,20 (AGC done)

    if (SSOutValid || cckOutValid)
    {
        int k,b,c,n,y; wiUWORD X,Y={0};

        X.word = SSOutValid? SSOut.w2 : cckOut.w8;
        n = SSOutValid? (DBPSK? 1:2) : (RX->bDataRate.w8==110? 8:4);

        for (k=0; k<n; k++)
        {
            b = (X.w8>>k) & 1; // input bit
            c = S.b3 ^ S.b6;   // scrambler value
            y = b^c;           // output bit
            S.w7 = (S.w6<<1) | b; // update state of shift register

            Y.word = Y.word | y << k;   // output word
            RX->a[RX->da++].word = y; // output bit in serial array
        }
        DesOut->D.word = Y.w8;         // output word to SFD/Header modules
        DesOutValid->D.word = 1;
    }
    else DesOutValid->D.word = 0;
}
// end of bMojave_RX_Descrambler()

// ================================================================================================
// FUNCTION  : bMojave_Downsample()
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
void bMojave_Downsample(wiBoolean posedgeCLK22MHz, wiBoolean posedgeCLK11MHz, wiUInt DeciOutEnB,
                                wiBoolean IntpFreeze, wiUInt SymPhase, wiWORD IntpOutR, wiWORD IntpOutI, 
                                wiWORD *Deci22OutR, wiWORD *Deci22OutI, wiWORD *DeciOutR, wiWORD *DeciOutI, wiUReg *DeciFreeze)
{
    static wiReg  d44R, d44I;
    static wiReg  d22R[2], d22I[2];
    static wiReg  d11R, d11I;
    static wiUReg s22, s11; // controller states for 22/11 MHz outputs, clocked at 44/22 MHz
    static wiUReg Deci22Freeze;
    
    //if (posedgeCLK44MHz) implied by function call
    {
        ClockRegister(d44R); ClockRegister(d44I);
        ClockRegister(s22);
    }
    if (posedgeCLK22MHz) {
        ClockRegister(d22R[0]); ClockRegister(d22I[0]);
        ClockRegister(d22R[1]); ClockRegister(d22I[1]);
        ClockRegister(Deci22Freeze);
        ClockRegister(s11);
    }
    if (posedgeCLK11MHz) {
        ClockRegister(d11R); ClockRegister(d11I);
    }
    d44R.D = IntpOutR;
    d44I.D = IntpOutI;

    d22R[0].D = s22.Q.b0? IntpOutR : d44R.Q;
    d22I[0].D = s22.Q.b0? IntpOutI : d44I.Q;
    
    d22R[1].D = d22R[0].Q;
    d22I[1].D = d22I[0].Q;

    d11R.D = s11.Q.b0? d22R[0].Q:d22R[1].Q;
    d11I.D = s11.Q.b0? d22I[0].Q:d22I[1].Q;

    if (DeciOutEnB)
    {
        s11.D.word = s22.D.word = 0;
        DeciFreeze->D.word = Deci22Freeze.D.word = 0 ;
    }
    else
    {
        switch (s22.Q.w2)
        {
            case 0: Deci22Freeze.D.word = 0; if (IntpFreeze) {s22.D.word = 1;} break;
            case 1: Deci22Freeze.D.word = 0; if (IntpFreeze) {Deci22Freeze.D.word = 1; s22.D.word = 3;} break;
            case 2: break; // unused
            case 3: Deci22Freeze.D.word = 1; s22.D.word = 0; break;
        }
        switch (s11.Q.word)
        {
            case 0: DeciFreeze->D.word = 0; if (SymPhase || Deci22Freeze.Q.b0) s11.D.word = 1; break;
            case 1: DeciFreeze->D.word = 0; if (Deci22Freeze.Q.b0) {DeciFreeze->D.word = 1; s11.D.word = 3;} break;
            case 2: DeciFreeze->D.word = 0; if (Deci22Freeze.Q.b0) s11.D.word = 1; break;
            case 3: DeciFreeze->D.word = 1; s11.D.word = 2; break;
        }
    }
    // ------------
    // Wire Outputs
    // ------------
    Deci22OutR->word = d22R[0].Q.w8;
    Deci22OutI->word = d22I[0].Q.w8;

    DeciOutR->word = DeciOutEnB? 0:d11R.Q.w8;
    DeciOutI->word = DeciOutEnB? 0:d11I.Q.w8;
}
// end of bMojave_Downsample()

// ================================================================================================
// FUNCTION  : bMojave_RX_top()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level DSSS/CCK receiver and front-end control
// Parameters: bRXEnB      -- enable DSSS/CCK receiver
//             qReal,qImag -- input signal (from OFDM downmix output)
//             bAGain      -- AGain from bAGC
//             bLNAGain    -- LNA gain from bAGC
//             LgSigAFE    -- large signal detect from radio
//             CCACCK      -- CCA from DSSS/CCK RX
//             RSSICCK     -- RSSI reported from energy detect
//             Run11b      --
// ================================================================================================
void bMojave_RX_top(Mojave_RX_State *pRX, bMojave_RX_State *RX, MojaveRegisters *Reg,
                    wiBoolean bRXEnB, wiWORD *qReal, wiWORD *qImag,
                    wiUWORD *bAGain, wiUWORD *bLNAGain, wiBoolean  LgSigAFE, wiUWORD *CCACCK, 
                    wiUWORD *RSSICCK, wiBoolean Run11b, 
                    wiBoolean Restart, Mojave_bSigState *bSigOut, Mojave_bSigState bSigIn)
{
    // variables for state machine
    static wiUReg state;       // state of the digital front end control
    static wiUReg count;       // counter at 22MHz clock
    static wiUReg SFDcount;    // timeout counter for SFD search
    static wiUReg SymCount;    // counter for DSSS symbol boundaries (1us period)
    static wiUReg SymCountCCK; // counter for CCK symbol boundaries (8/11 * 1us period)
    static wiUReg LengthCount; // counter for PSDU length (1us period) with state 36

    // variables for AGC
    static wiUWORD AbsEnergy;    // absolute energy used for antenna switching
    static wiUWORD AbsEnergyOld;// previous absolute energy used for antenna switching

    // variables for channel estimation
    static wiBoolean CEDone;    // indicator that channel estimation is finished

    // variables for PSDU decoding
    static wiBoolean cckInit;    // flag for per-packet initialization of CCK demodulator
    static wiBoolean IntpFreeze;

    // variables for PLL
    static wiWORD cCP;         // carrier phase error correction
    static wiWORD CCKWalOutR,    CCKWalOutI;    // correlator output
    static wiWORD CCKMatOutR[8], CCKMatOutI[8]; // CCK-TVMF outputs
    static wiWORD *SSMatOutR=0,    *SSMatOutI=0;

    // variables for symbol decoding
    static wiWORD BFReal[22],BFImag[22];   // feedback filter input

    static wiBoolean INTERPinit;      // intialization flag for interpolator
    static wiBoolean ResetState = 0;  // state reset for restart

    static wiWORD *ccReal, *ccImag;

    // memories for implementation delays
    static wiBoolean dRestart=1;

    wiBoolean pCCACS=0, pCCAED=0;
    static wiBoolean SFDCntEnB;

    // Explicit Registers
    static wiUReg LNAGain;              // radio (b0) and external (b1) LNA gain output in AGC
    static wiUReg AGain;                // radio SGA gain output in AGC
    static wiReg  DGain;                // digital gain output in AGC
    static wiUReg EDOut;                // energy detector output
    static wiUReg AbsPwr;               // absolute power (from AGC)
    static wiUReg CQPeak={0};           // peak position for CQOut
    static wiUReg CQOut;                // output from CorrelationQuality block
    static wiUReg LargeSignal;          // flag large signal AGC update
    static wiUReg GainUpdate;           // flag AGC update
    static wiReg  cSP;                  // sampling phase error correction
    static wiReg  CFO;                  // CFO estimate
    static wiUReg SSOut, SSOutValid;    // DSSS demodulator output
    static wiUReg cckWalOutValid;       // CCK correlator output valid (cycle accurate)
    static wiUReg cckOut, cckOutValid;  // CCK demodulator output (not exactly cycle accurate)
    static wiUReg DesOut, DesOutValid;  // descrambler output
    static wiReg  SSCorOutR, SSCorOutI; // DSSS rake receiver Barker correlator outputs
    static wiUReg SSCorOutValid;        // indicate valid output on SSCorOutR/I
    static wiUReg SSSliOutR, SSSliOutI; // DSSS demod quantized constellation coordinates
    static wiUReg SFDDone, Preamble;
    static wiUReg LgSigDetAFE;          // AFE LgSigDet

    static bMojave_FrontCtrlRegType inReg, outReg; // packed register I/O
    static wiUReg Phase44, DeciFreeze;

    static wiBitDelayLine DMSym;       // demodulate symbol trigger
    static wiBitDelayLine cckSym;      // demodulate symbol trigger
    static wiBitDelayLine PSDUDone;
    static wiBitDelayLine HDRDone;
    static wiBitDelayLine posedge_cckWalOutValid; // used to align DPLL with demodulator
    static wiBitDelayLine posedge_SSCorOutValid;  // used to align DPLL with demodulator
    static wiBitDelayLine dDeciFreeze;            // deciFreeze delayed on 11 MHz clock 

    static unsigned CQWinCount, EDWinCount, SSWinCount, SFDWinCount;

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
        unsigned StepDnTh = (AbsEnergyOld.w8 < 2*Reg->bThStepDown.w4)? 0: AbsEnergyOld.w8 - 2*Reg->bThStepDown.w4;
        bSigOut->StepUp   =((AbsPwr.Q.w8 > AbsEnergyOld.w8+2*Reg->bThStepUp.w4) && (AbsEnergyOld.w8<Reg->bThStepUpRefPwr.w8))? 1:0;
        bSigOut->StepDown = (AbsPwr.Q.w8 < StepDnTh)? 1:0;
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
        ClockRegister(LNAGain);     // AGC
        ClockRegister(AGain);       // AGC
        ClockRegister(DGain);       // AGC
        ClockRegister(GainUpdate);  // AGC
        ClockRegister(LargeSignal); // AGC
        ClockRegister(AbsPwr);      // AGC
        ClockRegister(cSP);         // DPLL/Resample
        ClockRegister(Phase44);     // clock phase
        ClockDelayLine(posedge_SSCorOutValid,   SSCorOutValid.D.b0 & posedgeCLK11MHz);
        ClockDelayLine(posedge_cckWalOutValid, cckWalOutValid.D.b0 & posedgeCLK11MHz);
        Phase44.D.w2 = posedgeCLK11MHz? 1:Phase44.Q.w2+1; // phase of 44 MHz clock wrt 11 MHz
    }
     if (posedgeCLK22MHz)
    {
        ClockRegister(state);
        ClockRegister(count);
        ClockRegister(EDOut);       // EnergyDetect
        ClockRegister(CQOut);       // CorrelationQuality
        ClockRegister(CQPeak);      // CorrelationQuality
        ClockRegister(CFO);         // EstimateCFO
        ClockRegister(SFDcount);    // SFD timeout
        ClockRegister(LgSigDetAFE); // AFE Large signal detector
        outReg = inReg;
    }
    if (posedgeCLK11MHz)
    {
        ClockRegister(SSOut);
        ClockRegister(SSOutValid);
        ClockRegister(SSCorOutR);
        ClockRegister(SSCorOutI);
        ClockRegister(SSCorOutValid);
        ClockRegister(SSSliOutR);
        ClockRegister(SSSliOutI);
        ClockRegister(cckOut);
        ClockRegister(cckWalOutValid);
        ClockRegister(cckOutValid);
        ClockRegister(DesOut);
        ClockRegister(DesOutValid);
        ClockRegister(SymCount);       // DSSS symbol boundary counter
        ClockRegister(SymCountCCK);    // CCK  symbol boundary counter
        ClockRegister(LengthCount);    // PSDU timer for unsupported modulation (PBCC)
        ClockRegister(Preamble);
        ClockRegister(SFDDone);
        ClockRegister(DeciFreeze);
        ClockDelayLine(dDeciFreeze, DeciFreeze.Q.b0);

        // ---------------------------------------------
        // Conditionally Clock Symbol Marker Delay Lines
        // ...stop if DeciFreeze pauses demodulation
        // ---------------------------------------------
        if (!dDeciFreeze.delay_0T ) ClockDelayLine(DMSym, outReg.SymCMF);
        ClockDelayLine(cckSym, outReg.SymCCK);
    }
    // -----------
    // Wire Output
    // -----------
    bAGain->word    = AGain.Q.w6;
    bLNAGain->word  = LNAGain.Q.w2; // b0=Radio LNA, b1=External LNA
    CCACCK->word    = outReg.CCA;

    // ------------------------------------------
    // Increment State Machine Counters (default)
    // ------------------------------------------
    if (posedgeCLK22MHz)
    {
        count.D.word = count.Q.w11 + 1;
        SFDcount.D.word = SFDCntEnB? 0:(SFDcount.Q.w13+1);
    }
    // -------------
    // Signal Detect
    // -------------
    pCCAED = (AbsPwr.Q.w8 >=  Reg->EDthreshold.w6) && !Reg->RXMode.b0; // compare only for 11b
    pCCACS = (CQOut.Q.w13 >= (Reg->CQthreshold.w2<<(Reg->CQthreshold.w6>>2)));

    // -----------------------------------------------------------------------------------
    // FRONT END CONTROL STATE MACHINE
    //   Note that the switch-case structure is overclocked to provide pseudo-concurrent
    //   signal updates from the function calls below. However, the state and counter
    //   registers are clocked at 22 MHz, so that is the execution speed of the controller
    // -----------------------------------------------------------------------------------
    if (posedgeCLK22MHz || (posedgeCLK44MHz && state.Q.w6<24))   // run at 22MHz clock
    {
        switch (state.Q.w6)
       {
            case 0:   // initial state
                bSigOut->SigDet      = 0;
                bSigOut->AGCdone     = 0;
                bSigOut->ValidRSSI   = 0;
                bSigOut->SyncFound   = 0;
                bSigOut->HeaderValid = 0;
                bSigOut->RxDone      = 0;

                bSigOut->CSCCK = 0;

                inReg.CCA = outReg.CCA && bSigIn.StepUp; // retain CCA on step-up restart; otherwise clear
                inReg.BCEnB  = 1;    // disable Barker correlator
                inReg.EDEnB  = 1;    // disable energy detect
                inReg.CQEnB  = 1;    // disable correlation quality
                inReg.AGCEnB     = 1; // initialization: AGC
                inReg.AGCInit    = 1;
                inReg.AGCbounded = 0;
                inReg.ArmLgSigDet= 0;
                AbsEnergy.w8     = 0;
                AbsEnergyOld.w8  = 0;
                
                inReg.CEEnB  = 1;     // disable channel estimation
                inReg.CEMode = 0;
                inReg.CEGet  = 0;
                CEDone = 0;
                Preamble.D.word = 0;
                
                inReg.CFOeEnB = 1;   // initialize CFO correction
                inReg.CFOcEnB = 1;
                inReg.CFOeGet = 0;
                
                inReg.SPEnB = 1;     // demodulator symbol timing
                inReg.DMEnB  = 1;    // disable DSSS demodulator
                inReg.CMFEnB = 1;    // disable CMF
                inReg.SymGo = 0;     // disable DSSS/CCK symbol counters
                inReg.SymGoCCK = 0;
                inReg.SymPhase = 0;  // initialize symbol sync
                
                inReg.SFDEnB = 1;    // disable SFD search
                SFDCntEnB = 1;
                
                inReg.PLLEnB  = 1;   // disable DPLL
                inReg.HDREnB  = 1;   // disable header decoder
                inReg.PSDUEnB = 1;   // disable PSDU decoder
                inReg.TransCCK = 0;
                cckInit = 0;
                INTERPinit = 1; // initialize interpolator
                inReg.DeciOutEnB = 1;

                // initial displacements of various memories (for simulation purposes)
                RX->da = 0; // descrambled bits
                RX->db = 0; // detected bits

                CCKWalOutR.word = 0;
                CCKWalOutI.word = 0;

                count.D.word = 0; 
                if (posedgeCLK22MHz) state.D.word = bRXEnB? 0:63; // hold control in state 0 (bRXEnB=1 and EnableTraceCCK=1)
                break;
            case 63:  // receiver startup
                inReg.AGCInit = 0;
                if (!count.Q.w11) // compute counter limits: in RTL these are combinational logic; they are placed here
                {                // to reduce simulation overhead; the registers do not change in WiSE during a packet
                    CQWinCount  = 22 * (1<<Reg->CQwindow.w2);
                    EDWinCount  = 22 * (1<<Reg->EDwindow.w2);
                    SSWinCount  = 22 *((1<<Reg->SSwindow.w3) + 1); // 22*(+1) accounts for Barker Correlator delay
                    SFDWinCount =(22 *     Reg->SFDWindow.w8)- 32;
                }
                if (count.Q.w11 >= Reg->INITdelay.w8-2 && !Phase44.Q.b1) inReg.BCEnB  = 0;
                if (count.Q.w11 >= Reg->INITdelay.w8-1 &&  Phase44.Q.b1)
                {
                    inReg.EDEnB  = Reg->RXMode.b0; // enable for 11b but not 11g mode
                    inReg.BCEnB  = 0;              // enable Barker correlator
                    inReg.BCMode = 0;              // select 22 MS/s input
                    inReg.CSEnB  = 0;              // configure CQ module for carrier sense
                    inReg.DBPSK  = 1;              // DBPSK (1 Mbps) modulation during preamble
                    count.D.word = 0;
                    state.D.word = 1;
                }
                break;
            case 1: // estimate energy and correlation quality
                if (count.Q.w11==22)                 inReg.CQEnB = 0; // enable CQ after Barker Correlator delay line is filled
                if (count.Q.w11 > (CQWinCount + 21)) inReg.CQEnB = 1; // correlation-based carrier sense complete
                if (count.Q.w11 >  EDWinCount)       inReg.EDEnB = 1; // energy based signal-detect complete
                if (count.Q.w11 > (EDWinCount+1) && (count.Q.w11 > (CQWinCount+24))) // wait for CQ/ED outputs to become valid
                    state.D.word = 2;
                break;
            case 2:   // carrier sensing
                count.D.word = 0;
                if ((pCCACS && pCCAED) || (pCCACS && Reg->CSMode.b0) || (pCCAED && Reg->CSMode.b1))
                {
                    bSigOut->CSCCK= 1; // set carrier sense flag
                    inReg.BCEnB   = 1; // disable Barker correlator
                    inReg.CSEnB   = 1; // carrier sense complete; configure CQ for symbol sync
                    state.D.word  = 4; // proceed to AGC
                }
                else if (Reg->bSwDEn.b0 && (Reg->RXMode.w2==2))
                {
                    bSigOut->AntSel = bSigIn.AntSel^1; // switch antennas
                    state.D.word = 3; // wait for antenna switch propagation
                }
                else
                {
                    inReg.EDEnB  = Reg->RXMode.b0; // enable for 11b but not 11g mode
                    state.D.word = 1;              // return to carrier sense phase
                }
                break;
            case 3:   // antenna switching
                if (count.Q.w11 == 2*Reg->bSwDDelay.w5-1)
                {
                    inReg.BCEnB  = 0; 
                    inReg.BCMode = 0;      
                    inReg.CSEnB  = 0;
                    inReg.EDEnB  = 0;
                    count.D.word = 0;
                    state.D.word = 1;
                }
                break;
            case 4:   // wait before AGC starts
                if (Reg->RXMode.w2==3) // [802.11g]
                {
                    count.D.word = 0;  // reset counter (assume ED input is invalid until Run11b)
                    if (posedgeCLK22MHz && Run11b) state.D.word = 7; // force bAGCdelay before start of AGC updates
                }
                else // [802.11b]
                {
                    inReg.AGCbounded  = Reg->bAGCBounded4.b0;
                    if (count.Q.w11==Reg->bWaitAGC.w6-1)
                    {
                        inReg.EDEnB  = 0;
                        count.D.word = 0;
                        state.D.word = 5;
                    }
                    inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                }
                bSigOut->SigDet = 1;
                break;
            case 5:   // estimate signal power
                inReg.ArmLgSigDet= 1; // arm the large signal detector
                if ((count.Q.w11 == EDWinCount+1) || LgSigDetAFE.Q.b0)
                {
                    inReg.EDEnB     = 1;
                    inReg.AGCEnB    = 0;
                    count.D.word    = 0;
                    state.D.word    = 6;
                }
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 6:   // branch according to AGC result
                inReg.AGCEnB = 1;
                inReg.AGCbounded  = 0;
                inReg.ArmLgSigDet = 0; // reset the large signal detector
                count.D.word = 0;
                if    (LargeSignal.Q.b0) state.D.word = 7;
                else if (GainUpdate.Q.b0) state.D.word = 8;
                else                     state.D.word = 9;
                break;
            case 7:   // wait until signal is settled
                if (count.Q.w11==Reg->bAGCdelay.w6-1)
                {
                    inReg.EDEnB = 0;
                    count.D.word = 0;
                    state.D.word = 5;
                }
                inReg.CCA = 1; // set CCA unconditionally on a large signal gain update
                break;
            case 8: // wait until signal is settled
                if (count.Q.w11 == Reg->bAGCdelay.w6-1)
                {
                    count.D.word = 0;
                    state.D.word = 9;
                }
                break;
            case 9: // check for antenna switch
                AbsEnergyOld.w8 = AbsEnergy.w8;
                if ((Reg->bSwDEn.b0 && Reg->RXMode.w2==2) && (AbsEnergy.w8<Reg->bSwDTh.w8))
                {
                    bSigOut->AntSel = bSigIn.AntSel^1; // switch antenna
                    count.D.word = 0;
                    state.D.word = 10;
                }
                else
                {
                    inReg.EDEnB = Reg->RXMode.b0; // re-enable power meter (ED) in 11b mode
                    state.D.word = 17;
                }
                break;
            case 10: // antenna switching
                if (count.Q.w11 == 2*Reg->bSwDDelay.w5)
                {
                    inReg.EDEnB  = 0;
                    count.D.word = 0;
                    state.D.word = 11;
                }
                break;
            case 11: // estimate signal power
                inReg.ArmLgSigDet= 1; // arm the large signal detector
                if (count.Q.w11 == EDWinCount + 1)
                {
                    inReg.EDEnB  = 1;
                    inReg.AGCEnB = 0;
                    inReg.AGCbounded = 1;
                    count.D.word = 0;
                    state.D.word = 12;
                }
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 12: // branch according to AGC result
                inReg.AGCEnB = 1;
                inReg.AGCbounded  = 0;
                inReg.ArmLgSigDet = 0; // reset the large signal detector
                count.D.word = 0;
                if    (LargeSignal.Q.b0) state.D.word = 13;
                else if (GainUpdate.Q.b0) state.D.word = 16;
                else                     state.D.word = 18;
                break;
            case 13: // wait until signal is settled
                if (count.Q.w11 == Reg->bAGCdelay.w6-1)
                {
                    inReg.EDEnB  = 0;
                    count.D.word = 0;
                    state.D.word = 14;
                }
                inReg.CCA = 1; // set CCA unconditionally on a large signal gain update
                break;
            case 14: // estimate signal power
                inReg.ArmLgSigDet= 1; // arm the large signal detector
                if (count.Q.w11 > EDWinCount+1)
                {
                    inReg.EDEnB = 1;
                    inReg.AGCEnB = 0;
                    count.D.word = 0;
                    state.D.word = 15;
                }
                break;
            case 15: // branch according to AGC result
                inReg.ArmLgSigDet = 0; // reset the large signal detector
                inReg.AGCEnB = 1;
                count.D.word = 0;
                if    (LargeSignal.Q.b0) state.D.word = 13;
                else if (GainUpdate.Q.b0) state.D.word = 16;
                else {                   state.D.word = 17;
                    AbsEnergyOld.w8 = AbsEnergy.w8;
                }
                break;
            case 16: // wait until signal is settled
                if (count.Q.w11 == Reg->bAGCdelay.w6+EDWinCount+1)
                {
                    AbsEnergyOld.w8 = AbsEnergy.w8;
                    count.D.word = 0;
                    state.D.word = 17;
                }
                break;
            case 17: // end of AGC
                inReg.EDEnB = Reg->RXMode.b0; // enable for 11b, disable for 11g
                bSigOut->AGCdone = 1;         // signal AGC (and antenna selection) complete
                RX->Nh = 0;                   // clear so CMF output is zero until CEDone
                state.D.word = 20;            // continue preamble processing
                break;
            case 18: // check for antenna switch
                count.D.word = 0;
                if (AbsEnergy.w8<AbsEnergyOld.w8)
                {
                    bSigOut->AntSel = bSigIn.AntSel^1; // switch antenna
                    state.D.word = 19;
                }
                else
                {
                    AbsEnergyOld.w8 = AbsEnergy.w8;
                    state.D.word = 17;
                }
                break;
            case 19: // antenna switching
                if (count.Q.w11 == 2*Reg->bSwDDelay.w5-1)
                {
                    count.D.word = 0;
                    state.D.word = 17;
                }
                break;
            case 20: // start symbol sync and CFO estimation
                inReg.BCEnB  = 0;
                inReg.BCMode = 0;
                count.D.word = 0;
                state.D.word = 21;
                break;
            case 21: // wait for the first valid correlation
                if (count.Q.w11 == 23) // delay of 24T to match BC with 22 MS/s input
                {
                    inReg.CQEnB  = 0;
                    inReg.CFOeEnB= 0;
                    count.D.word = 0;
                    state.D.word = 22;
                }
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 22: // wait until symbol sync and CFO estimation are finished
                if (count.Q.w11 == SSWinCount-1)
                {
                    inReg.BCEnB   = 1;
                    inReg.CQEnB   = 1;
                    inReg.CFOeGet = 1;
                }
                if (count.Q.w11 == SSWinCount+2)
                {
                    inReg.DeciOutEnB = 0;
                    if (CQPeak.Q.b0 ^ Phase44.Q.b1)
                    {
                        inReg.SymPhase = 1;
                        count.D.word = 0;
                        state.D.word = 23;
                    }
                    else
                    {
                        inReg.SymPhase = 0;
                        count.D.word = 0;
                        state.D.word = 23;
                    }
                }
                if (count.Q.w11+22 >= EDWinCount) bSigOut->ValidRSSI = Reg->RXMode.b0^1; // set in 802.11b mode
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 23: // wait for the begining of precursors
                inReg.CFOeGet = 0;
                inReg.CFOcEnB = 0;
                inReg.CFOeEnB = 1;
                if (count.Q.w11==(CQPeak.Q.w5+3)%22)
                {
                    if (Phase44.Q.w2==3)
                    {
                        inReg.BCEnB = 0;   // start correlation at first precursor
                        inReg.BCMode = 1;   // correlation for channel estimation purpose
                        count.D.word = 0;
                        state.D.word = 24;
                    }
                    else count.D.word = count.Q.word;
                }
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 24: // wait until the peak and first valid correlation
                if (count.Q.w11 == 18) {
                    inReg.DMEnB = 0;
                    inReg.SymGo = 1; // start counter for DMSym strobe (mine is delayed 22T_22 from Simon's)
                }
                if (count.Q.w11 == 20)
                {
                    inReg.SFDEnB = 0; // start SFD search at first precursor
                    inReg.SPEnB = 0;  // enable CCK FIFO
                    SFDCntEnB   = 0;  // enable SFD search timeout counter
                }
                if (count.Q.w11 == 22)
                {
                    inReg.CEEnB = 0;
                    inReg.CEMode = 0;
                    inReg.CMFEnB = 0; // will output zero until response is updated (okay to start early in WiSE)
                    state.D.word = 25;
                }
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 25:    // wait until first channel estimation is finished
                        // this is expected to be done before the SFD
                if (CEDone == 1)
                {
                    CEDone = 0;       // clear CEDone so it is not picked up entering state 26
                    inReg.BCEnB = 0;   // continue second channel estimation
                    inReg.CEMode = 1;
                    inReg.PLLEnB = 0;
                    state.D.word = 26;
                }
                inReg.CCA = outReg.CCA || pCCAED; // check for sufficient energy to set CCA (mode 5)
                break;
            case 26: // wait until SFD is found or channel estimation is ended
                if (SFDcount.Q.w13 > SFDWinCount)
                {
                    count.D.word = 0;
                    state.D.word = 0;
                }
                else if (CEDone && SFDDone.Q.b0)
                {
                    inReg.CEEnB = 1;
                    inReg.SFDEnB = 1;
                    inReg.BCEnB = 1;
                    inReg.DBPSK = Preamble.Q.b0? 0:1;
                    inReg.HDREnB = 0;
                    bSigOut->SyncFound = 1; // signal valid SFD
                    state.D.word = 31;
                }
                else if (SFDDone.Q.b0)
                {
                    inReg.SFDEnB = 1;
                    count.D.word = 0;
                    inReg.HDREnB = 0;
                    inReg.DBPSK = Preamble.Q.b0? 0:1;
                    bSigOut->SyncFound = 1; // signal valid SFD
                    state.D.word = 28; 
                }
                else if (CEDone)
                {
                    inReg.CEEnB = 1;
                    inReg.BCEnB = 1;
                    state.D.word = 27;
                }
                break;
            case 27: // wait until SFD is found
                if (SFDcount.Q.w13 > SFDWinCount)
                {
                    count.D.word = 0;
                    state.D.word = 0;
                    if (pRX->TraceKeyOp) wiPrintf("<<SFD TIMEOUT @k22=%d>>\n",k22);
                }
                else if (SFDDone.Q.b0==1)
                {
                    inReg.SFDEnB = 1;
                    inReg.HDREnB = 0;
                    inReg.DBPSK = Preamble.Q.b0? 0:1;
                    bSigOut->SyncFound = 1; // signal valid SFD
                    state.D.word = 31; 
                }
                break;
            case 28: // start HDR decoding
                SFDCntEnB = 1;
                state.D.word = 29;
                break;
            case 29: // wait until channel estimation is finished
                if (CEDone==1)
                {
                    inReg.BCEnB = 1;
                    inReg.CEEnB = 1;
                    state.D.word = 32;
                }
                else if (count.Q.w11==22*Reg->SymHDRCE.w6-1)
                {
                    inReg.CEGet = 1;
                    state.D.word = 30;
                }
                break;
            case 30: // wait until forced ending of channel estimation
                if (CEDone==1)
                {
                    inReg.BCEnB = 1;
                    inReg.CEEnB = 1;
                    inReg.CEGet = 0;
                    state.D.word = 32;
                }
                break;
            case 31: // end of preamble
                SFDCntEnB = 1;
                state.D.word = 32;
                pRX->N_PHY_RX_WR = 0; // clear counter for PHY_RX_D writes (WiSE)
                break;
            case 32: // decode header
                if (HDRDone.delay_2T == 1)
                {
                    if (RX->CRCpass) // PLCP Valid: CRC correct
                    {
                        if ((RX->bDataRate.w8==10 || RX->bDataRate.w8==20)) // DSSS (1,2 Mbps)
                        {
                            inReg.PSDUEnB = 0;
                            inReg.DBPSK   = (RX->bDataRate.w8==10)? 1:0;
                            state.D.word  = 34;
                        }
                        else if ((RX->bDataRate.w8==55 || RX->bDataRate.w8==110) && !RX->bModulation.b0) // CCK (5.5, 11 Mbps)
                        {
                            inReg.PSDUEnB = 0;
                            cckInit       = 1;
                            state.D.word  = 33;
                        }
                        else 
                        {
                            state.D.word = 36; // unsupported modulation
                        }
                        inReg.HDREnB = 1;         // disable header processing
                        bSigOut->HeaderValid = 1; // signal valid PLCP header
                        if (pRX->TraceKeyOp) wiPrintf("<CRC OK @k=%d>",k22);
                    }
                    else // PLCP decoding error: CRC failed
                    {
                        state.D.word = 0;
                        if (pRX->TraceKeyOp) wiPrintf("<CRC FAILED: RESTART@k22=%d>\n",k22);
                    }
                }
                bSigOut->CSCCK = 0; // clear carrier sense signal (prevent false detect on 2nd packet in 11g)
                count.D.word = 0; // clear counter
                break;
            case 33: // transition from DSSS to CCK demodulation
                inReg.TransCCK= 1;
                inReg.DMEnB   = 1;
                inReg.SymGo   = 0; // stop DSSS symbol counter
                inReg.CCA     = 1; // declare channel busy due to valid PLCP header
                if (count.Q.w11 ==  3) inReg.CMFEnB = 1; // delayed to flush SSMatOutX delay lines for last DPLL update
                if (count.Q.w11 == 12) inReg.SymGoCCK = 1;
                if (count.Q.w11 == (unsigned int)(11+RX->Np2)*2) // assert TransCCK for (11+Np2)*T11
                {
                    inReg.TransCCK = 0;
                    count.D.word = 0;
                    state.D.word = 34;
                }
                break;
            case 34: // decode PSDU
                if (PSDUDone.delay_2T)
                {
                    inReg.DMEnB = 1;
                    inReg.PSDUEnB = 1;
                    inReg.SymGo = 0;
                    inReg.SymGoCCK = 0;
                    bSigOut->SigDet = 0;
                    bSigOut->SyncFound = 0;
                    bSigOut->HeaderValid = 0;
                    inReg.CCA = 0;    // packet is done: clear channel until next carrier sense
                    state.D.word = 35;
                    //wiPrintf("\n ======================================= PSDU DONE ==========================================\n\n");
                    //wiPrintf("PSDU Done: SymPhase=%d, cSP =%4d %c\n",outReg.SymPhase,cSP.word,(outReg.SymPhase&&cSP.word<-32 || !outReg.SymPhase&&abs(cSP.word)<8)? '-':'<');
                }
                count.D.word   = 0; // clear general counter
                break;
            case 35: // end of packet
                bSigOut->RxDone = 1; // signal end of PSDU to top-level control
                inReg.CMFEnB = 1;
                if (count.Q.w11==11) state.D.word = 0; // return to top of control after 0.5us delay  - wait for next packet
                break;
            case 36: // wait until the end of the packet, for LenUSEC usec.
                count.D.word  = 0; // clear general counter
                inReg.CCA     = 1; // declare channel busy due to valid PLCP header
                inReg.DMEnB   = 1; // disable DSSS demodulator
                inReg.CMFEnB  = 1; // disable CMF
                inReg.PLLEnB  = 1; // disable PLL
                inReg.CFOcEnB = 1; // disable offset correction
                inReg.DeciOutEnB = 1; // disabled decimator output
                bSigOut->SigDet = 0;
                if (LengthCount.Q.w16>=bRX.bLENGTH.w16) state.D.word = 35; // end of unsupported packet
                break;
            default:
                state.D.word = 0;
        }
        // end of state machine

        RSSICCK->w8 = AbsEnergyOld.w8;
    }
    // end of 22 MHz clocked operation

    // --------------------------------------
    // State Machine Reset/Restart Conditions
    // --------------------------------------
    if (posedgeCLK22MHz)
    {
        // ---------------------
        // State Machine Restart
        // ---------------------
        ResetState = (Restart && !dRestart) ? 1 : 0;
        dRestart = Restart;

        // -------------------------------------------
        // State Machine Reset on bRXEnB
        // ...configure state and signal when inactive
        // -------------------------------------------
        if (bRXEnB)
        {
            // initialize antenna select
            bSigOut->AntSel = Reg->bSwDEn.b0? (Reg->bSwDSave.b0? bSigIn.AntSel : Reg->bSwDInit.b0) : 0;

            state.D.word    = 0; // reset the controller state
            inReg.CCA       = 0; // CCA
            RSSICCK->word   = 0; // RSSI
            bSigOut->CSCCK  = 0; // carrier sense from 11b modem
            bSigOut->RxDone = 0; // clear RxDone
            bSigOut->SigDet = 0; // clear signal detect
            if (!RX->EnableTraceCCK) return; // skip subsequent processing to speed up simulation
        }
    }
    if (ResetState) state.D.word = 0;

    if (k44>=RX->N44) { bSigOut->RxDone = 1; pRX->RTL_Exception.kLimit_FrontCtrl=1; return; } // abort if at end of input array (to avoid memory fault)

    // --------------------------------------------------------------------------------
    //
    // CALL RECEIVER MODULE FUNCTIONS
    //
    // --------------------------------------------------------------------------------
    if (posedgeCLK40MHz)
    {
        bMojave_RX_Filter(0, qReal, RX->xReal+k40); // filter #0: real
        bMojave_RX_Filter(1, qImag, RX->xImag+k40); // filter #1: imag
        bMojave_RX_DigitalGain(RX->xReal+k40, RX->xImag+k40, RX->yReal+k40, RX->yImag+k40, DGain.Q);
    }
    if (posedgeCLK40MHz || posedgeCLK44MHz)
    {
        bMojave_RX_Interpolate(RX->yReal+k40, RX->yImag+k40, RX->sReal+k44, RX->sImag+k44, 
            cSP.Q, outReg.PLLEnB, outReg.TransCCK, INTERPinit, &IntpFreeze, pRX);

        if (posedgeCLK40MHz && posedgeCLK44MHz) INTERPinit = 0;
    }
    if (!posedgeCLK44MHz) return; // skip the rest of the function except for 11/22/44 MHz clock edges

    // ----------------------------------------------------------------------
    //
    // 44 MHz Clock Domain
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK44MHz)
    {
        bMojave_AGC(outReg.AGCEnB, outReg.AGCInit, EDOut.Q, outReg.AGCbounded, LgSigDetAFE.Q.b0, &LNAGain,
                        &AGain, &DGain, &LargeSignal, &GainUpdate, &AbsEnergy, &AbsPwr, Reg);

        bMojave_Downsample(posedgeCLK22MHz, posedgeCLK11MHz, inReg.CFOcEnB, IntpFreeze, outReg.SymPhase, RX->sReal[k44], RX->sImag[k44],
                         RX->zReal+k22, RX->zImag+k22, RX->vReal+k11, RX->vImag+k11, &DeciFreeze);

        bMojave_RX_DPLL(outReg.PLLEnB, SSMatOutR, SSMatOutI, posedge_SSCorOutValid, SSCorOutR.Q, SSCorOutI.Q, posedge_cckWalOutValid, 
                      CCKMatOutR, CCKMatOutI, CCKWalOutR, CCKWalOutI, ccReal, ccImag, &cCP, &cSP.D, Reg);
    }
    // ----------------------------------------------------------------------
    //
    // 22 MHz Clock Domain
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK22MHz)
    {
        bMojave_CorrectCFO(outReg.CFOcEnB, outReg.TransCCK, dDeciFreeze, CFO.Q, cCP, RX->vReal[k11], RX->vImag[k11], 
                         RX->uReal+k11, RX->uImag+k11, posedgeCLK11MHz);

        bMojave_BarkerCorrelator(RX->zReal+k22, RX->zImag+k22, RX->uReal+k11, RX->uImag+k11,
                               RX->rReal+k22, RX->rImag+k22, outReg.BCMode, outReg.BCEnB);

        bMojave_EnergyDetect(RX->zReal+k22,   RX->zImag+k22,   outReg.EDEnB, Reg->EDwindow, &EDOut);

        bMojave_CorrelationQuality(RX->rReal[k22], RX->rImag[k22], outReg.CQEnB, outReg.CSEnB, Reg->CQwindow, Reg->SSwindow, 
                                            &CQOut, &CQPeak);

        bMojave_EstimateCFO(outReg.CFOeEnB, outReg.CFOeGet, RX->rReal[k22], RX->rImag[k22], &CFO, Reg);
    }
    // ----------------------------------------------------------------------
    //
    // 11 MHz Clock Domain
    // ...(k11>22) insures that the CMF does not look before array element 0
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK11MHz && k11>22)
    {
        wiWORD *wR, *wI;

        bMojave_ChanEst(outReg.CEEnB, outReg.CEMode, outReg.CEGet, RX->rReal[k22], RX->rImag[k22], SSOut.Q, SSOutValid.Q.b0, outReg.DBPSK, 
                      &CEDone, RX, Reg);

        bMojave_RX_CMF(outReg.CMFEnB, DMSym, dDeciFreeze, RX->uReal+k11, RX->uImag+k11, &SSMatOutR, &SSMatOutI,
                            RX->Nh, RX->hReal, RX->hImag);

        bMojave_RX_DSSS(outReg.DMEnB, RX->uReal+k11, RX->uImag+k11, SSMatOutR, SSMatOutI, &SSCorOutR, &SSCorOutI, 
                      &SSCorOutValid, &SSSliOutR, &SSSliOutI, &SSOut, &SSOutValid, outReg.DBPSK, CEDone, outReg.HDREnB, DMSym, BFReal, BFImag);

        bMojave_RX_SP(RX->uReal+k11, RX->uImag+k11, &wR, &wI, RX->Np2, outReg.SPEnB, outReg.TransCCK, dDeciFreeze);

        if (!outReg.PSDUEnB && cckSym.delay_10T)
        {
            bMojave_RX_CCK(wR-10,wI-10, &(cckOut.D), &(cckWalOutValid.D), RX->bDataRate.w8, CCKMatOutR, CCKMatOutI, 
                                &CCKWalOutR, &CCKWalOutI, &ccReal, &ccImag, RX->hReal, RX->hImag, RX->Nh, RX->Np, RX->Np2, 
                                BFReal, BFImag, SSSliOutR.Q, SSSliOutI.Q, &cckInit);
        }
        else cckWalOutValid.D.word = 0;
        cckOutValid.D = cckWalOutValid.Q; // approximate timing for cckOutValid (cckWalOutValid is cycle accurate)

        bMojave_RX_Descrambler(SSOut.Q, SSOutValid.Q.b0, cckOut.Q, cckOutValid.Q.b0, outReg.DBPSK, &DesOut, &DesOutValid, RX, state.Q.w6);

        bMojave_RX_SFD(outReg.SFDEnB, DesOut.Q, DesOutValid.Q.b0, &Preamble, &SFDDone);

        bMojave_RX_Header(outReg.HDREnB, outReg.DBPSK, Preamble.Q.b0, SSOutValid.Q.b0, RX->a, &HDRDone, RX);

        bMojave_RX_PSDU(outReg.PSDUEnB, DesOutValid.Q.b0, &PSDUDone, pRX, Reg);
    }
    /**
    if (posedgeCLK44MHz && state.Q.word>22 && state.Q.word <37)
        wiPrintf("clk11=%d, k11=%5d, DMSym=%d, cckSym=%d, SSCorOutValid=%d,%d, SSOutValid=%d, cckOutValid=%d,%d, CFOOutR=%3d, TransCCK=%d, cCP=%3d, cSP=%3d\n",posedgeCLK11MHz, RX->k11,DMSym.delay_0T,cckSym.delay_0T,SSCorOutValid.Q.b0,posedge_SSCorOutValid.delay_0T,SSOutValid.Q.b0,cckOutValid.Q.b0,posedge_cckWalOutValid.delay_0T,RX->uReal[k11].word, TransCCK,cCP.word,cSP.word);
    /**/
    // -----------------------
    // Receive Symbol Counters
    // -----------------------
    if (posedgeCLK11MHz)
    {
        inReg.SymCMF = outReg.SymGo    && (SymCount.Q.w11    ==  0)? 1:0; // setup flag for DSSS symbol at CMF output
        inReg.SymCCK = outReg.SymGoCCK && (SymCountCCK.Q.w11 ==  0)? 1:0; // setup flag for CCK symbol at CMF output
    }
    SymCount.D.word    = (outReg.SymGo    && !dDeciFreeze.delay_0T )? ((SymCount.Q.w4    +1) % 11) :10;
    SymCountCCK.D.word = (outReg.SymGoCCK && !dDeciFreeze.delay_19T)? ((SymCountCCK.Q.w4 +1) %  8) : 7;
    if (state.Q.w6==36) {
        if (!SymCount.Q.w4) LengthCount.D.word = LengthCount.Q.w16 + 1; // 1 microsecond counter for unsupported packet types
    }
    else LengthCount.D.word = 2; // offset to reduce error due to DSSS decoding delay

    // ---------------------------------------
    // Qualify/Latch AFE Large Signal Detector
    // ---------------------------------------
    LgSigDetAFE.D.word = outReg.ArmLgSigDet & (LgSigAFE || LgSigDetAFE.Q.b0) & LNAGain.Q.b0 & Reg->bLgSigAFEValid.b0;

    // ----------------------------------------------------------------------
    //
    // Trace Signals
    //
    // ----------------------------------------------------------------------
    if (posedgeCLK22MHz && RX->EnableTraceCCK)
    {
        // ----------------------
        // CCK Baseband Interface
        // ----------------------
        RX->traceCCK[k22].bRXEnB   = bRXEnB;
        RX->traceCCK[k22].Run11b   = Run11b;
        RX->traceCCK[k22].LgSigAFE = LgSigAFE;
        RX->traceCCK[k22].bRXDone  = bSigOut->RxDone;
        RX->traceCCK[k22].AntSel   = bSigOut->AntSel;
        RX->traceCCK[k22].CCACCK   = CCACCK->b0;
        RX->traceCCK[k22].CSCCK    = bSigIn.CSCCK;
        RX->traceCCK[k22].Restart  = Restart;
        RX->traceCCK[k22].RSSICCK  = RSSICCK->w8;
        RX->traceCCK[k22].LNAGain  = bLNAGain->b0;
        RX->traceCCK[k22].AGain    = bAGain->w6;
        RX->traceCCK[k22].DGain    = DGain.Q.w5;
        RX->traceCCK[k22].LNAGain2 = bLNAGain->b1;

        // --------------
        // Baseband State
        // --------------
        RX->traceState[k22].State     = state.Q.w6;
        RX->traceState[k22].Counter   = count.Q.w11;
        RX->traceState[k22].CQPeak    = CQPeak.Q.w5;

        // ---------------
        // Control Signals
        // ---------------
        RX->traceControl[k22].EDEnB      = outReg.EDEnB;
        RX->traceControl[k22].traceValid = 1;
        RX->traceControl[k22].CQEnB      = outReg.CQEnB;
        RX->traceControl[k22].CSEnB      = outReg.CSEnB;
        RX->traceControl[k22].LargeSignal= LargeSignal.Q.b0;
        RX->traceControl[k22].GainUpdate = GainUpdate.Q.b0;
        RX->traceControl[k22].CEEnB      = outReg.CEEnB;
        RX->traceControl[k22].CEDone     = CEDone;
        RX->traceControl[k22].CEMode     = outReg.CEMode;
        RX->traceControl[k22].CEGet      = outReg.CEGet;
        RX->traceControl[k22].CFOeEnB    = outReg.CFOeEnB;
        RX->traceControl[k22].CFOcEnB    = outReg.CFOcEnB;
        RX->traceControl[k22].SFDEnB     = outReg.SFDEnB;
        RX->traceControl[k22].SFDDone    = SFDDone.Q.b0;
        RX->traceControl[k22].SymPhase   = outReg.SymPhase;
        RX->traceControl[k22].Preamble   = Preamble.Q.b0;
        RX->traceControl[k22].HDREnB     = outReg.HDREnB;
        RX->traceControl[k22].HDRDone    = HDRDone.delay_0T;
        RX->traceControl[k22].PSDUEnB    = outReg.PSDUEnB;
        RX->traceControl[k22].PSDUDone   = PSDUDone.delay_1T;
        RX->traceControl[k22].TransCCK   = outReg.TransCCK;
        RX->traceControl[k22].DeciFreeze = DeciFreeze.Q.b0;

        RX->traceFrontReg[k22].word      = outReg.word;

        // ---------------------
        // DPLL and Interpolator
        // ---------------------
        RX->traceDPLL[k22].PLLEnB     = outReg.PLLEnB;
        RX->traceDPLL[k22].IntpFreeze = IntpFreeze;
        RX->traceDPLL[k22].InterpInit = INTERPinit;
        RX->traceDPLL[k22].cCP        = cCP.w6;
        RX->traceDPLL[k22].cSP        = cSP.D.w14;

        // -----------------
        // Barker Correlator
        // -----------------
        RX->traceBarker[k22].BCEnB    = outReg.BCEnB;
        RX->traceBarker[k22].BCMode   = outReg.BCMode;
        RX->traceBarker[k22].BCOutR   = RX->rReal[k22].w10;
        RX->traceBarker[k22].BCOutI   = RX->rImag[k22].w10;

        // ----------------
        // DSSS Demodulator
        // ----------------
        RX->traceDSSS[k22].DMSym      = DMSym.delay_0T;
        RX->traceDSSS[k22].SSOutValid = SSOutValid.Q.b0;
        RX->traceDSSS[k22].SSOut      = SSOut.Q.w2;
        RX->traceDSSS[k22].SSSliOutR  = SSSliOutR.Q.b0;
        RX->traceDSSS[k22].SSSliOutI  = SSSliOutI.Q.b0;
        RX->traceDSSS[k22].SSCorOutR  = SSCorOutR.Q.w12;
        RX->traceDSSS[k22].SSCorOutI  = SSCorOutI.Q.w12;
        if (k11>22)
        {
            RX->tReal[k11].word = SSMatOutR? SSMatOutR[0].word:0;
            RX->tImag[k11].word = SSMatOutI? SSMatOutI[0].word:0;
        }
        // -----------
        // Descrambler
        // -----------
        RX->traceBarker[k22].DesOutValid = DesOutValid.Q.b0;
        RX->traceBarker[k22].DesOut      = DesOut.Q.w8;

        // ------------------
        // Non-packed Signals
        // ------------------
        RX->EDOut[k22].word = EDOut.Q.w6;
        RX->CQOut[k22].word = CQOut.Q.word;
        RX->State[k22].word = state.D.w6;
        RX->CQPeak = CQPeak.Q;

        RX->k80[k22] = pRX->k80;
    }
}
// end of bMojave_RX_top()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// Post Architecture-Freeze Revisions
//
// 2006-10-09 Brickner:Modified conditions for correction filter in channel estimation per Y.Chen 10/6/06
// 2006-10-10 Brickner:Reset descrambler state during bMojave_RX_top state 17
// 2006-12-05 Brickner:Added FrontState=22 to FrontState=17 for descrambler init per Y.Chen 12/5/06
//
// Post Silicon Revisions
//
// 2007-08-08 Brickner:Modified phase counter initialization in bMojave_TX_Resample_Core
// 2007-09-26 Brickner:Handled very rate NULL pointer case in bMojave_RX_CCK
// 2008-08-11 Brickner:Added option to force the PLCP header value

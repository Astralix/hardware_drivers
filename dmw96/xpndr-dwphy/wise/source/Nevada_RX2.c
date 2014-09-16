//--< Nevada_RX2.c >-------------------------------------------------------------------------------
//=================================================================================================
//
//  Nevada - Baseband Receiver Model (OFDM RX2)
//  Copyright 2001-2003 Bermai, 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> 
#include "wiUtil.h"
#include "Nevada.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Subcarrier types by index {0 = null, 1 = data, 2 = pilot}
//
static const int subcarrier_type_a[64] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                           1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                           0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                           1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 }; // 48-63

static const int subcarrier_type_n[64] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                           1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                           0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                           1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 }; // 48-63

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg)) 
#define XSTATUS(arg) if (WICHK(arg) < 0) return WI_ERROR_RETURNED;

#define ClockRegister(arg) {(arg).Q=(arg).D;};
#define ClockDelayLine(SR,value) {(SR).word = ((SR).word << 1) | ((value)&1);};

// Mathematical Macros
//
#ifndef log2
#define log2(x) (log((double)(x))/log(2.0))
#endif

#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#endif

// ================================================================================================
// FUNCTION  : Nevada_MarkValidTraceRX2
// ------------------------------------------------------------------------------------------------
// Purpose   : Mark a valid waveform data location in the traceRX2. 
// Parameters: WaveformType -- {1: FFT output}
//             k            -- Start position
//             n            -- number of samples to mark starting from k0
// ================================================================================================
wiStatus Nevada_MarkValidTraceRX2(int WaveformType, int k, int n)
{
    Nevada_RxState_t *pRX = Nevada_RxState();

    if (pRX->EnableTrace)
    {
        int i;

        switch (WaveformType)
        {
            case 1: for (i=0; i<n; i++, k++) pRX->traceRX2[k].ValidOutputFFT = 1; break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    return WI_SUCCESS;
}
// end of Nevada_MarkValidTraceRX2()

// ================================================================================================
// FUNCTION  : Nevada_DFT_pilot
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement the recieve DFT for pilot tones
// Parameters: xR -- Receive data input, real component (15 bit circuit input)
//             xI -- Receive data input, imag component (15 bit circuit input)
//             pR -- Receive data output, real component (11 bit circuit output)
//             pI -- Receive data output, imag component (11 bit circuit output)
// ================================================================================================
void Nevada_DFT_pilot(wiWORD xR[], wiWORD xI[], wiWORD *pR, wiWORD *pI)
{
    int n;
    wiWORD a, b;
    wiWORD c7, s7, c21, s21;
    wiWORD ac7, as7, bc7, bs7;
    wiWORD ac21, as21, bc21, bs21;

    wiWORD ip7R,im7R,ip7I,im7I;
    wiWORD ip21R,im21R,ip21I,im21I;

    wiWORD jp7R,jm7R,jp7I,jm7I;
    wiWORD jp21R,jm21R,jp21I,jm21I;

    wiWORD accp7R,accm7R,accp7I,accm7I;
    wiWORD accp21R,accm21R,accp21I,accm21I;

    accp7R.word = 0;
    accm7R.word = 0;
    accp7I.word = 0;
    accm7I.word = 0;
    accp21R.word = 0;
    accm21R.word = 0;
    accp21I.word = 0;
    accm21I.word = 0;

    for (n=0; n<64; n++)
    {
        a.word = xR[n].w15/1;
        b.word = xI[n].w15/1;

        a.word = a.word>2047 ? 2047 : (a.word<-2047 ? -2047 : a.word);
        b.word = b.word>2047 ? 2047 : (b.word<-2047 ? -2047 : b.word);

        c7.word  = (int)(64.0 * cos(-2.0*3.14159265359*(double)n* 7.0/64.0));
        s7.word  = (int)(64.0 * sin(-2.0*3.14159265359*(double)n* 7.0/64.0));
        c21.word = (int)(64.0 * cos(-2.0*3.14159265359*(double)n*21.0/64.0));
        s21.word = (int)(64.0 * sin(-2.0*3.14159265359*(double)n*21.0/64.0));

        ac7.word  = (a.w12 * c7.w8 )>>6;
        as7.word  = (a.w12 * s7.w8 )>>6;
        bc7.word  = (b.w12 * c7.w8 )>>6;
        bs7.word  = (b.w12 * s7.w8 )>>6;
        ac21.word = (a.w12 * c21.w8)>>6;
        as21.word = (a.w12 * s21.w8)>>6;
        bc21.word = (b.w12 * c21.w8)>>6;
        bs21.word = (b.w12 * s21.w8)>>6;

        im21R.word = ac21.w20 + bs21.w20;
        im21I.word = bc21.w20 - as21.w20;
        im7R.word  = ac7.w20 + bs7.w20;
        im7I.word  = bc7.w20 - as7.w20;
        ip7R.word  = ac7.w20 - bs7.w20;
        ip7I.word  = bc7.w20 + as7.w20;
        ip21R.word = ac21.w20 - bs21.w20;
        ip21I.word = bc21.w20 + as21.w20;

        jp7R.word  = ip7R.w21 >2047 ? 2047 : (ip7R.w21 <-2047 ? -2047 : ip7R.w21);
        jm7R.word  = im7R.w21 >2047 ? 2047 : (im7R.w21 <-2047 ? -2047 : im7R.w21);
        jm7I.word  = im7I.w21 >2047 ? 2047 : (im7I.w21 <-2047 ? -2047 : im7I.w21);
        jp7I.word  = ip7I.w21 >2047 ? 2047 : (ip7I.w21 <-2047 ? -2047 : ip7I.w21);
        jp21I.word = ip21I.w21>2047 ? 2047 : (ip21I.w21<-2047 ? -2047 : ip21I.w21);
        jm21I.word = im21I.w21>2047 ? 2047 : (im21I.w21<-2047 ? -2047 : im21I.w21);
        jm21R.word = im21R.w21>2047 ? 2047 : (im21R.w21<-2047 ? -2047 : im21R.w21);
        jp21R.word = ip21R.w21>2047 ? 2047 : (ip21R.w21<-2047 ? -2047 : ip21R.w21);

        accp7R.word  += jp7R.w12;
        accm7R.word  += jm7R.w12;
        accm7I.word  += jm7I.w12;
        accp7I.word  += jp7I.w12;
        accp21I.word += jp21I.w12;
        accm21I.word += jm21I.w12;
        accm21R.word += jm21R.w12;
        accp21R.word += jp21R.w12;
    }

    pR[0].word = accm21R.w18/16;  pR[0].word = pR[0].word>1023 ? 1023 : (pR[0].word<-1023 ? -1023 : pR[0].word);
    pI[0].word = accm21I.w18/16;  pI[0].word = pI[0].word>1023 ? 1023 : (pI[0].word<-1023 ? -1023 : pI[0].word);
    pR[1].word = accm7R.w18/16;   pR[1].word = pR[1].word>1023 ? 1023 : (pR[1].word<-1023 ? -1023 : pR[1].word);
    pI[1].word = accm7I.w18/16;   pI[1].word = pI[1].word>1023 ? 1023 : (pI[1].word<-1023 ? -1023 : pI[1].word);
    pR[2].word = accp7R.w18/16;   pR[2].word = pR[2].word>1023 ? 1023 : (pR[2].word<-1023 ? -1023 : pR[2].word);
    pI[2].word = accp7I.w18/16;   pI[2].word = pI[2].word>1023 ? 1023 : (pI[2].word<-1023 ? -1023 : pI[2].word);
    pR[3].word = accp21R.w18/16;  pR[3].word = pR[3].word>1023 ? 1023 : (pR[3].word<-1023 ? -1023 : pR[3].word);
    pI[3].word = accp21I.w18/16;  pI[3].word = pI[3].word>1023 ? 1023 : (pI[3].word<-1023 ? -1023 : pI[3].word);
}
// end of Nevada_DFT_pilot()

// ================================================================================================
// FUNCTION  : Nevada_WeightedPilotPhaseTrack
// ------------------------------------------------------------------------------------------------
// Purpose   : Correct the phase error measured by the digital PLL
// Parameters: FInit   -- memory initialization
//             PathSel -- Active digital paths, 2 bits, (circuit input)
//             v0R-v1I -- Data path inputs, path 0-1, real/imag, 11 bit (circuit input)
//             h0R-u1I -- Estimated channel, path 0-1, real/imag, 10 bit (circuit input)
//             cSP     -- Correction for sampling phase error, 13 bit, (model input)
//             cCCP    -- Correction for carrier phase error, 12 bit, (model output)
//             pReg    -- programmable registers
  
//             HoldVCO -- Set phase error to be zero and hold VCO
//             i       -- Symbol index; first symbol indexed 0  
// ================================================================================================
void Nevada_WeightedPilotPhaseTrack( int FInit, int HoldVCO, wiUWORD PathSel, 
                                     wiWORD v0R[], wiWORD v0I[], wiWORD v1R[], wiWORD v1I[],
                                     wiWORD h0R[], wiWORD h0I[], wiWORD h1R[], wiWORD h1I[],
                                     wiWORD cSP, wiWORD AdvDly, wiWORD *cCPP, Nevada_RxState_t *pRX, 
                                     NevadaRegisters_t *pReg, wiBoolean nMode, int i)
{
    const int pilot_tones[4] = {11, 25, 39, 53};

    int k,m,n;
    wiWORD P;
    wiWORD t;
    double a;
    wiWORD cR,cI;

    wiWORD x0R,x0I,x1R,x1I;   // phase corrected pilot tones
    wiWORD u0R,u0I,u1R,u1I;   // phase corrected pilot tones limited to 10bits
    wiWORD m10,m20,m30,m40;
    wiWORD m11,m21,m31,m41;
    wiWORD a10,a20,a11,a21;   // equalized pilot tones
    wiWORD yR,yI;         // combined pilot tones
    wiWORD wR,wI;
    wiWORD zR,zI;
    wiWORD tR,tI;
    wiWORD angle;
    wiWORD accC;         // estimated phase error
    wiWORD sR,sI;

    wiWORD delayline;      // sampling phase correction due to window shift
    wiUWORD aC,bC,cC;      // PLL gains
    int D;

    Nevada_WeightedPilotPhaseTrack_t *pState = &(pRX->WeightedPilotPhaseTrack);
    
    // initialize memories
    if (FInit)
    {
        pState->mcVCO.w18 = 0;
        pState->mcI.w17 = 0;        
        pState->cSP_dly.Q.word = 0;
        pState->Shift_Reg = 0x7f;  
    }

    if (HoldVCO)
    {
        pState->cSP_dly.Q.word = pState->cSP_dly.D.word = cSP.w13; 
        accC.word = 0;       
    }
    else
    {    
        pState->cSP_dly.D.word = cSP.w13;

        // decision for pilot tones
        D = ((pState->Shift_Reg>>3) & 0x01) ^ ((pState->Shift_Reg>>6) & 0x01);
        pState->Shift_Reg = ((pState->Shift_Reg<<1) & 0x7e) | D;

        D = (D==1) ? -1 : 1;

        // ----------------------------------------------
        // sampling phase correction due to window shift
        // ----------------------------------------------
             if (AdvDly.w2== 1) delayline.word = 102944*2/64; // 2*pi/64
        else if (AdvDly.w2==-1) delayline.word =-102944*2/64;
        else                    delayline.word = 0;

        // phase error estimation
        accC.word = 0;
        sR.word = sI.word = 0;

        for (m=0; m<4; m++)
        {
            k = pilot_tones[m];

            // Phase correction
            //
            t.word = (pState->cSP_dly.Q.w13+delayline.w13)*(k-32)/64 + pState->mcVCO.w18/64;
            t.word = t.word>1608 ? t.word-3217 : t.word<-1608 ? t.word+3217 : t.word;
            t.word = t.word/4;

            // Trigonometric Lookup Table
            //
            a = (double)t.w10/128.0;
            cR.word=(int)(512.0 * cos(a));
            cI.word=(int)(512.0 * sin(a));

            // Phase Correction
            //
            x0R.word = PathSel.b0? (v0R[m].w11*cR.w11 + v0I[m].w11*cI.w11)/1024 : 0;
            x0I.word = PathSel.b0? (v0I[m].w11*cR.w11 - v0R[m].w11*cI.w11)/1024 : 0;
            x1R.word = PathSel.b1? (v1R[m].w11*cR.w11 + v1I[m].w11*cI.w11)/1024 : 0;
            x1I.word = PathSel.b1? (v1I[m].w11*cR.w11 - v1R[m].w11*cI.w11)/1024 : 0;

            // Limit output to 10-bits
            // 
            u0R.word = (abs(x0R.w11)<512)? x0R.w11 : (x0R.b10? -511:511);
            u0I.word = (abs(x0I.w11)<512)? x0I.w11 : (x0I.b10? -511:511);
            u1R.word = (abs(x1R.w11)<512)? x1R.w11 : (x1R.b10? -511:511);
            u1I.word = (abs(x1I.w11)<512)? x1I.w11 : (x1I.b10? -511:511);

            // equalization
            //
            m10.word = u0R.w10 * h0R[k].w10;
            m20.word = u0R.w10 * h0I[k].w10;
            m30.word = u0I.w10 * h0R[k].w10;
            m40.word = u0I.w10 * h0I[k].w10;
            a10.word = m10.w19 + m40.w19;
            a20.word = m30.w19 - m20.w19;

            m11.word = u1R.w10 * h1R[k].w10;
            m21.word = u1R.w10 * h1I[k].w10;
            m31.word = u1I.w10 * h1R[k].w10;
            m41.word = u1I.w10 * h1I[k].w10;
            a11.word = m11.w19 + m41.w19;
            a21.word = m31.w19 - m21.w19;

            // ---------------------------------
            // Combine the two digital path data
            // ---------------------------------
            yR.word = (PathSel.b0?         a10.w20:0) + (PathSel.b1?         a11.w20:0);
            yI.word = (PathSel.b0?         a20.w20:0) + (PathSel.b1?         a21.w20:0);

            // --------------------------
            // Form the output to the PLL
            // --------------------------
            n = (PathSel.w2==3)? 1:0; // right shift by 1 for two-path reception
            wR.word = yR.w21 >> n;
            wI.word = yI.w21 >> n;

            zR.word = (wR.word==wR.w18)? wR.w18 : (wR.word<0)? -131071:131071;
            zI.word = (wI.word==wI.w18)? wI.w18 : (wI.word<0)? -131071:131071;

            // phase error detection
        
            // decision
            if ((nMode ? (i+m)%4 : m) == 3)  // HT Pilot depends on the symbol index
            {
                tR.w18 = -zR.w18*D;
                tI.w18 = -zI.w18*D;
            }
            else
            {
                tR.w18 = zR.w18*D;
                tI.w18 = zI.w18*D;
            }

            // accumulate
            sR.word += tR.w18;
            sI.word += tI.w18;
        }
    
        // --------------------------------------------------
        // significant bits: reduce to three significant bits
        // --------------------------------------------------
        n = (int)(log2(abs(sR.w20)>>2) + 1E-9); sR.w20 = (sR.w20/(1<<n))*(1<<n);
        n = (int)(log2(abs(sI.w20)>>2) + 1E-9); sI.w20 = (sI.w20/(1<<n))*(1<<n);

        // -----------------------------------------
        // calculate phase error of each sub-carrier
        // -----------------------------------------
        if (sR.w20 <= 0) 
            angle.word = 0;
        else 
        {
            angle.word = (sI.w20<<9) / sR.w20;
            angle.word = (abs(angle.word) < 512) ? angle.w10 : ((angle.word < 0) ? -511:511);
        }
        accC.word = angle.w10;
        pState->cSP_dly.Q.word = pState->cSP_dly.D.w13;
   }

    // -------------------------------------------------
    // Extract loop gain parameters from register memory
    // -------------------------------------------------
    aC.word = 1 << (pReg->aC.w3);
    bC.word = 1 << (pReg->bC.w3);
    cC.word = 1 << (pReg->cC.w3);

    // phase correction output
    if (pReg->PilotPLLEn.b0)
        cCPP->word = pState->mcVCO.w18 + accC.w10*64/cC.w5;
    else
        cCPP->word = 0;
    if (abs(cCPP->w19) > 102944)
    {
        cCPP->word = cCPP->w19> 102944 ? cCPP->w19-102944*2 
               : cCPP->w19<-102944 ? cCPP->w19+102944*2 : cCPP->w19;
    }
    cCPP->word = cCPP->w18/64;

    // -----------------------------------------------------
    // Loop filter and VCO for carrier phase locked loop
    // -----------------------------------------------------
    // loop filtering
    P.word   = (aC.w5==0)? 0 : accC.w10*64/aC.w5;
    if (bC.w8) pState->mcI.word = accC.w10*64/bC.w8 + pState->mcI.w17;
    pState->mcI.word = pState->mcI.w18>65535 ? 65535 : pState->mcI.w18<-65535 ? -65535 : pState->mcI.w18;

    // VCO
    pState->mcVCO.word = pState->mcVCO.w18 + P.w16 + pState->mcI.w17;
    if (abs(pState->mcVCO.w19) > 102944)
    {
        pState->mcVCO.word = pState->mcVCO.w19> 102944 ? pState->mcVCO.w19-102944*2 
              : pState->mcVCO.w19<-102944 ? pState->mcVCO.w19+102944*2 : pState->mcVCO.w19;
    }    
}

// ================================================================================================
// FUNCTION  : Nevada_PhaseCorrect
// ------------------------------------------------------------------------------------------------
// Purpose   : Correct the phase error measured by the digital PLL
// Parameters: PathSel -- Active digital paths, 2 bits, (circuit input)
//             Nk      -- Number of 20 MHz clock periods to operate (model input)
//             cSP     -- Correction for sampling phase error, 13 bit, (model input)
//             cCP     -- Correction for carrier phase error, 12 bit, (model input)
//             v0R-v1I -- Data path inputs, path 0-1, real/imag, 11 bit (circuit input)
//             u0R-u1I -- Data path outputs, path 0-1, real/imag, 10 bit (circuit output)
// Developed : by Younggyun Kim; translated to aNevada format by B. Brickner
// ================================================================================================
void Nevada_PhaseCorrect( wiUWORD PathSel, int Nk, wiWORD cSP, wiWORD cCP, wiWORD cCPP, Nevada_RxState_t *pRX,
                          wiWORD *v0R, wiWORD *v0I, wiWORD *v1R, wiWORD *v1I,
                          wiWORD *u0R, wiWORD *u0I, wiWORD *u1R, wiWORD *u1I)
{
    wiWORD x0R, x0I, x1R, x1I;
    wiWORD t,cR,cI;
    double a; int k;
    int k1 = pRX->RX_nMode? 3: 5;  // Distiguish HT and legacy to match with Mojave verification
    int k2 = pRX->RX_nMode? 61:59;
 
    // ---------------------------------------------------
    // TEST Feature (not in ASIC): Disable DPLL Correction
    // ---------------------------------------------------
    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL)
    { 
        cSP.word = 0;
        cCP.word = 0;
        cCPP.word= 0;
    }
    // -----------------------------------------------------
    // Operate correction for the Nk frequency domain points
    // -----------------------------------------------------
    for (k=0; k<Nk; k++)
    {
        t.word = cSP.w13*(k-32)/64 + cCP.w12 + cCPP.w12;
        t.word = t.word>1608 ? t.word-3217 : t.word<-1608 ? t.word+3217 : t.word;
        t.word = t.word/4;

        // --------------------------
        // Trigonometric Lookup Table
        // --------------------------
        a = (double)t.w10/128.0;
        cR.word=(int)(512.0 * cos(a));
        cI.word=(int)(512.0 * sin(a));

        // ------------------
        // Output Multiplexer
        // ------------------
        if ((k>k1) && (k<k2))  // revised to accommodate HT
        {
            // --------------------
            // Implement Correction
            // --------------------
            x0R.word = PathSel.b0? (v0R[k].w11*cR.w11 + v0I[k].w11*cI.w11)/1024 : 0;
            x0I.word = PathSel.b0? (v0I[k].w11*cR.w11 - v0R[k].w11*cI.w11)/1024 : 0;
            x1R.word = PathSel.b1? (v1R[k].w11*cR.w11 + v1I[k].w11*cI.w11)/1024 : 0;
            x1I.word = PathSel.b1? (v1I[k].w11*cR.w11 - v1R[k].w11*cI.w11)/1024 : 0;

            // -----------------------
            // Limit output to 10-bits
            // -----------------------
            u0R[k].word = (abs(x0R.w11)<512)? x0R.w11 : (x0R.b10? -511:511);
            u0I[k].word = (abs(x0I.w11)<512)? x0I.w11 : (x0I.b10? -511:511);
            u1R[k].word = (abs(x1R.w11)<512)? x1R.w11 : (x1R.b10? -511:511);
            u1I[k].word = (abs(x1I.w11)<512)? x1I.w11 : (x1I.b10? -511:511);
        }
        else
        {
            // ------------
            // Output zeros
            // ------------
            u0R[k].word = 0;
            u0I[k].word = 0;
            u1R[k].word = 0;
            u1I[k].word = 0;
        }
    }
}
// end of Nevada_PhaseCorrect()


// ================================================================================================
// FUNCTION  : Nevada_PLL
// ------------------------------------------------------------------------------------------------
// Purpose   : Digital phase locked loop
// Parameters: 
//           : iFlag    : Reset DPLL
//           : HoldVCO  : Set Phase Error be zero and hold VCO values
//           : nMode    : separate argument in case conflict with pRX->RX_nMode
//           : UpdateH  : Update the Channel Power from EqD
// Notes     : Update the channel estiamtion power and weight upon UpdateH signal. Only subcarriers
//             common to both L-OFDM and HT-OFDM are used for the phase error detector
// ================================================================================================
void Nevada_PLL( wiWORD PLL_ZR[], wiWORD PLL_ZI[], wiWORD PLL_H2[], wiWORD PLL_DR[], wiWORD PLL_DI[], 
                 int iFlag, int HoldVCO, int UpdateH, wiWORD *PLL_AdvDly, wiWORD *cSP, wiWORD *cCP, 
                 Nevada_RxState_t *pRX, NevadaRegisters_t *pReg )
{
    wiUWORD aC,bC; // loop gain for carrier loop
    wiUWORD aS,bS; // loop gain for sampling phase locked loop
   
    wiWORD eSP;   // estimate of sampling phase error
    wiWORD eCP;   // estimate of carrier phase error

    wiWORD zR,zI; // in- and quadrature-phase phase-equalized signal, for phase error calculation
    wiWORD tR,tI;
    wiWORD angle; // angle of individual carrier
    wiWORD accC;  // accumulated carrier phase error
    wiWORD accS;  // accumulated sampling phase error

    int wAddr;
    int n, k;

    const int *subcarrier_type = subcarrier_type_a; // Use legacy subcarriers for all cases

    Nevada_PLL_t *pState = &(pRX->PLL);
    wiWORD *w = pState->w;
    wiWORD *wc = pState->wc;
    wiWORD *ws = pState->ws;

    // --------------
    // Initialization
    // --------------
    if (iFlag == 1)
    {
        cSP->word = 0;
        cCP->word = 0;
        aC.word = 1;
        bC.word = 1;
        aS.word = 1;
        bS.word = 1;
        pState->sym_cnt.word = 0;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute and store weights for phase error estimation
    //  Updates occur during preamble L2 symbol or on UpdateH
    //  
    if (pState->sym_cnt.w4 == 0 || UpdateH)
    {
        wAddr = 0;
        for (k=0; k<64; k++)
        {
            if (subcarrier_type[k] != NEVADA_NULL_SUBCARRIER)
            {
                if (PLL_H2[k].w18)
                {
                    n = (int)(log2(PLL_H2[k].w18) + 1E-9);
                    w[wAddr].word = 128 >> max(0, 13-n);
                }
                else w[wAddr].word = 0;            
                wAddr++;                
            }
        }
    }
    if (pState->sym_cnt.w4 == 0 || UpdateH)
    {
        pState->sumwc.word = 0;
        pState->sumws.word = 0;
      
        for (k=0; k<26; k++)
        {
            wc[k].word = wc[51-k].word = (w[k].word>w[51-k].word)? w[51-k].word : w[k].word;
            ws[k].word = ws[k+26].word = (w[k].word>w[k+26].word)? w[k+26].word : w[k].word;
        }
        for (k=0; k<26; k++)
        {
            pState->sumwc.word += wc[k].word;
            pState->sumws.word += ws[k].word;
        }       
        n = (int)(log2(abs(pState->sumwc.word)>>4) + 1E-9); pState->sumwc.word = (pState->sumwc.word/(1<<n))*(1<<n);
        n = (int)(log2(abs(pState->sumws.word)>>4) + 1E-9); pState->sumws.word = (pState->sumws.word/(1<<n))*(1<<n);
    }
    // -------------------------------------------------
    // Extract loop gain parameters from register memory
    // -------------------------------------------------
    aC.word = 1 << pReg->Ca.w3;
    bC.word = 1 << pReg->Cb.w3;
    aS.word = 1 << pReg->Sa.w3;
    bS.word = 1 << pReg->Sb.w3;

    if (HoldVCO) // Hold VCO value during HT preamble;
    { 
        eCP.word = 0;
        eSP.word = 0;
    }
    else
    {
        // --------------------------------------------------------
        // PHASE DETECTOR: Decision directed phase error estimation
        // --------------------------------------------------------
        accC.word = accS.word = wAddr = 0;
             
        for (k=0; k<64; k++)
        {
            //--------------
            // bit alignment
            //--------------
            int mag = max(abs(PLL_ZR[k].w18), abs(PLL_ZI[k].w18));
            if (mag != 0)
            {
                n = (int)(log2(mag) + 1E-9);
                if (n>=10)
                {
                    zR.word = PLL_ZR[k].w18 / (1<<(n-9));
                    zI.word = PLL_ZI[k].w18 / (1<<(n-9));
                }
                else
                {
                    zR.word = PLL_ZR[k].w18 * (1<<(9-n));
                    zI.word = PLL_ZI[k].w18 * (1<<(9-n));
                }
            }
            else
            {
                zR.word = 0;
                zI.word = 0;
            }
            if (subcarrier_type[k] != NEVADA_NULL_SUBCARRIER) // use pilot and data subcarriers
            {
                /////////////////////////////////////////////////////////////////////////
                //
                //  Remove the desired phase rotation
                //
                //  The input is multiplied by the conjugate of the ideal constellation
                //  point. This effectively rotates the desired signal to the real axis.
                //  
                //      t = z * conj(PLL_D)
                //
                //  Then, t is reduced to three significant bits. This is to reduce
                //  hardware complexity.
                //
                tR.word = (zR.w11 * PLL_DR[k].w5) + (zI.w11 * PLL_DI[k].w5);
                tI.word = (zI.w11 * PLL_DR[k].w5) - (zR.w11 * PLL_DI[k].w5);

                n = (int)(log2(abs(tR.w16)>>2) + 1E-9); tR.w16 = (tR.w16/(1<<n)) * (1<<n);
                n = (int)(log2(abs(tI.w16)>>2) + 1E-9); tI.w16 = (tI.w16/(1<<n)) * (1<<n);

                /////////////////////////////////////////////////////////////////////////
                //
                //  Calculate the subcarrier phase error
                //
                //  Because the phase rotation due to the desired modulation was remved
                //  above, the residual angle is the phase error. The expectation is this
                //  is a relatively small angle, so it is approximated with
                //
                //      angle = 512 * arctan(tI/tR) ~ 512 * tI/tR
                //
                //  where the scaling is pi/2 = 512.
                //
                if (tR.w16 <= 0) 
                    angle.word = 0;
                else 
                {
                    angle.word = (tI.w16<<9) / tR.w16;
                    angle.word = abs(angle.word)<512 ? angle.w10 : (angle.word<0)? -511:511;
                }
                /////////////////////////////////////////////////////////////////////////
                //
                //  Accumulators for Carrier and Timing Phase Errors
                //
                accC.word += angle.w10 * wc[wAddr].word/8;
                accS.word += angle.w10 * ws[wAddr].word/8 * (wAddr<26 ? -1:1);
                wAddr++;
            }		
        }
		    
        //---------------------------------------------------------
        // estimation of phase errors
        //---------------------------------------------------------
        accC.word =  4 * accC.word;
        accS.word = 16 * accS.word; // 64*8/27 for Legacy 
  
        //-------------------------------------------------------------
        // significant bits
        //-------------------------------------------------------------
        n = (int)(log2(abs(accC.word)>>4) + 1E-9);  accC.word = (accC.word/(1<<n)) * (1<<n);
        n = (int)(log2(abs(accS.word)>>4) + 1E-9);  accS.word = (accS.word/(1<<n)) * (1<<n);

        eCP.word = accC.word / (pState->sumwc.word ? pState->sumwc.word:1);
        eCP.word = abs(eCP.word) < 512 ? eCP.w10 : (eCP.word < 0 ? -511:511);

        eSP.word = accS.word / (pState->sumws.word ? pState->sumws.word:1);
        eSP.word = abs(eSP.word) < 512 ? eSP.w10 : (eSP.word < 0 ? -511:511);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Loop Filter and VCO: Sample Timing Phase Tracking
    //
    {
        wiWORD     P;            // proportional path output

        // initialize memories
        if (iFlag==1)
        {
            pState->mSI.word=0; 
            pState->mSVCO.word=0;       
            pState->counter = 0;
            pState->delayline.word = 0;
        }

        // increase counter value
        if (pState->counter)
            pState->counter++;

        // loop filtering
        P.word   = (aS.w4==0) ? 0 : (eSP.w10*64/((int)aS.w5));
        if (bS.w7) pState->mSI.word = eSP.w10*64/((int)bS.w8) + pState->mSI.w16;
        pState->mSI.word = (pState->mSI.w17>32767)? 32767 : pState->mSI.w17<-32767 ? -32767 : pState->mSI.w17;

        // VCO
        if (pState->counter == 2)
        {
            pState->mSVCO.word = P.w16 + pState->mSI.w16 + pState->mSVCO.w19 + pState->delayline.w19;
            pState->delayline.word = 0;
        }
        else
            pState->mSVCO.word = P.w16 + pState->mSI.w16 + pState->mSVCO.w19;

        // enable wrapping-around
        if (pState->counter == 15) pState->counter = 0;

        // generate advance/delay indicator
        if (pState->mSVCO.w23 > 102944 && pState->counter==0)         // delay the window by one sample
        {
            PLL_AdvDly->word = -1;
            pState->delayline.word = -102944*2;
            pState->counter = 1;
        }
        else if (pState->mSVCO.w23<-102944 && pState->counter==0)      // advance the window by one sample
        {
            PLL_AdvDly->word = 1;
            pState->delayline.word = 102944*2;
            pState->counter = 1;
        }
        else
            PLL_AdvDly->word = 0;

        // phase error to correct
        cSP->word = pState->mSVCO.w19/64;		
    }  

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Loop Filter and VCO: Carrier Phase Tracking
    //
    {
        wiWORD P;             // proportional path output

        // initialize memories
        if (iFlag==1)
        {
            pState->mCI.word=0; 
            pState->mCVCO.word=0;       
        }
        // loop filtering
        P.word = (aC.w4==0) ? 0 : (eCP.w10*64/((int)aC.w5));
        if (bC.w7) pState->mCI.word = eCP.w10*64/((int)bC.w8) + pState->mCI.w17;
        pState->mCI.word = pState->mCI.w18>65535 ? 65535 : pState->mCI.w18<-65535 ? -65535 : pState->mCI.w18;

        // VCO
        pState->mCVCO.word = pState->mCVCO.w18 + P.w16 + pState->mCI.w17;
        if (abs(pState->mCVCO.w19) > 102944)
        {
            pRX->EventFlag.b7 = 1;
            pState->mCVCO.word = pState->mCVCO.w19>102944 ? pState->mCVCO.w19-102944*2 : pState->mCVCO.w19<-102944 ? pState->mCVCO.w19+102944*2 : pState->mCVCO.w19;
        }

        // phase error to correct
        cCP->word = pState->mCVCO.w18/64;		
    }
    // ----------------------------------------
    // Increment Symbol Counter to 15
    // Still remain counter though no Gearshift
    // sym_cnt 0: load weight
    // ----------------------------------------
    if (pState->sym_cnt.w4 < 15) pState->sym_cnt.word = pState->sym_cnt.w4 + 1; 
}
// end of Nevada_PLL()


// ================================================================================================
// FUNCTION  : Nevada_ChEst_Core()
// ------------------------------------------------------------------------------------------------
// Purpose   : Core functional block for the channel estimator (ChEst)
// Parameters: ChEstInR     -- Signal input, real component, 10 bit (circuit input)
//             ChEstInI     -- Signal input, imag component, 10 bit (circuit input)
//             ChEstOutNoise-- Channel noise power estimate, 4 bit (circuit output)
//             ChEstOutShift-- Noise shift for EqD, 4 bit (circuit output) Limited 
//             ChEstOutR    -- Channel response, real, 10 bit (circuit output)
//             ChEstOutI    -- Channel response, imag, 10 bit (circuit output)
//             ChEstOutP    -- Channel power, 20 bit (circuit output)
//             MM_HT_LTF    -- Indicate channel estimation using mixed-mode HT-LTF
// ================================================================================================
void Nevada_ChEst_Core( wiWORD ChEstInR[], wiWORD ChEstInI[], wiUWORD ChEstMinShift, wiUWORD *ChEstOutNoise,
                        wiUWORD *ChEstOutShift, wiWORD *ChEstOutR, wiWORD *ChEstOutI, wiWORD *ChEstOutP, 
                        wiBoolean MM_HT_LTF )
{
    const int L[64]={ 0,0,0,0,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,0,
                      1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,-1,-1,0,0,0 };
    int k;
    wiWORD  d, z;
    wiUWORD PwrAcm, Shift, p;

    if (MM_HT_LTF)
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Channel Estimate using Mixed-Mode HT-LTF
        //
        //  For mixed-mode 802.11n packets, the channel is re-estimated using the HT-LTF
        //  field following HT-SIG and HT-STF. For modes supported by the Nevada baseband
        //  only a single HT-LTF symbol is present.
        //
        for (k=0; k<64; k++) 
        {
            ChEstOutR[k].word = L[k] * ChEstInR[k].w10;
            ChEstOutI[k].word = L[k] * ChEstInI[k].w10;            

            ChEstOutP[k].word = (ChEstOutR[k].w10*ChEstOutR[k].w10) + (ChEstOutI[k].w10*ChEstOutI[k].w10);
        }
    }
    else 
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Channel Estimate using L-LTF or Greenfield HT-LTF1
        //
        //  Both the legacy packet and Greenfield HT-LTF1 field contain two identical LTF
        //  symbols. Channel estimation averages the two symbols to improve the estimate.
        //
        for (k=0; k<64; k++) 
        {
            ChEstOutR[k].word = L[k] * ((ChEstInR[k].w10 + ChEstInR[k+64].w10) >> 1);
            ChEstOutI[k].word = L[k] * ((ChEstInI[k].w10 + ChEstInI[k+64].w10) >> 1); 
 
            ChEstOutP[k].word = (ChEstOutR[k].w10*ChEstOutR[k].w10) + (ChEstOutI[k].w10*ChEstOutI[k].w10);
        }

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Noise Power Estimate
        //
        //  Noise power is computed by averaging the difference between two LTF symbols.
        //  Only subcarriers valid for both L-LTF and GF HT-LTF1 are used.
        //
        PwrAcm.word = 0;

        for (k=6; k<59; k++)    
        {
            d.word = (ChEstInR[k].w10 - ChEstInR[k+64].w10) >> 2;
            z.word = (abs(d.w9)>63)? ((d.w9<0)? -63:63):d.w7;
            p.word = z.w7 * z.w7;

            if (L[k]) PwrAcm.word = PwrAcm.w15 + p.w13;
        }
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Compute EqD Divider
        //
        //  The LLR computation in EqD uses division by noise power which is approximated
        //  as a right shift. A priority encoder computes Shitf = floor(log2(PwrAcm).
        //  The output value is contrained to have a minimum programmable value.
        //
        if     (PwrAcm.b15) Shift.word = 15;
        else if (PwrAcm.b14) Shift.word = 14;
        else if (PwrAcm.b13) Shift.word = 13;
        else if (PwrAcm.b12) Shift.word = 12;
        else if (PwrAcm.b11) Shift.word = 11;
        else if (PwrAcm.b10) Shift.word = 10;
        else if (PwrAcm.b9 ) Shift.word =  9;
        else if (PwrAcm.b8 ) Shift.word =  8;
        else if (PwrAcm.b7 ) Shift.word =  7;
        else if (PwrAcm.b6 ) Shift.word =  6;
        else if (PwrAcm.b5 ) Shift.word =  5;
        else if (PwrAcm.b4 ) Shift.word =  4;
        else if (PwrAcm.b3 ) Shift.word =  3;
        else if (PwrAcm.b2 ) Shift.word =  2;
        else if (PwrAcm.b1 ) Shift.word =  1;
        else                Shift.word =  0;

        ChEstOutShift->word = (Shift.w4 > ChEstMinShift.w4) ? Shift.w4 : ChEstMinShift.w4;
        ChEstOutNoise->word = Shift.w4;
    }
}
// end of Nevada_ChEst_Core()

// ================================================================================================
// FUNCTION  : Nevada_ChEst()
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate channel response and signal/noise power
// Parameters: ChEstIn0R     -- Signal, path 0, real component, 10 bit (circuit input)
//             ChEstIn0I     -- Signal, path 0, imag component, 10 bit (circuit input)
//             ChEstIn1R     -- Signal, path 1, real component, 10 bit (circuit input)
//             ChEstIn1I     -- Signal, path 1, imag component, 10 bit (circuit input)
//             ChEstOutNoise0-- Noise power estimate, path 0,    4 bit (circuit output)
//             ChEstOutNoise1-- Noise power estimate, path 1,    4 bit (circuit output)
//             ChEstOutShift0-- Shift for EqD, path 0,           4 bit (circuit output)
//             ChEstOutShift1-- Shift for EqD, path 1,           4 bit (circuit output)
//             ChEstOut0R    -- Channel response, path 0, real, 10 bit (circuit output)
//             ChEstOut0I    -- Channel response, path 0, imag, 10 bit (circuit output)
//             ChEstOutP0    -- Channel power,    path 0,       19 bit (circuit output) // question, why 19 bit? 
//             ChEstOut1R    -- Channel response, path 1, real, 10 bit (circuit output)
//             ChEstOut1I    -- Channel response, path 1, imag, 10 bit (circuit output)
//             ChEstOutP1    -- Channel power,    path 1,       19 bit (circuit output)
//             pReg          -- Pointer to modeled register file (model input)
//             nMode         -- Indicator of HT preamble or legacy preamble  1: HT 
// ================================================================================================
void Nevada_ChEst( wiWORD ChEstIn0R[], wiWORD ChEstIn0I[], wiWORD ChEstIn1R[], wiWORD ChEstIn1I[],
                   wiUWORD *ChEstOutNoise0, wiUWORD *ChEstOutNoise1, 
                   wiUWORD *ChEstOutShift0, wiUWORD *ChEstOutShift1, 
                   wiWORD *ChEstOut0R, wiWORD *ChEstOut0I, wiWORD *ChEstOutP0, 
                   wiWORD *ChEstOut1R, wiWORD *ChEstOut1I, wiWORD *ChEstOutP1, 
                   NevadaRegisters_t *pReg, wiBoolean MM_HT_LTF )
{
    Nevada_ChEst_Core(ChEstIn0R, ChEstIn0I, pReg->MinShift, ChEstOutNoise0, ChEstOutShift0, ChEstOut0R, ChEstOut0I, ChEstOutP0, MM_HT_LTF);
    Nevada_ChEst_Core(ChEstIn1R, ChEstIn1I, pReg->MinShift, ChEstOutNoise1, ChEstOutShift1, ChEstOut1R, ChEstOut1I, ChEstOutP1, MM_HT_LTF);
}
// end of Nevada_ChEst()


// ================================================================================================
// FUNCTION   : Nevada_AdaptChEst()   
// ------------------------------------------------------------------------------------------------
// Purpose    : Core channel estimate adaptation called from the EqD module
// Parameters : XdR, XdI             -- Delayed EqD input signal
//              DsR, DsI             -- Reconstructed (from decoder output) constellation point
//              ChEstInR, ChEstInI   -- Current channel estimate
//              ChEstOutR, CheStOutI -- Updated channel estimate (output)
//              ChEstOutP            -- Subcarrier power estimate (output)
// ================================================================================================
void Nevada_AdaptChEst( wiWORD XdR, wiWORD XdI, wiWORD DsR, wiWORD DsI, wiWORD ChEstInR, wiWORD ChEstInI, 
                        wiWORD *ChEstOutR, wiWORD *ChEstOutI, wiWORD *ChEstOutP, NevadaRegisters_t *pReg )
{
    wiWORD dHr, dHi; // Updating term
    wiWORD eR, eI; // Error signal
    wiWORD XsR, XsI;  // Reconstructed RX signal
    wiUWORD TrackGain1, TrackGain2;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute scaling terms
    //
    TrackGain1.word = 1 << pReg->ChTrackGain1.w3;
    TrackGain2.word = 1 << pReg->ChTrackGain2.w3;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Reconstruct the target receive signal
    //  This is the estimated constellation point multiplied by the channel estimate.
    //
    XsR.word = (ChEstInR.w10 * DsR.w5) - (ChEstInI.w10 * DsI.w5);
    XsI.word = (ChEstInR.w10 * DsI.w5) + (ChEstInI.w10 * DsR.w5); //w15

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute the equalization error
    //  This is the difference between the actual received value and the reconstructed 
    //  target signal. The actual value is scaled by 13 which is the baseband scaling for 
    //  BPSK signals. The error is by the programmable TrackGain1 value and limited to 8 bits.
    //
    eR.word =  (13 * XdR.w10 - XsR.w15) / (int)TrackGain1.w8;
    eI.word =  (13 * XdI.w10 - XsI.w15) / (int)TrackGain1.w8;

    if (abs(eR.w16) > 127) eR.word = (eR.word<0) ? -127:127;                        
    if (abs(eI.w16) > 127) eI.word = (eI.word<0) ? -127:127;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute the adjustment in the channel estimate
    //
    dHr.word = (eR.w8 * DsR.w5 + eI.w8 * DsI.w5) / (int)(8*TrackGain2.w8); // mu*E*D^H    
    dHi.word = (eI.w8 * DsR.w5 - eR.w8 * DsI.w5) / (int)(8*TrackGain2.w8);

    if (abs(dHr.word) > 255) dHr.word = (dHr.word<0) ? -255:255;
    if (abs(dHi.word) > 255) dHr.word = (dHi.word<0) ? -255:255;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Update the channel estimate and power estimate
    //  Limit the channel estimate to 10 bits
    //
    ChEstOutR->word = ChEstInR.w10 + dHr.w9;  
    ChEstOutI->word = ChEstInI.w10 + dHi.w9; 
    
    if (abs(ChEstOutR->word) > 511) ChEstOutR->word = (ChEstOutR->word < 0) ? -511:511;
    if (abs(ChEstOutI->word) > 511) ChEstOutI->word = (ChEstOutI->word < 0) ? -511:511;
     
    ChEstOutP->word =  (ChEstOutR->w10 * ChEstOutR->w10) + (ChEstOutI->w10 * ChEstOutI->w10);
}
// end of Nevada_AdaptChEst()

// ================================================================================================
// FUNCTION   : Nevada_EqD()   
// ------------------------------------------------------------------------------------------------
// Purpose    : Combined equalizer and soft demapper
// Parameters : Initialize    -- Initialize the EqD for Channel Tracking
//              EqDPathSel    -- Select active digital paths (2 bit input)
//              EqDRate       -- Data rate selection (4 bit input)
//              EqDShiftIn0   -- Log-2 noise estimate, path 0 (4 bit input)
//              EqDShiftIn1   -- Log-2 noise estimate, path 1 (4 bit input)
//              EqDChEst0R    -- Channel estimate, path 0, real component (10 bit input)
//              EqDChEst0I    -- Channel estimate, path 0, imag component (10 bit input)
//              EqDChEst1R    -- Channel estimate, path 1, real component (10 bit input)
//              EqDChEst1I    -- Channel estimate, path 1, imag component (10 bit input)
//              EqDChEstP0    -- Channel power estimate, path 0 (19 bit input)
//              EqDChEstP1    -- Channel power estimate, path 1 (19 bit input)
//              EqDIn0R       -- Data input, path 0, real component (10 bit input)
//              EqDIn0I       -- Data input, path 0, imag component (10 bit input)
//              EqDIn1R       -- Data input, path 1, real component (10 bit input)
//              EqDIn1I       -- Data input, path 1, imag component (10 bit input)
//              EqDOutH2      -- Channel power estimate output to PLL (18 bit output)
//              EqDOutZR      -- Channel estimate to PLL, real component (18 bit output)
//              EqDOutZI      -- Channel estimate to PLL, imag component (18 bit output)
//              EqDOutDR      -- Hard decision output to PLL, real component (5 bit output)
//              EqDOutDI      -- Hard decision output to PLL, imag component (5 bit output)
//              k0            -- Sample index offset (model input)
//              EqDOut        -- Soft decision output (serialized for model, 5 bit output)
//              nMode         -- Mode indicator: 1 for HT symbol with 56 subcarriers (model input)
//              pRX           -- pointer to receiver state
//              sym           -- index of data symbol starting from 0. Unsigned int
// ================================================================================================
wiStatus Nevada_EqD( wiBoolean Initialize, wiUWORD EqDPathSel, wiUWORD EqDRate, 
                     wiUWORD EqDShiftIn0, wiUWORD EqDShiftIn1,
                     wiWORD EqDChEst0R[], wiWORD EqDChEst0I[], wiWORD EqDChEst1R[], wiWORD EqDChEst1I[],
                     wiWORD EqDChEstP0[], wiWORD EqDChEstP1[],
                     wiWORD EqDIn0R[], wiWORD EqDIn0I[], wiWORD EqDIn1R[], wiWORD EqDIn1I[],
                     wiWORD *EqDOutH2, wiWORD *EqDOutZR, wiWORD *EqDOutZI, 
                     wiWORD *EqDOutDR, wiWORD *EqDOutDI, int k0, wiUWORD *EqDOut, 
                     wiBoolean nMode,  Nevada_RxState_t *pRX, unsigned int sym )
{
    wiWORD a1[2], a2[2];
    wiWORD m1[2], m2[2], m3[2], m4[2];
    wiWORD b[8], c[8], d[8];
    wiWORD Xr[2], Xi[2], Hr[2], Hi[2];
    int k, i, j, s, n;
    wiUWORD Rate; // combines nMode and EqDRate;
    wiUWORD AdrR, AdrI;   
    wiWORD dR, dI, pdR, pdI; wiUWORD H2, pH2; 
    wiUWORD MagAcmR, MagAcmI; // Sum of Magnitude for rotation check
         
    NevadaRegisters_t    *pReg = Nevada_Registers();
	Nevada_LookupTable_t *pLut = Nevada_LookupTable();

    unsigned int LMSDelay = 0; // Delay as a function of rate
    unsigned int  v_offset, k_offset = 0;  // information offset due to delay feedback; RX input offset;
    
    const int *subcarrier_type = nMode ? subcarrier_type_n : subcarrier_type_a;

    Rate.w5 = EqDRate.w4 | ((nMode ? 1:0)<<4);

    if (pRX->CheckRotate) 
    {
        pRX->Rotated6Mbps = 0;
        pRX->Rotated6MbpsValid = 0;  
        MagAcmR.word = MagAcmI.word = 0;
    }
   
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Select constellation dependent normalization
    //
    switch (Rate.w5)
    {
        case 0xD: case 0xF: case 0x10:                       s = 0; break; // BPSK
        case 0x5: case 0x7: case 0x11: case 0x12:            s = 1; break; // QPSK
        case 0x9: case 0xB: case 0x13: case 0x14:            s = 2; break; // 16-QAM
        case 0x1: case 0x3: case 0x15: case 0x16: case 0x17: s = 3; break; // 64-QAM
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    switch (EqDPathSel.w2)
    {
        case 1: s = s + EqDShiftIn0.w4; break;
        case 2: s = s + EqDShiftIn1.w4; break;
        case 3: s = s + ((EqDShiftIn0.w4 + EqDShiftIn1.w4) >> 1); break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Setup for adaptive channel estimation
    //
    //  LMSDelay is the number of symbols the input must be delayed to account for pipeline
    //  and processing delays to the Viterbi decoder output and reconstruction of the signal 
    //  (reference weekly meeting 2008-04-29).
    //
    //  The symbol start index is not a uniform interval if the timing PLL asserts AdvDly to
    //  move the FFT window forward/back by a symbol. A circular buffer records the starting
    //  index for the current symbol so it is known when the delayed version is referenced
    //  for the channel estimation.
    //
    if (pReg->ChTracking.b0)
    {
        if (Initialize) pRX->EqD.dv = 0;
        
        switch (Rate.w5)    
        {
            case 0x0D: case 0x10:                                  LMSDelay = 9; break;
            case 0x0F:                                             LMSDelay = 7; break;
            case 0x05: case 0x11:                                  LMSDelay = 6; break; 
            case 0x07: case 0x12:                                  LMSDelay = 5; break;
            case 0x09: case 0x0B: case 0x13: case 0x14:            LMSDelay = 4; break;   
            case 0x01: case 0x03: case 0x15: case 0x16: case 0x17: LMSDelay = 3; break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
         }
         
         k_offset = LMSDelay - 1 - (sym % LMSDelay);    
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Subcarrier Sequential Processing
    //
    //  Process each subcarrier in sequence. This processing includes generation of both soft
    //  and hard decisions and channel estimate tracking (if enabled).
    //
    for (j=k=0; k<64; k++)
    {
        if (subcarrier_type[k] != NEVADA_NULL_SUBCARRIER) // data/pilot subcarrier processing block
        {
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Compute X * conj(H)
            //
            //  The input signal X is multiplied by conj(H) where H is the estimated
            //  channel response. The real component is stored in a1 and the conjugate
            //  term in a2.
            //
            Xr[0].word = EqDIn0R[k+k0].w10;
            Xi[0].word = EqDIn0I[k+k0].w10;
            Hr[0].word = EqDChEst0R[k].w10;
            Hi[0].word = EqDChEst0I[k].w10;

            m1[0].word = Xr[0].w10 * Hr[0].w10;
            m2[0].word = Xr[0].w10 * Hi[0].w10;
            m3[0].word = Xi[0].w10 * Hr[0].w10;
            m4[0].word = Xi[0].w10 * Hi[0].w10;
            a1[0].word = m1[0].w19 + m4[0].w19;
            a2[0].word = m3[0].w19 - m2[0].w19; // X*conj(H)

            Xr[1].word = EqDIn1R[k+k0].w10;
            Xi[1].word = EqDIn1I[k+k0].w10;
            Hr[1].word = EqDChEst1R[k].w10;
            Hi[1].word = EqDChEst1I[k].w10;

            m1[1].word = Xr[1].w10 * Hr[1].w10;
            m2[1].word = Xr[1].w10 * Hi[1].w10;
            m3[1].word = Xi[1].w10 * Hr[1].w10;
            m4[1].word = Xi[1].w10 * Hi[1].w10;
            a1[1].word = m1[1].w19 + m4[1].w19;
            a2[1].word = m3[1].w19 - m2[1].w19;

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Combine the two signals
            //
            //  Information from the two paths is combined based on which are active
            //  (selected with PathSel). This combination with the X * conj(H) operation
            //  provides an MRC result for the soft demapper.
            //
            dR.word = (EqDPathSel.b0?         a1[0].w20:0) + (EqDPathSel.b1?         a1[1].w20:0);
            dI.word = (EqDPathSel.b0?         a2[0].w20:0) + (EqDPathSel.b1?         a2[1].w20:0);
            H2.word = (EqDPathSel.b0? EqDChEstP0[k].w20:0) + (EqDPathSel.b1? EqDChEstP1[k].w20:0);  

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Form the output to the PLL
            //
            //  The result above is provided to the DPLL. If both paths are enabled, a 
            //  divide-by-two is approximated with a right shift to avoid increasing the
            //  word size.
            //
            n = (EqDPathSel.w2==3)? 1:0; 
            pdR.word = dR.w21 >> n; 
            pdI.word = dI.w21 >> n;
            pH2.word = H2.w21 >> n;

            EqDOutH2[k+k0].word = (pH2.word==pH2.w18)? pH2.w18 :                       131071;
            EqDOutZR[k+k0].word = (pdR.word==pdR.w18)? pdR.w18 : (pdR.word<0)? -131071:131071;
            EqDOutZI[k+k0].word = (pdI.word==pdI.w18)? pdI.w18 : (pdI.word<0)? -131071:131071;

            if (pH2.word!=pH2.w18 || pdR.word!=pdR.w18 || pdI.word!=pdI.w18) pRX->EventFlag.b12 = 1;

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Scale and Limit
            // 
            //  The combined result is scaled by 13/2. The 13 corresponds to the internal
            //  scaling for BPSK constellation points. The divide-by-two follows from the
            //  definition for LLR computation.
            //
            //  The limiter forces the result to a reasonable word width. In general, the
            //  limit should not be reached, but limiting provides a better behavior than
            //  allowing the sign to potentially reverse if the MSB is removed from a 24b
            //  word.
            //
            //  ***** The limiter below map not produced the desired behavior for large
            //  ***** negative values. This should be fixed and the RTL implementation
            //  ***** checked for consistency.
            //
            dR.word = (dR.w21 * 13) >> 1;
            dI.word = (dI.w21 * 13) >> 1;
            
            // [Barrett-110214] The limit and assignment on H2 exceeds the w21 range (it
            // was previously sized for w24). This was modified post-tapeout. The actual
            // implementation was not checked as this limit is neither checked in DV nor
            // should it be exceeded as H2 is the sum of two 20-bit words.
            //
            if (abs(dR.w24) > 4194303) { dR.w24 = (dR.w24 < 0) ? -4194303 : 4194303; }
            if (abs(dI.w24) > 4194303) { dI.w24 = (dI.w24 < 0) ? -4194303 : 4194303; }
            if (abs(H2.w21) > 2097151) { H2.w21 =                           2097151; } 

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Compute Soft Information (LLR)
            //
            //  For the PSK/QAM constellations used, LLRs for AWGN are piece-wise lienar
            //  functions. These are computed recursively below in array "b." Note that
            //  the calculation for 256-QAM is shown but not required to be implemented.
            //  This is a deprecated mode left from the Dakota/Mojave basebands.
            //
            //  The LLR result is scaled by 2^-s (implemented as a right shift) in c[] 
            //  and limited to a 5-bit signed word on the range (-15,+15) in d[].
            //  
            b[0].word = dR.w23;                    b[4].word = dI.w23;
            b[1].word = 4*H2.w21 - abs(b[0].w23);  b[5].word = 4*H2.w21 - abs(b[4].w23);
            b[2].word = 2*H2.w21 - abs(b[1].w24);  b[6].word = 2*H2.w21 - abs(b[5].w24);
            b[3].word = 1*H2.w21 - abs(b[2].w25);  b[7].word = 1*H2.w21 - abs(b[6].w25); // 256-QAM (obsolete)

            for (i=0; i<8; i++)
            {
                c[i].word = b[i].w26 >> s;
                d[i].word = abs(c[i].w26) > 15 ? (c[i].w26 < 0 ? -15:15) : c[i].w26; 
            }

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Hard Demapper (LUT Index)
            //
            //  The DPLL uses hard demapping to drive the phase error detector (rather
            //  than using a more accurate but delayed signal reconstructued from the
            //  decoder output. The result is taken from a lookup table DemodROM indexed
            //  by the AdrR and AdrI values below.
            //
            switch (Rate.w5)
            {
                case 0x0D: case 0x0F: //////////// QPSK (includes BPSK)
                case 0x05: case 0x07:
                case 0x10: case 0x11: case 0x12:
                    AdrR.w4 = (b[0].word>=0)? 1:0;
                    AdrI.w4 = (b[4].word>=0)? 1:0;
                    break;

                case 0x09: case 0x0B: //////////// 16-QAM
                case 0x13: case 0x14:
                    AdrR.w4 = ((b[0].word>=0)? 2:0) | ((b[1].word>=0)? 1:0);
                    AdrI.w4 = ((b[4].word>=0)? 2:0) | ((b[5].word>=0)? 1:0);
                    break;

                case 0x01: case 0x03: //////////// 64-QAM
                case 0x15: case 0x16: case 0x17:
                    AdrR.w4 = ((b[0].word>=0)? 4:0) | ((b[1].word>=0)? 2:0) | ((b[2].word>=0)? 1:0);
                    AdrI.w4 = ((b[4].word>=0)? 4:0) | ((b[5].word>=0)? 2:0) | ((b[6].word>=0)? 1:0);
                    break;
            }
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Hard Demapper (Output) and BPSK Rotation Processing
            // 
            //  The DemodROM LUT combines the AdrX indices from above with the data rate
            //  or MCS stored in Rate to form an address. The combined address is used
            //  to index the DemodROM word to be output to the DPLL.
            //
            //  Special handling exists for 6 Mbps (RATE = 0xD) because the constellation
            //  is rotated 90 degrees for HT-SIG symbols.
            //
            //  If CheckRotate is asserted, the magnitude for the two quadrature terms
            //  is accumulated over subcarriers. Rotation is indicated if the imaginary
            //  term is greater than the real term.
            //
            if (subcarrier_type[k] == NEVADA_DATA_SUBCARRIER)
            {
                AdrR.word = (Rate.w5<<4) | AdrR.w4;
                AdrI.word = (Rate.w5<<4) | AdrI.w4;

                if (Rate.w5==0xD && pRX->CheckRotate) // Valid for L_SIG or HT-SIG1 duration  
                {               
                    EqDOutDR[k+k0].word = abs(pdI.w21)>abs(pdR.w21) ? 0 : pLut->DemodROM[AdrR.w9].word;
                    EqDOutDI[k+k0].word = abs(pdI.w21)>abs(pdR.w21) ? pLut->DemodROM[AdrI.w9].word : 0;
                   
                }
                else if (Rate.w5==0xD && pRX->Rotated6Mbps) // HT-SIG2: CheckRoated disabled but RX->Rotated6Mbps kept 1
                {
                    EqDOutDR[k+k0].word = 0; 
                    EqDOutDI[k+k0].word = pLut->DemodROM[AdrI.w9].word; 
                }
                else
                {
                    EqDOutDR[k+k0].word =  pLut->DemodROM[AdrR.w9].word;
                    EqDOutDI[k+k0].word = (Rate.w5==0xD || Rate.w5==0xF || Rate.w5 == 0x10) ? 0 : pLut->DemodROM[AdrI.w9].word;                                
                } 

                /////////////////////////////////////////////////////////////////////////
                //
                //  Check BPSK Rotation
                //
                //  The HT-SIG field is BPSK modulated but the constellation is rotated
                //  90 degrees. Here, the subcarrier signal magnitudes are summed for
                //  each quadrature component and compared after the last data subcarrier
                //  to determine the rotation.
                //
                //  XYU> Need to make sure CheckRotation only vaid for L-SIG and HT-SIG1
                //
                if (pRX->CheckRotate)
                {                         
                    MagAcmR.word = MagAcmR.w16 + abs(pdR.w20>>10);
                    MagAcmI.word = MagAcmI.w16 + abs(pdI.w20>>10);
                    
                    if (k == 58)  // last legacy data subcarrier
                        pRX->Rotated6Mbps = (MagAcmI.w16 > MagAcmR.w16) ? 1:0;
                }
            }
            else if (subcarrier_type[k] == NEVADA_PILOT_SUBCARRIER)
            { 
                AdrR.word =  0xD0 | ((b[0].word>=0)? 1:0);
                AdrI.word =  0xD0 | AdrI.w4;
                EqDOutDR[k+k0].word =  pLut->DemodROM[AdrR.w9].word;
                EqDOutDI[k+k0].word = 0;               
            }  
                       
            //////////////////////////////////////////////////////////////////////////////////
            //
            //  Adaptive Channel Estimation
            //
            //  LMS tracking of the channel response. The algorithm uses "ideal" data symbols.
            //  In the ASIC, the data are reconstructed from the Viterbi decoder output.
            //  Unless decoding errors occur, the ASIC and this model will match. Except for
            //  the purpose of RTL verification, differences due to decoding errors do not
            //  affect performance because the packet is bad once the first error occurs.
            //
            if (pReg->ChTracking.b0)
            { 
                if (sym >= LMSDelay)
                {
                    Nevada_TxState_t *pTX = Nevada_TxState(); // retrieve transmitted data
                    wiWORD DsR, DsI; // TX ideal signal

                    int kXd = k + pRX->EqD.k0_Delay[k_offset];
                    v_offset = LMSDelay<<6; // each symbol with 64 samples                   

                    DsR.word = pTX->vReal[pRX->EqD.dv-v_offset+k].word;
                    DsI.word = pTX->vImag[pRX->EqD.dv-v_offset+k].word;
                   
                    if (DsR.word) DsR.word = (DsR.w9/16) + (nMode ? (DsR.w9<0? -1:1) : 0);  // Must be divided by 16, not right shift
                    if (DsI.word) DsI.word = (DsI.w9/16) + (nMode ? (DsI.w9<0? -1:1) : 0);  //w5  round to the nearest 5 bit signal
                                       
                    if (EqDPathSel.b0)
                    {
                        Nevada_AdaptChEst( EqDIn0R[kXd], EqDIn0I[kXd], DsR, DsI, Hr[0], Hi[0],
                                           EqDChEst0R+k, EqDChEst0I+k, EqDChEstP0+k, pReg );
                    }
                    if (EqDPathSel.b1)
                    {                    
                        Nevada_AdaptChEst( EqDIn1R[kXd], EqDIn1I[kXd], DsR, DsI, Hr[1], Hi[1],
                                           EqDChEst1R+k, EqDChEst1I+k, EqDChEstP1+k, pReg );
                    }
                }
            }
          
            ///////////////////////////////////////////////////////////////////////////////////
            //
            //  Block low magnitude points
            //  Noise on 64-QAM constellation points near the origin produce a greater phase
            //  rotation for a given amplitude error. Removing these points improves the DPLL.
            //           
            if (subcarrier_type[k] == NEVADA_DATA_SUBCARRIER)
            {
                if (Rate.w5==0x01 || Rate.w5==0x03 || Rate.w5==0x15 || Rate.w5==0x16 || Rate.w5==0x17)
                {
                    if ((abs(EqDOutDR[k+k0].w5) <= 1) && (abs(EqDOutDI[k+k0].w5) <= 1))
                        EqDOutDR[k+k0].word = EqDOutDI[k+k0].word = 0;
                }
            }
     
            // Skip subsequence processing for pilot tones
            // 
            if (subcarrier_type[k] == NEVADA_PILOT_SUBCARRIER) continue;

            //////////////////////////////////////////////////////////////////////////////////
            //
            //  Output soft information
            //
            //  The parallel computation of quantized soft-information produces signed values
            //  for the largest constellation. The relative values are selected and converted
            //  to unsigned format via a shift by 16 which moves values on (-15...+15) to the
            //  range (1...31). Thus a value of 16 corresponds to an LLR of 0 (no information)
            //
            switch (Rate.w5)
            {            
                case 0x0F:  case 0x10:
                    EqDOut[j++].word = d[0].w5 + 16;
                    break;

                case 0x0D: case 0x05: case 0x07:   // 0xD: Treat 6 Mbps as QPSK modulated. Output both I/Q
                case 0x11: case 0x12:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    break;

                case 0x09: case 0x0B:
                case 0x13: case 0x14:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[1].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    EqDOut[j++].word = d[5].w5 + 16;
                    break;

                case 0x01: case 0x03:
                case 0x15: case 0x16: case 0x17:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[1].w5 + 16;
                    EqDOut[j++].word = d[2].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    EqDOut[j++].word = d[5].w5 + 16;
                    EqDOut[j++].word = d[6].w5 + 16;
                    break;
            }
        } // end data/pilot subcarrier processing block
        else // processing for non-data subcarrier
        {
            EqDOutZR[k+k0].word = 0;
            EqDOutZI[k+k0].word = 0;
            EqDOutH2[k+k0].word = 0;
        }
        
        if (pRX->CheckRotate && (k==60))   
           pRX->Rotated6MbpsValid = 1; // introduce some delay between Rotate6Mbps and the Valid signal.          
    }  

    if (pReg->ChTracking.b0)
    {
        pRX->EqD.dv += 64;
        pRX->EqD.k0_Delay[k_offset] = k0;           
    }
    return WI_SUCCESS;
} 
// end of Nevada_EqD()

// ================================================================================================
// FUNCTION   : Nevada_Depuncture()
// ------------------------------------------------------------------------------------------------
// Purpose    : Received soft-information depuncturer
// Parameters : N       -- Number of output pairs/data bits (model input)
//            : DepIn   -- Data input, aka {DepunIn0,DepunIn1}, 5 bits (circuit input)
//            : DepOut0 -- Data output 1/2, 5 bits (circuit output)
//            : DepOut1 -- Data output 2/2, 5 bits (circuit output)
//            : DepRate -- Data rate parameter, 4 bits (circuit input)
//            : nMode   -- indicate if it is HT mode with 56 subcarriers (circuit input)  
// ================================================================================================
void Nevada_Depuncture( int Nbits, wiUWORD DepIn[], wiUWORD DepOut0[], wiUWORD DepOut1[], 
                        wiUWORD DepRate, wiBoolean nMode )
{
    const int pA_12[] = {1};                  // Rate 1/2...Puncturer is rate 1/1
    const int pB_12[] = {1};

    const int pA_23[] = {1,1,1,1,1,1};        // Rate 2/3...Puncturer is rate 3/4
    const int pB_23[] = {1,0,1,0,1,0};

    const int pA_34[] = {1,1,0,1,1,0,1,1,0};  // Rate 3/4...Puncturer is rate 2/3
    const int pB_34[] = {1,0,1,1,0,1,1,0,1};

    const int pA_56[] = {1,1,0,1,0};		  // Rate 5/6...Puncturer is rate 5/6
    const int pB_56[] = {1,0,1,0,1};

    const int *pA, *pB; // pointers to "const int" arrays above
    int k, M;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Select the RATE/MCS depedent de-puncturing pattern. The base code rate of 1/2 can
    //  be punctured to 2/3, 3/4, or 5/6 correspondig to the arrays above. For these, a 0
    //  denoted a punctured bit. M is the length of the puncturing pattern (in bits).
    // 
    if (nMode)
    {
        switch (DepRate.w7) // in "n" mode, DepRate = MCS
        {
            case 0: pA = pA_12; pB = pB_12; M=1; break;
		    case 1: pA = pA_12; pB = pB_12; M=1; break;
		    case 2: pA = pA_34; pB = pB_34; M=9; break;
		    case 3: pA = pA_12; pB = pB_12; M=1; break;
		    case 4: pA = pA_34; pB = pB_34; M=9; break;
		    case 5: pA = pA_23; pB = pB_23; M=6; break;
		    case 6: pA = pA_34; pB = pB_34; M=9; break;
		    case 7: pA = pA_56; pB = pB_56; M=5; break;
            default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
        }
    }
    else    
    {
        switch (DepRate.w4) // 802.11a/g OFDM rates
        {
            case 0xD: case 0x5: case 0x9:           pA=pA_12; pB=pB_12; M=1; break;
            case 0xF: case 0x7: case 0xB: case 0x3: pA=pA_34; pB=pB_34; M=9; break;
            case 0x1:                               pA=pA_23; pB=pB_23; M=6; break;
            default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Depuncture according to the pattern selected above. The LLR for punctured bits is
    //  replaced with 16 which is the offset binary encoding for 0 (no information) in
    //  the format used here.
    //
    for (k=0; k<Nbits; k++)
    {
        DepOut0[k].word = pA[k%M] ? (DepIn++)->w5 : 16;
        DepOut1[k].word = pB[k%M] ? (DepIn++)->w5 : 16;
    }
}
// end of Nevada_Depuncture()

// ================================================================================================
// FUNCTION   : Nevada_VitDec()
// ------------------------------------------------------------------------------------------------
// Purpose    : Soft-input Viterbi decoder (trace-back path management)
// Parameters : Nbits   -- Number of data bits (model input)
//              VitDetLLR0 -- LLR for encoder output bit A, 5 bit (circuit input)
//              VitDetLLR1 -- LLR for encoder output bit B, 5 bit (circuit input)
//              VitDetOut  -- Decoded output data, 1 bit (circuit output)
//              pRX        -- Pointer to Nevada RX data structure
// ================================================================================================
void Nevada_VitDec(int Nbits, wiUWORD VitDetLLR0[], wiUWORD VitDetLLR1[], wiUWORD VitDetOut[], Nevada_RxState_t *pRX)
{
    static const int sA0[64]={0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0};
    static const int sA1[64]={1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1};
    static const int sB0[64]={0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0};
    static const int sB1[64]={1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1};
 
    wiUWORD Mk[64], Mk_1[64];
    wiUWORD LLR_A, LLR_B, _LLR_A, _LLR_B, b0, b1, m0, m1, t;    
    int i, k;
    int S, S0, S1, Sx;
    int dec_st, dec_bit;
    unsigned *DB_u, *DB_l; // array of decision bits (upper, lower 32 of 64 bits)
 
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialization
    //
    //  Set the metric for state 0 to 0 (best value) and bias the remainder to less
    //  favorable values (255). This is appropriate because the encoder initial state
    //  is 0. 
    //
    //  Clear the normalization flag (t).
    //
    //  Allocate memory to represent the path history RAMs. The DB arrays are 64b wide
    //  represented by and upper and lower 32b unsigned integer, DB_u and DB_l.
    //
    Mk_1[0].word = 0x000;     // initial state is state 0
    for (S=1; S<64; S++)      // for all other states,
        Mk_1[S].word = 0x0FF; // ...bias to a less favorable metric

    t.bit = 0;                // clear the normalization flag

    if (!pRX->VitDec.DB_l || (pRX->VitDec.Nbits < Nbits))
    {
        WIFREE( pRX->VitDec.DB_l );
        WIFREE( pRX->VitDec.DB_u );

        DB_l = pRX->VitDec.DB_l = (unsigned *)wiCalloc(Nbits+128, sizeof(unsigned));
        DB_u = pRX->VitDec.DB_u = (unsigned *)wiCalloc(Nbits+128, sizeof(unsigned));
        if (!pRX->VitDec.DB_l || !pRX->VitDec.DB_u) { 
            WICHK(WI_ERROR_MEMORY_ALLOCATION); 
            exit(1); 
        } 
        pRX->VitDec.Nbits = Nbits;
    }
    else
    {
        DB_l = (unsigned *)memset(pRX->VitDec.DB_l, 0, (Nbits+128) * sizeof(unsigned));
        DB_u = (unsigned *)memset(pRX->VitDec.DB_u, 0, (Nbits+128) * sizeof(unsigned));
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Viterbi Decoder
    //
    //  Processing is data-bit serial over Nbits. The number of iterations is greater
    //  to accomodate time for the path history trace-back process.
    //
    for (k=0; k<(Nbits+128); k++)
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Trellis Processing
        //  
        //  This operates for the first Nbits iterations of the for (k loop; subsequent
        //  loops are required only to accomodate the trace-back pipeline delay.
        //
        if (k < Nbits)
        {
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Precompute +/- LLRs
            //
            //  The decoder input is the demapper LLR value. Computation of path metrics
            //  below used both LLR and -LLR. The word _LLR_X is the offset binary form
            //  of -LLR_X.
            //
            LLR_A.word = VitDetLLR0[k].w5;
            LLR_B.word = VitDetLLR1[k].w5;
        
            _LLR_A.w5 = (~LLR_A.w5 + 1) | ((~LLR_A.w5) & 0x10);
            _LLR_B.w5 = (~LLR_B.w5 + 1) | ((~LLR_B.w5) & 0x10);

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Per-State Operations
            //
            //  Loop through 64 states indexed as S. In RTL these computations occur in
            //  parallel. States S0 and S1 are indices for states from the previous
            //  stage in the trellis that lead to state S.
            //
            for (S=0; S<64; S++)
            {
                S0 = (S << 1) & 0x3F;
                S1 = S0 | 0x01;

                /////////////////////////////////////////////////////////////////////////
                //
                //  Add path and branch metrics
                //  
                //  Values b0 and b1 are the LLRs produced by the code state implied by
                //  transitions from S0 and S1 to S. These are combined with the prior
                //  path metric to form a total path metric m0 and m1. The latter is
                //  stored as a 10b value. No truncation should be required, but the
                //  word is explicitly mapped as a 10b value to match RTL.
                //
                //  Note that the metrics are formed such that smaller is better.
                //
                b0.word = (sA0[S]? _LLR_A.w5:LLR_A.w5) + (sB0[S]? _LLR_B.w5:LLR_B.w5);
                b1.word = (sA1[S]? _LLR_A.w5:LLR_A.w5) + (sB1[S]? _LLR_B.w5:LLR_B.w5);

                m0.word = Mk_1[S0].w10 + b0.w6;
                m1.word = Mk_1[S1].w10 + b1.w6;

                m0.word = m0.w10;
                m1.word = m1.w10;

                /////////////////////////////////////////////////////////////////////////
                //
                //  Compare the competing metrics m0 and m1 and select the smaller of the
                //  two. Store the path metric Mk[S] and the prior state Sx which led to
                //  the chosen path into state S.
                //
                if (m0.word <= m1.word)
                {
                    Mk[S] = m0; Sx = S0;
                }
                else
                {
                    Mk[S] = m1; Sx = S1;
                }
                /////////////////////////////////////////////////////////////////////////
                //
                //  Store the path history in the DB array.
                //
                if (S < 32)
                {
                    DB_l[k] = DB_l[k] | ((m0.word <= m1.word ? 0:1)<<S); 
                }
                else
                {
                    DB_u[k] = DB_u[k] | ((m0.word <= m1.word ? 0:1)<<(S-32));
                }
            }
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Min Finder
            //
            //  Select the state with the smallest accumulated metric. The comparison 
            //  ignores the bottom two LSBs by masking the result with 0x3FC; simulations
            //  have shown this has negligible performance impact but improves timing
            //  for the circuit.
            //
            //  If multiple states have the minimum metric and arbitrary state can be
            //  chosen. As shown, the highest numbered state will be selected.
            //
            Sx = 0;
            for (S=1; S<64; S++) 
            {
                if ((Mk[S].w10 & 0x3FC) < (Mk[Sx].w10 & 0x3FC))
                {
                    Sx = S;
                }
                else if ((Mk[S].w10 & 0x3FC) == (Mk[Sx].w10 & 0x3FC))
                {
                    Sx = S;
                    pRX->EventFlag.b11 = 1;
                }
            }
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Normalize Path Metrics
            //
            //  If the MSB for all of the path metrics is set, then following the next
            //  code bit period, the MSB is cleared. This equivalent to subtracting 512
            //  from all metrics to avoid overflow.
            // 
            if (t.bit)
            {
                for (S=0; S<64; S++)
                    Mk[S].b9 = 0;
                pRX->EventFlag.b10 = 1;
            }
 
            t.bit = 1;
            for (S=0; S<64; S++)
                t.bit = t.bit & Mk[S].b9;

            // Clock Metric registers ///////////////////////////////////////////////////
            //
            for (S=0; S<64; S++) 
                Mk_1[S] = Mk[S];
        } 
        else
        {
            Sx = 0;
        }
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Decoder Output via Traceback
        // 
        //  Output occurs after 96 data bits have been processed by the decoder trellis.
        //  For SIGNAL field processing, RTL may implement an alternate but equivalent
        //  output generation to reduce decoder latency.
        //
        if (k>= 96)
        {
            if (k%32 == 0)
            {
                dec_st = Sx & 0x3F;
                for (i=0; i<96; i++)
                {
                    if (dec_st < 32)
                    {
                        dec_bit = DB_l[k-i] >> dec_st&0x1;
                    }
                    else
                    {
                        dec_bit = DB_u[k-i] >> (dec_st-32) & 0x1;
                    }
                    dec_st = (dec_st<<1 | dec_bit) & 0x3F;
                    if ((i>63) & (k-i>5))
                    {
                        VitDetOut[k-i-6].bit = dec_bit;
                    }
                }
            }
        }      
    } // end of for (k...
}
// end of Nevada_VitDec()

// ================================================================================================
// FUNCTION   : Nevada_Descramble()
// ------------------------------------------------------------------------------------------------
// Purpose    : Received data descrambler
// Parameters : n        -- Number of data bits to process (even, model input)
//            : DescrIn  -- Data input,  2 bit (circuit input)
//            : DescrOut -- Data output, 2 bit (circuit output)
// ================================================================================================
wiStatus Nevada_Descramble(int n, wiUWORD DescrIn[], wiUWORD DescrOut[])
{
    wiWORD x0, x1, X1, X2; int k;
    wiUWORD *b = DescrIn;
    wiUWORD *a = DescrOut;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute Scrambler Seed
    //
    //  The initial scrambler state is not known to the reciever. However, the first
    //  seven scrambled bits (SERVICE field) are required to be zero. The seed is
    //  computed based on this information.
    //
    {
        unsigned int x[7];
    
        x[0] = b[6].bit ^ b[2].bit;
        x[1] = b[5].bit ^ b[1].bit;
        x[2] = b[4].bit ^ b[0].bit;
        x[3] = b[3].bit ^ x[0];
        x[4] = b[2].bit ^ x[1];
        x[5] = b[1].bit ^ x[2];
        x[6] = b[0].bit ^ x[3];

        X1.word = x[0] | (x[2]<<1) | (x[4]<<2) | (x[6]<<3);
        X2.word = x[1] | (x[3]<<1) | (x[5]<<2);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Descramble
    //
    //  The message is descrambled by applying an identical scrambling operating to the
    //  already scrambled messaged. The scrambler uses two shift registers A and B clocked
    //  at half the bit rate with 2-bit input/output.
    //
    for (k=0; k<n; k+=2)
    {
        x0.bit = X2.b1 ^ X1.b3;
        x1.bit = X1.b1 ^ X2.b2;
        a[k+0].bit = b[k+0].bit ^ x0.bit;
        a[k+1].bit = b[k+1].bit ^ x1.bit;
        X1.w4 = (X1.w4 << 1) | x1.bit;
        X2.w3 = (X2.w3 << 1) | x0.bit;
    }
    return WI_SUCCESS;
}
// end of Nevada_Descramble()

// ================================================================================================
// FUNCTION : Nevada_TraceChEst()
// ------------------------------------------------------------------------------------------------
// Purpose  : If trace arrays are enabled, store the time-varying channel estimate and subcarrier
//            power values
// ================================================================================================
void Nevada_TraceChEst(int k0)
{
    Nevada_RxState_t *pRX = Nevada_RxState();
    int i, j, k;

    if (pRX->EnableTrace) 
    {
        for (i=0; i<2; i++)
        {
            for (j=0; j<64; j++)
            {
                k = k0 + j;
                pRX->traceHReal[i][k] = pRX->hReal[i][j];
                pRX->traceHImag[i][k] = pRX->hImag[i][j];
                pRX->traceH2[i][k]    = pRX->h2[i][j];
            }
        }
    }
}
// end of Nevada_TraceChEst()

// ================================================================================================
// FUNCTION : Nevada_RX2_SigDemod()
// ------------------------------------------------------------------------------------------------
// Purpose  : demodulation of the L-SIGNAL duration or GF HT-SIG1
// ================================================================================================
wiStatus Nevada_RX2_SigDemod(Nevada_aSigState_t *aSigOut, Nevada_RxState_t *pRX, NevadaRegisters_t *pReg)
{    
    unsigned int dx, dv, du; 
    wiUWORD SignalRate = {0xD};    
    wiWORD pReal[2][4], pImag[2][4];
       
    aSigOut->RxFault = 0; 
    pRX->cCPP.word = 0; // initial carrier phase error is 0
    pRX->N_PHY_RX_WR = 0;
    pRX->dAdvDly.word = pRX->d2AdvDly.word = 0;
    pRX->cSP[0].word = pRX->cCP[0].word = 0;
    
    if (!pRX->xReal) return STATUS(WI_ERROR);

    pRX->RX_nMode = 0; 
    pRX->Fault = 0;
    pRX->CheckRotate = pReg->RXMode.b2 ? 1 : 0;   
    pRX->Rotated6Mbps = 0;
    pRX->Rotated6MbpsValid = 0; 
	pRX->RxRot6Mbps = 0;
	pRX->RxRot6MbpsValid = 0;
    pRX->Unsupported = 0;
    pRX->Greenfield = 0; 
    pRX->ReservedHTSig = 0;
	pRX->LegacyPacket = 0; 
	pRX->HtHeaderValid = 0;
	pRX->HtRxFault = 0; 

    // ------------
    // Clear arrays
    // ------------   
    memset(pRX->AdvDly, 0, pRX->Nx*sizeof(wiWORD));    
   
    // --------------------
    // Estimate the channel
    // --------------------
    dx = pRX->x_ofs;            // starting position of FFT
    dv = dx + NEVADA_FFT_DELAY; // 20MHz sample offset to FFT output
    du = dv + NEVADA_PLL_DELAY; // 20MHz sample offset to phase corrector output

    // ---------------------------------------------------
    // Extract the FFT of the two long preamble symbols
    // dx: FFT in/output offset. output invalid until dv
    // ---------------------------------------------------
    if (pReg->PathSel.b0)
    {
        Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx,    pRX->xImag[0]+dx,    NULL, NULL, pRX->vReal[0]+dx,    pRX->vImag[0]+dx);
        Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx+64, pRX->xImag[0]+dx+64, NULL, NULL, pRX->vReal[0]+dx+64, pRX->vImag[0]+dx+64);
    }
    if (pReg->PathSel.b1)
    {
        Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx,    pRX->xImag[1]+dx,    NULL, NULL, pRX->vReal[1]+dx,    pRX->vImag[1]+dx);
        Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx+64, pRX->xImag[1]+dx+64, NULL, NULL, pRX->vReal[1]+dx+64, pRX->vImag[1]+dx+64);
    }
    if (pReg->PathSel.w2) XSTATUS(Nevada_MarkValidTraceRX2(1, dv, 128));

    // ---------------------------------------
    // Phase Correction and Channel Estimation
    // dv:valid FFT output for long preamble 
    // ---------------------------------------
    Nevada_PhaseCorrect( pReg->PathSel, 64, pRX->cSP[0], pRX->cCP[0], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, pRX->vImag[1]+dv,
                         pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du  );

    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[0], pRX->cCP[0], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv+64, pRX->vImag[0]+dv+64, pRX->vReal[1]+dv+64, pRX->vImag[1]+dv+64,
                         pRX->uReal[0]+du+64, pRX->uImag[0]+du+64, pRX->uReal[1]+du+64, pRX->uImag[1]+du+64  );
      
    Nevada_ChEst( pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du,
                  &(pReg->ChEstShift0), &(pReg->ChEstShift1), &(pRX->NoiseShift[0]), &(pRX->NoiseShift[1]), 
                  pRX->hReal[0], pRX->hImag[0], pRX->h2[0], pRX->hReal[1], pRX->hImag[1], pRX->h2[1], pReg, 0);

    // -------------------------------------------------
    // Compute vector offset and check for array overrun
    // -------------------------------------------------
    dx+=144; dv+=144; du+=144; // 144 = 128 (preamble) + 16 (SIGNAL Guard Interval) Start of SIG 
    if (dx>pRX->Nx) 
    {
        pRX->Fault = 2;
        pRX->EventFlag.b8 = 1;
        pRX->RTL_Exception.xLimit_SIGNAL = 1;
        return WI_WARN_PHY_BAD_PACKET;
    }

    // ---------------------------------------------------
    // Record the channel estimation if trace is enabled
    // The CE arrays use offset du for EqD input
    // The first valid CE line up with EqD input.for L-SIG
    // ---------------------------------------------------
    Nevada_TraceChEst(du);
    
    pRX->kFFT[0] = dx;
    if (pReg->PathSel.b0) Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx, pRX->xImag[0]+dx, NULL, NULL, pRX->vReal[0]+dx, pRX->vImag[0]+dx);
    if (pReg->PathSel.b1) Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx, pRX->xImag[1]+dx, NULL, NULL, pRX->vReal[1]+dx, pRX->vImag[1]+dx);
    if (pReg->PathSel.w2) XSTATUS(Nevada_MarkValidTraceRX2(1, dv, 64));

    if (pReg->PathSel.b0) Nevada_DFT_pilot(pRX->xReal[0]+dx, pRX->xImag[0]+dx, pReal[0], pImag[0]);
    if (pReg->PathSel.b1) Nevada_DFT_pilot(pRX->xReal[1]+dx, pRX->xImag[1]+dx, pReal[1], pImag[1]);

    // carrier phase tracking based on pilot tones
    Nevada_WeightedPilotPhaseTrack( 1, 0, pReg->PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], 
                                    pRX->cSP[0], pRX->d2AdvDly, &(pRX->cCPP), pRX, pReg,pRX->RX_nMode,0 ); // second argin HoldVCO

    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[0], pRX->cCP[0], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, pRX->vImag[1]+dv,
                         pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du );
     
    Nevada_EqD( 1, pReg->PathSel, SignalRate, pRX->NoiseShift[0], pRX->NoiseShift[1], 
                pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], pRX->h2[0], pRX->h2[1], 
                pRX->uReal[0], pRX->uImag[0], pRX->uReal[1], pRX->uImag[1], pRX->H2, 
                pRX->dReal, pRX->dImag, pRX->DReal, pRX->DImag, du, pRX->Lp, pRX->RX_nMode, pRX, 0 ); //  FInit 1 nMode 0, pRX, sym 0,
   
    Nevada_PLL( pRX->dReal+du, pRX->dImag+du, pRX->H2+du, pRX->DReal+du, pRX->DImag+du, 1, 0, 0,
                 pRX->AdvDly+du, pRX->cSP+0, pRX->cCP+0, pRX, pReg );
    pRX->d2AdvDly = pRX->dAdvDly; pRX->dAdvDly = pRX->AdvDly[du];

    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL) pRX->dAdvDly.word = 0; // TEST Feature not in ASIC
    
    Nevada_Interleaver   (SignalRate, pRX->Lp, pRX->Lc, 1, pRX->RX_nMode, pRX->Rotated6Mbps); // TX/RX, nMode, Rotated6Mpbs. RX_nMode is 0 for this block
    
    // --------------------------------
    // Advance and Save Pointers 
    // Moved the position;
    // --------------------------------
    pRX->dx     = dx+80;
    pRX->du     = du+80;
    pRX->dv     = dv+80;   
    pRX->dp     = 96; // LLR of both I/Q for 6Mbps
    pRX->dc     = 48; // code bit (LLR) offset
    return WI_SUCCESS;   
} 
// end of Nevada_RX2_SigDemod()

// ================================================================================================
// FUNCTION : Nevada_RX2_SigDecode()
// ------------------------------------------------------------------------------------------------
// Purpose  : decoding of the L-SIGNAL symbol
// ================================================================================================
wiStatus Nevada_RX2_SigDecode(Nevada_aSigState_t *aSigOut, Nevada_RxState_t *pRX, NevadaRegisters_t *pReg)
{
    int m, i;
    wiUWORD SignalRate = {0xD};
    unsigned int    du = pRX->du-80; // start of SIG. 

    Nevada_Depuncture(24, pRX->Lc, pRX->cA, pRX->cB, SignalRate, pRX->RX_nMode);
    Nevada_VitDec    (24, pRX->cA, pRX->cB, pRX->b, pRX);

    pRX->RX_RATE.w7 = 0; // Increased word length for MCS 0-127
    pRX->RX_RATE.b3 = pRX->b[0].bit; // Legacy Rate: 4 bits
    pRX->RX_RATE.b2 = pRX->b[1].bit;
    pRX->RX_RATE.b1 = pRX->b[2].bit;
    pRX->RX_RATE.b0 = pRX->b[3].bit;

    pRX->RX_LENGTH.w16 = 0; // Increased word length for HT packets
    for (i=0; i<12; i++)
        pRX->RX_LENGTH.w16 = pRX->RX_LENGTH.w16 | (pRX->b[5+i].bit << i);

	pRX->L_LENGTH.word = pRX->RX_LENGTH.w12;

    if (pRX->RX_LENGTH.w16==0 || (pRX->RX_LENGTH.w16>>4) > pReg->maxLENGTH.w8)
    {
        pRX->Fault = 1;
        pRX->EventFlag.b1 = 1;
        aSigOut->RxFault = 1;
    }
    // ------------
    // Parity check
    // ------------
    {
        int z;
        for (z=0, i=0; i<24; i++)
            z = z ^ pRX->b[i].bit;
    
        if (z) 
        {
            pRX->Fault = 1;
            pRX->EventFlag.b2 = 1;
            aSigOut->RxFault = 1;
        }
    }

    // -------------------------------------------------------
    // Determine the code and subcarrier modulation parameters
    // -------------------------------------------------------
    switch (pRX->RX_RATE.w4)  // change to 7 bit word
    {
        case 0xD: pRX->N_BPSC = 1; pRX->N_DBPS =  24; break; //  6 Mbps
        case 0xF: pRX->N_BPSC = 1; pRX->N_DBPS =  36; break; //  9 Mbps
        case 0x5: pRX->N_BPSC = 2; pRX->N_DBPS =  48; break; // 12 Mbps
        case 0x7: pRX->N_BPSC = 2; pRX->N_DBPS =  72; break; // 18 Mbps
        case 0x9: pRX->N_BPSC = 4; pRX->N_DBPS =  96; break; // 24 Mbps
        case 0xB: pRX->N_BPSC = 4; pRX->N_DBPS = 144; break; // 36 Mbps
        case 0x1: pRX->N_BPSC = 6; pRX->N_DBPS = 192; break; // 48 Mbps
        case 0x3: pRX->N_BPSC = 6; pRX->N_DBPS = 216; break; // 54 Mbps
        default : pRX->N_BPSC = 1; pRX->N_DBPS =  24;        // defaults to prevent divide-by-zero below
            pRX->Fault = 1;
            pRX->EventFlag.b3 = 1;
            aSigOut->RxFault = 1;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Check the "Reserved" Bit
    //
    //  Register bit Reserved0 allows this to optionally be required to be 0 to improve
    //  rejection of bad headers.
    //
    if (pReg->Reserved0.b0 && pRX->b[4].bit) aSigOut->RxFault = 1;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Declare RxFault for 6 Mbps with LENGTH < 7 bytes. At this length the implied
    //  payload length is only N_SYM < 4. In the RTL design this creates multiple issues
    //  due to changes meant to handle HT-MF packets (thus only 6 Mbps is affected).
    //  Because the current 802.11 protocol never uses a PSDU size fewer than 14 bytes,
    //  it is valid to not pass the data to the MAC.
    //
    if ((pRX->RX_RATE.w4 == 0xD) && (pRX->RX_LENGTH.w12 < 7)) aSigOut->RxFault = 1;

	/////////////////////////////////////////////////////////////////////////////////////
	//
    //  Header Valid
    //
    if (!aSigOut->RxFault)
    {
	    /////////////////////////////////////////////////////////////////////////////////////
	    //
        //  Compute the number of bits in the DATA field
	    //  
	    //  The data field contains 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits, and 
	    //  enough pad bits to fill a whole number of OFDM symbols, each containing 
	    //  N_DBPS bits.
	    //
        pRX->N_SYM = (16 + 8*pRX->RX_LENGTH.w12 + 6 + (pRX->N_DBPS-1)) / pRX->N_DBPS;
       
	    /////////////////////////////////////////////////////////////////////////////////////
	    //
	    //  Check for packet too long
	    //
	    //  If the header indicates a packet length that is longer than the allocated arrays
	    //  indicate a fault and set the number of symbols to 1 so the simulation can
	    //  continue (packet will be bad). This situation can occur if either the PLCP header
	    //  is decoded incorrectly or if the aLength and rLength parameters is too small
	    //  for the received packet.
	    //
        if ((pRX->N_SYM >= (pRX->rLength-4*du)/320) || (pRX->N_SYM > pRX->aLength/pRX->N_DBPS) 
        ||  (pRX->N_SYM > (NEVADA_MAX_N_SYM - 8)))
	    {
		    pRX->N_SYM = 1; 
		    pRX->Fault = 4; 
		    pRX->RTL_Exception.Truncated_N_SYM = 1;
        }

        pRX->Nu = pRX->Nv = 64*(3+pRX->N_SYM);  // N_SYM+ L_LTF+L_SIG?

        // --------------------------------------------------------------
        // Check that the decoded LENGTH is valid for the receiver buffer
        // --------------------------------------------------------------
        if ((pRX->N_SYM+6) * 320 > pRX->rLength)  
        {
            pRX->Fault = 2;
            pRX->EventFlag.b8 = 1;
            aSigOut->RxFault = 1;
            pRX->RTL_Exception.rLimit_SIGNAL = 1;
            return WI_WARN_PHY_SIGNAL_LENGTH;
        }
        // ------------------------------------------------------------------
        // Compute the convolutioanl encoder rate x 12, e.g., CodeRate = m/12
        // Also compute the number of code bits per symbol
        // ------------------------------------------------------------------
        m = pRX->N_DBPS / (4 * pRX->N_BPSC);
        pRX->N_CBPS = pRX->N_DBPS * 12 / m;

        if (!pReg->RXMode.b2 || pRX->RX_RATE.word != 0xD) // modified to distinguish RXMode 5, 7 from 1, 3 
        {
            pRX->CheckRotate = 0;
            pRX->Rotated6Mbps = 0;
            pRX->LegacyPacket = 1; // Start normal legacy packet processing
        } 
        aSigOut->HeaderValid = 1; // valid header if no fault
        pRX->aPacketDuration.word = (pRX->N_SYM < 2) ? 0 : (4*pRX->N_SYM - 5); // adjust offset to match hardware latency
    }
    else
    {
        aSigOut->HeaderValid = 0;
        pRX->aPacketDuration.word = 0;
    }
    pRX->Rotated6MbpsValid = 0; // Must clear this signal before state transfer from state 11 for valid header; needed for state 20
  
    return WI_SUCCESS;
}
// end of Nevada_RX2_SigDecode()

// ================================================================================================
// FUNCTION : Nevada_RX2_HtSigDemod1() 
// ------------------------------------------------------------------------------------------------
// Purpose  : Demod HTSIG1 or DATA symbol 1 of legacy 6M 
//          : CheckRotate is set to 1 from the SigDemod
// ================================================================================================
wiStatus Nevada_RX2_HtSigDemod1(Nevada_RxState_t *pRX, NevadaRegisters_t *pReg)
{
    unsigned int dp = pRX->dp;
    unsigned int dc = pRX->dc;
    unsigned int dx = pRX->dx;
    unsigned int dv = pRX->dv;
    unsigned int du = pRX->du;

    wiUWORD SignalRate = {0xD};
    wiWORD pReal[2][4], pImag[2][4];

    if (dx>pRX->Nx) 
    {        
        pRX->Fault = 2;
        pRX->EventFlag.b8 = 1;
        pRX->RTL_Exception.xLimit_SIGNAL = 1;
        return WI_WARN_PHY_BAD_PACKET;
    }
    
    // ---------------------------------------------------
    // Record the channel estimation if trace is enabled
    // The CE arrays use offset du for EqD input
    // Record input CE for HT-SIG1
    // ---------------------------------------------------
    Nevada_TraceChEst(du);

    pRX->kFFT[1] = dx;

    if (pReg->PathSel.b0) Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx, pRX->xImag[0]+dx, NULL, NULL, pRX->vReal[0]+dx, pRX->vImag[0]+dx);
    if (pReg->PathSel.b1) Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx, pRX->xImag[1]+dx, NULL, NULL, pRX->vReal[1]+dx, pRX->vImag[1]+dx);
    if (pReg->PathSel.w2) XSTATUS(Nevada_MarkValidTraceRX2(1, dv, 64));

    Nevada_DFT_pilot(pRX->xReal[0]+dx, pRX->xImag[0]+dx, pReal[0], pImag[0]);
    Nevada_DFT_pilot(pRX->xReal[1]+dx, pRX->xImag[1]+dx, pReal[1], pImag[1]);

    // carrier phase tracking based on pilot tones
    // pRX->RX_nMode = 0 before decoding of HT-SIG
    Nevada_WeightedPilotPhaseTrack( 0, 0, pReg->PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], 
                                    pRX->cSP[0], pRX->d2AdvDly, &(pRX->cCPP), pRX, pReg,pRX->RX_nMode, 0);

    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[0], pRX->cCP[0], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, pRX->vImag[1]+dv,
                         pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du );
    

    Nevada_EqD( 0, pReg->PathSel, SignalRate, pRX->NoiseShift[0], pRX->NoiseShift[1],  
                pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], pRX->h2[0], pRX->h2[1], 
                pRX->uReal[0], pRX->uImag[0], pRX->uReal[1], pRX->uImag[1], pRX->H2, pRX->dReal, pRX->dImag, 
                pRX->DReal, pRX->DImag, du, pRX->Lp+dp, pRX->RX_nMode, pRX, 0 ); //FInit 0  nMode 0, pRX, sym 0, 
   
    Nevada_PLL( pRX->dReal+du, pRX->dImag+du, pRX->H2+du, pRX->DReal+du, pRX->DImag+du, 0, 0, 0,
		        pRX->AdvDly+du, pRX->cSP+1, pRX->cCP+1, pRX, pReg );
    pRX->d2AdvDly = pRX->dAdvDly; pRX->dAdvDly = pRX->AdvDly[du];

    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL) pRX->dAdvDly.word = 0; // TEST Feature not in ASIC

    Nevada_Interleaver(SignalRate, pRX->Lp+dp, pRX->Lc+dc, 1, 0, pRX->Rotated6Mbps);
    pRX->CheckRotate = 0;      

    // -----------------------------------------------
    // Advance and save array pointers for next symbol
    // -----------------------------------------------
    pRX->dx = dx + 80+pRX->d2AdvDly.w2; 
    pRX->dv = dv + 80+pRX->d2AdvDly.w2;
    pRX->du = du + 80+pRX->d2AdvDly.w2;
    pRX->dp = dp + 96; // EqD output (I/Q) LLR for 6 Mbps
    pRX->dc = dc + 48; // Interleaver output LLR  for 6 Mbps
     
    return WI_SUCCESS;
}
// end of Nevada_RX2_HtSigDemod1()

// ================================================================================================
// FUNCTION : Nevada_RX2_HtSigDemod2()  
// ------------------------------------------------------------------------------------------------
// Purpose  : Demodulate HT-SIG2 for Mixed mode and Greenfield mode
// ================================================================================================
wiStatus Nevada_RX2_HtSigDemod2(Nevada_RxState_t *pRX, NevadaRegisters_t *pReg, wiBoolean Greenfield)
{
    unsigned int dc = pRX->dc;
    unsigned int dp = pRX->dp;
    unsigned int dx = pRX->dx;
    unsigned int dv = pRX->dv;
    unsigned int du = pRX->du; 

    wiUWORD SignalRate = {0xD};   
    wiWORD pReal[2][4], pImag[2][4];

    if (dx>pRX->Nx) 
    {        
        pRX->Fault = 2;
        pRX->EventFlag.b8 = 1;
        pRX->RTL_Exception.xLimit_DataDemod = 1;
        return WI_WARN_PHY_BAD_PACKET;
    }

    // ---------------------------------------------------
    // Record the channel estimation if trace is enabled
    // The CE arrays use offset du for EqD input
    // Record input CE for HT-SIG2
    // ---------------------------------------------------
    Nevada_TraceChEst(du);

    pRX->kFFT[Greenfield ? 1:2] = dx;        

    if (pReg->PathSel.b0) Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx, pRX->xImag[0]+dx, NULL, NULL, pRX->vReal[0]+dx, pRX->vImag[0]+dx);
    if (pReg->PathSel.b1) Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx, pRX->xImag[1]+dx, NULL, NULL, pRX->vReal[1]+dx, pRX->vImag[1]+dx);
    if (pReg->PathSel.w2) XSTATUS(Nevada_MarkValidTraceRX2(1, dv, 64));
    
    if (pReg->PathSel.b0) Nevada_DFT_pilot(pRX->xReal[0]+dx, pRX->xImag[0]+dx, pReal[0], pImag[0]);
    if (pReg->PathSel.b1) Nevada_DFT_pilot(pRX->xReal[1]+dx, pRX->xImag[1]+dx, pReal[1], pImag[1]);

    // carrier phase tracking based on pilot tones
    // note: pRX->RX_nMode = 0 before decoding of HT-SIG
    Nevada_WeightedPilotPhaseTrack( 0, 0, pReg->PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], 
                                    pRX->cSP[Greenfield ? 0:1], pRX->d2AdvDly, &(pRX->cCPP), 
                                    pRX, pReg, pRX->RX_nMode, 0 );
    
    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[Greenfield ? 0:1], pRX->cCP[Greenfield ? 0:1], 
                         pRX->cCPP, pRX, pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, 
                         pRX->vImag[1]+dv, pRX->uReal[0]+du, pRX->uImag[0]+du, 
                         pRX->uReal[1]+du, pRX->uImag[1]+du );
        
    Nevada_EqD( 0, pReg->PathSel, SignalRate, pRX->NoiseShift[0], pRX->NoiseShift[1],  
                pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], pRX->h2[0], pRX->h2[1], 
                pRX->uReal[0], pRX->uImag[0], pRX->uReal[1], pRX->uImag[1], pRX->H2, pRX->dReal, pRX->dImag, 
                pRX->DReal, pRX->DImag, du, pRX->Lp+dp, 0, pRX, 0 ); //  FInit 0 nMode 0, pRX, sym 0,
   
    Nevada_PLL( pRX->dReal+du, pRX->dImag+du, pRX->H2+du, pRX->DReal+du, pRX->DImag+du, 0, 0, 0,
                pRX->AdvDly+du, pRX->cSP+(Greenfield ? 1:2), pRX->cCP+(Greenfield ? 1:2), pRX, pReg );

    pRX->d2AdvDly = pRX->dAdvDly; pRX->dAdvDly = pRX->AdvDly[du];
    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL) pRX->dAdvDly.word = 0;

    Nevada_Interleaver(SignalRate, pRX->Lp+dp, pRX->Lc+dc, 1, 0, pRX->Rotated6Mbps);
    pRX->Rotated6Mbps = 0;   // Reset for Data Demod
  
	// -----------------------------------------------
	// Advance and save array pointers for next symbol
	// -----------------------------------------------
	pRX->dx = dx + 80+pRX->d2AdvDly.w2;
	pRX->dv = dv + 80+pRX->d2AdvDly.w2;
	pRX->du = du + 80+pRX->d2AdvDly.w2;
	pRX->dp = dp + 96; //EqD output LLR (I/Q) for 6 Mbps
	pRX->dc = dc + 48; // Interleaver output LLR;
    return WI_SUCCESS;
}
// end of Nevada_RX2_HtSigDemod2()

// ================================================================================================
// FUNCTION : Nevada_RX2_NumSymLDPC()  
// ------------------------------------------------------------------------------------------------
// Purpose  : Compute the number of symbols in an LDPC packet
// ================================================================================================
wiStatus Nevada_RX2_NumSymLDPC(wiUWORD m_STBC)
{
    // The numerator of the code rate. For both BCC and LDPC
    const unsigned int FECRateNum[77]  = { 1, 1, 3, 1, 3, 2, 3, 5, 1, 1, 3, 1, 3, 2, 3, 5, //  0-15
                                           1, 1, 3, 1, 3, 2, 3, 5, 1, 1, 3, 1, 3, 2, 3, 5, // 16-31
                                           1, 1, 1, 1, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 3, 3, // 32-47
                                           3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 48-63
                                           1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};         // 64-76

    // The denominator of the code rate. For both BCC and LDPC
    const unsigned int FECRateDen[77]  = { 2, 2, 4, 2, 4, 3, 4, 6, 2, 2, 4, 2, 4, 3, 4, 6, //  0-15
                                           2, 2, 4, 2, 4, 3, 4, 6, 2, 2, 4, 2, 4, 3, 4, 6, // 16-31
                                           2, 2, 2, 2, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 4, 4, // 32-47
                                           4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 48-63
                                           2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};         // 64-76

	unsigned int RateNum, RateDen, RatePrty, Npld, Navbits, N_CBPS, L_LDPC, N_CW, Nshrt, Npunc;
	unsigned int Ninfo, Nprty;
	unsigned int Nbit1, Nbit2, Nbit3, Nbit4;
     
    Nevada_RxState_t *pRX = Nevada_RxState();

	RateNum  = FECRateNum[pRX->RX_RATE.w7]; // Numerator of encoder rate
	RateDen  = FECRateDen[pRX->RX_RATE.w7]; // Denominator
	RatePrty = RateDen - RateNum;           // For all code rate, this should result in 1. It can be removed.
	N_CBPS   = pRX->N_DBPS*RateDen/RateNum; // RTL option: put N_CBPS into LUT, 20M and 40M        

	Npld = pRX->RX_LENGTH.w16*8 + 16;  
	Navbits = N_CBPS * m_STBC.w2  * ((Npld +m_STBC.w2*pRX->N_DBPS-1)/(m_STBC.w2*pRX->N_DBPS)); // Ceiling to the nearest STBC frame

	switch (RateNum)
	{
		case 1: Nbit1 = 456; Nbit2 = 732; Nbit3 = 1458; Nbit4 =  972; break;                      
		case 2: Nbit1 = 304; Nbit2 = 488; Nbit3 =  972; Nbit4 = 1296; break;      
		case 3: Nbit1 = 228; Nbit2 = 366; Nbit3 =  729; Nbit4 = 1458; break;
		case 5: Nbit1 = 152; Nbit2 = 244; Nbit3 =  486; Nbit4 = 1620; break;         
		default: return STATUS(WI_ERROR_UNDEFINED_CASE);
	}      

	if (Navbits <= 648)  
	{
		N_CW = 1;             
		L_LDPC = (Navbits >= (Npld + Nbit1)) ? 1296:648;
		//L_LDPC = (Navbits >= (Npld + 912*RatePrty/RateDen)) ? 1296:648;  // The two calculations give same result
	}
	else if (Navbits <= 1296)
	{
		N_CW = 1;            
		L_LDPC = (Navbits >= (Npld + Nbit2)) ? 1944:1296;
		//L_LDPC = (Navbits >= (Npld + 1464*RatePrty/RateDen)) ? 1944:1296; 
	}
	else if (Navbits <= 1944)  
	{
		N_CW = 1; 
		L_LDPC = 1944;           
	}
	else if (Navbits <= 2592)
	{
		N_CW = 2;            
		L_LDPC = (Navbits >= (Npld + Nbit3)) ? 1944:1296;
		//L_LDPC = (Navbits >= (Npld + 2916*RatePrty/RateDen)) ? 1944:1296;           
	}
	else
	{            
		N_CW = (Npld + Nbit4 - 1)/(Nbit4);
		//N_CW = (Npld + 1944*RateNum/RateDen -1)/(1944*RateNum/RateDen);
		L_LDPC = 1944;
	}

	Ninfo = L_LDPC*RateNum/RateDen; 
	Nprty = L_LDPC- Ninfo;

	Nshrt = (N_CW * Ninfo) - Npld;
	Npunc = (unsigned) max(0, (int) (N_CW*L_LDPC - Navbits - Nshrt));
        
	if ( ((Npunc > (unsigned int)(0.1*N_CW * Nprty)) && (Nshrt < (unsigned int)(1.2*Npunc*RateNum/RatePrty))) 
	||    (Npunc > (unsigned int)(0.3*N_CW * Nprty)) )
	{
		Navbits = Navbits + N_CBPS * m_STBC.w2;
	}            
	pRX->N_SYM = Navbits/N_CBPS;

    return WI_SUCCESS;
}
// end of Nevada_RX2_NumSymLDPC()

// ================================================================================================
// FUNCTION : Nevada_RX2_HtSigDecode()  
// ------------------------------------------------------------------------------------------------
// Purpose  : Decode HT-SIG for Mixed mode and Greenfield mode
//          : HtHeaderValid and HtRxFault replaces HeaderValid and RxFault in aSigState to generate
//            appropriate pipeline delay between PHY_RX_EN and HeaderValid in RX top state  
// ================================================================================================
wiStatus Nevada_RX2_HtSigDecode(Nevada_RxState_t *pRX, NevadaRegisters_t *pReg, wiBoolean Greenfield)
{
    // Number of encoder streams for BCC at 40 MHz. all 20M MCS has N_ES =1; LDPC code has N_ES of 1
    // Reference: 11n Draft Table 20-29 to 20-43    
    const unsigned int N_ES_40M[77]  = {  1, 1, 1, 1, 1, 1, 1, 1, 1 ,1 ,1 ,1, 1, 1, 1, 1,   //  0-15                 
                                          1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,   // 16-31            
                                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47         
                                          1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   // 48-63
                                          1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2 };          // 64-76

    // Number of spatial streams for 20 MHz and 40 MHz. MCS32 is invalid for 20M. The entry is for 40 MHz
    const unsigned int N_SS_All[77]  = {  1 ,1 ,1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,  //  0-15     
                                          3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,  // 16-31
                                          1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 32-47
                                          3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // 48-63
                                          4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };         // 64-76

    // Number of data bits per symbol for 20 MHz.  MCS 32 is not valid for 20 M. Set to get 24 for 40M 
    const unsigned int N_DBPS_20M[77] =  {  26, 52, 78,104,156,208,234,260, 52,104,156,208,312,416,468,520,  //  0-15
                                            78,156,234,312,468,624,702,780,104,208,312,416,624,832,936,1040, // 16-31
                                            12,156,208,260,234,312,390,208,260,260,312,364,364,416,312,390,  // 32-47     
                                           390,468,546,546,624,260,312,364,312,364,416,468,416,468,520,520,  // 48-63  
                                           572,390,468,546,468,546,624,702,624,702,780,780,858 }; // 64-76
                                          
    // Separate LUT for number of data bits per symbol for 40 MHz if not calculating from N_DBPS_20M 
    const unsigned int N_DBPS_40M[77] =  { 54,108,162,216,324,432,486,540,108, 216,324,432,648,864,972,1080,       // 0-15
                                           162,324,486,648,972,1296,1458,1620,216,432,648,864,1296,1728,1944,2160, // 16-31
                                           24,324,432,540,486,648,810,432,540,540,648,756,756,864,648,810,         // 32-47
                                           810,972,1134,1134,1296,540,648,756,648,756,864,972,864,972,1080,1080,   // 48-63
                                           1188,810,972,1134,972,1134,1296,1458,1296,1458,1620,1620,1782}; // 64-76

    int m, i;
    unsigned int dc = pRX->dc-96;    
    unsigned int db = Greenfield ? 0:24;      
    unsigned int du = pRX->du-80; // start of HT-SIG2. 

    wiUWORD SignalRate = {0xD};   
    wiBoolean BW40, Short_GI, LDPC; 
    wiUWORD N_ES;   // number of encoder stream for BCC
    wiUWORD N_ESS;  // number of extension spatial streams
    wiUWORD N_STS;  // number of space-time streams
    wiUWORD N_SS;   // number of spatial streams.
    wiUWORD N_DLTF; // number of data HT-LTF
    wiUWORD N_ELTF; // number of extension HT-LTF
    wiUWORD N_LTF;  // number of total HT-LTF
    wiUWORD STBC, m_STBC; // STBC: difference between N_STS and N_SS, m_STBC: STBC coding on 2 OFDM symbols. Required for calculation N_SYM 

    Nevada_Depuncture(48, pRX->Lc+dc, pRX->cA+db, pRX->cB+db, SignalRate, pRX->RX_nMode);
    Nevada_VitDec    (48, pRX->cA+db, pRX->cB+db, pRX->b+db, pRX);

    // MCS  
    pRX->RX_RATE.w7 = 0; 
    for (i=0; i<7; i++)
        pRX->RX_RATE.w7 = pRX->RX_RATE.w7 | (pRX->b[db+i].bit << i);

    pRX->RX_LENGTH.w16 = 0;
    for (i=0; i<16; i++)
        pRX->RX_LENGTH.w16 = pRX->RX_LENGTH.w16 | (pRX->b[db+8+i].bit << i);

    pRX->Aggregation = pRX->b[db+27].bit;  // set the aggregation bit; 
  
    // ------------
    // CRC check
    // ------------
    {         
        wiUInt  k, z;  // CRC Counter;
        wiUWORD c;     // CRC Shift Register Coefficient; 
        wiUInt  input; // shift register input, need to be unsigned; 

        c.word = 0xFF;  // Reset shift register to all ones

        for (k=0; k<34; k++)
        {       
            input=pRX->b[db+k].bit ^ c.b7;
            c.w8=(c.w7<<1) |(input &1);
            c.b1=c.b1^input;
            c.b2=c.b2^input;;
        }    
     
        for (z=0, k=0; k<8; k++)
            z+=pRX->b[db+41-k].bit^(~(c.word>>k) & 1);
        
        if (z) 
        {
            pRX->Fault = 1;
            pRX->EventFlag.b2 = 1;
            pRX->HtRxFault = 1;
        }
    }
  
    BW40 = pRX->b[db+7].bit;
    STBC.word = (pRX->b[db+29].bit<<1) +  pRX->b[db+28].bit; // STBC: LSB transmitted first    
    m_STBC.word = STBC.w2 ? 2:1;  // 2 if STBC is used, Alamouti encoding across symbols.  1 else;
    LDPC = pRX->b[db+30].bit;
    Short_GI = pRX->b[db+31].bit;
    N_ESS.word = (pRX->b[db+33].bit<<1) + pRX->b[db+32].bit;  // N_ESS: LSB transmitted first 
    if (STBC.w2 == 3) pRX->ReservedHTSig = 1; // ReservedHTSIG defined in 11n Draft

    // -------------------------------------------------------
    // Determine the code and subcarrier modulation parameters
    // -------------------------------------------------------
    if (!BW40)  // 20 MHz bandwidth
    {
        if (pRX->RX_RATE.w7 <8)
        {
            N_ES.word = 1;
            N_SS.word = N_SS_All[pRX->RX_RATE.w7];
            N_STS.word = N_SS.w3 + STBC.w2; 

            switch (pRX->RX_RATE.w7) // MCS[6:0]
            {
                case 0: pRX->N_BPSC = 1; pRX->N_DBPS =  26; break;
	            case 1: pRX->N_BPSC = 2; pRX->N_DBPS =  52; break;
		        case 2: pRX->N_BPSC = 2; pRX->N_DBPS =  78; break;
		        case 3: pRX->N_BPSC = 4; pRX->N_DBPS = 104; break;
		        case 4: pRX->N_BPSC = 4; pRX->N_DBPS = 156; break;
		        case 5: pRX->N_BPSC = 6; pRX->N_DBPS = 208; break;
		        case 6: pRX->N_BPSC = 6; pRX->N_DBPS = 234; break;
		        case 7: pRX->N_BPSC = 6; pRX->N_DBPS = 260; break;
                default:
                     pRX->N_BPSC = 1; pRX->N_DBPS =  24; // defaults to prevent divide-by-zero below
                     pRX->Fault = 1;
                     pRX->EventFlag.b3 = 1;
                     pRX->HtRxFault = 1;
                     break; 
            }
        }
        else if (pRX->RX_RATE.w7<32 || (pRX->RX_RATE.w7>32 && pRX->RX_RATE.w7<77))  // MCS 8-31 MCS 33-76 as unsupported
        {
            N_ES.word = 1;
            N_SS.word = N_SS_All[pRX->RX_RATE.w7];
            N_STS.word = N_SS.w3 + STBC.w2; 
            pRX->N_DBPS = N_DBPS_20M[pRX->RX_RATE.w7];
            pRX->N_BPSC = 1;  //  default to prevent divide-by-zero below. Note: That is not the actual BPSC 
            pRX->Unsupported = 1;
        }
        else  // MCS 77-127 Reserved,  MCS 32 not defined
        {
            pRX->N_BPSC = 1; pRX->N_DBPS =  24; N_ES.word =1; N_SS.word =1; // defaults to prevent divide-by-zero below
            N_STS.word = N_SS.w3 + STBC.w2;
            pRX->ReservedHTSig = 1; 
        }    
    }
    else    // 40 MHz bandwidth 
    {
        if (pRX->RX_RATE.w7<77)  // MCS 0-76 unsupported
        {
            N_ES.word = N_ES_40M[pRX->RX_RATE.w7];
            N_SS.word = N_SS_All[pRX->RX_RATE.w7];
            N_STS.word = N_SS.w3 + STBC.w2;  
            //pRX->N_DBPS = 108*N_DBPS_20M[pRX->RX_RATE.w7]/52; // Mapped from 20M to 40M with # of subcarrier change
            pRX->N_DBPS = N_DBPS_40M[pRX->RX_RATE.w7];
            pRX->N_BPSC = 1;  //  default to prevent divide-by-zero below. Note: That is not the actual BPSC 
            pRX->Unsupported = 1;
        }
        else  // MCS 77-127  Reserved 
        {            
            pRX->N_BPSC = 1; pRX->N_DBPS =  24; N_ES.word =1; N_SS.word =1; // defaults to prevent divide-by-zero below        
            N_STS.word = N_SS.w3 + STBC.w2;
            pRX->ReservedHTSig = 1;
        }
    }

    switch (N_STS.w3)
    {
        case 1:         N_DLTF.word = 1; break;
        case 2:         N_DLTF.word = 2; break;
        case 3: case 4: N_DLTF.word = 4; break;
        default:        N_DLTF.word = 0;  
    }
    switch (N_ESS.w2)
    {
        case 0:         N_ELTF.word = 0; break;
        case 1:         N_ELTF.word = 1; break;
        case 2:         N_ELTF.word = 2; break;
        case 3:         N_ELTF.word = 4; break;
        default:        N_ELTF.word = 0;  
    }

    N_LTF.word = N_DLTF.w3 + N_ELTF.w3;  // Total HT-LTF;  
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Check Limits on N_LTF:
    //	  - Condition 1: N_LTF <=5 
	//    - Condition 2: N_STS + N_ESS <=4  (N_STS =4, N_ESS =1 invalid although N_LTF<=5) 
	//    - Check on condition 2 will cover condition 1.
    //
    if ((N_STS.w3 + N_ESS.w2 >4) || !N_DLTF.w3) 
    {
        N_LTF.word = 0; 
        pRX->Fault = 1;
        pRX->HtRxFault = 1;
    }
   
    // ------------------------
    // Check the "Reserved" Bit
    // ------------------------
    if (pReg->nReserved1.b0 && !pRX->b[db+26].bit) pRX->ReservedHTSig = 1; // reserved bit is "1" not "0", pReg->nReserved1 set to 1 to enable checking;

	
    if (pRX->ReservedHTSig) // Treat ReservedHTSig as RxFault 
    {
        pRX->Fault = 1;
        pRX->EventFlag.b3 = 1;
        pRX->HtRxFault = 1;
    }
	
	if (!pRX->HtRxFault)
	{
		//////////////////////////////////////////////////////////////////////////////////////
		//
		//  Compute the number of symbols in the DATA field. For BCC, the data field contains
		//  16 SERVICE bits, 8*LENGTH data bits, 6*N_ES tail bits, and enough pad bits to
		//  fill a whole number of OFDM symbols depending on STBC, each containing N_DBPS 
		//  bits.
        //
		//  The calculation for LDPC follows IEEE 802.11n section 20.3.10.6.5.
        //
		//	RX LENGTH of 0 indicates a NULL DATA Packet (NDP). N_SYM should set to 0.
		//
        if (pRX->RX_LENGTH.w16 == 0)
        {
            pRX->N_SYM = 0; // Null data packet.
        }
        else if (LDPC)
		{   
            XSTATUS( Nevada_RX2_NumSymLDPC( m_STBC ) );
        }
		else // BCC (per 802.11n 20.3.10 equation 20-32)
		{
			pRX->N_SYM = m_STBC.w2*((16 + 8*pRX->RX_LENGTH.w16 + 6*N_ES.w2 + (m_STBC.w2*pRX->N_DBPS-1)) /(m_STBC.w2*pRX->N_DBPS));
		}

		if (pRX->RX_LENGTH.w16 == 0) pRX->Unsupported = 1; // Treat HT LENGTH of 0 as unsupported
		if (!pRX->b[db+25].bit)		 pRX->Unsupported = pReg->NotSounding.b0; // sounding PSDU unsupported;
		if (STBC.w2 != 0)			 pRX->Unsupported = 1; // STBC unsupported;
		if (LDPC)					 pRX->Unsupported = 1; // LDPC unsupported
		if (Short_GI)				 pRX->Unsupported = 1; // Short GI unsupported
		if (N_ESS.w2 != 0)			 pRX->Unsupported = 1; // ESS unsupported;
		if (Greenfield)				 pRX->Unsupported = 1; // Only HT Mixed Mode is supported
        if ((pRX->RX_LENGTH.w16) > (pReg->maxLENGTHnH.w8<<8 | pReg->maxLENGTHnL.w8))
            pRX->Unsupported = 1;                          // Treat HT LENGTH > maxLENGTHn as unsupported

		/////////////////////////////////////////////////////////////////////////////////////
		//
		//  Check for packet too long
		//
		//  If the header indicates a packet length that is longer than the allocated arrays
		//  indicate a fault and set the number of symbols to 1 so the simulation can
		//  continue (packet will be bad). This situation can occur if either the PLCP header
		//  is decoded incorrectly or if the aLength and rLength parameters is too small
		//  for the received packet.
		//
		if ((pRX->N_SYM >= (pRX->rLength-4*du)/(Short_GI? 288:320)) || (pRX->N_SYM > pRX->aLength/pRX->N_DBPS) )		
		{ 
			pRX->N_SYM = 1; 
			pRX->Fault = 4; 
			pRX->RTL_Exception.Truncated_N_SYM = 1; 
		}

		pRX->Nu = pRX->Nv = 64 * (pRX->N_SYM + (Greenfield ? 4:7)); // 2 L-LTF + L_SIG +2HT-SIG +HT-LTF + HT-STF (MM)  2 LTF+2HT-SIG

		// --------------------------------------------------------------
		// Check that the decoded LENGTH is valid for the receiver buffer
		// --------------------------------------------------------------
		if ((pRX->N_SYM *(Short_GI? 288:320) + (Greenfield? 6+N_LTF.w3 : 9+N_LTF.w3) * 320 )> pRX->rLength)   // 6 for legacy, set 10 MM, 7 GF
		{        
			pRX->Fault = 2;
			pRX->EventFlag.b8 = 1;
			pRX->HtRxFault = 1;
			pRX->RTL_Exception.rLimit_SIGNAL = 1;
			return WI_WARN_PHY_SIGNAL_LENGTH;
		}

		/////////////////////////////////////////////////////////////////////////////////////
		//
		//  Compute the convolutioanl encoder rate x 12, e.g., CodeRate = m/12.
		//  Also compute the number of code bits per symbol. For the HT mode, the relation
		//  before integar implementation is CodeRate = m/13, which is obvious from 52=4*13. 
		//  For integar m and code rate 1/2, 2/3, 3/4, 5/6, m=floor(13*CodeRate)=12*CodeRate
		//  That is the equation looks exactly as the legacy case.
		//
		m = pRX->N_DBPS / (4 * pRX->N_BPSC); 
		pRX->N_CBPS = pRX->N_DBPS * 12 / m;

		
		/////////////////////////////////////////////////////////////////////////////////////
		//
		//  The PacketDuration predicts the rest of packet time in order to set CCA.
		//  For supported packet (mixed mode), the current solution sends HT header to the 
		//  MAC and sets PktTime after HT-SIG decoder; therefore, there is 8 us accounting
		//  for HT preamble time.
		//
		//  For unsupported packets, the PktTime is also set after HT-SIG decoder.
		//
		if (!pRX->Unsupported) 
        {
            // aPacketDuration need fine adjustment based on RTL results 5.5us delay. Rounded to 5
            pRX->aPacketDuration.word = 4*pRX->N_SYM+8-5; // 8us accounts for HT preambles
        }
        else if (Greenfield)  
        {   
            // ceil operation to align packets with Short GI to 4us boundary for CCA (11n Draft 20.3.21.5, 20.4.3). Dummy operation for regular GI. 
            if ((pRX->N_SYM + N_LTF.w3 - 1) < 2)
                pRX->aPacketDuration.word = 0;
            else
                pRX->aPacketDuration.word = (unsigned int)(4*(N_LTF.w3-1)+4*(int)ceil((Short_GI?3.6:4)*pRX->N_SYM/4)-5); 
        }
        else
        {
			// if it is unsupported MM mode, UpdatePktDuration enables recalculate PktDuration based on HT-SIG
            // minimum is two symbols for HT-STF and HT-LTF1; N_LTF+1: HT-STF + HT-LTFs			
            pRX->aPacketDuration.word = (unsigned int)(4*(N_LTF.w3+1) + 4*(int)ceil((Short_GI?3.6:4)*pRX->N_SYM/4)-5); 
			
        }
		if (pRX->aPacketDuration.word > (pReg->aPPDUMaxTimeH.w8<<8 | pReg->aPPDUMaxTimeL.w8))
			pRX->HtRxFault = 1;
	}
        
    pRX->HtHeaderValid = pRX->HtRxFault? 0:1; // valid HT header if no fault. aSigState HeaderValid will be update at certain time later
    	
    if (pRX->HtHeaderValid)
    {
        pRX->RX_nMode = 1;
		pRX->Greenfield = Greenfield ? 1:0;     
    }
    else
	{
        pRX->aPacketDuration.word = 0;  
		pRX->Unsupported = 0;
	}
    return WI_SUCCESS;
}
// end of Nevada_RX2_HtSigDecode()


// ================================================================================================
// FUNCTION : Nevada_RX2_HtChEst()
// ------------------------------------------------------------------------------------------------
// Purpose  : Process the HT preamble for mixed mode. Channel estimation based on HT-LTF
//          : pRX->RX_nMode is 1 for this block
// ================================================================================================
wiStatus Nevada_RX2_HtChEst(Nevada_aSigState_t *aSigOut, Nevada_RxState_t *pRX, NevadaRegisters_t *pReg)
{
    unsigned int dx = pRX->dx;
    unsigned int dv = pRX->dv;
    unsigned int du = pRX->du;
    
    wiWORD pReal[2][4], pImag[2][4];
    
    // HT-STF

    // ---------------------------------------------------
    // Record the channel estimation if trace is enabled
    // The CE arrays use offset du for EqD input
    // Record input CE for HT-STF
    // ---------------------------------------------------
    Nevada_TraceChEst(du);

    pRX->kFFT[3] = dx;   

    // FFT block, EqD block no use; Not necessary to put here; Both PLL only holding the VCO values; 
    if (pReg->PathSel.b0) Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx, pRX->xImag[0]+dx, NULL, NULL, pRX->vReal[0]+dx, pRX->vImag[0]+dx);
    if (pReg->PathSel.b1) Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx, pRX->xImag[1]+dx, NULL, NULL, pRX->vReal[1]+dx, pRX->vImag[1]+dx);

    Nevada_DFT_pilot(pRX->xReal[0]+dx, pRX->xImag[0]+dx, pReal[0], pImag[0]);
    Nevada_DFT_pilot(pRX->xReal[1]+dx, pRX->xImag[1]+dx, pReal[1], pImag[1]);
    
    // Necessary to hold loop filter VCO and update the total VCO hence phase error
    Nevada_WeightedPilotPhaseTrack( 0, 1, pReg->PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], 
                                    pRX->cSP[2], pRX->d2AdvDly, &(pRX->cCPP), pRX, pReg, pRX->RX_nMode, 0);
    
    // HT-STF is not used, this correction is not essential
    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[2], pRX->cCP[2], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, pRX->vImag[1]+dv,
                         pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du);

    // Necessary to hold loop filter VCO and update the total VCO hence phase error
    Nevada_PLL(pRX->dReal+du, pRX->dImag+du, pRX->H2+du, pRX->DReal+du, pRX->DImag+du, 0, 1, 0, pRX->AdvDly+du, pRX->cSP+3, pRX->cCP+3, pRX, pReg);
    pRX->d2AdvDly = pRX->dAdvDly; pRX->dAdvDly = pRX->AdvDly[du]; // Hold the DPLL VCO value for the HT short preamble;
    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL) pRX->dAdvDly.word = 0; // TEST Feature not in ASIC

    // Channel Estimation on HT-LTF
    dx += 80 + pRX->d2AdvDly.w2;
    dv += 80 + pRX->d2AdvDly.w2;
    du += 80 + pRX->d2AdvDly.w2;

    pRX->kFFT[4] = dx;      
    if (pReg->PathSel.b0) Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx, pRX->xImag[0]+dx, NULL, NULL, pRX->vReal[0]+dx, pRX->vImag[0]+dx);
    if (pReg->PathSel.b1) Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx, pRX->xImag[1]+dx, NULL, NULL, pRX->vReal[1]+dx, pRX->vImag[1]+dx);
    if (pReg->PathSel.w2) XSTATUS(Nevada_MarkValidTraceRX2(1, dv, 64));

    Nevada_DFT_pilot(pRX->xReal[0]+dx, pRX->xImag[0]+dx, pReal[0], pImag[0]);
    Nevada_DFT_pilot(pRX->xReal[1]+dx, pRX->xImag[1]+dx, pReal[1], pImag[1]);

    // Hold VCO values for HT-LTF
    Nevada_WeightedPilotPhaseTrack( 0, 1, pReg->PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], 
                                    pRX->cSP[3], pRX->d2AdvDly, &(pRX->cCPP), pRX, pReg, pRX->RX_nMode,0 );
    
    // --------------------------------------------------
    // Phase Correction and Channel Estimation on HT-LTF
    // Phase correct on HT subcarriers
    // --------------------------------------------------    
    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[3], pRX->cCP[3], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, pRX->vImag[1]+dv,
                         pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du);
      

    Nevada_ChEst( pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du,
                  NULL, NULL, NULL, NULL, 
                  pRX->hReal[0], pRX->hImag[0], pRX->h2[0], pRX->hReal[1], pRX->hImag[1], pRX->h2[1], pReg, 1);
    aSigOut->ChEstDone = 1; 

    // Hold VCO values for HT-LTF
    Nevada_PLL( pRX->dReal+du, pRX->dImag+du, pRX->H2+du, pRX->DReal+du, pRX->DImag+du, 0, 1, 0,
		        pRX->AdvDly+du, pRX->cSP+4, pRX->cCP+4, pRX, pReg);
    pRX->d2AdvDly = pRX->dAdvDly; pRX->dAdvDly = pRX->AdvDly[du]; 
    
	if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL) pRX->dAdvDly.word = 0; // TEST Feature not in ASIC

    // -----------------------------------------------
    // Advance and save array pointers for next symbol
    // -----------------------------------------------
    pRX->dx = dx + 80 + pRX->d2AdvDly.w2;
    pRX->dv = dv + 80 + pRX->d2AdvDly.w2;
    pRX->du = du + 80 + pRX->d2AdvDly.w2;
   
    return WI_SUCCESS;
}
// end of Nevada_RX2_HtChEst

// ================================================================================================
// FUNCTION : Nevada_RX2_DATA_Demod()
// ------------------------------------------------------------------------------------------------
// Purpose  : Implement RX2 demodulate/demapping of OFDM DATA symbols
// ================================================================================================
wiStatus Nevada_RX2_DATA_Demod(unsigned int n, Nevada_RxState_t *pRX, NevadaRegisters_t *pReg)
{
    unsigned int dc = pRX->dc;
    unsigned int dp = pRX->dp;
    unsigned int dx = pRX->dx;
    unsigned int dv = pRX->dv;
    unsigned int du = pRX->du;
    wiWORD pReal[2][4], pImag[2][4];
	int ofs; // offset for phase correction
    int UpdateH; // Update the DPLL channel power
	
    // -----------------------------------------------------------------------
    // Demodulate the "DATA" OFDM symbols and de-interleave the soft decisions
    // -----------------------------------------------------------------------
    if (dx > pRX->Nx) 
    {
        pRX->N_SYM = n - 2; // set N_SYM to prevent trace errors
        pRX->Fault = 2;
        pRX->EventFlag.b8 = 1;
        pRX->RTL_Exception.xLimit_DataDemod = 1;
        return WI_WARN_PHY_BAD_PACKET;
    }
      
    // ---------------------------------------------------
    // Record the channel estimation if trace is enabled
    // The CE arrays use offset du for EqD input
    // Record input CE for Data Symbol
    // ---------------------------------------------------
    Nevada_TraceChEst(du);

    ofs = pRX->RX_nMode? 4: 0;  
   
    pRX->kFFT[pRX->RX_nMode ? (n+4):n] = dx;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set UpdateH to update CE pwr from Eqd after Eqd of first HT Data symbol. 
    //  For channel tracking update for each symbol
    //
    UpdateH = (pReg->ChTracking.b0 || (pRX->RX_nMode && n == 1)) ? 1 : 0;

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  FFT
	//
	//  Select the FFT and DFT based on PathSel. Hardware will disable the unused paths
	//  to save power. Here it is done to improve simulation time. The FFT is used for
	//  normal data decoding with the DFT_pilot module computes DFTs only on the pilot
	//  tones for use by the pilot tone DPLL.
	// 
    if (pReg->PathSel.b0)
	{
	    Nevada_FFT(1, NULL, NULL, pRX->xReal[0]+dx, pRX->xImag[0]+dx, NULL, NULL, pRX->vReal[0]+dx, pRX->vImag[0]+dx);
	    Nevada_DFT_pilot(pRX->xReal[0]+dx, pRX->xImag[0]+dx, pReal[0], pImag[0]);
	}
    if (pReg->PathSel.b1)
	{
		Nevada_FFT(1, NULL, NULL, pRX->xReal[1]+dx, pRX->xImag[1]+dx, NULL, NULL, pRX->vReal[1]+dx, pRX->vImag[1]+dx);
	    Nevada_DFT_pilot(pRX->xReal[1]+dx, pRX->xImag[1]+dx, pReal[1], pImag[1]);
	}
    if (pReg->PathSel.w2) XSTATUS(Nevada_MarkValidTraceRX2(1, dv, 64));

    // carrier phase tracking based on pilot tones
    Nevada_WeightedPilotPhaseTrack( 0, 0, pReg->PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], 
                                    pRX->cSP[n+ofs-1], pRX->d2AdvDly, &(pRX->cCPP), pRX, pReg, pRX->RX_nMode,n-1 );

    Nevada_PhaseCorrect( pReg->PathSel, 80, pRX->cSP[n+ofs-1], pRX->cCP[n+ofs-1], pRX->cCPP, pRX,
                         pRX->vReal[0]+dv, pRX->vImag[0]+dv, pRX->vReal[1]+dv, pRX->vImag[1]+dv,
                         pRX->uReal[0]+du, pRX->uImag[0]+du, pRX->uReal[1]+du, pRX->uImag[1]+du );
    

    Nevada_EqD( 0, pReg->PathSel, pRX->RX_RATE, pRX->NoiseShift[0], pRX->NoiseShift[1],  
                pRX->hReal[0], pRX->hImag[0], pRX->hReal[1], pRX->hImag[1], pRX->h2[0], pRX->h2[1], 
                pRX->uReal[0], pRX->uImag[0], pRX->uReal[1], pRX->uImag[1], pRX->H2, pRX->dReal, pRX->dImag, 
                pRX->DReal, pRX->DImag, du, pRX->Lp+dp, pRX->RX_nMode, pRX, n-1); //Finit 0
       
    Nevada_PLL( pRX->dReal+du, pRX->dImag+du, pRX->H2+du, pRX->DReal+du, pRX->DImag+du, 0, 0, UpdateH,
		        pRX->AdvDly+du, pRX->cSP+n+ofs, pRX->cCP+n+ofs, pRX, pReg );
    pRX->d2AdvDly = pRX->dAdvDly; pRX->dAdvDly = pRX->AdvDly[du];

    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisablePLL) pRX->dAdvDly.word = 0; // TEST Feature not in ASIC

    if (pRX->d2AdvDly.w2 && n<pRX->N_SYM) pRX->EventFlag.b6 = 1; // symbol timing shift during packet
    
    Nevada_Interleaver(pRX->RX_RATE, pRX->Lp+dp, pRX->Lc+dc, 1, pRX->RX_nMode, pRX->Rotated6Mbps);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Advance and save array pointers for next symbol
    //
    //  For arrays x (FFT input), v (FFT output), and u (phase correct output), samples
    //  are stored at 20 MS/s so each symbol is 80T. If sample time tracking causes the
    //  time window to move forward/back by a sample, the effect is stored in d2AdvDly.
    //
    //  Arrays p and c depend on the number of LLR values (code bits/symbol which is 
    //  rate/MCS selective). For 6 Mbps (RATE = 0xD), BPSK demodulated LLR are stored
    //  along with another set computed for a rotated constellation so the "p" array
    //  contains twice the N_CBPS samples.
    //
    pRX->dx = dx + 80 + pRX->d2AdvDly.w2;
    pRX->dv = dv + 80 + pRX->d2AdvDly.w2;
    pRX->du = du + 80 + pRX->d2AdvDly.w2;
    pRX->dp = dp + pRX->N_CBPS * (pRX->RX_RATE.word == 0xD? 2:1);  
    pRX->dc = dc + pRX->N_CBPS;
  
    return WI_SUCCESS;
}
// end of Nevada_RX2_DATA_Demod()

// ================================================================================================
// FUNCTION : Nevada_RX2_DATA_Decode()
// ------------------------------------------------------------------------------------------------
// Purpose  : Implement RX2 decoding of OFDM DATA symbols
//          : Here the input of nMode is pRX->RX_nMode. Put here as a separate input instead of using 
//          : pRX directly in case they are not fit. 
// ================================================================================================
wiStatus Nevada_RX2_DATA_Decode(wiBoolean nMode, Nevada_RxState_t *pRX, NevadaRegisters_t *pReg)
{
    int da = nMode? 24*3: 24; 
    int dc = nMode? 48*3: 48;
    int Nbits  = 16 + 8*pRX->RX_LENGTH.w16 + 6; // number of bits in DATA field (including tail)
    wiUWORD *a = pRX->a + da; // serial data pointer offset past SIGNAL symbol
    unsigned int i, k;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Data Decoding
    //
    //  The de-interleaver output bit stream is de-punctured and then decoded using a
    //  soft-input Viterbi algorithm. The decoded bits are descrabled to restore the
    //  original message.
    //
    Nevada_Depuncture(pRX->N_SYM*pRX->N_DBPS, pRX->Lc+dc, pRX->cA+da, pRX->cB+da, pRX->RX_RATE, nMode);
    Nevada_VitDec    (Nbits, pRX->cA+da, pRX->cB+da, pRX->b+da, pRX);
    Nevada_Descramble(Nbits, pRX->b +da, pRX->a+da);
   
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Re-packet the SERVICE and PSDU fields
    //
    //  The SERVICE field is recorded in read-only registers for diagnostic purposes;
    //  this is the first 16-bits in array "a." The subsequent data bits are packed
    //  into bytes in the PHY_RX_D array starting at byte 8. The first 8 bytes contain
    //  the PHY header which is not part of the PSDU. The total number of bytes written
    //  to PHY_RX_D is recorded in N_PHY_RX_WR.
    //
    pRX->N_PHY_RX_WR = 8 + pRX->RX_LENGTH.w16; // number of bytes on PHY_RX_D

    pReg->RX_SERVICE.word = 0;
    for (k=0; k<16; k++, a++)
        pReg->RX_SERVICE.w16 = pReg->RX_SERVICE.w16 | (a->bit << k);

    for (i=8; i<pRX->N_PHY_RX_WR; i++) 
    {
        pRX->PHY_RX_D[i].word = 0;      
        for (k=0; k<8; k++, a++)
            pRX->PHY_RX_D[i].word = pRX->PHY_RX_D[i].w8 | (a->bit << k);
    }
     
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set RTL Execption Flags
    //
    //  The flags here are used to provide information for RTL verification tests.
    //  What is indicates is whether an OFDM packet has been decoded and the total
    //  number of decoded packets (limited to 3). Only data for the last packet is
    //  retained in PHY_RX_D.
    //
    pRX->RTL_Exception.DecodedOFDM = 1; // flag completion of OFDM decode
    pRX->RTL_Exception.PacketCount = min(3, pRX->RTL_Exception.PacketCount+1);

    return WI_SUCCESS;
}
// end of Nevada_RX2_DATA_Decode()

// ================================================================================================
// FUNCTION : Nevada_RX2()
// ------------------------------------------------------------------------------------------------
// Purpose  : 
// Timing   : This function runs at 20 MHz (ASIC contains faster clocks)
// ================================================================================================
wiStatus Nevada_RX2( Nevada_aSigState_t *aSigIn, Nevada_aSigState_t aSigOut, Nevada_CtrlState_t outReg,
                     Nevada_RxState_t *pRX, NevadaRegisters_t *pReg )
{
    if (outReg.State <= 1) pRX->RX2.kRise = pRX->RX2.kFall = 0; // reset kRise, kFall while in Sleep mode
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  RX2 Symbol Alignment
    //
    //  Symbol processing is synchronized with the PeakFound signal which indicates the
    //  boundary between the short and long training fields. 
    //
    //  x_ofs is the position of the FFT window for the first LTF preamble symbol.
    //
    //  kRX2 is the time at which the demodulator output is computed for the L-SIG symbol
    //  (legacy and mixed-mode format) or HT-SIG1 symbol (greenfield format) output.
    //
    if (aSigOut.PeakFound) 
    {
        pRX->x_ofs = pRX->k20 + NEVADA_FFT_OFFSET;
        pRX->RX2.kRX2 = pRX->x_ofs + NEVADA_FFT_DELAY + NEVADA_PLL_DELAY + 64 + 80 + 64;

        aSigIn->RxDone = 0;
        pRX->RX2.SigDemodDone = 0; // Distiguish Demod and Decode under the same state 11 in C code. Necessary for C code
        pRX->RX2.GfSig = 0;  
        pRX->RX2.kRise = 0;
        pRX->PacketType = 0; // OFDM
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  RX2 OFDM SYMBOL PROCESSING
    //
    //  The functions below are called at a symbol rate synchronous with the 20 MHz clock.
    //  The time index for the operation is stored in kRX2 and is updated after each
    //  processing step. Operations are also qualified with TurnOnOFDM which must be set
    //  when processing OFDM packets.
    //
    if ((pRX->k20 == pRX->RX2.kRX2) && outReg.TurnOnOFDM)
    {
        if (outReg.State == 11)
        {
            pRX->RX2.kFall = 0; // Reset kFall

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Process the SIGNAL symbol. This can either be L-SIG for a legacy or
            //  mixed-mode packet or HT-SIG for greenfield. In any case, all processing
            //  occurs during top-level state 11. 
            //  
            //  The flag SigDemodDone forms an internal state so that demodulation of the 
            //  first symbol occurs at one instance and subsequent decoding at a second 
            //  instance. In RTL, sequencing of these operations is handled with a
            //  controller not explicitly modeled here.
            //
            if (!pRX->RX2.SigDemodDone)
            {   
                //////////////////////////////////////////////////////////////////////////////
                //
                //  Demodulate the first SIGNAL symbol.
                //
                //  For a legacy or mixed-mode format, this is the L-SIG symbol; for greefield
                //  formats, it is the first of two HT-SIG symbols. The modulation is BPSK,
                //  but it is rotated 90 deg. for greenfield. The rotation check is examined
                //  to distinguish between greenfield and not-greenfield.
                //        
                XSTATUS( Nevada_RX2_SigDemod(aSigIn, pRX, pReg) );
                pRX->RX2.SigDemodDone = 1;             
                pRX->RX2.GfSig = (pReg->RXMode.b2 && pRX->Rotated6MbpsValid && pRX->Rotated6Mbps) ? 1 : 0;  // record the rotation checking after SigDemod
    
                // Note: when RXMode.b2 is 0, pRX->Rotated6MbpsValid should be 0. Checking RXMode.b2 to make sure
                if (pRX->RX2.GfSig) 
                {
                    pRX->CheckRotate =0;
                    pRX->RX2.kRX2 = pRX->x_ofs + 4*80 + NEVADA_HTSIG_LATENCY;  // time at which HT-SIG (GF) decoding must finish
                }
                else
                {
					// Rising edge of PHY_RX_EN based on Leo's timing chart
                    // ...from first FFTSS to rising of PHY_RX_EN 8.275ns = 662 (80M) = 165.5 (20 MHz)
					// For NEVADA_SIG_LATENCY = 70, kRise = kRX2;
                    pRX->RX2.kRise = pRX->kFFT[0] + 166;  // Legacy (none 6 Mbps rising) 
                    pRX->RX2.kRX2 = pRX->x_ofs + 3*80 + NEVADA_SIG_LATENCY; // time at which L-SIG decoding must finish     
				
                }
            }
            else 
            {
                if (!pRX->RX2.GfSig)
                {
                    //////////////////////////////////////////////////////////////////////////////
                    //
                    //  Decode L-SIG
                    //
                    //  After demodulating the first symbol, no rotation indicates the symbol is
                    //  L-SIG for either a legacy or mixed-mode packet. Decoding here occurs when
                    //  the output becomes valid in RTL.
                    //        
					unsigned int n;
                    XSTATUS( Nevada_RX2_SigDecode(aSigIn, pRX, pReg) );
		            n = 1 + (16 + 8*pRX->RX_LENGTH.w12 + 6 - 1) % pRX->N_DBPS; // number of bits in last symbol
                    pRX->RX2.nDone = n/4 + 36;    // excess clocks at 20 MHz  reference: RX timing doc
					// Advance the HtSigDemod1 time to match RTL PHY_RX_EN timing for 6 Mbps
					// 69 is the offset from Legacy PHY_RX_EN to 6 Mbps PHY_RX_EN;
                    pRX->RX2.kRX2 = pRX->kFFT[0] + 166 + 69;  					
                    pRX->RX2.nSym = 1;            // set symbol count to 1
                }
                else
                {
                    //////////////////////////////////////////////////////////////////////////////
                    //
                    //  Decode Greenfield HT-SIG
                    //
                    //  A rotation in the first SIGNAL symbol indicates the packet is greenfield
                    //  format. Demodulate the second HT-SIG symbol and decode the pair.
                    //  Decoding here occurs when the output becomes valid in RTL.
                    //        
					unsigned int n;
                    XSTATUS( Nevada_RX2_HtSigDemod2(pRX, pReg, 1) );
                    XSTATUS( Nevada_RX2_HtSigDecode(pRX, pReg, 1) ); 
					aSigIn->HeaderValid = pRX->HtHeaderValid;
					aSigIn->RxFault = pRX->HtRxFault;
                    if (pRX->HtHeaderValid)
                    {
                        // As we do not support GF data decoding, the rest of this block is not essential
                        n = 1 + (16 + 8*pRX->RX_LENGTH.w16 + 6 - 1) % pRX->N_DBPS; // number of bits in last symbol
                        pRX->RX2.nDone = n/4 + 36;     // excess clocks at 20 MHz
                        pRX->RX2.kRX2  = pRX->du + 64; // time to evaluate next
                        pRX->RX2.nSym  = 1;            // set symbol count to 1
                    }
                }
            }
        }
        
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Legacy 6 Mbps First DATA or Mixed-Mode HT-SIG Demodulation
        //  Evaluates when RTL gets the rotation check result on the symbol following
		//  ... L-SIG. It is the time to generate PHY_RX_EN for 6 Mbps
        //  The top state is not changed at this point     
        if (outReg.State == 20 && !pRX->Rotated6MbpsValid)
		{
			// Set top state 20->21 (20->13) transition time to match RTL. 
			pRX->RX2.kRX2 = pRX->du + 81; 

			// Call the Demod block
			XSTATUS( Nevada_RX2_HtSigDemod1(pRX, pReg) );
			
            if (pRX->Rotated6MbpsValid && !pRX->Rotated6Mbps)			  
			{
				// 6 Mbps PHY_RX_EN generation
				pRX->RX2.kRise = pRX->kFFT[0] + 166 + 69; 
				pRX->LegacyPacket = 1;
			}
		}
		/////////////////////////////////////////////////////////////////////////////////
        //
		//  Generate signal for RX top state transfer from 20->21 or 20->13
        //
		else if (outReg.State == 20)
        {
            pRX->RxRot6Mbps = pRX->Rotated6Mbps;
		    pRX->RxRot6MbpsValid = pRX->Rotated6MbpsValid;
            
            if (pRX->Rotated6MbpsValid && pRX->Rotated6Mbps)			          
				pRX->RX2.kRX2 = pRX->kFFT[0]+ 166 +163; // Time to call HT-SIG decoding and assert HT PHY_RX_EN. The top state is not changed until several clkslater
            else
            {               			
				pRX->RX2.kRX2 = pRX->du + 64;
                pRX->RX2.nSym = 2;  // second symbol of 6 Mbps     			
            } 	
        }

		////////////////////////////////////////////////////////////////////////////////////////
        //
        //  Mixed-Mode HT-SIG Decoding and HT PHY_RX_EN assertion
		//  Based on RTL knowledge that HT PHY_RX_EN asserted before top state 22-23 transition 
        //  To match RTL HT PHY_RX_EN timing, the HTSigDecode is called earlier than that
		//  in wise 3.10.6 and before  
		//		
        if (outReg.State == 22 && !pRX->HtHeaderValid && !pRX->HtRxFault)
        {                 
            XSTATUS( Nevada_RX2_HtSigDemod2(pRX, pReg, 0) );
            XSTATUS( Nevada_RX2_HtSigDecode(pRX, pReg, 0) );  
			pRX->RX2.kRise = pRX->RX2.kRX2; // HT PHY_RX_EN assertion time           
			pRX->RX2.kRX2 = pRX->x_ofs+ 5*80 + NEVADA_HTSIG_LATENCY;  // time for top state 22-23 transition            
        }
	
		/////////////////////////////////////////////////////////////////////////////////
		// 
		//	Update the HeaderValid signal in aSigIn for Top state 22-23 transition
		//  This is needed to match RTL state transition and aPacketDuration
		//  It is after WiSE HtSigDecode calling
        //
		else if (outReg.State == 22)
		{	
			aSigIn->HeaderValid = pRX->HtHeaderValid;
			aSigIn->RxFault = pRX->HtRxFault;
			pRX->RX2.kRX2 = pRX->du + 2*80; // time to evaluate next      // It reflex the time change in HtSigDecode 		
		}

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Mixed-Mode HT-LTF Processing
        //  
        //  Estimate the channel for HT processing using HT-LTF. This design supports
        //  only single spatial streams without beamforming, so the number of HT-LTF
        //  fields to process is only one.
        //        
        if (outReg.State == 23)
        {   
            unsigned n;
            XSTATUS( Nevada_RX2_HtChEst(aSigIn, pRX, pReg) ); 

            n = 1 + (16 + 8*pRX->RX_LENGTH.w16 + 6 - 1) % pRX->N_DBPS; // number of bits in last symbol
			
			if (pRX->RX_RATE.w16 == 0x7)
				pRX->RX2.nDone = n/4 + 29; // Viterbi decoding of 65 Mbps starts 7 clk earlier than other rates
			else
                pRX->RX2.nDone = n/4 + 36; // excess clocks at 20 MHz          
            pRX->RX2.kRX2  = pRX->du + 64; // time to evaluate next
            pRX->RX2.nSym  = 1;            // set symbol count to 1
        }
       
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Data Symbol Demodulation/Decoding
        //  
        if ((outReg.State >= 13) && (outReg.State <= 15) && pRX->dc) // State from 13 to 15 
        {            
            if (pRX->RX2.nSym < pRX->N_SYM) 
            {                
                XSTATUS( Nevada_RX2_DATA_Demod(pRX->RX2.nSym++, pRX, pReg) );
                pRX->RX2.kRX2 = pRX->du + 64;
            }
            else if (pRX->RX2.nSym == pRX->N_SYM) 
            {
                unsigned int n;
                XSTATUS( Nevada_RX2_DATA_Demod(pRX->RX2.nSym++, pRX, pReg) );
                
                n = pRX->RX_nMode ? (pRX->N_SYM + 4) : pRX->N_SYM;
                pRX->RX2.kRX2 = pRX->kFFT[n] + NEVADA_FFT_DELAY + NEVADA_PLL_DELAY + 80 + pRX->RX2.nDone;
				
                // RTL waveform timing (T = 1/20 MHz)
                //
                //   Last FFTSS to FFTEnB rising  144T, from FFTEnB  to PHY_RX_EN = 0 is 115.75T (259.75T total)
				//   Last FFTSS to last ChEstDS is 83T, from ChEstDS to PHY_RX_EN = 0 is 176.75T (259.75T total)
                //
				pRX->RX2.kFall = pRX->kFFT[n] + 259;
            }
            else  
            { 
                XSTATUS( Nevada_RX2_DATA_Decode(pRX->RX_nMode, pRX, pReg) );
                aSigIn->RxDone = 1;
                pRX->RX2.kRX2 = 0; // done processing: block further RX2 processing               
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  RX2 OFDM PHY_RX_EN Generation
    //
    //  Generates OFDM PHY_RX_EN at time defined by kRise and kFall
	//  HtRxFault, RxFault (in legacy cases) and Unsupported info are available at the 
    //  same time or earlier (6 Mbps) than kRise.
    //
    if ((pRX->k20) && outReg.TurnOnOFDM)
	{		
        if (pRX->k20 == pRX->RX2.kRise && (pRX->LegacyPacket || pRX->RX_nMode))
            aSigIn->RX_EN = 1;

		if ((pRX->k20 == pRX->RX2.kFall) || pRX->HtRxFault || aSigIn->RxFault || pRX->Unsupported) {
            aSigIn->RX_EN = 0;
            pRX->RX2.kRise = 0;
        }
	}

    ///////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Trace Signals
    //
    if (pRX->EnableTrace)
    {
        unsigned int k = pRX->k20;

        pRX->traceRX2[k].traceValid        = 1;
        pRX->traceRX2[k].CheckRotate       = pRX->CheckRotate;
        pRX->traceRX2[k].Rotated6Mbps      = pRX->Rotated6Mbps;
        pRX->traceRX2[k].Rotated6MbpsValid = pRX->Rotated6MbpsValid;
        pRX->traceRX2[k].Greenfield        = pRX->Greenfield;
        pRX->traceRX2[k].Unsupported       = pRX->Unsupported;
    }

    return WI_SUCCESS;
}
// end of Nevada_RX2()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------


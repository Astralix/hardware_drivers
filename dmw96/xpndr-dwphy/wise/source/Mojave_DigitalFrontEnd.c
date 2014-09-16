//--< Mojave_DigitalFrontEnd.c >-------------------------------------------------------------------
//=================================================================================================
//
//  Mojave - OFDM Digital Front End
//           
//  Developed by Younggyun Kim, Barrett Brickner, and Simon Chen
//  Copyright 2001-2003 Bermai, 2005-2006 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Mojave.h"
#include "Mojave_DigitalFrontEnd.h"
#include "wiUtil.h"
#include "wiType.h"

//=================================================================================================
//--  MACROS
//=================================================================================================
#define ClockRegister(arg) {(arg).Q=(arg).D;};
#define PI 3.14159265359

// ================================================================================================
// MACRO  : Base-2 Logarithm
// ================================================================================================
#ifndef log2
#define log2(x) (log((double)(x))/log(2.0))
#endif

// subcarrier types {0=null, 1=data, 2=pilot}
static int subcarrier_type[64] = {  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,    //  0-15
                                                1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,    // 16-31
                                                0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,    // 32-47
                                                1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };  // 48-63

// ================================================================================================
// FUNCTION  : aMojave_DFT_pilot
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement the recieve DFT for pilot tones
// Parameters: xR -- Receive data input, real component (15 bit circuit input)
//             xI -- Receive data input, imag component (15 bit circuit input)
//             pR -- Receive data output, real component (11 bit circuit output)
//             pI -- Receive data output, imag component (11 bit circuit output)
// ================================================================================================
void aMojave_DFT_pilot(wiWORD xR[], wiWORD xI[], wiWORD *pR, wiWORD *pI)
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

    for (n=0;n<64;n++)
    {
        a.word = xR[n].w15/1;
        b.word = xI[n].w15/1;

        a.word = a.word>2047 ? 2047 : a.word<-2047 ? -2047 : a.word;
        b.word = b.word>2047 ? 2047 : b.word<-2047 ? -2047 : b.word;

        c7.word  = (int)(cos(-2.0*PI*(double)n*7.0/64.0)*64.0);
        s7.word  = (int)(sin(-2.0*PI*(double)n*7.0/64.0)*64.0);
        c21.word = (int)(cos(-2.0*PI*(double)n*21.0/64.0)*64.0);
        s21.word = (int)(sin(-2.0*PI*(double)n*21.0/64.0)*64.0);

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

        jp7R.word  = ip7R.w21 >2047 ? 2047 : ip7R.w21 <-2047 ? -2047 : ip7R.w21;
        jm7R.word  = im7R.w21 >2047 ? 2047 : im7R.w21 <-2047 ? -2047 : im7R.w21;
        jm7I.word  = im7I.w21 >2047 ? 2047 : im7I.w21 <-2047 ? -2047 : im7I.w21;
        jp7I.word  = ip7I.w21 >2047 ? 2047 : ip7I.w21 <-2047 ? -2047 : ip7I.w21;
        jp21I.word = ip21I.w21>2047 ? 2047 : ip21I.w21<-2047 ? -2047 : ip21I.w21;
        jm21I.word = im21I.w21>2047 ? 2047 : im21I.w21<-2047 ? -2047 : im21I.w21;
        jm21R.word = im21R.w21>2047 ? 2047 : im21R.w21<-2047 ? -2047 : im21R.w21;
        jp21R.word = ip21R.w21>2047 ? 2047 : ip21R.w21<-2047 ? -2047 : ip21R.w21;

        accp7R.word  += jp7R.w12;
        accm7R.word  += jm7R.w12;
        accm7I.word  += jm7I.w12;
        accp7I.word  += jp7I.w12;
        accp21I.word += jp21I.w12;
        accm21I.word += jm21I.w12;
        accm21R.word += jm21R.w12;
        accp21R.word += jp21R.w12;
    }

    pR[0].word = accm21R.w18/16;  pR[0].word = pR[0].word>1023 ? 1023 : pR[0].word<-1023 ? -1023 : pR[0].word;
    pI[0].word = accm21I.w18/16;  pI[0].word = pI[0].word>1023 ? 1023 : pI[0].word<-1023 ? -1023 : pI[0].word;
    pR[1].word = accm7R.w18/16;   pR[1].word = pR[1].word>1023 ? 1023 : pR[1].word<-1023 ? -1023 : pR[1].word;
    pI[1].word = accm7I.w18/16;   pI[1].word = pI[1].word>1023 ? 1023 : pI[1].word<-1023 ? -1023 : pI[1].word;
    pR[2].word = accp7R.w18/16;   pR[2].word = pR[2].word>1023 ? 1023 : pR[2].word<-1023 ? -1023 : pR[2].word;
    pI[2].word = accp7I.w18/16;   pI[2].word = pI[2].word>1023 ? 1023 : pI[2].word<-1023 ? -1023 : pI[2].word;
    pR[3].word = accp21R.w18/16;  pR[3].word = pR[3].word>1023 ? 1023 : pR[3].word<-1023 ? -1023 : pR[3].word;
    pI[3].word = accp21I.w18/16;  pI[3].word = pI[3].word>1023 ? 1023 : pI[3].word<-1023 ? -1023 : pI[3].word;
}

// ================================================================================================
// FUNCTION  : aMojave_WeightedPilotPhaseTrack
// ------------------------------------------------------------------------------------------------
// Purpose   : Correct the phase error measured by the digital PLL
// Parameters: FInit   -- memory initialization
//             PathSel -- Active digital paths, 2 bits, (circuit input)
//             v0R-v1I -- Data path inputs, path 0-1, real/imag, 11 bit (circuit input)
//             h0R-u1I -- Estimated channel, path 0-1, real/imag, 10 bit (circuit input)
//             cSP     -- Correction for sampling phase error, 13 bit, (model input)
//             cCCP    -- Correction for carrier phase error, 12 bit, (model output)
//             pReg    -- programmable registers
// ================================================================================================
void aMojave_WeightedPilotPhaseTrack(int FInit, wiUWORD PathSel, 
                                     wiWORD v0R[], wiWORD v0I[], wiWORD v1R[], wiWORD v1I[],
                                     wiWORD h0R[], wiWORD h0I[], wiWORD h1R[], wiWORD h1I[],
                                     wiWORD cSP, wiWORD AdvDly, wiWORD *cCPP,MojaveRegisters *pReg)

{
    int k,m;

    static wiWORD mcI;      // integral path memory
    static wiWORD mcVCO;   // VCO memory
    wiWORD P;
    wiWORD t;
    double a;
    wiWORD cR,cI;
    int n;

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

    wiUWORD aC,bC,cC;      // PLL gains

    static int pilot_tones[4] = {11, 25, 39, 53};
    static int Shift_Reg;
    int D;

    static wiReg cSP_dly;   // a delay for the cSP input
    wiWORD delayline;      // sampling phase correction due to window shift

    // initialize memories
    if (FInit)
    {
        mcVCO.w18 = 0;
        mcI.w17 = 0;
        Shift_Reg = 0x7f;
        cSP_dly.Q.word = 0;
    }

    cSP_dly.D.word = cSP.w13;

    // decision for pilot tones
    D = ((Shift_Reg>>3) & 0x01) ^ ((Shift_Reg>>6) & 0x01);
    Shift_Reg = ((Shift_Reg<<1) & 0x7e) | D;

    D = D==1 ? -1 : 1;

    // ----------------------------------------------
    // sampling phase correction due to window shift
    // ----------------------------------------------
         if (AdvDly.w2== 1) delayline.word = 102944*2/64;
    else if (AdvDly.w2==-1) delayline.word =-102944*2/64;
    else                   delayline.word = 0;

    // phase error estimation
    accC.word = 0;
    sR.word = sI.word = 0;
    for (m=0; m<4; m++)
    {
        k=pilot_tones[m];

        // phase correction
        t.word = (cSP_dly.Q.w13+delayline.w13)*(k-32)/64 + mcVCO.w18/64;
        t.word = t.word>1608 ? t.word-3217 : t.word<-1608 ? t.word+3217 : t.word;
        t.word = t.word/4;

        // --------------------------
        // Trigonometric Lookup Table
        // --------------------------
        a = (double)t.w10/128.0;
        cR.word=(int)(512.0 * cos(a));
        cI.word=(int)(512.0 * sin(a));

        // --------------------
        // Phase Correction
        // --------------------
        x0R.word = PathSel.b0? (v0R[m].w11*cR.w11 + v0I[m].w11*cI.w11)/1024 : 0;
        x0I.word = PathSel.b0? (v0I[m].w11*cR.w11 - v0R[m].w11*cI.w11)/1024 : 0;
        x1R.word = PathSel.b1? (v1R[m].w11*cR.w11 + v1I[m].w11*cI.w11)/1024 : 0;
        x1I.word = PathSel.b1? (v1I[m].w11*cR.w11 - v1R[m].w11*cI.w11)/1024 : 0;

        // -----------------------
        // Limit output to 10-bits
        // -----------------------
        u0R.word = (abs(x0R.w11)<512)? x0R.w11 : (x0R.b10? -511:511);
        u0I.word = (abs(x0I.w11)<512)? x0I.w11 : (x0I.b10? -511:511);
        u1R.word = (abs(x1R.w11)<512)? x1R.w11 : (x1R.b10? -511:511);
        u1I.word = (abs(x1I.w11)<512)? x1I.w11 : (x1I.b10? -511:511);

        // equalization
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
        if (m==3)
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
    if (sR.w20<=0) angle.word=0;
    else 
    {
        angle.word = (sI.w20<<9) / sR.w20;
        angle.word = (abs(angle.word)<512)? angle.w10 : (angle.word<0)? -511:511;
    }
    accC.word = angle.w10;

    // -------------------------------------------------
    // Extract loop gain parameters from register memory
    // -------------------------------------------------
    aC.word = 1 << (pReg->aC.w3);
    bC.word = 1 << (pReg->bC.w3);
    cC.word = 1 << (pReg->cC.w3);

    // phase correction output
    if (pReg->PilotPLLEn.b0)
        cCPP->word = mcVCO.w18 + accC.w10*64/cC.w5;
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
    if (bC.w8) mcI.word=accC.w10*64/bC.w8 + mcI.w17;
    mcI.word = mcI.w18>65535 ? 65535 : mcI.w18<-65535 ? -65535 : mcI.w18;

    // VCO
    mcVCO.word = mcVCO.w18 + P.w16 + mcI.w17;
    if (abs(mcVCO.w19) > 102944)
    {
        mcVCO.word = mcVCO.w19> 102944 ? mcVCO.w19-102944*2 
                 : mcVCO.w19<-102944 ? mcVCO.w19+102944*2 : mcVCO.w19;
    }
    cSP_dly.Q.word = cSP_dly.D.w13;
}

// ================================================================================================
// FUNCTION  : aMojave_PhaseCorrect
// ------------------------------------------------------------------------------------------------
// Purpose   : Correct the phase error measured by the digital PLL
// Parameters: PathSel -- Active digital paths, 2 bits, (circuit input)
//             Nk      -- Number of 20 MHz clock periods to operate (model input)
//             cSP     -- Correction for sampling phase error, 13 bit, (model input)
//             cCP     -- Correction for carrier phase error, 12 bit, (model input)
//             v0R-v1I -- Data path inputs, path 0-1, real/imag, 11 bit (circuit input)
//             u0R-u1I -- Data path outputs, path 0-1, real/imag, 10 bit (circuit output)
// Developed : by Younggyun Kim; translated to aMojave format by B. Brickner
// ================================================================================================
void aMojave_PhaseCorrect(wiUWORD PathSel, int Nk, wiWORD cSP, wiWORD cCP, wiWORD cCPP, Mojave_RX_State *pRX,
                         wiWORD *v0R, wiWORD *v0I, wiWORD *v1R, wiWORD *v1I,
                         wiWORD *u0R, wiWORD *u0I, wiWORD *u1R, wiWORD *u1I)
{
    wiWORD x0R, x0I, x1R, x1I;
    wiWORD t,cR,cI;
    double a; int k;

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
        if ((k>5) && (k<59))
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
// end of aMojave_PhaseCorrect()

// ================================================================================================
// FUNCTION  : aMojave_PLL
// ------------------------------------------------------------------------------------------------
// Purpose   : Digital phase locked loop
// Parameters: 
// Developed : by Younggyun Kim; translated to aMojave format by B. Brickner
// ================================================================================================
void aMojave_PLL(wiWORD PLL_ZR[], wiWORD PLL_ZI[], wiWORD PLL_H2[], wiWORD PLL_DR[], wiWORD PLL_DI[], 
                int iFlag, wiWORD *PLL_AdvDly,   wiWORD *cSP, wiWORD *cCP, Mojave_RX_State *pRX, MojaveRegisters *pReg)
{
    static wiWORD w[64], wc[64], ws[64]; // weight storage
    static wiWORD sumwc, sumws; // accumulator storage
    static wiUWORD sym_cnt;     // symbol counter

    wiUWORD aCGear,bCGear; // loop gain for carrier loop
    wiUWORD aSGear,bSGear; // loop gain for sampling phase locked loop
    
    wiWORD eSP;   // estimate of sampling phase error
    wiWORD eCP;   // estimate of carrier phase error

    wiWORD zR,zI; // in- and quadrature-phase phase-equalized signal, for phase error calculation
    wiWORD tR,tI;
    wiWORD angle; // angle of individual carrier
    wiWORD accC;  // accumulated carrier phase error
    wiWORD accS;  // accumulated sampling phase error

    int wAddr;
    int n, k;

    // --------------
    // Initialization
    // --------------
    if (iFlag==1)
    {
        cSP->word=0;
        cCP->word=0;
        sym_cnt.word=0;
        aCGear.word=1;
        bCGear.word=1;
        aSGear.word=1;
        bSGear.word=1;
    }
    // ----------------------------------------------------
    // Compute and store weights for phase error estimation
    // ----------------------------------------------------
    if (sym_cnt.w4==0) // occurs during preamble L2 symbol
    {
        wAddr = 0;
        for (k=0; k<64; k++)
        {
            if (subcarrier_type[k])
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
    if (sym_cnt.w4==0) // occurs during SIGNAL symbol
    {
        sumwc.word = 0;
        sumws.word = 0;
        for (k=0; k<26; k++)
        {
            wc[k].word = wc[51-k].word = (w[k].word>w[51-k].word)? w[51-k].word : w[k].word;
            ws[k].word = ws[k+26].word = (w[k].word>w[k+26].word)? w[k+26].word : w[k].word;
        }
        for (k=0; k<26; k++)
        {
            sumwc.word += wc[k].word;
            sumws.word += ws[k].word;
        }
        n = (int)(log2(abs(sumwc.word)>>4) + 1E-9); sumwc.word = (sumwc.word/(1<<n))*(1<<n);
        n = (int)(log2(abs(sumws.word)>>4) + 1E-9); sumws.word = (sumws.word/(1<<n))*(1<<n);
    }
    // -------------------------------------------------
    // Extract loop gain parameters from register memory
    // -------------------------------------------------
    aCGear.word = 1 << (pReg->Ca_Cb[sym_cnt.w4].w6>>3);
    bCGear.word = 1 << (pReg->Ca_Cb[sym_cnt.w4].w3   );
    aSGear.word = 1 << (pReg->Sa_Sb[sym_cnt.w4].w6>>3);
    bSGear.word = 1 << (pReg->Sa_Sb[sym_cnt.w4].w3   );

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
        if (subcarrier_type[k]) // use pilot and data subcarriers
        {
            //------------------------------------------------------------
            // remove the phase of the input signal
            //------------------------------------------------------------
            tR.word = (zR.w11*PLL_DR[k].w5) + (zI.w11*PLL_DI[k].w5);
            tI.word = (zI.w11*PLL_DR[k].w5) - (zR.w11*PLL_DI[k].w5);

            //---------------------------------------------------
            // significant bits: reduce to three significant bits
            //---------------------------------------------------
            n = (int)(log2(abs(tR.w16)>>2) + 1E-9); tR.w16 = (tR.w16/(1<<n))*(1<<n);
            n = (int)(log2(abs(tI.w16)>>2) + 1E-9); tI.w16 = (tI.w16/(1<<n))*(1<<n);

            //-------------------------------------------------------------
            // calculate phase error of each sub-carrier
            //-------------------------------------------------------------
            if (tR.w16<=0) angle.word=0;
            else 
            {
                angle.word = (tI.w16<<9) / tR.w16;
                angle.word = (abs(angle.word)<512)? angle.w10 : (angle.word<0)? -511:511;
            }
            // ----------------------------------------
            // accumulators for carrier and phase error
            // ----------------------------------------
            accC.word += angle.w10 * wc[wAddr].word/8;
            accS.word += angle.w10 * ws[wAddr].word/8 * (wAddr<26 ? -1:1);

            wAddr++;
        }
    }
    //---------------------------------------------------------
    // estimation of phase errors
    //---------------------------------------------------------
    accC.word =  4 * accC.word;
    accS.word = 16 * accS.word; // 64*8/27;

    //-------------------------------------------------------------
    // significant bits
    //-------------------------------------------------------------
    n = (int)(log2(abs(accC.word)>>4) + 1E-9);  accC.word = (accC.word/(1<<n))*(1<<n);
    n = (int)(log2(abs(accS.word)>>4) + 1E-9);  accS.word = (accS.word/(1<<n))*(1<<n);

    eCP.word = accC.word/(sumwc.word? sumwc.word:1);
    eCP.word = (abs(eCP.word)<512)? eCP.w10 : (eCP.word<0)? -511:511;

    eSP.word = accS.word/(sumws.word? sumws.word:1);
    eSP.word = (abs(eSP.word)<512)? eSP.w10 : (eSP.word<0)? -511:511;

    // --------------------------------------------------
    // Loop filter and VCO for sampling phase locked loop
    // --------------------------------------------------
    {
        static wiWORD mSI;       // accumulator for intergral path, sampling phase
        static wiWORD mSVCO;     // accumulator for VCO, sampling phase
        static wiWORD delayline; // phase adjustment due to the advance/delay signal
        static int counter;      // time past from the last update of delay/advance value
        wiWORD     P;            // proportional path output

        // initialize memories
        if (iFlag==1) 
        {
            mSI.word=0; 
            mSVCO.word=0;       
            counter=0;
            delayline.word=0;
        }

        // increase counter value
        if (counter)
            counter++;

        // loop filtering
        P.word   = (aSGear.w4==0)? 0 : eSP.w10*64/aSGear.w5;
        if (bSGear.w7) mSI.word = eSP.w10*64/bSGear.w8 + mSI.w16;
        mSI.word = (mSI.w17>32767)? 32767 : mSI.w17<-32767 ? -32767 : mSI.w17;

        // VCO
        if (counter==2)
        {
            mSVCO.word=P.w16+mSI.w16+mSVCO.w19+delayline.w19;
            delayline.word=0;
        }
        else
            mSVCO.word=P.w16+mSI.w16+mSVCO.w19;

        // enable wrapping-around
        if (counter==15) counter = 0;

        // generate advance/delay indicator
        if (mSVCO.w23>102944 && counter==0)         // delay the window by one sample
        {
            PLL_AdvDly->word = -1;
            delayline.word=-102944*2;
            counter=1;
        }
        else if (mSVCO.w23<-102944 && counter==0)      // advance the window by one sample
        {
            PLL_AdvDly->word = 1;
            delayline.word=102944*2;
            counter=1;
        }
        else
            PLL_AdvDly->word = 0;

        // phase error to correct
        cSP->word = mSVCO.w19/64;
    }

    // -----------------------------------------------------
    // Loop filter and VCO for carrier phase locked loop
    // -----------------------------------------------------
    {
        static wiWORD   mCI;    // accumulator for intergral path, carrier phase
        static wiWORD   mCVCO; // accumulator for VCO, carrier phase
        wiWORD P;             // proportional path output

        // initialize memories
        if (iFlag==1) 
        {
            mCI.word=0; 
            mCVCO.word=0;       
        }
        // loop filtering
        P.word   = (aCGear.w4==0)? 0 : eCP.w10*64/aCGear.w5;
        if (bCGear.w7) mCI.word=eCP.w10*64/bCGear.w8 + mCI.w17;
        mCI.word = mCI.w18>65535 ? 65535 : mCI.w18<-65535 ? -65535 : mCI.w18;

        // VCO
        mCVCO.word = mCVCO.w18 + P.w16 + mCI.w17;
        if (abs(mCVCO.w19) > 102944)
        {
            pRX->EventFlag.b7 = 1;
            mCVCO.word = mCVCO.w19>102944 ? mCVCO.w19-102944*2 : mCVCO.w19<-102944 ? mCVCO.w19+102944*2 : mCVCO.w19;
        }

        // phase error to correct
        cCP->word = mCVCO.w18/64;
    }
    // ------------------------------
    // Increment Symbol Counter to 15
    // ------------------------------
    if (sym_cnt.w4<15) sym_cnt.word = sym_cnt.w4 + 1;
}
// end of aMojave_PLL()

// ================================================================================================
// FUNCTION  : aMojave_CFO_Angle
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the angle for a cartesian coordinate
// Parameters: 
// ================================================================================================
int aMojave_CFO_Angle(wiWORD b, wiWORD a)
{
    wiUWORD x, y, z, yz; wiWORD phi;

    // -------------------------------------------------
    // Handle undefined and angles at 90degree intervals
    // -------------------------------------------------
    if (!a.word && !b.word) return 0;            // undefined
    if (!a.word) return (b.word<0)? -2048:2048;  // -/+ pi/2
    if (!b.word) return (a.word<0)? -4095:0;     // -/+ pi

    // -----------------------------------------
    // Reduce lookup table range to 0<= y/x <= 1
    // -----------------------------------------
    if (abs(a.word) >= abs(b.word))
    {
        x.word = abs(a.word); x.word = x.w22;
        y.word = abs(b.word); y.word = y.w22;
    }
    else
    {
        x.word = abs(b.word); x.word = x.w22;
        y.word = abs(a.word); y.word = y.w22;
    }
    // -------------------------------------
    // Reduce resolution for arctan argument
    // -------------------------------------
    while (x.word >= 512)
    {
        x.word = x.word >> 1;
        y.word = y.word >> 1;
    }
    while (x.word < 256)
    {
        x.word = x.word << 1;
        y.word = y.word << 1;
    }
    // -------------------------------
    // Compute the arctangent of (y/x)
    // -------------------------------
    z.word   = (int)((1<<18)/((double)(x.word<<1)+1)+0.5);
    yz.word  = (y.w9 * z.w9) >> 7;
    phi.word = (int)((4096/PI)*atan(yz.word/1024.0) + 0.5);

    // -------------------------------------------------
    // Convert 45 degree segement into full circle angle
    // -------------------------------------------------
    if (abs(a.word) < abs(b.word)) phi.word = 2048 - phi.w13;
    if (a.word<0 && b.word>0)      phi.word = 4096 - phi.w13;
    else if (a.word<0 && b.word<0) phi.word = 4096 + phi.w13;
    else if (a.word>0 && b.word<0) phi.word = 8192 - phi.w13;

    return (phi.w13);
}
// end of aMojave_CFO_Angle()

// ================================================================================================
// FUNCTION  : aMojave_CFO
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate and correct the carrier frequency offset
// Parameters: CFOPathSel -- Select the active digital paths (2 bit input)
//             SigLen     -- Length of input/output arrays (model input)
//             sCFO       -- Start CFO index
//             peak       -- Delayed end of short preamble index
//             CFODI0R    -- Signal input, path 0, real component (12 bit input)
//             CFODI0I    -- Signal input, path 0, imag component (12 bit input)
//             CFODI1R    -- Signal input, path 1, real component (12 bit input)
//             CFODI1I    -- Signal input, path 1, imag component (12 bit input)
//             CFODO0R    -- Signal output, path 0, real component (15 bit output)
//             CFODO0I    -- Signal output, path 0, imag component (15 bit output)
//             CFODO1R    -- Signal output, path 1, real component (15 bit output)
//             CFODO1I    -- Signal output, path 1, imag component (15 bit output)
// ================================================================================================
void aMojave_CFO(wiUWORD CFOEnB, wiUWORD CFOPathSel, wiUWORD CFOStart, wiUWORD CFOEnd,
                wiWORD  CFODI0R, wiWORD  CFODI0I, wiWORD  CFODI1R, wiWORD  CFODI1I,
                wiWORD *CFODO0R, wiWORD *CFODO0I, wiWORD *CFODO1R, wiWORD *CFODO1I,
                Mojave_RX_State *pRX, MojaveRegisters *pReg)
{
    static wiWORD axR,axI;         // correlator output
    static wiWORD AcmAxR,AcmAxI;   // accumuator output
    static wiWORD anginR,anginI;   // input to angle operator
    static wiWORD phi;            // output from angle operator, unit pi
    static wiWORD cR,cI;         // CFO correction terms
    static int     i,j,k;
    static double angle;

    static wiUReg state, counter;
    static wiReg  theta, step;
    static wiReg  s0R, s0I, s1R, s1I;
    static wiUReg startCFO, endCFO;

    static wiWORD w0R[80], w0I[80], w1R[80], w1I[80]; // RAM for delay and lag
    static wiReg  x0R[2], x0I[2], x1R[2], x1I[2];
    wiWORD y0R, y0I, y1R, y1I;

    static unsigned int debug_nT; // counter for debug

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(s0R); s0R.D.word = CFOEnB.b0? 0:CFODI0R.w12;
    ClockRegister(s0I); s0I.D.word = CFOEnB.b0? 0:CFODI0I.w12;
    ClockRegister(s1R); s1R.D.word = CFOEnB.b0? 0:CFODI1R.w12;
    ClockRegister(s1I); s1I.D.word = CFOEnB.b0? 0:CFODI1I.w12;

    for (i=0; i<2; i++)
    {
        ClockRegister(x0R[i]); ClockRegister(x0I[i]);
        ClockRegister(x1R[i]); ClockRegister(x1I[i]);
    }
    ClockRegister(startCFO);
    ClockRegister(endCFO);
    ClockRegister(step);
    ClockRegister(theta);
    ClockRegister(state);
    ClockRegister(counter); counter.D.word = counter.Q.word + 1;
    k = (k+1) % 80;

    // -----------
    // wire inputs
    // -----------
    startCFO.D = CFOStart;
    endCFO.D   = CFOEnd;

    // ------------
    // Wire Outputs
    // ------------
    *CFODO0R = x0R[0].Q;
    *CFODO0I = x0I[0].Q;
    *CFODO1R = x1R[0].Q;
    *CFODO1I = x1I[0].Q;

    angle   = (PI/4096.0) * ((double)(theta.Q.w17>>4));

    cR.word = (int)floor(511*cos(angle) + 0.5);
    cI.word = (int)floor(511*sin(angle) + 0.5);

    // --------------------------------------------------
    // TEST FEATURE: Disable CFO Correction (not in ASIC)
    // --------------------------------------------------
    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisableCFO) {cR.word = 511; cI.word = 0;}

    // --------------------
    // Apply CFO correction
    // --------------------
    x0R[0].D.word=(s0R.Q.w12*cR.word + s0I.Q.w12*cI.word)/256; // result is 13.5 bits
    x0I[0].D.word=(s0I.Q.w12*cR.word - s0R.Q.w12*cI.word)/256;
    x1R[0].D.word=(s1R.Q.w12*cR.word + s1I.Q.w12*cI.word)/256;
    x1I[0].D.word=(s1I.Q.w12*cR.word - s1R.Q.w12*cI.word)/256;

    if (CFOEnB.b0)
    {
        phi.word = 0;
        AcmAxR.word=0;
        AcmAxI.word=0;

        theta.D.word = 0;
        theta.Q.word = 0;
        step.D.word  = 0;
        step.Q.word  = 0;

        state.D.word = 0;
        state.Q.word = 0;
        counter.D.word = 0;
        k = 0;
        return;
    }
    // ------------------------
    // Controller State Machine
    // ------------------------
    switch (state.Q.word)
    {
        case 0:
            state.D.word = 1;
            debug_nT = 0;
        case 1:
            if (startCFO.D.b0)
            {
                counter.D.word = 0;
                state.D.word = 21;
            }
            break;
        case 21:
            if (endCFO.Q.b0)
            {
                counter.D.word = 0;
                state.D.word = 41;
            }
            else
            {
                state.D.word = 2; counter.D.word=0;
            }
            break;
        case 2:
            if (endCFO.Q.b0)
            {
                counter.D.word = 0;
                state.D.word = 41;
            }
            else if (counter.Q.word == (unsigned)(pReg->CFOFilter.b0? 31:30))
            {
                counter.D.word = 0;
                state.D.word = 3;
            }
            break;
        case 3:
            if (endCFO.Q.b0)
            {
                counter.D.word = 0;
                state.D.word = 41;
            }
            break;

        case 4:  state.D.word = 41; break;
        case 41: state.D.word = 5; break;
        case 5:  state.D.word = 6; break;

        case 6:
            if (startCFO.D.b0)
            {
                counter.D.word = 0;
                state.D.word = 7;
            }
            break;
        case 7:
            if (counter.Q.word == (unsigned)(pReg->CFOFilter.b0? 63:62))
            {
                counter.D.word = 0;
                state.D.word = 8;
            }                                                                                
            break;
        case 8:
            if (counter.Q.word == (unsigned)(pReg->CFOFilter.b0? 62:63))
            {
                counter.D.word = 0;
                state.D.word = 9;
            }
            break;

        case 9 : state.D.word = 10; break;
        case 10: state.D.word = 11; break;
        case 11: break;

        default: state.D.word = 0;
    }

    for (i=1; i<2; i++)
    {
        x0R[1].D = x0R[0].Q;
        x0I[1].D = x0I[0].Q;
        x1R[1].D = x1R[0].Q;
        x1I[1].D = x1I[0].Q;
    }
    // -------------------------------------------------
    // Generate low-resolution sample for CFO estimation
    // -------------------------------------------------
    y0R.word = pReg->CFOFilter.b0? (x0R[0].D.w14>>1)-(x0R[1].Q.w14>>1) : x0R[0].D.w14;
    y0I.word = pReg->CFOFilter.b0? (x0I[0].D.w14>>1)-(x0I[1].Q.w14>>1) : x0I[0].D.w14;
    y1R.word = pReg->CFOFilter.b0? (x1R[0].D.w14>>1)-(x1R[1].Q.w14>>1) : x1R[0].D.w14;
    y1I.word = pReg->CFOFilter.b0? (x1I[0].D.w14>>1)-(x1I[1].Q.w14>>1) : x1I[0].D.w14;

    w0R[k].word = (abs(y0R.w14)<1024)? (y0R.w11/8) : ((y0R.w14<0)? -127:127);
    w0I[k].word = (abs(y0I.w14)<1024)? (y0I.w11/8) : ((y0I.w14<0)? -127:127);
    w1R[k].word = (abs(y1R.w14)<1024)? (y1R.w11/8) : ((y1R.w14<0)? -127:127);
    w1I[k].word = (abs(y1I.w14)<1024)? (y1I.w11/8) : ((y1I.w14<0)? -127:127);

    // ---------------------
    // Coarse CFO Estimation
    // ---------------------
    if (state.Q.word==3 || state.Q.word==4)
    {
        i = (k + 80 - 32     ) % 80;
        j = (k + 80 - 32 - 16) % 80;

        axR.word = (w0R[i].w8 * w0R[j].w8) + (w0I[i].w8 * w0I[j].w8);         
        axI.word = (w0I[i].w8 * w0R[j].w8) - (w0R[i].w8 * w0I[j].w8);
        axR.word+= (w1R[i].w8 * w1R[j].w8) + (w1I[i].w8 * w1I[j].w8);
        axI.word+= (w1I[i].w8 * w1R[j].w8) - (w1R[i].w8 * w1I[j].w8);

        if (CFOPathSel.w2 == 3)
        {
            axR.word = axR.word / 2;
            axI.word = axI.word / 2;
        }
        if (!pRX->NonRTL.Enabled || (debug_nT<pRX->NonRTL.MaxSamples_CFO)) // if (1) in RTL
        {                                                                 
            AcmAxR.word = AcmAxR.w23 + axR.w16;
            AcmAxI.word = AcmAxI.w23 + axI.w16;
            debug_nT++; //[WiSE Only]
        }
    }
    // -------------------
    // Fine CFO Estimation
    // -------------------
    if (state.Q.word==8)
    {      
        i = (k + 80     ) % 80;
        j = (k + 80 - 64) % 80;

        axR.word = (w0R[i].w8 * w0R[j].w8) + (w0I[i].w8 * w0I[j].w8);         
        axI.word = (w0I[i].w8 * w0R[j].w8) - (w0R[i].w8 * w0I[j].w8);
        axR.word+= (w1R[i].w8 * w1R[j].w8) + (w1I[i].w8 * w1I[j].w8);
        axI.word+= (w1I[i].w8 * w1R[j].w8) - (w1R[i].w8 * w1I[j].w8);

        if (CFOPathSel.w2 == 3)
        {
            axR.word = axR.word / 2;
            axI.word = axI.word / 2;
        }
        AcmAxR.word = AcmAxR.w23 + axR.w16;
        AcmAxI.word = AcmAxI.w23 + axI.w16;
    }
    // ------------------------- 
    // Compute Coarse Correction
    // ------------------------- 
    if (state.Q.word==5)
    {
        anginR.word = AcmAxR.w23;
        anginI.word = AcmAxI.w23;

        // ---------------
        // Calculate Angle
        // ---------------
        phi.word   = aMojave_CFO_Angle(anginI, anginR);

        theta.D.word = 2 * phi.w13;
        step.D.word  = phi.w13;

        AcmAxR.word=0;
        AcmAxI.word=0;

        pRX->EVM.cCFO  = PI*(double)phi.word/4096.0; // store CFO for simulation purposes
        pRX->EVM.NcCFO = debug_nT;                  // store CFO estimator sample size
        if (pRX->TraceKeyOp) wiPrintf("[NcCFO=%2d, k20=%3d, phi=%4d]",debug_nT,pRX->k20,phi.word);
    }
    // -----------------------
    // Compute Fine Correction
    // -----------------------
    else if (state.Q.word==10)
    {
        anginR.word=AcmAxR.w23;
        anginI.word=AcmAxI.w23;

        // ---------------
        // calculate angle
        // ---------------
        phi.word     = aMojave_CFO_Angle(anginI, anginR)/4;
        theta.D.word = theta.Q.w17 + step.Q.word + (64 + 3) * (phi.w13);
        step.D.word  = step.Q.w17 + phi.w13;

        AcmAxR.word=0;
        AcmAxI.word=0;

        pRX->CFO.word = abs(step.D.w14)<2048? (step.D.w14>>4) : (step.D.b13? -127:127); // store 8-bit version of estimate
        pRX->EVM.fCFO= PI*(double)phi.word/4096.0; // store CFO for simulation purposes
    }
    else
    {
        step.D = step.Q;
        theta.D.word = theta.Q.w17 + step.Q.w17;
    }
}
// end of aMojave_CFO()

// ================================================================================================
// FUNCTION  : aMojave_FS_UIntToFloat
// ------------------------------------------------------------------------------------------------
// Purpose   : Convert an unsigned integer to floating point
// Parameters: 
// ================================================================================================
void aMojave_FS_UIntToFloat(wiUWORD *significand, wiWORD *exponent, unsigned int t)
{
    int i;
    if (t>1023)
    {
        for (i=0; t>1023; i++) t=t>>1;
        significand->word = t;
        exponent->word = i;
    }
    else if (t>0)
    {
        for (i=1; t<1024; i--) t=t<<1;
        significand->word = t>>1;
        exponent->word = i;
    }
    else
    {
        significand->word = 0;
        exponent->word = -10;
    }
}
// end of aMojave_FS_UIntToFloat() 

// ================================================================================================
// FUNCTION  : aMojave_FrameSync
// ------------------------------------------------------------------------------------------------
// Purpose   : Synchronize the receiver with the OFDM symbol frame
// Parameters: FSEnB         -- Enable the circuit, active low (input)
//             FSPathSel     -- Select the active digital paths (2 bit input)
//             FSAGCDone     -- Flag to indicate gain updates have completed (1 bit input)
//             FSPeakSearch  -- Enables the peak search algorithm (1 bit input)
//             FSDI0R        -- Signal input, path 0, real component (13->10 bit input)
//             FSDI0I        -- Signal input, path 0, imag component (13->10 bit input)
//             FSDI1R        -- Signal input, path 1, real component (13->10 bit input)
//             FSDI1I        -- Signal input, path 1, imag component (13->10 bit input)
//             FSPeakFound   -- Indicates the peak has been located (1 bit output)
//             FSSigDetValid -- Indicates the signal detect is valid (1 bit ouput)
//             pReg          -- Pointer to modeled register file (model input)
// ================================================================================================
void aMojave_FrameSync(wiUWORD FSEnB, wiUWORD FSPathSel, wiUWORD FSAGCDone, wiUWORD FSPeakSearch,
                      wiWORD FSDI0R, wiWORD FSDI0I, wiWORD FSDI1R, wiWORD FSDI1I, wiUWORD *FSPeakFound, 
                      wiUWORD *FSSigDetValid, Mojave_RX_State *pRX, MojaveRegisters *pReg)
{
    static wiUWORD  tpeak;      // peak location, temporary
    static wiWORD   mem0R[176]; // real, delay line, first path
    static wiWORD   mem0I[176]; // imaginary, delay line, first path
    static wiWORD   mem1R[176]; // real, delay line, second path
    static wiWORD   mem2I[176]; // imaginary, delay line, second path
    wiWORD          x0R;        // real, received signal, first path
    wiWORD          x0I;        // imaginary, received signal, first path
    wiWORD          x1R;        // real, received signal, second path
    wiWORD          x1I;        // imaginary, received signal, second path
    wiWORD          y0R, y0I, y1R, y1I;
    wiWORD          ax1R,ax1I,ax2R,ax2I;  // correlator output
    wiWORD          pw1,pw2,pw3,pw4;      // squared magnitude of signal

    wiUWORD AxRSig,AxISig; wiWORD AxRExp,AxIExp; // significand and exponent of Ax?Acm
    wiUWORD Pw1Sig,Pw2Sig; wiWORD Pw1Exp,Pw2Exp; // significand and exponent of Pw?Acm
    wiUWORD Ax1Sig,Ax2Sig; wiWORD Ax1Exp,Ax2Exp; // significand and exponent of |Ax?|^2

    wiWORD            PwSigExp;   // exponent for the product of Pw1Sig * Pw2Sig
    wiWORD            RatioSigExp;// exponent for 

    wiWORD  sumSig;
    wiUWORD ThSig;   wiWORD ThExp;   // locally formatted version for SyncThSig and SyncThExp

    static int    A0, A1, A2, A3;         // address counters
    static wiUReg State;                  // controller state
    static wiReg  w0R[3], w0I[3], w1R[3], w1I[3];     // signal input registers
    wiUWORD AcmClrB, AcmEnB1, AcmEnB2;    // power accumulator control signals
    wiBoolean RatioValid;                 // ratio (metric) is valid

    static wiReg  AcmAxR, AcmAxI;         // autocorrelation accumulator
    static wiReg  AcmPw0, AcmPw1;         // power measurement accumulator
    static wiUReg PeakFound, SigDetValid; // peak found, signal detection valid
    static wiUReg SigDet;                 // signal detected (OFDM carrier sense)
    static wiUReg EnB, PathSel;
    static wiUReg AGCDone, PeakSearch;
    int i;

    // ------------------------------
    // Floating Point Value Registers
    // ------------------------------
    static wiReg  AxExp;    static wiUReg AxSig;   // autocorrelation term in floating point (numerator)
    static wiReg  PwExp;    static wiUReg PwSig;   // power measurement product in floating point (denominator)
    static wiReg  RatioExp; static wiUReg RatioSig;// test ratio of autocorrelation to power
    static wiReg  MaxExp;   static wiUReg MaxSig;  // maximum measured ratio

    // ---------------
    // Input Registers
    // ---------------
    ClockRegister(EnB);        EnB.D = FSEnB;
    ClockRegister(PathSel);    PathSel.D = FSPathSel;
    ClockRegister(AGCDone);    AGCDone.D = FSAGCDone;
    ClockRegister(PeakSearch); PeakSearch.D = FSPeakSearch;

    w0R[0].Q.word = EnB.Q.b0? 0:w0R[0].D.w10;  w0R[0].D.word = max(-511,min(511,FSDI0R.w12));
    w0I[0].Q.word = EnB.Q.b0? 0:w0I[0].D.w10;  w0I[0].D.word = max(-511,min(511,FSDI0I.w12));
    w1R[0].Q.word = EnB.Q.b0? 0:w1R[0].D.w10;  w1R[0].D.word = max(-511,min(511,FSDI1R.w12));
    w1I[0].Q.word = EnB.Q.b0? 0:w1I[0].D.w10;  w1I[0].D.word = max(-511,min(511,FSDI1I.w12));

    // ------------------------
    // Clock Internal Registers
    // ------------------------
    ClockRegister(AcmPw0);
    ClockRegister(AcmPw1);
    ClockRegister(AcmAxR);
    ClockRegister(AcmAxI);
    ClockRegister(PwSig);
    ClockRegister(PwExp);
    ClockRegister(AxSig);
    ClockRegister(AxExp);
    ClockRegister(RatioSig);
    ClockRegister(RatioExp);
    ClockRegister(MaxSig);
    ClockRegister(MaxExp);
    ClockRegister(State);
    ClockRegister(PeakFound);

    for (i=1; i<=2; i++)
    {
        ClockRegister(w0R[i]);  w0R[i].D = w0R[i-1].Q;
        ClockRegister(w0I[i]);  w0I[i].D = w0I[i-1].Q;
        ClockRegister(w1R[i]);  w1R[i].D = w1R[i-1].Q;
        ClockRegister(w1I[i]);  w1I[i].D = w1I[i-1].Q;
    }
    // ------------------
    // Increment Counters
    // ------------------
    A0 = (A0 + 1) % 176;
    A1 = (A1 + 1) % 176;
    A2 = (A2 + 1) % 176;
    A3 = (A3 + 1) % 176;
    tpeak.word = tpeak.word + 1; // tpeak is shown as 32b resolution; in RTL it is 5b and a secondary signal
                                // is generated with SigDetValid to enable examination of the signal; thus,
                                // in RTL if it rolls over to 0 and counts to SyncDelay, there will not be
                                // a PeakFound event.

    // ------------------------------
    // Update FrameSync State Machine
    // ------------------------------
    AcmClrB.b0   = (State.Q.w8==  0)? 0:1;
    AcmEnB1.b0   = (State.Q.w8>=  2)? 0:1;
    AcmEnB1.b1   = (State.Q.w8>=146)? 0:1;
    AcmEnB2.b0   = (State.Q.w8>= 18)? 0:1;
    AcmEnB2.b1   = (State.Q.w8>=162)? 0:1;
    RatioValid   = (State.Q.w8>= 28)? 1:0; // new to avoid false peaks on first few samples
    State.D.word = EnB.Q.b0? 0 : ((State.Q.w8==163)? 163:(State.Q.w8+1));

    // --------------
    // Reset Counters
    // --------------
    if (!AcmClrB.b0) 
    { 
        A0=0; A1=160; A2=32; A3=16; 
        tpeak.word = pReg->SyncDelay.w5 + 1;

        MaxSig.D.word    =  0;
        MaxExp.D.word    = -32;
        PeakFound.D.word = 0;
        SigDet.D.word    = 0;
    }
    // ----------------------------------------------
    // Model: return to avoid unnecessary computation
    // ----------------------------------------------
    if (EnB.Q.b0) return;

    // -------------------------------------------------------
    // Generate input samples: Y(D)=W(D) or Y(D)=W(D)(1-D^2)/2
    // -------------------------------------------------------
    if (pReg->SyncFilter.b0)
    {
        y0R.word = (w0R[0].Q.w10 - w0R[2].Q.w10) >> 1;
        y0I.word = (w0I[0].Q.w10 - w0I[2].Q.w10) >> 1;
        y1R.word = (w1R[0].Q.w10 - w1R[2].Q.w10) >> 1;
        y1I.word = (w1I[0].Q.w10 - w1I[2].Q.w10) >> 1;
    }
    else
    {
        y0R.word = w0R[0].Q.w10;
        y0I.word = w0I[0].Q.w10;
        y1R.word = w1R[0].Q.w10;
        y1I.word = w1I[0].Q.w10;
    }
    // ------------------------------------------------------------
    // Select a reduced-resolution input sample if the AGC is done;
    // otherwise, choose a scaled sign of the input
    // ------------------------------------------------------------
    if (PathSel.Q.b0)
    {
        x0R.word = AGCDone.Q.b0? (y0R.w10/16) : ((y0R.w10==0)? 0 : (y0R.b9? -(int)(pReg->SyncMag.w5):pReg->SyncMag.w5));
        x0I.word = AGCDone.Q.b0? (y0I.w10/16) : ((y0I.w10==0)? 0 : (y0I.b9? -(int)(pReg->SyncMag.w5):pReg->SyncMag.w5));
    }
    if (PathSel.Q.b1)
    {
        x1R.word = AGCDone.Q.b0? (y1R.w10/16) : ((y1R.w10==0)? 0 : (y1R.b9? -(int)(pReg->SyncMag.w5):pReg->SyncMag.w5));
        x1I.word = AGCDone.Q.b0? (y1I.w10/16) : ((y1I.w10==0)? 0 : (y1I.b9? -(int)(pReg->SyncMag.w5):pReg->SyncMag.w5));
    }
    // --------------------------------
    // Write the input sample to memory
    // --------------------------------
    if (PathSel.Q.b0)
    {
        mem0R[A0].word = x0R.w6;
        mem0I[A0].word = x0I.w6;
    }
    if (PathSel.Q.b1)
    {
        mem1R[A0].word = x1R.w6;
        mem2I[A0].word = x1I.w6;
    }
    // ---------------------------------------------
    // Calculate Correlator and Signal Power Samples
    // ---------------------------------------------
    if (PathSel.Q.w2==1)
    {
        ax1R.word= x0R.w6*mem0R[A1].w6 + x0I.w6*mem0I[A1].w6;
        ax1I.word= x0I.w6*mem0R[A1].w6 - x0R.w6*mem0I[A1].w6;
        pw1.word = x0R.w6*x0R.w6       + x0I.w6*x0I.w6;
        pw2.word = mem0R[A1].w6*mem0R[A1].w6 + mem0I[A1].w6*mem0I[A1].w6;

        ax2R.word= mem0R[A2].w6*mem0R[A3].w6 + mem0I[A2].w6*mem0I[A3].w6;
        ax2I.word= mem0I[A2].w6*mem0R[A3].w6 - mem0R[A2].w6*mem0I[A3].w6;
        pw3.word = mem0R[A2].w6*mem0R[A2].w6 + mem0I[A2].w6*mem0I[A2].w6;
        pw4.word = mem0R[A3].w6*mem0R[A3].w6 + mem0I[A3].w6*mem0I[A3].w6;
    }
    if (PathSel.Q.w2==2)
    {
        ax1R.word= x1R.w6*mem1R[A1].w6 + x1I.w6*mem2I[A1].w6;
        ax1I.word= x1I.w6*mem1R[A1].w6 - x1R.w6*mem2I[A1].w6;
        pw1.word = x1R.w6*x1R.w6       + x1I.w6*x1I.w6;
        pw2.word = mem1R[A1].w6*mem1R[A1].w6 + mem2I[A1].w6*mem2I[A1].w6;
    
        ax2R.word= mem1R[A2].w6*mem1R[A3].w6 + mem2I[A2].w6*mem2I[A3].w6;
        ax2I.word= mem2I[A2].w6*mem1R[A3].w6 - mem1R[A2].w6*mem2I[A3].w6;
        pw3.word = mem1R[A2].w6*mem1R[A2].w6 + mem2I[A2].w6*mem2I[A2].w6;
        pw4.word = mem1R[A3].w6*mem1R[A3].w6 + mem2I[A3].w6*mem2I[A3].w6;
    }
    if (PathSel.Q.w2==3)
    {
        ax1R.word= (x0R.w6*mem0R[A1].w6 + x0I.w6*mem0I[A1].w6 + x1R.w6*mem1R[A1].w6 + x1I.w6*mem2I[A1].w6)/2;
        ax1I.word= (x0I.w6*mem0R[A1].w6 - x0R.w6*mem0I[A1].w6 + x1I.w6*mem1R[A1].w6 - x1R.w6*mem2I[A1].w6)/2;
        pw1.word = (x0R.w6*x0R.w6       + x0I.w6*x0I.w6       + x1R.w6*x1R.w6       + x1I.w6*x1I.w6      )/2;
        pw2.word = (mem0R[A1].w6*mem0R[A1].w6 + mem0I[A1].w6*mem0I[A1].w6 + mem1R[A1].w6*mem1R[A1].w6 + mem2I[A1].w6*mem2I[A1].w6)/2;
    
        ax2R.word= (mem0R[A2].w6*mem0R[A3].w6 + mem0I[A2].w6*mem0I[A3].w6 + mem1R[A2].w6*mem1R[A3].w6 + mem2I[A2].w6*mem2I[A3].w6)/2;
        ax2I.word= (mem0I[A2].w6*mem0R[A3].w6 - mem0R[A2].w6*mem0I[A3].w6 + mem2I[A2].w6*mem1R[A3].w6 - mem1R[A2].w6*mem2I[A3].w6)/2;
        pw3.word = (mem0R[A2].w6*mem0R[A2].w6 + mem0I[A2].w6*mem0I[A2].w6 + mem1R[A2].w6*mem1R[A2].w6 + mem2I[A2].w6*mem2I[A2].w6)/2;
        pw4.word = (mem0R[A3].w6*mem0R[A3].w6 + mem0I[A3].w6*mem0I[A3].w6 + mem1R[A3].w6*mem1R[A3].w6 + mem2I[A3].w6*mem2I[A3].w6)/2;
    }
    // ------------
    // Accumulators
    // ------------
    if (AcmClrB.b0) // accumulators enabled
    {
        AcmAxR.D.word = (AcmAxR.Q.w20) + (AcmEnB2.b0? 0:ax1R.w12) - (AcmEnB2.b1? 0:ax2R.w12);
        AcmAxI.D.word = (AcmAxI.Q.w20) + (AcmEnB2.b0? 0:ax1I.w12) - (AcmEnB2.b1? 0:ax2I.w12);
        AcmPw0.D.word = (AcmPw0.Q.w20) + (AcmEnB1.b0? 0:pw1.w12 ) - (AcmEnB1.b1? 0:pw3.w12 );
        AcmPw1.D.word = (AcmPw1.Q.w20) + (AcmEnB2.b0? 0:pw2.w12 ) - (AcmEnB2.b1? 0:pw4.w12 );
    }
    else           // clear accumulators
    {
        AcmAxR.D.word = 0;
        AcmAxI.D.word = 0;
        AcmPw0.D.word = 0;
        AcmPw1.D.word = 0;
    }
    // ---------------------------------------------------
    // Convert Accumulator Output Values to Floating Point
    // ---------------------------------------------------
    aMojave_FS_UIntToFloat(&AxRSig, &AxRExp, abs(AcmAxR.Q.w20));
    aMojave_FS_UIntToFloat(&AxISig, &AxIExp, abs(AcmAxI.Q.w20));
    aMojave_FS_UIntToFloat(&Pw1Sig, &Pw1Exp, AcmPw0.Q.w20);
    aMojave_FS_UIntToFloat(&Pw2Sig, &Pw2Exp, AcmPw1.Q.w20);

    // ------------------------------------------------   
    // Compute the Test Metric Denominator Terms: Power
    // ------------------------------------------------   
    aMojave_FS_UIntToFloat(&(PwSig.D), &PwSigExp, Pw1Sig.w11*Pw2Sig.w11); 
    PwExp.D.word = PwSigExp.w7 + Pw1Exp.w6 + Pw2Exp.w6;

    // -----------------------------------------------------------------
    // Compute the Test Metric Numerator Terms: Autocorrelation w/Lag=16
    // -----------------------------------------------------------------
    aMojave_FS_UIntToFloat(&Ax1Sig, &Ax1Exp, (AxRSig.w11 * AxRSig.w11));
    aMojave_FS_UIntToFloat(&Ax2Sig, &Ax2Exp, (AxISig.w11 * AxISig.w11));

    Ax1Exp.word = Ax1Exp.w7 + (AxRExp.w6 << 1);
    Ax2Exp.word = Ax2Exp.w7 + (AxIExp.w6 << 1);

    if (Ax1Exp.w7>Ax2Exp.w7)          // addition of two floating values
        while (Ax1Exp.w7>Ax2Exp.w7)
        {
            Ax2Sig.word = Ax2Sig.w11/2;
            Ax2Exp.word = Ax2Exp.w7 +1;
        }
    else
        while (Ax1Exp.w7<Ax2Exp.w7)
        {
            Ax1Sig.word =Ax1Sig.w11/2;
            Ax1Exp.word =Ax1Exp.w7 +1;
        }

    sumSig.word  = Ax1Sig.w11 + Ax2Sig.w11;
    AxSig.D.word = (sumSig.word>1023)? (sumSig.w12/2):sumSig.w11;
    AxExp.D.word = (sumSig.word>1023)? Ax1Exp.w7+1   :Ax1Exp.w7;

    // -----------------------------------------
    // Compute Ratio of Autocorrelation to Power
    // -----------------------------------------
    if (PwSig.Q.w11>0)
    {
        aMojave_FS_UIntToFloat(&(RatioSig.D), &RatioSigExp, ((AxSig.Q.w11<<11)/PwSig.Q.w11)); 
        RatioExp.D.word = RatioSigExp.w8 + AxExp.Q.w7 - PwExp.Q.w7 - 11;
    }
    else
    {
        RatioSig.D.word =  0;
        RatioExp.D.word = -31;
    }
    // --------------------------
    // Format the Threshold Terms
    // --------------------------
    ThSig.w11 = (pReg->SyncThSig.w4 << 7);
    ThExp.w5  = (pReg->SyncThExp.w4  - 7);

    // ----------------------------------------------------------------------
    // Compare the Ratio to Determine the Sync Position and SigDet Validation
    // ----------------------------------------------------------------------
    if (((RatioExp.Q.w8>ThExp.w5) || (RatioExp.Q.w8==ThExp.w5 && RatioSig.Q.w11>ThSig.w11)) && RatioValid)
    {
        SigDetValid.D.b0 = 1; // signal detection is valid

        // find peak
        if (PeakSearch.Q.b0 && ((RatioExp.Q.w8>MaxExp.Q.w8) ||   (RatioExp.Q.w8==MaxExp.Q.w8 && RatioSig.Q.w11>MaxSig.Q.w11)))
        {
            MaxExp.D.word=RatioExp.Q.w8;
            MaxSig.D.word=RatioSig.Q.w11;
            tpeak.word = 0;
        }
        else
        {
            MaxExp.D = MaxExp.Q;
            MaxSig.D = MaxSig.Q;
        }
    }
    else
    {
        SigDetValid.D.b0 = 0;
    }
    // -------------
    // Peak Detector
    // -------------
    PeakFound.D.b0 = (tpeak.word==pReg->SyncDelay.w5)? 1:0;

    // ---------------------------------------------------
    // Output Registers (clocked and wired)
    // (Preclock to accomodate DFE function calling order)
    // ---------------------------------------------------
    ClockRegister(PeakFound);   *FSPeakFound = PeakFound.Q;
    ClockRegister(SigDetValid); *FSSigDetValid = SigDetValid.Q;

    // -------------
    // Trace Signals
    // -------------
    if (pRX->EnableTrace)
    {
        pRX->traceFrameSync[pRX->k20].FSEnB      = FSEnB.b0;
        pRX->traceFrameSync[pRX->k20].State      = State.Q.w8;
        pRX->traceFrameSync[pRX->k20].RatioSig   = RatioSig.Q.w11;
        pRX->traceFrameSync[pRX->k20].RatioExp   = RatioExp.Q.w8;
        pRX->traceFrameSync[pRX->k20].SigDetValid= FSSigDetValid->b0;
        pRX->traceFrameSync[pRX->k20].PeakFound  = FSPeakFound->b0;
        pRX->traceFrameSync[pRX->k20].AGCDone    = FSAGCDone.b0;
        pRX->traceFrameSync[pRX->k20].PeakSearch = FSPeakSearch.b0;
    }
}
// end of aMojave_FrameSync()

// ================================================================================================
// FUNCTION  : aMojave_Linear_to_dB4
// ------------------------------------------------------------------------------------------------
// Purpose   : Convert a linear scaled value x to y where x ~= 2^(y/4)
// Parameters: x -- Linear scaled unsigned word
//             y -- log2(4*x)
// ================================================================================================
void aMojave_Linear_to_dB4(wiUWORD x, wiUWORD *y)
{
    wiUWORD z; z.word = x.w23 >> 4;

    if (z.word==0) y->word = 0;
    else {
        int tmp = (int)floor(log2(z.word) + 1E-9);
        if (tmp>=3) y->word = (int)floor(0.5 + 4*log2((z.word>>(tmp-3))/8.0)) + 4*tmp;
        else       y->word = (int)floor(0.5 + 4*log2((z.word<<(3-tmp))/8.0)) + 4*tmp;
    }
}
// end of aMojave_Linear_to_dB4()

// ================================================================================================
// FUNCTION  : aMojave_DigitalAmp_Core
// ------------------------------------------------------------------------------------------------
// Purpose   : Core functionality for aMojave_DigitalAmp
// Parameters: n     -- specified module instance (input, 'c' model only)
//             DAEnB -- Enable (active low)
//             DADI  -- Data (12 bit input)
//             DADO  -- Data (12 bit output)
//             Gain  -- Gain as multiplier (15 bit input)
// ================================================================================================
void aMojave_DigitalAmp_Core(int n, wiBoolean DAEnB, wiWORD  DADI, wiWORD *DADO, wiUWORD Gain)
{
    static wiReg DO[4]; // output registers
    wiWORD p, q;        // intermediate calculations
    
    // -----------
    // Digital Amp
    // -----------
    p.word = ((int)DADI.w12 * Gain.w15) / 1024;                // multiplier output
    q.word = (abs(p.w17)<2048)? p.w12:((p.w17<0)? -2047:2047); // limited for CFO input

    // ---------------
    // Output Register
    // ---------------
    DO[n].Q = DO[n].D;
    DO[n].D.word = DAEnB? 0 : q.w12;
    DADO->word   = DO[n].Q.w12;
}
// end of aMojave_DigitalAmp_Core()

// ================================================================================================
// FUNCTION  : aMojave_DigitalAmp
// ------------------------------------------------------------------------------------------------
// Purpose   : Digital amplifier (AGC controlled)
// Parameters: DAEnB     -- Digital amp enable (active low)
//             DAGain    -- Digital gain (6 bit input, A=2^(x/4))
//             DADI0R    -- Signal input, path 0, real component (12 bit input)
//             DADI0I    -- Signal input, path 0, imag component (12 bit input)
//             DADI1R    -- Signal input, path 1, real component (12 bit input)
//             DADI1I    -- Signal input, path 1, imag component (12 bit input)
//             DADO0R    -- Signal output, path 0, real component (12 bit input)
//             DADO0I    -- Signal output, path 0, imag component (12 bit input)
//             DADO1R    -- Signal output, path 1, real component (12 bit input)
//             DADO1I    -- Signal output, path 1, imag component (12 bit input)
//             DAGain    -- Digital gain (6 bit input, A=2^(x/4))
// ================================================================================================
void aMojave_DigitalAmp(wiBoolean DAEnB, wiWORD DAGain,
                                wiWORD  DADI0R, wiWORD  DADI0I, wiWORD  DADI1R, wiWORD  DADI1I,
                                wiWORD *DADO0R, wiWORD *DADO0I, wiWORD *DADO1R, wiWORD *DADO1I)
{
    int j, tmpi;  wiUWORD A;

    // ------------------------------------------------
    // Compute the linear gain in floating point format
    // ------------------------------------------------
    A.word = (unsigned)floor(0.5+512.0*pow(2.0,(double)DAGain.w6/4.0));
    j=0; tmpi=abs(A.w15);
    while (tmpi>63) { tmpi=tmpi>>1; j++;}; 
    if (tmpi & 0x01 && tmpi>31) tmpi++;
    for (;j>0;j--) tmpi=tmpi<<1;
    A.word= A.w15>0 ? tmpi : -tmpi;

    // ------------------------------
    // Replicate the four multipliers
    // ------------------------------
    aMojave_DigitalAmp_Core(0, DAEnB, DADI0R, DADO0R, A); // path 0, real
    aMojave_DigitalAmp_Core(1, DAEnB, DADI0I, DADO0I, A); // path 0, imaginary
    aMojave_DigitalAmp_Core(2, DAEnB, DADI1R, DADO1R, A); // path 1, real
    aMojave_DigitalAmp_Core(3, DAEnB, DADI1I, DADO1I, A); // path 1, imaginary
}
// end of aMojave_DigitalAmp()

// ================================================================================================
// FUNCTION  : aMojave_AGC
// ------------------------------------------------------------------------------------------------
// Purpose   : Automatic gain control
// Parameters: AGCEnB        -- Enable the circuit, active low (input 2b)
//             AGCPathSel    -- Select the active digital paths (2 bit input)
//             AGCInitGains  -- Initialize AGC controlled gains, active high (input)
//             AGCUpdateGains-- Update the AGC controlled gains, active high (input)
//             AGCLgSigAFE   -- Indicates a large signal at the AFE (1 bit input)
//             AGCDI0R       -- Signal input, path 0, real component (12 bit input)
//             AGCDI0I       -- Signal input, path 0, imag component (12 bit input)
//             AGCDI1R       -- Signal input, path 1, real component (12 bit input)
//             AGCDI1I       -- Signal input, path 1, imag component (12 bit input)
//             AGCLNAGain    -- LNA gain switch (2 bit output)
//             AGCAGain      -- Analog VGA setting (5 bit output)
//             AGCLgSigDet   -- Large signal detected (1 bit output)
//             AGCCCA        -- Clear channel assessment (2 bit output)
//             AGCRSSI0      -- Received signal power, path 0 (7 bit output)
//             AGCRSSI1      -- Received signal power, path 1 (7 bit output)
//             AGCCFODI0R    -- AGC output to CFO, path 0, real component (14 bit output)
//             AGCCFODI0I    -- AGC output to CFO, path 0, imag component (14 bit output)
//             AGCCFODI1R    -- AGC output to CFO, path 1, real component (14 bit output)
//             AGCCFODI1I    -- AGC output to CFO, path 1, real component (14 bit output)
//             w0R           -- Reduced resolution version of AGCDIxx (10 bits)
//             w0I           -- Reduced resolution version of AGCDIxx (10 bits)
//             w1R           -- Reduced resolution version of AGCDIxx (10 bits)
//             w1I           -- Reduced resolution version of AGCDIxx (10 bits)
//             pReg          -- Pointer to modeled register file (model input)
//             pRX           -- Pointer to RX state for test output (model input)
// ================================================================================================
void aMojave_AGC(wiUWORD AGCEnB, wiUWORD AGCDAEnB, wiUWORD AGCPathSel, wiUWORD AGCInitGains, wiUWORD AGCUpdateGains, 
                 wiBoolean AGCLgSigAFE, wiUWORD AGCUseRecordPwr, wiUWORD AGCRecordPwr, wiUWORD AGCRecordRefPwr,
                 wiUWORD AGCAntSel, wiUWORD *AGCAntSuggest, wiUWORD *AGCAntSearch,wiWORD  AGCDI0R, wiWORD  AGCDI0I, 
                 wiWORD  AGCDI1R, wiWORD AGCDI1I, wiUWORD *AGCLNAGain, wiUWORD *AGCAGain, wiUWORD *AGCLgSigDet, 
                 wiUReg  *AGCCCA, wiUWORD *AGCRSSI0, wiUWORD *AGCRSSI1, wiWORD  *AGCCFODI0R, wiWORD *AGCCFODI0I, 
                 wiWORD *AGCCFODI1R, wiWORD *AGCCFODI1I, wiWORD  w0R, wiWORD w0I, wiWORD w1R, wiWORD w1I, 
                 MojaveRegisters *pReg, Mojave_RX_State *pRX, Mojave_aSigState *SigOut)
{
    // -------------------------------------------------
    // Internal Memory Devices (RAM, Register, Counters)
    // -------------------------------------------------
    static wiWORD   mem0R[17],mem0I[17]; // delay line for path1
    static wiWORD   mem1R[17],mem1I[17]; // delay line for path2
    static wiWORD  DI0R, DI0I;          // data input, path 0
    static wiWORD  DI1R, DI1I;          // data input, path 1
    static wiUReg  State;               // controller state
    static wiUReg  AcmPwr0, AcmPwr1;    // power measurement accumulators
    static wiUReg  AGain;               // analog gain
    static wiReg   DGain;               // digital gain
    static wiUReg  LNAGain;             // LNA gain switch
    static wiUReg  LgSigDet;            // large signal detected
    static wiUReg  EnB, DAEnB;          // overall AGC and DGain enables
    static wiUReg  PathSel;
    static wiUReg  InitGains;
    static wiUReg  UpdateGains;
    static wiUWORD UpdateCount;         // implicit register (update counter)
    static wiUWORD A1, A2;              // memory address counters
    static wiUWORD detThDFSDn;          // delay line for RSSI < ThDFSDn
    
    static wiUReg RSSI0,   RSSI1;   // calibrated RSSI values
    static wiUReg msrpwrA, msrpwrB; // recorded measured power (antenna selection)
    static wiUReg abspwrA, abspwrB; // recorded absolute power (antenna selection)
    static wiUReg refpwr;           // recorded packet reference power (StepUp/StepDown)
    static wiUReg ValidRefPwr;      // refpwr is valid for current packet
    
    wiUWORD pw0,pw1,pw2,pw3;    // quadrature component power measures
    wiUWORD msrpwr0;            // measured digital power in the power of 2^(1/4)
    wiUWORD msrpwr1;            // second path, measured digital power in the power of 2^(1/4)
    wiUWORD abspwr0;            // measured absolute power in the power of 2^(1/4)
    wiUWORD abspwr1;            // second path, absolute signal power
    wiWORD  AGainUpdate;        // the amount of update of the analog gain in the power of 2^(1/4)
    wiWORD  pathgain;           // controlled path gain from LNA, VGA, digital amplifier
    wiUWORD msrpwr;             // measured power = max(msrpwr0, msrpwr1)
    wiUWORD abspwr;             // absolute power = max(abspwr0, abspwr1)
    wiUWORD RSSI;               // RSSI = max(RSSI0, RSSI1)
    wiWORD  d0R, d0I, d1R, d1I; // wired output from RAM, w?? input delay by 16 samples
    wiUWORD AcmClrB, AcmEnB;    // power accumulator control signals
    wiWORD  AGain1, AGain2, AGain3;// intermediate calculation for AGain
    wiWORD  DGain1;                // intermediate calculation for DGain
    wiWORD  deltaGain;          // total change in gain

    // ---------------
    // Input Registers
    // ---------------
    ClockRegister(EnB);         EnB.D        = AGCEnB;
    ClockRegister(DAEnB);       DAEnB.D      = AGCDAEnB;
    ClockRegister(PathSel);     PathSel.D    = AGCPathSel;
    ClockRegister(InitGains);   InitGains.D  = AGCInitGains;
    ClockRegister(UpdateGains); UpdateGains.D= AGCUpdateGains;  
    
    // ------------------------
    // Clock Internal Registers
    // ------------------------
    ClockRegister(AcmPwr0);
    ClockRegister(AcmPwr1);
    ClockRegister(State);
    ClockRegister(LNAGain);
    ClockRegister(AGain);
    ClockRegister(DGain);
    ClockRegister(LgSigDet);

    ClockRegister(RSSI0); ClockRegister(msrpwrA); ClockRegister(abspwrA);
    ClockRegister(RSSI1); ClockRegister(msrpwrB); ClockRegister(abspwrB);
    ClockRegister(refpwr);ClockRegister(ValidRefPwr);

    // ---------------------------
    // Wire Inputs Based On Enable
    // ---------------------------
    DI0R.word = EnB.Q.b0? 0:AGCDI0R.w12;
    DI0I.word = EnB.Q.b0? 0:AGCDI0I.w12;
    DI1R.word = EnB.Q.b0? 0:AGCDI1R.w12;
    DI1I.word = EnB.Q.b0? 0:AGCDI1I.w12;

    // ------------------
    // Increment Counters
    // ------------------
    A1.word = (A1.w5 + 1) % 17;
    A2.word = (A2.w5 + 1) % 17;

    // ------------------------
    // Update AGC State Machine
    // ------------------------
    AcmClrB.b0   = (State.Q.w5== 0)? 0:1;
    AcmEnB.b0    = (State.Q.w5>= 3)? 0:1;
    AcmEnB.b1    = (State.Q.w5==19)? 0:1;
    State.D.word = EnB.Q.b0? 0 : ((State.Q.w5==19)? 19:(State.Q.w5+1));

    // --------------
    // Reset Counters
    // --------------
    if (!AcmClrB.b0) {A1.word=0; A2.word=1;}

    // ---------------------------
    // Digital Amplifier Submodule
    // ---------------------------
    aMojave_DigitalAmp(DAEnB.Q.b0, DGain.Q, DI0R, DI0I, DI1R, DI1I, AGCCFODI0R, AGCCFODI0I, AGCCFODI1R, AGCCFODI1I);

    // ----------------------------------------------------------------
    // Path 0 Power Measurement
    // Note: should store computed power not the quadrature components.
    //       The current arrangement requires 2x power.
    // ----------------------------------------------------------------
    if (PathSel.Q.b0)
    {
        mem0R[A1.w5].word = w0R.w10; // write new samples to RAM at address A1
        mem0I[A1.w5].word = w0I.w10;

        d0R.word = mem0R[A2.w5].w10; // read 16T delayed samples from RAM at address A2
        d0I.word = mem0I[A2.w5].w10;

        pw0.word = (w0R.w10 * w0R.w10) + (w0I.w10 * w0I.w10);
        pw1.word = (d0R.w10 * d0R.w10) + (d0I.w10 * d0I.w10);

        if (AcmClrB.b0) AcmPwr0.D.word = (AcmPwr0.Q.w23) + (AcmEnB.b0? 0:pw0.w19) - (AcmEnB.b1? 0:pw1.w19);
        else           AcmPwr0.D.word = 0;

        aMojave_Linear_to_dB4(AcmPwr0.Q, &msrpwr0);
    }
    // ------------------------
    // Path 1 Power Measurement
    // ------------------------
    if (PathSel.Q.b1)
    {
        mem1R[A1.w5].word = w1R.w10; // write new samples to RAM at address A1
        mem1I[A1.w5].word = w1I.w10;

        d1R.word = mem1R[A2.w5].w10; // read 16T delayed samples from RAM at address A2
        d1I.word = mem1I[A2.w5].w10;

        pw2.word = (w1R.w10 * w1R.w10) + (w1I.w10 * w1I.w10);
        pw3.word = (d1R.w10 * d1R.w10) + (d1I.w10 * d1I.w10);

        if (AcmClrB.b0) AcmPwr1.D.word = (AcmPwr1.Q.w23) + (AcmEnB.b0? 0:pw2.w19) - (AcmEnB.b1? 0:pw3.w19);
        else           AcmPwr1.D.word = 0;

        aMojave_Linear_to_dB4(AcmPwr1.Q, &msrpwr1);
    }
    // --------------------------------------------------
    // Convert measured power to estimated absolute power
    // --------------------------------------------------
    {
        wiWORD a1, a2, a3;
        a1.word = LNAGain.Q.b0? 0 : -((signed)pReg->StepLNA.w5);  // LNA
        a2.word = LNAGain.Q.b1? 0 : -((signed)pReg->StepLNA2.w5); // LNA2
        a3.word = AGain.Q.w6 - pReg->InitAGain.w6;                // analog VGA
        pathgain.word = 2 * (a1.w6 + a2.w6 + a3.w7);              // total gain

        abspwr0.word = PathSel.Q.b0 ? max(0, (msrpwr0.w7 - pathgain.w8)) : 0;
        abspwr1.word = PathSel.Q.b1 ? max(0, (msrpwr1.w7 - pathgain.w8)) : 0;
    }
    // -----------------------------------------------------
    // Select the larger measured power for the gain control
    // -----------------------------------------------------
    {
        wiBoolean s = (!PathSel.Q.b0 || ((PathSel.Q.w2==0x3) && (msrpwr1.w7>msrpwr0.w7)))? 1:0;
        msrpwr.word = s? msrpwr1.w7 : msrpwr0.w7;
        abspwr.word = s? abspwr1.w8 : abspwr0.w8;
        RSSI.word   = s? RSSI1.Q.w8 : RSSI0.Q.w8;
    }
    // --------------------------------------------
    // Added circuit for switched antenna diversity
    // --------------------------------------------
    AGCAntSearch->b0  = (msrpwr.w7<pReg->OFDMSwDTh.w8 && AGCPathSel.w3!=3)? 1:0;
    AGCAntSuggest->b0 = (msrpwrA.Q.w7 > msrpwrB.Q.w7)? 0:1;
    if (AGCUseRecordPwr.b0)
    {
        if (AGCAntSel.b0) {
            abspwr.word = abspwrA.Q.w8;
            msrpwr.word = msrpwrA.Q.w7;
        }
        else {
            abspwr.word = abspwrB.Q.w8;
            msrpwr.word = msrpwrB.Q.w7;
        }
    }
    if (AGCRecordPwr.b0)
    {
        if (AGCAntSel.b0) {
            abspwrB.D.word = abspwr.w8;
            msrpwrB.D.word = msrpwr.w7;
        }
        else {
            abspwrA.D.word = abspwr.w8;
            msrpwrA.D.word = msrpwr.w7;
        }
    }
    if (AGCRecordRefPwr.b0)
    {
        refpwr.D.word = abspwr.w8;
        ValidRefPwr.D.word = 1;
    }
    // ------------------------
    // Initialize Gain Settings
    // ------------------------
    if (InitGains.Q.b0)
    {
        AGain3.word = pReg->InitAGain.w6;
        DGain1.word = 0;
        LNAGain.D.word  = 3; // b0=1, b1=1
        UpdateCount.word = 0;
        refpwr.D.word = 0;
        ValidRefPwr.D.word = 0;
    }
    // ------------------------------
    // Update Analog and Digital Gain
    // ------------------------------
    else if (UpdateGains.D.b0)
    {
        wiBoolean GainLimit = (UpdateCount.w3==pReg->MaxUpdateCount.w3)? 1:0;
        UpdateCount.w3++; // increment gain update counter

        // --------------------------------------------
        // Analog Front End Overload: Decrease LNA Gain
        // --------------------------------------------
        if (!GainLimit && AGCLgSigAFE)
        {
            AGain1.word   = AGain.Q.w6 - pReg->StepLgSigDet.w5;
            DGain1.word   = 0;
            LgSigDet.D.b0 = 1;           // flag large signal detect
            pRX->EventFlag.b5 = 1;       // flag LNA gain change
        }
        // ---------------------------
        // Large Measured Signal Power
        // ---------------------------
        else if (!GainLimit && msrpwr.w7 > pReg->ThSigSmall.w7)
        {
            AGain1.word   = AGain.Q.w6 - ((msrpwr.w7>pReg->ThSigLarge.w7)? pReg->StepLarge.w5 : pReg->StepSmall.w5);
            DGain1.word   = 0;
            LgSigDet.D.b0 = 1;
        }
        // -------------------------------------
        // Low to Moderate Measured Signal Power
        // -------------------------------------
        else
        {
            if     (abspwr.w8<=(int)pReg->AbsPwrL.w7) AGainUpdate.word = pReg->AbsPwrL.w7 - msrpwr.w7;
            else if (abspwr.w8>=(int)pReg->AbsPwrH.w7) AGainUpdate.word = pReg->AbsPwrH.w7 - msrpwr.w7;
            else                                      AGainUpdate.word =        abspwr.w8 - msrpwr.w7;

            AGain1.word   = AGain.Q.w6 + AGainUpdate.w8/2;
            DGain1.word   = (pReg->RefPwrDig.w7-msrpwr.w7)/2 - AGainUpdate.w7/2; // slight change from old calc
            LgSigDet.D.b0 = 0;
        }   
        // ---------------------------------
        // Check LNA Gain Reduction Criteria
        // ---------------------------------
        deltaGain.word = ((int)pReg->InitAGain.w6-AGain1.word) 
                        + (int)(LNAGain.Q.b0? 0:pReg->StepLNA.w5) 
                        + (int)(LNAGain.Q.b1? 0:pReg->StepLNA2.w5); // attenuation relative to initial gain

        if (LNAGain.Q.b0 && (deltaGain.w7 > (signed)pReg->ThSwitchLNA.w6))
        {
            LNAGain.D.b0 = 0;                              // switch LNA to low gain mode
            AGain2.word  = AGain1.w7 + pReg->StepLNA.w5;   // increase VGA gain by LNA gain difference
            if (pReg->UpdateOnLNA.b0) LgSigDet.D.b0 = 1;    // force LgSigDet to 1
            pRX->EventFlag.b5 = 1;                         // flag LNA gain change
        }
        else AGain2.word = AGain1.word; // no switch

        if (LNAGain.Q.b1 && (deltaGain.w7 > (signed)pReg->ThSwitchLNA2.w6))
        {
            LNAGain.D.b1 = 0;                              // switch LNA2 to low gain mode
            AGain3.word  = AGain2.w7 + pReg->StepLNA2.w5;  // increase VGA gain by LNA gain difference
            if (pReg->UpdateOnLNA.b0) LgSigDet.D.b0 = 1;    // force LgSigDet to 1
        }
        else AGain3.word = AGain2.w7; // no switch
    }
    // ------------------------------
    // Maintain Current Gain Settings
    // ------------------------------
    else
    {
        LgSigDet.D.b0 = 0;
        AGain3.word   = AGain.Q.w6;
        DGain1.word   = DGain.Q.w6;
    }
    // -------------------------------------------
    // Limit AGain to Valid Range (5-bit unsigned)
    // -------------------------------------------
         if (AGain3.w7 < 0)  AGain.D.word =  0;
    else if (AGain3.w7 > 63) AGain.D.word = 63;
    else                    AGain.D.word = AGain3.w7;

    // -------------------------------------------
    // Limit DGain to Valid Range (6-bit unsigned)
    // -------------------------------------------
         if (DGain1.w6 <-19) DGain.D.word = -19;
    else if (DGain1.w6 > 19) DGain.D.word =  19;
    else                    DGain.D.word = DGain1.w6;

    // ----------------------------
    // Calibration to antenna power
    // ----------------------------
    {
        int s0 = (AGCPathSel.b0 && (abspwr0.w8 > pReg->Pwr100dBm.w5))? 1:0;
        int s1 = (AGCPathSel.b1 && (abspwr1.w8 > pReg->Pwr100dBm.w5))? 1:0;
        RSSI0.D.word = s0? (abspwr0.w8 - pReg->Pwr100dBm.w5) : 0;
        RSSI1.D.word = s1? (abspwr1.w8 - pReg->Pwr100dBm.w5) : 0;
    }
    // ----------------------------------------------------------
    // Detect Signal Power Increase/Decreases (after AGC is done)
    // ----------------------------------------------------------
    if (SigOut->ValidRSSI && ValidRefPwr.Q.b0)
    {
        SigOut->StepUp   =((abspwr.w8 > refpwr.Q.w8+2*pReg->ThStepUp.w4) && (refpwr.Q.w8 < pReg->ThStepUpRefPwr.w8) && refpwr.Q.w8)? 1:0;
        SigOut->StepDown = (abspwr.w8 < refpwr.Q.w8-2*pReg->ThStepDown.w4)? 1:0;
    }
    else SigOut->StepUp = SigOut->StepDown = 0;

    // ----------------
    // DFS Pulse Detect
    // ----------------
    if (EnB.Q.b0)        SigOut->PulseDet = 0; // reset pulse detector if AGC is disabled
    else if (SigOut->ValidRSSI) 
    {
        wiBoolean PulseUp = (RSSI.w8 >=pReg->ThDFSUp.w7) && ((detThDFSDn.w32 & 0xFFFC0000) == 0xFFFC0000);
        wiBoolean PulseDn = (RSSI.w8 < pReg->ThDFSDn.w7);

        if     (PulseUp) SigOut->PulseDet = 1; // pulse rising edge
        else if (PulseDn) SigOut->PulseDet = 0; // pulse falling edge
        else ;                                 // keep previous value
    }  else ;                                 // hold detector output during gain update
    detThDFSDn.word = (detThDFSDn.w32 << 1) | ((SigOut->ValidRSSI && (RSSI.w8 < pReg->ThDFSDn.w7))? 1:0);

    // ------------------------
    // Clear Channel Assessment
    // ------------------------
    if (AGCInitGains.b0)
        AGCCCA->D.word = 0; // clear both CCA values
    else
    {
        AGCCCA->D.b0 = ((RSSI.w8>pReg->ThCCA1.w6)? 1:0); // threshold for SigDet based CCA
        if (SigOut->ValidRSSI)
            AGCCCA->D.b1 = pReg->DFSCCA.b0? SigOut->PulseDet : ((RSSI.w8>pReg->ThCCA2.w6)? 1:0); // absolute CCA (power only)
    }

    // ------------
    // Wire outputs
    // ------------
    pRX->DGain[pRX->k20] = DGain.Q;
    AGCAGain->word    = pReg->FixedGain.b0? pReg->InitAGain.w6 : AGain.Q.w6;
    AGCLNAGain->word  = pReg->FixedGain.b0? (2*pReg->FixedLNA2Gain.b0 + pReg->FixedLNAGain.b0) : LNAGain.Q.w2;
    AGCLgSigDet->word = LgSigDet.Q.b0;
    AGCRSSI0->word    = RSSI0.Q.w8;
    AGCRSSI1->word    = RSSI1.Q.w8;

    // -----------------------
    // Trace Operation in WiSE
    // -----------------------
    if (pRX->TraceKeyOp)
    {
        if (InitGains.Q.b0 && !InitGains.D.b0) wiPrintf("((%d,%d;%d,%d))",LNAGain.D.b0,LNAGain.D.b1,AGain.D,DGain.D);
        if (UpdateGains.D.b0) wiPrintf("(%d,%d;%d,%2d)",LNAGain.D.b1,LNAGain.D.b0,AGain.D,DGain.D);
    }
    // ------------- 
    // Trace Signals
    // ------------- 
    if (pRX->EnableTrace)
    {
        pRX->traceAGC[pRX->k20].AGCEnB      = AGCEnB.b0;
        pRX->traceAGC[pRX->k20].InitGains   = AGCInitGains.b0;
        pRX->traceAGC[pRX->k20].UpdateGains = AGCUpdateGains.b0;
        pRX->traceAGC[pRX->k20].State       = State.Q.w5;
        pRX->traceAGC[pRX->k20].LNAGain     = AGCLNAGain->w2;
        pRX->traceAGC[pRX->k20].AGAIN       = AGCAGain->w6;
        pRX->traceAGC[pRX->k20].DGAIN       = DGain.Q.w6;
        pRX->traceAGC[pRX->k20].CCA         = AGCCCA->Q.w2;
        pRX->traceAGC[pRX->k20].LgSigAFE    = AGCLgSigAFE;
        pRX->traceAGC[pRX->k20].AntSearch   = AGCAntSearch->b0;
        pRX->traceAGC[pRX->k20].AntSuggest  = AGCAntSuggest->b0;
        pRX->traceAGC[pRX->k20].RecordPwr   = AGCRecordPwr.b0;
        pRX->traceAGC[pRX->k20].UseRecordPwr= AGCUseRecordPwr.b0;
        pRX->traceAGC[pRX->k20].UpdateCount = UpdateCount.w3;

        pRX->traceRSSI[pRX->k20].RSSI0 = AGCRSSI0->w8;
        pRX->traceRSSI[pRX->k20].RSSI1 = AGCRSSI1->w8;
        pRX->traceRSSI[pRX->k20].msrpwr0 = msrpwr0.w7;
        pRX->traceRSSI[pRX->k20].msrpwr1 = msrpwr1.w7;
    }
}
// end of aMojave_AGC()

// ================================================================================================
// FUNCTION  : aMojave_SignalDetect  
// ------------------------------------------------------------------------------------------------
// Purpose   : Detect the presence of a signal in the data channel
// Parameters: SDEnB     -- Signal detect enable, active low (1 bit input)
//             SDPathSel -- Select the active digital paths (2 bit input)
//             SDDI0R    -- Signal input, path 0, real component (12->10 bit input)
//             SDDI0I    -- Signal input, path 0, imag component (12->10 bit input)
//             SDDI1R    -- Signal input, path 1, real component (12->10 bit input)
//             SDDI1I    -- Signal input, path 1, imag component (12->10 bit input)
//             SGSigDet  -- Signal detected (1 bit output)
//             pReg      -- Pointer to modeled register file (model input)
// ================================================================================================
void aMojave_SignalDetect(wiUWORD SDEnB, wiUWORD SDPathSel, wiWORD SDDI0R, wiWORD SDDI0I, wiWORD SDDI1R, 
                          wiWORD SDDI1I, wiUWORD *SDSigDet, Mojave_RX_State *pRX, MojaveRegisters *pReg)
{
    // -------------------------------------------------
    // Internal Memory Devices (RAM, Register, Counters)
    // -------------------------------------------------
    static wiWORD mem0R[96], mem0I[96];  // delay line memory, path 0
    static wiWORD mem1R[96], mem1I[96];  // delay line memory, path 1
    static wiReg  AcmAxR, AcmAxI;        // autocorrelation accumulator
    static wiUReg  AcmPwr;               // power measurement accumulator
    static wiWORD  w0R[3], w0I[3];       // input registers, path 0
    static wiWORD  w1R[3], w1I[3];       // input registers, path 1
    static wiUReg SigDet;                // signal detect
    static wiUReg State;                 // controller state
    static wiUReg EnB;                   // block enable
    static wiUReg PathSel;               // data path select
    static int    A0, A1, A2, A3, A4;    // address counters
    static int    counter;               // counter for number of measured points
    
    wiWORD  ax1R, ax1I, ax2R, ax2I;       // per-sample auto correlator output (1=current, 2=delayed)
    wiUWORD pw1, pw2;                    // squared amplitude of signal (1=current, 2=delayed)
    wiUWORD AcmClrB, AxAcmEnB, PwAcmEnB; // accumulator control signals
    wiWORD  x0R, x0I, x1R, x1I;          // filtered signal input
    wiUWORD x, t;
    int i;

    // ---------------
    // Input Registers
    // ---------------
    ClockRegister(EnB);     EnB.D.word = SDEnB.b0;
    ClockRegister(PathSel); PathSel.D.word = SDPathSel.w2;

    // -----------------------
    // Input Signal Delay Line
    // -----------------------
    for (i=2; i>0; i--)
    {
        w0R[i] = w0R[i-1];  w0I[i] = w0I[i-1];
        w1R[i] = w1R[i-1];  w1I[i] = w1I[i-1];
    }
    w0R[0].word = EnB.Q.b0? 0:SDDI0R.w10;
    w0I[0].word = EnB.Q.b0? 0:SDDI0I.w10;
    w1R[0].word = EnB.Q.b0? 0:SDDI1R.w10;
    w1I[0].word = EnB.Q.b0? 0:SDDI1I.w10;

    // ------------------------
    // Clock Internal Registers
    // ------------------------
    ClockRegister(AcmAxR);
    ClockRegister(AcmAxI);
    ClockRegister(AcmPwr);
    ClockRegister(SigDet);
    ClockRegister(State);

    // ------------------
    // Increment counters
    // ------------------
    A0  = (A0 + 1) % 96;
    A1  = (A1 + 1) % 96;
    A2  = (A2 + 1) % 96;
    A3  = (A3 + 1) % 96;
    A4  = (A4 + 1) % 96;
    counter++;

    // ---------------------------
    // Update SigDet State Machine
    // ---------------------------
    AcmClrB.b0   = (State.Q.w8== 0)? 0:1;
    AxAcmEnB.b0  = (State.Q.w8>=18)? 0:1;
    AxAcmEnB.b1  = (State.Q.w8>=50)? 0:1;
    PwAcmEnB.b0  = (State.Q.w8>=50)? 0:1;
    PwAcmEnB.b1  = (State.Q.w8>=82)? 0:1;
    State.D.word = EnB.Q.b0? 0 : ((State.Q.w8==pReg->SigDetDelay.w8)? pReg->SigDetDelay.w8:(State.Q.w8+1));

    // --------------
    // Initialization
    // --------------
    if (!AcmClrB.b0)
    {
        counter=0;
        A0=0; A1=80; A2=64; A3=48; A4=16;
        SigDet.D.word = 0;
    }
    // ----------------------
    // Generate input samples
    // ----------------------
    if (pReg->SigDetFilter.b0)
    {
        x0R.word = (w0R[0].w10 - w0R[2].w10) >> 1;
        x0I.word = (w0I[0].w10 - w0I[2].w10) >> 1;
        x1R.word = (w1R[0].w10 - w1R[2].w10) >> 1;
        x1I.word = (w1I[0].w10 - w1I[2].w10) >> 1;
    }
    else
    {
        x0R.word = w0R[0].w10;
        x0I.word = w0I[0].w10;
        x1R.word = w1R[0].w10;
        x1I.word = w1I[0].w10;
    }
    // -------------------------------------------
    // Write the current input to the delay memory
    // -------------------------------------------
    mem0R[A0].word = x0R.w10;
    mem0I[A0].word = x0I.w10;
    mem1R[A0].word = x1R.w10;
    mem1I[A0].word = x1I.w10;

    // ----------------------------------------
    // Compute Autocorrelation with a Lag of 16
    // ----------------------------------------
    if (PathSel.Q.w2==1)
    {
        ax1R.word= x0R.w10*mem0R[A1].w10 + x0I.w10*mem0I[A1].w10;
        ax1I.word= x0I.w10*mem0R[A1].w10 - x0R.w10*mem0I[A1].w10;

        ax2R.word= mem0R[A2].w10*mem0R[A3].w10 + mem0I[A2].w10*mem0I[A3].w10;
        ax2I.word= mem0I[A2].w10*mem0R[A3].w10 - mem0R[A2].w10*mem0I[A3].w10;
    }
    if (PathSel.Q.w2==2)
    {
        ax1R.word= x1R.w10*mem1R[A1].w10 + x1I.w10*mem1I[A1].w10;
        ax1I.word= x1I.w10*mem1R[A1].w10 - x1R.w10*mem1I[A1].w10;

        ax2R.word= mem1R[A2].w10*mem1R[A3].w10 + mem1I[A2].w10*mem1I[A3].w10;
        ax2I.word= mem1I[A2].w10*mem1R[A3].w10 - mem1R[A2].w10*mem1I[A3].w10;
    }
    if (PathSel.Q.w2==3)
    {
        ax1R.word= (x0R.w10*mem0R[A1].w10 + x0I.w10*mem0I[A1].w10 + x1R.w10*mem1R[A1].w10 + x1I.w10*mem1I[A1].w10)/2;
        ax1I.word= (x0I.w10*mem0R[A1].w10 - x0R.w10*mem0I[A1].w10 + x1I.w10*mem1R[A1].w10 - x1R.w10*mem1I[A1].w10)/2;

        ax2R.word= (mem0R[A2].w10*mem0R[A3].w10 + mem0I[A2].w10*mem0I[A3].w10 + mem1R[A2].w10*mem1R[A3].w10 + mem1I[A2].w10*mem1I[A3].w10)/2;
        ax2I.word= (mem0I[A2].w10*mem0R[A3].w10 - mem0R[A2].w10*mem0I[A3].w10 + mem1I[A2].w10*mem1R[A3].w10 - mem1R[A2].w10*mem1I[A3].w10)/2;
    }
    // -----------------------------------
    // Compute the Squared Samples (Power)
    // -----------------------------------
    if (PathSel.Q.w2==1)
    {
        pw1.word= mem0R[A3].w10*mem0R[A3].w10+mem0I[A3].w10*mem0I[A3].w10;
        pw2.word= mem0R[A4].w10*mem0R[A4].w10+mem0I[A4].w10*mem0I[A4].w10;
    }
    if (PathSel.Q.w2==2)
    {
        pw1.word= mem1R[A3].w10*mem1R[A3].w10+mem1I[A3].w10*mem1I[A3].w10;
        pw2.word= mem1R[A4].w10*mem1R[A4].w10+mem1I[A4].w10*mem1I[A4].w10;
    }
    if (PathSel.Q.w2==3)
    {
        pw1.word= (mem0R[A3].w10*mem0R[A3].w10+mem0I[A3].w10*mem0I[A3].w10 + mem1R[A3].w10*mem1R[A3].w10+mem1I[A3].w10*mem1I[A3].w10)/2;
        pw2.word= (mem0R[A4].w10*mem0R[A4].w10+mem0I[A4].w10*mem0I[A4].w10 + mem1R[A4].w10*mem1R[A4].w10+mem1I[A4].w10*mem1I[A4].w10)/2;
    }
    // ---------------------------------------------------------------------------------------------------
    // Accumulators
    // Worst-case resolution is 1+log2(32*2*512^2) = 25b; Power should be unsigned. Then pw? requires 19b
    // and the accumulator worst-case is 24b. In RTL accumulators should subtract then add.
    // ---------------------------------------------------------------------------------------------------
    if (AcmClrB.b0) // accumulators enabled
    {
        AcmAxR.D.word = (AcmAxR.Q.w25) + (AxAcmEnB.b0? 0:ax1R.w20) - (AxAcmEnB.b1? 0:ax2R.w20);   
        AcmAxI.D.word = (AcmAxI.Q.w25) + (AxAcmEnB.b0? 0:ax1I.w20) - (AxAcmEnB.b1? 0:ax2I.w20);   
        AcmPwr.D.word = (AcmPwr.Q.w24) + (PwAcmEnB.b0? 0:pw1.w19 ) - (PwAcmEnB.b1? 0:pw2.w19 );   
    }
    else           // clear accumulators
    {
        AcmAxR.D.word = 0;
        AcmAxI.D.word = 0;
        AcmPwr.D.word = 0;
    }
    // ------------------
    // Signal Detect Test
    // ------------------
    x.word = (unsigned)((abs(AcmAxR.Q.w25) + abs(AcmAxI.Q.w25)) >> 5);
    t.word = (unsigned)((AcmPwr.Q.w24 * pReg->SigDetTh1.w6) >> 6);

    SigDet.D.b0 = ((State.Q.w8==pReg->SigDetDelay.w8) && (x.w20 >= t.w23) && (x.w20 >= (pReg->SigDetTh2H.w8<<8|pReg->SigDetTh2L.w8)))? 1:0;

    // ----------------------------------------------------------
    // Wire the Output (preclock to accomodate DFE code ordering)
    // ----------------------------------------------------------
    ClockRegister(SigDet);  *SDSigDet = SigDet.Q;

    // -------------
    // Trace Signals
    // -------------
    if (pRX->EnableTrace)
    {
        pRX->traceSigDet[pRX->k20].SDEnB = SDEnB.b0;
        pRX->traceSigDet[pRX->k20].State = State.Q.w8;
        pRX->traceSigDet[pRX->k20].SigDet= SigDet.Q.w2; // only b0 used; b1 available for future use
        pRX->traceSigDet[pRX->k20].x     = x.w21;       
    }
}
// end of aMojave_SignalDetect()

// ================================================================================================
// FUNCTION  : aMojave_CheckFCS
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute and check the 802.11 FCS (algorithm from FrameGen2 program)
// Parameters:
// ================================================================================================
wiStatus aMojave_CheckFCS(wiUInt nBytes, wiUWORD X[], wiUInt *FCSPass)
{
    unsigned int i, j;
    unsigned int p = 0xEDB88320UL;
    unsigned int c = 0xFFFFFFFFUL;
    unsigned int a, b;
    wiUWORD d = {0};

    for (i=0; i<nBytes; i++)
    {
        a = (c & 0xFF) ^ X[i].w8;
        for (j=0; j<8; j++)
            a = p*(a&1) ^ (a>>1);
        b = c >> 8;
        c = a ^ b;
    }

    for (i=0; i<32; i++) {
       d.word = d.word << 1;
       d.b0 = (c&1);
       c = c>>1;
    }
    *FCSPass = (d.word == 0xC704DD7BUL)? 1 : 0;
    return WI_SUCCESS;

}
// end of aMojave_CheckFCS()

// ================================================================================================
// FUNCTION  : aMojave_DFS
// ------------------------------------------------------------------------------------------------
// Purpose   : Detect the presence of non-802.11 signals (specifically radar pulses)
// Timing    : Operates on 20 MHz clock
// Parameters: DFSEnB    -- enable for DFS module (TBD...probably follow DFEEnB)
//             SigIn     -- input signal array
//             SigOut    -- output sinal array
//             pRX, pReg -- pointers to WiSE RX state and configuration register array
// ================================================================================================
void aMojave_DFS(wiBoolean DFSEnB, Mojave_aSigState SigIn, Mojave_aSigState *SigOut, 
                 Mojave_RX_State *pRX, MojaveRegisters *pReg)
{
    if (DFSEnB) return;
    else 
    {
        static wiUReg state;        // state machine; 3 bits
        static wiUReg timer;        // master timer; 19 bits
        static wiUReg wCount;       // pulse width counter; 12 bits
        static wiUReg gCount;       // pulse gap width counter; 12 bits
        static wiUReg pCount;       // in packet detector counter; 8 bits
        static wiUReg inPacket;     // during packet receiving
        static wiUReg restart;      // delayed restart of the FSM when inPacket
        static wiUReg FCSPass;      // FCS (CRC) correct
        static wiUReg radarDet;     // radar detected flag

        wiUWORD PulseStart;         // PulseDet rising edge/RSSI increases
        wiUWORD PulseEnd;           // PulseDet falling edge/RSSI decreases
        wiUWORD dInterval;          // interval error; 18 bits
        wiWORD  dInterval2;         // interval error; 18 bits
        wiUWORD dInterval05;        // interval error; 18 bits
        wiBoolean match;            // shift register pattern matched
        wiBoolean checkFCS=0;       // check (latch) FCS [BB: added initialization]

        int pass;                   // pulse qualifies for favored candidate
        int i, j, bits, zeros[2];
        unsigned int mask;

        static struct {
            wiUReg interval;        // recorded pulse interval; 18 bits
            wiUReg iCount;          // pulse interval counter; 18 bits
            wiUReg tInterval;       // temporary interval memory; 18 bits
            wiUReg pulseSR;         // shift register of pulse candidates, 17 bits
            wiUReg tPulses;         // temporary pulse counter; 3 bits
        } pulse[2]; // pulse candidates

        // ----------------
        // Basic Indicators
        // ----------------
        if (state.Q.word > 0) {
            if (SigIn.HeaderValid) pReg->detHeader.b0   = 1; // indicate valid PLCP header
            if (SigIn.SyncFound)   pReg->detPreamble.b0 = 1; // indicate valid OFDM preamble
            if (SigIn.StepUp)      pReg->detStepUp.b0   = 1; // indicate RSSI step-up event
            if (SigIn.StepDown)    pReg->detStepDown.b0 = 1; // indicate RSSI step-down event
        }

        // clock registers
        ClockRegister(state);
        ClockRegister(timer);
        ClockRegister(wCount);
        ClockRegister(gCount);
        ClockRegister(pCount);
        ClockRegister(inPacket);
        ClockRegister(restart);
        ClockRegister(FCSPass);
        ClockRegister(radarDet);
        for (i=0; i<2; i++) {
            ClockRegister(pulse[i].interval);
            ClockRegister(pulse[i].iCount);
            ClockRegister(pulse[i].tInterval);
            ClockRegister(pulse[i].pulseSR);
            ClockRegister(pulse[i].tPulses);
        }

        // assign default D input of registers
        state.D.word    = state.Q.word;
        timer.D.word    = timer.Q.word;
        wCount.D.word   = wCount.Q.word;
        gCount.D.word   = gCount.Q.word;
        pCount.D.word   = pCount.Q.word;
        inPacket.D.word = inPacket.Q.word;
        restart.D.word  = restart.Q.word;
        FCSPass.D.word  = FCSPass.Q.word;
        radarDet.D.word = radarDet.Q.word;
        for (i=0; i<2; i++) {
            pulse[i].interval.D.word  = pulse[i].interval.Q.word;
            pulse[i].iCount.D.word    = pulse[i].iCount.Q.word;
            pulse[i].tInterval.D.word = pulse[i].tInterval.Q.word;
            pulse[i].pulseSR.D.word   = pulse[i].pulseSR.Q.word;
            pulse[i].tPulses.D.word   = pulse[i].tPulses.Q.word;
        }

        // detect pulse edges
        if ((!SigIn.PulseDet) && SigOut->PulseDet) PulseStart.word = 1;
        else PulseStart.word = 0;
        if (SigIn.PulseDet && (!SigOut->PulseDet)) PulseEnd.word = 1;
        else PulseEnd.word = 0;

        // pattern matching for the candidate shift register
        mask = (1<<pReg->DFSminPPB.w3)-1;
        bits = pReg->DFSminPPB.w3+(4-pReg->DFSPattern.w2);
        for (i=0; i<2; i++) {
            zeros[i] = 0;
            for (j=0; j<bits; j++)
             zeros[i] += !((pulse[i].pulseSR.Q.w17>>j)&1);
        }
        if ((pulse[0].pulseSR.Q.w10&mask) == mask ||
        (pulse[1].pulseSR.Q.w10&mask) == mask) {
            match = 1;
        } else if (pReg->DFSPattern.w2>0) {
            if (zeros[0]==1 || zeros[1]==1)
             match = 1;
            else
             match = 0;
        } else {
            match = 0;
        }

        // interval counters, timers
        if (pReg->DFSOn.b0) {
            timer.D.word = timer.Q.w19  + 1;
            for (i=0; i<2; i++) {
             pulse[i].iCount.D.word = (pulse[i].iCount.Q.word==(1<<18)-1)? (1<<18)-1 : pulse[i].iCount.Q.w18+1;
             if (pulse[i].pulseSR.Q.word > 1) {
                 dInterval2.word = (signed)pulse[i].iCount.Q.w18-(signed)pulse[i].interval.Q.w18;
                 if ((dInterval2.w18>>3) > (signed)(pReg->DFSdTPRF.w4)) {
                            //printf("Pulse erased for %d @ %d!\n", i, pRX->k20);
                            pulse[i].iCount.D.word = pulse[i].iCount.Q.w18-pulse[i].interval.Q.w18+1;
                            if (!(state.Q.word == 1 && match)) {
                         pulse[i].pulseSR.D.word = pulse[i].pulseSR.Q.w17<<1;
                         if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                            }
                 }
             }
            }
        }

        // detect packet receiving
        if (pRX->DFEState[pRX->k20].word == 17) {
            inPacket.D.word = 1;
            checkFCS = 0;
            pCount.D.word = 0;
        } else if (pCount.Q.word == 200) {       // wait 10us after out of 17
            inPacket.D.word = 0;
            checkFCS = 1;
            pCount.D.word = 0;
        } else if (inPacket.Q.word) {
            inPacket.D.word = 1;
            checkFCS = 0;
            pCount.D.word = (pCount.Q.w8==255)? 255 : (pCount.Q.w8+1);
        }

        // ----------------
        // Radar Classifier
        // ----------------
        if (!pReg->DFSOn.b0 || timer.Q.word==((1<<19)-1)) {
            state.D.word = 0;
            timer.D.word = 0;
            pReg->detRadar.word    = 0;
        } 
        else 
        {
            switch (state.Q.word)
            {
                case 0: // idle state/clear indicators and initialize for DFS scan
              pReg->detHeader.word   = 0;
              pReg->detPreamble.word = 0;
              pReg->detStepUp.word   = 0;
              pReg->detStepDown.word = 0;

              timer.D.word   = 0;
              state.D.word   = 1;
              wCount.D.word  = 0;
              gCount.D.word  = 0;
              FCSPass.D.word = 0;
              for (i=0; i<2; i++) {
                        pulse[i].interval.D.word  = 0;
                        pulse[i].iCount.D.word    = 0;
                        pulse[i].pulseSR.D.word   = 0;
                        pulse[i].tInterval.D.word = 0;
                        pulse[i].tPulses.D.word   = 0;
              }
              break;

                case 1: // start scan
              wCount.D.word = 0;
              if (match) {
                        gCount.D.word = 0;
                        if (!inPacket.Q.word) state.D.word = 7;
              } else if (PulseStart.word) {
                        //printf("Pulse found @ %d.\n", pRX->k20);
                        pulse[0].iCount.D.word = 1;
                        pulse[1].iCount.D.word = 1;
                        pulse[0].tInterval.D.word = pulse[0].iCount.Q.w18;
                        pulse[1].tInterval.D.word = pulse[1].iCount.Q.w18;
                        state.D.word = 2;
              }
              break;

                case 2: // end scan
              gCount.D.word = 0;
              wCount.D.word = (wCount.Q.word == 4095)? 4095 : wCount.Q.w12+1;
              if (PulseEnd.word) {
                        if ((wCount.Q.word>>4) > pReg->DFSmaxWidth.w8) {
                      pulse[0].iCount.D.word = pulse[0].iCount.Q.w18+pulse[0].tInterval.Q.w18+1;
                      pulse[1].iCount.D.word = pulse[1].iCount.Q.w18+pulse[1].tInterval.Q.w18+1;
                      state.D.word = pReg->DFSSmooth.b0? 3 : 1;
                        } else {
                      state.D.word = pReg->DFSSmooth.b0? 3 : 4;
                        }
              }
              break;

                case 3: // merge immediately follwoing pulses
              if (!SigIn.PulseDet) gCount.D.word = (gCount.Q.word == 4095)? 4095 : gCount.Q.w12+1;
              else gCount.D.word = 0;
              if (PulseStart.word) {
                        wCount.D.word = (wCount.Q.word+gCount.Q.word >= 4095)? 4095 : wCount.Q.w12+gCount.Q.w12+1;
              } else if (SigIn.PulseDet) {
                        wCount.D.word = (wCount.Q.word == 4095)? 4095 : wCount.Q.w12+1;
              }

              if ((gCount.Q.word>>5) == pReg->DFSminGap.w7) {
                        if ((wCount.Q.word>>4) > pReg->DFSmaxWidth.w8) {
                      pulse[0].iCount.D.word = pulse[0].iCount.Q.w18+pulse[0].tInterval.Q.w18+1;
                      pulse[1].iCount.D.word = pulse[1].iCount.Q.w18+pulse[1].tInterval.Q.w18+1;
                      state.D.word = 1;
                        } else {
                      state.D.word = 4;
                        }
              }
              break;

                case 4: // qualify pulse
              wCount.D.word = 0;
              gCount.D.word = 0;
              state.D.word = pReg->DFS2Candidates.b0? 5:1;
              pass = 0;
              for (i=0; i<=0; i++) {
                        if (pulse[i].pulseSR.Q.word>1) {
                      dInterval.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18);
                      dInterval05.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18/2);
                        } else {
                      dInterval.word = 0;
                      dInterval05.word = 0;
                        }
                        //printf("pulse[%0d,%02hX]: ", i, pulse[i].pulseSR.Q.word);
                        //printf("wc;(p,pc,dp) = %d;(%d,%d,%d)\n", wCount.Q.w12,pulse[i].interval.Q.w18,pulse[i].tInterval.Q.w18,dInterval.w18);
                        //printf("dTPRF=(%d,%d),%d,%d,%d\n",dInterval.word,dInterval05.word,((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8),((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8),((dInterval.w18>>3) <= pReg->DFSdTPRF.word));
                        //printf("bits=%d,mask=%d,zeros=%d\n",bits,mask,zeros[0]);

                        if ((!pulse[i].pulseSR.Q.word ||
                      (((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                       ((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8))) &&
                            ((dInterval.w18>>3) <= pReg->DFSdTPRF.word)) {
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.w17<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      if (pulse[i].pulseSR.Q.word==1) {
                          pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                      }
                        } else if ((pulse[i].pulseSR.Q.word > 1) &&
                            ((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                            ((dInterval05.w18>>3) <= (pReg->DFSdTPRF.word))) {
                      //printf("Erased pulse!\n");
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.b7<<15)+(pulse[i].pulseSR.Q.b6<<13)+(pulse[i].pulseSR.Q.b5<<11)+(pulse[i].pulseSR.Q.b4<<9)+(pulse[i].pulseSR.Q.b3<<7)+(pulse[i].pulseSR.Q.b2<<5)+(pulse[i].pulseSR.Q.b1<<3)+(pulse[i].pulseSR.Q.b0<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                        } else {
                      pulse[i].iCount.D.word = pulse[i].iCount.Q.w18+pulse[i].tInterval.Q.w18+1;
                        }
              }
              if (!pass) {
                        if (pReg->DFS2Candidates.b0) state.D.word = 5;
                        else if (!inPacket.Q.word) state.D.word = 0;
                        else restart.D.word = 1;
              } else if (!pulse[1].pulseSR.Q.word) {
                        state.D.word = 1;
              }
              break;

                case 5: // qualify pulse
              wCount.D.word = 0;
              gCount.D.word = 0;
              state.D.word = 1;
              pass = 0;
              for (i=1; i<=(int)pReg->DFS2Candidates.b0; i++) {
                        if (pulse[i].pulseSR.Q.word>1) {
                      dInterval.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18);
                      dInterval05.word = abs(pulse[i].tInterval.Q.w18-pulse[i].interval.Q.w18/2);
                        } else {
                      dInterval.word = 0;
                      dInterval05.word = 0;
                        }
                        //printf("pulse[%0d,%02hX]: ", i, pulse[i].pulseSR.Q.word);
                        //printf("wc;(p,pc,dp) = %d;(%d,%d,%d)\n", wCount.Q.w12,pulse[i].interval.Q.w18,pulse[i].tInterval.Q.w18,dInterval.w18);
                        //printf("dTPRF=(%d,%d),%d,%d,%d\n",dInterval.word,dInterval05.word,((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8),((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8),((dInterval.w18>>3) <= pReg->DFSdTPRF.word));
                        //printf("bits=%d,mask=%d,zeros=%d\n",bits,mask,zeros[0]);

                        if ((!pulse[i].pulseSR.Q.word ||
                      (((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                       ((pulse[i].tInterval.Q.word>>10) <= pReg->DFSmaxTPRF.w8))) &&
                            ((dInterval.w18>>3) <= pReg->DFSdTPRF.word)) {
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.w17<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      if (pulse[i].pulseSR.Q.word==1) {
                          pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                      }
                        } else if ((pulse[i].pulseSR.Q.word > 1) &&
                            ((pulse[i].tInterval.Q.word>>5) >= pReg->DFSminTPRF.w8) &&
                            ((dInterval05.w18>>3) <= (pReg->DFSdTPRF.word))) {
                      //printf("Erased pulse!\n");
                      pass = 1;
                      pulse[i].pulseSR.D.word = (pulse[i].pulseSR.Q.b7<<15)+(pulse[i].pulseSR.Q.b6<<13)+(pulse[i].pulseSR.Q.b5<<11)+(pulse[i].pulseSR.Q.b4<<9)+(pulse[i].pulseSR.Q.b3<<7)+(pulse[i].pulseSR.Q.b2<<5)+(pulse[i].pulseSR.Q.b1<<3)+(pulse[i].pulseSR.Q.b0<<1)+1;
                      if (inPacket.Q.word) pulse[i].tPulses.D.word = (pulse[i].tPulses.Q.word==7)? 7 : pulse[i].tPulses.Q.w3+1;
                      pulse[i].interval.D.word = pulse[i].tInterval.Q.w18;
                        } else {
                      pulse[i].iCount.D.word = pulse[i].iCount.Q.w18+pulse[i].tInterval.Q.w18+1;
                        }
              }
              if (!pass) {
                        if (!inPacket.Q.word) state.D.word = 0;
                        else restart.D.word = 1;
              }
              break;

                case 7: // check FCS and record radar
              FCSPass.D.word = 0;
              if (FCSPass.Q.word == 1) {
                        state.D.word = 1;
              } else {
                        if (match) {
                      //printf("My_Checker: Radar found! SR=(%02hX,%02hX)\n", pulse[0].pulseSR.Q.w8,pulse[1].pulseSR.Q.w8);
                      radarDet.D.word = 1;
                      pReg->detRadar.word = 1;
                      state.D.word = 0;
                        } else {
                      state.D.word = 1;
                        }
              }
              break;

                default: state.D.word = 0;
            }
        }

        // roll back pulse counter if FCS passes
        if (checkFCS && pRX->N_PHY_RX_WR >= 8) {
            aMojave_CheckFCS(pRX->N_PHY_RX_WR-8, pRX->PHY_RX_D+8, &FCSPass.D.word);
            if (FCSPass.D.word) {
             pulse[0].pulseSR.D.word = pulse[0].pulseSR.Q.w17>>pulse[0].tPulses.Q.w4;
             pulse[1].pulseSR.D.word = pulse[1].pulseSR.Q.w17>>pulse[1].tPulses.Q.w4;
            } else if (restart.Q.word) {
             restart.D.word = 0;
             state.D.word = 0;
            }
            pulse[0].tPulses.D.word = 0;
            pulse[1].tPulses.D.word = 0;
        }

        // -------------
        // Trace Signals
        // -------------
        if (pRX->EnableTrace)
        {
            pRX->traceDFS[pRX->k20].State     = state.Q.w3;
            pRX->traceDFS[pRX->k20].PulseDet  = SigIn.PulseDet;
            pRX->traceDFS[pRX->k20].PulsePass = pass;
            pRX->traceDFS[pRX->k20].CheckFCS  = checkFCS;
            pRX->traceDFS[pRX->k20].FCSPass   = FCSPass.Q.b0;
            pRX->traceDFS[pRX->k20].InPacket  = inPacket.Q.b0;;
            pRX->traceDFS[pRX->k20].Match     = match;
            pRX->traceDFS[pRX->k20].RadarDet  = radarDet.Q.b0;
            pRX->traceDFS[pRX->k20].Counter   = wCount.Q.w12;
            pRX->traceDFS[pRX->k20].PulseSR   = pulse[0].pulseSR.Q.w10;
        }
    }
}
// end of aMojave_DFS()

// ================================================================================================
// FUNCTION  : aMojave_DigitalFrontEnd
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the digital front end
// Parameters: 
// ================================================================================================
void aMojave_DigitalFrontEnd(wiBoolean DFEEnB, wiBoolean DFERestart, wiBoolean DFELgSigAFE,
                             wiWORD  DFEIn0R,  wiWORD  DFEIn0I,  wiWORD  DFEIn1R,  wiWORD  DFEIn1I,
                             wiWORD *DFEOut0R, wiWORD *DFEOut0I, wiWORD *DFEOut1R, wiWORD *DFEOut1I,
                             wiUWORD *DFELNAGain, wiUWORD *DFEAGain,
                             wiUWORD *DFERSSI0, wiUWORD *DFERSSI1, wiUWORD *DFECCA,
                             Mojave_RX_State *pRX, MojaveRegisters *pReg, wiBoolean CSCCK, 
                             wiBoolean RestartRX, Mojave_aSigState *SigOut, Mojave_aSigState SigIn)
{
    // gains
    static wiUWORD LNAGain;      // LNA gain {0->low gain, 1->high gain} b0->LNAGain, b1->LNAGain2
    static wiUWORD AGain;      // VGA gain

    // state machine
    static wiUReg  State;      // controller state
    static wiUReg  counter;      // counter used for state machine
    static wiUReg  SigDetCount;// counter for elapsed time from a signal detect event
    static wiUReg  RSSITimer;  // down counter for period when RSSI is invalid

    // control signals
    static wiUWORD SigDetValid;// flag indicating frame sync has validated signal detection
    static wiUWORD SigDet;     // temporary flag for FlagDS
    static wiUWORD LgSigDFE;   // large signal observed at the digital amp output
    static wiUWORD AGCdone;    // AGC is done
    static wiUWORD PeakFound;  // peak found (frame sync)
    static wiUWORD PeakSearch; // enabling peak search
    static wiUWORD StartCFO;   // start CFO coarse estimation
    static wiUWORD ArmLgSigDet;// arm AFE large signal detector

    static wiUReg  AGCCCA;     // CCA output from AGC (registered in AGC)
    static wiUWORD CCA1;       // CCA based on signal detect and ThCCA1

    static wiUReg  LgSigDetAFE;// AFE LgSigDet
    static wiUReg  oldLNAGain; // register previous LNAGain
    static wiUReg  oldAGain;   // register previous AGain
    static wiUWORD UpdateGains;// AGC should update gains

    static wiUWORD InitGains;  // AGC initialize gains
    static wiUWORD AGCEnB, DAEnB, SDEnB, FSEnB, CFOEnB; // module enable signals

    static int GainUpdateCount;
    static wiUWORD Antenna;

    static wiUWORD UseRecordPwr;
    static wiUWORD RecordPwr;
    static wiUWORD RecordRefPwr;
    static wiUWORD AntSel;
    static wiUWORD AntSearch;
    static wiUWORD RSSI_A, RSSI_B; // for 'C' model only (not in RTL)
    static wiUWORD AntSuggest;

    wiWORD w0R, w0I, w1R, w1I; // reduced resolution version of DFEIn?? for RSSI and SignalDetect
    wiWORD s0R, s0I, s1R, s1I; // wired AGC data output and input for CFO

    wiBoolean GainChange; // flag change in the AGAIN,LNAGAIN values
    wiUWORD DFEPathSel = {pReg->PathSel.w2};

    SigOut->PeakFound  = 0;
    SigOut->Run11b     = 0;
    SigOut->dValidRSSI = SigIn.ValidRSSI; // 20 MHz delayed version of ValidRSSI

    // -----------------------
    // Clock Registers (20MHz)
    // -----------------------
    ClockRegister(State);
    ClockRegister(oldLNAGain);
    ClockRegister(oldAGain);
    ClockRegister(LgSigDetAFE);
    ClockRegister(AGCCCA);

    // -------------------------------------------------------------------------------------
    // Wire Outputs
    // The outputs are connected here to provide cycle accuracy. The state machine executes
    // before the submodules...to make the timing accurate, the output register delay of the
    // submodule is removed to provide the proper timing to the state machine. Placing the
    // outputs prior to the function calls gives the proper delay to the calling function.
    // -------------------------------------------------------------------------------------
    DFECCA->b0 = CCA1.b0;     // order of execution implies CCA1 is a registered output
    DFECCA->b1 = AGCCCA.Q.b1; // explicit register

//("DFEState = %2d, AGAIN=%2u, Count=%3d, DFERestart=%d, InitGains=%d\n",State.Q.word,DFEAGain->w6,counter.Q.w8,DFERestart,InitGains);

    // ---------------
    // Update Counters
    // ---------------
    ClockRegister(counter);
    ClockRegister(SigDetCount); 
    ClockRegister(RSSITimer);
    counter.D.word     = counter.Q.w8 + 1;    // default input is incremented count
    SigDetCount.D.word = SigDetCount.Q.w8 + 1;// default input is incremented count
    
    // --------------------
    // Check for valid RSSI
    // --------------------
    GainChange = (oldAGain.Q.w6!=AGain.w6 || oldLNAGain.Q.w2!=LNAGain.w2)? 1:0;
    if (GainChange || State.Q.w5==19 || State.Q.w5==24 || State.Q.w5==30)
        RSSITimer.D.word = pReg->DelayRSSI.w6;
    else if (RSSITimer.Q.w6) RSSITimer.D.word = RSSITimer.Q.w6 - 1;
    SigOut->ValidRSSI = State.Q.w5 && !GainChange && !RSSITimer.Q.w6;

    // ----------------------------
    // DFE Controller State Machine
    // ----------------------------
    switch (State.Q.word)
    {
        // --------------
        // Initialize DFE
        // --------------
        case 0 :
            SigOut->SigDet      = 0;
            SigOut->AGCdone     = 0;
            SigOut->SyncFound   = 0;
            SigOut->HeaderValid = 0;
            SDEnB.b0        = 1;
            FSEnB.b0        = 1;
            AGCEnB.b0       = 1;
            DAEnB.b0        = 1;
            CFOEnB.b0       = 1;
            InitGains.b0    = 1;
            AGCdone.b0      = 0;
            PeakFound.b0    = 0;
            PeakSearch.b0   = 0;
            UpdateGains.b0  = 0;
            SigDetValid.b0  = 0;
            StartCFO.b0     = 0;
            ArmLgSigDet.b0  = 0;
            UseRecordPwr.b0 = 0;
            RecordPwr.b0    = 0;
            RecordRefPwr.b0 = 0;
            AntSel.b0       = 0;
            if (pReg->OFDMSwDEn.b0)
                if (!pReg->OFDMSwDSave.b0) 
                    Antenna.b0 = pReg->OFDMSwDInit.b0;
                else
                    Antenna.b0 = Antenna.b0;
            else Antenna.b0 = 0;

            CCA1.b0 = 0;
            SigOut->Run11b = 0;
            if (counter.Q.w8==18) // startup delay (includes wait for valid SigDet and CCA)
            {
                State.D.word=1;
                counter.D.word = 0;
            }
            break;

        // ----------------------         
        // Wait for Signal Detect
        // ----------------------         
        case 1 : 
            SigOut->SigDet      = 0;
            SigOut->AGCdone     = 0;
            SigOut->SyncFound   = 0;
            SigOut->HeaderValid = 0;
            CCA1.b0             = 0; // clear the signal detect based CCA
            UseRecordPwr.b0 = 0;
            RecordPwr.b0 = 0;
            AntSel.b0 = 0; RSSI_A.word = 0; RSSI_B.word = 0;
            AGCEnB.b0    = 0; // enable the AGC (incl RSSI)
            DAEnB.b0     = pReg->IdleDAEnB.b0; // enable the digital amplifier
            SDEnB.b0     = pReg->IdleSDEnB.b0; // enable the signal detector
            FSEnB.b0     = pReg->IdleFSEnB.b0; // register selectable enable of FrameSync prior to SigDet
            CFOEnB.b0    = 1; // disable/reset the CFO block
            InitGains.b0 = 0; // allow gains to change from initial values
            ArmLgSigDet.b0 = 0; // clear/squelch large signal detector
            pRX->EVM.nAGC= 0; // clear gain update counter [diagnostic]

            if (SigDet.b0)      // signal detected
            {
                if (pRX->TraceKeyOp) wiPrintf("(SigDet@%d)",pRX->k20);
                State.D.word   = 2;
            }
            else if (pReg->RXMode.w2==3 && CSCCK)
            {
                if (pRX->TraceKeyOp) wiPrintf("('b'SigDet@%d)",pRX->k20);
                State.D.word   = 2;
            }
            counter.D.word  = 0;
            GainUpdateCount = 0;
            SigDetCount.D.word = 0; // clear SigDet counter
            break;

        // --------------------------------------------
        // Delay between Signal Detect and start of AGC
        // --------------------------------------------
        case 2:
            SigOut->SigDet = 1;
            FSEnB.b0     = 0;  // enable the frame sync regardless of state 1 condition
            ArmLgSigDet.b0=1;  // enable latching of large signal detector
            SDEnB.b0 = 1;      // Disable the signal detector
            DAEnB.b0 = 0;      // enable the digital amplifier
            CFOEnB.b0= 0;      // Enable the CFO block
            CCA1.b0  = CCA1.b0 || AGCCCA.Q.b0;   // CCA based on signal detect
            if (LgSigDetAFE.Q.b0) 
                State.D.word = 4;                      // fast start AGC and skip antenna select
            else if (counter.Q.w8==pReg->WaitAGC.w7)    // delay for power measurement after signal detect
                State.D.word = pReg->OFDMSwDEn.b0? 19:4;
            break;

        // -------------------------------------
        // Wait for AGC signal power measurement
        // -------------------------------------
        case 3:
            ArmLgSigDet.b0   = pReg->ArmLgSigDet3.b0; // arm large signal detect for successive update
            CCA1.b0  = CCA1.b0 || AGCCCA.Q.b0;   // CCA based on signal detect
            State.D.word = (counter.Q.w8==15 || LgSigDetAFE.Q.b0)?  4:3;
            break;

        // -----------
        // Record PwrA
        // -----------
        case 19:
            if (AntSearch.b0)
            {
                RecordPwr.b0 = 1;
                RSSI_A = *DFERSSI0;
                CCA1.b0  = CCA1.b0 || AGCCCA.Q.b0;   // CCA based on signal detect
                Antenna.b0 = Antenna.b0 ^ 1;
                FSEnB.b0  = 1; // disable the frame sync
                counter.D.word = 0;
                State.D.word = 20;
            }
            else
                State.D.word = 4;
            break;

        // -----------------------
        // Wait for Antenna Switch
        // -----------------------
        case 20:
            ArmLgSigDet.b0 = 0; // clear large signal detect
            RecordPwr.b0 = 0;
            AntSel.b0 = 1;
            if (counter.Q.w8>pReg->OFDMSwDDelay.w5)
                State.D.word   = 21;
            break;

        // -----------
        // Record PwrB
        // -----------
        case 21:
            counter.D.word = 0;
            RecordPwr.b0 = 1;
            RSSI_B = *DFERSSI0;
            CCA1.b0  = CCA1.b0 || AGCCCA.Q.b0;   // CCA based on signal detect
            State.D.word = 22;
            break;

        // --------------
        // Select Antenna
        // --------------
        case 22:
            RecordPwr.b0 = 0;
            if (counter.Q.w8 == 1) {
                State.D.word = AntSuggest.b0? 23:24;
            }
            break;

        case 23:
            AntSel.b0 = 1;
            UseRecordPwr.b0 = 1;
            State.D.word = 4;
            FSEnB.b0     = 0; // enable the frame sync
            break;

        case 24:
            Antenna.b0 = Antenna.b0 ^ 1;
            AntSel.b0 = 0;
            State.D.word = 25;
            counter.D.word = 0;
            break;

        case 25:
            State.D.word = 25;
            if (counter.Q.w8>pReg->DelayAGC.w5) 
            {
                FSEnB.b0     = 0; // enable the frame sync
                State.D.word = 4;
            }
            break;

        // ------------------------------
        // Update the AGC-controlled gain
        // ------------------------------
        case 4:
            ArmLgSigDet.b0 = 1; // arm large signal detect
            SigOut->SigDet = 1;
            UseRecordPwr.b0 = 0; // (add for SwD)
            FSEnB.b0     = 0; // enable the frame sync (add for SwD)
            CCA1.b0  = CCA1.b0 || AGCCCA.Q.b0;   // CCA based on signal detect
            UpdateGains.b0 = 1;
            State.D.word   = 5;
            GainUpdateCount++;
            pRX->EVM.nAGC++; // increment gain update counter [diagnostic]
            break;

        // --------------------------------------------------
        // Wait for gain update propagation through registers
        // --------------------------------------------------
        case 5:
            ArmLgSigDet.b0 = 0;    // clear large signal detect value
            UpdateGains.b0 = 0; 
            State.D.word = 6;
            break;

        // -----------------------------------
        // Check the result of the gain update
        // -----------------------------------
        case 6:
        {
            wiBoolean GainChange;

            counter.D.word = 0;
            ArmLgSigDet.b0 = 0;  // clear latched large sig det value
            GainChange = (oldAGain.Q.w6!=AGain.w6 || oldLNAGain.Q.w2!=LNAGain.w2)? 1:0;
                 if (LgSigDFE.b0) State.D.word = 7; // nonlinear measurement
            else if (GainChange ) State.D.word = 8; // linear measurement, last update
            else                 State.D.word = 9; // no analog gain change
        }  break;

        // --------------------------------------------------------------
        // Wait for the gain update to propagate to the DFE
        // ...then continue AGC because the update was for a large signal
        // --------------------------------------------------------------
        case 7:
            CCA1.b0  = 1; // CCA based on signal detect
            FSEnB.b0 = 1; // disable frame sync on gain change [NEW: BB, restart after AGC]
            if (counter.Q.w8==pReg->DelayAGC.w5)
            {
                State.D.word   = 3;
                counter.D.word = 0;
            }
            break;

        // ----------------------------------------------------------------------
        // Wait for the gain update to propagate to the DFE
        // ...then wait for the FrameSync (gain update was for a moderate signal)
        // ----------------------------------------------------------------------
        case 8:
            FSEnB.b0 = 1; // disable frame sync on gain change [NEW: BB, restart after AGC]
            if (counter.Q.w8==pReg->DelayAGC.w5)
            {
                State.D.word   = 10;
                counter.D.word = 0;
            }
            break;

        // ----------------------------------------------------
        // AGC is complete (no update, so no propagation delay)
        // ...wait for FrameSync
        // ----------------------------------------------------
        case 9:
            AGCdone.b0     = 1;
            State.D.word   = 10;
            counter.D.word = 0;
            break;

        // -----------------------------------------------------
        // AGC is complete...delay start of peak search (>= 16T)
        // -----------------------------------------------------
        case 10:
            FSEnB.b0         = 0; // enable the frame sync [NEW: BB, restart after AGC]
            AGCdone.b0       = 1; // flag AGC complete
            if (counter.Q.w8==pReg->DelayCFO.w5) State.D.word=11;
            break;

        // ---------------------
        // Start the peak search
        // ---------------------
        case 11:
            RecordRefPwr.b0 = 1;  // record power reference for StepUp/StepDown signals
            SigOut->AGCdone = 1;  // signal gain update complete
            StartCFO.b0     = 1;  // start coarse CFO estimation
            PeakSearch.b0   = 1;  // start frame synchronization
            counter.D.word  = 0;
            State.D.word    = 12;
            break;

        // --------------------------------------------------------------
        // Wait for the FrameSync PeakFound signal or SigDetValid timeout
        // --------------------------------------------------------------
        case 12:
            StartCFO.b0  = 0;
            RecordRefPwr.b0 = 0; // signal AGC to record packet power level
            CCA1.b0      = CCA1.b0 || AGCCCA.Q.b0;   // CCA based on signal detect

            if (RestartRX)
            {
                State.D.word = 26;
            }
            else if (PeakFound.b0)      
            {
                SigOut->SyncFound=  1; // signal synchronization done
                State.D.word     = 15; 
                counter.D.word   =  0;
            }
            else if (!SigDetValid.b0 && (SigDetCount.Q.w8==pReg->SigDetWindow.w8))
            {
                State.D.word   = 13; // not a valid 802.11a packet
                counter.D.word = 0;
                pRX->EventFlag.b4 = 1;
            }
            break;

        // ------------------------------------------------------
        // OFDM Qualifier Expired: DSSS preamble or False Detect?
        // ------------------------------------------------------
        case 13:
            AGCdone.b0    =  0; // reset AGCdone flag
            PeakSearch.b0 =  0; // turn off peak search
            FSEnB.b0      =  1; // disable frame sync
            SigOut->SigDet=  0; // clear "a" SigDet
            if (RestartRX)
            {
                State.D.word = 26;
            }
            else if (pReg->RXMode.w2==3 && CSCCK)
            {
                SigOut->Run11b = 1;
                State.D.word = 28;
            }
            else
            {
                InitGains.b0 =  1; // reset gain if not a DSSS preamble
                State.D.word = 14;
            }
            break;

        // ----------------------------------
        // Recover from a false signal detect
        // ----------------------------------
        case 14:
            CCA1.b0  = 0; // clear signal detect CCA
            SigOut->Run11b    = 0; // stop any DSSS/CCK processing
            if (counter.Q.w8==pReg->DelayAGC.w5)
            {
                State.D.word   = 1;
                counter.D.word = 0;
            }
            break;

        // ----------------------------------------------------------
        // FrameSync peak found...delay to give desired FFT alignment
        // ----------------------------------------------------------
        case 15:
            SigOut->SigDet = 0; // clear SigDet; SyncFound is active
            FSEnB.b0 = 1;       // disable frame sync;
            PeakSearch.b0 =  0; // turn off peak search
            if (RestartRX)
                State.D.word = 26;
            else 
                if (counter.Q.w8==pReg->SyncShift.w4) State.D.word = 16;
            break;

        // ----------------------------------------------------
        // FrameSync complete...continue with packet processing
        // ----------------------------------------------------
        case 16:
            SigOut->PeakFound = 1; // flag peak found from DFE to RX controller
            StartCFO.b0   =  1; // start fine CFO estimation
            State.D.word  = 17;
            pRX->EventFlag.word = pRX->EventFlag.word | (GainUpdateCount << 14);
            break;

        // ----------------------------------
        // OFDM Packet: Demodulate and Decode
        // ----------------------------------
        case 17:
            SigOut->PeakFound = 0;
            StartCFO.b0       = 0;
            counter.D.word    = 0;
            SigDetCount.D.word= 0;
            if (RestartRX )      State.D.word = 26; // abort restart
            else if (DFERestart) State.D.word = 29; // normal restart
            break;

        // --------------------------------------
        // DSSS/CCK Packet: Demodulate and Decode
        // --------------------------------------
        case 28:
            SigOut->PeakFound = 0;
            SigOut->Run11b    = 1;
            DAEnB.b0          = 1; // disable Digital Amp
            CFOEnB.b0         = 1; // disable OFDM CFO
            StartCFO.b0       = 0;
            counter.D.word    = 0;
            SigDetCount.D.word= 0;
            if (RestartRX )      State.D.word = 26; // abort restart
            else if (DFERestart) State.D.word = 29; // normal restart
            break;

        // ---------------------------------------
        // Packet Abort/DFE Restart for New SigDet
        // ---------------------------------------
        case 26:
            SigOut->SyncFound = 0; // clear sync indicator on restart
            ArmLgSigDet.b0 = 1; // arm large signal detect
            SigOut->Run11b = 0; // turn off "b" baseband processing
            SigOut->SigDet = 0; // clear SigDet on restart event
            SigOut->AGCdone= 0; // clear AGCdone indicator
            CFOEnB.b0      = 1; // disable/reset the CFO block
            FSEnB.b0       = 1; // disable/reset FrameSync
            DAEnB.b0       = 0;     // enable the digital amplifier
            PeakSearch.b0  = 0; // turn off peak search
            InitGains.b0   = 1; // reset AGC gains to initial values
            GainUpdateCount= 0; // WiSE only
            AGCdone.b0     = 0; // reset AGCdone flag
            State.D.word   = 27;
            break;

        case 27:
            counter.D.word = 0; // clear the counter
            CFOEnB.b0      = 0; // Enable the CFO block
            InitGains.b0   = 0; // clear InitGains from state 26
            SigOut->SyncFound = 0; // clear sync indicator on restart
            CCA1.b0 = SigIn.StepUp && AGCCCA.Q.b0; // set signal detect CCA
            if (SigIn.StepUp)
                State.D.word=  3; // start AGC (assume new packet detect)
            else
                State.D.word= 31; // restart DFE
            break;

        // -----------------------------
        // DFE Restart States (RX-to-RX)
        // -----------------------------
        case 29:
            SigOut->SyncFound = 0; // clear sync indicator on restart
            AGCdone.b0    =  0; // reset AGCdone flag
            PeakSearch.b0 =  0; // turn off peak search
            FSEnB.b0      =  1; // disable frame sync to reset
            DAEnB.b0 = 0;     // enable the digital amplifier
            CFOEnB.b0     = SigIn.Run11b; // =Run11b.Q (enabled for OFDM)
            SigOut->Run11b= SigIn.Run11b; // hold state of Run11b
            CCA1.b0       =  0; // clear signal detect CCA
            SigOut->SigDet = 0; // clear the signal detect flag
            if (counter.Q.w8==pReg->DelayRestart.w8)
                State.D.word = 30;
            break;

        case 30:
            SigOut->AGCdone=  0; // clear AGCdone indicator
            InitGains.b0   =  1; // reset AGC gains to initial values
            CFOEnB.b0      =  1; // disable OFDM CFO
            counter.D.word =  0; // clear counter
            State.D.word   = 31;
            break;

        case 31:
            ArmLgSigDet.b0= 0; // clear large signal detect
            InitGains.b0  = 0; // clear gain initialization
            FSEnB.b0      = 1; // disable FrameSync during gain settling
            SigOut->Run11b    = 0; // end of any DSSS/CCK processing
            if (counter.Q.w8==pReg->DelayAGC.w5)
            {
                State.D.word   = 1;
                counter.D.word = 0;
            }
            break;

        // --------------------------------------
        // Default case: return to initialization
        // --------------------------------------
        case 18: State.D.word = 0; break;
        default: State.D.word = 0;
    }
    // ----------------------------------
    // Initialize when DFE is NOT Enabled
    // ----------------------------------
    if (DFEEnB)
    {
        State.D.word   = 0;
        counter.D.word = 0;
    }
    oldLNAGain.D.word = LNAGain.w2;
    oldAGain.D.word   = AGain.w6;

    // ---------------------------------------------
    // Reduced Resolution for RSSI and Signal Detect
    // ---------------------------------------------
    w0R.word = max(-511,min(511,DFEIn0R.w12>>1));
    w0I.word = max(-511,min(511,DFEIn0I.w12>>1));
    w1R.word = max(-511,min(511,DFEIn1R.w12>>1));
    w1I.word = max(-511,min(511,DFEIn1I.w12>>1));
    
    // ----------------------
    // Automatic Gain Control
    // ----------------------
    aMojave_AGC( AGCEnB, DAEnB, DFEPathSel, InitGains, UpdateGains, LgSigDetAFE.Q.b0, UseRecordPwr, RecordPwr, RecordRefPwr,
                AntSel, &AntSuggest, &AntSearch, DFEIn0R, DFEIn0I, DFEIn1R, DFEIn1I, &LNAGain, &AGain, &LgSigDFE, &AGCCCA, 
                DFERSSI0, DFERSSI1, &s0R, &s0I, &s1R, &s1I, w0R, w0I, w1R, w1I, pReg, pRX, SigOut);

    // ---------------
    // Signal Detector
    // ---------------
    aMojave_SignalDetect(SDEnB, DFEPathSel, w0R, w0I, w1R, w1I, &SigDet, pRX, pReg);

    // ---------------------
    // Frame Synchronization
    // ---------------------
    aMojave_FrameSync(FSEnB, DFEPathSel, AGCdone, PeakSearch, s0R, s0I, s1R, s1I, &PeakFound, &SigDetValid, pRX, pReg);

    // ------------------------
    // Carrier Frequency Offset
    // ------------------------
    aMojave_CFO(CFOEnB, DFEPathSel, StartCFO, PeakFound, s0R, s0I, s1R, s1I, DFEOut0R, DFEOut0I, DFEOut1R, DFEOut1I, pRX, pReg);

    // ---------------------------------------
    // Qualify/Latch AFE Large Signal Detector
    // ---------------------------------------
    LgSigDetAFE.D.word = ArmLgSigDet.b0 & (DFELgSigAFE || LgSigDetAFE.Q.b0) & pReg->LgSigAFEValid.b0;

    // -----------------------
    // Save Intermediate State
    // -----------------------
    pRX->wReal[0][pRX->k20] = w0R;
    pRX->wImag[0][pRX->k20] = w0I;
    pRX->wReal[1][pRX->k20] = w1R;
    pRX->wImag[1][pRX->k20] = w1I;

    pRX->sReal[0][pRX->k20] = s0R;
    pRX->sImag[0][pRX->k20] = s0I;
    pRX->sReal[1][pRX->k20] = s1R;
    pRX->sImag[1][pRX->k20] = s1I;

    // -----------------------
    // Wire Registered Outputs
    // -----------------------
    DFEAGain->word   = AGain.w6;
    DFELNAGain->word = LNAGain.w2;
    SigOut->AntSel   = Antenna.b0;

    // -------------
    // Trace Signals
    // -------------
    if (pRX->EnableTrace)
    {
        pRX->AGain[pRX->k20]        = AGain;
        pRX->DFEState[pRX->k20]     = State.Q;

        pRX->DFEFlags[pRX->k20].b0  = LNAGain.b0;
        pRX->DFEFlags[pRX->k20].b1  = AGCEnB.b0;
        pRX->DFEFlags[pRX->k20].b2  = SDEnB.b0;
        pRX->DFEFlags[pRX->k20].b3  = FSEnB.b0;
        pRX->DFEFlags[pRX->k20].b4  = DFELgSigAFE;
        pRX->DFEFlags[pRX->k20].b5  = 0;
        pRX->DFEFlags[pRX->k20].b6  = InitGains.b0;
        pRX->DFEFlags[pRX->k20].b7  = SigDet.b0;
        pRX->DFEFlags[pRX->k20].b8  = SigDetValid.b0;
        pRX->DFEFlags[pRX->k20].b9  = UpdateGains.b0;
        pRX->DFEFlags[pRX->k20].b10 = AGCdone.b0;
        pRX->DFEFlags[pRX->k20].b11 = PeakSearch.b0;
        pRX->DFEFlags[pRX->k20].b12 = PeakFound.b0;
        pRX->DFEFlags[pRX->k20].b13 = SigOut->PeakFound;
        pRX->DFEFlags[pRX->k20].b14 = DFECCA->b0;
        pRX->DFEFlags[pRX->k20].b15 = DFEEnB;
        pRX->DFEFlags[pRX->k20].b16 = LNAGain.b1;
        pRX->DFEFlags[pRX->k20].b17 = DFECCA->b1;
        pRX->DFEFlags[pRX->k20].b18 = RestartRX;
        pRX->DFEFlags[pRX->k20].b19 = CSCCK;
        pRX->DFEFlags[pRX->k20].b31 = 0;

        pRX->traceDFE[pRX->k20].traceValid = 1;
        pRX->traceDFE[pRX->k20].DFEEnB     = DFEEnB;
        pRX->traceDFE[pRX->k20].State      = State.Q.w6;
        pRX->traceDFE[pRX->k20].LgSigAFE   = DFELgSigAFE;
        pRX->traceDFE[pRX->k20].AntSel     = SigOut->AntSel;
        pRX->traceDFE[pRX->k20].RestartRX  = RestartRX; 
        pRX->traceDFE[pRX->k20].CCA        = DFECCA->w2;
        pRX->traceDFE[pRX->k20].CSCCK      = CSCCK;
        pRX->traceDFE[pRX->k20].Run11b     = SigIn.Run11b;
        pRX->traceDFE[pRX->k20].counter    = counter.Q.w8;
        pRX->traceDFE[pRX->k20].PeakFound  = SigIn.PeakFound;
    }
}
// end of aMojave_DigitalFrontEnd()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// 2006-10-16 Brickner: Adjusted CCA and RSSITimer to reduce bad PHY_CCA at the end of the packet
// 2008-12-20 Brickner: Add explicit (int) cast to product of unsigned * signed terms in 
//                      aMojave_DigitalAmp_Core().

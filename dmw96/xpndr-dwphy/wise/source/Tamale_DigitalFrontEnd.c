//--< Tamale_DigitalFrontEnd.c >-------------------------------------------------------------------
//=================================================================================================
//
//  Tamale - OFDM Digital Front End
//  Copyright 2001-2003 Bermai, 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wiType.h"
#include "wiUtil.h"
#include "Tamale.h"

//=================================================================================================
//--  MACROS
//=================================================================================================
#ifndef ClockRegister
#define ClockRegister(arg) {(arg).Q=(arg).D;};
#endif

#ifndef ClockDelayLine
#define ClockDelayLine(SR,value) {(SR).word = ((SR).word << 1) | ((value)&1);};
#endif

#ifndef log2
#define log2(x) (log((double)(x))/log(2.0))
#endif

// ================================================================================================
// FUNCTION  : Tamale_CFO_Angle
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the angle for a cartesian coordinate
// Parameters: 
// ================================================================================================
int Tamale_CFO_Angle(wiWORD b, wiWORD a)
{
    wiUWORD x, y, z, yz; wiWORD phi;

    /////////////////////////////////////////////////////////////////////////////////////
    // 
    //  Handle undefined and angles at 90 degree intervals
    //
    if (!a.word && !b.word) return 0;            // undefined
    if (!a.word) return (b.word<0) ? -2048:2048; // -/+ pi/2
    if (!b.word) return (a.word<0) ? -4095:0;    // -/+ pi

    /////////////////////////////////////////////////////////////////////////////////////
    // 
    //  Reduce lookup table range to 0 <= y/x <= 1
    //
    //  If the input is not on the range (0, 45 deg), the input is converted to this
    //  range to reduce complexity of the atan calculation below.
    // 
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
    /////////////////////////////////////////////////////////////////////////////////////
    // 
    //  Reduce resolution for arctan argument
    //
    //  x and y are scaled so that the divider in the arctan argument is a 9-bit
    //  unsigned integer on the range 256 < x <= 512.
    // 
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
    /////////////////////////////////////////////////////////////////////////////////////
    // 
    //  Compute phi = atan(y/x)
    //
    z.word   = (int)((1<<18) / ((double)(x.word<<1)+1)+0.5);
    yz.word  = (y.w9 * z.w9) >> 7;
    phi.word = (int)((4096/3.14159265359)*atan(yz.word/1024.0) + 0.5);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Convert 45 degreesegement into full circle angle
    //  
    //  The computed value is mapped onto a full circle by examining the relative signs
    //  of the input terms a and b.
    // 
    if (abs(a.word) < abs(b.word)) phi.word = 2048 - phi.w13;

         if (a.word<0 && b.word>0) phi.word = 4096 - phi.w13;
    else if (a.word<0 && b.word<0) phi.word = 4096 + phi.w13;
    else if (a.word>0 && b.word<0) phi.word = 8192 - phi.w13;

    return (phi.w13);
}
// end of Tamale_CFO_Angle()

// ================================================================================================
// FUNCTION  : Tamale_CFO
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
void Tamale_CFO( wiUWORD CFOEnB, wiUWORD CFOPathSel, wiUWORD CFOStart, wiUWORD CFOEnd,
                 wiWORD  CFODI0R, wiWORD  CFODI0I, wiWORD  CFODI1R, wiWORD  CFODI1I,
                 wiWORD *CFODO0R, wiWORD *CFODO0I, wiWORD *CFODO1R, wiWORD *CFODO1I,
                 Tamale_RxState_t *pRX, TamaleRegisters_t *pReg )
{
    wiWORD axR,axI;         // correlator output
    wiWORD anginR,anginI;   // input to angle operator
    wiWORD phi;            // output from angle operator, unit pi
    wiWORD cR,cI;         // CFO correction terms
    int    i,j,k;
    double angle;

    wiWORD y0R, y0I, y1R, y1I;

    Tamale_CFO_t *p = &(pRX->CFOState);

    wiWORD *w0R = p->w0R;
    wiWORD *w0I = p->w0I;
    wiWORD *w1R = p->w1R;
    wiWORD *w1I = p->w1I;

    wiReg *x0R = p->x0R;
    wiReg *x0I = p->x0I;
    wiReg *x1R = p->x1R;
    wiReg *x1I = p->x1I;

    // ---------------
    // Clock Registers
    // ---------------
    ClockRegister(p->s0R); p->s0R.D.word = CFOEnB.b0 ? 0:CFODI0R.w12;
    ClockRegister(p->s0I); p->s0I.D.word = CFOEnB.b0 ? 0:CFODI0I.w12;
    ClockRegister(p->s1R); p->s1R.D.word = CFOEnB.b0 ? 0:CFODI1R.w12;
    ClockRegister(p->s1I); p->s1I.D.word = CFOEnB.b0 ? 0:CFODI1I.w12;

    for (i=0; i<2; i++)
    {
        ClockRegister(x0R[i]); ClockRegister(x0I[i]);
        ClockRegister(x1R[i]); ClockRegister(x1I[i]);
    }
    ClockRegister(p->startCFO);
    ClockRegister(p->endCFO);
    ClockRegister(p->step);
    ClockRegister(p->theta);
    ClockRegister(p->state);
    ClockRegister(p->counter); p->counter.D.word = p->counter.Q.word + 1;
    k = p->k = (p->k+1) % 80;

    // -----------
    // wire inputs
    // -----------
    p->startCFO.D = CFOStart;
    p->endCFO.D   = CFOEnd;

    // ------------
    // Wire Outputs
    // ------------
    *CFODO0R = x0R[0].Q;
    *CFODO0I = x0I[0].Q;
    *CFODO1R = x1R[0].Q;
    *CFODO1I = x1I[0].Q;

    angle   = (3.14159265359/4096.0) * ((double)(p->theta.Q.w17>>4));

    cR.word = (int)floor(511*cos(angle) + 0.5);
    cI.word = (int)floor(511*sin(angle) + 0.5);

    // --------------------------------------------------
    // TEST FEATURE: Disable CFO Correction (not in ASIC)
    // --------------------------------------------------
    if (pRX->NonRTL.Enabled && pRX->NonRTL.DisableCFO) {cR.word = 511; cI.word = 0;}

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  CFO Correction
    //
    //  The CFO corrector multiplies the input signal (s?R + j*s?I) by the correction
    //  mixer term (cR + j*cI). The input is a 12b word and the corrector is 10b. After
    //  dividing by 256, the output is 14b though the maximum value should be only 13.5b
    //  because the corrector is a sin/cos pair.
    //
    x0R[0].D.word = (p->s0R.Q.w12*cR.word + p->s0I.Q.w12*cI.word) / 256;
    x0I[0].D.word = (p->s0I.Q.w12*cR.word - p->s0R.Q.w12*cI.word) / 256;
    x1R[0].D.word = (p->s1R.Q.w12*cR.word + p->s1I.Q.w12*cI.word) / 256;
    x1I[0].D.word = (p->s1I.Q.w12*cR.word - p->s1R.Q.w12*cI.word) / 256;

    if (CFOEnB.b0)
    {
        phi.word = 0;
        p->AcmAxR.word = 0;
        p->AcmAxI.word = 0;

        p->theta.D.word = 0;
        p->theta.Q.word = 0;
        p->step.D.word  = 0;
        p->step.Q.word  = 0;

        p->state.D.word = 0;
        p->state.Q.word = 0;
        p->counter.D.word = 0;
        p->k = 0;
        return;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Controller State Machine
    //
    switch (p->state.Q.word)
    {
        case 0:
            p->state.D.word = 1;
            p->debug_nT = 0;
        case 1:
            if (p->startCFO.D.b0)
            {
                p->counter.D.word = 0;
                p->state.D.word = 21;
            }
            break;
        case 21:
            if (p->endCFO.Q.b0)
            {
                p->counter.D.word = 0;
                p->state.D.word = 41;
            }
            else
            {
                p->state.D.word = 2; p->counter.D.word=0;
            }
            break;
        case 2:
            if (p->endCFO.Q.b0)
            {
                p->counter.D.word = 0;
                p->state.D.word = 41;
            }
            else if (p->counter.Q.word == (unsigned)(pReg->CFOFilter.b0 ? 31:30))
            {
                p->counter.D.word = 0;
                p->state.D.word = 3;
            }
            break;
        case 3:
            if (p->endCFO.Q.b0)
            {
                p->counter.D.word = 0;
                p->state.D.word = 41;
            }
            break;

        case 4:  p->state.D.word = 41; break;
        case 41: p->state.D.word = 5; break;
        case 5:  p->state.D.word = 6; break;

        case 6:
            if (p->startCFO.D.b0)
            {
                p->counter.D.word = 0;
                p->state.D.word = 7;
            }
            break;
        case 7:
            if (p->counter.Q.word == (unsigned)(pReg->CFOFilter.b0 ? 63:62))
            {
                p->counter.D.word = 0;
                p->state.D.word = 8;
            }                                                                                
            break;
        case 8:
            if (p->counter.Q.word == (unsigned)(pReg->CFOFilter.b0 ? 62:63))
            {
                p->counter.D.word = 0;
                p->state.D.word = 9;
            }
            break;

        case 9 : p->state.D.word = 10; break;
        case 10: p->state.D.word = 11; break;
        case 11: break;

        default: p->state.D.word = 0;
    }

    x0R[1].D = x0R[0].Q;
    x0I[1].D = x0I[0].Q;
    x1R[1].D = x1R[0].Q;
    x1I[1].D = x1I[0].Q;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Filter and Generate Low-Resolution Samples for CFO Estimation
    //
    //  An optional H(D) ~ (1 - D^2)/2 filter attenuates signals at DC and 1/2T.
    //
    //  Resolution of the samples used for estimation is reduced to save power/area.
    //  A lower resolution is acceptable because (1) the estimator requires less SNR
    //  than data decoding and (2) the samples used for estimation have a lower peak-to-
    //  average-amplitude than general OFDM symbols.
    //
    y0R.word = pReg->CFOFilter.b0 ? (x0R[0].D.w14>>1) - (x0R[1].Q.w14>>1) : x0R[0].D.w14;
    y0I.word = pReg->CFOFilter.b0 ? (x0I[0].D.w14>>1) - (x0I[1].Q.w14>>1) : x0I[0].D.w14;
    y1R.word = pReg->CFOFilter.b0 ? (x1R[0].D.w14>>1) - (x1R[1].Q.w14>>1) : x1R[0].D.w14;
    y1I.word = pReg->CFOFilter.b0 ? (x1I[0].D.w14>>1) - (x1I[1].Q.w14>>1) : x1I[0].D.w14;

    w0R[k].word = abs(y0R.w14) < 1024 ? (y0R.w11/8) : (y0R.w14 < 0 ? -127:127);
    w0I[k].word = abs(y0I.w14) < 1024 ? (y0I.w11/8) : (y0I.w14 < 0 ? -127:127);
    w1R[k].word = abs(y1R.w14) < 1024 ? (y1R.w11/8) : (y1R.w14 < 0 ? -127:127);
    w1I[k].word = abs(y1I.w14) < 1024 ? (y1I.w11/8) : (y1I.w14 < 0 ? -127:127);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Coarse CFO Estimation
    //
    //  CFO is estimated by determining the angle for the autocorrelation of a periodic
    //  signal (in this case, the short preamble field). The frequency error is computed
    //  based on the phase created at a particular autocorrelation delay. In this section
    //  the autocorrelation is computed in the short preamble. The lag is 16T, which is
    //  the period of the short preamble symbols at 20 MHz.
    //  
    //  The "NotRTL" section allows the number of samples to be limited. This is a
    //  simulation feature; in RTL and for normal operation, the accumulator operates
    //  during states 3 and 4.
    // 
    if (p->state.Q.word == 3 || p->state.Q.word == 4)
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
        if (!pRX->NonRTL.Enabled || (p->debug_nT<pRX->NonRTL.MaxSamplesCFO)) // if (1) in RTL
        {                                                                 
            p->AcmAxR.word = p->AcmAxR.w23 + axR.w16;
            p->AcmAxI.word = p->AcmAxI.w23 + axI.w16;
            p->debug_nT++; //[WiSE Only]
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Fine CFO Estimation
    //
    //  This is similar to coarse CFO estimation except that it operates on the long 
    //  preamble (L-LTF) field. The two LTF symbols are separated by 64T at 20 MHz.
    //
    if (p->state.Q.word == 8)
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
        p->AcmAxR.word = p->AcmAxR.w23 + axR.w16;
        p->AcmAxI.word = p->AcmAxI.w23 + axI.w16;
    }
    // ------------------------- 
    // Compute Coarse Correction
    // ------------------------- 
    if (p->state.Q.word == 5)
    {
        anginR.word = p->AcmAxR.w23;
        anginI.word = p->AcmAxI.w23;

        // ---------------
        // Calculate Angle
        // ---------------
        phi.word   = Tamale_CFO_Angle(anginI, anginR);

        p->theta.D.word = 2 * phi.w13;
        p->step.D.word  = phi.w13;

        p->AcmAxR.word = 0;
        p->AcmAxI.word = 0;
    }
    // -----------------------
    // Compute Fine Correction
    // -----------------------
    else if (p->state.Q.word == 10)
    {
        anginR.word=p->AcmAxR.w23;
        anginI.word=p->AcmAxI.w23;

        // ---------------
        // calculate angle
        // ---------------
        phi.word        = Tamale_CFO_Angle(anginI, anginR)/4;
        p->theta.D.word = p->theta.Q.w17 + p->step.Q.word + (64 + 3) * (phi.w13);
        p->step.D.word  = p->step.Q.w17 + phi.w13;

        p->AcmAxR.word = 0;
        p->AcmAxI.word = 0;

        pRX->CFO.word = abs(p->step.D.w14)<2048 ? (p->step.D.w14>>4) : (p->step.D.b13 ? -127:127); // store 8-bit version of estimate
    }
    else
    {
        p->step.D = p->step.Q;
        p->theta.D.word = p->theta.Q.w17 + p->step.Q.w17;
    }
}
// end of Tamale_CFO()

// ================================================================================================
// FUNCTION  : Tamale_FS_UIntToFloat
// ------------------------------------------------------------------------------------------------
// Purpose   : Convert an unsigned integer to floating point
// Parameters: 
// ================================================================================================
void Tamale_FS_UIntToFloat(wiUWORD *significand, wiWORD *exponent, unsigned int t)
{
    int i;
    if (t > 1023)
    {
        for (i=0; t>1023; i++) t=t>>1;
        significand->word = t;
        exponent->word = i;
    }
    else if (t > 0)
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
// end of Tamale_FS_UIntToFloat() 

// ================================================================================================
// FUNCTION  : Tamale_FrameSync
// ------------------------------------------------------------------------------------------------
// Purpose   : Synchronize the receiver with the OFDM symbol frame
// Parameters: FSEnB         -- Enable the circuit, active low (input)
//             FSPathSel     -- Select the active digital paths (2 bit input)
//             FSAGCDone     -- Flag to indicate gain updates have completed (1 bit input)
//             FSPeakSearch  -- Enables the peak search algorithm (1 bit input)
//             FSDI0R        -- Signal input, path 0, real component (12->10 bit input)
//             FSDI0I        -- Signal input, path 0, imag component (12->10 bit input)
//             FSDI1R        -- Signal input, path 1, real component (12->10 bit input)
//             FSDI1I        -- Signal input, path 1, imag component (12->10 bit input)
//             FSPeakFound   -- Indicates the peak has been located (1 bit output)
//             FSSigDetValid -- Indicates the signal detect is valid (1 bit ouput)
//             pReg          -- Pointer to modeled register file (model input)
// ================================================================================================
void Tamale_FrameSync( wiUWORD FSEnB, wiUWORD FSPathSel, wiUWORD FSAGCDone, wiUWORD FSPeakSearch,
                       wiWORD FSDI0R, wiWORD FSDI0I, wiWORD FSDI1R, wiWORD FSDI1I, wiUWORD *FSPeakFound, 
                       wiUWORD *FSSigDetValid, Tamale_RxState_t *pRX, TamaleRegisters_t *pReg )
{
    wiWORD            x0R;         // real, received signal, first path
    wiWORD            x0I;         // imaginary, received signal, first path
    wiWORD            x1R;         // real, received signal, second path
    wiWORD            x1I;         // imaginary, received signal, second path
    wiWORD            y0R, y0I, y1R, y1I;
    wiWORD            ax1R,ax1I,ax2R,ax2I;   // correlator output
    wiWORD            pw1,pw2,pw3,pw4;      // squared magnitude of signal

    wiWORD            PwSigExp;   // exponent for the product of Pw1Sig * Pw2Sig
    wiWORD            RatioSigExp;// exponent for 

    wiUWORD AxRSig,AxISig; wiWORD AxRExp,AxIExp; // significand and exponent of Ax?Acm
    wiUWORD Pw1Sig,Pw2Sig; wiWORD Pw1Exp,Pw2Exp; // significand and exponent of Pw?Acm
    wiUWORD Ax1Sig,Ax2Sig; wiWORD Ax1Exp,Ax2Exp; // significand and exponent of |Ax?|^2
    wiUWORD AcmClrB, AcmEnB1, AcmEnB2;    // power accumulator control signals

    wiWORD  sumSig;
    wiUWORD ThSig;   wiWORD ThExp;   // locally formatted version for SyncThSig and SyncThExp

    int    A0, A1, A2, A3;         // address counters
    wiBoolean RatioValid;                 // ratio (metric) is valid

    int i;

    Tamale_FrameSync_t *p = &(pRX->FrameSync);

    wiWORD *mem0R = p->mem0R;
    wiWORD *mem0I = p->mem0I;
    wiWORD *mem1R = p->mem1R;
    wiWORD *mem1I = p->mem1I;

    wiReg *w0R = p->w0R;
    wiReg *w0I = p->w0I;
    wiReg *w1R = p->w1R;
    wiReg *w1I = p->w1I;
    
    // ---------------
    // Input Registers
    // ---------------
    ClockRegister(p->EnB);        p->EnB.D = FSEnB;
    ClockRegister(p->PathSel);    p->PathSel.D = FSPathSel;
    ClockRegister(p->AGCDone);    p->AGCDone.D = FSAGCDone;
    ClockRegister(p->PeakSearch); p->PeakSearch.D = FSPeakSearch;

    w0R[0].Q.word = p->EnB.Q.b0 ? 0:w0R[0].D.w10;  w0R[0].D.word = max(-511,min(511,FSDI0R.w12));
    w0I[0].Q.word = p->EnB.Q.b0 ? 0:w0I[0].D.w10;  w0I[0].D.word = max(-511,min(511,FSDI0I.w12));
    w1R[0].Q.word = p->EnB.Q.b0 ? 0:w1R[0].D.w10;  w1R[0].D.word = max(-511,min(511,FSDI1R.w12));
    w1I[0].Q.word = p->EnB.Q.b0 ? 0:w1I[0].D.w10;  w1I[0].D.word = max(-511,min(511,FSDI1I.w12));

    // ------------------------
    // Clock Internal Registers
    // ------------------------
    ClockRegister(p->AcmPw0);
    ClockRegister(p->AcmPw1);
    ClockRegister(p->AcmAxR);
    ClockRegister(p->AcmAxI);
    ClockRegister(p->PwSig);
    ClockRegister(p->PwExp);
    ClockRegister(p->AxSig);
    ClockRegister(p->AxExp);
    ClockRegister(p->RatioSig);
    ClockRegister(p->RatioExp);
    ClockRegister(p->MaxSig);
    ClockRegister(p->MaxExp);
    ClockRegister(p->State);
    ClockRegister(p->PeakFound);

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
    A0 = p->A0 = (p->A0 + 1) % 176;
    A1 = p->A1 = (p->A1 + 1) % 176;
    A2 = p->A2 = (p->A2 + 1) % 176;
    A3 = p->A3 = (p->A3 + 1) % 176;
    (p->tpeak.word)++;           // tpeak is shown as 32b resolution; in RTL it is 5b and a secondary signal
                                 // is generated with SigDetValid to enable examination of the signal; thus,
                                 // in RTL if it rolls over to 0 and counts to SyncDelay, there will not be
                                 // a PeakFound event.

    // ------------------------------
    // Update FrameSync State Machine
    // ------------------------------
    AcmClrB.b0   = (p->State.Q.w8 ==   0) ? 0:1;
    AcmEnB1.b0   = (p->State.Q.w8 >=   2) ? 0:1;
    AcmEnB1.b1   = (p->State.Q.w8 >= 146) ? 0:1;
    AcmEnB2.b0   = (p->State.Q.w8 >=  18) ? 0:1;
    AcmEnB2.b1   = (p->State.Q.w8 >= 162) ? 0:1;
    RatioValid   = (p->State.Q.w8 >=  28) ? 1:0; // new to avoid false peaks on first few samples
    p->State.D.word = p->EnB.Q.b0 ? 0 : ((p->State.Q.w8 == 163) ? 163 : (p->State.Q.w8+1));

    // --------------
    // Reset Counters
    // --------------
    if (!AcmClrB.b0) 
    { 
        A0 = p->A0 =   0; 
        A1 = p->A1 = 160; 
        A2 = p->A2 =  32; 
        A3 = p->A3 =  16; 
        p->tpeak.word = pReg->SyncDelay.w5 + 1;

        p->MaxSig.D.word    =  0;
        p->MaxExp.D.word    = -32;
        p->PeakFound.D.word = 0;
        p->SigDet.D.word    = 0;
    }
    // ----------------------------------------------
    // Model: return to avoid unnecessary computation
    // ----------------------------------------------
    if (p->EnB.Q.b0) return;

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
    if (p->PathSel.Q.b0)
    {
        x0R.word = p->AGCDone.Q.b0 ? (y0R.w10/16) : (y0R.w10==0 ? 0 : (y0R.b9 ? -(int)(pReg->SyncMag.w5) : pReg->SyncMag.w5));
        x0I.word = p->AGCDone.Q.b0 ? (y0I.w10/16) : (y0I.w10==0 ? 0 : (y0I.b9 ? -(int)(pReg->SyncMag.w5) : pReg->SyncMag.w5));
    }
    if (p->PathSel.Q.b1)
    {
        x1R.word = p->AGCDone.Q.b0 ? (y1R.w10/16) : (y1R.w10==0 ? 0 : (y1R.b9? -(int)(pReg->SyncMag.w5) : pReg->SyncMag.w5));
        x1I.word = p->AGCDone.Q.b0 ? (y1I.w10/16) : (y1I.w10==0 ? 0 : (y1I.b9? -(int)(pReg->SyncMag.w5) : pReg->SyncMag.w5));
    }
    // --------------------------------
    // Write the input sample to memory
    // --------------------------------
    if (p->PathSel.Q.b0)
    {
        mem0R[A0].word = x0R.w6;
        mem0I[A0].word = x0I.w6;
    }
    if (p->PathSel.Q.b1)
    {
        mem1R[A0].word = x1R.w6;
        mem1I[A0].word = x1I.w6;
    }
    // ---------------------------------------------
    // Calculate Correlator and Signal Power Samples
    // ---------------------------------------------
    if (p->PathSel.Q.w2 == 1)
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
    if (p->PathSel.Q.w2 == 2)
    {
        ax1R.word= x1R.w6*mem1R[A1].w6 + x1I.w6*mem1I[A1].w6;
        ax1I.word= x1I.w6*mem1R[A1].w6 - x1R.w6*mem1I[A1].w6;
        pw1.word = x1R.w6*x1R.w6       + x1I.w6*x1I.w6;
        pw2.word = mem1R[A1].w6*mem1R[A1].w6 + mem1I[A1].w6*mem1I[A1].w6;
    
        ax2R.word= mem1R[A2].w6*mem1R[A3].w6 + mem1I[A2].w6*mem1I[A3].w6;
        ax2I.word= mem1I[A2].w6*mem1R[A3].w6 - mem1R[A2].w6*mem1I[A3].w6;
        pw3.word = mem1R[A2].w6*mem1R[A2].w6 + mem1I[A2].w6*mem1I[A2].w6;
        pw4.word = mem1R[A3].w6*mem1R[A3].w6 + mem1I[A3].w6*mem1I[A3].w6;
    }
    if (p->PathSel.Q.w2 == 3)
    {
        ax1R.word= (x0R.w6*mem0R[A1].w6 + x0I.w6*mem0I[A1].w6 + x1R.w6*mem1R[A1].w6 + x1I.w6*mem1I[A1].w6)/2;
        ax1I.word= (x0I.w6*mem0R[A1].w6 - x0R.w6*mem0I[A1].w6 + x1I.w6*mem1R[A1].w6 - x1R.w6*mem1I[A1].w6)/2;
        pw1.word = (x0R.w6*x0R.w6       + x0I.w6*x0I.w6       + x1R.w6*x1R.w6       + x1I.w6*x1I.w6      )/2;
        pw2.word = (mem0R[A1].w6*mem0R[A1].w6 + mem0I[A1].w6*mem0I[A1].w6 + mem1R[A1].w6*mem1R[A1].w6 + mem1I[A1].w6*mem1I[A1].w6)/2;
    
        ax2R.word= (mem0R[A2].w6*mem0R[A3].w6 + mem0I[A2].w6*mem0I[A3].w6 + mem1R[A2].w6*mem1R[A3].w6 + mem1I[A2].w6*mem1I[A3].w6)/2;
        ax2I.word= (mem0I[A2].w6*mem0R[A3].w6 - mem0R[A2].w6*mem0I[A3].w6 + mem1I[A2].w6*mem1R[A3].w6 - mem1R[A2].w6*mem1I[A3].w6)/2;
        pw3.word = (mem0R[A2].w6*mem0R[A2].w6 + mem0I[A2].w6*mem0I[A2].w6 + mem1R[A2].w6*mem1R[A2].w6 + mem1I[A2].w6*mem1I[A2].w6)/2;
        pw4.word = (mem0R[A3].w6*mem0R[A3].w6 + mem0I[A3].w6*mem0I[A3].w6 + mem1R[A3].w6*mem1R[A3].w6 + mem1I[A3].w6*mem1I[A3].w6)/2;
    }
    // ------------
    // Accumulators
    // ------------
    if (AcmClrB.b0) // accumulators enabled
    {
        p->AcmAxR.D.word = (p->AcmAxR.Q.w20) + (AcmEnB2.b0 ? 0:ax1R.w12) - (AcmEnB2.b1 ? 0:ax2R.w12);
        p->AcmAxI.D.word = (p->AcmAxI.Q.w20) + (AcmEnB2.b0 ? 0:ax1I.w12) - (AcmEnB2.b1 ? 0:ax2I.w12);
        p->AcmPw0.D.word = (p->AcmPw0.Q.w20) + (AcmEnB1.b0 ? 0:pw1.w12 ) - (AcmEnB1.b1 ? 0:pw3.w12 );
        p->AcmPw1.D.word = (p->AcmPw1.Q.w20) + (AcmEnB2.b0 ? 0:pw2.w12 ) - (AcmEnB2.b1 ? 0:pw4.w12 );
    }
    else           // clear accumulators
    {
        p->AcmAxR.D.word = 0;
        p->AcmAxI.D.word = 0;
        p->AcmPw0.D.word = 0;
        p->AcmPw1.D.word = 0;
    }
    // ---------------------------------------------------
    // Convert Accumulator Output Values to Floating Point
    // ---------------------------------------------------
    Tamale_FS_UIntToFloat(&AxRSig, &AxRExp, abs(p->AcmAxR.Q.w20));
    Tamale_FS_UIntToFloat(&AxISig, &AxIExp, abs(p->AcmAxI.Q.w20));
    Tamale_FS_UIntToFloat(&Pw1Sig, &Pw1Exp, p->AcmPw0.Q.w20);
    Tamale_FS_UIntToFloat(&Pw2Sig, &Pw2Exp, p->AcmPw1.Q.w20);

    // ------------------------------------------------   
    // Compute the Test Metric Denominator Terms: Power
    // ------------------------------------------------   
    Tamale_FS_UIntToFloat(&(p->PwSig.D), &PwSigExp, Pw1Sig.w11*Pw2Sig.w11); 
    p->PwExp.D.word = PwSigExp.w7 + Pw1Exp.w6 + Pw2Exp.w6;

    // -----------------------------------------------------------------
    // Compute the Test Metric Numerator Terms: Autocorrelation w/Lag=16
    // -----------------------------------------------------------------
    Tamale_FS_UIntToFloat(&Ax1Sig, &Ax1Exp, (AxRSig.w11 * AxRSig.w11));
    Tamale_FS_UIntToFloat(&Ax2Sig, &Ax2Exp, (AxISig.w11 * AxISig.w11));

    Ax1Exp.word = Ax1Exp.w7 + (AxRExp.w6 << 1);
    Ax2Exp.word = Ax2Exp.w7 + (AxIExp.w6 << 1);

    if (Ax1Exp.w7 > Ax2Exp.w7)          // addition of two floating values
        while (Ax1Exp.w7 > Ax2Exp.w7)
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
    p->AxSig.D.word = (sumSig.word>1023) ? (sumSig.w12/2) : sumSig.w11;
    p->AxExp.D.word = (sumSig.word>1023) ? Ax1Exp.w7+1    : Ax1Exp.w7;

    // -----------------------------------------
    // Compute Ratio of Autocorrelation to Power
    // -----------------------------------------
    if (p->PwSig.Q.w11 > 0)
    {
        Tamale_FS_UIntToFloat(&(p->RatioSig.D), &RatioSigExp, ((p->AxSig.Q.w11<<11)/p->PwSig.Q.w11)); 
        p->RatioExp.D.word = RatioSigExp.w8 + p->AxExp.Q.w7 - p->PwExp.Q.w7 - 11;
    }
    else
    {
        p->RatioSig.D.word =  0;
        p->RatioExp.D.word = -31;
    }
    // --------------------------
    // Format the Threshold Terms
    // --------------------------
    ThSig.w11 = (pReg->SyncThSig.w4 << 7);
    ThExp.w5  = (pReg->SyncThExp.w4  - 7);

    // ----------------------------------------------------------------------
    // Compare the Ratio to Determine the Sync Position and SigDet Validation
    // ----------------------------------------------------------------------
    if (((p->RatioExp.Q.w8 > ThExp.w5) || (p->RatioExp.Q.w8 == ThExp.w5 && p->RatioSig.Q.w11 > ThSig.w11)) && RatioValid)
    {
        p->SigDetValid.D.b0 = 1; // signal detection is valid

        // find peak
        if (p->PeakSearch.Q.b0 && ((p->RatioExp.Q.w8 > p->MaxExp.Q.w8) || (p->RatioExp.Q.w8 == p->MaxExp.Q.w8 && p->RatioSig.Q.w11 > p->MaxSig.Q.w11)))
        {
            p->MaxExp.D.word = p->RatioExp.Q.w8;
            p->MaxSig.D.word = p->RatioSig.Q.w11;
            p->tpeak.word = 0;
        }
        else
        {
            p->MaxExp.D = p->MaxExp.Q;
            p->MaxSig.D = p->MaxSig.Q;
        }
    }
    else
    {
        p->SigDetValid.D.b0 = 0;
    }
    // -------------
    // Peak Detector
    // -------------
    p->PeakFound.D.b0 = (p->tpeak.word==pReg->SyncDelay.w5) ? 1:0;

    // ---------------------------------------------------
    // Output Registers (clocked and wired)
    // (Preclock to accomodate DFE function calling order)
    // ---------------------------------------------------
    ClockRegister(p->PeakFound);   *FSPeakFound   = p->PeakFound.Q;
    ClockRegister(p->SigDetValid); *FSSigDetValid = p->SigDetValid.Q;

    // -------------
    // Trace Signals
    // -------------
    if (pRX->EnableTrace)
    {
        pRX->traceFrameSync[pRX->k20].FSEnB      = FSEnB.b0;
        pRX->traceFrameSync[pRX->k20].State      = p->State.Q.w8;
        pRX->traceFrameSync[pRX->k20].RatioSig   = p->RatioSig.Q.w11;
        pRX->traceFrameSync[pRX->k20].RatioExp   = p->RatioExp.Q.w8;
        pRX->traceFrameSync[pRX->k20].SigDetValid= FSSigDetValid->b0;
        pRX->traceFrameSync[pRX->k20].PeakFound  = FSPeakFound->b0;
        pRX->traceFrameSync[pRX->k20].AGCDone    = FSAGCDone.b0;
        pRX->traceFrameSync[pRX->k20].PeakSearch = FSPeakSearch.b0;
    }
}
// end of Tamale_FrameSync()

// ================================================================================================
// FUNCTION  : Tamale_Linear_to_dB4
// ------------------------------------------------------------------------------------------------
// Purpose   : Convert a linear scaled value x to y where x ~= 2^(y/4)
// Parameters: x -- Linear scaled unsigned word
//             y -- log2(4*x)
// ================================================================================================
void Tamale_Linear_to_dB4(wiUWORD x, wiUWORD *y)
{
    wiUWORD z; z.word = x.w23 >> 4;

    if (z.word == 0) y->word = 0;
    else 
    {
        int tmp = (int)floor(log2(z.word) + 1E-9);
        if (tmp>=3) y->word = (int)floor(0.5 + 4*log2((z.word>>(tmp-3))/8.0)) + 4*tmp;
        else        y->word = (int)floor(0.5 + 4*log2((z.word<<(3-tmp))/8.0)) + 4*tmp;
    }
}
// end of Tamale_Linear_to_dB4()

// ================================================================================================
// FUNCTION  : Tamale_DigitalAmp_Core
// ------------------------------------------------------------------------------------------------
// Purpose   : Core functionality for Tamale_DigitalAmp
// Parameters: DAEnB -- Enable (active low)
//             DADI  -- Data (12 bit input)
//             DADO  -- Data (12 bit output)
//             Gain  -- Gain as multiplier (15 bit input)
//             OutReg-- Register for output
// ================================================================================================
void Tamale_DigitalAmp_Core(wiBoolean DAEnB, wiWORD  DADI, wiWORD *DADO, wiUWORD Gain, wiReg *OutReg)
{
    wiWORD p, q;        // intermediate calculations
    
    // -----------
    // Digital Amp
    // -----------
    p.word = (DADI.w12 * (int)Gain.w15) / 1024;                  // multiplier output
    q.word = abs(p.w17) < 2048 ? p.w12:(p.w17 < 0 ? -2047:2047); // limited for CFO input

    // ---------------
    // Output Register
    // ---------------
    OutReg->Q = OutReg->D;
    OutReg->D.word = DAEnB ? 0 : q.w12;
    DADO->word   = OutReg->Q.w12;
}
// end of Tamale_DigitalAmp_Core()

// ================================================================================================
// FUNCTION  : Tamale_DigitalAmp
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
void Tamale_DigitalAmp ( wiBoolean DAEnB, wiWORD DAGain,
                         wiWORD  DADI0R, wiWORD  DADI0I, wiWORD  DADI1R, wiWORD  DADI1I,
                         wiWORD *DADO0R, wiWORD *DADO0I, wiWORD *DADO1R, wiWORD *DADO1I,
                         Tamale_RxState_t *pRX)
{
    int j, tmpi;  wiUWORD A;

    // ------------------------------------------------
    // Compute the linear gain in floating point format
    // ------------------------------------------------
    A.word = (unsigned)floor(0.5 + 512.0*pow(2.0, (double)DAGain.w6/4.0));
    j = 0; tmpi = A.w15;
    while (tmpi>63) { tmpi=tmpi>>1; j++;}; 
    if (tmpi & 0x01 && tmpi>31) tmpi++;
    for ( ; j>0; j--) tmpi=tmpi<<1;
    A.word = A.w15>0 ? tmpi : -tmpi;

    // ------------------------------
    // Replicate the four multipliers
    // ------------------------------
    Tamale_DigitalAmp_Core(DAEnB, DADI0R, DADO0R, A, pRX->DigitalAmpOut+0); // path 0, real
    Tamale_DigitalAmp_Core(DAEnB, DADI0I, DADO0I, A, pRX->DigitalAmpOut+1); // path 0, imaginary
    Tamale_DigitalAmp_Core(DAEnB, DADI1R, DADO1R, A, pRX->DigitalAmpOut+2); // path 1, real
    Tamale_DigitalAmp_Core(DAEnB, DADI1I, DADO1I, A, pRX->DigitalAmpOut+3); // path 1, imaginary
}
// end of Tamale_DigitalAmp()

// ================================================================================================
// FUNCTION  : Tamale_AGC
// ------------------------------------------------------------------------------------------------
// Purpose   : Automatic gain control
// Parameters: AGCEnB        -- Enable the circuit, active low (input 2b)
//             AGCPathSel    -- Select the active digital paths (2 bit input)
//             AGCInitGains  -- Initialize AGC controlled gains, active high (input)
//             AGCUpdateGains-- Update the AGC controlled gains, active high (input)
//             AGCForceLinear-- Force a linear gain update (ignore LgSigDet) (input)
//             AGCLgSigAFE   -- Indicates a large signal at the AFE (1 bit input)
//             AGCDI0R       -- Signal input, path 0, real component (12 bit input)
//             AGCDI0I       -- Signal input, path 0, imag component (12 bit input)
//             AGCDI1R       -- Signal input, path 1, real component (12 bit input)
//             AGCDI1I       -- Signal input, path 1, imag component (12 bit input)
//             AGCLNAGain    -- LNA gain switch (2 bit output)
//             AGCAGain      -- Analog VGA setting (5 bit output)
//             AGCLgSigDet   -- Large signal detected (1 bit output)
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
void Tamale_AGC( wiUWORD AGCEnB, wiUWORD AGCDAEnB, wiUWORD AGCPathSel, wiUWORD AGCInitGains, 
                 wiUWORD AGCUpdateGains, wiUWORD AGCForceLinear, wiBoolean AGCLgSigAFE, 
                 wiUWORD AGCUseRecordPwr, wiUWORD AGCRecordPwr, wiUWORD AGCRecordRefPwr, 
                 wiUWORD AGCAntSel, wiUWORD *AGCAntSuggest, wiUWORD *AGCAntSearch,
                 wiWORD  AGCDI0R, wiWORD  AGCDI0I, wiWORD  AGCDI1R, wiWORD AGCDI1I, wiUWORD *AGCLNAGain, 
                 wiUWORD *AGCAGain, wiUWORD *AGCLgSigDet, wiUWORD *AGCRSSI0, wiUWORD *AGCRSSI1, 
                 wiWORD  *AGCCFODI0R, wiWORD *AGCCFODI0I, wiWORD *AGCCFODI1R, wiWORD *AGCCFODI1I, 
                 wiWORD  w0R, wiWORD w0I, wiWORD w1R, wiWORD w1I, 
                 TamaleRegisters_t *pReg, Tamale_RxState_t *pRX, Tamale_aSigState_t *SigOut)
{
    Tamale_AGC_t *p = &(pRX->AGC);

    wiUWORD pw0,pw1,pw2,pw3;    // quadrature component power measures
    wiUWORD msrpwr0 = {0};      // measured digital power in the power of 2^(1/4)
    wiUWORD msrpwr1 = {0};      // second path, measured digital power in the power of 2^(1/4)
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
    wiBoolean w511, wPeakDet, SwDPowerRange;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Clock Model Registers and Wire Registered Input
    //
    //  Explicit registers operate at 20 MHz. Input signal that pass through a pipeline
    //  register are "wired" to the register structure "D" element.
    //
    ClockRegister(p->EnB);        p->EnB.D       = AGCEnB;
    ClockRegister(p->DAEnB);      p->DAEnB.D     = AGCDAEnB;
    ClockRegister(p->PathSel);    p->PathSel.D   = AGCPathSel;
    ClockRegister(p->InitGains);  p->InitGains.D = AGCInitGains;
    
    ClockRegister(p->AcmPwr0);
    ClockRegister(p->AcmPwr1);
    ClockRegister(p->State);
    ClockRegister(p->LNAGain);
    ClockRegister(p->AGain);
    ClockRegister(p->DGain);
    ClockRegister(p->LgSigDet);

    ClockRegister(p->RSSI0); ClockRegister(p->msrpwrA); ClockRegister(p->abspwrA);
    ClockRegister(p->RSSI1); ClockRegister(p->msrpwrB); ClockRegister(p->abspwrB);
    ClockRegister(p->refpwr);ClockRegister(p->ValidRefPwr);

    // ---------------------------
    // Wire Inputs Based On Enable
    // ---------------------------
    p->DI0R.word = p->EnB.Q.b0 ? 0:AGCDI0R.w12;
    p->DI0I.word = p->EnB.Q.b0 ? 0:AGCDI0I.w12;
    p->DI1R.word = p->EnB.Q.b0 ? 0:AGCDI1R.w12;
    p->DI1I.word = p->EnB.Q.b0 ? 0:AGCDI1I.w12;

    // ------------------
    // Increment Counters
    // ------------------
    p->A1.word = (p->A1.w5 + 1) % 17;
    p->A2.word = (p->A2.w5 + 1) % 17;

    // ------------------------
    // Update AGC State Machine
    // ------------------------
    AcmClrB.b0   = (p->State.Q.w5 ==  0) ? 0:1;
    AcmEnB.b0    = (p->State.Q.w5 >=  3) ? 0:1;
    AcmEnB.b1    = (p->State.Q.w5 == 19) ? 0:1;
    p->State.D.word = p->EnB.Q.b0 ? 0 : (p->State.Q.w5 == 19 ? 19:(p->State.Q.w5+1));

    // --------------
    // Reset Counters
    // --------------
    if (!AcmClrB.b0) {p->A1.word = 0; p->A2.word = 1;}

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Peak Detect
    //
    //  Look for signals that reach peak amplitude and use as an indicator that a large
    //  signal is present.
    //
    w511 = ((abs(w0R.w10) == 511)
         || (abs(w0I.w10) == 511) 
         || (abs(w1R.w10) == 511) 
         || (abs(w1I.w10) == 511)) ? 1:0;
    ClockDelayLine(p->wPeak, w511);
    wPeakDet = ((p->wPeak.word & 0xFFF) != 0) ? 1:0;

    // ---------------------------
    // Digital Amplifier Submodule
    // ---------------------------
    Tamale_DigitalAmp( p->DAEnB.Q.b0, p->DGain.Q, p->DI0R, p->DI0I, p->DI1R, p->DI1I, 
                       AGCCFODI0R, AGCCFODI0I, AGCCFODI1R, AGCCFODI1I, pRX );

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Power Measurement
    //
    //  Power is measured over a 16T (800 ns) moving windows. The squared input is added
    //  to an accumulator and a 16T delayed version is subtracted. The power value is
    //  reported on a logarithmic space with units of 3/4 dB (the Linear_to_dB4 func).
    //
    //  A more efficient implementation would precompute the magnitude-squared term and
    //  store than instead of the input term. This reduces the computational load and
    //  does not require more storage space.
    //
    if (p->PathSel.Q.b0) // Path 0 //////////////////////////////////////////////////////////
    {
        p->mem0R[p->A1.w5].word = w0R.w10; // write new samples to RAM at address A1
        p->mem0I[p->A1.w5].word = w0I.w10;

        d0R.word = p->mem0R[p->A2.w5].w10; // read 16T delayed samples from RAM at address A2
        d0I.word = p->mem0I[p->A2.w5].w10;

        pw0.word = (w0R.w10 * w0R.w10) + (w0I.w10 * w0I.w10);
        pw1.word = (d0R.w10 * d0R.w10) + (d0I.w10 * d0I.w10);

        if (AcmClrB.b0) p->AcmPwr0.D.word = (p->AcmPwr0.Q.w23) + (AcmEnB.b0 ? 0:pw0.w19) - (AcmEnB.b1 ? 0:pw1.w19);
        else            p->AcmPwr0.D.word = 0;

        Tamale_Linear_to_dB4(p->AcmPwr0.Q, &msrpwr0);
    }
    if (p->PathSel.Q.b1) // Path 1 //////////////////////////////////////////////////////////
    {
        p->mem1R[p->A1.w5].word = w1R.w10; // write new samples to RAM at address A1
        p->mem1I[p->A1.w5].word = w1I.w10;

        d1R.word = p->mem1R[p->A2.w5].w10; // read 16T delayed samples from RAM at address A2
        d1I.word = p->mem1I[p->A2.w5].w10;

        pw2.word = (w1R.w10 * w1R.w10) + (w1I.w10 * w1I.w10);
        pw3.word = (d1R.w10 * d1R.w10) + (d1I.w10 * d1I.w10);

        if (AcmClrB.b0) p->AcmPwr1.D.word = (p->AcmPwr1.Q.w23) + (AcmEnB.b0 ? 0:pw2.w19) - (AcmEnB.b1 ? 0:pw3.w19);
        else            p->AcmPwr1.D.word = 0;

        Tamale_Linear_to_dB4(p->AcmPwr1.Q, &msrpwr1);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Coverter Measured Power to Absolute Power
    //
    //  The measured power indicates the signal level at the baseband input. This is
    //  translated to absolute power by considering the radio gain. The reference gain is
    //  the initial gain (AGAIN = InitAGain and LNAGain1 = LNAGain2 = 1). The difference
    //  between the current and initial gain is computed and scaled by 2 (to convert
    //  from units of 1.5 dB to 3/4 dB) and subtracted from the measured power.
    //
    {
        wiWORD a1, a2, a3;
        a1.word = p->LNAGain.Q.b0 ? 0 : -((signed)pReg->StepLNA.w5);  // LNA
        a2.word = p->LNAGain.Q.b1 ? 0 : -((signed)pReg->StepLNA2.w5); // LNA2
        a3.word = p->AGain.Q.w6 - pReg->InitAGain.w6;                // analog VGA
        pathgain.word = 2 * (a1.w6 + a2.w6 + a3.w7);              // total gain

        abspwr0.word = p->PathSel.Q.b0 ? max(0, (msrpwr0.w7 - pathgain.w8)) : 0;
        abspwr1.word = p->PathSel.Q.b1 ? max(0, (msrpwr1.w7 - pathgain.w8)) : 0;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Select the Larger Measured Power
    //
    //  When both receive paths are enabled, AGC operates on the stronger of the two
    //  signals. This maximizes fidelity for the strong signal.
    //  
    {
        wiBoolean s = (!p->PathSel.Q.b0 || ((p->PathSel.Q.w2 == 3) && (msrpwr1.w7 > msrpwr0.w7))) ? 1:0;
        msrpwr.word = s ? msrpwr1.w7 : msrpwr0.w7;
        abspwr.word = s ? abspwr1.w8 : abspwr0.w8;
        RSSI.word   = s ? p->RSSI1.Q.w8 : p->RSSI0.Q.w8;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Switched Antenna Diversity Circuits
    //
    //  The AGC provides several support functions for antenna selection diversity.
    //
    //  1) AGC determines whether checking the alterate antenna is appropriate if
    //     the current antenna provides a low input signal (set AGCAntSearch).
    //
    //  2) The power measurement for each antenna is recorded (when AGCRecordPwr) is set.
    //
    //  3) If the alternate antenna is stronger, signal AGCAntSuggest is set
    //
    //  4) The recorded power values are restored once an antenna has been selected.
    //
    SwDPowerRange = ((msrpwr.w7 < pReg->OFDMSwDTh.w8) && (msrpwr.w7 > pReg->OFDMSwDThLow.w7)) ? 1:0;
    AGCAntSearch->b0  = (SwDPowerRange && (AGCPathSel.w3 != 3)) ? 1:0;
    AGCAntSuggest->b0 = (p->msrpwrA.Q.w7 > p->msrpwrB.Q.w7) ? 0:1;

    if (AGCUseRecordPwr.b0)
    {
        if (!AGCAntSel.b0) {
            abspwr.word = p->abspwrA.Q.w8;
            msrpwr.word = p->msrpwrA.Q.w7;
        }
        else {
            abspwr.word = p->abspwrB.Q.w8;
            msrpwr.word = p->msrpwrB.Q.w7;
        }
    }
    if (AGCRecordPwr.b0)
    {
        if (AGCAntSel.b0) {
            p->abspwrB.D.word = abspwr.w8;
            p->msrpwrB.D.word = msrpwr.w7;
        }
        else {
            p->abspwrA.D.word = abspwr.w8;
            p->msrpwrA.D.word = msrpwr.w7;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialize Gain Settings
    //
    //  During RX idle, gain is set to its initial (high) value. This includes setting
    //  AGAIN = InitAGain and both LNAGain lines to 1.
    //
    if (p->InitGains.Q.b0)
    {
        AGain3.word = pReg->InitAGain.w6;
        DGain1.word = 0;
        p->LNAGain.D.word  = (pReg->InitLNAGain2.b0 << 1) | pReg->InitLNAGain.b0;
        p->UpdateCount.word = 0;
        p->refpwr.D.word = 0;
        p->ValidRefPwr.D.word = 0;
    }
    else if (AGCUpdateGains.b0)
    {
        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Gain Update
        //
        //  When the DFE controller indicates a gain update should occur, AGC implements a
        //  rule-based update. 
        //
        //  1) The number of gain updates is checked against register MaxUpdateCount. If the 
        //     number of updates is equal to this value, only one more update is allowed.
        //
        //  2) If the LgSigAFE signal from the radio is set, gain is reduced by StepLgSigDet
        //     and the LgSigDet signal is set so the DFE controller knows additional gain
        //     measure-update cycles are required.
        //
        //  3) If the measured power exceeds the ThSigSmall threshold, a fixed attenuation
        //     step is taken based on whether the power is above ThSigLarge or is only
        //     above ThSigSmall. LgSigDet is set to cause additional measure-update cycles.
        //
        //  4) If the large signal conditions do not exists, an update is made assuming the
        //     measured amplitude accurate reflects the actual signal level (no distortion).
        //     LgSigDet is cleared to indicate AGC is complete.
        //
        wiBoolean GainLimit = (p->UpdateCount.w3 == pReg->MaxUpdateCount.w3) || AGCForceLinear.b0 ? 1:0;
        p->UpdateCount.w3++; // increment gain update counter

        // Analog Front End Overload: Decrease LNA Gain
        //
        if (!GainLimit && AGCLgSigAFE)
        {
            AGain1.word   = p->AGain.Q.w6 - pReg->StepLgSigDet.w5;
            DGain1.word   = 0;
            p->LgSigDet.D.b0  = 1;       // flag large signal detect
            pRX->EventFlag.b5 = 1;       // flag LNA gain change
        }
        // Large Measured Signal Power
        //
        else if (!GainLimit && msrpwr.w7 > pReg->ThSigSmall.w7)
        {
            if (msrpwr.w7 > pReg->ThSigLarge.w7) AGain1.word = p->AGain.Q.w6 - pReg->StepLarge.w5;
            else if (wPeakDet)                   AGain1.word = p->AGain.Q.w6 - pReg->StepSmallPeak.w5;
            else                                 AGain1.word = p->AGain.Q.w6 - pReg->StepSmall.w5;

            DGain1.word   = 0;
            p->LgSigDet.D.b0 = 1;
        }
        // Low to Moderate Measured Signal Power
        //
        else
        {
                 if (abspwr.w8 <= (int)pReg->AbsPwrL.w7) AGainUpdate.word = pReg->AbsPwrL.w7 - msrpwr.w7;
            else if (abspwr.w8 >= (int)pReg->AbsPwrH.w7) AGainUpdate.word = pReg->AbsPwrH.w7 - msrpwr.w7;
            else                                         AGainUpdate.word =        abspwr.w8 - msrpwr.w7;

            AGain1.word   = p->AGain.Q.w6 + AGainUpdate.w8/2;
            DGain1.word   = (int)(pReg->RefPwrDig.w7-msrpwr.w7)/2 - AGainUpdate.w7/2;
            p->LgSigDet.D.b0 = 0;
        }
        // Force AGain1 value when FixedGain is set
        //
        if (pReg->FixedGain.b0) AGain1.word = p->AGain.Q.w6;

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Distribute Gain Between LNAGAIN and AGAIN
        //  
        //  The LNA gain is reduced when the change in gain exceeds a programmable
        //  threshold (ThSwitchLNA for LNAGAIN and ThSwitchLNA2 for LNAGAIN2). If these
        //  are switched to a low gain state, AGAIN is adjusted based on the programmed
        //  attenuation step for each LNA to achieve the target attenuation level.
        //  
        //  If UpdateOnLNA is set and the GainLimit has not been reached LgSigDet is set
        //  to 1 to force a subsequent gain measure-update cycle. Checking GainLimit is
        //  new to Tamale.
        //
        deltaGain.word = ((int)pReg->InitAGain.w6-AGain1.word) 
                       + (int)(p->LNAGain.Q.b0 ? 0:pReg->StepLNA.w5) 
                       + (int)(p->LNAGain.Q.b1 ? 0:pReg->StepLNA2.w5); // attenuation relative to initial gain

        if (p->LNAGain.Q.b0 && (deltaGain.w7 > (signed)pReg->ThSwitchLNA.w6))
        {
            p->LNAGain.D.b0 = 0;                           // switch LNA to low gain mode
            AGain2.word  = AGain1.w7 + pReg->StepLNA.w5;   // increase VGA gain by LNA gain difference
            if (pReg->UpdateOnLNA.b0 && !GainLimit) p->LgSigDet.D.b0 = 1;    // force LgSigDet to 1
            pRX->EventFlag.b5 = 1;                         // flag LNA gain change
        }
        else AGain2.word = AGain1.word; // no switch

        if (p->LNAGain.Q.b1 && (deltaGain.w7 > (signed)pReg->ThSwitchLNA2.w6))
        {
            p->LNAGain.D.b1 = 0;                           // switch LNA2 to low gain mode
            AGain3.word  = AGain2.w7 + pReg->StepLNA2.w5;  // increase VGA gain by LNA gain difference
            if (pReg->UpdateOnLNA.b0 && !GainLimit) p->LgSigDet.D.b0 = 1;    // force LgSigDet to 1
        }
        else AGain3.word = AGain2.w7; // no switch

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Determine type of initial gain update. This is used for AGC characterization
        //  at the architecture level and is not included in the actual ASIC.
        //
        {
            int i = p->UpdateCount.w3 - 1;

            if (!GainLimit && AGCLgSigAFE)           pRX->NonRTL.GainUpdateType[i] = TAMALE_GAIN_UPDATE_LGSIGAFE;
            else if (!GainLimit && msrpwr.w7 > pReg->ThSigSmall.w7)
            {
                if (msrpwr.w7 > pReg->ThSigLarge.w7) pRX->NonRTL.GainUpdateType[i] = TAMALE_GAIN_UPDATE_THSIGLARGE;
                else if (wPeakDet)                   pRX->NonRTL.GainUpdateType[i] = TAMALE_GAIN_UPDATE_THSMALLPEAK;
                else                                 pRX->NonRTL.GainUpdateType[i] = TAMALE_GAIN_UPDATE_THSIGSMALL;
            }
            else                                     pRX->NonRTL.GainUpdateType[i] = TAMALE_GAIN_UPDATE_LINEAR;
        }
    }
    else // Maintain Current Gain (no initialization or update) /////////////////////////
    {
        p->LgSigDet.D.b0 = 0;
        AGain3.word   = p->AGain.Q.w6;
        DGain1.word   = p->DGain.Q.w6;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Limit AGain and DGain to a Valid Range
    //  
    //  AGain is a 6-bit value (0 to 63)
    //  DGain is a signed 6-bit value but is only valid from -19 to +19
    // 
    // [Barrett-110214] The comparison AGain3.w7 > 63 is invalid because AGain3 is signed
    // so 63 is the maximum value. The code is changed from ">" to ">=" to suppress
    // compiler warnings. Technically, the bit width should have been .w8, but because
    // the upper value for AGAIN in RF52/22 is 40, there is no real issue for the ASIC.
    //
         if (AGain3.w7 < 0)  p->AGain.D.word =  0;
    else if (AGain3.w7 >=63) p->AGain.D.word = 63;
    else                     p->AGain.D.word = AGain3.w7;

         if (DGain1.w6 <-19) p->DGain.D.word = -19;
    else if (DGain1.w6 > 19) p->DGain.D.word =  19;
    else                     p->DGain.D.word = DGain1.w6;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Output RSSI
    //
    //  RSSI is calibrated to be RSSI = 4/3 * (Pr[dBm] + 100). If the path is not enabled
    //  the output is zero so software can select the maximum of the two values as a
    //  link quality indicator.
    //
    {
        int s0 = (AGCPathSel.b0 && (abspwr0.w8 > pReg->Pwr100dBm.w5)) ? 1:0;
        int s1 = (AGCPathSel.b1 && (abspwr1.w8 > pReg->Pwr100dBm.w5)) ? 1:0;

        p->RSSI0.D.word = s0 ? (abspwr0.w8 - pReg->Pwr100dBm.w5) : 0;
        p->RSSI1.D.word = s1 ? (abspwr1.w8 - pReg->Pwr100dBm.w5) : 0;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Step-Up/Down Detector
    //
    //  The top-level control can abort reception of a packet and restart if the signal
    //  power deviates up or down from the initial packet power. Detection of this event
    //  is done here. First, a reference power (refpwr) is recorded when AGCRecordRefPwr
    //  is asserted (from the DFE controller after AGC is complete). The current power
    //  is compared against this reference power when valid. The StepUp detection also
    //  requires the reference power to be less than a programmable threshold. StepDown
    //  is qualified only when the msrpwr value is below ThStepDownMsrPwr.
    //
    if (AGCRecordRefPwr.b0)
    {
        p->refpwr.D.word = abspwr.w8;
        p->ValidRefPwr.D.word = 1;
        pRX->RefPwrAGC = abspwr.w8;
    }
    if (SigOut->ValidRSSI && p->ValidRefPwr.Q.b0)
    {
        SigOut->StepUp   =((abspwr.w8 > p->refpwr.Q.w8+2*pReg->ThStepUp.w4) && (p->refpwr.Q.w8 < pReg->ThStepUpRefPwr.w8) && p->refpwr.Q.w8) ? 1:0;
        SigOut->StepDown =((abspwr.w8 < p->refpwr.Q.w8-2*pReg->ThStepDown.w4) && (msrpwr.w7 < pReg->ThStepDownMsrPwr.w6)) ? 1:0;
    }
    else 
        SigOut->StepUp = SigOut->StepDown = 0;
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  DFS (Radar) Pulse Detect
    //
    //  The pulse detector operates on the RSSI value comparing it against two thresholds
    //  to provide hysteresis. Additionally, the rising edge of the pulse must follow
    //  a delayed period where the level is below the bottom threshold for 14T. These
    //  conditions are intended to reduce false positives due to normal power variations
    //  during OFDM packets.
    //
    if (p->EnB.Q.b0)        
    {
        SigOut->PulseDet = 0; // reset pulse detector if AGC is disabled
    }
    else if (SigOut->ValidRSSI) 
    {
        wiBoolean PulseUp = (RSSI.w8 >=pReg->ThDFSUp.w7) && ((p->detThDFSDn.w32 & 0xFFFC0000) == 0xFFFC0000);
        wiBoolean PulseDn = (RSSI.w8 < pReg->ThDFSDn.w7);

        if      (PulseUp) SigOut->PulseDet = 1; // pulse rising edge
        else if (PulseDn) SigOut->PulseDet = 0; // pulse falling edge
        else ;                                  // keep previous value
    }                                           // otheriwse, hold detector output during gain update

    p->detThDFSDn.word = (p->detThDFSDn.w32 << 1) | ((SigOut->ValidRSSI && (RSSI.w8 < pReg->ThDFSDn.w7)) ? 1:0);
    
    // ------------
    // Wire outputs
    // ------------
    pRX->DGain[pRX->k20] = p->DGain.Q;
    AGCAGain->word    = pReg->FixedGain.b0 ? pReg->InitAGain.w6 : p->AGain.Q.w6;
    AGCLNAGain->word  = pReg->FixedGain.b0 ? (2*pReg->InitLNAGain2.b0 + pReg->InitLNAGain.b0) : p->LNAGain.Q.w2;
    AGCLgSigDet->word = p->LgSigDet.Q.b0;
    AGCRSSI0->word    = p->RSSI0.Q.w8;
    AGCRSSI1->word    = p->RSSI1.Q.w8;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Trace Signals
    //
    //  Record signals used for waveform display and digital design verification. This
    //  function is enabled with parameter EnableTrace.
    //
    if (pRX->EnableTrace)
    {
        pRX->traceAGC[pRX->k20].AGCEnB      = AGCEnB.b0;
        pRX->traceAGC[pRX->k20].InitGains   = AGCInitGains.b0;
        pRX->traceAGC[pRX->k20].UpdateGains = AGCUpdateGains.b0;
        pRX->traceAGC[pRX->k20].State       = p->State.Q.w5;
        pRX->traceAGC[pRX->k20].LNAGain     = AGCLNAGain->w2;
        pRX->traceAGC[pRX->k20].AGAIN       = AGCAGain->w6;
        pRX->traceAGC[pRX->k20].DGAIN       = p->DGain.Q.w6;
        pRX->traceAGC[pRX->k20].wPeakDet    = wPeakDet;
        pRX->traceAGC[pRX->k20].LgSigAFE    = AGCLgSigAFE;
        pRX->traceAGC[pRX->k20].AntSearch   = AGCAntSearch->b0;
        pRX->traceAGC[pRX->k20].AntSuggest  = AGCAntSuggest->b0;
        pRX->traceAGC[pRX->k20].RecordPwr   = AGCRecordPwr.b0;
        pRX->traceAGC[pRX->k20].UseRecordPwr= AGCUseRecordPwr.b0;
        pRX->traceAGC[pRX->k20].UpdateCount = p->UpdateCount.w3;

        pRX->traceRSSI[pRX->k20].RSSI0 = AGCRSSI0->w8;
        pRX->traceRSSI[pRX->k20].RSSI1 = AGCRSSI1->w8;
        pRX->traceRSSI[pRX->k20].msrpwr0 = msrpwr0.w7;
        pRX->traceRSSI[pRX->k20].msrpwr1 = msrpwr1.w7;
    }
}
// end of Tamale_AGC()

// ================================================================================================
// FUNCTION  : Tamale_SignalDetect  
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
void Tamale_SignalDetect(wiUWORD SDEnB, wiUWORD SDPathSel, wiWORD SDDI0R, wiWORD SDDI0I, wiWORD SDDI1R, 
                          wiWORD SDDI1I, wiUWORD *SDSigDet, Tamale_RxState_t *pRX, TamaleRegisters_t *pReg)
{
    wiBoolean CheckSigDet;           
    wiWORD  ax1R, ax1I, ax2R, ax2I;      // per-sample auto correlator output (1=current, 2=delayed)
    wiUWORD pw1, pw2;                    // squared amplitude of signal (1=current, 2=delayed)
    wiUWORD AcmClrB, AxAcmEnB, PwAcmEnB; // accumulator control signals
    wiWORD  x0R, x0I, x1R, x1I;          // filtered signal input
    wiUWORD x, t;
    int    A0, A1, A2, A3, A4;    // address counters
    int i;

    Tamale_SignalDetect_t *p = &(pRX->SignalDetect);

    wiWORD *mem0R = p->mem0R;
    wiWORD *mem0I = p->mem0I;
    wiWORD *mem1R = p->mem1R;
    wiWORD *mem1I = p->mem1I;

    // ---------------
    // Input Registers
    // ---------------
    ClockRegister(p->EnB);     p->EnB.D.word = SDEnB.b0;
    ClockRegister(p->PathSel); p->PathSel.D.word = SDPathSel.w2;

    // -----------------------
    // Input Signal Delay Line
    // -----------------------
    for (i=2; i>0; i--)
    {
        p->w0R[i] = p->w0R[i-1];  p->w0I[i] = p->w0I[i-1];
        p->w1R[i] = p->w1R[i-1];  p->w1I[i] = p->w1I[i-1];
    }
    p->w0R[0].word = p->EnB.Q.b0 ? 0:SDDI0R.w10;
    p->w0I[0].word = p->EnB.Q.b0 ? 0:SDDI0I.w10;
    p->w1R[0].word = p->EnB.Q.b0 ? 0:SDDI1R.w10;
    p->w1I[0].word = p->EnB.Q.b0 ? 0:SDDI1I.w10;

    // ------------------------
    // Clock Internal Registers
    // ------------------------
    ClockRegister(p->AcmAxR);
    ClockRegister(p->AcmAxI);
    ClockRegister(p->AcmPwr);
    ClockRegister(p->SigDet);
    ClockRegister(p->State);

    // ------------------
    // Increment counters
    // ------------------
    A0 = p->A0 = (p->A0 + 1) % 96;
    A1 = p->A1 = (p->A1 + 1) % 96;
    A2 = p->A2 = (p->A2 + 1) % 96;
    A3 = p->A3 = (p->A3 + 1) % 96;
    A4 = p->A4 = (p->A4 + 1) % 96;

    // ---------------------------
    // Update SigDet State Machine
    // ---------------------------
    AcmClrB.b0   = (p->State.Q.w8 ==  0) ? 0:1;
    AxAcmEnB.b0  = (p->State.Q.w8 >= 18) ? 0:1;
    AxAcmEnB.b1  = (p->State.Q.w8 >= 50) ? 0:1;
    PwAcmEnB.b0  = (p->State.Q.w8 >= 50) ? 0:1;
    PwAcmEnB.b1  = (p->State.Q.w8 >= 82) ? 0:1;
    p->State.D.word = p->EnB.Q.b0 ? 0 : ((p->State.Q.w8 == pReg->SigDetDelay.w8) ? pReg->SigDetDelay.w8 : (p->State.Q.w8+1));

    // --------------
    // Initialization
    // --------------
    if (!AcmClrB.b0)
    {
        A0 = p->A0 =  0; 
        A1 = p->A1 = 80; 
        A2 = p->A2 = 64; 
        A3 = p->A3 = 48; 
        A4 = p->A4 = 16;
        p->SigDet.D.word = 0;
    }
    // ----------------------
    // Generate input samples
    // ----------------------
    if (pReg->SigDetFilter.b0)
    {
        x0R.word = (p->w0R[0].w10 - p->w0R[2].w10) >> 1;
        x0I.word = (p->w0I[0].w10 - p->w0I[2].w10) >> 1;
        x1R.word = (p->w1R[0].w10 - p->w1R[2].w10) >> 1;
        x1I.word = (p->w1I[0].w10 - p->w1I[2].w10) >> 1;
    }
    else
    {
        x0R.word = p->w0R[0].w10;
        x0I.word = p->w0I[0].w10;
        x1R.word = p->w1R[0].w10;
        x1I.word = p->w1I[0].w10;
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
    if (p->PathSel.Q.w2 == 1)
    {
        ax1R.word= x0R.w10*mem0R[A1].w10 + x0I.w10*mem0I[A1].w10;
        ax1I.word= x0I.w10*mem0R[A1].w10 - x0R.w10*mem0I[A1].w10;

        ax2R.word= mem0R[A2].w10*mem0R[A3].w10 + mem0I[A2].w10*mem0I[A3].w10;
        ax2I.word= mem0I[A2].w10*mem0R[A3].w10 - mem0R[A2].w10*mem0I[A3].w10;
    }
    if (p->PathSel.Q.w2 == 2)
    {
        ax1R.word= x1R.w10*mem1R[A1].w10 + x1I.w10*mem1I[A1].w10;
        ax1I.word= x1I.w10*mem1R[A1].w10 - x1R.w10*mem1I[A1].w10;

        ax2R.word= mem1R[A2].w10*mem1R[A3].w10 + mem1I[A2].w10*mem1I[A3].w10;
        ax2I.word= mem1I[A2].w10*mem1R[A3].w10 - mem1R[A2].w10*mem1I[A3].w10;
    }
    if (p->PathSel.Q.w2 == 3)
    {
        ax1R.word= (x0R.w10*mem0R[A1].w10 + x0I.w10*mem0I[A1].w10 + x1R.w10*mem1R[A1].w10 + x1I.w10*mem1I[A1].w10)/2;
        ax1I.word= (x0I.w10*mem0R[A1].w10 - x0R.w10*mem0I[A1].w10 + x1I.w10*mem1R[A1].w10 - x1R.w10*mem1I[A1].w10)/2;

        ax2R.word= (mem0R[A2].w10*mem0R[A3].w10 + mem0I[A2].w10*mem0I[A3].w10 + mem1R[A2].w10*mem1R[A3].w10 + mem1I[A2].w10*mem1I[A3].w10)/2;
        ax2I.word= (mem0I[A2].w10*mem0R[A3].w10 - mem0R[A2].w10*mem0I[A3].w10 + mem1I[A2].w10*mem1R[A3].w10 - mem1R[A2].w10*mem1I[A3].w10)/2;
    }
    // -----------------------------------
    // Compute the Squared Samples (Power)
    // -----------------------------------
    if (p->PathSel.Q.w2 == 1)
    {
        pw1.word= mem0R[A3].w10*mem0R[A3].w10+mem0I[A3].w10*mem0I[A3].w10;
        pw2.word= mem0R[A4].w10*mem0R[A4].w10+mem0I[A4].w10*mem0I[A4].w10;
    }
    if (p->PathSel.Q.w2 == 2)
    {
        pw1.word= mem1R[A3].w10*mem1R[A3].w10+mem1I[A3].w10*mem1I[A3].w10;
        pw2.word= mem1R[A4].w10*mem1R[A4].w10+mem1I[A4].w10*mem1I[A4].w10;
    }
    if (p->PathSel.Q.w2 == 3)
    {
        pw1.word= (mem0R[A3].w10*mem0R[A3].w10+mem0I[A3].w10*mem0I[A3].w10 + mem1R[A3].w10*mem1R[A3].w10+mem1I[A3].w10*mem1I[A3].w10)/2;
        pw2.word= (mem0R[A4].w10*mem0R[A4].w10+mem0I[A4].w10*mem0I[A4].w10 + mem1R[A4].w10*mem1R[A4].w10+mem1I[A4].w10*mem1I[A4].w10)/2;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Accumulators
    //
    //  Worst-case resolution is 1+log2(32*2*512^2) = 25b; Power should be unsigned. 
    //  Then pw? requires 19b and the accumulator worst-case is 24b. In RTL accumulators 
    //  should subtract then add.
    //
    if (AcmClrB.b0) // accumulators enabled
    {
        p->AcmAxR.D.word = (p->AcmAxR.Q.w25) + (AxAcmEnB.b0 ? 0:ax1R.w20) - (AxAcmEnB.b1 ? 0:ax2R.w20);   
        p->AcmAxI.D.word = (p->AcmAxI.Q.w25) + (AxAcmEnB.b0 ? 0:ax1I.w20) - (AxAcmEnB.b1 ? 0:ax2I.w20);   
        p->AcmPwr.D.word = (p->AcmPwr.Q.w24) + (PwAcmEnB.b0 ? 0:pw1.w19 ) - (PwAcmEnB.b1 ? 0:pw2.w19 );   
    }
    else           // clear accumulators
    {
        p->AcmAxR.D.word = 0;
        p->AcmAxI.D.word = 0;
        p->AcmPwr.D.word = 0;
    }

    // ------------------
    // Signal Detect Test
    // ------------------
    CheckSigDet = p->State.Q.w8 == pReg->SigDetDelay.w8 ? 1:0;
    x.word = (unsigned)((abs(p->AcmAxR.Q.w25) + abs(p->AcmAxI.Q.w25)) >> 5);
    t.word = (unsigned)((p->AcmPwr.Q.w24 * pReg->SigDetTh1.w6) >> 6);

    p->SigDet.D.b0 = (CheckSigDet && (x.w20 >= t.w23) && (x.w20 >= (pReg->SigDetTh2H.w8<<8|pReg->SigDetTh2L.w8))) ? 1:0;

    // ----------------------------------------------------------
    // Wire the Output (preclock to accomodate DFE code ordering)
    // ----------------------------------------------------------
    ClockRegister(p->SigDet);  *SDSigDet = p->SigDet.Q;

    // -------------
    // Trace Signals
    // -------------
    if (pRX->EnableTrace)
    {
        pRX->traceSigDet[pRX->k20].SDEnB = SDEnB.b0;
        pRX->traceSigDet[pRX->k20].State = p->State.Q.w8;
        pRX->traceSigDet[pRX->k20].SigDet= p->SigDet.Q.w2; // only b0 used; b1 available for future use
        pRX->traceSigDet[pRX->k20].x     = x.w20;       
        pRX->traceSigDet[pRX->k20].CheckSigDet = CheckSigDet;
    }
}
// end of Tamale_SignalDetect()

// ================================================================================================
// FUNCTION  : Tamale_DigitalFrontEnd
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void Tamale_DigitalFrontEnd( wiBoolean DFEEnB, wiBoolean DFERestart, wiBoolean CheckRIFS, wiBoolean HoldGain,
                             wiBoolean UpdateAGC, wiBoolean DFELgSigAFE,
                             wiWORD  DFEIn0R,  wiWORD  DFEIn0I,  wiWORD  DFEIn1R,  wiWORD  DFEIn1I,
                             wiWORD *DFEOut0R, wiWORD *DFEOut0I, wiWORD *DFEOut1R, wiWORD *DFEOut1I,
                             wiUWORD *DFELNAGain, wiUWORD *DFEAGain, 
                             wiUWORD *DFERSSI0, wiUWORD *DFERSSI1,
                             Tamale_RxState_t *pRX, TamaleRegisters_t *pReg, wiBoolean CSCCK, 
                             wiBoolean RestartRX, Tamale_aSigState_t *SigOut, Tamale_aSigState_t SigIn )
{
    Tamale_DigitalFrontEnd_t *p = &(pRX->DigitalFrontEnd);

    wiWORD w0R, w0I, w1R, w1I; // reduced resolution version of DFEIn?? for RSSI and SignalDetect
    wiWORD s0R, s0I, s1R, s1I; // wired AGC data output and input for CFO

    wiBoolean GainChange; // flag change in the AGAIN,LNAGAIN values
    wiUWORD   DFEPathSel; // local name for PathSel

    DFEPathSel.word = pReg->PathSel.w2;

    SigOut->PeakFound  = 0;
    SigOut->Run11b     = 0;
    SigOut->dValidRSSI = SigIn.ValidRSSI; // 20 MHz delayed version of ValidRSSI

    // -----------------------
    // Clock Registers (20MHz)
    // -----------------------
    ClockRegister(p->State);
    ClockRegister(p->oldLNAGain);
    ClockRegister(p->oldAGain);
    ClockRegister(p->LgSigDetAFE);

    // ---------------
    // Update Counters
    // ---------------
    ClockRegister(p->counter);
    ClockRegister(p->SigDetCount); 
    ClockRegister(p->RSSITimer);
    p->counter.D.word     = p->counter.Q.w8 + 1;    // default input is incremented count
    p->SigDetCount.D.word = p->SigDetCount.Q.w8 + 1;// default input is incremented count
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Check for Valid RSSI
    //
    //  The RSSI result is invalid during startup and immediately after gain changes. If
    //  a condition which invalidates RSSI exists, the RSSITimer counter is set to the
    //  DelayRSSI value. Once the condition clears, RSSITimer decrements on the 20 MHz
    //  clock. RSSI is valid once the timer expires.
    //
    GainChange = (p->oldAGain.Q.w6 != p->AGain.w6 || p->oldLNAGain.Q.w2 != p->LNAGain.w2) ? 1:0;

    if (GainChange || p->State.Q.w6 == 48 || p->State.Q.w6 == 53 || p->State.Q.w6 == 30)
        p->RSSITimer.D.word = pReg->DelayRSSI.w6;
    else 
        if (p->RSSITimer.Q.w6) 
            p->RSSITimer.D.word = p->RSSITimer.Q.w6 - 1;

    SigOut->ValidRSSI = p->State.Q.w6 && !GainChange && !p->RSSITimer.Q.w6;

    // ----------------------------
    // DFE Controller State Machine
    // ----------------------------
    switch (p->State.Q.w6)
    {
        // --------------
        // Initialize DFE
        // --------------
        case 0 :
            SigOut->SigDet      = 0;
            SigOut->AGCdone     = 0;
            SigOut->SyncFound   = 0;
            SigOut->HeaderValid = 0;
            p->SDEnB.b0        = 1;
            p->FSEnB.b0        = 1;
            p->AGCEnB.b0       = 1;
            p->DAEnB.b0        = 1;
            p->CFOEnB.b0       = 1;
            p->InitGains.b0    = HoldGain? 0:1;
            p->AGCdone.b0      = 0;
            p->PeakFound.b0    = 0;
            p->PeakSearch.b0   = 0;
            p->UpdateGains.b0  = 0;
            p->ForceLinear.b0  = 0;
            p->SigDetValid.b0  = 0;
            p->StartCFO.b0     = 0;
            p->ArmLgSigDet.b0  = 0;
            p->UseRecordPwr.b0 = 0;
            p->RecordPwr.b0    = 0;
            p->RecordRefPwr.b0 = 0;
            p->AntSel.b0       = 0;
            if (pReg->OFDMSwDEn.b0)
                if (!pReg->OFDMSwDSave.b0) 
                    p->Antenna.b0 = pReg->OFDMSwDInit.b0;
                else
                    p->Antenna.b0 = p->Antenna.b0;
            else p->Antenna.b0 = 0;

            SigOut->Run11b = 0;
			SigOut->RIFSDet = 0;
			SigOut->TimeoutRIFS = 0;
            if (p->counter.Q.w8 == 18) // startup delay (includes wait for valid SigDet)
            {
                p->State.D.word=1;
                p->counter.D.word = 0;
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
            SigOut->RIFSDet     = 0;
			SigOut->TimeoutRIFS = 0;
			p->UseRecordPwr.b0 = 0;
            p->RecordPwr.b0 = 0;
            p->AntSel.b0 = 0; 
            p->RSSI_A.word = 0; 
            p->RSSI_B.word = 0;
            p->AGCEnB.b0    = 0; // enable the AGC (incl RSSI)
            p->DAEnB.b0     = pReg->IdleDAEnB.b0; // enable the digital amplifier
            p->SDEnB.b0     = pReg->IdleSDEnB.b0; // enable the signal detector
            p->FSEnB.b0     = pReg->IdleFSEnB.b0; // register selectable enable of FrameSync prior to SigDet
            p->CFOEnB.b0    = 1; // disable/reset the CFO block
            p->InitGains.b0 = 0; // allow gains to change from initial values
            p->ArmLgSigDet.b0 = 0; // clear/squelch large signal detector

            if (p->SigDet.b0)      // signal detected
            {				
                p->State.D.word   = 2;
				p->counter.D.word = 0;
            }
            else if (pReg->RXMode.w2==3 && CSCCK)
            {
                p->State.D.word   = 2;
			    p->counter.D.word = 0;
            }
			
			else if (CheckRIFS && p->counter.Q.w8 == pReg->DelayRIFS.w8) // DelayRIFS: RX RIFS waiting window 
			{
				p->SDEnB.b0 = 1;   // disable Signal Det
                SigOut->TimeoutRIFS = 1; // indicate a RIFS time out
                p->State.D.word = 30;	
			}
            if (!CheckRIFS) p->counter.D.word  = 0;
            p->GainUpdateCount = 0;
            p->SigDetCount.D.word = 0; // clear SigDet counter
            break;

        // --------------------------------------------
        // Delay between Signal Detect and start of AGC
        // --------------------------------------------
        case 2:
            SigOut->SigDet = 1;
            p->FSEnB.b0       = 0; // enable the frame sync regardless of state 1 condition
            p->ArmLgSigDet.b0 = 1; // enable latching of large signal detector
            p->SDEnB.b0 = 1;       // Disable the signal detector
            p->DAEnB.b0 = 0;       // enable the digital amplifier
            p->CFOEnB.b0= 0;       // Enable the CFO block
            
			if(CheckRIFS && p->SigDet.b0) SigOut->RIFSDet = 1;   
			
			if (p->LgSigDetAFE.Q.b0) 
                p->State.D.word = 4;                       // fast start AGC and skip antenna select
            else if (p->counter.Q.w8==pReg->WaitAGC.w7)    // delay for power measurement after signal detect
                p->State.D.word = (pReg->OFDMSwDEn.b0 && !SigOut->RIFSDet) ? 48:4;
            break;

        // -------------------------------------
        // Wait for AGC signal power measurement
        // -------------------------------------
        case 3:
            p->ArmLgSigDet.b0   = pReg->ArmLgSigDet3.b0; // arm large signal detect for successive update
            p->State.D.word = (p->counter.Q.w8 == 15 || p->LgSigDetAFE.Q.b0) ? 4:3;
            break;

        // ------------------------------
        // Update the AGC-controlled gain
        // ------------------------------
        case 4:
            p->ArmLgSigDet.b0  = 1; // arm large signal detect
            SigOut->SigDet  = 1;
            p->FSEnB.b0        = 0; // enable the frame sync (add for SwD)
            p->UpdateGains.b0 = 1;
            p->State.D.word   = 5;
            p->GainUpdateCount++;
            break;

        // --------------------------------------------------
        // Wait for gain update propagation through registers
        // --------------------------------------------------
        case 5:
            p->UseRecordPwr.b0 = 0; // (add for SwD)
            p->ArmLgSigDet.b0  = 0; // clear large signal detect value
            p->UpdateGains.b0  = 0; 
            p->State.D.word    = 6;
            break;

        // -----------------------------------
        // Check the result of the gain update
        // -----------------------------------
        case 6:
        {
            wiBoolean GainChange;

            p->counter.D.word = 0;
            p->ArmLgSigDet.b0 = 0;  // clear latched large sig det value
            GainChange = (p->oldAGain.Q.w6 != p->AGain.w6 || p->oldLNAGain.Q.w2 != p->LNAGain.w2) ? 1:0;

                 if (p->LgSigDFE.b0) p->State.D.word = 7; // nonlinear measurement
            else if (GainChange ) p->State.D.word = 8; // linear measurement, last update
            else                  p->State.D.word = 9; // no analog gain change
        }  break;

        // --------------------------------------------------------------
        // Wait for the gain update to propagate to the DFE
        // ...then continue AGC because the update was for a large signal
        // --------------------------------------------------------------
        case 7:
            p->FSEnB.b0 = 1; // disable frame sync on gain change [NEW: BB, restart after AGC]
            if (p->counter.Q.w8 == pReg->DelayAGC.w5)
            {
                p->State.D.word   = 3;
                p->counter.D.word = 0;
            }
            break;

        // ----------------------------------------------------------------------
        // Wait for the gain update to propagate to the DFE
        // ...then wait for the FrameSync (gain update was for a moderate signal)
        // ----------------------------------------------------------------------
        case 8:
            p->FSEnB.b0 = 1; // disable frame sync on gain change [NEW: BB, restart after AGC]
            if (p->counter.Q.w8 == pReg->DelayAGC.w5)
            {
                p->State.D.word   = 10;
                p->counter.D.word = 0;
            }
            break;

        // ----------------------------------------------------
        // AGC is complete (no update, so no propagation delay)
        // ...wait for FrameSync
        // ----------------------------------------------------
        case 9:
            p->AGCdone.b0     = 1;
            p->State.D.word   = 10;
            p->counter.D.word = 0;
            break;

        // -----------------------------------------------------
        // AGC is complete...delay start of peak search (>= 16T)
        // -----------------------------------------------------
        case 10:
            p->FSEnB.b0         = 0; // enable the frame sync [NEW: BB, restart after AGC]
            p->AGCdone.b0       = 1; // flag AGC complete
            if (p->counter.Q.w8 == pReg->DelayCFO.w5) p->State.D.word = 11;
            break;

        // ---------------------
        // Start the peak search
        // ---------------------
        case 11:
            p->RecordRefPwr.b0 = 1;  // record power reference for StepUp/StepDown signals
            SigOut->AGCdone = 1;  // signal gain update complete
            p->StartCFO.b0     = 1;  // start coarse CFO estimation
            p->PeakSearch.b0   = 1;  // start frame synchronization
            p->counter.D.word  = 0;
            p->State.D.word    = 12;
            break;

        // --------------------------------------------------------------
        // Wait for the FrameSync PeakFound signal or SigDetValid timeout
        // --------------------------------------------------------------
        case 12:
            p->StartCFO.b0  = 0;
            p->RecordRefPwr.b0 = 0; // signal AGC to record packet power level

            if (RestartRX)
            {
                p->State.D.word = 26;
            }
            else if (p->PeakFound.b0)      
            {
                SigOut->SyncFound=  1; // signal synchronization done
                p->State.D.word     = 15; 
                p->counter.D.word   =  0;
            }
            else if (!p->SigDetValid.b0 && (p->SigDetCount.Q.w8 == pReg->SigDetWindow.w8))
            {
                p->State.D.word   = 13; // not a valid 802.11a packet
                p->counter.D.word = 0;
                pRX->EventFlag.b4 = 1;
            }
            break;

        // ------------------------------------------------------
        // OFDM Qualifier Expired: DSSS preamble or False Detect?
        // ------------------------------------------------------
        case 13:
            p->AGCdone.b0    =  0; // reset AGCdone flag
            p->PeakSearch.b0 =  0; // turn off peak search
            p->FSEnB.b0      =  1; // disable frame sync
            SigOut->SigDet=  0; // clear "a" SigDet
            if (RestartRX)
            {
                p->State.D.word = 26;
            }
            else if (pReg->RXMode.w2 == 3 && CSCCK)
            {
                p->State.D.word = pReg->UpdateGain11b.b0 ? 21:28;
            }
            else
            {
                p->InitGains.b0 =  1; // reset gain if not a DSSS preamble
                p->State.D.word = 14;
            }
            break;

        // ----------------------------------
        // Recover from a false signal detect
        // ----------------------------------
        case 14:
            SigOut->Run11b = 0; // stop any DSSS/CCK processing
            if (p->counter.Q.w8 == pReg->DelayAGC.w5)
            {
                p->State.D.word   = 1;
                p->counter.D.word = 0;
            }
            break;

        // ----------------------------------------------------------
        // FrameSync peak found...delay to give desired FFT alignment
        // ----------------------------------------------------------
        case 15:
            SigOut->SigDet = 0; // clear SigDet; SyncFound is active
            p->FSEnB.b0 = 1;       // disable frame sync;
            p->PeakSearch.b0 =  0; // turn off peak search
            if (RestartRX)
                p->State.D.word = 26;
            else 
                if (p->counter.Q.w8 == pReg->SyncShift.w4) p->State.D.word = 16;
            break;

        // ----------------------------------------------------
        // FrameSync complete...continue with packet processing
        // ----------------------------------------------------
        case 16:
            SigOut->PeakFound = 1; // flag peak found from DFE to RX controller
            p->StartCFO.b0   =  1;    // start fine CFO estimation
            p->State.D.word  = 17;
            pRX->EventFlag.word = pRX->EventFlag.word | (p->GainUpdateCount << 14);
            break;

        // ----------------------------------
        // OFDM Packet: Demodulate and Decode
        // ----------------------------------
        case 17:
            SigOut->PeakFound   = 0;
            p->StartCFO.b0         = 0;
            p->counter.D.word      = 0;
            p->SigDetCount.D.word  = 0;
            SigOut->RIFSDet     = 0;
			SigOut->TimeoutRIFS = 0; // Reset RIFS check timeout
            p->SDEnB.b0 = pReg->SDEnB17.b0;
            if      (RestartRX ) p->State.D.word = 26; // abort restart
            else if (UpdateAGC ) p->State.D.word = 18; // HT-STF AGC
			else if (CheckRIFS ) p->State.D.word = 20; // start RIFS searching 
            else if (DFERestart) p->State.D.word = 29; // normal restart
            break;

        case 18: // HT-MF Gain Update in HT-STF
            p->SDEnB.b0 = 1; // disable SigDet
            p->UpdateGains.b0 = pReg->UpdateGainMF.b0;
            p->ForceLinear.b0 = pReg->UpdateGainMF.b0;
            p->State.D.word = 19;
            break;

        case 19:
            SigOut->PeakFound   = 0;
            p->StartCFO.b0         = 0;
            p->counter.D.word      = 0;
            p->SigDetCount.D.word  = 0;
            p->UpdateGains.b0 = 0;
            p->ForceLinear.b0 = 0;
            if      (RestartRX ) p->State.D.word = 26; // abort restart
			else if (CheckRIFS ) p->State.D.word = 20; // start RIFS searching 
            else if (DFERestart) p->State.D.word = 29; // normal restart
            break;

		// -------------------------------------
        // HT OFDM Packet: Carrier Sense in RIFS
        // -------------------------------------
        case 20:
            SigOut->SyncFound = 0; // clear sync indicator on restart
            p->AGCdone.b0    =  0; // reset AGCdone flag
            p->PeakSearch.b0 =  0; // turn off peak search
            p->FSEnB.b0      =  1; // disable frame sync to reset
            p->DAEnB.b0 = 0;     // enable the digital amplifier
            SigOut->SigDet = 0; // clear the signal detect flag
            p->SDEnB.b0 = 0;
            if (p->counter.Q.w8>=(pReg->DelayRestart.w8+20*4)) {p->CFOEnB.b0 = 1;}  // 4us Adv + DelayRestart 
            if (p->SigDet.b0)      // signal detected
            {              
                SigOut->AGCdone = 0;
                SigOut->RIFSDet = 1;
                p->counter.D.word  = 0; 
                p->State.D.word    = 2;                
            }
            else if (p->counter.Q.w8 == pReg->DelayRIFS.w8) // DelayRIFS: RX RIFS waiting window 
            {
                p->SDEnB.b0 = 1;   // disable Signal Det
                SigOut->TimeoutRIFS = 1; // indicate a RIFS time out
                p->State.D.word = 30;			  
            }
            p->SigDetCount.D.word = 0; // clear SigDet counter           
            break; 

        // -------------------------------------------
        // Additional Gain Update for DSSS/CCK Packets
        // -------------------------------------------
        case 21:
            p->UpdateGains.b0 = 1;
            p->GainUpdateCount++;
            p->counter.D.word = 0;
            p->State.D.word   = 22;
            break;

        case 22:
            p->UpdateGains.b0 = 0; 
            if (p->counter.Q.w8 == (pReg->DelayAGC.w5 + pReg->DelayCFO.w5))
                p->State.D.word = 28;
            break;

        // --------------------------------------
        // DSSS/CCK Packet: Demodulate and Decode
        // --------------------------------------
        case 28:
            SigOut->PeakFound = 0;
            SigOut->Run11b    = 1;
            p->DAEnB.b0          = 1; // disable Digital Amp
            p->CFOEnB.b0         = 1; // disable OFDM CFO
            p->StartCFO.b0       = 0;
            p->counter.D.word    = 0;
            p->SigDetCount.D.word= 0;
            if (RestartRX )      p->State.D.word = 26; // abort restart
            else if (DFERestart) p->State.D.word = 29; // normal restart
            break;

        // ---------------------------------------
        // Packet Abort/DFE Restart for New SigDet
        // ---------------------------------------
        case 26:
            SigOut->SyncFound = 0; // clear sync indicator on restart
            p->ArmLgSigDet.b0 = 1; // arm large signal detect
            SigOut->Run11b = 0; // turn off "b" baseband processing
            SigOut->SigDet = 0; // clear SigDet on restart event
            SigOut->AGCdone= 0; // clear AGCdone indicator
            p->SDEnB.b0       = 1; // disable SigDet
            p->CFOEnB.b0      = 1; // disable/reset the CFO block
            p->FSEnB.b0       = 1; // disable/reset FrameSync
            p->DAEnB.b0       = 0; // enable the digital amplifier
            p->PeakSearch.b0  = 0; // turn off peak search
            p->InitGains.b0   = 1; // reset AGC gains to initial values
            p->GainUpdateCount= 0; // WiSE only
            p->AGCdone.b0     = 0; // reset AGCdone flag
            p->State.D.word   = 27;
            break;

        case 27:
            p->counter.D.word    = 0; // clear the counter
            p->CFOEnB.b0         = 0; // Enable the CFO block
            p->InitGains.b0      = 0; // clear InitGains from state 26
            SigOut->SyncFound = 0; // clear sync indicator on restart
            if (SigIn.StepUp)
                p->State.D.word  = 3; // start AGC (assume new packet detect)
            else
                p->State.D.word = 31; // restart DFE
            break;

        // -----------------------------
        // DFE Restart States (RX-to-RX)
        // -----------------------------
        case 29:
            SigOut->SyncFound = 0; // clear sync indicator on restart
            p->AGCdone.b0    = 0;     // reset AGCdone flag
            p->PeakSearch.b0 = 0;     // turn off peak search
            p->SDEnB.b0      = 1;     // disable SigDet
            p->FSEnB.b0      = 1;     // disable frame sync to reset
            p->DAEnB.b0      = 0;     // enable the digital amplifier
            p->CFOEnB.b0     = SigIn.Run11b; // =Run11b.Q (enabled for OFDM)
            SigOut->Run11b= SigIn.Run11b; // hold state of Run11b
            SigOut->SigDet = 0;   // clear the signal detect flag
            if (p->counter.Q.w8 == pReg->DelayRestart.w8)
                p->State.D.word = 30;
            break;

        case 30:
            SigOut->AGCdone=  0; // clear AGCdone indicator
            p->InitGains.b0   =  1; // reset AGC gains to initial values
            p->CFOEnB.b0      =  1; // disable OFDM CFO
            p->counter.D.word =  0; // clear counter
            p->State.D.word   = 31;
            break;

        case 31:
            p->ArmLgSigDet.b0 = 0; // clear large signal detect
            p->InitGains.b0   = 0; // clear gain initialization
            p->FSEnB.b0       = 1; // disable FrameSync during gain settling
            SigOut->Run11b = 0; // end of any DSSS/CCK processing
            if (p->counter.Q.w8 == pReg->DelayAGC.w5)
            {
                p->State.D.word   = 1;
                p->counter.D.word = 0;
            }
            break;

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Antenna Selection
        //
        // -----------
        // Record PwrA
        // -----------
        case 48:
            if (p->AntSearch.b0)
            {
                p->RecordPwr.b0 = 1;
                p->RSSI_A = *DFERSSI0;
                p->Antenna.b0 = p->Antenna.b0 ^ 1;
                p->FSEnB.b0  = 1; // disable the frame sync
                p->counter.D.word = 0;
                p->State.D.word = 49;
            }
            else
                p->State.D.word = 4;
            break;

        // -----------------------
        // Wait for Antenna Switch
        // -----------------------
        case 49:
            p->ArmLgSigDet.b0 = 0; // clear large signal detect
            p->RecordPwr.b0 = 0;
            p->AntSel.b0 = 1;
            if (p->counter.Q.w8>pReg->OFDMSwDDelay.w5)
                p->State.D.word   = 50;
            break;

        // -----------
        // Record PwrB
        // -----------
        case 50:
            p->counter.D.word = 0;
            p->RecordPwr.b0 = 1;
            p->RSSI_B = *DFERSSI0;
            p->State.D.word = 51;
            break;

        // --------------
        // Select Antenna
        // --------------
        case 51:
            p->RecordPwr.b0 = 0;
            if (p->counter.Q.w8 == 1) {
                p->State.D.word = p->AntSuggest.b0 ? 52:53;
            }
            break;

        case 52:
            p->AntSel.b0 = 1;
            p->UseRecordPwr.b0 = 1;
            p->State.D.word = 4;
            p->FSEnB.b0     = 0; // enable the frame sync
            break;

        case 53:
            p->Antenna.b0 = p->Antenna.b0 ^ 1;
            p->AntSel.b0 = 0;
            p->UseRecordPwr.b0 = 1;
            p->State.D.word = 54;
            p->counter.D.word = 0;
            break;

        case 54:
            if (p->counter.Q.w8>pReg->DelayAGC.w5) 
				p->State.D.word = 4;            
            break;

        /////////////////////////////////////////////////////////////////////////////////

        // --------------------------------------
        // Default case: return to initialization
        // --------------------------------------
        default: p->State.D.word = 0; break;
    }
    // ----------------------------------
    // Initialize when DFE is NOT Enabled
    // ----------------------------------
    if (DFEEnB)
    {
        p->State.D.word   = 0;
        p->counter.D.word = 0;
    }
    p->oldLNAGain.D.word = p->LNAGain.w2;
    p->oldAGain.D.word   = p->AGain.w6;

    // ---------------------------------------------
    // Reduced Resolution for RSSI and Signal Detect
    // ---------------------------------------------
    w0R.word = max(-511, min(511, DFEIn0R.w12>>1));
    w0I.word = max(-511, min(511, DFEIn0I.w12>>1));
    w1R.word = max(-511, min(511, DFEIn1R.w12>>1));
    w1I.word = max(-511, min(511, DFEIn1I.w12>>1));
    
    // ----------------------
    // Automatic Gain Control
    // ----------------------
    Tamale_AGC( p->AGCEnB, p->DAEnB, DFEPathSel, p->InitGains, p->UpdateGains, p->ForceLinear, 
                p->LgSigDetAFE.Q.b0, p->UseRecordPwr, p->RecordPwr, p->RecordRefPwr, 
                p->AntSel, &p->AntSuggest, &p->AntSearch, 
                DFEIn0R, DFEIn0I, DFEIn1R, DFEIn1I, &p->LNAGain, &p->AGain, &p->LgSigDFE,
                DFERSSI0, DFERSSI1, &s0R, &s0I, &s1R, &s1I, w0R, w0I, w1R, w1I, pReg, pRX, SigOut );

    // ---------------
    // Signal Detector
    // ---------------
    Tamale_SignalDetect(p->SDEnB, DFEPathSel, w0R, w0I, w1R, w1I, &p->SigDet, pRX, pReg);

    // ---------------------
    // Frame Synchronization
    // ---------------------
    Tamale_FrameSync(p->FSEnB, DFEPathSel, p->AGCdone, p->PeakSearch, s0R, s0I, s1R, s1I, &p->PeakFound, &p->SigDetValid, pRX, pReg);

    // ------------------------
    // Carrier Frequency Offset
    // ------------------------
    Tamale_CFO(p->CFOEnB, DFEPathSel, p->StartCFO, p->PeakFound, s0R, s0I, s1R, s1I, DFEOut0R, DFEOut0I, DFEOut1R, DFEOut1I, pRX, pReg);

    // ---------------------------------------
    // Qualify/Latch AFE Large Signal Detector
    // ---------------------------------------
    p->LgSigDetAFE.D.word = p->ArmLgSigDet.b0 & (DFELgSigAFE || p->LgSigDetAFE.Q.b0) & pReg->LgSigAFEValid.b0;

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
    DFEAGain->word   = p->AGain.w6;
    DFELNAGain->word = p->LNAGain.w2;
    SigOut->AntSel   = p->Antenna.b0;

    // -------------
    // Trace Signals
    // -------------
    if (pRX->EnableTrace)
    {
        pRX->AGain[pRX->k20]        = p->AGain;
        pRX->DFEState[pRX->k20]     = p->State.Q;

        pRX->DFEFlags[pRX->k20].b0  = p->LNAGain.b0;
        pRX->DFEFlags[pRX->k20].b1  = p->AGCEnB.b0;
        pRX->DFEFlags[pRX->k20].b2  = p->SDEnB.b0;
        pRX->DFEFlags[pRX->k20].b3  = p->FSEnB.b0;
        pRX->DFEFlags[pRX->k20].b4  = DFELgSigAFE;
        pRX->DFEFlags[pRX->k20].b5  = 0;
        pRX->DFEFlags[pRX->k20].b6  = p->InitGains.b0;
        pRX->DFEFlags[pRX->k20].b7  = p->SigDet.b0;
        pRX->DFEFlags[pRX->k20].b8  = p->SigDetValid.b0;
        pRX->DFEFlags[pRX->k20].b9  = p->UpdateGains.b0;
        pRX->DFEFlags[pRX->k20].b10 = p->AGCdone.b0;
        pRX->DFEFlags[pRX->k20].b11 = p->PeakSearch.b0;
        pRX->DFEFlags[pRX->k20].b12 = p->PeakFound.b0;
        pRX->DFEFlags[pRX->k20].b13 = SigOut->PeakFound;
        pRX->DFEFlags[pRX->k20].b15 = DFEEnB;
        pRX->DFEFlags[pRX->k20].b16 = p->LNAGain.b1;
        pRX->DFEFlags[pRX->k20].b18 = RestartRX;
        pRX->DFEFlags[pRX->k20].b19 = CSCCK;
        pRX->DFEFlags[pRX->k20].b31 = 0;

        pRX->traceDFE[pRX->k20].traceValid = 1;
        pRX->traceDFE[pRX->k20].DFEEnB     = DFEEnB;
        pRX->traceDFE[pRX->k20].State      = p->State.Q.w6;
        pRX->traceDFE[pRX->k20].LgSigAFE   = DFELgSigAFE;
        pRX->traceDFE[pRX->k20].AntSel     = SigOut->AntSel;
        pRX->traceDFE[pRX->k20].RestartRX  = RestartRX; 
        pRX->traceDFE[pRX->k20].CSCCK      = CSCCK;
        pRX->traceDFE[pRX->k20].Run11b     = SigIn.Run11b;
        pRX->traceDFE[pRX->k20].counter    = p->counter.Q.w8;
        pRX->traceDFE[pRX->k20].PeakFound  = SigIn.PeakFound;
    }
}
// end of Tamale_DigitalFrontEnd()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

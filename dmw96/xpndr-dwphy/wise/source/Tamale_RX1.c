//--< Tamale_RX1.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Tamale - Baseband Receiver Model (RX1 - Non-DFE blocks prior to FFT)
//  Copyright 2001-2003 Bermai, 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include <memory.h>
#include "wiUtil.h"
#include "Tamale.h"

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

#define ClockRegister(arg) {(arg).Q=(arg).D;};
#define ClockDelayLine(SR,value) {(SR).word = ((SR).word << 1) | ((value)&1);};

// ================================================================================================
// FUNCTION  : Tamale_ADC_Core()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models an ADC quadrature pair
// Parameters: OPM    -- Operating Mode, 2 bits
//             rIn    -- Analog intput (complex)
//             rOutR  -- ADC output, real component, 10 bits
//             rOutI  -- ADC output, imag component, 10 bits
// ================================================================================================
void Tamale_ADC_Core(unsigned OPM, wiComplex rIn, wiUWORD *rOutR, wiUWORD *rOutI, Tamale_RxState_t *pRX)
{
    if (OPM == 3)
    {
        int xR, xI;
    
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Quantization to 10 bits
        //
        //  Real and imaginary terms are quantized independently after application of a
        //  common scaling term (ADC.c). If ADC.Rounding is enabled, the signal is 
        //  quantized using proper rounding; otherwise, simple truncation applies. In
        //  both cases an offset can be added to the result.
        //
        if (pRX->ADC.Rounding)
        {
            if (rIn.Real < 0) xR = (int)(pRX->ADC.c * rIn.Real - 0.5) + pRX->ADC.Offset;
            else              xR = (int)(pRX->ADC.c * rIn.Real + 0.5) + pRX->ADC.Offset;

            if (rIn.Imag < 0) xI = (int)(pRX->ADC.c * rIn.Imag - 0.5) + pRX->ADC.Offset;
            else              xI = (int)(pRX->ADC.c * rIn.Imag + 0.5) + pRX->ADC.Offset;
        }
        else
        {
            xR = (int)(pRX->ADC.c * rIn.Real) + pRX->ADC.Offset;
            xI = (int)(pRX->ADC.c * rIn.Imag) + pRX->ADC.Offset;
        }

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Output Clipping to 10 bits (two's complement).
        //
        if (abs(xR) > 511) xR = (xR<0) ? -512:511;
        if (abs(xI) > 511) xI = (xI<0) ? -512:511;

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Format Output with Resolution Limit
        //
        //  The ADC output is offset binary with 512 representing a zero signal level. 
        //  The output is masked which allows a selected number of LSBs to be forced to
        //  zero for the purpose of modeling effective resolutions less than 10 bits.
        //
        rOutR->word = (unsigned)(xR + 512) & pRX->ADC.Mask;
        rOutI->word = (unsigned)(xI + 512) & pRX->ADC.Mask;
    }
    else 
    {   
        rOutR->word = 0; // ADC disabled
        rOutI->word = 0;
    }
}
// end of Tamale_ADC_Core()


// ================================================================================================
// FUNCTION  : Tamale_ADC()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the effect of the analog-to-digital converter
// Parameters: OPM    -- Operating Mode, 2 bits
//             rIn    -- Analog intput (complex)
//             rOutR  -- ADC output, real component, 10 bits
//             rOutI  -- ADC output, imag component, 10 bits
// ================================================================================================
void Tamale_ADC( Tamale_DataConvCtrlState_t SigIn, Tamale_DataConvCtrlState_t *SigOut, 
                 Tamale_RxState_t *pRX, wiComplex ADCAIQ, wiComplex ADCBIQ, 
                 wiUWORD *ADCOut1I, wiUWORD *ADCOut1Q, wiUWORD *ADCOut2I, wiUWORD *ADCOut2Q)
{   
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Pipeline Delay for ADC
    //
    pRX->ADC.rA[pRX->ADC.i] = ADCAIQ;
    pRX->ADC.rB[pRX->ADC.i] = ADCBIQ;
    pRX->ADC.i     = (pRX->ADC.i+1) % 6; // delay is relative to sample clock @ 40 MHz (6T = 150ns)

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Data converter core model
    //
    Tamale_ADC_Core(SigIn.ADCAOPM, pRX->ADC.rA[pRX->ADC.i], ADCOut1I, ADCOut1Q, pRX);
    Tamale_ADC_Core(SigIn.ADCBOPM, pRX->ADC.rB[pRX->ADC.i], ADCOut2I, ADCOut2Q, pRX);
}
// end of Tamale_ADC()

// ================================================================================================
// FUNCTION : Tamale_DSel()
// ------------------------------------------------------------------------------------------------
// Purpose   : Select the analog-to-digital path mapping and data format converion
// Parameters: DSelIn0R   -- data select in  (0) real, 10 bits (circuit input @ 80MHz)
//             DSelIn0I   -- data select in  (0) imag, 10 bits (circuit input @ 80MHz)
//             DSelIn1R   -- data select in  (1) real, 10 bits (circuit input @ 80MHz)
//             DSelIn1I   -- data select in  (1) imag, 10 bits (circuit input @ 80MHz)
//             DSelOut1R  -- data select out (0) real, 10 bits (circuit output @ 80MHz)
//             DSelOut1I  -- data select out (0) imag, 10 bits (circuit output @ 80MHz)
//             DSelOut1R  -- data select out (1) real, 10 bits (circuit output @ 80MHz)
//             DSelOut1I  -- data select out (1) imag, 10 bits (circuit output @ 80MHz)
//             DSelPathSel-- data enable for digital paths, 2 bits (circuit input, constant)
//             Rst40B     -- global reset
// ================================================================================================
void Tamale_DSel( wiUWORD  DSelIn0R, wiUWORD DSelIn0I,  wiUWORD DSelIn1R,  wiUWORD DSelIn1I,
                  wiWORD *DSelOut0R, wiWORD *DSelOut0I, wiWORD *DSelOut1R, wiWORD *DSelOut1I, 
                  wiUWORD DSelPathSel, wiBoolean Rst40B, Tamale_RxState_t *pRX )
{
    if (!Rst40B)
    {
        const wiReg Zero = {{0},{0}};
        pRX->DSel.Out0R = pRX->DSel.Out0I = Zero;
        pRX->DSel.Out1R = pRX->DSel.Out1I = Zero;
    }
    else
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Input Multiplexer and Format Conversion
        //  
        //  The design includes a multiplexer to allow the ADC output to be replaced with
        //  a test bus input. This multiplexer is not shown in the 'C' model. The output
        //  is converted from offset binary to two's complement.
        //
        pRX->DSel.Out0R.D.word = (signed)(DSelIn0R.w10 - 512);
        pRX->DSel.Out0I.D.word = (signed)(DSelIn0I.w10 - 512);
        pRX->DSel.Out1R.D.word = (signed)(DSelIn1R.w10 - 512);
        pRX->DSel.Out1I.D.word = (signed)(DSelIn1I.w10 - 512);

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Output Registers
        //  
        //  If a path is unused (selected by the PathSel register), its output is
        //  explicitly forced to zero.
        //
        DSelOut0R->word = DSelPathSel.b0? pRX->DSel.Out0R.Q.w10:0;
        DSelOut0I->word = DSelPathSel.b0? pRX->DSel.Out0I.Q.w10:0;
        DSelOut1R->word = DSelPathSel.b1? pRX->DSel.Out1R.Q.w10:0;
        DSelOut1I->word = DSelPathSel.b1? pRX->DSel.Out1I.Q.w10:0;

        ClockRegister(pRX->DSel.Out0R);
        ClockRegister(pRX->DSel.Out0I);
        ClockRegister(pRX->DSel.Out1R);
        ClockRegister(pRX->DSel.Out1I);
    }
}
// end of Tamale_DSel()

// ================================================================================================
// FUNCTION  : Tamale_DCX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the ADC DC offset and subtract from the samples
// Clock     : 40 MHz
// Parameters: n         -- path number (model input, specifies instance, -1=RESET)
//             DCXIn     -- data input from data select block, 10 bits (circuit input)
//             DCXOut    -- data output to Downmixer, 10 bits (circuit output)
//             DCXClear  -- reinitialize DCX and clear the output (infinite pole)
// ================================================================================================
void Tamale_DCX( int n, wiWORD DCXIn, wiWORD *DCXOut, wiBoolean DCXClear, Tamale_RxState_t *pRX, 
                 TamaleRegisters_t *pReg ) 
{
    wiReg *z = pRX->DCX.z;

    if (n < 0)
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Update Accumulator Registers
        //
        //  The DCX filter accumulators are implemented as state variables to
        //  improve execution time (compared with accessing individual high and low words
        //  from the register structure). The internal contents and loaded to/from the
        //  register structure by calling this function with a negative path number.
        //
        //  These operations are continuous in the ASIC implementation; i.e., the 
        //  DCXAcc*** registers always reflect the DCX accumulator values.
        //
        //      -1 : Load internal accumulators from the register structure
        //
        //      -2 : Update register structure values from the internal accumulators. For
        //           this case, the "D" side is used so that RegMap operations can reload
        //           the same value with option -1 and not cause a timing error.
        //
        if (n == -1) // load model accumulators from registers
        {
            z[0].Q.word = z[0].D.word = 256*pReg->DCXAcc0RH.w5 + pReg->DCXAcc0RL.w8;
            z[1].Q.word = z[1].D.word = 256*pReg->DCXAcc0IH.w5 + pReg->DCXAcc0IL.w8;
            z[2].Q.word = z[2].D.word = 256*pReg->DCXAcc1RH.w5 + pReg->DCXAcc1RL.w8;
            z[3].Q.word = z[3].D.word = 256*pReg->DCXAcc1IH.w5 + pReg->DCXAcc1IL.w8;
        }
        else if (n == -2) // load register from accumulator
        {
            pReg->DCXAcc0RH.word =            z[0].D.word >> 8;
            pReg->DCXAcc0RL.word = ((unsigned)z[0].D.word) & 0xFF;
            pReg->DCXAcc0IH.word =            z[1].D.word >> 8;
            pReg->DCXAcc0IL.word = ((unsigned)z[1].D.word) & 0xFF;
            pReg->DCXAcc1RH.word =            z[2].D.word >> 8;
            pReg->DCXAcc1RL.word = ((unsigned)z[2].D.word) & 0xFF;
            pReg->DCXAcc1IH.word =            z[3].D.word >> 8;
            pReg->DCXAcc1IL.word = ((unsigned)z[3].D.word) & 0xFF;
        }
        if (DCXOut) DCXOut->word = 0;
    }
    else
    {
        z[n].Q = z[n].D;   // clock register

        if (DCXClear)
        {
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Clear Accumulators
            //
            //  DCXClear is asserted at startup and on gain changes to clear stale values
            //  from the filter. If DCXFastLoad is set, a scaled version of the current
            //  sample is loaded (improves convergence if no signal is present).
            //
            if (!pReg->DCXHoldAcc.b0)
                z[n].D.word  = pReg->DCXFastLoad.b0 ?  (DCXIn.w10 << (2+pReg->DCXShift.w3)) : 0;
            DCXOut->word = 0;
        }
        else
        {
            /////////////////////////////////////////////////////////////////////////////
            //
            //  DCX Filter
            //
            //  Offset cancellation uses a single-pole high pass filter implemented by
            //  subtracting a scaled accumulation of the output. Register DCXShift sets
            //  the scaling term 2^-(DCXShift+2) which adjusts the filter bandwidth.
            //
            wiWORD a, b, c, d;

            b.word = z[n].Q.w13 >> (2+pReg->DCXShift.w3);
            c.word = DCXIn.w10 - b.w11;
            a.word = z[n].Q.w13 + (pReg->DCXHoldAcc.b0? 0:c.w12);
            z[n].D.word = (abs(a.w14)<4096)? a.w13 : (a.w14<0)? -4095:4095;
            d.word = (abs(c.w12)<512)? c.w10 : (c.w12<0)? -511:511;
            DCXOut->word = pReg->DCXSelect.b0? d.w10 : DCXIn.w10;
        }
    }
}
// end of Tamale_DCX()

// ================================================================================================
// FUNCTION  : Tamale_RxIQ()
// ------------------------------------------------------------------------------------------------
// Purpose   : Receive I/Q balance correction
// Clock     : 40 MHz
// Parameters: n       -- path number
//             Xr, Xi  -- input  real/imaginary (I/Q) terms
//             Yr, Yi  -- output real/imaginary (I/Q) terms
// ================================================================================================
void Tamale_RxIQ( int n, wiWORD Xr, wiWORD Xi, wiWORD *Yr, wiWORD *Yi, 
                 Tamale_RxState_t *pRX, TamaleRegisters_t *pReg) 
{
    wiWORD RxIQa11, RxIQa22, RxIQa12;
    wiWORD yR, yI;
 
    // Correction coefficients are selected by a multiplexer for sources from
    //
    //     0 : Register fields (default)
    //     1 : I/Q Calibration engine with a 1T register delay
    //
    if (n == 0)
    {
        switch (pReg->sRxIQ.b0)
        {
            case 0: 
                RxIQa11.word = pReg->RxIQ0a11.word;
                RxIQa22.word = pReg->RxIQ0a22.word;
                RxIQa12.word = pReg->RxIQ0a12.word;
                break;

            case 1: 
                RxIQa11.word = pRX->dRxIQ0a11.word;  pRX->dRxIQ0a11 = pRX->RxIQ0a11;
                RxIQa22.word = pRX->dRxIQ0a22.word;  pRX->dRxIQ0a22 = pRX->RxIQ0a22;
                RxIQa12.word = pRX->dRxIQ0a12.word;  pRX->dRxIQ0a12 = pRX->RxIQ0a12;
                break;           
        }
    }
    else
    {
        switch (pReg->sRxIQ.b1)
        {
            case 0: 
                RxIQa11.word = pReg->RxIQ1a11.word;
                RxIQa22.word = pReg->RxIQ1a22.word;
                RxIQa12.word = pReg->RxIQ1a12.word;
                break;

            case 1: 
                RxIQa11.word = pRX->dRxIQ1a11.word;  pRX->dRxIQ1a11 = pRX->RxIQ1a11;
                RxIQa22.word = pRX->dRxIQ1a22.word;  pRX->dRxIQ1a22 = pRX->RxIQ1a22;
                RxIQa12.word = pRX->dRxIQ1a12.word;  pRX->dRxIQ1a12 = pRX->RxIQ1a12;
                break;           
        }
    }

    // Apply I/Q correction
    // 
    yR.word = Xr.w10 + ((Xi.w10 * RxIQa12.w5) >> 8);
    yI.word = Xi.w10 + ((Xr.w10 * RxIQa12.w5) >> 8);

    yR.word = yR.w11 + ((yR.w11 * RxIQa11.w5) >> 7);	
    yI.word = yI.w11 + ((yI.w11 * RxIQa22.w5) >> 7);	

    if (abs(yR.word)>511) yR.word = (yR.word<0) ? -511:511;
    if (abs(yI.word)>511) yI.word = (yI.word<0) ? -511:511;


    *Yr = yR;
    *Yi = yI;
}
// end of Tamale_RxIQ()


// ================================================================================================
// FUNCTION  : Tamale_Downmix()
// ------------------------------------------------------------------------------------------------
// Purpose   : Frequency shift the receive channel by -10 MHz
// Clock     : 40 MHz
// Parameters: n           -- path number (model input, specifies module instance)
//             DownmixInR  -- mixer input, real, 10 bits (circuit input)
//             DownmixInI  -- mixer input, imag, 10 bits (circuit input)
//             DownmixOutR -- mixer output, real,10 bits (circuit output)
//             DownmixOutI -- mixer output, imag,10 bits (circuit output)
//             Rst40B      -- global reset
//             pRX, pReg   -- pointers to RX state and configuration register arrays
// ================================================================================================
void Tamale_Downmix( int n, wiWORD DownmixInR, wiWORD DownmixInI, wiWORD *DownmixOutR, wiWORD *DownmixOutI, 
                     wiBoolean SquelchRX, Tamale_RxState_t *pRX, TamaleRegisters_t *pReg ) 
{
    Tamale_Downmix_t *p = &(pRX->Downmix);

    if (SquelchRX) // Reset the counter phase and squelch the output
    {
        p->c[n] = pRX->DownmixPhase;  // reset the mixer phase
        p->InR[0].D.word = p->InR[1].D.word = p->InI[0].D.word = p->InI[1].D.word = 0;
        DownmixOutR->word = 0;
        DownmixOutI->word = 0;
    }
    else // Normal operation
    {
        if (pReg->DownmixOff.b0 && !pReg->DownmixConj.b0) // no mixer or conjugation
        {
            DownmixOutR->word = DownmixInR.w10;
            DownmixOutI->word = DownmixInI.w10;
        }
        else // run either mixer, conjugation, or both (use input register)
        {
            wiWORD yR, yI; // computed mixer output words

            // ----------------------------------
            // Input Registers - Limit [-511,511]
            // ----------------------------------
            ClockRegister(p->InR[n]); p->InR[n].D.word = max(-511, DownmixInR.w10);
            ClockRegister(p->InI[n]); p->InI[n].D.word = max(-511, DownmixInI.w10);

            // -----
            // Mixer
            // -----
            switch (p->c[n])
            {
                case 0: yR.word = p->InR[n].Q.w10; yI.word = p->InI[n].Q.w10; break;
                case 1: yR.word = p->InI[n].Q.w10; yI.word =-p->InR[n].Q.w10; break;
                case 2: yR.word =-p->InR[n].Q.w10; yI.word =-p->InI[n].Q.w10; break;
                case 3: yR.word =-p->InI[n].Q.w10; yI.word = p->InR[n].Q.w10; break;
            }

            // -----------------
            // Complex Conjugate
            // -----------------
            if (pReg->DownmixConj.b0) yR.word = -yR.word;

            // -----------------------------
            // 2-bit counter for mixer phase
            // -----------------------------
            p->c[n] = pReg->DownmixOff.b0? 0 : (pReg->DownmixUp.b0? (p->c[n]+3)%4 : (p->c[n]+1)%4);

            DownmixOutR->word = yR.w10;
            DownmixOutI->word = yI.w10;
        }
    }
}
// end of Tamale_Downmix()

// ================================================================================================
// FUNCTION  : Tamale_DownsamFIR()
// ------------------------------------------------------------------------------------------------
// Purpose   : Low pass filter and down sample the signal by 2x
// Clock     : Input @ 40 MHz, Filter and Output at 20 MHz
// Parameters: n          -- filter number (model input, specifies module instance)
//             DownsamIn  -- input, (10 bit circuit input)
//             DownsamOut -- output (12 bit circuit output)
//             Rst20B     -- input, global reset (circuit input)
//             Rst40B     -- input, global reset (circuit input)
// ================================================================================================
void Tamale_DownsamFIR( int n, wiWORD  DownsamIn, wiWORD *DownsamOut, 
                        wiBoolean Rst20B, wiBoolean Rst40B, Tamale_RxState_t *pRX)
{
    const int b[11] = {3, 0, -7, 0, 20, 32, 20, 0, -7, 0, 3};

    int i, s;
    wiWORD a; // accumulator
    int *c = pRX->DownsamFIR.c;

    // --------------
    // 40 MHz Circuit
    // --------------
    if (!Rst40B) {
        c[n] = 0; // reset the output phase counter
    }
    else
    {
        // -------------------------------------------------------------------------
        // Count 20 MHz clock periods in each 40 MHz period
        // Used to model the relationship between registers in the two clock domains
        // -------------------------------------------------------------------------
        c[n] = (c[n]+1)                   % 2; // phase of the T40=1/40MHz period in the T20=1/20 MHz period
        s    = (2-c[n]+pRX->DownsamPhase) % 2; // enumerate the selected input register (1 of 2)
        pRX->DownsamFIR.In[n][s].Q      = pRX->DownsamFIR.In[n][s].D; 
        pRX->DownsamFIR.In[n][s].D.word = DownsamIn.w10;
    }
    // --------------
    // 20 MHz Circuit
    // --------------
    if (c[n] == pRX->DownsamPhase)
    {
        wiReg *x = &(pRX->DownsamFIR.x[n][0]);

        if (!Rst20B)
        {
            for (i=0; i<11; i++) x[i].D.word = x[i].Q.word = 0;
            pRX->DownsamFIR.Out[n].D.word = pRX->DownsamFIR.Out[n].Q.word = 0;
            DownsamOut->word = 0;
        }
        else
        {
            // --------------------
            // Clock the delay line
            // --------------------
            for (i=0; i< 2; i++) { x[i].Q = x[i].D; x[i].D = pRX->DownsamFIR.In[n][i].Q;   }
            for ( ;   i<11; i++) { x[i].Q = x[i].D; x[i].D =  x[i-2].Q; }

            // -----------------------
            // FIR Filter Calculations
            // -----------------------
            a.word = 0;
            for (i=0; i<11; i++) 
                a.w17 = a.w17 + (b[i] * x[i].Q.w10);
            pRX->DownsamFIR.Out[n].D.word = a.w17 / 32;
        }
    }
    // -------------------------
    // Output Register (20 MHz)
    // -------------------------
    if (c[n] == pRX->DownsamPhase) pRX->DownsamFIR.Out[n].Q = pRX->DownsamFIR.Out[n].D;
    DownsamOut->word = pRX->DownsamFIR.Out[n].Q.w12;
}
// end of Tamale_DownsamFIR()

// ================================================================================================
// FUNCTION  : Tamale_Downsam()
// ------------------------------------------------------------------------------------------------
// Purpose   : Low pass filter and down sample the signal by 2x
// Parameters: n           -- path number (model input, specifies module instance)
//             DownsamInR  -- input, real, 10 bits (circuit input @ 40MHz)
//             DownsamInI  -- input, imag, 10 bits (circuit input @ 40MHz)
//             DownsamOutR -- output, real,12 bits (circuit output@ 40MHz)
//             DownsamOutI -- output, imag,12 bits (circuit output@ 40MHz)
//             Rst20B      -- input, global reset  (circuit input @ 20MHz)
//             Rst40B      -- input, global reset  (circuit input @ 40MHz)
// ================================================================================================
void Tamale_Downsam(int n, wiWORD  DownsamInR,  wiWORD  DownsamInI, wiWORD *DownsamOutR, wiWORD *DownsamOutI,
                    wiBoolean Rst20B, wiBoolean Rst40B, Tamale_RxState_t *pRX) 
{
    Tamale_DownsamFIR(n+0, DownsamInR, DownsamOutR, Rst20B, Rst40B, pRX);
    Tamale_DownsamFIR(n+2, DownsamInI, DownsamOutI, Rst20B, Rst40B, pRX);
}
// end of Tamale_Downsam()

// ================================================================================================
// FUNCTION : Tamale_ImageDetector()
// ------------------------------------------------------------------------------------------------
// Purpose   : Detect the signal power of a specified tone 
// Parameters: DownSamOutR -- input sample from downsampler, real, 12 bits (circuit input @ 20MHz)
//             DownSamOutI -- input sample from downsampler, imag, 12 bits (circuit input @ 20MHz)
//             ImgDetEnB   -- input signal from I/Q calibration engine
//             ImgDetDone  -- register output to calibration engine/Register map
//             ImgDetPwr   -- register output to calibration engine/Register map
//             pReg        -- pointer to configuration register
//             pRX         -- pointer to RX state
// ================================================================================================
void Tamale_ImageDetector(wiWORD DownSamOutR, wiWORD DownSamOutI, wiBoolean ImgDetEnB, 
                          wiUReg *ImgDetDone, wiUReg *ImgDetPwr, 
                          TamaleRegisters_t *pReg, Tamale_RxState_t *pRX)
{
    Tamale_ImageDetector_t *pState = &(pRX->ImageDetector);

    wiWORD         coefR, coefI;  
    wiWORD         filtOutR, filtOutI;
    wiWORD         xR, xI;
    wiUWORD        processLen;
    wiBoolean      acmAveEn, pwrCalcEn, keepResult, detDone;

    ClockRegister(pState->EnB);  
    ClockRegister(pState->State);  
    ClockRegister(pState->absSqr);  
    ClockRegister(pState->inR);  
    ClockRegister(pState->inI);
    ClockRegister(pState->preAcmR);  
    ClockRegister(pState->preAcmI);  
    ClockRegister(pState->acmR);  
    ClockRegister(pState->acmI);  
    ClockRegister(pState->acmAveR);  
    ClockRegister(pState->acmAveI);  
    ClockRegister(pState->aveCounter);  
    ClockRegister(pState->delay);  

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    //  Input Registers
    //
    //  Image detector works on samples from one path only at any given time.
    //  
    pState->EnB.D.word = (!pReg->ImgDetEn.b0) && (ImgDetEnB); 
 
    if (pState->EnB.Q.b0)
    {
        pState->inR.D.word = 0;
        pState->inI.D.word = 0;
    }
    else
    {
        if (pState->delay.Q.word == 0)
        {
            pState->inR.D.word = DownSamOutR.w12;
            pState->inI.D.word = DownSamOutI.w12;
        }
        else
        {
            pState->inR.D.word = 0;
            pState->inI.D.word = 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    //  Wire Outputs
    //
    pReg->ImgDetPowerH.word = ImgDetPwr->Q.w16 >> 8; 
    pReg->ImgDetPowerL.word = ImgDetPwr->Q.w16 & 0xFF;
    pReg->ImgDetDone.word   = ImgDetDone->Q.b0;

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    //  Controller State Machine and Internal Signals
    //
    processLen.word = pReg->ImgDetLengthH.b0 * 256 + pReg->ImgDetLength.w8;

    acmAveEn    = (pState->State.Q.w10 == processLen.w9 + 2) ?  1 : 0;
    pwrCalcEn   = (pState->State.Q.w10 == processLen.w9 + 3) ?  1 : 0;
    detDone     = (pState->State.Q.w10 >= processLen.w9 + 4) ?  1 : 0;
    keepResult  = (pState->State.Q.w10 == processLen.w9 + 4) ?  1 : 0;

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    // Trace Signals
    //
    if (pRX->EnableTrace)
    {
        pRX->traceImgDet[pRX->k20].EnB        = pState->EnB.Q.b0;
        pRX->traceImgDet[pRX->k20].State      = pState->State.Q.w10;
        pRX->traceImgDet[pRX->k20].AcmAveEn   = acmAveEn;
        pRX->traceImgDet[pRX->k20].PwrCalcEn  = pwrCalcEn;
        pRX->traceImgDet[pRX->k20].DetDone    = detDone;
        pRX->traceImgDet[pRX->k20].KeepResult = keepResult;
        pRX->traceImgDet[pRX->k20].ImgDetDone = ImgDetDone->Q.b0;
        pRX->traceImgDet[pRX->k20].ImgDetPwr  = ImgDetPwr->Q.w16;
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initial Conditions
    //
    //  The done signal (ImgDetDone->Q) from previous process will be set to zero 
    //  (delayed by 2 cycles) when EnB.D is asserted again.
    //
    //  NOTE: The previous output image power (ImgDetPwr->Q) will remain unchanged until EnB.D is deasserted
    //        and the new measurement is valid.
    //
    if (pState->EnB.Q.b0)
    {
        pState->preAcmR.D.word = 0;
        pState->preAcmI.D.word = 0;
        pState->acmR.D.word    = 0;
        pState->acmI.D.word    = 0;
        pState->State.D.word   = 0;

        pState->acmAveR.D.word    = 0;
        pState->acmAveI.D.word    = 0;
        pState->aveCounter.D.word = 0;
        pState->delay.D.word      = 0;

        ImgDetDone->D.word = 0;
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    //     
    //  Limiter
    //
    xR.word = pState->inR.Q.w12;
    xI.word = pState->inI.Q.w12;

    if (abs(xR.word) > 1023) xR.word = (xR.word < 0) ? -1023 : 1023;
    if (abs(xI.word) > 1023) xI.word = (xI.word < 0) ? -1023 : 1023;

    pState->preAcmR.D.word = xR.w11;
    pState->preAcmI.D.word = xI.w11;

    ///////////////////////////////////////////////////////////////////////////////////////
    //    
    //  Complex Multiply + Accumulator + Limiter
    //
    coefR.word = 256*pReg->ImgDetCoefRH.w4 + pReg->ImgDetCoefRL.w8;
    coefI.word = 256*pReg->ImgDetCoefIH.w4 + pReg->ImgDetCoefIL.w8;
    
    filtOutR.word = (pState->acmR.Q.w18 * coefR.w12) - (pState->acmI.Q.w18 * coefI.w12);
    filtOutI.word = (pState->acmR.Q.w18 * coefI.w12) + (pState->acmI.Q.w18 * coefR.w12);
    filtOutR.word = filtOutR.w30 >> 11;
    filtOutI.word = filtOutI.w30 >> 11;

    filtOutR.word = pState->preAcmR.Q.w11 + filtOutR.w19;
    filtOutI.word = pState->preAcmI.Q.w11 + filtOutI.w19;

    if (abs(filtOutR.word) > 131071) filtOutR.word = (filtOutR.word < 0) ? -131071 : 131071;
    if (abs(filtOutI.word) > 131071) filtOutI.word = (filtOutI.word < 0) ? -131071 : 131071;

    pState->acmR.D.word = filtOutR.w18;
    pState->acmI.D.word = filtOutI.w18;

    ///////////////////////////////////////////////////////////////////////////////////////
    //    
    //  Accumulator Output Averaging
    //
    if (acmAveEn)
    {
        // clear register input
        pState->preAcmR.D.word = 0;
        pState->preAcmI.D.word = 0;
        pState->acmR.D.word    = 0;
        pState->acmI.D.word    = 0;
        pState->inR.D.word     = 0;
        pState->inI.D.word     = 0;
        
        pState->acmAveR.D.word = pState->acmAveR.Q.w25 + pState->acmR.Q.w18;
        pState->acmAveI.D.word = pState->acmAveI.Q.w25 + pState->acmI.Q.w18;

        pState->aveCounter.D.word = pState->aveCounter.Q.w8 + 1;

        pState->delay.D.word = ( pState->aveCounter.Q.w8 == (unsigned int)((1<<pReg->ImgDetAvgNum.w3)-1) )  ?                          0 : pReg->ImgDetDelay.w8;
        pState->State.D.word = ( pState->aveCounter.Q.w8 == (unsigned int)((1<<pReg->ImgDetAvgNum.w3)-1) )  ?  (pState->State.Q.w10 + 1) : 0;
    }
    else
    {
        pState->delay.D.word = pState->delay.Q.w8 == 0 ? 0 : (pState->delay.Q.w8 - 1);
        
        if (pState->delay.Q.word == 0)
            pState->State.D.word  = (pState->State.Q.w10 == processLen.w9 + 5) ? (pState->State.Q.w10) : (pState->State.Q.w10 + 1);
        else
            pState->State.D.word = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    //    
    //  Power Calculation 
    //
    if (pwrCalcEn)
    {
        wiWORD tmpR, tmpI;

        tmpR.word = pState->acmAveR.Q.w25 >> (pReg->ImgDetAvgNum.w3 + pReg->ImgDetPreShift.w3);
        tmpI.word = pState->acmAveI.Q.w25 >> (pReg->ImgDetAvgNum.w3 + pReg->ImgDetPreShift.w3);

        if (abs(tmpR.word) > 2047) tmpR.word = (tmpR.word < 0) ? -2047 : 2047;
        if (abs(tmpI.word) > 2047) tmpI.word = (tmpI.word < 0) ? -2047 : 2047;

        pState->pwr.word = tmpR.w12 * tmpR.w12 + tmpI.w12 * tmpI.w12;
        pState->absSqr.D.word = pState->pwr.w23;
    }
    else
    {
        pState->absSqr.D.word = 0;
    }

    pState->pwr.word = pState->absSqr.Q.w23 >> pReg->ImgDetPostShift.w3;
    pState->pwr.word = pState->pwr.word > 65535 ? 65535 : pState->pwr.w16;

    ///////////////////////////////////////////////////////////////////////////////////////
    //    
    //  Output Registers 
    //
    ImgDetPwr->D.word = keepResult ? pState->pwr.w16 : ImgDetPwr->Q.w16;
    ImgDetDone->D.word = detDone;
}
// end of Tamale_ImageDetector()

// ================================================================================================
// FUNCTION : Tamale_IQCalibration()
// ------------------------------------------------------------------------------------------------
// Purpose   : Calibrate I/Q correction coefficients for Tx/Rx.
// Parameters: ImgDetDone  -- input from image detector
//             ImgDetPwr   -- detected power from image detector
//             ImgDetEnB   -- output signal to image detector
//             pReg        -- pointer to configuration register
//             pRX         -- pointer to RX state
// ================================================================================================
void Tamale_IQCalibration(wiUReg ImgDetDoneReg, wiUReg ImgDetPwrReg, wiBoolean *ImgDetEnB, 
                          TamaleRegisters_t *pReg, Tamale_RxState_t *pRX)
{
    const int a11LUT[61] = {15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 
                            5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0, -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, 
                            -6, -6, -7, -7, -8, -8, -9, -9, -10, -10, -11, -11, -12, -12, -13, -13, 
                            -14, -14, -15};

    const int a22LUT[61] = {-15, -14, -14, -13, -13, -12, -12, -11, -11, -10, -10, -9, -9, -8, -8,
                            -7, -7, -6, -6, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1, 0, 0, 1, 1, 2, 2,
                            3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 
                            14, 14, 15, 15};

    const int a12LUT[31] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
                            -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15};

    wiUWORD   pwrAdjust;
    wiUWORD   gLUTAddr, pLUTAddr;
    wiWORD    gainIndex, phaseIndex; 
    wiBoolean testGain1, testGain2, testPhase1, testPhase2;

    Tamale_TxState_t *pTX = Tamale_TxState();
    Tamale_IQCalibration_t *p = (&Tamale_RxState()->IQCalibration);

    /////////////////////////////////////////////////////////////////
    //
    //  Clock Registers
    // 
    ClockRegister(p->calEn); 
    ClockRegister(p->state); 
    ClockRegister(p->gainLUTIndx);
    ClockRegister(p->phaseLUTIndx); 
    ClockRegister(p->iterationCount);
    ClockRegister(p->neighborCount);
    ClockRegister(p->delayCount);
    ClockRegister(p->pwr);
    ClockRegister(p->skipImgDet);
    ClockRegister(p->solutionFound);
    ClockRegister(p->typeSel);
    ClockRegister(p->adjustVector);
    ClockRegister(p->a11Value);
    ClockRegister(p->a22Value);
    ClockRegister(p->a12Value);

    /////////////////////////////////////////////////////////////////
    //
    //  Update/Clock Correction Coefficient Registers
    //
    pTX->TxIQa11.word = (p->state.Q.w4 != 0 && !pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a11Value.Q.w5 : pTX->TxIQa11.w5;
    pTX->TxIQa22.word = (p->state.Q.w4 != 0 && !pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a22Value.Q.w5 : pTX->TxIQa22.w5;
    pTX->TxIQa12.word = (p->state.Q.w4 != 0 && !pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a12Value.Q.w5 : pTX->TxIQa12.w5;

    if (pReg->PathSel.b0)
    {
        pRX->RxIQ0a11.word = (p->state.Q.w4 != 0 && pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a11Value.Q.w5 : pRX->RxIQ0a11.w5;
        pRX->RxIQ0a22.word = (p->state.Q.w4 != 0 && pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a22Value.Q.w5 : pRX->RxIQ0a22.w5;
        pRX->RxIQ0a12.word = (p->state.Q.w4 != 0 && pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a12Value.Q.w5 : pRX->RxIQ0a12.w5;
    }
    else if (pReg->PathSel.b1)    
    {
        pRX->RxIQ1a11.word = (p->state.Q.w4 != 0 && pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a11Value.Q.w5 : pRX->RxIQ1a11.w5;
        pRX->RxIQ1a22.word = (p->state.Q.w4 != 0 && pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a22Value.Q.w5 : pRX->RxIQ1a22.w5;
        pRX->RxIQ1a12.word = (p->state.Q.w4 != 0 && pReg->CalMode.b0 && pReg->CalCoefUpdate.b0) ? p->a12Value.Q.w5 : pRX->RxIQ1a12.w5;
    }

    /////////////////////////////////////////////////////////////////
    //
    //  Calibration Engine/Register Map
    //
    p->calEn.D.word = pReg->CalRun.b0; 

    pReg->CalDone.b0       = p->state.Q.w4 == 12 ? 1 : 0;
    pReg->CalResultType.b0 = p->state.Q.w4 ==  0 ? 0 : (p->neighborCount.Q.w3 == 4 ? 1 : 0);

    if (p->state.Q.w4 == 12)
    {   
        pReg->CalGain.word  = p->gainLUTIndx.Q.w6;  // load register map with calibration result
        pReg->CalPhase.word = p->phaseLUTIndx.Q.w5; // load register map with calibration result
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    //
    // Trace Signals
    //
    if (pRX->EnableTrace)
    {
        pRX->traceIQCal[pRX->k20].CalEn         = p->calEn.Q.b0;
        pRX->traceIQCal[pRX->k20].State         = p->state.Q.w4;
        pRX->traceIQCal[pRX->k20].Delay         = p->delayCount.Q.w7;
        pRX->traceIQCal[pRX->k20].SkipImgDet    = p->skipImgDet.Q.b0;
        pRX->traceIQCal[pRX->k20].SolFound      = p->solutionFound.Q.b0;
        pRX->traceIQCal[pRX->k20].IteCount      = p->iterationCount.Q.w3;
        pRX->traceIQCal[pRX->k20].NeighborCount = p->neighborCount.Q.w3;
        pRX->traceIQCal[pRX->k20].GainLUTIndx   = p->gainLUTIndx.Q.w6;
        pRX->traceIQCal[pRX->k20].PhaseLUTIndx  = p->phaseLUTIndx.Q.w5;
        pRX->traceIQCal[pRX->k20].CalDone       = pReg->CalDone.b0;
    }

    /////////////////////////////////////////////////////////////////
    //
    //  State Machine Input Signal
    //
    p->ImgDetPwr.word = ImgDetPwrReg.Q.w16;
    p->ImgDetDone     = ImgDetDoneReg.Q.b0;
 

    if (p->skipImgDet.Q.b0)
        p->validPoint = 0;
    else
    {
        if (p->iterationCount.Q.w3 > 0 && pReg->CalPwrAdjust.w1)
            pwrAdjust.w17 = ( p->pwr.Q.w16 * (32 + pReg->CalPwrRatio.w5) ) >> 5;
        else
            pwrAdjust.w17 = ( p->pwr.Q.w16 * (32 - pReg->CalPwrRatio.w5) ) >> 5;


        if (p->ImgDetPwr.w16 < pwrAdjust.w17)
            p->validPoint = 1;
        else
            p->validPoint = 0;
    }

    p->delayCount.D.word = p->delayCount.Q.w7 == 127 ? 127 : (p->delayCount.Q.w7 + 1);

    //////////////////////////////////////////////////////////////////
    //
    //  Calibration State Machine
    //
    switch (p->state.Q.w4)
    {        
        case 0: // initial/idle state ----------------------------------------------- 

            p->gainLUTIndx.D.word  = pReg->CalGain.w6;  // load initial value from register map
            p->phaseLUTIndx.D.word = pReg->CalPhase.w5; // load initial value from register map

            p->delayCount.D.word     = 0; 
            p->iterationCount.D.word = 0; 
            p->neighborCount.D.word  = 0;
            p->adjustVector.D.word   = 0;
            p->pwr.D.word            = 0;
            p->skipImgDet.D.word     = 0;
            p->solutionFound.D.word  = 0;
            p->typeSel.D.word        = 0;
 
            p->recordLUTIndx      = 0;
            p->recordImgPwr       = 0;
            p->clearNeighborCount = 0;
            p->dirChange          = 0;        
            p->imgDetEnable       = 0;
            p->incIteCount        = 0;
            p->incNeighborCount   = 0;
            p->loadCoarseStep     = 0;
            p->loadFineStep       = 0;
    
            *ImgDetEnB         = 1; // model to accommodate the 'return'

            if (p->calEn.Q.b0) p->state.D.word = 1;
            return;

        case 1: //-------------------------------------------------------------------

            if (!p->ImgDetDone)
            {
                p->imgDetEnable = 1; // enable image detector
                p->state.D.word = 2;
            }
            break;

        case 2: // wait for initial power measuremnt result -------------------------

            if (p->ImgDetDone)
            {
                p->imgDetEnable      = 0; // disable image detector
                p->recordImgPwr      = 1; // record image power
                p->loadCoarseStep    = 1; // load coarse-step from register map
                p->delayCount.D.word = 0; // reset delay counter
                p->state.D.word      = 3;
            }
            break;

        case 3: //-------------------------------------------------------------------
 
            p->recordImgPwr   = 0; // disable image power recording
            p->loadCoarseStep = 0; // disable coarse step loading

            if (!p->ImgDetDone) p->state.D.word = 4;
            break;

        case 4: //-------------------------------------------------------------------
 
            p->imgDetEnable = 1; // enable image detector
  
            if (p->solutionFound.Q.b0)
            {
                p->imgDetEnable = 0; // disable image detector
                p->state.D.word = 12;
            }
            else if (p->ImgDetDone || p->skipImgDet.Q.b0)
            {
                p->imgDetEnable = 0; // disable image detector

                if (p->validPoint)
                {
                    p->recordLUTIndx      = 1; // record LUT index
                    p->recordImgPwr       = 1; // record image power
                    p->clearNeighborCount = 1; // set neighbor count to zero
                }
                else
                {
                    p->dirChange        = 1; // change sign of adjustVector
                    p->incNeighborCount = 1; // increment neighbor count
                }    

                p->delayCount.D.word = 0; // reset delay counter
                p->state.D.word      = 5;
            }
            break;

        case 5: //-------------------------------------------------------------------

            p->recordLUTIndx      = 0; // disable LUT index recording
            p->recordImgPwr       = 0; // disable image power recording
            p->clearNeighborCount = 0; // disable neighbor count clearing
            p->dirChange          = 0; // disable sign change on adjustVector
            p->incNeighborCount   = 0; // disable neighbor count increment

            if (!p->ImgDetDone)
            {
                if (p->typeSel.Q.b0) p->state.D.word = 9;
                else                 p->state.D.word = 6;
            }
            break;

        case 6: //-------------------------------------------------------------------

            p->imgDetEnable = 1; // enable image detector

            if (p->solutionFound.Q.b0)
            {
                p->imgDetEnable = 0; // disable image detector
                p->state.D.word = 12;
            }
            else if (p->ImgDetDone || p->skipImgDet.Q.b0)
            {
                p->imgDetEnable      = 0; // enable image detector       
                p->delayCount.D.word = 0; // reset delay counter

                if (p->validPoint)
                {
                    p->recordLUTIndx      = 1; // record LUT index
                    p->recordImgPwr       = 1; // record image power
                    p->clearNeighborCount = 1; // set neighbor count to zero
                    p->state.D.word       = 7;
                }
                else
                {
                    p->incNeighborCount = 1; // increment neighbor count
                    p->typeSel.D.b0     = 1; // switch to a12 calibration
                    p->state.D.word     = 8;
                }
            }
            break;

        case 7: //-------------------------------------------------------------------

            p->recordLUTIndx      = 0; // disable LUT index recording
            p->recordImgPwr       = 0; // disable image power recording
            p->clearNeighborCount = 0; // disable neighbor count clearing

            if (!p->ImgDetDone) p->state.D.word = 6;
            break;

        case 8: //-------------------------------------------------------------------

            p->incNeighborCount = 0; // disable neighbor count increment        

            if (!p->ImgDetDone) p->state.D.word = 4;
            break;

        case 9: //-------------------------------------------------------------------

            p->imgDetEnable = 1; // enable image detector

            if (p->solutionFound.Q.b0)
            {
                p->imgDetEnable = 0; // disable image detector
                p->state.D.word = 12;
            }
            else if (p->ImgDetDone || p->skipImgDet.Q.b0)
            {
                p->imgDetEnable      = 0; // disable image detector
                p->delayCount.D.word = 0; // reset delay counter
       
                if (p->validPoint)
                {
                    p->recordLUTIndx      = 1; // record LUT index
                    p->recordImgPwr       = 1; // record image power
                    p->clearNeighborCount = 1; // set neighbor count to zero
                    p->state.D.word       = 10;
                }
                else
                {
                    p->incIteCount      = 1; // increment fine-step loop count
                    p->loadFineStep     = 1; // load fine-step from register map
                    p->incNeighborCount = 1; // increment neighbor count
                    p->typeSel.D.b0     = 0; // switch to a11/a22 calibration
                    p->state.D.word     = 11;
                }
            }
            break;

        case 10: //------------------------------------------------------------------

            p->recordLUTIndx      = 0; // disable LUT index recording
            p->recordImgPwr       = 0; // disable image power recording
            p->clearNeighborCount = 0; // disable neighbor count clearing

            if (!p->ImgDetDone) p->state.D.word = 9;
            break;

        case 11: //------------------------------------------------------------------

            p->incIteCount      = 0; // disable fine-step loop count increment
            p->loadFineStep     = 0; // disable fine-step loading
            p->incNeighborCount = 0; // disable neighbor count increment

            if (!p->ImgDetDone) p->state.D.word = 4;
            break;
         
        case 12: // solution found and wait for calEn deassertion ------------------
            break;

        default: p->state.D.word = 0; break;
    }
   
    if (!p->calEn.Q.b0) p->state.D.word = 0;


    ///////////////////////////////////////////////////////////////////////
    //
    //  Signal to Image Detector
    // 
    *ImgDetEnB = ((p->delayCount.Q.w7 >= pReg->CalDelay.w7) && p->imgDetEnable && (!p->skipImgDet.Q.b0)) ? 0 : 1;   

    ///////////////////////////////////////////////////////////////////////
    //
    //  Registers Update
    // 
    p->pwr.D.word = p->recordImgPwr ? p->ImgDetPwr.w16 : p->pwr.Q.w16;

    p->iterationCount.D.word = p->incIteCount ? (p->iterationCount.Q.w3 + 1) : p->iterationCount.Q.w3;

    p->neighborCount.D.word = p->clearNeighborCount ? 0 : ((p->iterationCount.Q.w3 > 0 && p->incNeighborCount) ? (p->neighborCount.Q.w3 + 1) : p->neighborCount.Q.w3);

    p->solutionFound.D.b0 = ((p->iterationCount.Q.w3 > (pReg->CalIterationTh.w2+1)) || p->neighborCount.Q.w3 == 4) ? 1 : 0;

         if (p->loadCoarseStep) p->adjustVector.D.word = pReg->CalCoarseStep.w3;
    else if (p->dirChange)      p->adjustVector.D.word = 0 - p->adjustVector.Q.w4;
    else if (p->loadFineStep)   p->adjustVector.D.word = 1 + pReg->CalFineStep.w1;
    else                     p->adjustVector.D.word = p->adjustVector.Q.w4;

    p->gainLUTIndx.D.word  = (p->recordLUTIndx && !p->typeSel.Q.b0) ? (p->gainLUTIndx.Q.w6  + p->adjustVector.Q.w4) : p->gainLUTIndx.Q.w6;
    p->phaseLUTIndx.D.word = (p->recordLUTIndx &&  p->typeSel.Q.b0) ? (p->phaseLUTIndx.Q.w5 + p->adjustVector.Q.w4) : p->phaseLUTIndx.Q.w5;

    ///////////////////////////////////////////////////////////////////////
    //
    //  Generate Look-up Table Read Address
    // 
    gainIndex.word  = (p->typeSel.Q.b0 == 0) ? (p->gainLUTIndx.Q.w6  + p->adjustVector.Q.w4) : 30;
    phaseIndex.word = (p->typeSel.Q.b0 == 1) ? (p->phaseLUTIndx.Q.w5 + p->adjustVector.Q.w4) : 15;

    testGain1  = ( gainIndex.w8 < 0) || ( gainIndex.w8 > 60);
    testPhase1 = (phaseIndex.w7 < 0) || (phaseIndex.w7 > 30);

    p->skipImgDet.D.b0 = (testGain1 || testPhase1) ? 1 : 0;

    testGain2  = (!testGain1 ) && (!p->typeSel.Q.b0);
    testPhase2 = (!testPhase1) && ( p->typeSel.Q.b0);

    gLUTAddr.w6 = (!p->solutionFound.Q.b0 &&  testGain2) ?  gainIndex.w6 :  p->gainLUTIndx.Q.w6;
    pLUTAddr.w5 = (!p->solutionFound.Q.b0 && testPhase2) ? phaseIndex.w5 : p->phaseLUTIndx.Q.w5;

    ///////////////////////////////////////////////////////////////////////
    //
    //  Look-up Table Output
    // 
    p->a11Value.D.word = a11LUT[gLUTAddr.w6];
    p->a22Value.D.word = a22LUT[gLUTAddr.w6];
    p->a12Value.D.word = a12LUT[pLUTAddr.w5];
}
// end of Tamale_IQCalibration()

// ================================================================================================
// FUNCTION  : Tamale_RX1_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Reset model state in anticipation of an RX operation
// Parameters: none
// ================================================================================================
wiStatus Tamale_RX1_Restart(void)
{
    wiWORD   Zero = {0}, Dummy; 
    wiUWORD UZero = {0};
    int i;
    Tamale_RxState_t  *pRX  = Tamale_RxState();
    TamaleRegisters_t *pReg = Tamale_Registers();

    // Reset DSel Registers
    //
    Tamale_DSel(UZero, UZero, UZero, UZero, &Dummy, &Dummy, &Dummy, &Dummy, pReg->PathSel, 0, pRX);

    // Reset the phase counters in the mixer and LPF
    //
    Tamale_Downsam(0, Zero, Zero, &Dummy, &Dummy, 0, 0, pRX);
    Tamale_Downsam(1, Zero, Zero, &Dummy, &Dummy, 0, 0, pRX);

    Tamale_Downmix(0, Zero, Zero, &Dummy, &Dummy, 1, pRX, pReg);
    Tamale_Downmix(1, Zero, Zero, &Dummy, &Dummy, 1, pRX, pReg);

    // Clear the downmix output arrays
    //
    for (i=0; i<2; i++)
    {   
        if (pRX->zReal[i]) memset(pRX->zReal[i], 0, pRX->N40*sizeof(wiWORD));
        if (pRX->zImag[i]) memset(pRX->zImag[i], 0, pRX->N40*sizeof(wiWORD));
        if (pRX->yReal[i]) memset(pRX->yReal[i], 0, pRX->N20*sizeof(wiWORD));
        if (pRX->yImag[i]) memset(pRX->yImag[i], 0, pRX->N20*sizeof(wiWORD));
    }
  
    // Reset/Disable the DCX block
    //
    Tamale_DCX(-1, Zero, NULL, 0, pRX, pReg);

    // Define the ADC Mask for Effective Resolution
    //
    pRX->ADC.Mask = ((1<<(pRX->ADC.nEff))-1) << (10-pRX->ADC.nEff);
    pRX->ADC.c    = 1024.0 / pRX->ADC.Vpp;

    return WI_SUCCESS;
}
// end of Tamale_RX1_Restart()

// ================================================================================================
// FUNCTION : Tamale_RX1()
// ------------------------------------------------------------------------------------------------
// Purpose  : Front-end top for OFDM Baseband (ADC output to FFT input) with cycle
//            accurate timing.
// Timing   : Call on 40 MHz clock
// ================================================================================================
wiStatus Tamale_RX1( wiBoolean DFEEnB, wiBoolean LgSigAFE, wiUWORD *LNAGainOFDM, wiUWORD *AGainOFDM, 
                     wiUWORD *RSSI0OFDM, wiUWORD *RSSI1OFDM, 
                     wiBoolean CSCCK, wiBoolean bPathMux, wiReg *bInR, wiReg *bInI, 
                     wiBoolean RestartRX, wiBoolean DFERestart, wiBoolean CheckRIFS, wiBoolean HoldGain,
                     wiBoolean UpdateAGC, wiBoolean ClearDCX, wiBoolean SquelchRX,
                     Tamale_aSigState_t *aSigIn, Tamale_aSigState_t aSigOut, 
                     Tamale_RxState_t *pRX, TamaleRegisters_t *pReg )
{
    int k40 = pRX->k40;
    int k20 = pRX->k20;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Front-End Signal Processing: HPF and Mixer for all modes (802.11a/b/g)
    //
    Tamale_DSel(pRX->qReal[0][k40], pRX->qImag[0][k40], pRX->qReal[1][k40], pRX->qImag[1][k40], 
                pRX->rReal[0]+k40,  pRX->rImag[0]+k40,  pRX->rReal[1]+k40,  pRX->rImag[1]+k40, pReg->PathSel, 1, pRX);

    aSigIn->LargeDCX = 0;
 
    if (pReg->PathSel.b0)
    { 
        Tamale_DCX(0, pRX->rReal[0][k40], pRX->cReal[0]+k40, ClearDCX, pRX, pReg);
        Tamale_DCX(1, pRX->rImag[0][k40], pRX->cImag[0]+k40, ClearDCX, pRX, pReg);
        Tamale_RxIQ(0, pRX->cReal[0][k40], pRX->cImag[0][k40], pRX->tReal[0]+k40, pRX->tImag[0]+k40, pRX, pReg);
        Tamale_Downmix(0, pRX->tReal[0][k40], pRX->tImag[0][k40], pRX->zReal[0]+k40, pRX->zImag[0]+k40, 
                       SquelchRX, pRX, pReg);
    }
    if (pReg->PathSel.b1)
    {
        Tamale_DCX(2, pRX->rReal[1][k40], pRX->cReal[1]+k40, ClearDCX, pRX, pReg);
        Tamale_DCX(3, pRX->rImag[1][k40], pRX->cImag[1]+k40, ClearDCX, pRX, pReg);
        Tamale_RxIQ(1, pRX->cReal[1][k40], pRX->cImag[1][k40], pRX->tReal[1]+k40, pRX->tImag[1]+k40, pRX, pReg);
        Tamale_Downmix(1, pRX->tReal[1][k40], pRX->tImag[1][k40], pRX->zReal[1]+k40, pRX->zImag[1]+k40,
                       SquelchRX, pRX, pReg);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  OFDM Lowpass Filter in 802.11a/g/n mode
    //
    if (pReg->RXMode.b0)
    {
        Tamale_Downsam(0, pRX->zReal[0][k40], pRX->zImag[0][k40], pRX->yReal[0]+k20, pRX->yImag[0]+k20, 1, 1, pRX);
        Tamale_Downsam(1, pRX->zReal[1][k40], pRX->zImag[1][k40], pRX->yReal[1]+k20, pRX->yImag[1]+k20, 1, 1, pRX);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Digital Front End (Runs at 20 MHz in 802.11a/g/n mode)
    //
    if (pRX->clock.posedgeCLK20MHz && pReg->RXMode.b0)
    {
        Tamale_DigitalFrontEnd( DFEEnB, DFERestart, CheckRIFS, HoldGain, UpdateAGC, LgSigAFE,
                                pRX->yReal[0][k20], pRX->yImag[0][k20], pRX->yReal[1][k20], pRX->yImag[1][k20],
                                pRX->xReal[0]+k20,  pRX->xImag[0]+k20,  pRX->xReal[1]+k20,  pRX->xImag[1]+k20, 
                                LNAGainOFDM, AGainOFDM, RSSI0OFDM, RSSI1OFDM, 
                                pRX, pReg, CSCCK, RestartRX, aSigIn, aSigOut); 
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  I/Q Calibration and Image Detector (Run at 20 MHz in 802.11a/g/n mode)
    //
    if (pRX->clock.posedgeCLK20MHz && pReg->RXMode.b0 && pRX->EnableIQCal)
    {
        // interface between Calibration Engine and Image Detector
        ClockRegister(pRX->RX1.ImgDetDone);  
        ClockRegister(pRX->RX1.ImgDetPwr);    


        Tamale_IQCalibration(pRX->RX1.ImgDetDone, pRX->RX1.ImgDetPwr, &pRX->RX1.ImgDetEnB, pReg, pRX);


        if (pReg->PathSel.b0)
        {
            Tamale_ImageDetector(pRX->yReal[0][k20], pRX->yImag[0][k20], 
                                 pRX->RX1.ImgDetEnB, &pRX->RX1.ImgDetDone, &pRX->RX1.ImgDetPwr, pReg, pRX);
        }
        else if (pReg->PathSel.b1)
        {
            Tamale_ImageDetector(pRX->yReal[1][k20], pRX->yImag[1][k20],
                                 pRX->RX1.ImgDetEnB, &pRX->RX1.ImgDetDone, &pRX->RX1.ImgDetPwr, pReg, pRX);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  DSSS/CCK Receiver Input Mux
    //
    bInR->D.word = bPathMux ? pRX->zReal[1][k40].w10 : pRX->zReal[0][k40].w10;
    bInI->D.word = bPathMux ? pRX->zImag[1][k40].w10 : pRX->zImag[0][k40].w10;

    return WI_SUCCESS;
}
// end of Tamale_RX1()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

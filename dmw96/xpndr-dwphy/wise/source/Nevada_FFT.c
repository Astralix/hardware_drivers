//--< Nevada_FFT.c >-------------------------------------------------------------------------------
//=================================================================================================
//
//  Nevada Baseband (I)FFT Module
//  Copyright 2001 Bermai, 2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <math.h>
#include "wiType.h"

#define   FFTWLn     15    // Word Length of general data in DataPath
#define   FFTBFWLn   17    // Word Length of butterfly/multiplier output
#define   FFTTFWLn   10    // Word Length of twiddle factors
#define   FFTTFFBn    8    // Number of Fractional Bits of Twiddle Factors

#define   FFTWL     w15    // Word Length of general data in DataPath
#define   FFTBFWL   w17    // Word Length of butterfly/multiplier output
#define   FFTTFWL   w10    // Word Length of twiddle factors

#define   BFWLPLUSONE         w18 // Word Length used in complex multiplier
#define   BFWLPLUSTFWL        w27
#define   BFWLPLUSTFWLPLUSONE w28

// ================================================================================================
static void fixFFT64(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int shift);
static void FFTFourPointButterfly(wiWORD *inr, wiWORD *ini, wiWORD *outr, wiWORD *outi);
static void FFTComplexMultiplier(wiWORD Ar, wiWORD Ai, wiWORD Br, wiWORD Bi, wiWORD *Yr, wiWORD *Yi);
static int  FFTSaturation(wiWORD x, int ifft);
static void FFTROMTF(int idx, wiWORD *TFr, wiWORD *TFi);
// ================================================================================================

// ================================================================================================
// FUNCTION  : Nevada_FFT()
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement the transmit IFFT + guard interval insertion and the recieve FFT
// Parameters: TxBRx    -- Transmit (0) or receive (1) flag (circuit input)
//             FFTTxDIR -- Transmit data input, real component (9 bit circuit input)
//             FFTTxDII -- Transmit data input, imag component (9 bit circuit input)
//             FFTRxDIR -- Receive data input, real component (15 bit circuit input)
//             FFTRxDII -- Receive data input, imag component (15 bit circuit input)
//             FFTTxDOR -- Transmit data output, real component (10 bit circuit output)
//             FFTTxDOI -- Transmit data output, imag component (10 bit circuit output)
//             FFTRxDOR -- Receive data output, real component (11 bit circuit output)
//             FFTRxDOI -- Receive data output, imag component (11 bit circuit output)
// ================================================================================================
void Nevada_FFT ( int TxBRx, wiWORD FFTTxDIR[], wiWORD FFTTxDII[], wiWORD FFTRxDIR[], wiWORD FFTRxDII[],
                             wiWORD *FFTTxDOR,  wiWORD *FFTTxDOI, wiWORD *FFTRxDOR,  wiWORD *FFTRxDOI )
{
    int k;

    // ----------------------------------------------------
    // Offset output array to account for FFT block latency
    // ----------------------------------------------------
    FFTRxDOR += 82;
    FFTRxDOI += 82;
    
    // -------------------------------
    // Implement (I)FFT using fixFFT64
    // -------------------------------
    if (TxBRx)  // receiver (FFT)
    {
        fixFFT64(FFTRxDIR, FFTRxDII, FFTRxDOR, FFTRxDOI, 0);

        for (k=0; k<64; k++) 
        {
            FFTRxDOR[k].word = FFTRxDOR[k].w15 >> 4;
            FFTRxDOI[k].word = FFTRxDOI[k].w15 >> 4;
        }
    }
    else // transmitter (IFFT)
    {
        fixFFT64(FFTTxDIR, FFTTxDII, FFTTxDOR, FFTTxDOI, 1);

        for (k=0; k<64; k++) 
        {
            if (FFTTxDOR[k].w15 != FFTTxDOR[k].w10) FFTTxDOR[k].word = (FFTTxDOR[k].word<0)? -512:511;
            if (FFTTxDOI[k].w15 != FFTTxDOI[k].w10) FFTTxDOI[k].word = (FFTTxDOI[k].word<0)? -512:511;
        }
    }
}
// end of Nevada_FFT()

// ================================================================================================
// fixFFT64
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static void fixFFT64(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft)
{
    int i, j, k;
    wiWORD BFIr[4], BFIi[4], BFOr[4], BFOi[4], TFr[3], TFi[3];
    wiWORD memr[64], memi[64];

    //for ifft, flip and conjugate the inputs.
    if (ifft)
    {
        for (i=0; i<64; i++)
        {
            memr[i].word = xr[(i+32)%64].word;
            memi[i].word = -xi[(i+32)%64].word;
        }
    }
    else
    {
        for (i=0; i<64; i++)
        {
            memr[i].word = xr[i].word;
            memi[i].word = xi[i].word;
        }
    }

    //the 1st stage of SFG
    for (i=0; i<16; i++)
    {
        for (j=0; j<4; j++)
        {
            BFIr[j].word = memr[j*16+i].word;
            BFIi[j].word = memi[j*16+i].word;
        }
        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
        FFTROMTF(i, TFr, TFi);
        for (j=1; j<4; j++)   FFTComplexMultiplier(BFOr[j], BFOi[j], TFr[j-1], TFi[j-1], memr+j*16+i, memi+j*16+i);
        memr[i].word = BFOr[0].word;
        memi[i].word = BFOi[0].word;
    }
    for (j=0; j<64; j++)
    {
        memr[j].word = FFTSaturation(memr[j], ifft);
        memi[j].word = FFTSaturation(memi[j], ifft);
    }

    //the 2nd stage of SFG
    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            for (k=0; k<4; k++)
            {
                BFIr[k].word = memr[i*16+j+k*4].word;
                BFIi[k].word = memi[i*16+j+k*4].word;
            }
            FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
            FFTROMTF(j*4, TFr, TFi);
            for (k=1; k<4; k++)   FFTComplexMultiplier(BFOr[k], BFOi[k], TFr[k-1], TFi[k-1], memr+i*16+j+k*4, memi+i*16+j+k*4);
            memr[i*16+j].word = BFOr[0].word;
            memi[i*16+j].word = BFOi[0].word;
            }
    }
    for (j=0; j<64; j++)
    {
        memr[j].word = FFTSaturation(memr[j], ifft*2);
        memi[j].word = FFTSaturation(memi[j], ifft*2);
    }

    //the 3rd stage of SFG, no twiddle factor multiplication
    for (i=0; i<64; i=i+4)
    {
        for (j=0; j<4; j++)
        {
            BFIr[j].word = memr[i+j].word;
            BFIi[j].word = memi[i+j].word;
        }
        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
        for (j=0; j<4; j++)
        {
            memr[i+j].word = BFOr[j].word;
            memi[i+j].word = BFOi[j].word;
        }
    }
    for (j=0; j<64; j++)
    {
        memr[j].word = FFTSaturation(memr[j], ifft);
        memi[j].word = FFTSaturation(memi[j], ifft);
    }

    //reorder the outputs
    if (ifft)
    {
        for (i=0; i<64; i++)      //if ifft, conjugate output
        {
            yr[i+16].word = memr[(i%4)*16 + ((i/4)%4)*4 + i/16].word;
            yi[i+16].word = -memi[(i%4)*16 + ((i/4)%4)*4 + i/16].word;
        }
        for (i=0; i<16; i++)      //grard interval
        {
            yr[i].word = yr[i+64].word;
            yi[i].word = yi[i+64].word;
        }
    }
    else
    {
        for (i=0; i<64; i++)      //if fft, flip output
        {
            j = (i+32)%64;
            yr[i].word = memr[(j%4)*16 + ((j/4)%4)*4 + j/16].word;
            yi[i].word = memi[(j%4)*16 + ((j/4)%4)*4 + j/16].word;
        }
    }
}
// ================================================================================================
// FFTFourPointButterfly
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static void FFTFourPointButterfly(wiWORD *xr, wiWORD *xi, wiWORD *outr, wiWORD *outi)
{
    wiWORD   yr[4], yi[4];

    yr[0].word = xr[0].FFTWL + xr[2].FFTWL;
    yr[1].word = xr[0].FFTWL - xr[2].FFTWL;
    yr[2].word = xr[1].FFTWL + xr[3].FFTWL;
    yr[3].word = xr[1].FFTWL - xr[3].FFTWL;
    yi[0].word = xi[0].FFTWL + xi[2].FFTWL;
    yi[1].word = xi[0].FFTWL - xi[2].FFTWL;
    yi[2].word = xi[1].FFTWL + xi[3].FFTWL;
    yi[3].word = xi[1].FFTWL - xi[3].FFTWL;

    outr[0].word = yr[0].FFTBFWL + yr[2].FFTBFWL;
    outr[1].word = yr[1].FFTBFWL + yi[3].FFTBFWL;
    outr[2].word = yr[0].FFTBFWL - yr[2].FFTBFWL;
    outr[3].word = yr[1].FFTBFWL - yi[3].FFTBFWL;
    outi[0].word = yi[0].FFTBFWL + yi[2].FFTBFWL;
    outi[1].word = yi[1].FFTBFWL - yr[3].FFTBFWL;
    outi[2].word = yi[0].FFTBFWL - yi[2].FFTBFWL;
    outi[3].word = yi[1].FFTBFWL + yr[3].FFTBFWL;

}
// ================================================================================================
// FFTComplexMultiplier
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static void FFTComplexMultiplier(wiWORD ar, wiWORD ai, wiWORD br, wiWORD bi, wiWORD *Yr, wiWORD *Yi)
{
    wiWORD   f1, f2, f3, f4, f5, f6, f7, f8;

    f1.word = ar.FFTBFWL - ai.FFTBFWL;
    f2.word = br.FFTTFWL - bi.FFTTFWL;
    f3.word = br.FFTTFWL + bi.FFTTFWL;
    f4.word = ar.FFTBFWL * f2.FFTTFWL;
    f5.word = ai.FFTBFWL * f3.FFTTFWL;
    f6.word = f1.BFWLPLUSONE * bi.FFTTFWL;
    f7.word = f4.BFWLPLUSTFWL + f6.BFWLPLUSTFWLPLUSONE;
    f8.word = f5.BFWLPLUSTFWL + f6.BFWLPLUSTFWLPLUSONE;

    (*Yr).word = f7.word >> FFTTFFBn;
    (*Yi).word = f8.word >> FFTTFFBn;
}
// ================================================================================================
// FFTSaturation
// ------------------------------------------------------------------------------------------------
// Usage: shift could be only 0, 1 or 2.
// ================================================================================================
static int FFTSaturation(wiWORD x, int shift)
{
    wiWORD y, SgnBits;
    if (shift==2)
    {
        y.word = x.FFTBFWL>>2;
        SgnBits.w4 = (y.word>>(FFTWLn-1)) & 15;
        if ((SgnBits.b3==1) && (SgnBits.w4!=-1))   y.word=-(1<<(FFTWLn-1));
        else if ((SgnBits.b3==0) && (SgnBits.w4!=0))   y.word=(1<<(FFTWLn-1))-1;
    }
    else if (shift==1)
    {
        y.word = x.FFTBFWL>>1;
        SgnBits.w5 = (y.word>>(FFTWLn-1)) & 31;
        if ((SgnBits.b4==1) && (SgnBits.w5!=-1))   y.word=-(1<<(FFTWLn-1));
        else if ((SgnBits.b4==0) && (SgnBits.w5!=0))   y.word=(1<<(FFTWLn-1))-1;
    }
    else
    {
        y.word = x.FFTBFWL;
        SgnBits.w6 = (y.word>>(FFTWLn-1)) & 63;
        if ((SgnBits.b5==1) && (SgnBits.w6!=-1))   y.word=-(1<<(FFTWLn-1));
        else if ((SgnBits.b5==0) && (SgnBits.w6!=0))   y.word=(1<<(FFTWLn-1))-1;
    }
    return(y.FFTWL);
}
// ================================================================================================
// FFTROMTF
// ------------------------------------------------------------------------------------------------
// Usage: 
// ================================================================================================
static void FFTROMTF(int idx, wiWORD *TFr, wiWORD *TFi)
{
    int i;

    for (i=0; i<3; i++)
    {
        TFr[i].word = (int)(cos(2.0*3.1415926536*((double)((i+1)*idx))/64)*(1<<FFTTFFBn)+0.5);
        TFi[i].word = (int)(-sin(2.0*3.1415926536*((double)((i+1)*idx))/64)*(1<<FFTTFFBn)+0.5);;
    }
}
//   end of fixFFT

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

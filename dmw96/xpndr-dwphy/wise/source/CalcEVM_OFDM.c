//--< CalcEVM_OFDM.c >-----------------------------------------------------------------------------
//=================================================================================================
//
//  Floating-point OFDM Transmitter and Receiver with EVM Calculator
//  Copyright 2006-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "CalcEVM_OFDM.h"
#include "CalcEVM.h"
#include "wiMath.h"
#include "wiParse.h"
#include "wiUtil.h"

static CalcEVM_OFDM_TxState_t TX = {0};
static CalcEVM_OFDM_RxState_t RX = {0};
static wiBoolean TX_RefreshMemory = 1; // reallocate TX memory arrays

static const int subcarrier_type_a[64] = {  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                            0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                            1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 }; // 48-63

static const int subcarrier_type_n[64] = {  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                            1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                            0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                            1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 }; // 48-63


static const int datacarrier_map[48] = {-26,-25,-24,-23,-22,
                                        -20,-19,-18,-17,-16,-15,-14,-13,-12,-11,-10, -9, -8,
                                         -6, -5, -4, -3, -2, -1,
                                          1,  2,  3,  4,  5,  6,
                                          8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                         22, 23, 24, 25, 26}; //map 48 data subcarriers to 64

static const int PunctureA_12[] = {1};
static const int PunctureB_12[] = {1};

static const int PunctureA_23[] = {1,1,1,1,1,1};
static const int PunctureB_23[] = {1,0,1,0,1,0};

static const int PunctureA_34[] = {1,1,0,1,1,0,1,1,0};
static const int PunctureB_34[] = {1,0,1,1,0,1,1,0,1};

static const int PunctureA_56[] = {1,1,0,1,0};		  // Rate 5/6...Puncturer is rate 5/6
static const int PunctureB_56[] = {1,0,1,0,1};		      

static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg) < 0) return WI_ERROR_RETURNED;

//=================================================================================================
//--  MISCELLANEOUS MACROS
//=================================================================================================
#ifndef SQR
#define SQR(x)   ((x)*(x))
#endif

#ifndef log10
#define log10(x) (log(x) / log(10.0))
#endif

//============================================================================
//  Function  : CalcEVM_OFDM_TX_IDFT                                          
//----------------------------------------------------------------------------
//  Purpose   : Compute the 64-point IDFT using a mapping where the input     
//              sequence is for a wiReal-sided spectrum, e.g., the frequency  
//              points are, in order -32,...,-1,0,1,...,31.                   
//  Parameters: X -- Input                                                    
//              y -- Output                                                   
//============================================================================
wiStatus CalcEVM_OFDM_TX_IDFT(wiComplex X[], wiComplex y[])
{
    int n, k, i;
    wiComplex W[64], Z[64];

    // -----------------------
    // Compute the multipliers
    // -----------------------
    for (k=0; k<32; k++)
    {
        W[k].Real = cos(2.0 * pi * (wiReal)k/64);
        W[k].Imag = sin(2.0 * pi * (wiReal)k/64);

        W[k+32].Real = -W[k].Real;
        W[k+32].Imag = -W[k].Imag;
    }
    // ------------------------------------------
    // Permute X into standard form (0...63) in Z
    // ------------------------------------------
    for (k=0; k<64; k++)
    {
        n = (k + 32) % 64;
        Z[k].Real = X[n].Real;
        Z[k].Imag = X[n].Imag;
    }
    // ---------------------------
    // Implement the 64-point IDFT
    // ---------------------------
    for (n=0; n<64; n++)
    {
        y[n].Real = y[n].Imag = 0.0;

        for (k=0; k<64; k++)
        {
            i = (k * n) % 64;
            y[n].Real += (Z[k].Real * W[i].Real - Z[k].Imag * W[i].Imag);
            y[n].Imag += (Z[k].Real * W[i].Imag + Z[k].Imag * W[i].Real);
        }
        y[n].Real = y[n].Real / 64.0;
        y[n].Imag = y[n].Imag / 64.0;
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_IDFT()


//============================================================================
//  Function  : CalcEVM_OFDM_TX_InsertSignal                                  
//----------------------------------------------------------------------------
//  Purpose   : Insert a signal segement into the baseband signal after       
//              applying a time-domain window.                                
//  Parameters: x  -- Sample-time baseband signal                             
//              y  -- Signal to be added to x                                 
//              Ny -- Number of points in y                                   
//============================================================================
static int CalcEVM_OFDM_TX_InsertSignal(wiComplex *x, wiComplex *y, int Ny)
{
    int k;

    x[0].Real += 0.5 * y[0].Real;
    x[0].Imag += 0.5 * y[0].Imag;

    for (k=1; k<Ny; k++)
    {
        x[k].Real += y[k].Real;
        x[k].Imag += y[k].Imag;
    }
    x[Ny].Real += 0.5 * y[Ny-64].Real;
    x[Ny].Imag += 0.5 * y[Ny-64].Imag;

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_InsertSignal()


//============================================================================
//  Function  : Insert_OFDM_Symbol                                            
//----------------------------------------------------------------------------
//  Purpose   : Create an OFDM symbol from the discrete Fourier sequence. A   
//              gaurd interval is prepended and the symbol is inserted into   
//              the time domain sequence.                                     
//  Parameters: d -- OFDM symbol (48 point complex sequence)                  
//              x -- Sampled-time sequence into which the symbol is added     
//============================================================================
static int Insert_OFDM_Symbol(wiComplex *d, wiComplex *x)
{
    wiComplex S[64], s[81];
    int k;

    // --------------------------
    // Clear the frequency series
    // --------------------------
    for (k=0; k<64; k++)
        S[k].Real = S[k].Imag = 0.0;

    // -----------------------------------------------
    // Map the 48 modulated points onto 64 subcarriers
    // -----------------------------------------------
    for (k=0; k<48; k++)
    {
        S[datacarrier_map[k]+32].Real = d[k].Real;
        S[datacarrier_map[k]+32].Imag = d[k].Imag;
    }
    // ----------------------
    // Insert the pilot tones
    // ----------------------
    {
        int *X = &(TX.Pilot_Shift_Register);
        int x; wiReal p;

        x = ((*X>>6) ^ (*X>>3)) & 1;   // P(X) = X^7 + X^4 + 1;
        *X = (*X << 1) | x;

        p = x? -1.0:1.0;

        k = -21+32; S[k].Real = p; S[k].Imag = 0.0;
        k =  -7+32; S[k].Real = p; S[k].Imag = 0.0;
        k =   7+32; S[k].Real = p; S[k].Imag = 0.0;
        k =  21+32; S[k].Real =-p; S[k].Imag = 0.0;
    }
    // -------------------------------------------------
    // Convert the OFDM symbol into a time-domain signal
    // -------------------------------------------------
    CalcEVM_OFDM_TX_IDFT(S, s+16);

    // ------------------------------------------
    // Add the guard interval by cyclic extension
    // ------------------------------------------
    for (k=0; k<16; k++)
    {
        s[k].Real = s[k+64].Real;
        s[k].Imag = s[k+64].Imag;
    }
    // ----------------------------------------------------
    // Insert the symbol to the baseband time-domain signal
    // ----------------------------------------------------
    CalcEVM_OFDM_TX_InsertSignal(x, s, 80);

    return WI_SUCCESS;
}
// end of Insert_OFDM_Symbol

//===========================================================================
//  Function  : CalcEVM_OFDM_TX_FreeMemory()                                
//----------------------------------------------------------------------------
//  Purpose   : Free memory for the signal arrays                         
//  Parameters: none                                                          
//===========================================================================
static wiStatus CalcEVM_OFDM_TX_FreeMemory(void)
{
    WIFREE( TX.a );  TX.Na = 0;
    WIFREE( TX.b );  TX.Nb = 0;
    WIFREE( TX.c );  TX.Nc = 0;
    WIFREE( TX.p );  TX.Np = 0;
    WIFREE( TX.d );  TX.Nd = 0;
    WIFREE( TX.x );  TX.Nx = 0;
    WIFREE( TX.s );  TX.Ns = 0;	
    
    TX_RefreshMemory = 1;

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_FreeMemory()

//===========================================================================
//  Function  : CalcEVM_OFDM_TX_AllocateMemory()                                
//---------------------------------------------------------------------------
//  Purpose   : Allocate memory for the signal arrays                         
//  Parameters: none                                                          
//===========================================================================
static wiStatus CalcEVM_OFDM_TX_AllocateMemory(void)
{
    XSTATUS(CalcEVM_OFDM_TX_FreeMemory());

    TX.Na = TX.N_SYM * TX.N_DBPS;
    TX.Nb = TX.Na;
    TX.Nc = TX.N_SYM * TX.N_CBPS;
    TX.Np = TX.Nc;
    TX.Nd = TX.N_SYM * 48;
    TX.Nx = (TX.N_SYM * 80) + 400 + 2 + TX.PadFront + TX.PadBack;
    TX.Ns = TX.M * (TX.Nx + 10);

    WICALLOC( TX.a, TX.Na, int );
    WICALLOC( TX.b, TX.Nb, int );
    WICALLOC( TX.c, TX.Nc, int );
    WICALLOC( TX.p, TX.Np, int );
    WICALLOC( TX.d, TX.Nd, wiComplex );
    WICALLOC( TX.x, TX.Nx, wiComplex );
    WICALLOC( TX.s, TX.Ns, wiComplex );

    TX_RefreshMemory = 0;

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_AllocateMemory(()

/*===========================================================================+
|  Function  : CalcEVM_OFDM_TX_PHY_TXSTART()                                 |
|----------------------------------------------------------------------------|
|  Purpose   : Signal the start of transmission.                             |
|  Parameters: none                                                          |
+===========================================================================*/
static wiStatus CalcEVM_OFDM_TX_PHY_TXSTART(void)
{
    if (!TX.RATE) return STATUS(WI_ERROR);
    TX.Pilot_Shift_Register=0x7F;
    if (!TX.Scrambler_Shift_Register)
            TX.Scrambler_Shift_Register = 0x7F;

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_PHY_TXSTART()

/*===========================================================================+
|  Function  : CalcEVM_OFDM_TX_Scrambler()                                   |
|----------------------------------------------------------------------------|
|  Purpose   : Scramble the input sequence using PRBS x^7+x^4+1.             |
|  Parameters: n -- Number of bits to scramble                               |
|              a -- Input array                                              |
|              b -- Output array                                             |
|              X -- Scrambler state                                          |
+===========================================================================*/
wiStatus CalcEVM_OFDM_TX_Scrambler(int n, int a[], int b[])
{
    if (TX.Scrambler_Disabled)
    {
        memcpy(b, a, n*sizeof(int));
    }
    else
    {
        int k, x;
        int X = TX.Scrambler_Shift_Register;

        for (k=0; k<n; k++)
        {
            x = ((X>>6) ^ (X>>3)) & 1;   // P(X) = X^7 + X^4 + 1;
            X = (X << 1) | x;

            b[k] = a[k] ^ x;
        }
        TX.Scrambler_Shift_Register = X;
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_Scrambler()

/*===========================================================================+
|  Function  : CalcEVM_OFDM_TX_Encoder()                                     |
|----------------------------------------------------------------------------|
|  Purpose   : Implement a rate 1/2 convolutional encoder.                   |
|  Parameters: Nbits -- Number of bits in x                                  |
|              x     -- Input array                                          |
|              y     -- Output array (must be 2*Nbits long)                  |
|              m     -- CodeRate * 12...defines the puncturing pattern       |
+===========================================================================*/
wiStatus CalcEVM_OFDM_TX_Encoder(int Nbits, int x[], int y[], int m)
{
    int i, M, X = 0;
    const int *pA, *pB;

    // ------------
    // Uncoded Case
    // ------------
    if (m==12)
    {
        memcpy(y, x, Nbits*sizeof(int));
        return WI_SUCCESS;
    }
    // -----------------------------
    // Select the puncturing pattern
    // -----------------------------
    switch (m)
    {
        case  6: pA = PunctureA_12;  pB = PunctureB_12;  M = 1;  break;
        case  8: pA = PunctureA_23;  pB = PunctureB_23;  M = 6;  break;
        case  9: pA = PunctureA_34;  pB = PunctureB_34;  M = 9;  break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // -------------------------------
    // Apply the convolutional encoder
    // -------------------------------
    for (i=0; i<Nbits; i++)
    {
        // ----------------------------------------------------------------
        // Clock the shift register, current input goes into the LSB of 'X'
        // ----------------------------------------------------------------
        X = (X << 1) | x[i];

        // -----------------------------------------------------------------
        // Output two bits from octal polynomials 0133 and 0171
        // The bits are output only if the puncturing pattern contains a '1'
        // -----------------------------------------------------------------
        if (pA[i%M]) *(y++) = ((X>>0) ^ (X>>2) ^ (X>>3) ^ (X>>5) ^ (X>>6)) & 1;
        if (pB[i%M]) *(y++) = ((X>>0) ^ (X>>1) ^ (X>>2) ^ (X>>3) ^ (X>>6)) & 1;
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_Encoder()

/*===========================================================================+
|  Function  : CalcEVM_OFDM_TX_Interleave()                                  |
|----------------------------------------------------------------------------|
|  Purpose   : Implement the interleaver of 802.11 section 17.3.5.6.         |
|  Parameters: N_CBPS -- Number of code bits per OFDM symbol                 |
|              N_BPSC -- Number of bits per subcarrier                       |
|              x      -- Input array                                         |
|              y      -- Output array                                        |
+===========================================================================*/
wiStatus CalcEVM_OFDM_TX_Interleave(int N_CBPS, int N_BPSC, int x[], int y[])
{
    if (TX.Interleaver_Disabled)
    {
        memcpy(y, x, N_CBPS*sizeof(int));
    }
    else
    {
        int i, j, k;
        int s = max(N_BPSC/2, 1);

        for (k=0; k<N_CBPS; k++)
        {
            i = (N_CBPS/16) * (k%16) + (k/16);
            j = s * (i/s) + ((i + N_CBPS - (16 * i/N_CBPS)) % s);
            y[j] = x[k];
        }
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_Interleave()

/*===========================================================================+
|  Function  : CalcEVM_OFDM_TX_Modulation()                                  |
|----------------------------------------------------------------------------|
|  Purpose   : Modulate 48 subcarriers in Y[] using the coded bits in x[]    |
|              and the modulation scheme specified by the number of bits to  |
|              be mapped to each subcarrier.                                 |
|  Parameters: Y      -- Frequency domain symbol sequence                    |
|              x      -- Input array (coded bits)                            |
|              N_BPSC -- Number of coded bits per subcarrier.                |
+===========================================================================*/
wiStatus CalcEVM_OFDM_TX_Modulation(wiComplex Y[], int x[], int N_BPSC)
{
    int bI, bQ, k;
    wiReal a=0;

    // --------------------------------------------
    // Get constellation scaling for constant power
    // --------------------------------------------
    if (TX.aModemRX)
    {
        switch (N_BPSC)
        {
            case  1: a = 13.0/13.0; break;
            case  2: a =  9.0/13.0; break;
            case  4: a =  4.0/13.0; break;
            case  6: a =  2.0/13.0; break;
            case  8: a =  1.0/13.0; break;
        }
    }
    else
    {
        switch (N_BPSC)
        {
            case  1: a = 1.0;               break;
            case  2: a = 1.0 / sqrt(  2.0); break;
            case  4: a = 1.0 / sqrt( 10.0); break;
            case  6: a = 1.0 / sqrt( 42.0); break;
            case  8: a = 1.0 / sqrt(170.0); break;
            case 10: a = 1.0 / sqrt(682.0); break;
        }
    }
    // ----------------------
    // Clear the output array
    // ----------------------
    for (k=0; k<48; k++)
        Y[k].Real = Y[k].Imag = 0.0;

    switch (N_BPSC)
    {           // ----
        case 1:  // BPSK
                    // ----
            for (k=0; k<48; k++)
            {
                Y[k].Real = x[k]? a:-a;
            }
            break;
                    // ----
        case 2:  // QPSK
                    // ----
            for (k=0; k<48; k++)
            {
                Y[k].Real = x[2*k  ]? a:-a;
                Y[k].Imag = x[2*k+1]? a:-a;

            }
            break;
                    // ------
        case 4:  // 16-QAM
                    // ------
            for (k=0; k<48; k++)
            {
                Y[k].Real = (x[4*k+0]? 1.0:-1.0) * (x[4*k+1]? a:3.0*a);
                Y[k].Imag = (x[4*k+2]? 1.0:-1.0) * (x[4*k+3]? a:3.0*a);
            }
            break;
                    // ------
        case 6:  // 64-QAM
        {        // ------
            const wiReal c[8] = {-7,-5,-1,-3, 7, 5, 1, 3};

            for (k=0; k<48; k++)
            {
                bI = (x[6*k+0]<<2) | (x[6*k+1]<<1) | (x[6*k+2]<<0);
                bQ = (x[6*k+3]<<2) | (x[6*k+4]<<1) | (x[6*k+5]<<0);

                Y[k].Real = c[bI] * a;
                Y[k].Imag = c[bQ] * a;
            }
        }  break;
                    // -------
        case 8:  // 256-QAM
        {        // -------
            const wiReal c[16] = {-15,-13,-9,-11,-1,-3,-7,-5,15,13,9,11,1,3,7,5};

            for (k=0; k<48; k++)
            {
                bI = (x[8*k+0]<<3) | (x[8*k+1]<<2) | (x[8*k+2]<<1) | (x[8*k+3]<<0);
                bQ = (x[8*k+4]<<3) | (x[8*k+5]<<2) | (x[8*k+6]<<1) | (x[8*k+7]<<0);

                Y[k].Real = c[bI] * a;
                Y[k].Imag = c[bQ] * a;
            }
        }  break;
                    // --------
        case 10: // 1024-QAM
        {        // --------
            const wiReal c[32] = {-31,-29,-25,-27,-17,-19,-23,-21,-1,-3,-7,-5,-15,-13,-9,-11,
                                31, 29, 25, 27, 17, 19, 23, 21, 1, 3, 7, 5, 15, 13, 9, 11};

            for (k=0; k<48; k++)
            {
                bI = (x[10*k+0]<<4) | (x[10*k+1]<<3) | (x[10*k+2]<<2) | (x[10*k+3]<<1) | (x[10*k+4]<<0);
                bQ = (x[10*k+5]<<4) | (x[10*k+6]<<3) | (x[10*k+7]<<2) | (x[10*k+8]<<1) | (x[10*k+9]<<0);

                Y[k].Real = c[bI] * a;
                Y[k].Imag = c[bQ] * a;
            }
        }  break;

        default:
            wiPrintf("\n*** ERROR: Invalid N_BPSC in CalcEVM_OFDM_TX_Modulation()\n\n");
            return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_Modulation()


// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_UpsamFIR()
// ------------------------------------------------------------------------------------------------
// Purpose   : FIR filter implementation used for upsampling from 20M to 80M
// Parameters: Nx -- Length of array x (model input)
//             x  -- Input data array,
//             Ny -- Length of array y (model input)
//             y  -- Output data array, 
// ================================================================================================
wiStatus CalcEVM_OFDM_UpsamFIR(int Nx, wiComplex x[], int Ny, wiComplex y[])
{   
    const int b[][12] = { { 0,  0,  0,  0,  0,  0, 64,  0,  0,  0,  0,  0},
                          { 0,  1, -2,  4, -7, 19, 57,-11,  5, -3,  1, -1},
                          {-1,  2, -3,  6,-12, 40, 40,-12,  6, -3,  2, -1},
                          {-1,  1, -3,  5,-11, 57, 19, -7,  4, -2,  1,  0} };
    
    wiComplex v[4], z[12];
    int i, j, k, m;
    
    for (j=0; j<12; j++)
    {
        z[j].Real=0.0;
        z[j].Imag=0.0;
    }
    
    for (i=0; i<Ny; i++)
    {  
        if (i%4 == 0)
        {
            for (j=11; j>0; j--)
            {
                z[j].Real = z[j-1].Real;
                z[j].Imag = z[j-1].Imag;
            }

            j = i/4;
            z[0].Real=(j<Nx)?x[j].Real:0.0;
            z[0].Imag=(j<Nx)?x[j].Imag:0.0;
            
            for (m=0; m<4; m++)
            {
                v[m].Real = v[m].Imag = 0.0;
                for (k=0; k<12; k++)
                {
                    v[m].Real+=z[k].Real*b[m][k];
                    v[m].Imag+=z[k].Imag*b[m][k];
                }
            }
    
        } // end if

        y[i].Real = v[i%4].Real/sqrt(52.0);  //revised 06-11-25 add sqrt(52) scaling to get unit Prms;
        y[i].Imag = v[i%4].Imag/sqrt(52.0);  
    }
    return WI_SUCCESS;         
}
// end of CalcEVM_OFDM_UpsamFIR()


// ===========================================================================
// Function  : CalcEVM_OFDM_TX_PHY()                                          
// ---------------------------------------------------------------------------
// Purpose   : Implement the 802.11a physical layer transmitter.           
// Parameters: none
//============================================================================
wiStatus CalcEVM_OFDM_TX_PHY(void)
{
    int i, k, m, n, N, ofs;
    int dx;
    
    // --------------------------------------------------------
    // Signal the TX components that a transmission is starting
    // --------------------------------------------------------
    XSTATUS(CalcEVM_OFDM_TX_PHY_TXSTART());

    //---------------------------------------------------------
    // Number of zeros padded in front of packets
    //---------------------------------------------------------
    dx = TX.PadFront;

    // ---------------------------------------------------------------------
    // Compute the number of bits in the DATA field. The data field contains
    // 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits, and enough pad bits
    // to fill a whole number of OFDM symbols, each containing N_DBPS bits.
    // ---------------------------------------------------------------------
    TX.N_SYM = (16 + 8*TX.LENGTH + 6 + (TX.N_DBPS-1)) / TX.N_DBPS;
    N        = TX.N_DBPS * TX.N_SYM;

    // ------------------------------------------------------------------
    // Compute the convolutioanl encoder rate x 12, e.g., CodeRate = m/12
    // Also compute the number of code bits per symbol
    // ------------------------------------------------------------------
    m = TX.N_DBPS / (4 * TX.N_BPSC);
    TX.N_CBPS = TX.N_DBPS * 12 / m;

    // --------------------------------------------------------------
    // Allocate memory if necessary; otherwise clear the output array
    // --------------------------------------------------------------
    if (TX_RefreshMemory)
        CalcEVM_OFDM_TX_AllocateMemory();
    else
        memset(TX.x, 0, TX.Nx*sizeof(wiComplex));

    // ----------------------------
    // Insert the "Short Sequences"
    // ----------------------------
    {
        wiComplex Y[64], y[160];
        wiReal  a = sqrt(13.0/6.0);
        int S[] = { 0,0,0,0,0,0,0,0,1,0,0,0,-1,0,0,0,1,0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,0,
                    0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0 };

        for (k=0; k<64; k++)
            Y[k].Real = Y[k].Imag = a * S[k];

        CalcEVM_OFDM_TX_IDFT(Y, y+96);

        for (k=0; k<96; k++)
        {
            y[k].Real = y[k%64+96].Real;
            y[k].Imag = y[k%64+96].Imag;
        }
        CalcEVM_OFDM_TX_InsertSignal(TX.x+dx, y, 160);
    }
    // ---------------------------
    // Insert the "Long Sequences"
    // ---------------------------
    {
        wiComplex Y[64], y[160];
        int L[] = { 0,0,0,0,0,0,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,0,
                    1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,0,0,0,0,0 };

        for (k=0; k<64; k++) 
        {
            Y[k].Real = (wiReal)L[k];
            Y[k].Imag = 0.0;
        }
        CalcEVM_OFDM_TX_IDFT(Y, y+96);

        for (k=0; k<96; k++)
        {
            y[k].Real = y[(k+32)%64+96].Real;
            y[k].Imag = y[(k+32)%64+96].Imag;
        }
        CalcEVM_OFDM_TX_InsertSignal(TX.x+dx+160, y, 160);
    }
    // ---------------------------------------
    // Generate and Insert the "SIGNAL" Symbol
    // ---------------------------------------
    {
        int b[24], c[48], p[48];

        // Create the bit field
        // --------------------
        for (i=0; i<4; i++)
            b[i] = (TX.RATE >> (3-i)) & 1;// RATE

        b[4] = 0;                        // Reserved (0)

        for (i=0; i<12; i++)              // LENGTH
            b[5+i] = (TX.LENGTH>>i) & 1;

        b[17] = 0;                       // Parity
        for (i=0; i<=16; i++)             // Even parity by summing mod 2 the
            b[17] = b[17] ^ b[i];         // ...previous bits

        for (i=18; i<24; i++)             // SIGNAL TAIL
            b[i] = 0;

        CalcEVM_OFDM_TX_Encoder   (24, b, c, 6);// Rate 1/2 convolutional code
        CalcEVM_OFDM_TX_Interleave(48, 1, c, p);// Interleaver
        CalcEVM_OFDM_TX_Modulation(TX.dSERVICE, p, 1);    // BPSK subcarrier modulalation
        Insert_OFDM_Symbol(TX.dSERVICE, TX.x+dx+320);       // Insert the symbol
    }
    // --------------------------------
    // Create the DATA field bit stream
    // --------------------------------
    for (n=0; n<16; n++)
        TX.a[n] = (TX.SERVICE >> n) & 1;      // SERVICE

    for (i=0; i<8*TX.LENGTH; i++, n++)
        TX.a[n] = (TX.PSDU[i/8] >> (i%8)) & 1;// PSDU

    for (i=0; i<6; i++, n++)
        TX.a[n] = 0;                          // Tail

    for (; n<N; n++)
        TX.a[n] = 0;                          // Pad bits

    // -----------------
    // Scramble the data
    // -----------------
    CalcEVM_OFDM_TX_Scrambler(N, TX.a, TX.b);

    // ---------------------------------------------------------------------
    // Reset the tail bits to zero. This is required because these bits will
    // be used to terminate the trellis when decoding the convolutional code
    // ---------------------------------------------------------------------
    for (i=0; i<6; i++)
        TX.b[16+8*TX.LENGTH+i] = 0;

    // -------------------------------
    // Apply the convolutional encoder
    // -------------------------------
    CalcEVM_OFDM_TX_Encoder(N, TX.b, TX.c, m);

    // ----------------------------------
    // Create the DATA field OFDM symbols
    // ----------------------------------
    for (i=0; i<TX.N_SYM; i++)
    {
        ofs = i*TX.N_CBPS;
        CalcEVM_OFDM_TX_Interleave(TX.N_CBPS, TX.N_BPSC, TX.c+ofs, TX.p+ofs);
        CalcEVM_OFDM_TX_Modulation(TX.d+48*i, TX.p+ofs, TX.N_BPSC);
        Insert_OFDM_Symbol(TX.d+48*i, TX.x+dx+80*i+400);
    }
    // -------------------
    // Upsampling
    // -------------------
    XSTATUS( CalcEVM_OFDM_UpsamFIR(TX.Nx, TX.x, TX.Ns, TX.s) );
     
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX_PHY()


// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level PHY routine
// Parameters: bps      -- PHY transmission rate (bits/second)
//             Length   -- Number of bytes in "data"
//             data     -- Data to be transmitted
//             Ns       -- Returned number of transmit samples
//             S        -- Array of transmit signals (by ref), only *S[0] is used
//             Fs       -- Sample rate for S (Hz)
//             Prms     -- Complex power in S (rms)
// ================================================================================================
wiStatus CalcEVM_OFDM_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms)
{
    int i;
    wiUWORD RATE={0}, LENGTH={0};
    static wiComplex *sTX[2];

    if (bps)
    {
        // -------------------------------------------------
        // Generate the Bermai PHY control block (MAC Level)
        // -------------------------------------------------
        if (InvalidRange(Length,1,4095)) return STATUS(WI_ERROR_PARAMETER2);
        LENGTH.w12 = (unsigned int)Length;
        switch ((unsigned)bps)
        {
            case  6000000: RATE.word = 0xD; break;
            case  9000000: RATE.word = 0xF; break;
            case 12000000: RATE.word = 0x5; break;
            case 18000000: RATE.word = 0x7; break;
            case 24000000: RATE.word = 0x9; break;
            case 36000000: RATE.word = 0xB; break;
            case 48000000: RATE.word = 0x1; break;
            case 54000000: RATE.word = 0x3; break;
            case 72000000: RATE.word = 0xA; break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    
        for (i=0; i<Length; i++)  TX.PSDU[i] = (unsigned char)data[i].w8;
    }
    else
    {
        // -------------------------------
        // Extract RATE from Bermai header
        // -------------------------------
        if (InvalidRange(Length,1,4095)) return STATUS(WI_ERROR_PARAMETER2);
        Length = (data[2].w4 << 8) | data[3].w8;
        for (i=8; i<(Length+8); i++) TX.PSDU[i-8] = (unsigned char)data[i].w8;
    }
    // -----------------------------
    // Load Parameters into TX State
    // -----------------------------
    TX_RefreshMemory = 1; // always refresh (for now)
    TX.RATE = RATE.word;
    switch (TX.RATE)
    {
        case 0xD: TX.N_BPSC = 1; TX.N_DBPS =  24; break;
        case 0xF: TX.N_BPSC = 1; TX.N_DBPS =  36; break;
        case 0x5: TX.N_BPSC = 2; TX.N_DBPS =  48; break;
        case 0x7: TX.N_BPSC = 2; TX.N_DBPS =  72; break;
        case 0x9: TX.N_BPSC = 4; TX.N_DBPS =  96; break;
        case 0xB: TX.N_BPSC = 4; TX.N_DBPS = 144; break;
        case 0x1: TX.N_BPSC = 6; TX.N_DBPS = 192; break;
        case 0x3: TX.N_BPSC = 6; TX.N_DBPS = 216; break;
        case 0xA: TX.N_BPSC = 8; TX.N_DBPS = 288; break;
        default: XSTATUS(WI_ERROR_RANGE);
    }
    TX.LENGTH = Length;

    // ----------------------
    // Call modem TX function
    // ----------------------
    XSTATUS( CalcEVM_OFDM_TX_PHY() );

    // -----------------
    // Return Parameters
    // -----------------
    {
        sTX[0] = TX.s;
        sTX[1] = 0;
        if (S)  *S = sTX;
        if (Ns) *Ns = TX.Ns;
        if (Fs) *Fs=80.0e6;
    }
  // if (Prms) *Prms = 52.0/64.0/64.0;
    if (Prms)  *Prms=1.0;  //revised 06-11-25 to ease receiver sigdet threshold setting;
        
    //-------------------------------------------------------------
    //dump the transmitter output signal;
    //-------------------------------------------------------------   
    
    if (TX.dumpTX)
    {
        WIFREE( TX.trace_t );

        TX.trace_Nt=TX.Ns;
        WICALLOC( TX.trace_t, TX.trace_Nt, wiComplex );

        for (i=0; i<TX.trace_Nt; i++)
        {
            TX.trace_t[i].Real = sTX[0][i].Real;
            TX.trace_t[i].Imag = sTX[0][i].Imag;
        }
    }
    if (TX.dumpTX) dump_wiComplex("t.out", TX.trace_t, TX.trace_Nt);
    TX.Pilot_Shift_Register=0x7F;
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_TX()


// ================================================================================================
// FUNCTION : CalcEVM_OFDM_RXWriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus CalcEVM_OFDM_TXWriteConfig(wiMessageFn MsgFunc)
{
    MsgFunc(" TX: PRBS              = 0x%02X \n", TX.Scrambler_Shift_Register);
    MsgFunc(" TX: Pad Front,Back    : PadFront = %5d, PadBack = %3d \n",TX.PadFront, TX.PadBack);
    
    return WI_SUCCESS;
}

// ================================================================================================
//  Function  : CalcEVM_OFDM_DCX()                                                   
// ------------------------------------------------------------------------------------------------
//  Purpose   : Apply a DCX model to the received signal; only act as HPF, not as Acc; 
//                Operating at 80MHz, the transfer function is changed accordingly from 40MHz                                                
//  Parameters: x         -- DCX model input signal (output replace input)      
//                Nx      -- Length of the input signal 
// ================================================================================================
wiStatus CalcEVM_OFDM_DCX(wiComplex *x, int Nx)
{        
    wiComplex z[3] = {{0}}; // delay model; 0 for input (z_k) and 1,2 for delayed (z_{k-1}, z_{k-2})
    wiComplex b, c;   
    int i;

    if (!x) return STATUS(WI_ERROR_PARAMETER1);

    for (i=0; i<Nx; i++) 
    {
        z[2].Real=z[1].Real;
        z[2].Imag=z[1].Imag;
        z[1].Real=z[0].Real;
        z[1].Imag=z[0].Imag;
        
        b.Real=z[2].Real*pow(2.0, -(RX.DCXShift+2.0));
        b.Imag=z[2].Imag*pow(2.0, -(RX.DCXShift+2.0));
        c.Real=x[i].Real-b.Real;
        c.Imag=x[i].Imag-b.Imag;
        z[0].Real=z[2].Real+c.Real;
        z[0].Imag=z[2].Imag+c.Imag;
  
        x[i].Real=RX.DCXSelect? c.Real: x[i].Real;
        x[i].Imag=RX.DCXSelect? c.Imag: x[i].Imag;
    }
    return WI_SUCCESS;
} 
// end of CalcEVM_OFDM_DCX

// ================================================================================================
//  Function  : CalcEVM_OFDM_Downmix()                                                   
// ------------------------------------------------------------------------------------------------
//  Purpose   : Downshift a +10 MHz IF signal
//  Parameters: x   -- Input/Output array (output replaces input)      
//              Nx  -- Length of the input signal 
// ================================================================================================
wiStatus CalcEVM_OFDM_Downmix(wiComplex *x, int Nx)
{        
    wiComplex y;
    wiComplex w[8];
    int j, k;

    if (!x) return STATUS(WI_ERROR_PARAMETER1);

    // Build the mixer look-up table
    //
    for (k=0; k<8; k++)
    {
        w[k].Real = cos(-2.0 * 3.14159265359 * (double)k / 8.0);
        w[k].Imag = sin(-2.0 * 3.14159265359 * (double)k / 8.0);
    }
    
    // Downmix
    //
    for (k=0; k<Nx; k++) 
    {
        j = k%8;
        y.Real = (w[j].Real * x[k].Real) - (w[j].Imag * x[k].Imag);
        y.Imag = (w[j].Real * x[k].Imag) + (w[j].Imag * x[k].Real);
        x[k].Real = y.Real;
        x[k].Imag = y.Imag;
    }
    return WI_SUCCESS;
} 
// end of CalcEVM_OFDM_Downmix

// ================================================================================================
//  Function  : CalcEVM_OFDM_DownsamFIR()
// ------------------------------------------------------------------------------------------------
//  Purpose   : channel selection and antialiasing filter
//  Parameters: x       -- DownsamFIR model input signal (output replace input), 80 MHz   
//              Nx      -- Length of the input signal 
// ================================================================================================
wiStatus CalcEVM_OFDM_DownsamFIR(wiComplex *x, int Nx)
{        
    const int N_LPF = 61;
    const double b[]={-0.002150, -0.016279,  0.003041,  0.004113,  0.006649,  0.005551,  0.000839, 
                      -0.005340, -0.009258, -0.007907, -0.001146,  0.007692,  0.013195,  0.011099,  
                       0.001315, -0.011326, -0.019094, -0.015891, -0.001426,  0.017364,  0.029147,  
                       0.024354,  0.001512, -0.029722, -0.051415, -0.045144, -0.001560,  0.073340,  
                       0.158603,  0.225993,  0.251566,  0.225993,  0.158603,  0.073340, -0.001560, 
                      -0.045144, -0.051415, -0.029722,  0.001512,  0.024354,  0.029147,  0.017364, 
                      -0.001426, -0.015891, -0.019094, -0.011326,  0.001315,  0.011099,  0.013195,  
                       0.007692, -0.001146, -0.007907, -0.009258, -0.005340,  0.000839,  0.005551,  
                       0.006649,  0.004113,  0.003041, -0.016279, -0.002150 };
    int i, k;
    wiComplex *y;

    if (!x) return STATUS(WI_ERROR_PARAMETER1);

    WICALLOC( y, Nx, wiComplex );

    for (i=0; i<Nx; i++)
    { 
        y[i].Real = b[0] * x[i].Real;
        y[i].Imag = b[0] * x[i].Imag;

        for (k=1; (k<N_LPF) && (i>=k); k++)
        {
            y[i].Real += b[k] * x[i-k].Real;
            y[i].Imag += b[k] * x[i-k].Imag;
        }
    }
    
    for (i=0; i<Nx; i++)
    {      
        x[i].Real = y[i].Real;  
        x[i].Imag = y[i].Imag;
    }
    WIFREE(y);
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_DownsamFIR


// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_SigDet  
// ------------------------------------------------------------------------------------------------
// Purpose   : Detect the presence of a signal in the data channel
// Parameters: 
//             x            -- Signal input
//             Nx            -- Number of Samples in Signal input
//             SGSigDet      -- Signal detected point
// ================================================================================================
wiStatus CalcEVM_OFDM_SigDet(wiComplex *x, int Nx, int *SGSigDet)
{
    wiComplex AcmAx={0}; //autocorrelation accumulator;
    wiReal AcmPwr=0;      //Power management accumulator;
    wiComplex w[3]={{0}};  //input register; 
    wiComplex *fx;       //filtered signal;
    
    wiComplex ax1={0}, ax2={0}; //per-sample auto correlation output
    wiReal pw1=0, pw2=0;          //per-sample power calculation;
    wiReal S;                   //signal detect (autocorrelation) signal;
    int i;
       
    if (!x) return STATUS(WI_ERROR_PARAMETER1);

    if (RX.SigDetFilter)
    {
        WICALLOC( fx, Nx, wiComplex );

        for (i=0; i<Nx; i++)
        {
            w[0]=x[i];
            fx[i].Real=(w[0].Real-w[2].Real)/2;
            fx[i].Imag=(w[0].Imag-w[2].Imag)/2;
            w[2].Real=w[1].Real;
            w[2].Imag=w[1].Imag;
            w[1].Real=w[0].Real;
            w[1].Imag=w[0].Imag;
        }
    }
    else
        fx=x;
    
    for (i=RX.SigDetDelay; i<Nx; i++)  //Start from certain point, usually larger than 80; For simulation, need to pad zeros in front of packet; 
    {         
        if (i>=16){
            ax1.Real=fx[i-16].Real*fx[i].Real+fx[i-16].Imag*fx[i].Imag;
            ax1.Imag=fx[i-16].Real*fx[i].Imag-fx[i-16].Imag*fx[i].Real;}
        
        if (i>=48){
            ax2.Real=fx[i-48].Real*fx[i-32].Real+fx[i-48].Imag*fx[i-32].Imag;
            ax2.Imag=fx[i-48].Real*fx[i-32].Imag-fx[i-48].Imag*fx[i-32].Real;         
            pw1=fx[i-48].Real*fx[i-48].Real+fx[i-48].Imag*fx[i-48].Imag;}

        if (i>=80)
            pw2=fx[i-80].Real*fx[i-80].Real+fx[i-80].Imag*fx[i-80].Imag;
        
        AcmAx.Real+=(ax1.Real-ax2.Real);
        AcmAx.Imag+=(ax1.Imag-ax2.Imag);

        AcmPwr+=(pw1-pw2)/32;

        S=(fabs(AcmAx.Real)+fabs(AcmAx.Imag))/32;
        
        if ((S>=(AcmPwr*RX.SigDetTh1/2)) &&(S>=RX.SigDetTh2f))
        {
            *SGSigDet=i; break;
        }
        
        if (i==Nx-1)  //claim a packet fault;
        {
            RX.Fault = 3;  break;
        }
    } 
    if (RX.SigDetFilter) WIFREE(fx);

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_SigDet


// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_FrameSync  
// ------------------------------------------------------------------------------------------------
// Purpose   : Synchronize the receiver with the OFDM symbol frame
// Parameters: 
//               FSPeak        -- Start the Peak finder (SGSigDet+FSDelay for this case)
//               x              -- Signal input to the FS model;   
//             FSOffset      -- Indicates Offset (begining of LPF with the position offset)
// ================================================================================================
wiStatus CalcEVM_OFDM_FrameSync(int FSPeak, wiComplex *x, int Nx, int *FSOffset)
{
    wiComplex AcmAx={0.0}; //autocorrelation accumulator;
    wiReal AcmPwr1=0.0, AcmPwr2=0.0; //Power management accumulator (current and delayed) ;
    wiComplex w[3]={{0}};  //input register; 
    wiComplex *fx;

    wiComplex ax1={0.0}, ax2={0.0}; //per-sample auto correlation output
    wiReal pw1=0.0, pw2=0.0, pw3=0.0, pw4=0.0; //per-sample power calculation;
    wiReal S;  //power of the autocorrelation;
    wiReal R; // ratio of the autocorrelation;
    wiReal Peak=0; //record the Peak ;
    wiBoolean Qualifier=0; // Check whether the peak satisfies the threshold condition;
    
    int i;
    unsigned count=0; //counter for latency requirement;
    
    if (RX.SyncFilter)
    {
        WICALLOC( fx, Nx, wiComplex );

        for (i=0; i<Nx; i++)
        {   
            w[0]=x[i];
            fx[i].Real=(w[0].Real-w[2].Real)/2;
            fx[i].Imag=(w[0].Imag-w[2].Imag)/2;
            w[2].Real=w[1].Real;
            w[2].Imag=w[1].Imag;
            w[1].Real=w[0].Real;
            w[1].Imag=w[0].Imag;
       }
    }
    else
        fx=x;
    
    for (i=0; i<Nx; i++) //accumulating from starting point
    {
        pw1=fx[i].Real*fx[i].Real+fx[i].Imag*fx[i].Imag;
        
        if (i>=16)
        {
            ax1.Real=fx[i-16].Real*fx[i].Real+fx[i-16].Imag*fx[i].Imag;
            ax1.Imag=fx[i-16].Real*fx[i].Imag-fx[i-16].Imag*fx[i].Real;
            pw3=fx[i-16].Real*fx[i-16].Real+fx[i-16].Imag*fx[i-16].Imag;
        }
        
        if (i>=144)
            pw2=fx[i-144].Real*fx[i-144].Real+fx[i-144].Imag*fx[i-144].Imag;
        
        if (i>=160) //144+16
        {
            ax2.Real=fx[i-160].Real*fx[i-144].Real+fx[i-160].Imag*fx[i-144].Imag;
            ax2.Imag=fx[i-160].Real*fx[i-144].Imag-fx[i-160].Imag*fx[i-144].Real;
            pw4=fx[i-160].Real*fx[i-160].Real+fx[i-160].Imag*fx[i-160].Imag;
        }   

        AcmAx.Real+=(ax1.Real-ax2.Real);
        AcmAx.Imag+=(ax1.Imag-ax2.Imag);

        S=AcmAx.Real*AcmAx.Real+AcmAx.Imag*AcmAx.Imag;
        AcmPwr1+=(pw1-pw2);
        AcmPwr2+=(pw3-pw4);
                            
        if (i>=FSPeak) //Peak finding starting point
        {
            R = S/(AcmPwr1*AcmPwr2);
                    
            if (R>Peak)
            {
                Peak=R; count=0; Qualifier=0;
            }
            
            if (R<RX.PKThreshold*Peak)  Qualifier=1;
        
            if ((count==RX.SyncPKDelay) && (Qualifier==1))
            {
                //Begining of LPF including the negtive offset; 1 sets offset as starting of LPF instead of end of SPF;
                *FSOffset=i-RX.SyncPKDelay+1+RX.SyncShift;  
                break;
            }   
            count += 1;
        }
        
        if (i >= Nx-240)   //claim a packet fault; 240: LPF+SIG, might also use other values 
        {
            RX.Fault = 2;
            break;
        }
    }
    if (RX.SyncFilter) WIFREE(fx);
    
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_FrameSync

 
// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_CFO
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate and correct the carrier frequency offset
// Parameters: x          -- Signal Input/Output 
//             CFOStart   -- Signal Detection Point (CFO starts SGSigDet+RX.DelayCFO;
//             CFOEnd     -- (FSoffset in FrameSync) (beginning of LPF-Frameoffset)
// ================================================================================================
wiStatus CalcEVM_OFDM_CFO(wiComplex *x, int Nx, int CFOStart, int CFOEnd)
{
    wiComplex ax={0.0};        // correlator output;
    wiComplex AcmAx={0.0}; //accumulator output; 
    wiReal   phi1=0.0,phi2=0.0;         
    wiComplex *y,*fx; //filtered input and intermediate signal;
    wiComplex w[2]={{0}}; //filter delay line;
    int   i;   
    
    WICALLOC( fx, Nx, wiComplex );
    WICALLOC(  y, Nx, wiComplex );

    for (i=0; i<CFOEnd+32+64*2; i++)
    {         
        y[i].Real=(i>=CFOEnd)?x[i].Real*cos(phi1*i)+x[i].Imag*sin(phi1*i):x[i].Real;  //apply CFO correction; The correction acturally starts from CFOEnd, phi1=0 before that;
        y[i].Imag=(i>=CFOEnd)?x[i].Imag*cos(phi1*i)-x[i].Real*sin(phi1*i):x[i].Imag;

            if (RX.CFOFilter)
            {      
                w[0].Real=y[i].Real;
                w[0].Imag=y[i].Imag;
                fx[i].Real=(w[0].Real-w[1].Real)/2;    //The filter is after the correction;
                fx[i].Imag=(w[0].Imag-w[1].Imag)/2;
                w[1].Real=w[0].Real;
                w[1].Imag=w[0].Imag;
            }
            else
                fx[i]=y[i];
        
        //   CFOStart=SGSigDet+RX.DelayCFO;
        if ((i>=CFOStart) &&(i<CFOEnd))  //coarse CFO detection;
        {
            ax.Real=fx[i-16].Real*fx[i].Real+fx[i-16].Imag*fx[i].Imag;
            ax.Imag=fx[i-16].Real*fx[i].Imag-fx[i-16].Imag*fx[i].Real;

            AcmAx.Real+=ax.Real;
            AcmAx.Imag+=ax.Imag;
            
        }
        else if (i>=CFOEnd+32+64)  //fine CFO detection;
        {
            ax.Real=fx[i-64].Real*fx[i].Real+fx[i-64].Imag*fx[i].Imag;
            ax.Imag=fx[i-64].Real*fx[i].Imag-fx[i-64].Imag*fx[i].Real;

            AcmAx.Real+=ax.Real;
            AcmAx.Imag+=ax.Imag;
        }

        if (i==(CFOEnd-1))  //last sample of short preamble;
        {
            phi1 = atan2(AcmAx.Imag,AcmAx.Real)/(16);  //unit phase offset due to CFO 
            RX.cCFO = phi1/(2.0*pi)*20*1e6;   
            CalcEVM_State()->EVM.cCFO = RX.cCFO;
            AcmAx.Real=AcmAx.Imag=0.0;
        }
        if (i==CFOEnd+32+2*64-1)  //last sample of long preamble
        {
            phi2 = atan2(AcmAx.Imag,AcmAx.Real)/(64);  //unit phase offset due to CFO
            RX.fCFO = phi2/(2.0*pi)*20*1e6;   
            CalcEVM_State()->EVM.fCFO = RX.fCFO;
        }
        x[i].Real=y[i].Real;
        x[i].Imag=y[i].Imag;
    } // end of i;
    
    for ( ; i<Nx; i++)
    {
        y[i].Real = x[i].Real*cos(phi1*i+phi2*(i-CFOEnd-32-64)) + x[i].Imag*sin(phi1*i+phi2*(i-CFOEnd-32-64));
        y[i].Imag = x[i].Imag*cos(phi1*i+phi2*(i-CFOEnd-32-64)) - x[i].Real*sin(phi1*i+phi2*(i-CFOEnd-32-64));

        x[i].Real = y[i].Real;
        x[i].Imag = y[i].Imag;
    }

    w[1].Real=0.0; w[1].Imag=0.0;

    WIFREE( fx );
    WIFREE(  y );
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_CFO


// ================================================================================================
//  Function  : CalcEVM_OFDM_DFT()                                                   
// ------------------------------------------------------------------------------------------------
//  Purpose   : The 64-point DFT, the output are in the order of -32:-1 0:31                                            
//  Parameters: x         -- Received Signal (Time domain);      
//              y         -- Received Signal (Frequency domain)   
// ================================================================================================
wiStatus CalcEVM_OFDM_DFT(wiComplex *x, wiComplex *y)
{
    wiComplex w[64]; //DFT base;
    wiComplex z[64]; //internal DFT output, original order
    int i,k,m;

    if (!x) return STATUS(WI_ERROR_PARAMETER1);
    if (!y) return STATUS(WI_ERROR_PARAMETER2);

    for (i=0; i<32; i++)   
    {
        w[i].Real =  cos(pi*i/32.0);
        w[i].Imag = -sin(pi*i/32.0);
        w[i+32].Real = -w[i].Real;
        w[i+32].Imag = -w[i].Imag;
    }

    for (k=0; k<64; k++)  // frequency index;  in 0-63 order
    {   
        z[k].Real = x[0].Real;
        z[k].Imag = x[0].Imag;
    
        for (i=1; i<64; i++)  // time index
        {   
            m = (i*k) % 64;
            z[k].Real += x[i].Real*w[m].Real - x[i].Imag*w[m].Imag;
            z[k].Imag += x[i].Imag*w[m].Real + x[i].Real*w[m].Imag;
            
        }
    }
    
    for (k=0; k<64; k++) //reorder the output to -32:-1 0:31
    {   
        i = (k+32) % 64;
        y[k].Real = z[i].Real;
        y[k].Imag = z[i].Imag;

    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_DFT


// ================================================================================================
//  Function  : CalcEVM_OFDM_PhaseDet()       //Phase Detection                                           
// ------------------------------------------------------------------------------------------------
//  Purpose   : Detect the Carrier phase errors and sampling phase errors 
//              Using legacy subcarriers for all mode                               
//  Parameters:   U        -- Received freqency domain OFDM signal
//                X_HD     -- Hard decision Frequency domain OFDM signal      
//                H        -- Channel Estimation for subcarriers  Using the Legacy CE 
// ================================================================================================
wiStatus CalcEVM_OFDM_PhaseDet(wiComplex *U, wiComplex *X_HD, wiComplex *H)
{
    wiComplex Z, T;
    wiReal phi[64];  //phase error for each subcarrier
    wiReal eCP_Acm=0.0, eSP_Acm=0.0, cwk_Acm=0.0, swk_Acm=0.0, cwk, swk; //accumulated value of carrier phase error and 
    int i,k,m;
	
    if (!U) return STATUS(WI_ERROR_PARAMETER1);
    if (!X_HD) return STATUS(WI_ERROR_PARAMETER2);
    if (!H) return STATUS(WI_ERROR_PARAMETER3);

    for (i=0; i<64; i++)
    { 
        if (!subcarrier_type_a[i] || (!X_HD[i].Real && !X_HD[i].Imag))  
        { 
            phi[i]=0.0;
        }
        else
        {      
            Z.Real=X_HD[i].Real*H[i].Real-X_HD[i].Imag*H[i].Imag;     // Z = conj(X) *conj(H);
            Z.Imag=-X_HD[i].Real*H[i].Imag-X_HD[i].Imag*H[i].Real;

            T.Real=U[i].Real*Z.Real-U[i].Imag*Z.Imag;  // T= U*Z;
            T.Imag=U[i].Real*Z.Imag+U[i].Imag*Z.Real;
                        
            phi[i]=atan2(T.Imag, T.Real);      
        }
    }

    for (i=6; i<32; i++) // Using Legacy subcarriers
    {		
        k=64-i;
        cwk=min(RX.SNR_Legacy[i], RX.SNR_Legacy[k]);  // Using Legacy CE 
        eCP_Acm+=cwk*(phi[i]+phi[k]);
        cwk_Acm+=cwk;

        m=i+27;
        swk=min(RX.SNR_Legacy[i], RX.SNR_Legacy[m]);
        eSP_Acm+=swk*(phi[m]-phi[i]);
        swk_Acm+=swk;
	
    }

    RX.eCP=(cwk_Acm)?eCP_Acm/(2.0*cwk_Acm):0.0;
    RX.eSP=(swk_Acm)?eSP_Acm/(27.0*swk_Acm):0.0;
	
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_PhaseDet

// ================================================================================================
//  Function  : CalcEVM_OFDM_DPLL()
// ------------------------------------------------------------------------------------------------
//  Purpose   : Loop Filter and VCO
//                                                           
//  Parameters: n: Sym index; 
//              HoldVCO: use phase error of zero and continue running VCO during HT preamble 
// ================================================================================================
wiStatus CalcEVM_OFDM_DPLL(int n, wiBoolean HoldVCO)
{
    wiReal cA, cB, sA, sB; //Loop filter coefficients;
    static wiReal wc[2]={0.0}, ws[2]={0.0}, yc[2]={0.0}, ys[2]={0.0};  //Delayline for Loop filter and VCO
    wiReal VCOIn_c,VCOIn_s;  //Loop filter output, VCO input;
	int sym;
	wiReal eCP, eSP;

	if (n<0) // L-SIG is -5;
		sym = n + 5; 
	else  // 5 header symbols for MM; 1 for Legacy
		sym = RX.MM_DATA? n+5: n+1;

    
	// HoldVCO for HT preamble
	eCP = HoldVCO? 0 : RX.eCP;
	eSP = HoldVCO? 0 : RX.eSP;

    cA=pow(2.0, -RX.Ca);
    cB=pow(2.0, -RX.Cb);
    sA=pow(2.0, -RX.Sa);
    sB=pow(2.0, -RX.Sb);

    wc[0]=cB*eCP+wc[1];
    VCOIn_c=cA*eCP+wc[0];
    yc[0]=VCOIn_c+yc[1];
    
    RX.cCP[sym]=yc[0];
            
    ws[0]=sB*eSP+ws[1];
    VCOIn_s=sA*eSP+ws[0];
    ys[0]=VCOIn_s+ys[1];
    
    RX.AdvDly[sym]=0;  //Reset the AdvDly for each SYM;
 
    if (RX.cSPEnabled && (fabs(ys[0])>(pi/64)))
    {
        RX.AdvDly[sym]=(ys[0]>0)?-1:1; //If clock is slower, advance by one sample, if clock is faster, delay one sample;
        ys[0]=(ys[0]>0)?(-pi/32+ys[0]):(pi/32+ys[0]); //Overadvanced, negtive phase; overdelayed, positive phase;
    }
  
    RX.cSP[sym]=ys[0];

    if (n==RX.N_SYM-1)  //Reset after each packets   // Need change for HT  RX.N_SYM need to refer to HT N_SYM
    {
        wc[1]=0.0;
        yc[1]=0.0;
        ws[1]=0.0;
        ys[1]=0.0;
    }
    else
    {
        wc[1]=wc[0];
        yc[1]=yc[0];
        ws[1]=ws[0];
        ys[1]=ys[0];
    }

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_DPLL


// ================================================================================================
//  Function  : CalcEVM_OFDM_PhaseCorr()
// ------------------------------------------------------------------------------------------------
//  Purpose   : Correcting the phase error for non-null subcarriers
//  Parameters:   n      -- Symbol index
//                X      -- Input signal 
//                Y      -- Output signal
// ================================================================================================
wiStatus CalcEVM_OFDM_PhaseCorr(int n, wiComplex *X, wiComplex *Y)
{ 
    int i;
    wiReal phi;
    wiComplex z;
	int sym;

    if (!X) return STATUS(WI_ERROR_PARAMETER1);
    if (!Y) return STATUS(WI_ERROR_PARAMETER2);

	if (n<0)
		sym = n+4;
	else 
		sym = RX.MM_DATA? n + 4: n; // DPLL output of HT-LTF (MM) or L-SIG (Legacy)

    for (i=0; i<64; i++)
    { 
        phi=RX.cCP[sym];
        z.Real=RX.cCPEnabled?X[i].Real*cos(phi)+X[i].Imag*sin(phi):X[i].Real; // X*exp(-phi)
        z.Imag=RX.cCPEnabled?X[i].Imag*cos(phi)-X[i].Real*sin(phi):X[i].Imag;

        if (RX.cSPEnabled)
        {   phi=(i-32)*RX.cSP[sym];
            Y[i].Real=z.Real*cos(phi)+z.Imag*sin(phi);
            Y[i].Imag=z.Imag*cos(phi)-z.Real*sin(phi);
        }
        else
        {
            Y[i].Real=z.Real;
            Y[i].Imag=z.Imag;
        }
    }
    return WI_SUCCESS;  
}
// end of CalcEVM_OFDM_PhaseCorr


// ================================================================================================
//  Function  : CalcEVM_OFDM_ChanlEst()                                                   
// ------------------------------------------------------------------------------------------------
//  Purpose   : Estimate the channel using the long training symbols. For legacy CE, using two symbols.
//			    For HT MM CE, using one symbol. Calculate noise power during legacy CE.
//              H array and S_N array contains valid information for DATA, pilot and center subcarrier
//                                                           
//  Parameters: x          -- Received Time Sequence starting from the first LP (without GI);      
//              H		   -- Channel Estimation 
//              S_N		   -- Average SNR per subcarrier
//              MM-HT-LTF  -- Mixedmode HT-LTF for HT CE. For legacy CE, it is zero
// ================================================================================================
wiStatus CalcEVM_OFDM_ChanlEst(wiComplex *x, wiComplex *H, wiReal *S_N, wiBoolean MM_HT_LTF)
{ 
    wiComplex S1[64], S2[64];        // frequency domain 
    wiReal aReal, aImag;
	static wiReal PwrAcm=0.0; // noise power accumulator;
    int i;
	const int L[64]   = { 0,0,0,0,0,0,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,  // L[32] set to 1 to estimate center subcarrier; 
                      1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,0,0,0,0,0 };
    
	const int L_HT[64]={ 0,0,0,0,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,  // L_HT[32] set to 1 to estimate center subcarrier; 
                      1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,-1,-1,0,0,0 };

    if (!x  ) return STATUS(WI_ERROR_PARAMETER1);
    if (!H  ) return STATUS(WI_ERROR_PARAMETER2);
    if (!S_N) return STATUS(WI_ERROR_PARAMETER3);

    XSTATUS(CalcEVM_OFDM_DFT(x, S1));
	
	if (MM_HT_LTF) // CE based on MM HT-LTF1
	{
		XSTATUS(CalcEVM_OFDM_PhaseCorr( -1, S1, S2));  // S2: Phase corrected signal for HT-LTF.  -1: The one before DATA symbol
		for (i=0; i<64; i++)
		{  
			H[i].Real = S2[i].Real * L_HT[i];
			H[i].Imag = S2[i].Imag * L_HT[i];
			S_N[i] = SQR(H[i].Real) + SQR(H[i].Imag);
		}
	}
	else
	{
		PwrAcm = 0.0; // initialize noise power accumulator 
		XSTATUS(CalcEVM_OFDM_DFT(x+64, S2));
		for (i=0; i<64; i++)
		{  
			H[i].Real = (S1[i].Real + S2[i].Real) * L[i] / 2.0;  // S2: The second L_LTF
			H[i].Imag = (S1[i].Imag + S2[i].Imag) * L[i] / 2.0;

			if (subcarrier_type_a[i]) // legacy subcarriers for noise power calculation
			{
				aReal = SQR(S1[i].Real - S2[i].Real);
				aImag = SQR(S1[i].Imag - S2[i].Imag);
  
                PwrAcm += aReal + aImag; // 2*Noise power;
			}
			S_N[i] = SQR(H[i].Real) + SQR(H[i].Imag);
        }
		PwrAcm /= (2.0*52.0);  // average over 52 legacy subcarriers, each with 2*Noise power; Use this noise power for HT estimation. 
	}
    
    if (PwrAcm)
        for (i=0; i<64; i++)		
            S_N[i] /= PwrAcm;
    else
        for (i=0; i<64; i++)
            S_N[i] *= 1e5;

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_ChanlEst


// ================================================================================================
//  Function  : CalcEVM_OFDM_Eq()
// ------------------------------------------------------------------------------------------------
//  Purpose   : Equalize the OFDM signal by dividing H
//  Parameters:   N         -- Number of samples, 64
//                X         -- Frequency domain OFDM signal      
//                H         -- Channel Estimation for subcarriers
//                Y         -- Equalized signals 
// ================================================================================================
wiStatus CalcEVM_OFDM_Eq(int N, wiComplex *X, wiComplex *H, wiComplex *Y)
{      
    int i;
    const int *subcarrier_type;        
    if (!X) return STATUS(WI_ERROR_PARAMETER2);
    if (!Y) return STATUS(WI_ERROR_PARAMETER4);

    subcarrier_type = RX.MM_DATA ? subcarrier_type_n : subcarrier_type_a; 
    for (i=0; i<N; i++)
	{
		if (subcarrier_type[i])
		{
            Y[i] = wiComplexDivide(X[i], H[i]);
		}
		else
		{
			Y[i].Real = 0.0;
			Y[i].Imag = 0.0;
		}
	}

    return WI_SUCCESS;   
}
// end of CalcEVM_OFDM_Eq()

// ================================================================================================
//  Function  : CalcEVM_OFDM_CheckRotate()
// ------------------------------------------------------------------------------------------------
//  Purpose   : Check the rotation of the symbol after L-SIG to distiguish 6 Mbps Data from HT-SIG1
//  Parameters: none
// ================================================================================================
wiStatus CalcEVM_OFDM_CheckRotate(void)
{      
    int i;
	wiComplex SIG1[64], E_SIG[64];
    wiReal AccmI = 0.0, AccmQ = 0.0;  

    XSTATUS(CalcEVM_OFDM_DFT(RX.v+RX.FSOffset+240+16, SIG1));
    XSTATUS(CalcEVM_OFDM_Eq(64, SIG1, RX.H, E_SIG));

    for (i=0; i<64; i++)
	{
		if (subcarrier_type_a[i] == 1)  // Accumulate over 48 legacy data subcarriers
		{
			AccmI += fabs(E_SIG[i].Real);
			AccmQ += fabs(E_SIG[i].Imag);			
		}
	}
	if (AccmQ > AccmI ) RX.Rotated6Mbps = 1; 

    return WI_SUCCESS;   
}
// end of CalcEVM_OFDM_CheckRotate()


//===========================================================================
//  Function  : CalcEVM_OFDM_Deintv()                                 
//---------------------------------------------------------------------------
//  Purpose   : Implement the inverse interleaver of 802.11 section 17.3.5.6.
//			    For HT DATA, the NCol is 13;	 
//  Parameters: N_CBPS -- Number of code bits per OFDM symbol                 
//              N_BPSC -- Number of bits per subcarrier                       
//              x      -- Input array                                         
//              y      -- Output array                                        
//===========================================================================
wiStatus CalcEVM_OFDM_Deintv(int N_CBPS, int N_BPSC, wiReal *x, wiReal *y)
{
    int i, j, k;
    int s = max(N_BPSC/2, 1);
	int NCol = RX.MM_DATA? 13: 16; 

    if (!x) return STATUS(WI_ERROR_PARAMETER3);
    if (!y) return STATUS(WI_ERROR_PARAMETER4);

    for (k=0; k<N_CBPS; k++)
    {
        i = (N_CBPS/NCol) * (k%NCol) + (k/NCol);
        j = s * (i/s) + ((i + N_CBPS - (NCol * i/N_CBPS)) % s);
        y[k] = x[j];
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_Deintv() 


//===========================================================================
//  Function  : CalcEVM_OFDM_SoftDetector                                   
//----------------------------------------------------------------------------
//  Purpose   : The soft-output detector provides approximate log-likelihood  
//              estimates for the coded data bits that were used to modulate  
//              the 48 (52 for HT-MM) information bearing subcarriers in the
//              OFDM symbol. A hard decision can be formed as h = (p>0)? 1:0.                
//  Parameters: N_BPSC -- Number of coded bits per subcarrier                 
//                        {1=BPSK, 2=QPSK, 4=16-QAM, 6=64-QAM, 8=256-QAM}     
//              d       -- Equalized OFDM symbol             
//              S_N     -- SNR per data subcarrier
//              Lp      -- Permuted Soft-decision (approximate log likelihood ratios) 
//              HTSIG   -- Rotated 6Mbps signal  
//===========================================================================
wiStatus CalcEVM_OFDM_SoftDetector(int N_BPSC, wiComplex *d, wiReal *S_N, wiReal *Lp, wiBoolean HTSIG)
{   
    wiReal A,P;
    wiReal Y_I, Y_Q, SN; // scaled input and SNR;  S_N=4.0*S_N/P; (note: Mojave uses factor of 2.0; Here 4.0 is used to take account the complex variance;);
    wiReal p;
    int i,k;
	const int *subcarrier_type = RX.MM_DATA ? subcarrier_type_n : subcarrier_type_a; 

    // --------------------
    // Error/Range Checking
    // --------------------
    if ((N_BPSC!=1) && (N_BPSC!=2) && (N_BPSC!=4) && (N_BPSC!=6) && (N_BPSC !=8)) return WI_ERROR_PARAMETER1;
    if (!d)  return WI_ERROR_PARAMETER2;
    if (!S_N)  return WI_ERROR_PARAMETER3;
    if (!Lp) return WI_ERROR_PARAMETER4;
    
    if (RX.aModemTX)
    {
		if (RX.MM_DATA)
		{
			switch (N_BPSC)
			{
				case 1: A = 201.0/201.0;   break;
				case 2: A = 201.0/139.0;   break;
				case 4: A = 201.0/ 62.0;   break;
				case 6: A = 201.0/ 31.0;   break;				
				default: return STATUS(WI_ERROR_UNDEFINED_CASE);
			}
		}
		else
		{
			switch (N_BPSC)
			{
				case 1: A = 13.0/13.0;   break;
				case 2: A = 13.0/ 9.0;   break;
				case 4: A = 13.0/ 4.0;   break;
				case 6: A = 13.0/ 2.0;   break;
				case 8: A = 13.0/ 1.0;   break;
				default: return STATUS(WI_ERROR_UNDEFINED_CASE);
			}
		}
    }
    else
    {
        switch (N_BPSC)
        {
            case 1: A = 1.0;          break;
            case 2: A = sqrt(2.0);     break;
            case 4: A = sqrt(10.0);    break;
            case 6: A = sqrt(42.0);    break;
            case 8: A = sqrt(170.0);   break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    
    P = A*A;
    
    for (k=0, i=0; k<64; k++)
    {
		if (subcarrier_type[k] == 1)
		{
			Y_I = A*d[k].Real;
			Y_Q = A*d[k].Imag;
			SN = 4.0*S_N[k]/P; //noise power enhanced 

			switch (N_BPSC)
			{
				case 1:
					Lp[i]=HTSIG? SN*Y_Q : SN*Y_I; // If HTSIG, output Q signal;
					break;
            
				case 2:
					Lp[2*i]=SN*Y_I;
					Lp[2*i+1]=SN*Y_Q;
					break;

				case 4:
					Lp[4*i]=SN*Y_I;
					p=2-fabs(Y_I);
					Lp[4*i+1]=SN*p;

					Lp[4*i+2]=SN*Y_Q;
					p=2-fabs(Y_Q);
					Lp[4*i+3]=SN*p;

					break;

				case 6:
					Lp[6*i]=SN*Y_I;
					p=4-fabs(Y_I);
					Lp[6*i+1]=SN*p;
					p=2-fabs(p);
					Lp[6*i+2]=SN*p;

					Lp[6*i+3]=SN*Y_Q;
					p=4-fabs(Y_Q);
					Lp[6*i+4]=SN*p;
					p=2-fabs(p);
					Lp[6*i+5]=SN*p;

					break;

				case 8:
					Lp[8*i]=SN*Y_I;
					p=8-fabs(Y_I);
					Lp[8*i+1]=SN*p;
					p=4-fabs(p);
					Lp[8*i+2]=SN*p;
					p=2-fabs(p);
					Lp[8*i+3]=SN*p;
                
					Lp[8*i+4]=SN*Y_Q;
					p=8-fabs(Y_Q);
					Lp[8*i+5]=SN*p;
					p=4-fabs(p);
					Lp[8*i+6]=SN*p;
					p=2-fabs(p);
					Lp[8*i+7]=SN*p;

					break;
				default: return STATUS(WI_ERROR_UNDEFINED_CASE);
			}
			i++;
		}
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_SoftDetector()


// ================================================================================================
//  Function  : CalcEVM_OFDM_ViterbiDecoder()                                      
//-------------------------------------------------------------------------------------------------
//  Purpose   : Implement a soft-input Viterbi decoder for the rate 1/2 convolutional 
//              code specified for 802.11a.                     
//  Parameters: NumBits         -- Number of (decoded) bits to compute in OutputData
//              pInputLLR       -- Input array of log-likelihood ratios                 
//              OutputData      -- Output array (decoded data bits)                     
//              TruncationDepth -- Trellis trunction depth
//              CodeRate12      -- CodeRate * 12...defines the puncturing pattern       
// ================================================================================================
wiStatus CalcEVM_OFDM_ViterbiDecoder(int NumBits, wiReal pInputLLR[], int OutputData[], 
                                     int CodeRate12, int TruncationDepth)
{
    int i, k, M;
    const int *pA, *pB;
    int nSR_1, tau;
    int A0[64], A1[64], B0[64], B1[64], S, S0, S1, Sx;    //A0, A1: output register for bit 0 corresponding to 

    unsigned int Pk[64][8], Pk_1[64][8], outBit;     //use two memories to do register exchange;

    wiReal Mk[64], Mk_1[64], MinMetric;
    wiReal LLR_A, LLR_B, m0, m1;
    
    if (InvalidRange(NumBits, 7, 16777216)) return WI_ERROR_PARAMETER1;
    if (pInputLLR == NULL) return WI_ERROR_PARAMETER2;
    if (OutputData == NULL) return WI_ERROR_PARAMETER3;
    if (InvalidRange(TruncationDepth, 15, 255)) return WI_ERROR_PARAMETER4;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Select the puncturing pattern
    //  Parameter CodeRate12 is the code rate multiplied by 12; i.e., for an unpunctured
    //  rate 1/2 code, CodeRate12 = 12(1/2) = 6.
    //  
    switch (CodeRate12)
    {
        case 6:  pA = PunctureA_12;  pB = PunctureB_12;  M = 1;  break;
        case 8:  pA = PunctureA_23;  pB = PunctureB_23;  M = 6;  break;
        case 9:  pA = PunctureA_34;  pB = PunctureB_34;  M = 9;  break;
		case 10: pA = PunctureA_56;  pB = PunctureB_56;  M = 5;  break; // M: pattern length 
        case 12:
        {
            for (k=0; k<NumBits; k++) // uncoded...simple hard decision
                OutputData[k] = (pInputLLR[k] >= 0.0) ? 1:0;

            return WI_SUCCESS;
            break;
        }
        default: return WI_ERROR_PARAMETER5;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Restrict the path memory depth (tau) to the message length. Then compute the
    //  number of shift registers required along with the position of the output bit.
    //
    tau = (TruncationDepth > NumBits) ? NumBits : TruncationDepth;
    nSR_1 = ((tau+32) / 32) - 1; 
    outBit = 1 << (tau % 32);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Clear the path history registers and initialize the metrics (bias so state 0 has
    //  the best starting metric.
    //
    for (S=0; S<64; S++)
        for (i=0; i<=nSR_1; i++)
            Pk_1[S][i] = 0;

    for (S=1; S<64; S++)
        Mk_1[S] = -1.0E+6;  // suppress these states
    Mk_1[0] = 0.0;          // initial state is state 0


    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Build a table of coded bits as a function of old+new state
    //
    for (S=0; S<64; S++)
    {
        A0[S] = ((S>>0) ^ (S>>2) ^ (S>>3) ^ (S>>5) ^ 0) & 1;
        A1[S] = ((S>>0) ^ (S>>2) ^ (S>>3) ^ (S>>5) ^ 1) & 1;

        B0[S] = ((S>>0) ^ (S>>1) ^ (S>>2) ^ (S>>3) ^ 0) & 1;
        B1[S] = ((S>>0) ^ (S>>1) ^ (S>>2) ^ (S>>3) ^ 1) & 1;
    }
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    // Apply the Viterbi decoder
    // 
    for (k=0; k<NumBits; k++)
    {
        LLR_A = pA[k%M] ? *(pInputLLR++) : 0.0;
        LLR_B = pB[k%M] ? *(pInputLLR++) : 0.0;

        ////////////////////////////////////////////////////////////////////////////
        //
        //  Loop through state-by-state operations
        //
        for (S=0; S<64; S++)
        {
            S0 = (S >> 1);  // The MSB is 0
            S1 = S0 | 0x20; // The MSB is 1

            ////////////////////////////////////////////////////////////////////////
            //
            //  Metric computation
            //  The sign of the LLR is reversed so a "minimum" metric is favored
            //
            m0 = Mk_1[S0] + (A0[S]? -LLR_A:LLR_A) + (B0[S]? -LLR_B:LLR_B);
            m1 = Mk_1[S1] + (A1[S]? -LLR_A:LLR_A) + (B1[S]? -LLR_B:LLR_B);

            // --------------------
            // Select survivor path
            // --------------------
            if (m0 <= m1)
            {
                Mk[S] = m0;
                Sx = S0;
            }
            else
            {
                Mk[S] = m1;
                Sx = S1;
            }
            // -----------------------------------
            // Copy the path memory shift register
            // -----------------------------------
            for (i=1; i<=nSR_1; i++)
                Pk[S][i] = (Pk_1[Sx][i]);

            Pk[S][0] = Pk_1[Sx][0] | (S&1);  // Shift the input bit to the path memory
        }
        // ---------------------
        // Output a decision bit
        // ---------------------
        if (k>=tau)
        {
            Sx = 0;
            MinMetric = Mk[0];
            for (S=1; S<64; S++) 
{
                if (Mk[S] < MinMetric) 
                {
                    MinMetric = Mk[S];
                    Sx = S;
                }
            }
            OutputData[k-tau] = (Pk[Sx][nSR_1] & outBit) ? 1 : 0;
        }
        // -------------------
        // Copy ?k[] to ?k_1[]
        // -------------------
        for (S=0; S<64; S++) 
        {
            Mk_1[S] = Mk[S];
            for (i=nSR_1; i>0; i--) {
                Pk_1[S][i] = (Pk[S][i] << 1) | (Pk[S][i-1] >> 31);
            }
            Pk_1[S][0] = Pk[S][0] << 1;
        }
    }
    // --------------------------------------------
    // Copy the survivor path into the output array
    // --------------------------------------------
    for ( ; k < (NumBits + tau); k++)
    {
        for (i=nSR_1; i>0; i--)
            Pk[0][i] = (Pk[0][i] << 1) | (Pk[0][i-1] >> 31);
        Pk[0][0] = Pk[0][0] << 1;

        OutputData[k-tau] = (Pk[0][nSR_1] & outBit) ? 1 : 0;

    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_ViterbiDecoder()


// =====================================================================================
//  Function  : CalcEVM_OFDM_Descrambler()                                                     
// -------------------------------------------------------------------------------------
//  Purpose   : Scramble the input sequence using PRBS x^7+x^4+1. Because the operation 
//              is modulo-2, it undoes a prior scrambler with the same polynomial. The 
//              first seven bits of the "unscrambled" bit sequence are zero, so this is
//              used to determine the state of the scrambler shift register.
// Parameters : n -- Number of bits to scramble
//              b -- Input array
//              a -- Output array
// =====================================================================================
wiStatus CalcEVM_OFDM_Descrambler(int n, int b[], int a[])
{
    int k, x, X=0;
    int X0, X1, X2, X3, X4, X5, X6;

    if (!b) return STATUS(WI_ERROR_PARAMETER2);
    if (!a) return STATUS(WI_ERROR_PARAMETER3);

    // ------------------------------------
    // Get the shift register initial state
    // ------------------------------------
    X0 = b[6] ^ b[2];
    X1 = b[5] ^ b[1];
    X2 = b[4] ^ b[0];
    X3 = b[3] ^ X0;
    X4 = b[2] ^ X1;
    X5 = b[1] ^ X2;
    X6 = b[0] ^ X3;

    X = X0 | (X1<<1) | (X2<<2) | (X3<<3) | (X4<<4) | (X5<<5) | (X6<<6);

    // ----------------------------------------------
    // Apply the scrambler to the rest of the message
    // ----------------------------------------------
    for (k=0; k<n; k++)
    {
        x = ((X>>6) ^ (X>>3)) & 1;   // P(X) = X^7 + X^4 + 1;
        X = (X << 1) | x;
        a[k] = b[k] ^ x;
    }
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_Descrambler()


// =====================================================================================
//  Function  : CalcEVM_OFDM_HD_EVM()                                                     
// -------------------------------------------------------------------------------------
//  Purpose   : Get hard decision and calulate the EVM;                                         
//  Parameters:   n        -- index of the OFDM symbol  // x.yu check if the first data symbol start with 0
//                N_BPSC   -- # of bits per subcarrier   
//                A        -- Scaling for constellations
//                X_de     -- Equalized signal  
//                HD       -- Hard decision of OFDM symbol   
// =====================================================================================
wiStatus CalcEVM_OFDM_HD_EVM(int n, int N_BPSC, wiComplex *X_de, wiComplex *HD, wiBoolean HTSIG)
{   
    int aReal, aImag, mReal, mImag; //hard decision and constellation boundary
    wiComplex eq, e; double e2;
    int k,m;
    int p_bit;
    wiReal A;
	const int *subcarrier_type = RX.MM_DATA? subcarrier_type_n: subcarrier_type_a; 
	int pilot_sym;
	int sym_ofs; // The evm equalized signal array offset of current symbol  

    static int X = 0x7F; //initial register state for pilot tone scramble

    CalcEVM_State_t *pState = CalcEVM_State();

    // -------------------------
    // Scaling for Constellation
    // -------------------------
    if (RX.aModemTX) // Using Nevada constellation
    {
		if (RX.MM_DATA)
		{
			switch (N_BPSC)
            {
                case 1: A = 201.0/201.0;   mReal= 1; mImag=0;     break;
                case 2: A = 201.0/139.0;   mReal= 1; mImag=mReal; break;
                case 4: A = 201.0/ 62.0;   mReal= 3; mImag=mReal; break;
                case 6: A = 201.0/ 31.0;   mReal= 7; mImag=mReal; break;
                default: return STATUS(WI_ERROR_UNDEFINED_CASE);
            }
		}
		else
		{
			switch (N_BPSC)
			{
				case 1: A = 13.0/13.0;	
					    if (HTSIG)
						{	mReal = 0; mImag = 1;}
					    else
						{	mReal = 1; mImag = 0;}  
					    break;
				case 2: A = 13.0/ 9.0;   mReal = 1; mImag = mReal; break;
				case 4: A = 13.0/ 4.0;   mReal = 3; mImag = mReal; break;
				case 6: A = 13.0/ 2.0;   mReal = 7; mImag = mReal; break;
				case 8: A = 13.0/ 1.0;   mReal =15; mImag = mReal; break;
				default: return STATUS(WI_ERROR_UNDEFINED_CASE);
			}
		}
     }
     else  // ideal constellation
     {
		 switch (N_BPSC)
         {
			 case 1: A = 1.0;         
					 if (HTSIG)
					 {	mReal = 0; mImag = 1;}
					 else
					 {	mReal = 1; mImag = 0;}  
					 break;
             case 2: A = sqrt(2.0);     mReal = 1;  mImag = mReal; break;
             case 4: A = sqrt(10.0);    mReal = 3;  mImag = mReal; break;
             case 6: A = sqrt(42.0);    mReal = 7;  mImag = mReal; break;
             case 8: A = sqrt(170.0);   mReal = 15; mImag = mReal; break;
             default: return STATUS(WI_ERROR_UNDEFINED_CASE);
         }
     }

	p_bit = ((X>>6) ^ (X>>3)) & 0x1;

    X = ((X<<1) & 0x7E) | p_bit;

	switch (n) 
	{
		case -5: sym_ofs = 0;  break;
		case -4: sym_ofs = 64; break;
		case -3: sym_ofs = 64*2; break;
		default: sym_ofs = RX.MM_DATA ? 64*(3+n) : 64*(1+n);
	}

	m = 0; // index of pilot tone;
    for (k=0; k<64; k++)
    {         
        
		eq.Real = A*X_de[k].Real;  //The equalized signal;
        eq.Imag = A*X_de[k].Imag;
		
		if (subcarrier_type[k] == 1)
		{
			aReal = (int)(1.0+2.0*floor(eq.Real/2.0));  //Real part of the constellation
			aImag = (int)(1.0+2.0*floor(eq.Imag/2.0));  //Imag part of the constellation
                
			if (abs(aReal)>mReal) aReal = (aReal>0) ? mReal:-mReal;
			if (abs(aImag)>mImag) aImag = (aImag>0) ? mImag:-mImag;

			e.Real = eq.Real-aReal;
			e.Imag = eq.Imag-aImag;
			e2 = (SQR(e.Real) + SQR(e.Imag)) / (SQR(A));

			pState->EVM.x[sym_ofs+k].Real = eq.Real; // EVM.x includes L-SIG, HTSIG, DATA symbol
			pState->EVM.x[sym_ofs+k].Imag = eq.Imag;

			if (n>=0)  // DATA subcarriers of DATA symbol
			{				
				pState->EVM.cSe2[k] += e2;
				pState->EVM.nSe2[n] += e2;
				pState->EVM.Se2 += e2;
			}
        
			switch (N_BPSC)
			{   
				case 1:
				case 2:
				case 4:
					HD[k].Real= (double) aReal;
					HD[k].Imag= (double) aImag;         
					break;
    
				case 6:
					if ((abs(aReal)<2) &&(abs(aImag)<2)) // block small values 
					{
						HD[k].Real=0.0;
						HD[k].Imag=0.0;            
					}
					else
					{
						HD[k].Real=(double) aReal;
						HD[k].Imag= (double) aImag;            
					}
					break;
            
				case 8:
					if ((abs(aReal)<4) &&(abs(aImag)<4))
					{
						HD[k].Real=0.0;
						HD[k].Imag=0.0;               
					}
					else
					{
						HD[k].Real=(double) aReal;
						HD[k].Imag= (double) aImag;               
					}
					break;
            
				default:
					return STATUS(WI_ERROR_UNDEFINED_CASE);
			}
		}
		else if (subcarrier_type[k] == 2)
		{
			pilot_sym = ((RX.MM_DATA ? (n+m)%4 : m) == 3) ? -1: 1;			
			HD[k].Real = (double) pilot_sym*(1-2*p_bit);
			HD[k].Imag = 0.0;
			pState->EVM.x[sym_ofs+k].Real = eq.Real;
			pState->EVM.x[sym_ofs+k].Imag = eq.Imag;
			m += 1;				
		}
		else 
		{
			HD[k].Real=0.0;
			HD[k].Imag=0.0;
		}
    } 
  
	if (n == RX.N_SYM-1) X=0x7F;
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_HD_EVM()

// =====================================================================================
//  Function  : CalcEVM_OFDM_SIG()                                                     
// -------------------------------------------------------------------------------------
//  Purpose   : Decode the SIG Symbol to get TX parameters  
// =====================================================================================
wiStatus CalcEVM_OFDM_SIG(void)
{   
    wiComplex SIG[64], E_SIG[64], HD[64]; //Freq Sig, Equalized Sig and hard decision;
    wiReal   pSIG[48], I_SIG[48];  //soft information for the SIG part; BPSK; before and after interleave; 
    int i,k, *b;
    wiReal bit[48];

    CalcEVM_State_t *pState = CalcEVM_State();
    
    XSTATUS(CalcEVM_OFDM_DFT(RX.v+RX.FSOffset+160+16, SIG));
    XSTATUS(CalcEVM_OFDM_Eq(64, SIG, RX.H, E_SIG));
    
    XSTATUS(CalcEVM_OFDM_SoftDetector(1, E_SIG, RX.SNR_Legacy, pSIG,0));
    XSTATUS(CalcEVM_OFDM_Deintv(48, 1, pSIG, I_SIG));
    
    for (i=0; i<48; i++)
        bit[i] = (pSIG[i]>0) ? 1.0 : ((pSIG[i]<0) ? -1.0:0.0);

    XSTATUS(CalcEVM_OFDM_ViterbiDecoder(24, I_SIG, RX.bSIG, 6, 64));

    b = RX.bSIG;
    RX.RATE.word = (b[0]<<3) | (b[1]<<2) | (b[2]<<1) | (b[3]<<0);
  
    RX.LENGTH = 0x00;
    for (i=0; i<12; i++)
        RX.LENGTH = RX.LENGTH | (b[5+i] << i);
  
	RX.L_LENGTH = RX.LENGTH;

    if (InvalidRange(RX.LENGTH,0,4095)) 
    {
        RX.Fault = 1;
        return STATUS(WI_WARN_PHY_SIGNAL_LENGTH);
    }

    switch (RX.RATE.w4)
    {
        case 0xD: RX.N_BPSC = 1; RX.N_DBPS =  24; break;
        case 0xF: RX.N_BPSC = 1; RX.N_DBPS =  36; break;
        case 0x5: RX.N_BPSC = 2; RX.N_DBPS =  48; break;
        case 0x7: RX.N_BPSC = 2; RX.N_DBPS =  72; break;
        case 0x9: RX.N_BPSC = 4; RX.N_DBPS =  96; break;
        case 0xB: RX.N_BPSC = 4; RX.N_DBPS = 144; break;
        case 0x1: RX.N_BPSC = 6; RX.N_DBPS = 192; break;
        case 0x3: RX.N_BPSC = 6; RX.N_DBPS = 216; break;
        case 0xA: RX.N_BPSC = 8; RX.N_DBPS = 288; break;
        default: 
            RX.Fault = 1;
            return STATUS(WI_WARN_PHY_SIGNAL_RATE);
    }
     
    RX.N_SYM = (16 + 8*(RX.LENGTH) + 6 + (RX.N_DBPS-1)) / RX.N_DBPS;
     
    if ((RX.N_SYM-(int)(RX.Nv-RX.FSOffset-240)/80)>0)
    {
        RX.Fault = 1; 
        return STATUS(WI_WARN_PHY_SIGNAL_LENGTH);
    }
    RX.N_CBPS=48*RX.N_BPSC;

    //------------------------------------------------------------------
    // Get the CalcEVM_State EVM State pointer and do the initialization;
	// Assign the array size according to legacy N_SYM. It is enough
	// For HT packets
    //------------------------------------------------------------------
    {
        pState->EVM.N_SYM = RX.N_SYM; 
       
        for (i=0; i<64; i++) {            
            pState->EVM.h[i].Real = pState->EVM.h[i].Imag = 0.0;
            pState->EVM.cSe2[i] = 0.0; // error by subcarrier
        }  
                        
        pState->EVM.Se2 = 0.0;   //total error
        for (i=0; i<RX.N_SYM; i++) pState->EVM.nSe2[i] = 0.0;  //error by OFDM symbol
                
		// The legacy N_SYM should cover the whole HT portion
        WIFREE  ( pState->EVM.u );
        WICALLOC( pState->EVM.u, 64*(pState->EVM.N_SYM+1), wiComplex );

		// The legacy N_SYM should cover the whole HT portion
        WIFREE  ( pState->EVM.x );
        WICALLOC( pState->EVM.x, 64*(pState->EVM.N_SYM+1), wiComplex );

        for (k=0; k<64; k++)
        {    
            pState->EVM.h[k].Real=RX.H[k].Real;  // save the channel response based on Legacy CE
            pState->EVM.h[k].Imag=RX.H[k].Imag;
            pState->EVM.u[k].Real=SIG[k].Real;  // phase corrected signal
            pState->EVM.u[k].Imag=SIG[k].Imag;		
        }        
    } 

	//---------------------------------------------------------
	// Allocate arrays for AdvDly, cCP, cSP
	// The size is based on legacy N_SYM. Sufficient for HT MM
	// --------------------------------------------------------
	if (!RX.Fault)
	{
		WIFREE( RX.AdvDly );
		WIFREE( RX.cCP    );
        WIFREE( RX.cSP    );

        WICALLOC( RX.AdvDly, RX.N_SYM + 1, int );
        WICALLOC( RX.cCP,    RX.N_SYM + 1, wiReal );
        WICALLOC( RX.cSP,    RX.N_SYM + 1, wiReal );
	}

    XSTATUS(CalcEVM_OFDM_HD_EVM(-5, 1, E_SIG, HD,0)); // DATA symbol starts from 0. Set L-SIG as -5 to account for HT header
    XSTATUS(CalcEVM_OFDM_PhaseDet(SIG, HD, RX.H));
    XSTATUS(CalcEVM_OFDM_DPLL(-5,0));   //Get the correcting phase for first Data symbol or HTSIG1

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_SIG()

// =====================================================================================
//  Function  : CalcEVM_OFDM_HTSIG_HTCE()                                                     
// -------------------------------------------------------------------------------------
//  Purpose   : Decode the HTSIG Symbol to get TX parameters for HT MM packets  
//  Note	  : GF packets not supported                                              
// =====================================================================================
wiStatus CalcEVM_OFDM_HTSIG_HTCE(void)
{   
    wiComplex FFT64[64], SIG1[64], SIG2[64], E_SIG[64], HD[64]; //Freq Sig, Phase Corrected, Equalized Sig and hard decision;
    wiReal  pHTSIG[96], I_HTSIG[96];  //soft information for the SIG part; BPSK; before and after interleave; 
    int i,k, *b;
	unsigned int PktDuration; // PktDuration including 

    // HT SIG one 
    XSTATUS(CalcEVM_OFDM_DFT(RX.v+RX.FSOffset+240+16, FFT64));
	XSTATUS(CalcEVM_OFDM_PhaseCorr( -4, FFT64, SIG1));  //Phase corrected signal HTSIG1
    XSTATUS(CalcEVM_OFDM_Eq(64, SIG1, RX.H, E_SIG));
    
    XSTATUS(CalcEVM_OFDM_SoftDetector(1, E_SIG, RX.SNR_Legacy, pHTSIG, 1)); // HTSIG: output imaginary signal
    XSTATUS(CalcEVM_OFDM_Deintv(48, 1, pHTSIG, I_HTSIG));
   
	XSTATUS(CalcEVM_OFDM_HD_EVM(-4, 1, E_SIG, HD,1)); // HTSIG: output imaginary signal
    XSTATUS(CalcEVM_OFDM_PhaseDet(SIG1, HD, RX.H));
    XSTATUS(CalcEVM_OFDM_DPLL(-4,0));   //Get the correcting phase for HTSIG2

	// HT SIG two
	XSTATUS(CalcEVM_OFDM_DFT(RX.v+RX.FSOffset+320+16, FFT64));
	XSTATUS(CalcEVM_OFDM_PhaseCorr( -3, FFT64, SIG2));  //Phase corrected signal HTSIG2
    XSTATUS(CalcEVM_OFDM_Eq(64, SIG2, RX.H, E_SIG));
    
    XSTATUS(CalcEVM_OFDM_SoftDetector(1, E_SIG, RX.SNR_Legacy, pHTSIG+48,1));
    XSTATUS(CalcEVM_OFDM_Deintv(48, 1, pHTSIG+48, I_HTSIG+48));
	XSTATUS(CalcEVM_OFDM_HD_EVM(-3, 1, E_SIG, HD,1)); // HTSIG: output imaginary signal

    XSTATUS(CalcEVM_OFDM_PhaseDet(SIG2, HD, RX.H));
    XSTATUS(CalcEVM_OFDM_DPLL(-3,0));   //Get the correcting phase for Symbol 0

    XSTATUS(CalcEVM_OFDM_ViterbiDecoder(48, I_HTSIG, RX.bHTSIG, 6, 64));
    	
    b = RX.bHTSIG;
	RX.RATE.w7 = 0; // Reset the 

	for (i=0; i<7; i++)
		RX.RATE.word = RX.RATE.w7 | (b[i] << i);
  
    RX.LENGTH = 0x00;
    for (i=0; i<16; i++)
        RX.LENGTH = RX.LENGTH | (b[8+i] << i);
  
    if (InvalidRange(RX.LENGTH, 0, 65535)) 
    {
        RX.Fault = 1;
        return STATUS(WI_WARN_PHY_SIGNAL_LENGTH);
    }

    switch (RX.RATE.w7) // Only support MCS 0-7
    {
        case 0: RX.N_BPSC = 1; RX.N_DBPS =  26; break;
	    case 1: RX.N_BPSC = 2; RX.N_DBPS =  52; break;
		case 2: RX.N_BPSC = 2; RX.N_DBPS =  78; break;
		case 3: RX.N_BPSC = 4; RX.N_DBPS = 104; break;
		case 4: RX.N_BPSC = 4; RX.N_DBPS = 156; break;
		case 5: RX.N_BPSC = 6; RX.N_DBPS = 208; break;
		case 6: RX.N_BPSC = 6; RX.N_DBPS = 234; break;
		case 7: RX.N_BPSC = 6; RX.N_DBPS = 260; break;
        default: 
            RX.Fault = 1;
            return STATUS(WI_WARN_PHY_SIGNAL_RATE);
    }
   
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
			input=b[k] ^ c.b7;
            c.w8=(c.w7<<1) |(input &1);
            c.b1=c.b1^input;
            c.b2=c.b2^input;;
        }    
     
        for (z=0, k=0; k<8; k++)
		{
            z += b[41-k]^(~(c.word>>k) & 1);
		}

        if (z) 
        {	
            RX.Fault = 1;
            return STATUS(WI_WARN_PHY_SIGNAL_PARITY);
        }
    }
   
	// Unsupported packets (including reserved bits)
	if ( b[7] || !b[25] || !b[26] || b[28] || b[29] || b[30] || b[31] || b[32] || b[33]) 
	{
		RX.Fault = 1;
        return STATUS(WI_WARN_PHY_SIGNAL_PARITY);
	}
    
	// Update the N_SYM to be HT N_SYM
    RX.N_SYM = (16 + 8*(RX.LENGTH) + 6 + (RX.N_DBPS-1)) / RX.N_DBPS; 

    if ((RX.N_SYM-(int)(RX.Nv-RX.FSOffset-560)/80)>0)
    {
        RX.Fault = 1; 
        return STATUS(WI_WARN_PHY_SIGNAL_LENGTH);
    }
	
	PktDuration = (RX.N_SYM+2)*4;  // HT DATA symbol + HT-STF + HT-LTF 

	if (PktDuration > (RX.aPPDUMaxTimeH *256 + RX.aPPDUMaxTimeL))
	{
		RX.Fault = 1;
		return STATUS(WI_WARN_PHY_SIGNAL_LENGTH); 
	}

	if (!RX.Fault)
	{
		RX.N_CBPS = 52*RX.N_BPSC;		
		RX.MM_DATA = 1; 
	}

    // HT Preamble

	// Hold VCO during HT-STF
	XSTATUS(CalcEVM_OFDM_DPLL(-2,1));   //Get the correcting phase for Symbol 0
	
    // HT Channel estimation
	XSTATUS(CalcEVM_OFDM_ChanlEst(RX.v+RX.FSOffset+320+160+16, RX.H_HT, RX.SNR_HT, 1)); // Including phase correction for HT-LTF

	// Hold VCO during HT-STF
	XSTATUS(CalcEVM_OFDM_DPLL(-1,1));   //Get the correcting phase for Symbol 0

    //-------------------------------------------------------------
    // Get the CalcEVM_State EVM State pointer and do the initialization;
    //-------------------------------------------------------------
    {
        CalcEVM_State_t *pState = CalcEVM_State();
    
        pState->EVM.N_SYM = RX.N_SYM; // update to HT-SYM          
        for (i=0; i<64; i++) {            
            pState->EVM.h[i].Real = pState->EVM.h[i].Imag = 0.0;
        }  
        
        for (k=0; k<64; k++)
        {    
            pState->EVM.h[k].Real=RX.H_HT[k].Real;  // save channel response
            pState->EVM.h[k].Imag=RX.H_HT[k].Imag;
            pState->EVM.u[64+k].Real=SIG1[k].Real;
            pState->EVM.u[64+k].Imag=SIG1[k].Imag;
			pState->EVM.u[64*2+k].Real=SIG2[k].Real;
            pState->EVM.u[64*2+k].Imag=SIG2[k].Imag;
        }        
    }   
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_HTSIG_HTCE()


// ================================================================================================
// FUNCTION : CalcEVM_OFDM_RX_ResetPSDU()
// ------------------------------------------------------------------------------------------------
//   Purpose: Reset PSDU Data to be zero         
// ================================================================================================
wiStatus CalcEVM_OFDM_RX_ResetPSDU()
{
	if (!RX.PHY_RX_D) {
        WICALLOC( RX.PHY_RX_D, 65544, wiUWORD );
    }
    else memset( RX.PHY_RX_D, 0, 65544*sizeof(wiUWORD) );

    RX.LENGTH=0;
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_RX_ResetPSDU()

// ================================================================================================
// FUNCTION : CalcEVM_OFDM_RXWriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus CalcEVM_OFDM_RXWriteConfig(wiMessageFn MsgFunc)
{
    MsgFunc(" RX: DCX               : DCXSelect=%d   DCXShift=%d \n", RX.DCXSelect, RX.DCXShift);
    MsgFunc(" RX: Downmix           : %s\n",RX.DownmixOn ? "+10 MHz IF" : "ZeroIF");
    MsgFunc(" RX: SigDet            : SigDetFilter=%d  SigDetTh1=%d SigDetTh2f=%e \n", RX.SigDetFilter, RX.SigDetTh1, RX.SigDetTh2f);
    MsgFunc(" RX: FrameSync         : SyncEnabled=%d  SyncFilter=%d  PositionShift=%d  StartDelay=%d  PeakDelay=%d \n", RX.SyncEnabled,    RX.SyncFilter, RX.SyncShift, RX.SyncSTDelay,RX.SyncPKDelay);
    MsgFunc(" RX: CFO Est           : CFOEnabled=%d  CFOFilter=%d  StartDelay=%d \n", RX.CFOEnabled, RX.CFOFilter, RX.DelayCFO);
    MsgFunc(" RX: DPLL              : Ca=%d   Cb=%d  Sa=%d  Sb=%d \n", RX.Ca, RX.Cb, RX.Sa, RX.Sb);
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_WriteConfig()


//===========================================================================
//  Function  : CalcEVM_OFDM_RX_FreeMemory()                                  
//---------------------------------------------------------------------------
//  Purpose   : Free the state arrays                                         
//  Parameters: none                                                          
//===========================================================================
wiStatus CalcEVM_OFDM_RX_FreeMemory(void)
{
    WIFREE( RX.trace_r );
    WIFREE( RX.r );
    WIFREE( RX.v );
    WIFREE( RX.d );
    WIFREE( RX.de );
    WIFREE( RX.Lp );
    WIFREE( RX.Lc );
    WIFREE( RX.b );
    WIFREE( RX.a );
    WIFREE( RX.PHY_RX_D );
    WIFREE( RX.cCP );
    WIFREE( RX.cSP );
    WIFREE( RX.AdvDly );

    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_RX_FreeMemory


// ================================================================================================
// FUNCTION : CalcEVM_OFDM_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus CalcEVM_OFDM_ParseLine(wiString text)
{
    if (strstr(text, "CalcEVM_OFDM"))
    {
        #define PSTATUS(arg) if ((ParseStatus = WICHK(arg)) <= 0) return ParseStatus;
        wiStatus ParseStatus;

        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.PadFront",     &(TX.PadFront),         0, 10000));  //default:1000
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.PadBack",      &(TX.PadBack),         0, 10000));  //default:200
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.dumpTX",       &(TX.dumpTX),         0, 1));  
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM_TX.PRBS",      &(TX.Scrambler_Shift_Register),0, 511));  //default: 0x7F

        PSTATUS(wiParse_Boolean(text,"CalcEVM_OFDM.aModemTX",    &(RX.aModemTX) ));
        PSTATUS(wiParse_Boolean(text,"CalcEVM_OFDM.DownmixOn",   &(RX.DownmixOn)));
		
		PSTATUS(wiParse_Boolean(text,"CalcEVM_OFDM.HTMixedMode", &(RX.HT_MM) ));
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.DCXSelect",    &(RX.DCXSelect),      0, 1));  //default: 0
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.DCXShift",     &(RX.DCXShift),         0, 7));  //default: 2;
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.DCXHoldACC",   &(RX.DCXHoldAcc),      0, 1)); //default:0;
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.DCXFastLoad",  &(RX.DCXFastLoad),      0, 1)); //default:1
                    
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.SigDetTh1",    &(RX.SigDetTh1),      0,63));  //default:4
        PSTATUS(wiParse_Real (text, "CalcEVM_OFDM.SigDetTh2f",   &(RX.SigDetTh2f),   0.0,1.0)); // default:0.4 of signal power 
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.SigDetDelay",  &(RX.SigDetDelay),   0,255)); //default: 0xA2  162
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.SigDetFilter", &(RX.SigDetFilter),    0,1)); //default:0
 
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.SyncEnabled",  &(RX.SyncEnabled),       0,1)); //default: 1
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.SyncFilter",   &(RX.SyncFilter),       0,1)); //default: 1
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.SyncPKDelay",  &(RX.SyncPKDelay),      0,31)); //default: 0x10  16; 
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.SyncSTDelay",  &(RX.SyncSTDelay),     0,31));
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.SyncShift",    &(RX.SyncShift),      -30,15));
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.FSOffset",     &(RX.FSOffset),         -16384, 16383)); 
        PSTATUS(wiParse_Real (text, "CalcEVM_OFDM.PKThreshold",  &(RX.PKThreshold),    0.0, 1.0)); //default: 0.98

        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.CFOEnabled",   &(RX.CFOEnabled),      0, 1)); //default 1;
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.CFOFilter",    &(RX.CFOFilter),      0, 1)); //default 1;
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.DelayCFO",     &(RX.DelayCFO),         0,120)); //default 44
    
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.cSPEnabled",   &(RX.cSPEnabled),      0, 1)); //default 1;
        PSTATUS(wiParse_UInt (text, "CalcEVM_OFDM.cCPEnabled",   &(RX.cCPEnabled),      0, 1)); //default 1;
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.Ca",           &(RX.Ca),               0,3)); 
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.Cb",           &(RX.Cb),               0,7)); 
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.Sa",           &(RX.Sa),               0,3)); 
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.Sb",           &(RX.Sb),               0,1000)); 
        
        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.dumpRX",       &(RX.dumpRX),         0, 1));  
//        PSTATUS(wiParse_Int  (text, "CalcEVM_OFDM.Verbose",      &(Verbose),           0, 1));
    } 
    return WI_WARN_PARSE_FAILED;
}
// end of CalcEVM_OFDM_ParseLine()


// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_Init
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the CalcEVM_OFDM module
// Parameters: none
// ================================================================================================
wiStatus CalcEVM_OFDM_Init(void)
{
    double th2f = 0.4*pow(10.0,(12.0-30.0)/10.0); //Corresponding to signal power 12dBm;

    TX.Pilot_Shift_Register     = 0x7F;
    TX.Scrambler_Shift_Register = 0x7F;
    TX.M = 4;
    TX.PadFront=1000;
    TX.PadBack=200;
    TX.dumpTX=0;
    TX.aModemRX=1;
    TX.SERVICE=0;

    RX.Fault=0;
    RX.N_RX = 1;
    RX.M = 4;
    RX.HT_MM = 0; 
	RX.MM_DATA = 0; 
	RX.Rotated6Mbps = 0;
	RX.aModemTX= 0;           // Set aModemTX as 0 to use ideal constellation in demapping
    RX.DCXClear=0;            // Active when 1;
    RX.DCXSelect=0;           // Select the DCX output
    RX.DCXShift=2;            // Selects pole position of DCX HPF
    RX.DCXHoldAcc=0;          // Hold the accumulator value (no updates)
    RX.DCXFastLoad=1;         // No fast-charge of accumulators on ClearDCX
    
    RX.SigDetFilter=0;        // (SD)  Signal detector input filter
    RX.SigDetTh1=4;           // (SD)  Signal detector threshold #1
    RX.SigDetTh2f=th2f;       // (SD) The default receiver power is -50dBm, set the autocorrelation threshold correspondingly;
    RX.SigDetTh2H=0;          // (SD)  Signal detector threshold #2 (bits [15:8])
    RX.SigDetTh2L=17;         // (SD)  Signal detector threshold #2 (bits [ 7:0]
    RX.SigDetDelay=80;        // (SD)  Signal detector startup delay


    RX.SyncEnabled=1;         // (FS)  Frame Sync Enabled;
    RX.SyncShift=-6;          // (FS)  Negative delay for sync position
    RX.SyncSTDelay=16;        // (FS)  Delay from SGSigDet to Frame SyncStart;
    RX.SyncPKDelay=16;        // (FS)  Delay for peak detection (qualify) hold time
    RX.SyncFilter=1;          // (FS)  FrameSync filter
    RX.FSOffset=1000+7 +160+RX.SyncShift;  //  Front padding of 1000 (20MHz), transmit filter group delay 27, short preamble 160; This is the starting point of LPF;
    RX.PKThreshold=0.98;      // (FS)  Peak qualifier; At least one ratio in SyncPKDelay window must drop below PKThreshold ratio of peak for a valid peak
                             
    
    RX.CFOEnabled=1;          // (DFE) CFO Detection and Correction Enabled;
    RX.CFOFilter=1;           // (DFE) CFO measurement filter
    RX.DelayCFO=44;           // (DFE) Delay to load at least 1 short preamble symbol and match Mojave timing

    RX.cSPEnabled=1;          // (DPLL) Sampling phase correction enabled;
    RX.cCPEnabled=1;          // (DPLL) Carrier phase correction enabled;
    
	RX.aPPDUMaxTimeH =  38;   // aPPDUMaxTime = 256*aPPDUMaxTimeH + aPPDUMaxTimeL =9976 for this setting
    RX.aPPDUMaxTimeL = 248;   

    RX.dumpRX=0;              //  Disable dumpRX;
    RX.trace_Nr=0;

    RX.Sa=1;
    RX.Sb=4;
    RX.Ca=0;
    RX.Cb=2;
    
    XSTATUS(wiParse_AddMethod(CalcEVM_OFDM_ParseLine));
    XSTATUS(CalcEVM_OFDM_TX_FreeMemory());
    XSTATUS(CalcEVM_OFDM_RX_FreeMemory());

    return WI_SUCCESS;
} 
// end of CalcEVM_OFDM_Init()

// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close the CalcEVM_OFDM module
// Parameters: none
// ================================================================================================
wiStatus CalcEVM_OFDM_Close(void)
{  
    STATUS(wiParse_RemoveMethod(CalcEVM_OFDM_ParseLine));
    XSTATUS(CalcEVM_OFDM_TX_FreeMemory());
    XSTATUS(CalcEVM_OFDM_RX_FreeMemory());
        
    return WI_SUCCESS;   
}
// end of CalcEVM_OFDM_Close()

// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_RxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
CalcEVM_OFDM_RxState_t *CalcEVM_OFDM_RxState(void)
{  
    return &RX;
}
// end of CalcEVM_OFDM_RxState()

// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_TxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
CalcEVM_OFDM_TxState_t *CalcEVM_OFDM_TxState(void)
{  
    return &TX;
}
// end of CalcEVM_OFDM_TxState()

// ================================================================================================
// FUNCTION  : CalcEVM_OFDM_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level floating point receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
wiStatus CalcEVM_OFDM_RX(int *pLength, wiUWORD *pData[], int Nr, wiComplex *R[])
{   
    int i,k;
    int N_bits;  //Number of data bits;
    int win_ofs=0; //Advdly of FFT window        

    CalcEVM_State_t *pState = CalcEVM_State();

    if (InvalidRange(Nr, 400, 1<<28)) return STATUS(WI_ERROR_PARAMETER3);
    if (!R   ) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[0]) return STATUS(WI_ERROR_PARAMETER4);
    
    RX.Fault = 0;
	RX.MM_DATA = 0;
	RX.Rotated6Mbps = 0; 

    //-------------------------------------------------------------
    // Reset PSDU
    //-------------------------------------------------------------   
    XSTATUS(CalcEVM_OFDM_RX_ResetPSDU()); 

    // -----------------------------------------------
    // Return PSDU and Length (drop 8 byte PHY header)
    // Especially for Fault case;
    // -----------------------------------------------
    if (pLength) *pLength = RX.LENGTH;
    if (pData)   *pData    = RX.PHY_RX_D + 8;

    //--------------------------------------------------------------
    // Get the local receive arrays
    //-------------------------------------------------------------   
    RX.Nr = Nr + 100; 
    RX.Nv = (int)floor(RX.Nr/4.0); 

    WIFREE  ( RX.r );
    WICALLOC( RX.r, RX.Nr, wiComplex );

    WIFREE  ( RX.v );
    WICALLOC( RX.v, RX.Nv, wiComplex );

    //-------------------------------------------------------------
    // Get the local receive arrays
    //-------------------------------------------------------------   
    for (i=0; i<Nr; i++)
        RX.r[i] = R[0][i];
      
    //-------------------------------------------------------------
    // DC Offset Cancellation
    //-------------------------------------------------------------   
    XSTATUS(CalcEVM_OFDM_DCX(RX.r,RX.Nr));
    
    // Downmixer
    //
    if (RX.DownmixOn) XSTATUS( CalcEVM_OFDM_Downmix(RX.r, RX.Nr) );

    //-------------------------------------------------------------
    // Channel Selection / Anti-Aliasing Filter
    //-------------------------------------------------------------
    XSTATUS(CalcEVM_OFDM_DownsamFIR(RX.r,RX.Nr));

    //-------------------------------------------------------------
    // Decimation by factor of 4 to 20MHz;
    //-------------------------------------------------------------
    for (i=0; (i<RX.Nv); i++)
    {
        RX.v[i].Real=RX.r[4*i].Real;
        RX.v[i].Imag=RX.r[4*i].Imag;
    }
    WIFREE( RX.r );

    // ------------------------------------------------------------
    // ------- Dump the signal before baseband processing ---------
    // ------------------------------------------------------------
    if (RX.trace_r && RX.dumpRX)
        dump_wiComplex("r.out", RX.trace_r, RX.trace_Nr);

    //-------------------------------------------------------------
    // Begin Baseband Processing
    //-------------------------------------------------------------
    //-------------------------------------------------------------
    // Signal Detect
    //-------------------------------------------------------------
    XSTATUS( CalcEVM_OFDM_SigDet(RX.v, RX.Nv, &RX.SGSigDet));

    if (RX.Fault)
        return WI_WARN_PHY_BAD_PACKET;
  
    //-------------------------------------------------------------
    // Frame Sync
    //-------------------------------------------------------------
    if (RX.SyncEnabled)
        XSTATUS( CalcEVM_OFDM_FrameSync(RX.SGSigDet+RX.SyncSTDelay, RX.v, RX.Nv, &RX.FSOffset));
 
    if (RX.Fault)
        return WI_WARN_PHY_BAD_PACKET;

    //-------------------------------------------------------------
    // CFO Estimation and Correction
    //-------------------------------------------------------------
    if (RX.CFOEnabled)
        XSTATUS( CalcEVM_OFDM_CFO(RX.v, RX.Nv, RX.SGSigDet+RX.DelayCFO, RX.FSOffset));

    //-------------------------------------------------------------
    // Channel Estimation
    //-------------------------------------------------------------
    XSTATUS(CalcEVM_OFDM_ChanlEst(RX.v+RX.FSOffset+32, RX.H, RX.SNR_Legacy, 0));

    //-------------------------------------------------------------
    // Decode the SIG Field and Initialze CalcEVM_State EVM
    //-------------------------------------------------------------
    XSTATUS( CalcEVM_OFDM_SIG() );
	
    if (RX.Fault) return WI_WARN_PHY_BAD_PACKET;

	//-------------------------------------------------------------------
	// For HT-MM packets, Decode HT-SIG and Channel Estimation on HT-LTF1 
    //-------------------------------------------------------------------
	if (RX.HT_MM) // Prior knowledge that it is an HT-MM packet
	{
		XSTATUS(CalcEVM_OFDM_HTSIG_HTCE());
	}
	else if ( RX.RATE.w4 == 0xD) // Rotation check on symbol following L-SIG
	{
		XSTATUS(CalcEVM_OFDM_CheckRotate());
		
		if (RX.Rotated6Mbps)
		{
			XSTATUS(CalcEVM_OFDM_HTSIG_HTCE());		
		}
    }
	if (RX.Fault)
        return WI_WARN_PHY_BAD_PACKET;

    // Initialize the Frequency OFDM signals (before and after Eq)
    // Initialize the LLR metric (interleaved and deinterleaved)
    // Initialize the bit sequence(scrambled and descrambled)
	// N_SYM is # of DATA symbols for legacy packets and # of HT DATA
	// symbols for HT mixed mode packets
    //-------------------------------------------------------------
    {   
        WIFREE( RX.d  );
        WIFREE( RX.de );
        WIFREE( RX.Lc );
        WIFREE( RX.Lp );
        WIFREE( RX.b  );
        WIFREE( RX.a  );

        RX.Nd  =        64*RX.N_SYM;
        RX.N_de=        64*RX.N_SYM;
        RX.Np  = RX.N_CBPS*RX.N_SYM;
        RX.Nc  = RX.N_CBPS*RX.N_SYM;
        RX.Nb  = RX.N_DBPS*RX.N_SYM;
        RX.Na  = RX.N_DBPS*RX.N_SYM;

        WICALLOC( RX.d, RX.Nd, wiComplex );
        WICALLOC( RX.de, RX.N_de, wiComplex );
        WICALLOC( RX.Lp, RX.Np, wiReal );
        WICALLOC( RX.Lc, RX.Nc, wiReal );
        WICALLOC( RX.b,  RX.Nb, int );
        WICALLOC( RX.a,  RX.Na, int );
    }

    //-------------------------------------------------------------
    // //Processing the DATA field
    //-------------------------------------------------------------
    {   
        int i,k;
        int rate_12;  //code rate *12, and length of PSDU;
        wiComplex FFT64[64], HD[64]; //FFT64: frequency domain signal before phase correction; HD64: hard decision;
        int header_ofs = RX.MM_DATA ? 560 : 240; // 7 symbols for HT MM, 3 for legacy
		int sym_ofs = RX.MM_DATA? 3:1; // offset for phase corrected and equalized signal
		int DPLL_ofs = RX.MM_DATA?4:0; // offset for AdvDly, cCP, cSP
		wiComplex *H = RX.MM_DATA ? RX.H_HT : RX.H; 
		wiReal *S_N = RX.MM_DATA ? RX.SNR_HT: RX.SNR_Legacy; 
		
        for (i=0; i<RX.N_SYM; i++)
        {
            win_ofs+=RX.AdvDly[i+DPLL_ofs];

            XSTATUS(CalcEVM_OFDM_DFT(RX.v+RX.FSOffset+header_ofs+80*i+16+win_ofs, FFT64));
			XSTATUS(CalcEVM_OFDM_PhaseCorr(i, FFT64, RX.d+64*i));  //Phase corrected signal;

            XSTATUS(CalcEVM_OFDM_Eq(64, RX.d+64*i, H, RX.de+64*i));  //Frequency domain equlization;
                                                                    
            XSTATUS(CalcEVM_OFDM_HD_EVM(i, RX.N_BPSC, RX.de+64*i, HD,0));  //Hard decision and EVM Calculation;

            XSTATUS(CalcEVM_OFDM_PhaseDet(RX.d+64*i, HD, H));
            XSTATUS(CalcEVM_OFDM_DPLL(i,0));

            for (k=0; k<64; k++)  //Save the frequency domain OFDM signal to EVM State
            {
                pState->EVM.u[64*(i+sym_ofs)+k].Real = RX.d[64*i+k].Real; //phase corrected 
                pState->EVM.u[64*(i+sym_ofs)+k].Imag = RX.d[64*i+k].Imag;
            }
            
            XSTATUS(CalcEVM_OFDM_SoftDetector(RX.N_BPSC, RX.de+64*i, S_N, RX.Lp+i*RX.N_CBPS,0));  
            XSTATUS(CalcEVM_OFDM_Deintv(RX.N_CBPS, RX.N_BPSC, RX.Lp+i*RX.N_CBPS, RX.Lc+i*RX.N_CBPS));
        }
        pState->EVM.EVM = sqrt(pState->EVM.Se2/(RX.MM_DATA? 52*RX.N_SYM : 48*(RX.N_SYM)));  
        pState->EVM.EVMdB = 20.0*log10(pState->EVM.EVM);
	
        {  // Calculate Weighted EVM
            int k;
			const int *subcarrier_type = RX.MM_DATA? subcarrier_type_n : subcarrier_type_a; 
            wiReal Acc_SNR=0, Acc_EVM=0 ;

            for (k=0; k<64; k++)
            {
                if (subcarrier_type[k] == 1)
				{
					Acc_SNR+=S_N[k];
					Acc_EVM+=S_N[k]*pState->EVM.cSe2[k];   
				}
            }   
            pState->EVM.W_EVMdB=10*log10(Acc_EVM/Acc_SNR/ RX.N_SYM);
        }
		
        rate_12=RX.N_DBPS/(4*RX.N_BPSC);
        N_bits=16+8*RX.LENGTH+6;		
        XSTATUS(CalcEVM_OFDM_ViterbiDecoder(N_bits, RX.Lc, RX.b, rate_12, 64));
        XSTATUS(CalcEVM_OFDM_Descrambler(N_bits, RX.b, RX.a));		
    }
    
    // ------------------------------------------------------------
    // ------- Obtain SERVICE and PSDU DATA -----------------------
    // ------------------------------------------------------------    
	RX.SERVICE = 0;
    for (i=0; i<16; i++)
        RX.SERVICE = RX.SERVICE|(RX.a[i]<<i); 

    RX.N_PHY_RX_WR = RX.LENGTH + 8;             // Number of PHY_RX_D writes
    for (i=0; i<RX.LENGTH; i++)
    { 
        RX.PHY_RX_D[i+8].word=0;
        for (k=0; k<8; k++)
            RX.PHY_RX_D[i+8].word=RX.PHY_RX_D[i+8].word|(RX.a[16+i*8+k]<<k);
    }
    if (pLength) *pLength = RX.LENGTH;
    
    // ------------------------------------------------------------
    // ------- Dump the signal for Good packets ---------
    // ------------------------------------------------------------  
    if (RX.trace_r && RX.dumpRX)
        dump_wiComplex("r.out", RX.trace_r, RX.trace_Nr);
    
    return WI_SUCCESS;
}
// end of CalcEVM_OFDM_RX

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// 2007-01-05 X.Yu: Modified code to include SIG in EVM.x and EVM.u. Size 64*(N_SYM+1) 
// 2007-07-17 X.Yu: Added parser and initialization for the new RX parameter PKThreshold. 
//                  The current default value 0.98 is set based on two sample packets (0.848~0.997)
//                  Added codes in Framesync to better qualify a valid peak and get rid of the bouncing effect due to random noise       
// 2007-07-17 X.Yu: Added PKThreshold in CalcEVM_OFDM_RX_State to better qualify a valid peak. 
//                  At least one ratio within SyncPKDelay window should drop below PKThreshold percentage of the peak
// 2007-12-03 Brickner: Added +10 MHz downmix option to the receiver (used to emulate MojaveBAR Test 13)
//
// Revised 2008-02-28, Brickner
//  - Increased max range on DelayCFO from 63 to 120
//  - Increased default DelayCFO from 20 to 44
//  - Removed call to CalcEVM_OFDM_RX_FreeMemory() at the end of CalcEVM_OFDM_RX. Memory is free'd
//    at the start of the function and when the PHY is unloaded via wiPHY so no memory leak occurs.
//    Keeping the values allows them to be retrieved from the structure for debug
// Revised 2008-08-03 Brickner: Added energy from center subcarrier to channel estimate for leakage check
// Revised 2008-08-22 Brickner: Added option to change RX.aModemTX
//
// Revised 2009-05-20, X.Yu
//  - Format change of arrays and functions. Most array size is changed from 52 to 64 to save all subcarriers
//  - Added EVM calculation for HT mixed mode signal. If HT_MM is set, the receiver will assume it is a HT MM packets. It will do HT-SIG
//    decoding. If HT_MM is not set, the receiver will check the constellation rotation of the symbol following L-SIG. If it is rotated 6Mbps,
//    the receiver will do mixed mode packet processing.
//
// Revised 2009-05-28 Brickner: Modifed CalcEVM_OFDM_DownsamFIR to use a filter with +/-0.2 dB passband ripple   
// Revised 2009-06-03 Brickner: Changed default SyncShift value from -4 to -6
// Revised 2010-03-30 Brickner: Removed unused variable pilot_index from CalcEVM_OFDM_HD_EVM
// Revised 2010-07-31 Brickner: Changed functions used to return state pointers
// Revised 2010-12-01 Brickner: Change for consistency with WiSE 4.X; general code cleanup

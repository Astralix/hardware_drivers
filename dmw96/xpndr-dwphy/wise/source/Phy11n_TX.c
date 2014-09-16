//--< Phy11n_TX.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Phy11n_TX - Phy11n Transmit Model
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "Phy11n.h"
#include "wiMath.h"
#include "wiUtil.h"

#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg) < 0) return WI_ERROR_RETURNED;

wiStatus Phy11n_SymbolCycleShift(wiComplex *in_sp, int nshift, int sizev);
wiStatus Phy11n_ScalingTXSig(wiComplex *x, wiReal sc, int N);
wiStatus Phy11n_TX_InsertSignal(wiComplex *x, wiComplex *y, int Ny);
wiStatus Phy11n_InsertSymbol(wiComplex *d, wiComplex *x, wiReal *pilot, int nCS, int Nsd);

static const int NonHTCSD4[4][4]={{0, 0, 0, 0},{0, -4, 0, 0},{0,-2,-4, 0},{0,-1,-2, -3}}; //Cyclic shift sample for NON_HT; 1 sample = 50ns 
static const int    HTCSD4[4][4]={{0, 0, 0, 0},{0, -8, 0, 0},{0,-8,-4, 0},{0,-8,-4,-12}}; //Cyclic shift sample for HT; 1 sample = 50ns

static const int NumDataSubcarriers = 52;

// ================================================================================================
// FUNCTION  : Phy11n_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level PHY routine
// Parameters: bps      -- PHY transmission rate (bits/second)
//             Length   -- Number of bytes in "data"
//             data     -- Data to be transmitted
//             Ns       -- Returned number of transmit samples
//             S        -- Array of transmit signals (by ref)
//             Fs       -- Sample rate for S (Hz)
//             Prms     -- Complex power in S (rms)
// ================================================================================================
wiStatus Phy11n_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms)
{
    int i, n, N, mSTBC;
    Phy11n_TxState_t *pTX = Phy11n_TxState();
    wiInt *a, *b, *c;
    wiReal pilot[4][4];

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!data) return STATUS(WI_ERROR_PARAMETER3);
    if (!Ns)   return STATUS(WI_ERROR_PARAMETER4);
    if (!S)    return STATUS(WI_ERROR_PARAMETER5);
    
    XSTATUS( Phy11n_TX_Init() );
	
    // -------------------------------------------------
    // Generate the Bermai PHY control block (MAC Level)
    // -------------------------------------------------
    if (InvalidRange(Length,1,65535)) return STATUS(WI_ERROR_PARAMETER2);

    // -----------------------------
    // Load Parameters into TX State
    // -----------------------------
    pTX->HT_LENGTH = Length;

    // ------------------------------------------------------
    // If scrambler state is 0, force to a valid value (0x7F)
    // ------------------------------------------------------
    if (!pTX->ScramblerShiftRegister) pTX->ScramblerShiftRegister = 0x7F;

    // -----------------------------------------------------------------------------------
    // Compute the number of bits in the DATA field. The data field contains
    // 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits*pTX->N_ES, and enough pad bits
    // to fill a whole number of OFDM symbols or STBC frames, each containing N_DBPS bits.
    // -----------------------------------------------------------------------------------
    mSTBC = pTX->STBC? 2:1;
    pTX->N_SYM = mSTBC*((16 + 8*Length + 6*pTX->N_ES + (mSTBC*pTX->N_DBPS-1)) / (mSTBC*pTX->N_DBPS)); 
    N = pTX->N_DBPS * pTX->N_SYM;

    XSTATUS( Phy11n_TX_AllocateMemory() );
    GET_WIARRAY( a, pTX->a, pTX->N_SYM * pTX->N_DBPS, wiInt );
    GET_WIARRAY( b, pTX->b, pTX->N_SYM * pTX->N_DBPS, wiInt );
    GET_WIARRAY( c, pTX->c, pTX->N_SYM * pTX->N_CBPS, wiInt );

    XSTATUS( Phy11n_TX_InsertLegacyPreamble(pTX->aPadFront) );
    XSTATUS( Phy11n_TX_InsertLSIG(pTX->aPadFront) );
    XSTATUS( Phy11n_TX_InsertHTSIG(pTX->aPadFront) );
    XSTATUS( Phy11n_TX_InsertHTPreamble(pTX->aPadFront) );

    // --------------------------------
    // Create the DATA field bit stream
    // --------------------------------
    for (n=0; n<16; n++)               a[n] = 0;                           // SERVICE
    for (i=0; i<8*Length; i++, n++)    a[n] = (data[i/8].w8 >> (i%8)) & 1; // PSDU
    for (i=0; i<6*pTX->N_ES; i++, n++) a[n] = 0;                           // Tail
    for (; n<N; n++)                   a[n] = 0;                           // pad bits

    XSTATUS( Phy11n_TX_Scrambler(pTX->a.Length, a, b) ); // a input

    // ---------------------------------------------------------------------
    // Reset the tail bits to zero. This is required because these bits will
    // be used to terminate the trellis when decoding the convolutional code
    // ---------------------------------------------------------------------
    for (i = 16+8*Length; i<22+8*Length; i++) b[i] = 0;
    
    XSTATUS( Phy11n_TX_Encoder(pTX->b.Length, b, c, pTX->CodeRate12) );
    XSTATUS( Phy11n_TX_StreamParser(pTX->c.Length, c, pTX->stsp, pTX->N_BPSC, pTX->N_SS) );

    for (i=0; i<pTX->N_SS; i++)
        XSTATUS( Phy11n_TX_Interleave(pTX->N_CBPSS[i], pTX->N_BPSC[i], i ,pTX->stsp[i], pTX->stit[i], pTX->Nsp[i]) );

    for (i=0; i<pTX->N_SS; i++)
        XSTATUS( Phy11n_TX_QAM(pTX->stmp[i], pTX->stit[i], pTX->N_BPSC[i], pTX->Nmp, 0) );

    XSTATUS( Phy11n_TX_STBC(pTX->stSTBC, pTX->N_STS, pTX->stmp, pTX->N_SS, pTX->Nmp) );
    XSTATUS( Phy11n_TX_SpatialMap(pTX->stSM, pTX->N_TX, pTX->stSTBC, pTX->N_STS, pTX->Nmp) );

    // ----------------------------------
    // Create the DATA field OFDM symbols
    // ----------------------------------
    for (n=0; n<pTX->N_SYM; n++)
    {
        XSTATUS( Phy11n_PilotTonePhase(pilot, pTX->N_STS, n, PHY11N_SYMBOL_IS_HT_OFDM) );

        for (i=0; i<pTX->N_STS; i++) 
        {
            XSTATUS( Phy11n_InsertSymbol( pTX->stSM[i] + n * NumDataSubcarriers, 
                                          pTX->x[i] + 80*n + 640 + (80*pTX->N_HT_DLTF) + pTX->aPadFront, 
                                          pilot[i], 
                                          HTCSD4[pTX->N_STS-1][i], 
                                          NumDataSubcarriers) ); 
        }
    }

    // -----------------
    // Return Parameters
    // -----------------
    if (S)  *S  = pTX->x;                   // point to array of pointers to transmit vectors
    if (Ns) *Ns = pTX->Nx;                  // length of transmit signal vectors
    if (Fs) *Fs = 20.0e6;                   // 20MHz sample rate
    if (Prms) *Prms = 56.0/64.0/56.0/64.0;  // unit complex power
                                            // 56.0/64.0: only 56 subcarriers are used
                                            // /56.0: data subcarriers are divided by the number of tones
                                            // /64.0: in TX_IFFT, the factor 64 is divided
    if (pTX->dumpTX)
    {
        dump_wiComplex("x0.txt",pTX->x[0],pTX->Nx);
        exit(1);
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX()

// ================================================================================================
// FUNCTION  : Phy11n_TX_InsertLegacyPreamble()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_TX_InsertLegacyPreamble(int dt)
{
    const int S[] = {0,0,0,0,0,0,0,0,1,0,0,0,-1,0,0,0,1,0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,0,
                     0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0};
        
    const int L[] = {0,0,0,0,0,0,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,0,
                     1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,0,0,0,0,0};

    wiReal a = sqrt(1.0/2.0);
    Phy11n_TxState_t *pTX = Phy11n_TxState();
    wiComplex Y[64], y[160], y1[64];
    int i, k;

    //  Clear the first sample. This is necessary because the signal is added with a window that
    //  includes the existing value of the first sample. Subsequent symbols are okay because the
    //  sample is initialized by the previous symbol.
    //  
    for (i=0; i<pTX->N_TX; i++)
        pTX->x[i][0] = wiComplexZero;
        
    // L-STF: Legacy Short Training Field ///////////////////////////////////////////////
    //
    for (k=0; k<64; k++) 
        Y[k].Real = Y[k].Imag = a * S[k];
        
    Phy11n_TX_IDFT(Y, y+96);
        
    for (k=0; k<96; k++) 
        y[k] = y[k%64 + 96];

    XSTATUS( Phy11n_SymbolCycleShift(y, NonHTCSD4[pTX->N_TX-1][0], 160) );  //Cyclic shift 
    XSTATUS( Phy11n_ScalingTXSig(y, 1.0/sqrt(pTX->N_TX*12.0), 160) ); //N L_STF=12, P. 178
    XSTATUS( Phy11n_TX_InsertSignal(pTX->x[0]+dt, y, 160) );      

    for (i=1; i<pTX->N_TX; i++)
    {  //Save Non HT-STF for 4 streams         
        XSTATUS( Phy11n_SymbolCycleShift(y, NonHTCSD4[pTX->N_TX-1][i]-NonHTCSD4[pTX->N_TX-1][i-1], 160) );  //Cyclic shift 
        XSTATUS( Phy11n_TX_InsertSignal(pTX->x[i]+dt, y, 160) );
    }

    // L-LTF: Legacy Long Training Field ////////////////////////////////////////////////
    //
    for (k=0; k<64; k++) 
    {
        Y[k].Real = (wiReal)L[k];
        Y[k].Imag = 0.0;
    }
    Phy11n_TX_IDFT(Y, y+96);
    for (k=0; k<64; k++) y1[k] = y[k+96];

    for (i=0; i<pTX->N_TX; i++)
    {
        XSTATUS( Phy11n_SymbolCycleShift(y + 96, NonHTCSD4[pTX->N_TX-1][i], 64) );
        
        for (k=0; k<64; k++) y[k+32] = y[k+ 96];
        for (k=0; k<32; k++) y[k]    = y[k+128];

        Phy11n_ScalingTXSig(y, 1.0/sqrt(52.0*pTX->N_TX), 160); // 52: number of L-LTF tones
        Phy11n_TX_InsertSignal(pTX->x[i] + dt + 160, y, 160);
            
        for (k=0; k<64; k++) 
            y[k+96] = y1[k];
    }
    return WI_SUCCESS;
}
// end of Phy11n_InsertLegacyPreamble()

// ================================================================================================
// FUNCTION  : Phy11n_TX_InsertLSIG()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_TX_InsertLSIG(int dt)
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();

    const int RATE = 0xD; // 6 Mbps
	int b[24], c[48], p[48], i, L_LENGTH;
    wiReal pilot[4];
    wiComplex d[48];

	for (i=0; i<4; i++)
		b[i] = (RATE >> (3-i)) & 1;// RATE
		
	b[4] = 0;                      // Reserved (0)
		
	L_LENGTH = 3*(pTX->N_SYM + 1 + pTX->N_HT_DLTF + pTX->N_HT_ELTF + 2) - 3; 
    if (L_LENGTH > 4095) L_LENGTH = 4095;

	for (i=0; i<12; i++)           // LENGTH
		b[5+i] = (L_LENGTH>>i) & 1;

	b[17] = 0;                     // Parity
	for (i=0; i<=16; i++)          // Even parity by summing mod 2 the
		b[17] = b[17] ^ b[i];      // ...previous bits
		
	for (i=18; i<24; i++)          // SIGNAL TAIL
		b[i] = 0;
		
	Phy11n_TX_Encoder         (24, b, c, 6);    // Rate 1/2 convolutional code
	Phy11n_TX_Interleave_NonHT(48, 1, c, p);    // Interleaver
	Phy11n_TX_QAM             (d, p, 1, 48, 0); // BPSK subcarrier modulalation  
		
    XSTATUS( Phy11n_PilotTonePhase(&pilot, 1, 1, PHY11N_SYMBOL_IS_LSIG) );

	for (i=0; i<pTX->N_TX; i++) 
    {
		//Number of total subcarriers = 52, data_subcarrier=48 in Non_HT mode
		XSTATUS( Phy11n_InsertSymbol(d,pTX->x[i]+dt+320,pilot,NonHTCSD4[pTX->N_TX-1][i],48) ); 
	}
    return WI_SUCCESS;
}
// end of Phy11n_InsertLSIG()
    
    
// ================================================================================================
// FUNCTION  : Phy11n_TX_InsertHTSIG()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_TX_InsertHTSIG(int dt)
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();

	int b[48], c[96], p[48], i, j;
    unsigned int input; // shift register input, need to be unsigned;
    wiUWORD X = {0xFF}; // CRC Shift Register Coeffient; 
    wiReal pilot[4];
    wiComplex d[96];
				
	// Create the bit field
	// --------------------
	for (i=0; i<7; i++)
		b[i] = (pTX->MCS >> i) & 1;  // RATE

	b[7] = 0;                        // BW40/20
		
	for (i=0; i<16; i++)             // LENGTH
		b[8+i] = (pTX->HT_LENGTH>>i) & 1;
		
	b[24] = 0;                       // Smoothing
	b[25] = 1;                       // Not Sounding   
	b[26] = 1;                       // Reserved One   
	b[27] = 0;                       // Aggregation
	b[28] =  pTX->STBC & 1;          // STBC      
	b[29] = (pTX->STBC>>1) & 1;                 
	b[30] = 0;						 // ADV Coding
	b[31] = 0;                       // Short GI
	b[32] = (pTX->N_HT_ELTF>>0) & 1; // # of extension spatial streams
	b[33] = (pTX->N_HT_ELTF>>1) & 1;

	for (i=0; i<34; i++)             // CRC
	{       
		input = b[i] ^ X.b7;
		X.w8 = (X.w7<<1) | (input & 1);
		X.b1 = X.b1 ^ input;
		X.b2 = X.b2 ^ input;
	}    
	b[34] = !X.b7;
	b[35] = !X.b6;   
	b[36] = !X.b5;
	b[37] = !X.b4;
	b[38] = !X.b3;
	b[39] = !X.b2;
	b[40] = !X.b1;
	b[41] = !X.b0;

    for (i=42; i<48; i++) b[i] = 0;  // SIGNAL TAIL
		
	Phy11n_TX_Encoder(48, b, c, 6);  // Rate 1/2 convolutional code
		
	for (j=0; j<2; j++)
	{
		XSTATUS( Phy11n_TX_Interleave_NonHT(48, 1, c+(48*j), p) );   // Interleaver
		XSTATUS( Phy11n_TX_QAM             (d+48*j, p,1,48, 1) );    // BPSK subcarrier modulalation  
		XSTATUS( Phy11n_PilotTonePhase     (&pilot, 1, 1, PHY11N_SYMBOL_IS_HTSIG) );

        for (i=0; i<pTX->N_TX; i++)
		{
			XSTATUS( Phy11n_InsertSymbol(d+48*j, pTX->x[i]+dt+400+(80*j), pilot, NonHTCSD4[pTX->N_TX-1][i], 48) );
		}
	}
    return WI_SUCCESS;
}
// end of Phy11n_InsertHTSIG()
    
// ================================================================================================
// FUNCTION  : Phy11n_TX_InsertHTPreamble()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_TX_InsertHTPreamble(int dt)
{
    const int HTS[] = {0,0,0,0,0,0,0,0,1,0,0,0,-1,0,0,0,1,0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,0,
                       0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0};

    const int HTL[] = {0,0,0,0,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,0,
                       1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,-1,-1,0,0,0};
        
    const int Phl[4][4]={{1, -1, 1, 1},{1, 1, -1, 1},{1, 1, 1, -1},{-1, 1,1,1}};

    wiComplex Y[4][64], y[80];
    int i, j, k;

    wiReal  a = sqrt(1.0/2.0);
    Phy11n_TxState_t *pTX = Phy11n_TxState();

    // Generate and Insert the HT-STF Symbol ////////////////////////////////////////////
    //
    for (k=0; k<64; k++)
        Y[0][k].Real = Y[0][k].Imag = a * HTS[k];
        
    Phy11n_TX_IDFT(Y[0], y+16);
        
    XSTATUS( Phy11n_SymbolCycleShift(y+16, HTCSD4[pTX->N_TX-1][0], 64) );
    for (k=0; k<16; k++)
        y[k] = y[k+16];

    Phy11n_ScalingTXSig(y, 1.0/sqrt(pTX->N_STS*12.0), 80); // 51: number of HT-LTF tones
    Phy11n_TX_InsertSignal(pTX->x[0]+dt+560, y, 80);
    for (i=1; i<pTX->N_TX; i++)
    {
        XSTATUS( Phy11n_SymbolCycleShift(y+16, HTCSD4[pTX->N_TX-1][i]-HTCSD4[pTX->N_TX-1][i-1], 64) );
        for (k=0; k<16; k++)
            y[k] = y[k+16];
        Phy11n_TX_InsertSignal(pTX->x[i]+dt+560, y, 80);
    }      
    
    // Generate and Insert the HT-LTF Symbols ///////////////////////////////////////////
    //
    for (j=0; j<pTX->N_HT_DLTF; j++)
    {
        for (i=0; i<pTX->N_TX; i++)
        {  
            for (k=0; k<64; k++) 
            {
                Y[i][k].Real = (wiReal)HTL[k]*Phl[i][j];
                Y[i][k].Imag = 0.0;                     
            }
        }
        for (i=0; i<pTX->N_TX; i++)
        {  
            XSTATUS( Phy11n_TX_IDFT(Y[i], y+16) );
            XSTATUS( Phy11n_SymbolCycleShift(y+16, HTCSD4[pTX->N_TX-1][i], 64) );
            for (k=0; k<16; k++) 
                y[k] = y[k+64];
            Phy11n_ScalingTXSig(y, 1.0/sqrt((double)pTX->N_STS*NumDataSubcarriers), 80);
            Phy11n_TX_InsertSignal(pTX->x[i]+dt+640+(80*j),y, 80);
        }      
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_InsertHTPreamble()

// ================================================================================================
// FUNCTION  : Phy11n_TX_InsertSignal()
// ------------------------------------------------------------------------------------------------
// Purpose   : Insert a signal segement into the baseband signal after applying a time-domain
//             windows.
// Parameters: x  -- Output array
//             y  -- Input array
//             Ny -- Number of points in y
// ================================================================================================
wiStatus Phy11n_TX_InsertSignal(wiComplex *x, wiComplex *y, int Ny)
{
    int k;

    x[0].Real += 0.5 * y[0].Real;
    x[0].Imag += 0.5 * y[0].Imag;

    for (k=1; k<Ny; k++)
        x[k] = y[k];

    x[Ny].Real = 0.5 * y[Ny-64].Real;
    x[Ny].Imag = 0.5 * y[Ny-64].Imag;

    return WI_SUCCESS;
}
// end of Phy11n_TX_InsertSignal()


// ================================================================================================
// FUNCTION  : Phy11n_TX_Interleave()
// ------------------------------------------------------------------------------------------------
// Purpose   : interleaver for 20MHz
// Parameters: x: input, y: output
// ================================================================================================
wiStatus Phy11n_TX_Interleave(int N_CBPSS, int N_BPSC, int Iss ,int x[], int y[], int Nsp)
{
    int Ncol = 13;
    int Nrot = 11;   
    int Nrow = 4*N_BPSC;

    int i, j, r, k, m;
    int s = max(N_BPSC/2, 1);
        
    Phy11n_TxState_t *pTX = Phy11n_TxState();
        
    if (pTX->InterleaverDisabled)
    {
        memcpy(y, x, Nsp*sizeof(int));
    }
    else
    {
        for (m=0; m<Nsp/N_CBPSS; m++)
        {
            for (k=0; k<N_CBPSS; k++)
            {
                i = Nrow * (k%Ncol) + (k/Ncol);
                j = s * (i/s) + ((i + N_CBPSS - (Ncol * i/N_CBPSS)) % s);
                r = (N_CBPSS+(j-((Iss*2)%3+3*(Iss/3))*Nrot*N_BPSC))%N_CBPSS;
                y[m*N_CBPSS+r] = x[m*N_CBPSS+k];
            }
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_Interleave()

// ================================================================================================
// FUNCTION  : Phy11n_TX_StreamParser()
// ------------------------------------------------------------------------------------------------
// Purpose   : Stream Parser
// Parameters: pState -- Pointer to the state structure (by reference)
// ================================================================================================
wiStatus Phy11n_TX_StreamParser(int Nc, int c[], int *stsp[], int N_BPSC[], int N_SS)
{
    int s[4], s_sum;
    int i, j, k, pt;

    s_sum = 0;
    for (i=0; i<N_SS; i++)
    {
       s[i] = max(1, N_BPSC[i]/2);
       s_sum += s[i];
    }

    pt = 0;
    for (k=0; k<Nc/s_sum; k++)
       for (i=0; i<N_SS; i++)
          for (j=0; j<s[i]; j++)
             stsp[i][k*s[i]+j] = c[pt++];

    return WI_SUCCESS;
 }
// end of Phy11n_TX_StreamParser()

// ================================================================================================
// FUNCTION  : Phy11n_TX_Scrambler()
// ------------------------------------------------------------------------------------------------
// Purpose   : Scramble the input sequence using PRBS x^7+x^4+1
// Parameters: n -- Number of bits to scramble
//             a -- Input array 
//             b -- Output array
//             X -- Scrambler state
// ================================================================================================
wiStatus Phy11n_TX_Scrambler(int n, int a[], int b[])
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();

    if (pTX->ScramblerDisabled)
    {
        memcpy(b, a, n*sizeof(int));
    }
    else
    {
        int k, x;
        int X = Phy11n_TxState()->ScramblerShiftRegister;

        for (k=0; k<n; k++)
        {
            x = ((X>>6) ^ (X>>3)) & 1;   // P(X) = X^7 + X^4 + 1;
            X = (X << 1) | x;

            b[k] = a[k] ^ x;
        }

        if (!pTX->FixedScramblerState)
        {
            X = pTX->ScramblerShiftRegister; 
	        x = ((X>>6) ^ (X>>3)) & 1;   // P(X) = X^7 + X^4 + 1;         
            pTX->ScramblerShiftRegister = (X << 1) | x;
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_Scrambler()

// ================================================================================================
// FUNCTION  : Phy11n_TX_Encoder()
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a rate 1/2 convolutional encoder
// Parameters: Nbits      -- Number of bits in x
//             x          -- Input array
//             y          -- Output array (must be Nbits*CodeRate12/12 long) 
//             CodeRate12 -- CodeRate * 12...defines the puncturing pattern
// ================================================================================================
wiStatus Phy11n_TX_Encoder(int Nbits, int x[], int y[], int CodeRate12)
{
    const int pA_12[] = {1};
    const int pB_12[] = {1};

    const int pA_23[] = {1,1,1,1,1,1};
    const int pB_23[] = {1,0,1,0,1,0};

    const int pA_34[] = {1,1,0,1,1,0,1,1,0};
    const int pB_34[] = {1,0,1,1,0,1,1,0,1};

    const int pA_56[] = {1,1,0,1,0};
    const int pB_56[] = {1,0,1,0,1};

    const int *pA, *pB;

    int i, M, X = 0;

    if (CodeRate12 == 12) // uncoded case
    {
        memcpy(y, x, Nbits*sizeof(int));
        return WI_SUCCESS;
    }
    else // select the puncturing pattern
    {
        switch (CodeRate12)
        {
            case  6: pA = pA_12;  pB = pB_12;  M = 1;  break;
            case  8: pA = pA_23;  pB = pB_23;  M = 6;  break;
            case  9: pA = pA_34;  pB = pB_34;  M = 9;  break;
            case 10: pA = pA_56;  pB = pB_56;  M = 5;  break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
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
// end of Phy11n_TX_Encoder()

// ================================================================================================
// FUNCTION  : Phy11n_TX_QAM
// ------------------------------------------------------------------------------------------------
// Purpose   : Modulate the data subcarriers in Y with the coded bits in x using the modulation
//             scheme corresponding to the number of bits to be mapped to each subcarrier
// Parameters: Y       -- Frequency domain symbol sequence
//             x       -- Input array (coded bits)
//             N_BPSC  -- Number of coded bits per subcarrier
//             IsHtSig -- Indicate to rotate BPSK for HT-SIG
// ================================================================================================
wiStatus Phy11n_TX_QAM(wiComplex Y[], int x[], int N_BPSC, int Nmp, int IsHtSig)
{
  	int bI, bQ, k;
    wiReal a;

    //  Constellation scaling for constant output power
    //
    switch (N_BPSC)
    {
        case  1: a = 1.0;               break;
	    case  2: a = 1.0 / sqrt(  2.0); break;
	    case  4: a = 1.0 / sqrt( 10.0); break;
	    case  6: a = 1.0 / sqrt( 42.0); break;
	    case  8: a = 1.0 / sqrt(170.0); break;
	    case 10: a = 1.0 / sqrt(682.0); break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }

    memset(Y, 0, Nmp * sizeof(wiComplex)); // clear the output array

	switch (N_BPSC)
    {          
        case 1: //  BPSK  //////////////////////////////////////////////////////////
               
	        if (IsHtSig)
      	        for (k=0; k<Nmp; k++)
		        {
         	        Y[k].Imag = x[k]? a:-a;
		        }
	        else
		        for (k=0; k<Nmp; k++)
		        {
         	        Y[k].Real = x[k]? a:-a;
		        }
            break;

        case 2: //  QPSK  //////////////////////////////////////////////////////////
              
            for (k=0; k<Nmp; k++)
            {
         	    Y[k].Real = x[2*k  ]? a:-a;
         	    Y[k].Imag = x[2*k+1]? a:-a;
            }
            break;
            
		case 4: //  16-QAM  ////////////////////////////////////////////////////////
               
            for (k=0; k<Nmp; k++)
            {
                Y[k].Real = (x[4*k+0]? 1.0:-1.0) * (x[4*k+1]? a:3.0*a);
                Y[k].Imag = (x[4*k+2]? 1.0:-1.0) * (x[4*k+3]? a:3.0*a);
            }
            break;

		case 6:  //  64-QAM  ///////////////////////////////////////////////////////
        {
      	    const wiReal c[8] = {-7,-5,-1,-3, 7, 5, 1, 3};

            for (k=0; k<Nmp; k++)
            {
                bI = (x[6*k+0]<<2) | (x[6*k+1]<<1) | (x[6*k+2]<<0);
                bQ = (x[6*k+3]<<2) | (x[6*k+4]<<1) | (x[6*k+5]<<0);

                Y[k].Real = c[bI] * a;
                Y[k].Imag = c[bQ] * a;
            }
            break;
        }

        case 8:  //  256-QAM  //////////////////////////////////////////////////////
        {
          	const wiReal c[16] = {-15,-13,-9,-11,-1,-3,-7,-5,15,13,9,11,1,3,7,5};

            for (k=0; k<Nmp; k++)
            {
                bI = (x[8*k+0]<<3) | (x[8*k+1]<<2) | (x[8*k+2]<<1) | (x[8*k+3]<<0);
                bQ = (x[8*k+4]<<3) | (x[8*k+5]<<2) | (x[8*k+6]<<1) | (x[8*k+7]<<0);

                Y[k].Real = c[bI] * a;
                Y[k].Imag = c[bQ] * a;
            }
            break;
        }

   	    default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
	return WI_SUCCESS;
}
// end of Phy11n_TX_QAM()


// ================================================================================================
// FUNCTION  : Phy11n_TX_STBC()
// ------------------------------------------------------------------------------------------------
// Purpose   : Space-time block codes the modulated symbols x[] into Y[]
// Parameters: Y      -- Space-time coded symbol sequence
//             N_STS  -- Number of space-time streams, i.e. # rows in Y
//             X      -- Input array (modulated symbols)
//             N_SS   -- Number of spatial streams, i.e. # rows in X
//             Nmp    -- Number of modulated symbols per stream, i.e # columns in X
// ================================================================================================
wiStatus Phy11n_TX_STBC(wiComplex *Y[], int N_STS, wiComplex *X[], int N_SS, int Nmp)
{
    int i,j;
    switch (N_STS-N_SS)  // =pTX->STBC
    {
        case 0: // No STBC; just copy X to Y

            if (N_STS == N_SS) {
                for (i=0; i<N_SS; i++)
                    for (j=0; j<Nmp; j++)
                        Y[i][j] = X[i][j];
            }
            else {
                wiPrintf("ERROR in Phy11n_TX_STBC(): data inconsistent: STBC=0 but N_STS!=N_SS\n");
                return STATUS(WI_ERROR);
            }
            break;

        case 1: // ST-code the first stream

            Phy11n_TX_STEncode(Y[0], Y[1], X[0], Nmp);  // ST-Encode X[0] and put the result in Y[0] and Y[1]
            for (i=1; i<N_SS; i++)
            {
                for (j=0; j<Nmp; j++)
                    Y[i+1][j] = X[i][j];
            }
            break;

        case 2: // ST-code the first and second streams
            Phy11n_TX_STEncode(Y[0], Y[1], X[0], Nmp);  // ST-Encode X[0] and put the result in Y[0] and Y[1]
            Phy11n_TX_STEncode(Y[2], Y[3], X[1], Nmp);  // ST-Encode X[1] and put the result in Y[2] and Y[3]
            break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_STBC()


// ================================================================================================
// FUNCTION  : Phy11n_TX_STEncode()
// ------------------------------------------------------------------------------------------------
// Purpose   : Space-time block code the spatial stream X into streams Y1 and Y2
// Parameters: Y1, Y2 -- (Output) Space-time coded symbol sequences
//             X      -- Input sequence (modulated symbols)
//             Nmp    -- Number of symbols in X, i.e the length of X, Y1, and Y2
// ================================================================================================
wiStatus Phy11n_TX_STEncode(wiComplex *Y1, wiComplex *Y2, wiComplex *X, int Nmp)
{
    int Pairs = Nmp/NumDataSubcarriers/2;  // number of pairs of symbols in each subcarrier
    int i, k, n1, n2;

    if ((Nmp/NumDataSubcarriers) % 2 != 0)  // require an even number of symbols
        return STATUS(WI_ERROR_PARAMETER4); 

    for (i=0; i<Pairs; i++)
    {
        for (k=0; k<NumDataSubcarriers; k++)
        {
            n1 = (2*i  )*NumDataSubcarriers + k;
            n2 = (2*i+1)*NumDataSubcarriers + k;

            Y1[n1].Real =  X[n1].Real; // Y1[n1] =  X[n1]
            Y1[n1].Imag =  X[n1].Imag;
                        
            Y2[n1].Real = -X[n2].Real; // Y2[n1] = -X[n2]*
            Y2[n1].Imag =  X[n2].Imag;
            
            Y1[n2].Real =  X[n2].Real; // Y1[n2] =  X[n2]
            Y1[n2].Imag =  X[n2].Imag;

            Y2[n2].Real =  X[n1].Real; // Y2[n2] =  X[n1]*
            Y2[n2].Imag = -X[n1].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_STEncode()


// ================================================================================================
// FUNCTION  : Phy11n_TX_SpatialMap() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Space-time block codes the modulated symbols x[] into Y[]
// Parameters: Y      -- Space-time coded symbol sequence
//             N_TX   -- Number of TX antennas, i.e. # rows in Y
//             X      -- Input array (modulated symbols)
//             N_STS  -- Number of space-time streams, i.e. # rows in X
//             Nmp    -- Number of modulated symbols per stream, i.e # columns in X
// ================================================================================================
wiStatus Phy11n_TX_SpatialMap(wiComplex *Y[], int N_TX, wiComplex *X[], int N_STS, int Nmp)
{
    const wiComplex Zero = {0,0};
    int i, j;

    for (i=0; i<N_TX; i++)
    {
        for (j=0; j<Nmp; j++)
        {
            Y[i][j] = (i < N_STS) ? X[i][j] : Zero;
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_SpatialMap()


// ================================================================================================
// FUNCTION  : Phy11n_SymbolCycleShift()
// ------------------------------------------------------------------------------------------------
// Purpose   : Cyclic shift for an OFDM symbol with 64 samples
// Parameters: in_sp  -- Input Sample-time baseband signal (output overwrites input)
//             nshift -- Number of shifting
//             sizev  -- Size of shifting vector
// ================================================================================================
wiStatus Phy11n_SymbolCycleShift(wiComplex *in_sp, int nshift, int sizev)
{
    int i;
    wiComplex temp[160];

    if (sizev > 160) return STATUS(WI_ERROR_PARAMETER3);

    for (i=0; i<sizev; i++) temp[i] = in_sp[i];

    if (nshift < 0)
    {
        for (i=0; i<-nshift; i++)
            in_sp[(sizev-1)-(-nshift-(i+1))] = temp[i];

        for (i=0; i<(sizev+nshift); i++)
            in_sp[i] = temp[i-nshift];
    }
    else
    {
        for (i=0; i<nshift; i++)
            in_sp[i] = temp[(sizev-1)-(nshift-(i+1))];

        for (i=0; i<(sizev-nshift); i++)
            in_sp[i+nshift] = temp[i];
    }
    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_ScalingTXSig() 
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_ScalingTXSig(wiComplex *x, wiReal c, int N)
{
    int i;
    for (i=0; i<N; i++)
        x[i] = wiComplexScaled(x[i], c);

    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_TX_Interleave_NonHT()
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement the interleaver of 802.11 section 17.3.5.6
// Parameters: N_CBPS -- Number of code bits per OFDM symbol
//             N_BPSC -- Number of bits per subcarrier
//             x      -- Input array
//             y      -- Output array
// ================================================================================================
wiStatus Phy11n_TX_Interleave_NonHT(int N_CBPS, int N_BPSC, int x[], int y[])
{
    if (Phy11n_TxState()->InterleaverDisabled)
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
// end of Phy11n_TX_Interleave_NonHT()

// ================================================================================================
// FUNCTION  : Phy11n_PilotTonePhase()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_PilotTonePhase(wiReal S[][4], int N_STS, int nSym, Phy11nSymbolFormat_t Format)
{
	const int Pilot_MX4[4][4]={{1,1, 1,-1},{1, 1,-1, 1},{ 1,-1,1, 1},{-1,1,1,1}};
	const int Pilot_MX3[3][4]={{1,1,-1,-1},{1,-1, 1,-1},{-1, 1,1,-1}};
	const int Pilot_MX2[2][4]={{1,1,-1,-1},{1,-1,-1, 1}};
    const int Pilot_MX1[1][4]={{1,1, 1,-1}};

    const int (* Pilot_MX)[4];

    Phy11n_TxState_t *pTX = Phy11n_TxState();
    
	int *X = &(pTX->PilotShiftRegister);
	int x, i, k; 
	wiReal p;
	
	x = ((*X>>6) ^ (*X>>3)) & 1;   // P(X) = X^7 + X^4 + 1;
	*X = (*X << 1) | x;
	p = x ? -1.0:1.0;

    switch (Format)
    {
        case PHY11N_SYMBOL_IS_LSIG:
        case PHY11N_SYMBOL_IS_HTSIG:
        case PHY11N_SYMBOL_IS_L_OFDM:
        {
            wiReal *s = S[0];            
		    s[0] = s[1] = s[2] = p; s[3] = -p; 
            break;
	    }
        case PHY11N_SYMBOL_IS_HT_OFDM:
        {
            switch (N_STS)
            {
                case 1: Pilot_MX = Pilot_MX1; break;
                case 2: Pilot_MX = Pilot_MX2; break;
                case 3: Pilot_MX = Pilot_MX3; break;
                case 4: Pilot_MX = Pilot_MX4; break;
                default: return STATUS(WI_ERROR_UNDEFINED_CASE);
            }

            for (i=0; i<N_STS; i++)
            {
                for (k=0; k<4; k++)
                    S[i][k] = p * Pilot_MX[i][(nSym+k) % 4];
            }
            break;
        }
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_PilotTonePhase()


// ================================================================================================
// FUNCTION  : Phy11n_InsertSymbol()
// ------------------------------------------------------------------------------------------------
// Purpose   : Create an OFDM symbol from the discrete Fourier sequence with Nsd samples in 
//             frequency domain. A gaurd interval is prepended and the symbol is inserted into the 
//             time domain sequence.
// Parameters: d   -- OFDM symbol (48 point complex sequence)
//             x   -- Cyclic shifted Sampled-time sequence into which the symbol is added
//             nCS -- # of cyclic shifted samples
//             Nsd -- Number of subcarriers for data
// ================================================================================================
wiStatus Phy11n_InsertSymbol(wiComplex *d, wiComplex *x, wiReal *pilot, int nCS, int Nsd)
{
    const int subcarrier_map_NON_HT[48] = {
        -26,-25,-24,-23,-22, -20,-19,-18,-17,-16,-15,-14,-13,-12,-11,-10, -9, -8, -6, -5, -4, -3, -2, -1,
          1,  2,  3,  4,  5,   6,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26 
    };

    const int subcarrier_map_HT[52] = {
        -28, -27, -26,-25,-24,-23,-22, -20,-19,-18,-17,-16,-15,-14,-13,-12,-11,-10, -9, -8, -6, -5, -4, -3, -2, -1,
          1,  2,  3,  4,  5,   6,   8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 23, 24, 25, 26, 27, 28 
    };

    Phy11n_TxState_t *pTX = Phy11n_TxState();
    wiComplex s[80], S[64] = {{0}};
    wiReal A = 1.0/sqrt((double)pTX->N_TX*NumDataSubcarriers);
    int k;

    // Map the # nSub modulated points onto 64 subcarriers
    //
    if (Nsd == 48) // L-OFDM Mapping (Nsd = # of data subcarriers)
    { 
        for (k=0; k<Nsd; k++)
            S[subcarrier_map_NON_HT[k]+32] = d[k];
    }
    else // HT-OFDM Mapping
    {
        for (k=0; k<Nsd; k++)
            S[subcarrier_map_HT[k]+32] = d[k];
    }

    // Insert the pilot tones
    //
    k = -21+32; S[k].Real = pilot[0]; S[k].Imag = 0.0;
    k =  -7+32; S[k].Real = pilot[1]; S[k].Imag = 0.0;
    k =   7+32; S[k].Real = pilot[2]; S[k].Imag = 0.0;
    k =  21+32; S[k].Real = pilot[3]; S[k].Imag = 0.0;
    
    XSTATUS( Phy11n_TX_IDFT(S, s+16) );
    XSTATUS( Phy11n_SymbolCycleShift(s+16, nCS, 64) );
    
    // Add the guard interval by cyclic extension
    //
    for (k=0; k<16; k++)
        s[k] = s[k+64];
    
    XSTATUS( Phy11n_ScalingTXSig   (s, A, 80) );
    XSTATUS( Phy11n_TX_InsertSignal(x, s, 80) );
        
    return WI_SUCCESS;
}
// end of Insert_OFDM_Symbol

// ================================================================================================
// FUNCTION  : Phy11n_TX_IDFT()
// ------------------------------------------------------------------------------------------------
//  Purpose  : Compute the 64-point IDFT using a mapping where the input sequence is in ascending
//             frequency order; i.e., -32,...,-1,0,1,...,31.
// ================================================================================================
wiStatus Phy11n_TX_IDFT(wiComplex X[], wiComplex y[])
{
    wiComplex *W = Phy11n_TxState()->IDFT_W;
    wiComplex Z[64];
    int n, k;

    //  Compute the multiplier weights
    //
    if (W[0].Real != 1.0)
    {
        for (k=0; k<32; k++)
        {
            wiReal a = 2.0 * 3.1415926536 * (wiReal)k/64.0;
            W[k].Real = cos(a) / 64.0;
            W[k].Imag = sin(a) / 64.0;

            W[k+32].Real = -W[k].Real; 
            W[k+32].Imag = -W[k].Imag;
        }
    }
    //  Permute X into standard form (0...63) in Z
    //
    for (k=0; k<64; k++)
        Z[k] = X[(k + 32) % 64];

    //  Implement the 64-point IDFT
    //
    for (n=0; n<64; n++)
    {
        y[n] = Z[0];

        for (k=1; k<64; k++)
            y[n] = wiComplexAdd( y[n], wiComplexMultiply(Z[k], W[(k*n)%64]) );
    }
    return WI_SUCCESS;
}
// end of Phy11n_TX_IDFT()

// ================================================================================================
// FUNCTION  : Phy11n_TX_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : set parameters
// Parameters: 
// ================================================================================================
wiStatus Phy11n_TX_Init(void)
{
    int i;
    Phy11n_TxState_t *pTX = Phy11n_TxState();

    if (!pTX->ThreadIsInitialized && wiProcess_ThreadIndex()) {
        XSTATUS( Phy11n_InitializeThread(wiProcess_ThreadIndex()) )
    }
    pTX->N_ES =   1;  // Number of convolutional code streams

    if (pTX->MCS < 32) //////////////////////////////////////////////////////////////////
    {
	    pTX->N_SS = 1 + pTX->MCS/8; // Number of spatial streams

        switch (pTX->MCS % 8)
        {
            case 0: pTX->N_BPSC[0] = 1; pTX->CodeRate12 =  6; break;
            case 1: pTX->N_BPSC[0] = 2; pTX->CodeRate12 =  6; break;
            case 2: pTX->N_BPSC[0] = 2; pTX->CodeRate12 =  9; break;
            case 3: pTX->N_BPSC[0] = 4; pTX->CodeRate12 =  6; break;
            case 4: pTX->N_BPSC[0] = 4; pTX->CodeRate12 =  9; break;
            case 5: pTX->N_BPSC[0] = 6; pTX->CodeRate12 = 10; break;
            case 6: pTX->N_BPSC[0] = 6; pTX->CodeRate12 = 10; break;
            case 7: pTX->N_BPSC[0] = 6; pTX->CodeRate12 = 10; break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        for (i=1; i<pTX->N_SS; i++) 
            pTX->N_BPSC[i] = pTX->N_BPSC[0];

	    pTX->N_HT_DLTF = (pTX->N_SS == 3) ? 4 : pTX->N_SS;
    }
    else ////////////////////////////////////////////////////////////////////////////////
    {
        switch (pTX->MCS)
        {
            case 62:

                pTX->N_SS = 4;
                pTX->N_BPSC[0] = 6; pTX->N_BPSC[1] = 6; pTX->N_BPSC[2] = 4; pTX->N_BPSC[3] = 4;
                pTX->N_HT_DLTF = 4;
                pTX->CodeRate12 = 6; // BCC 1/2      
                break;  
        
            case 53:

                pTX->N_SS = 4;
                pTX->N_BPSC[0] = 4; pTX->N_BPSC[1] = 2; pTX->N_BPSC[2] = 2; pTX->N_BPSC[3] = 2;
                pTX->N_HT_DLTF = 4;
                pTX->CodeRate12 = 6; // BCC 1/2      
                break;   

            case 52: // 156M 3x3
       
                pTX->N_SS = 3;
                pTX->N_BPSC[0] = 6; pTX->N_BPSC[1] = 6; pTX->N_BPSC[2] = 4; 
                pTX->CodeRate12 = 9; // BCC 3/4
                pTX->N_HT_DLTF = 4;
                break; 
        
          case 50:  //3X3 136.5Mbps  //DJ

                pTX->N_SS = 3;
                pTX->N_BPSC[0] = 6; pTX->N_BPSC[1] = 4; pTX->N_BPSC[2] = 4;
                pTX->N_HT_DLTF = 4;
                pTX->CodeRate12 = 9; // BCC 3/4      
                break;
        
            case 49: // 117M 3x3
        
                pTX->N_SS = 3;
                pTX->N_BPSC[0] = 6; pTX->N_BPSC[1] = 4; pTX->N_BPSC[2] = 2; 
                pTX->CodeRate12 = 9; // BCC 3/4
                pTX->N_HT_DLTF = 4;
                break; 

            case 37: // 78Mbps 2x2

                pTX->N_SS = 2;
                pTX->N_BPSC[0] = 6; pTX->N_BPSC[1] = 2; 
                pTX->CodeRate12 = 9; // BCC 3/4   
                pTX->N_HT_DLTF = 2;
                break;  

            case 34: // 52Mbps 2x2

                pTX->N_SS = 2;
                pTX->N_BPSC[0] = 6; pTX->N_BPSC[1] = 2; 
                pTX->CodeRate12 = 6; // BCC 1/2   
                pTX->N_HT_DLTF = 2;
                break;  

            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    // compute remaining parameters

    pTX->N_STS = pTX->STBC + pTX->N_SS;
    switch (pTX->N_STS)
    {
		case 1:	 pTX->N_HT_DLTF = 1; break;
		case 2:  pTX->N_HT_DLTF = 2; break;
		case 3:  pTX->N_HT_DLTF = 4; break;
		case 4:  pTX->N_HT_DLTF = 4; break;
		default: pTX->N_HT_DLTF = 0; 
    }
    
    pTX->N_ESS = 0;
    pTX->N_HT_ELTF = 0;

	if ((pTX->N_STS + pTX->N_ESS>4) || !pTX->N_STS)
		return STATUS(WI_ERROR_UNDEFINED_CASE);
    
    pTX->N_TBPS = 0;
    pTX->N_CBPS = 0;
    for (i=0; i<pTX->N_SS; i++)
    {
        pTX->N_CBPSS[i] = 52 * pTX->N_BPSC[i];
        pTX->N_TBPS += pTX->N_BPSC[i];
        pTX->N_CBPS += pTX->N_CBPSS[i]; // Number of coded bits per symbol
    }
    pTX->N_DBPS = pTX->CodeRate12*pTX->N_CBPS/12;  // Nuber of data bits per symbol
    
    // [Barrett] The pilot tone shift register needs to be reset between packets. I added
    // this code here to insure the sequence is repeatible, but did not test whether the
    // sequence actually generated is correct per the standard.
    //
    pTX->PilotShiftRegister = 0x7F;

    return WI_SUCCESS;
}
// end of Phy11n_TX_Init()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

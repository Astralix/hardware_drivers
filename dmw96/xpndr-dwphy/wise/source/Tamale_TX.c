//--< Tamale_TX.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Tamale - OFDM Transmit Model
//  Copyright 2001-2003 Bermai, 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wiUtil.h"
#include "wiMath.h"
#include "Tamale.h"

#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg) < 0) return WI_ERROR_RETURNED;

// ================================================================================================
// FUNCTION : Tamale_TX_Init() 
// ------------------------------------------------------------------------------------------------
// Purpose  : Initialize parameters needed for the TX module (runs when PHY is selected)
//            Extend the ModulationROM to accomodate HT-OFDM   
// ================================================================================================
wiStatus Tamale_TX_Init(void)
{
    const int PSK[2]   = {-1,1};
    const int QAM16[4] = {-3,-1,3,1};
    const int QAM64[8] = {-7,-5,-1,-3, 7, 5, 1, 3};
 
    wiUWORD X, Y, A;
    Tamale_TxState_t *pTX = Tamale_TxState();
    Tamale_DvState_t *pDV = Tamale_DvState();
	Tamale_LookupTable_t *pLut = Tamale_LookupTable();
 
    // ------------------
    // Default Parameters
    // ------------------
    if (!pTX->aPadFront) pTX->aPadFront =1000; // pad the packet start with 50 us
    if (!pTX->aPadBack)  pTX->aPadBack  = 200; // pad the packet end with   10 us (account for decoding latency)
    if (!pTX->DAC.nEff)  pTX->DAC.nEff  = 9;   // 9 bit effective DAC resolution   
    if (!pTX->DAC.Vpp)   pTX->DAC.Vpp   = 1.0; // 1.0Vpp full-scale output

    if (!pTX->bPadFront) pTX->bPadFront = 550;
    if (!pTX->bPadBack)  pTX->bPadBack  = 88;

    // -----------------------------------------------
    // Build the subcarrier mapper look-up table (ROM)
    // -----------------------------------------------
    for (Y.word=0; Y.word<32; Y.word++)
    {
        for (X.word=0; X.word<16; X.word++)
        {
            A.w9 = (Y.w5<<4) | X.w4;
            switch (Y.w5)
            {
                case 0xD: case 0xF:   pLut->ModulationROM[A.w9&0x1F1].word =  13*PSK  [X.w1]<<4; break; // Legacy BPSK scaling: 13*16 (final scale)             
                case 0x5: case 0x7:   pLut->ModulationROM[A.w9&0x1F1].word =   9*PSK  [X.w1]<<4; break;                 
                case 0x9: case 0xB:   pLut->ModulationROM[A.w9&0x1F3].word =   4*QAM16[X.w2]<<4; break; 
                case 0x1: case 0x3:   pLut->ModulationROM[A.w9&0x1F7].word =   2*QAM64[X.w3]<<4; break; 
                
                case 0x10:            pLut->ModulationROM[A.w9&0x1F1].word = 201*PSK  [X.w1];    break; // BPSK scaling for HT DATA 201
                case 0x11: case 0x12: pLut->ModulationROM[A.w9&0x1F1].word = 139*PSK  [X.w1];    break;  
                case 0x13: case 0x14: pLut->ModulationROM[A.w9&0x1F3].word =  62*QAM16[X.w2];    break; 
                case 0x15:
                case 0x16: case 0x17: pLut->ModulationROM[A.w9&0x1F7].word =  31*QAM64[X.w3];    break;
                default:              pLut->ModulationROM[A.w9&0x1FF].word =  0;
            }
        }
    }   

    // ----------------------------------------------------------------
    // Build the Tamale interleaver/de-interleaver address tables (ROM) 
    // 10 interleaver Patterns
    // ----------------------------------------------------------------
    {  
        const int cbps[]  = {288, 96, 192, 48, 52, 104, 208, 312, 48,96};  //Pattern 8: Q read out of deinterleaver Pattern 9: I,Q write into deinterleaver 
        const int s[]     = {3,1,2,1,1,1,2,3,1,1}; 
        const int d[]     = {3,4,4,4,3,3,4,4,4,4}; 
        const int N_col[] = {16,16,16,16,13,13,13,13,16,16}; // 16 for 11a and 13 for 11n
        int Ncol;
        int m, n, p, i[2], j[2], k[2], a[2], c[2], addr, data[2], t,r;  
        
        for (m=0; m<9; m++) // jROM is the write ROM for TX and read ROM for RX. Only the first 9 patten (0-8) are used for jROM
        {
            t=(int) floor(m/8)*2+m%2;  // 0,1 for first 8 patterns, 2 for pattern 8 and 3 for pattern 9
            Ncol=N_col[m];
             
            r=((int) floor(t/2)<<6)+(int) floor(t/2); // 65 for pattern 8; 0 for 0-7
            for (n=0; n<cbps[m]/2; n++) { // n: clock 
                for (p=0; p<2; p++) { // p: the pair
                    k[p] = 2*n + p;
                    i[p] = (cbps[m]/Ncol) * (k[p]%Ncol) + (k[p]/Ncol); // First permutation
                    j[p] = s[m]*(i[p]/s[m]) + ((i[p]+cbps[m]-(Ncol*i[p]/cbps[m]))%s[m]); // Second permutation
                    a[p] = j[p] / d[m];
                    c[p] = j[p] % d[m]; // if d[m] is 3, address change so the pair are in different RAM. if d[m] is 4, no change in address
                    addr = (m << 8) | n;
                    data[p] = (((a[p]&0x7F)<<2) | (c[p]&0x03))+r;  // address+65 for pattern 8: Q signal
                    pLut->jROM[p][addr].word = data[p];
                }
            }            
        }

        for (m=0; m<10; m++) // kROM is the write ROM for RX and read ROM for TX. Pattern 0-7 and 9 are used for kROM. Pattern 8 (48 Q Output) are not used. 
        {
            t=(int) floor(m/8)*2+m%2;
            Ncol=N_col[m];
 
			switch (t)
            {               
                case 3: // Pattern 9: Write I, Q of 6M 
                {    
                    for (n=0; n<cbps[m]/4; n++) { // I,Q each with 48 samples. 
                        for (p=0;p<2; p++){ // p does not indicate input pair. 
                            k[p]=2*n+p;
                            a[p] = k[p] / d[m]; // if d[m] is 3, address change so the pair are in different RAM. if d[m] is 4, no change in address
                            c[p] = k[p] % d[m];
                            data[p] = ((a[p]&0x7F)<<2) | (c[p]&0x03);
                            addr=((m-1)<<8)|k[p];  // Map the address lookup table for Pattern 9 to the position for unused Patten 8
                            for (r=0; r<2; r++) {  // r: input pair index. I and Q from a pair
                                pLut->kROM[r][addr].word = data[p]+(r<<6)+r; // Q 
                            }
                        }
                    } 
                    break;
                }                                
                case 0: 
                case 1:  // 0, 1: Pattern 0-7    
                {                   
                    for (n=0; n<cbps[m]/2; n++) {
                        for (p=0; p<2; p++) {
                            k[p] = 2*n + p;
                            a[p] = k[p] / d[m];
                            c[p] = k[p] % d[m];
                            addr = (m << 8) | n;
                            data[p] = (((a[p]&0x7F)<<2) | (c[p]&0x03));
                            pLut->kROM[p][addr].word = data[p];
                        } 
                    } 
                    break;
                }
               default:  // Pattern 8 is not valid for kROM
                    break;
                
            } // switch (t)
        }       
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Transmit Specific DV Parameters
    //
    pDV->aTxStart = 180;
    pDV->bTxStart = 180;
    pDV->aTxEnd   =  34;
    pDV->bTxEnd   =  36;
    pDV->aTxStartTolerance = 2; // +/- 25 ns
    pDV->aTxEndTolerance   = 0; // +/- 0
    pDV->bTxStartTolerance = 4; // +/- 50 ns
    pDV->bTxEndTolerance   = 2; // +/- 25 ns

    return WI_SUCCESS;
}
// end of Tamale_TX_Init()

// ================================================================================================
// FUNCTION  : Tamale_Prms()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns the transmit signal scaling. This is a model output used to calibrate
//             power in the channel model.
// Parameters: none
// Returns   : rms transmit power at DAC output
// ================================================================================================
wiReal Tamale_Prms(void)
{
    Tamale_TxState_t *pTX = Tamale_TxState();

    double Xout = 128.3495/512.0;     // RMS LSBs at DAC input
    double Pout = Xout * Xout * 2.0;  // Power into 1 Ohm (both DACs - complex)
    double ADAC = (pTX->DAC.Vpp/2.0); // Digital-to-Analog scaling (Digital 1=0 to Full Scale)
    Pout = Pout * (ADAC * ADAC);
    return Pout;
}
// end of Tamale_Prms()

// ================================================================================================
// FUNCTION  : Tamale_L_SIG()  
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate L-SIG (Legacy SIGNAL symbol)
// Parameters: RATE      -- Data rate code
//             LENGTH    -- PSDU Length
//             a         -- Data bits
//             b         -- Scrambled data output (scrambler disabled for L-SIG)
// ================================================================================================
void Tamale_L_SIG(wiUWORD RATE, wiUWORD LENGTH, wiUWORD a[], wiUWORD b[])
{    
    int i;
    Tamale_TxState_t *pTX = Tamale_TxState();

    if (pTX->UseForcedSIGNAL)
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Forced SIGNAL
        //
        //  This is a test feature used to force the SIGNAL symbol to an arbitrary value;
        //  it is not implemented in RTL
        //
        for (i=0; i<24; i++)
        {
            a[i].bit = ( pTX->ForcedSIGNAL.w24 >> i ) & 1;
        }
    }
    else
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Generate and Insert the Legacy SIGNAL Symbol
        //
        wiUWORD p;

        a[ 0].bit = RATE.b3;   // RATE[3]
        a[ 1].bit = RATE.b2;   // RATE[2]
        a[ 2].bit = RATE.b1;   // RATE[1]
        a[ 3].bit = RATE.b0;   // RATE[0]
        a[ 4].bit = 0;                  // Reserved (0)
        a[ 5].bit = LENGTH.b0; // LENGTH[0]  
        a[ 6].bit = LENGTH.b1; // LENGTH[1]  
        a[ 7].bit = LENGTH.b2; // LENGTH[2]  
        a[ 8].bit = LENGTH.b3; // LENGTH[3]  
        a[ 9].bit = LENGTH.b4; // LENGTH[4]  
        a[10].bit = LENGTH.b5; // LENGTH[5]  
        a[11].bit = LENGTH.b6; // LENGTH[6]  
        a[12].bit = LENGTH.b7; // LENGTH[7]  
        a[13].bit = LENGTH.b8; // LENGTH[8]  
        a[14].bit = LENGTH.b9; // LENGTH[9]  
        a[15].bit = LENGTH.b10;// LENGTH[10]  
        a[16].bit = LENGTH.b11;// LENGTH[11]  

        // Calculate the parity bit
        //
        p.bit = 0;
        for (i=0; i<17; i++)
            p.bit = p.bit ^ a[i].bit;

        a[17].bit = p.bit;    // Parity
        a[18].bit = 0;        // Decoder Termination Bit (0)
        a[19].bit = 0;        // Decoder Termination Bit (0)
        a[20].bit = 0;        // Decoder Termination Bit (0)
        a[21].bit = 0;        // Decoder Termination Bit (0)
        a[22].bit = 0;        // Decoder Termination Bit (0)
        a[23].bit = 0;        // Decoder Termination Bit (0)
    }
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Scrambler Output: No scrambling for L-SIG (SIGNAL) symbol
	//
    for (i=0; i<24; i++) b[i] = a[i];
}
// end of Tamale_L_SIG()


// ================================================================================================
// FUNCTION  : Tamale_HT_SIG()  
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate HT-SIG1 and HT-SIG2. For Tamale, most parameters set to default
//             Setting except MCS, LENGTH and Aggregation                 
// Parameters: MCS       -- MCS indicator
//             LENGTH    -- HT Packet Length    
//             a         -- HT SIG output
//             b         -- Scrambled data output (scrambler disabled for HT-SIG)
// ================================================================================================
void Tamale_HT_SIG(wiUWORD MCS, wiUWORD LENGTH, wiBoolean Aggregation, wiUWORD a[], wiUWORD b[])
{    
    const unsigned int ofs = 24;  // Offset for HT-SIG2;
	unsigned int i;
 
    Tamale_TxState_t *pTX = Tamale_TxState();

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Generate Symbols HT-SIG1 (1 of 2)
	//
    if (pTX->UseForcedHTSIG)
    {
        for (i=0; i<24; i++)
        {
            a[i].bit = (pTX->ForcedHTSIG1.w24 >> i) & 1;
        }
    }
    else
    {
        a[ 0].bit = MCS.b0;     // MCS[0]
        a[ 1].bit = MCS.b1;     // MCS[1]   
        a[ 2].bit = MCS.b2;     // MCS[2]
        a[ 3].bit = MCS.b3;     // MCS[3]
        a[ 4].bit = MCS.b4;     // MCS[4]    
        a[ 5].bit = MCS.b5;     // MCS[5]
        a[ 6].bit = MCS.b6;     // MCS[6]
        a[ 7].bit = 0;          // CBW 20/40 (0)
        a[ 8].bit = LENGTH.b0;  // HT Length[0]
        a[ 9].bit = LENGTH.b1;  // HT Length[1]
        a[10].bit = LENGTH.b2;  // HT Length[2]
        a[11].bit = LENGTH.b3;  // HT Length[3]
        a[12].bit = LENGTH.b4;  // HT Length[4]
        a[13].bit = LENGTH.b5;  // HT Length[5]
        a[14].bit = LENGTH.b6;  // HT Length[6]
        a[15].bit = LENGTH.b7;  // HT Length[7]
        a[16].bit = LENGTH.b8;  // HT Length[8]
        a[17].bit = LENGTH.b9;  // HT Length[9]
        a[18].bit = LENGTH.b10; // HT Length[10]
        a[19].bit = LENGTH.b11; // HT Length[11]
        a[20].bit = LENGTH.b12; // HT Length[12]
        a[21].bit = LENGTH.b13; // HT Length[13]
        a[22].bit = LENGTH.b14; // HT Length[14]
        a[23].bit = LENGTH.b15; // HT Length[15]
    }
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Generate Symbols HT-SIG2 (2 of 2)
	//
    if (pTX->UseForcedHTSIG)
    {
        for (i=0; i<18; i++)
        {
            a[i+ofs].bit = (pTX->ForcedHTSIG2.w18 >> i) & 1;
        }
    }
    else
    {
        a[ 0+ofs].bit = Tamale_Registers()->TxSmoothing.b0;
        a[ 1+ofs].bit = 1;          // Not Sounding (1)
        a[ 2+ofs].bit = 1;          // Reserved (1)
        a[ 3+ofs].bit = Aggregation;// Aggregation
	    a[ 4+ofs].bit = 0;          // STBC (LSB 0)
        a[ 5+ofs].bit = 0;          // STBC (MSB 0)  
        a[ 6+ofs].bit = 0;          // LDPC Coding (0)
        a[ 7+ofs].bit = 0;          // SHORT GI (0)
        a[ 8+ofs].bit = 0;          // # of Extension spatial Streams (LSB 0) 
        a[ 9+ofs].bit = 0;          // # of Extension spatial Streams (MSB 0)
    }
    if ( !(pTX->UseForcedHTSIG && pTX->UseForcedHTSIG_CRC))
	{
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Calculate CRC
		//
		//  This is for error detection in the HT-SIG symbol. Protection is applied to
	    //  HT-SIG1 and bits 0-9 of HT-SIG2. An 8-bit CRC is computed and the CRC result
	    //  is stored in bits 10-17.
		//  
        unsigned int input; // shift register input, need to be unsigned;
		wiUWORD c;          // CRC Shift Register Coeffient; 

		c.word = 0xFF;  // Reset shift register to all ones

		for (i=0; i<34; i++)
		{       
			input = a[i].bit ^ c.b7;
			c.w8 = (c.w7<<1) | (input & 1);
			c.b1 = c.b1 ^ input;
			c.b2 = c.b2 ^ input;
		}    
		a[10+ofs].bit = !c.b7;
		a[11+ofs].bit = !c.b6;   
		a[12+ofs].bit = !c.b5;
		a[13+ofs].bit = !c.b4;
		a[14+ofs].bit = !c.b3;
		a[15+ofs].bit = !c.b2;
		a[16+ofs].bit = !c.b1;
		a[17+ofs].bit = !c.b0;
        
        //  Decoder Termination Bits
        //
        for (i=18; i<24; i++)
            a[i+ofs].bit = 0;
	}
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Scrambler Output: No scrambling for HT-SIG
	//
    for (i=0; i<48; i++) b[i] = a[i];
}
// end of Tamale_HT_SIG()


// ================================================================================================
// FUNCTION  : Tamale_DataGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmitter Packet Data Generator
// Parameters: PDG_Header  -- Indicates the PHY controller header bytes are being read
//             PDG_Signal  -- Indicates the SIGNAL symbol is being generated 
//             PDG_Data    -- PSDU octets, 8 bits (circuit input from MAC FIFO)
//             a           -- serialized packet data to encoder (circuit output @ 80 MHz)
//             N_DBPS      -- number of data bits per symbol (model input)
//             pReg        -- pointer to PHY register array (model input)
//             TxDone      -- indicates end of packet (last symbol)
// ================================================================================================
wiStatus Tamale_DataGenerator( wiBoolean FirstSymbol, wiUWORD PDG_Data[], wiUWORD a[], 
                               wiUWORD PDG_Out[], int N_DBPS, wiBoolean TxDone )
{
    Tamale_TxState_t  *pTX  = Tamale_TxState();
    TamaleRegisters_t *pReg = Tamale_Registers();

    wiUWORD X;
    unsigned int bit_count;
    int k;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialize the bit_count(er) and load the scrambler state from the TX_PRBS
    //  register on the first symbol
    //
    if (FirstSymbol)
    {
        bit_count = 0;     // reset the data bit counter
        X = pReg->TX_PRBS; // load the PRBS seed for subsequent processing
    }
    else
    {
        bit_count = pTX->DataGenerator.bit_count;
        X         = pTX->DataGenerator.PRBS;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Create the DATA field bit stream
    //
    for (k=0; k<N_DBPS; k++, bit_count++)
    {
        if (bit_count < 16) 
            a[k].bit = (pReg->TX_SERVICE.w16 >> k) & 1;
        else if (bit_count < 16 + 8*pTX->LENGTH.w16) 
            a[k].bit = (PDG_Data[(bit_count-16)/8].w8 >> (bit_count%8)) & 1;
        else
            a[k].bit = 0;
    
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Data Scrambler
        //
        if ((bit_count < 16 + 8*pTX->LENGTH.w16) || (bit_count >= 22 + 8*pTX->LENGTH.w16))
        {
            X.w7 = (X.w6 << 1) | (X.b6 ^ X.b3);
            PDG_Out[k].bit = a[k].bit ^ (X.b6 ^ X.b3);
        }
        else
            PDG_Out[k].bit = a[k].bit;
	}
    if (TxDone) 
    {
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  At the end of the packet update the PRBS seed using a second PRBS. Thus, for
        //  successive packets, the initial scrambler state is itself a PRBS.
        //
        X.word = pReg->TX_PRBS.w7;
        pReg->TX_PRBS.word = (X.w6 << 1) | (X.b6 ^ X.b3);
    }
    pTX->DataGenerator.PRBS = X;
    pTX->DataGenerator.bit_count = bit_count;

    return WI_SUCCESS;
}
// end of Tamale_DataGenerator()


// ================================================================================================
// FUNCTION  : Tamale_Encoder()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit data encoder, rate 1/2 + puncturer
// Parameters: EncRate  -- Data rate selection, 4-bits (circuit input)
//             EncIn    -- Encoder input bit stream    (circuit output)
//             EncOut   -- Encoder output bit stream
//             Nbits    -- Length of input bit stream  (model input)
//             nMode    -- Indicating of nMode  (model input)
// ================================================================================================
void Tamale_Encoder(wiUWORD EncRATE, wiUWORD EncIn[], wiUWORD EncOut[], int Nbits, wiBoolean nMode)
{
    const int pA_12[] = {1};                  // Rate 1/2...Puncturer is rate 1/1
    const int pB_12[] = {1};

    const int pA_23[] = {1,1,1,1,1,1};        // Rate 2/3...Puncturer is rate 3/4
    const int pB_23[] = {1,0,1,0,1,0};

    const int pA_34[] = {1,1,0,1,1,0,1,1,0};  // Rate 3/4...Puncturer is rate 2/3
    const int pB_34[] = {1,0,1,1,0,1,1,0,1};

    const int pA_56[] = {1,1,0,1,0};		  // Rate 5/6...Puncturer is rate 5/6
    const int pB_56[] = {1,0,1,0,1};		      

    const int *pA, *pB;
    int i, M, A, B;
    
    wiUWORD *X = &(Tamale_TxState()->Encoder_Shift_Register);

    // Select the puncturing pattern based on nMode and RATE/MCS
    //
    if (nMode)
    {
        switch (EncRATE.w7) // MCS
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
        switch (EncRATE.w4)
        {
            case 0xD: pA = pA_12; pB = pB_12; M=1; break;
            case 0xF: pA = pA_34; pB = pB_34; M=9; break;
            case 0x5: pA = pA_12; pB = pB_12; M=1; break;
            case 0x7: pA = pA_34; pB = pB_34; M=9; break;
            case 0x9: pA = pA_12; pB = pB_12; M=1; break;
            case 0xB: pA = pA_34; pB = pB_34; M=9; break;
            case 0x1: pA = pA_23; pB = pB_23; M=6; break;
            case 0x3: pA = pA_34; pB = pB_34; M=9; break;
            default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Convolutional Encoder
    //
    //  Data is encoded with a rate 1/2 convolutional code generating interleaved code
    //  bit streams A and B using the octal polynomials 0133 and 0171.
    //
    //  The data are then selectively punctured to achieve rates of 1/2, 2/3, 3/4, or
    //  5/6. The puncture tables above indicate a bit to be removed with a 0. Variable
    //  M indicates the length of the puncturing pattern
    //
    for (i=0; i<Nbits; i++)
    {
        X->w7 = (X->w7 << 1) | EncIn[i].bit;       // data bit shift-register

        A = X->b0 ^ X->b2 ^ X->b3 ^ X->b5 ^ X->b6; // encoder output: A
        B = X->b0 ^ X->b1 ^ X->b2 ^ X->b3 ^ X->b6; // encoder output: B

        if (pA[i%M]) (EncOut++)->word = A;          // puncturing
        if (pB[i%M]) (EncOut++)->word = B;
    }
}
// end of Tamale_Encoder()


// ================================================================================================
// FUNCTION  : Tamale_Interleaver()                
// ------------------------------------------------------------------------------------------------
// Purpose   : Interleaver/Deinterleaver for both Transmit and Receive
//           : Define write and read pattern sepaerately for pattern 8 and 9   
//           : No longer support rate 72M 
// Parameters: InRATE       -- RATE input indicating the data rate (circuit input)
//             IntlvIn      -- Interleaver input, 5 bits
//             IntlvOut     -- Interleaver output, 5 bits
//             IntlvTxRx    -- transmit/receive switch, 0=transmit, 1=receive (circuit input)
//             nMode        -- Legacy/HT switch. 0= Legacy mode, 1=HT mix mode  (input)
//             Rotated6Mbps -- Indicates 6 Mbps constellation is rotated 90 degrees when set
// ================================================================================================
void Tamale_Interleaver( wiUWORD InRATE, wiUWORD IntlvIn[], wiUWORD IntlvOut[], int IntlvTxRx, 
                         wiBoolean nMode, wiBoolean Rotated6Mbps )
{
    const int cbps[] = {288, 96, 192, 48, 52,104,208,312, 48,96};
    wiUWORD RAM[4][128];
    wiUWORD *rROM[2], *wROM[2];
    wiUWORD addr, bpsc;
    wiUWORD bpsc_out,addr_out;  //handle reading 
    int a, c, i, p;
   
//    Tamale_TxState_t *pTX = Tamale_TxState();
	Tamale_LookupTable_t *pLut = Tamale_LookupTable();
    
    if (nMode) 
    {   
        switch (InRATE.w7) // MCS[6:0]
        {
            case 0x0:                     bpsc.w4 = 4; bpsc_out.w4 = 4; break;
            case 0x1: case 0x2:           bpsc.w4 = 5; bpsc_out.w4 = 5; break;
            case 0x3: case 0x4:           bpsc.w4 = 6; bpsc_out.w4 = 6; break;
            case 0x5: case 0x6: case 0x7: bpsc.w4 = 7; bpsc_out.w4 = 7; break;
            default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
        }
    }
    else
    {
        switch (InRATE.w4) // RATE[3:0]
        {
            case 0xF:           bpsc.w4 = 3; bpsc_out.w4 = 3; break; // 2 MSBs of Rate
            case 0x5: case 0x7: bpsc.w4 = 1; bpsc_out.w4 = 1; break;
            case 0x9: case 0xB: bpsc.w4 = 2; bpsc_out.w4 = 2; break;
            case 0x1: case 0x3: bpsc.w4 = 0; bpsc_out.w4 = 0; break;
            case 0xD:           bpsc.w4 = (IntlvTxRx) ? 9:3;  bpsc_out.w4 = (IntlvTxRx & Rotated6Mbps) ? 8:3; break;
            default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
        }
    }
    //Uses 4 bits of bpsc; 
    addr.b11 = bpsc.b3;
    addr.b10 = bpsc.b2;
    addr.b9  = bpsc.b1;
    addr.b8  = bpsc.b0;
    
    addr_out.b11 = bpsc_out.b3;
    addr_out.b10 = bpsc_out.b2;
    addr_out.b9  = bpsc_out.b1;
    addr_out.b8  = bpsc_out.b0;

    // The RX Write Patten 9: 
    // Write address corresponding to Lookup table kROM patten 9 (case 3).
    //
    if (bpsc.w4 == 9) addr.w12 = (bpsc.w4-1) << 8; 
   
    if (IntlvTxRx == 0) // Transmit -----------------
    {
        wROM[0] = pLut->jROM[0]; wROM[1] = pLut->jROM[1];
        rROM[0] = pLut->kROM[0]; rROM[1] = pLut->kROM[1];
    }
    else                // Receive ------------------
    {
        wROM[0] = pLut->kROM[0]; wROM[1] = pLut->kROM[1];
        rROM[0] = pLut->jROM[0]; rROM[1] = pLut->jROM[1];
    
    }
  
    for (i=0; i<cbps[bpsc.w4]/2; i++) { // Write --    
        addr.w8 = i;
        for (p=0; p<2; p++) {
            c = wROM[p][addr.w12].w9 & 0x3;  //Uses 12 bits address;  // 2 LSB determine 
            a = wROM[p][addr.w12].w9 >> 2;
            RAM[c][a].w5 = IntlvIn[2*i+p].w5;        
        }
    }      
    for (i=0; i<cbps[bpsc_out.w4]/2; i++) { // Read ---
        addr_out.w8 = i;
        for (p=0; p<2; p++) {
            c = rROM[p][addr_out.w12].w9 & 0x3;
            a = rROM[p][addr_out.w12].w9 >> 2;
            IntlvOut[2*i+p].word = RAM[c][a].w5;          
        }
    }  
}
// end of Tamale_Interleaver()


// ================================================================================================
// FUNCTION  : Tamale_Modulator()
// ------------------------------------------------------------------------------------------------
// Parameters: ModuRate  -- RATE input to define the constellation, 4-bit (circuit input)
//             ModuX     -- Modulator input, 1-bit (modeled circuit input @ 80 MHz)
//             ModuOutR  -- Modulator output, 9-bit (circuit output @ 20 MHz)
//             ModuOutI  -- Modulator output, 9-bit (circuit output @ 20 MHz)
//             nMode     -- Mode indicator;  Specially needed for L-SIG field;  
//             HT_SIG    -- HT SIG indicator 1: HT_SIG 
//             Greenfield-- HT packet format (0: Mixed Mode, 1: Greenfield)
//             n         -- DATA symbol index, required for HT DATA symbol pilots    
// ================================================================================================
void Tamale_Modulator( wiUWORD ModuRate, wiUWORD ModuX[], wiWORD ModuOutR[], wiWORD ModuOutI[],
                       wiBoolean nMode, wiBoolean HT_SIG, wiBoolean Greenfield, wiUInt n )
{
    // subcarrier types for legacy OFDM {0=null, 1=data, 2=pilot}
    const int subcarrier_type_a[64] = {  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                         1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                         0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                         1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 }; // 48-63

    // subcarrier types for HT Mixed OFDM {0=null, 1=data, 2=pilot}
    const int subcarrier_type_n[64] = {  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                         1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                         0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                         1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 }; // 48-63

    wiUWORD c, p, AdrR, AdrI;
    int k;
    
    Tamale_TxState_t     *pTX  = Tamale_TxState();
	Tamale_LookupTable_t *pLut = Tamale_LookupTable();

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  'C' Model Error Checking
	//
	//  The HT-SIG symbol must be sent at 6 Mbps. This should be guaranteed by the
	//  calling function, but the requirement is checked here and causes a simulation
	//  abort if not true.
	//
	if (HT_SIG && (ModuRate.w4 != 0xD) && !nMode)
	{
		STATUS( WI_ERROR );
		exit(1);
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Pilot Tone Polarity
	//
	//  For each symbol, the pilot tone polarity is modulated by a PRBS to avoid
	//  creating spectral tones. The "p" term extracted from the PRBS below is XOR'd
	//  with the base pilot tone polarity at the bottom of the function.
	//
    {
        wiUWORD *X = &(pTX->Pilot_Shift_Register);
        p.bit = X->b6 ^ X->b3;
        X->w7 = (X->w7 << 1) | p.bit;         
    }
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Set LUT (ModulationROM) address bits for constellation
	//
	//  The encoding for the Real and Imag address is
	//		AdrX[8]   = nMode
	//      AdrX[7:4] = RATE[3:0] or MCS[3:0] depending on nMode
	//      AdrX[3:0] = Constellation coordinate (bit AdrX[3] is unused; it is left over
	//                  from the Dakota/Mojave basebands where a 256-QAM constellation
	//                  was supported.
	//
    AdrR.w9 = AdrI.w9 = (ModuRate.w4<<4) + (nMode ? (1<<8) : 0); // HT scaling for HT DATA 

    if (Greenfield & HT_SIG) 
        AdrR.w9 = AdrI.w9 = 0x10 << 4; // GF HT-SIG

    if (nMode)  // HT-Mix Mode   
    {        
        int q = 0;

		for (k=0; k<64; k++)
        {		
            switch (subcarrier_type_n[k])
            {
                case TAMALE_NULL_SUBCARRIER: //------------------------------------------------------
                {
                    ModuOutR[k].word = 0;
                    ModuOutI[k].word = 0;
                    break;
                }
                case TAMALE_DATA_SUBCARRIER: //------------------------------------------------------
                {
                    switch (ModuRate.w4) // MCS[3:0]
                    {          
						case 0:
                            AdrR.b3=0; AdrR.b2=0; AdrR.b1=0; AdrR.b0=(ModuX++)->bit;
                            break;
            
                        case 1:
                        case 2:
                            AdrR.b3=0; AdrR.b2=0; AdrR.b1=0; AdrR.b0=(ModuX++)->bit;
                            AdrI.b3=0; AdrI.b2=0; AdrI.b1=0; AdrI.b0=(ModuX++)->bit;
                            break;

                        case 3:
                        case 4:  // First input to MSB
                            AdrR.b3=0; AdrR.b2=0; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                            AdrI.b3=0; AdrI.b2=0; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                            break;

                        case 5:
                        case 6:
                        case 7:
                            AdrR.b3=0; AdrR.b2=(ModuX++)->bit; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                            AdrI.b3=0; AdrI.b2=(ModuX++)->bit; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                            break;
                       
                    }
                    if (ModuRate.w4 == 0x0) // BPSK
                    {
                        ModuOutR[k].word = pLut->ModulationROM[AdrR.w9].w9;   // w9 for final scale
                        ModuOutI[k].word = 0;
                    }
                    else
                    {
                        ModuOutR[k].word = pLut->ModulationROM[AdrR.w9].w9;
                        ModuOutI[k].word = pLut->ModulationROM[AdrI.w9].w9;
                    }
                }  break;

                case TAMALE_PILOT_SUBCARRIER: //-----------------------------------------------------
                {   					        
                    c.bit = p.bit^((n+(q++))%4==3) ? 1:0;
                    ModuOutR[k].word = c.bit ? -201:201; // BPSK scaling for HT-Mix Mode 
                    ModuOutI[k].word = 0;					
                    break;
                }
            }
        }
    }
    else  // Legacy modulation: 52 subcarriers
    {
        for (k=0; k<64; k++)
        {
            switch (subcarrier_type_a[k])
            {            
                case TAMALE_NULL_SUBCARRIER: // -----------------------------------------------------
                {
                    ModuOutR[k].word = 0;
                    ModuOutI[k].word = 0;
                    break;
                }
                case TAMALE_DATA_SUBCARRIER: // -----------------------------------------------------
                {
                    switch (ModuRate.w4)
                    {          
                        case 0xD:                                                          
                        case 0xF:
                            AdrR.b3=0; AdrR.b2=0; AdrR.b1=0; AdrR.b0=(ModuX++)->bit;
                            break;
            
                        case 0x5:
                        case 0x7:
                            AdrR.b3=0; AdrR.b2=0; AdrR.b1=0; AdrR.b0=(ModuX++)->bit;
                            AdrI.b3=0; AdrI.b2=0; AdrI.b1=0; AdrI.b0=(ModuX++)->bit;
                            break;

                        case 0x9:
                        case 0xB:
                            AdrR.b3=0; AdrR.b2=0; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                            AdrI.b3=0; AdrI.b2=0; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                            break;

                        case 0x1:
                        case 0x3:
                            AdrR.b3=0; AdrR.b2=(ModuX++)->bit; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                            AdrI.b3=0; AdrI.b2=(ModuX++)->bit; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                            break;
                    }
                    if ((ModuRate.w4 == 0xD) || (ModuRate.w4 == 0xF)) // BPSK
                    {
                        if (!HT_SIG)  
                        {
                            ModuOutR[k].word = pLut->ModulationROM[AdrR.w9].w9;
                            ModuOutI[k].word = 0;
                        }
                        else // rotated BPSK (RATE = 0xD enforced above)
                        {                            
                            ModuOutR[k].word = 0;
                            ModuOutI[k].word = pLut->ModulationROM[AdrR.w9].w9;
                        }
                    }
                    else
                    {
                        ModuOutR[k].word = pLut->ModulationROM[AdrR.w9].w9;
                        ModuOutI[k].word = pLut->ModulationROM[AdrI.w9].w9;
                    }
                    break;
                }

                case TAMALE_PILOT_SUBCARRIER: //-----------------------------------------------------
                {            
                    c.bit = p.bit ^ (k == 53 ? 1:0);
                    ModuOutR[k].word = (c.bit ? -1:1) * (Greenfield & HT_SIG ? 201:208); // HT scale for GF HT-SIG 
					ModuOutI[k].word = 0;
                    break;
                } 
            } 
        }
    }  
}
// end of Tamale_Modulator()


// ================================================================================================
// FUNCTION  : Tamale_L_Preamble()
// ------------------------------------------------------------------------------------------------
// Purpose   : Stream out legacy OFDM preamble
// Parameters: PreamOutR -- Preamble output, real component, 10-bits (circuit output)
//             PreamOutI -- Preamble output, imag component, 10-bits (circuit output)
// ================================================================================================
void Tamale_L_Preamble(wiWORD PreamOutR[], wiWORD PreamOutI[])
{
    const int shortR[16] = { 19,-55,-6,59,38,59,-6,-55,19,1,-33,-5,0,-5,-33,1};
    const int shortI[16] = { 19,1,-33,-5,0,-5,-33,1,19,-55,-6,59,38,59,-6,-55};
    const int longR[64]  = { 65,-2,17,40,9,25,-48,-16,41,22,0,-57,10,24,-9,50,26,15,-24,-55,34,29,-25,-23,-15,-51,-53,31,-1,-38,38,5,-65,5,38,-38,-1,31,-53,-51,-15,-23,-25,29,34,-55,-24,15,26,50,-9,24,10,-57,0,22,41,-16,-48,25,9,40,17,-2};
    const int longI[64]  = { 0,-50,-46,34,12,-36,-23,-44,-11,2,-48,-20,-24,-6,67,-2,-26,41,16,27,38,6,34,-9,-63,-7,-9,-31,22,48,44,41,0,-41,-44,-48,-22,31,9,7,63,9,-34,-6,-38,-27,-16,-41,26,2,-67,6,24,20,48,-2,11,44,23,36,-12,-34,46,50};

    unsigned int k;
    
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Insert the Preamble
	//
	//  Preamble generation is done from a lookup table. The short preamble has a period
	//  of 16T (for a total of 160T), and the long preamble as a period of 64T (for a
	//  total of 160T). The left-shift is an implicit multiply-by-two included to match
	//  the modulated signal scaling.
	//
    for (k=0; k<320; k++)
    {
        if (k < 160)
        {
            PreamOutR->word = shortR[k%16] << 1;
            PreamOutI->word = shortI[k%16] << 1;
        }
        else
        {
            PreamOutR->word = longR[k%64] << 1;
            PreamOutI->word = longI[k%64] << 1;
        }
        PreamOutR++;
        PreamOutI++;
    }
}
// end of Tamale_L_Preamble()


// ================================================================================================
// FUNCTION  : Tamale_HT_Preamble()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit HT-STF (1 symbol) and HT-LTF (1 symbol) for HT mixed mode
//             or  HT-GF-STF (2 symbol) and HT-GF-LTF (2 symbol)        
// Parameters: PreamOutR  -- Preamble output, real component, 10-bits (circuit output)
//             PreamOutI  -- Preamble output, imag component, 10-bits (circuit output)
//             Greenfield -- Indicate a GF preamble format
// ================================================================================================
wiStatus Tamale_HT_Preamble(wiWORD PreamOutR[], wiWORD PreamOutI[], wiBoolean Greenfield)
{
    const int HT_shortR[16] = { 19,-55,-6,59,38,59,-6,-55,19,1,-33,-5,0,-5,-33,1}; 
    const int HT_shortI[16] = { 19,1,-33,-5,0,-5,-33,1,19,-55,-6,59,38,59,-6,-55}; 
    
    // from the 201 scaling of BPSK for 56 subcarriers
    const int HT_longR[64]={63,-2,16,39,8,24,-46,-15,39,21,0,-55,10,24,-9,48,25,15,-23,-53,33,28,-24,-23,-14,-49,-51,30,-1,-37,37,5,-63,5,37,-37,-1,30,-51,-49,-14,-23,-24,28,33,-53,-23,15,25,48,-9,24,10,-55,0,21,39,-15,-46,24,8,39,16,-2};
    const int HT_longI[64]={0,-59,-25,9,35,-55,-11,-44,-19,18,-67,2,-41,4,63,-8,-13,24,32,13,45,4,29,-2,-70,2,-15,-26,21,45,44,38,0,-38,-44,-45,-21,26,15,-2,70,2,-29,-4,-45,-13,-32,-24,13,8,-63,-4,41,-2,67,-18,19,44,11,55,-35,-9,25,59};
 
    unsigned int k;
    
    if (Greenfield)
    {
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Greenfield Preamble
		//  
		//  This has a total length of 320T and includes 160T of HT-STF and 160T of 
		//  HT-LTF symbols repeated as necessary.
		//
        for (k=0; k<320; k++)
        {
            if (k < 160)
            {
                PreamOutR->word = HT_shortR[k%16] << 1;
                PreamOutI->word = HT_shortI[k%16] << 1;
            }
            else
            {
                PreamOutR->word = HT_longR[k%64] << 1;
                PreamOutI->word = HT_longI[k%64] << 1;
            }
            PreamOutR++;
            PreamOutI++;
        }
    }
    else
    {
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Mixed Mode Preamble (HT-STF and HT-LTF Segements)
		//
		//  The mixed mode format uses the legacy preamble and SIGNAL symbol followed by
		//  HT-SIG then 80T of HT-STF and 80T of HT-LTF. The HT-STF and HT-LTF portions
		//  are added below.
		//
        for (k=0; k<160; k++)  
        {
            if (k < 80)
            {
                PreamOutR->word = HT_shortR[k%16] << 1;
                PreamOutI->word = HT_shortI[k%16] << 1;           
            }
            else
            {
                PreamOutR->word = HT_longR[(k+32)%64] << 1; // Long sequence
                PreamOutI->word = HT_longI[(k+32)%64] << 1;
            }
            PreamOutR++;
            PreamOutI++;
        }
    }
    return WI_SUCCESS;
}
// end of Tamale_HT_Preamble()


// ================================================================================================
// FUNCTION  : Tamale_UpsamFIR() 
// ------------------------------------------------------------------------------------------------
// Purpose   : Core FIR filter implementation used with module "Upsam"
// Parameters: Nx -- Length of array x (model input)
//             x  -- Input data array, 10 bit (circuit input) 20MHz
//             Ny -- Length of array x (model input)
//             y  -- Output data array, 11 bit (circuit output) 80MHz
// ================================================================================================
void Tamale_UpsamFIR(int Nx, wiWORD x[], int Ny, wiWORD y[])
{
    const int b0[] = {  0,  0,  0,  0,  0,  0,  0, 64,  0,  0,  0,  0,  0,  0};
    const int b1[] = {  1, -1,  2, -3,  4, -8, 19, 58,-11,  6, -3,  2, -1,  1};
    const int b2[] = {  1, -2,  3, -4,  7,-13, 40, 40,-13,  7, -4,  3, -2,  1};
    const int b3[] = {  1, -1,  2, -3,  6,-11, 58, 19, -8,  4, -3,  2, -1,  1};

    wiWORD v[4], z[14] = {{0}};
    int i, j, k;
    
    for (k=0; k<Ny; k++)
    {
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Operate FIR Filter at 20 MHz
		//
		//  This operation operates four parallel filters at 20 MHz. These represent
		//  four phases of the up-sampled signal.
		//
        if (k%4 == 0)
        {
            for (i=13; i>0; i--) 
                z[i].w10 = z[i-1].w10;
        
            j = k/4;
            z[0].word = (j<Nx) ? x[j].w10 : 0;
            v[0].word = v[1].word = v[2].word = v[3].word = 0;

            for (i=0; i<14; i++)
            {
                v[0].w16 += (b0[i] * z[i].w10);
                v[1].w16 += (b1[i] * z[i].w10);
                v[2].w16 += (b2[i] * z[i].w10);
                v[3].w16 += (b3[i] * z[i].w10);
            }
        }
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Output at 80 MHz
		//
		//  This is a multiplexer which selects one of four phases in succession from
		//  the filter bank above.
		//
        switch (k%4)
        {
            case 0: y[k].word = v[0].w16 >> 5; break;
            case 1: y[k].word = v[1].w16 >> 5; break;
            case 2: y[k].word = v[2].w16 >> 5; break;
            case 3: y[k].word = v[3].w16 >> 5; break;
        }
    }
}
// end of Tamale_UpsamFIR()


// ================================================================================================
// FUNCTION  : Tamale_Upsam()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmitter Up-Sampling (Interpolation) Filter
// Parameters: Nx        -- length of UpsamInR and UpsamInI arrays (model input)
//             UpsamInR  -- filter input, real component, 10 bits (circuit input @ 20MHz)
//             UpsamInI  -- filter input, imag component, 10 bits (circuit input @ 20MHz)
//             Ny        -- length of UpsamOutR and UpsamOutI arrays (model input)
//             UpsamOutR -- filter output, real component, 11 bits (circuit output @ 80MHz)
//             UpsamOutI -- filter output, imag component, 11 bits (circuit output @ 80MHz)
// ================================================================================================
void Tamale_Upsam( int Nx, wiWORD UpsamInR[],  wiWORD UpsamInI[], 
                   int Ny, wiWORD UpsamOutR[], wiWORD UpsamOutI[] )
{
    Tamale_UpsamFIR(Nx, UpsamInR, Ny, UpsamOutR);
    Tamale_UpsamFIR(Nx, UpsamInI, Ny, UpsamOutI);
}
// end of Tamale_Upsam()


// ================================================================================================
// FUNCTION  : Tamale_Upmix()
// ------------------------------------------------------------------------------------------------
// Purpose   : Frequency shift the transmit channel
// Parameters: N         -- length of input/output arrays (model input)
//             UpmixInR  -- mixer input, real component,  11 bits (circuit input  @ 80MHz)
//             UpmixInI  -- mixer input, imag component,  11 bits (circuit input  @ 80MHz)
//             UpmixOutR -- mixer output, real component, 10 bits (circuit output @ 80MHz)
//             UpmixOutI -- mixer output, imag component, 10 bits (circuit output @ 80MHz)
// ================================================================================================
void Tamale_Upmix(int N, wiWORD UpmixInR[], wiWORD UpmixInI[], wiWORD UpmixOutR[], wiWORD UpmixOutI[])
{
    const int wR[8] = { 256, 181,   0,-181,-256,-181,   0, 181}; 
    const int wI[8] = {   0, 181, 256, 181,   0,-181,-256,-181};
    int k, i, c;
    
    TamaleRegisters_t *pReg = Tamale_Registers();

    if (pReg->UpmixOff.b0)
    {
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Bypass Mixer
		//
		//  This is the normal operating mode for Xpandr products. The output is simply
		//  a the 11-bit input limited to a 10-bit value.
		//
        for (k=0; k<N; k++)
        {
            UpmixOutR[k].word = UpmixInR[k].w11;
            UpmixOutI[k].word = UpmixInI[k].w11;

            if (abs(UpmixOutR[k].w11) > 511) UpmixOutR[k].word = (UpmixOutR[k].word<0) ? -511:511;
            if (abs(UpmixOutI[k].w11) > 511) UpmixOutI[k].word = (UpmixOutI[k].word<0) ? -511:511;
        }
    }
    else
    {
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  +/- 10 MHz Mixer
		//
		//  When the mixer is enabled, it multiplies the input by the sin/cos LUT values.
		//  The difference between +/- mixing is the direction of the counter.
		//
        c = Tamale_TxState()->UpmixPhase;

        for (k=0; k<N; k++, c++)
        {
            i = pReg->UpmixDown.b0 ? (N+1-c)%8 : c%8;

            UpmixOutR[k].word = (wR[i] * UpmixInR[k].w11) - (wI[i] * UpmixInI[k].w11); 
            UpmixOutI[k].word = (wR[i] * UpmixInI[k].w11) + (wI[i] * UpmixInR[k].w11); 

            UpmixOutR[k].word = UpmixOutR[k].w20 >> 8;
            UpmixOutI[k].word = UpmixOutI[k].w20 >> 8;

            if (abs(UpmixOutR[k].word) > 511) UpmixOutR[k].word = (UpmixOutR[k].word<0) ? -511:511;
            if (abs(UpmixOutI[k].word) > 511) UpmixOutI[k].word = (UpmixOutI[k].word<0) ? -511:511;
	    }
    }
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Complex Conjugate
	//
	//  This is a test/risk mitigatation feature not used for normal operation. The
	//  output is the complex conjugate of the normal output. Although this is not the
	//  same as swapping I and Q, it can be used to recover from such a problem.
	//
    if (pReg->UpmixConj.b0)
    {
        for (k=0; k<N; k++)
            UpmixOutI[k].word = -UpmixOutI[k].word;
    }
}
// end of Tamale_Upmix()


// ================================================================================================
// FUNCTION : Tamale_TxIQSource()
// ------------------------------------------------------------------------------------------------
// Purpose   : Signal multiplexer for TX I/Q correction input
// Parameters: N      -- length of input/output arrays (model input)
//             xR, xI -- real/imaginary input (from Upmix)
//             yR, yI -- real.imaginary output
// ================================================================================================
void Tamale_TxIQSource(int N, wiWORD xR[], wiWORD xI[], wiWORD *yR, wiWORD *yI)
{
    const signed int cosLUT[160] = 
    {
         255,  255,  254,  253,  252,  250,  248,  245,  243,  239,  236,  232,  227,  222,  217,  212,
         206,  200,  194,  187,  180,  173,  166,  158,  150,  142,  133,  125,  116,  107,   98,   88,
          79,   69,   60,   50,   40,   30,   20,   10,    0,  -10,  -20,  -30,  -40,  -50,  -60,  -69,
         -79,  -88,  -98, -107, -116, -125, -133, -142, -150, -158, -166, -173, -180, -187, -194, -200,
        -206, -212, -217, -222, -227, -232, -236, -239, -243, -245, -248, -250, -252, -253, -254, -255,
        -255, -255, -254, -253, -252, -250, -248, -245, -243, -239, -236, -232, -227, -222, -217, -212,
        -206, -200, -194, -187, -180, -173, -166, -158, -150, -142, -133, -125, -116, -107,  -98,  -88,
         -79,  -69,  -60,  -50,  -40,  -30,  -20,  -10,    0,   10,   20,   30,   40,   50,   60,   69,
          79,   88,   98,  107,  116,  125,  133,  142,  150,  158,  166,  173,  180,  187,  194,  200,
         206,  212,  217,  222,  227,  232,  236,  239,  243,  245,  248,  250,  252,  253,  254,  255
    };

    const signed int sinLUT[160] = 
    {
           0,   10,   20,   30,   40,   50,   60,   69,   79,   88,   98,  107,  116,  125,  133,  142,
         150,  158,  166,  173,  180,  187,  194,  200,  206,  212,  217,  222,  227,  232,  236,  239,
         243,  245,  248,  250,  252,  253,  254,  255,  255,  255,  254,  253,  252,  250,  248,  245,
         243,  239,  236,  232,  227,  222,  217,  212,  206,  200,  194,  187,  180,  173,  166,  158,
         150,  142,  133,  125,  116,  107,   98,   88,   79,   69,   60,   50,   40,   30,   20,   10,
           0,  -10,  -20,  -30,  -40,  -50,  -60,  -69,  -79,  -88,  -98, -107, -116, -125, -133, -142,
        -150, -158, -166, -173, -180, -187, -194, -200, -206, -212, -217, -222, -227, -232, -236, -239,
        -243, -245, -248, -250, -252, -253, -254, -255, -255, -255, -254, -253, -252, -250, -248, -245,
        -243, -239, -236, -232, -227, -222, -217, -212, -206, -200, -194, -187, -180, -173, -166, -158,
        -150, -142, -133, -125, -116, -107,  -98,  -88,  -79,  -69,  -60,  -50,  -40,  -30,  -20,  -10
    };

    TamaleRegisters_t *pReg = Tamale_Registers();
    int k0 = Tamale_TxState()->TxTonePhase;
    int k;
    
    wiWORD TxWordR, TxWordI;

    TxWordR.word = (pReg->TxWordRH.w2 << 8) | pReg->TxWordRL.w8;
    TxWordI.word = (pReg->TxWordIH.w2 << 8) | pReg->TxWordIL.w8;
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  TX Source Mutliplexer
    //
    //  Select from the normal modulator (DSSS/CCK/OFDM) or from a test signal for each of the
    //  quadrature terms. The four inputs are
    //
    //    0 : Normal baseband signal (default)
    //    1 : Test port input (not modeled here)
    //    2 : Tone generator
    //    3 : Fixed word (register fields TxWordR and TxWordI)
    //
    switch (pReg->TxIQSrcR.w2)
    {
        case 0: for (k=0; k<N; k++) yR[k] = xR[k]; break;
        case 1: break;
        case 2: for (k=0; k<N; k++) yR[k].word = cosLUT[((k+k0)*pReg->TxToneDiv.w8)%160]; break;
        case 3: for (k=0; k<N; k++) yR[k].word = TxWordR.w10; break;
    }

    switch (pReg->TxIQSrcI.w2)
    {
        case 0: for (k=0; k<N; k++) yI[k] = xI[k]; break;
        case 1: break;
        case 2: for (k=0; k<N; k++) yI[k].word = sinLUT[((k+k0)*pReg->TxToneDiv.w8)%160]; break;
        case 3: for (k=0; k<N; k++) yI[k].word = TxWordI.w10; break;
    }
}
// end of Tamale_TxIQSource()


// ================================================================================================
// FUNCTION : Tamale_TxIQ()
// ------------------------------------------------------------------------------------------------
// Purpose   : TX I/Q correction
// Parameters: N      -- length of input/output arrays (model input)
//             xR, xI -- real/imaginary input (from TxIQSource)
//             yR, yI -- real.imaginary output
// ================================================================================================
void Tamale_TxIQ(int N, wiWORD xR[], wiWORD xI[], wiWORD *yR, wiWORD *yI)
{
    int k;
    wiWORD TxIQa11, TxIQa22, TxIQa12;

    TamaleRegisters_t *pReg = Tamale_Registers();
    Tamale_TxState_t  *pTX  = Tamale_TxState();
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    // Transmitter I/Q Corrector
    //
    // Correction coefficients are selected by a multiplexer for sources from
    //
    //     0 : Register fields (default)
    //     1 : I/Q Calibration engine 
    //
    if ( pReg->sTxIQ.b0 == 0)
    {
        TxIQa11.word = pReg->TxIQa11.word;
        TxIQa22.word = pReg->TxIQa22.word;
        TxIQa12.word = pReg->TxIQa12.word;
    }
    else
    {
        TxIQa11.word = pTX->dTxIQa11[1].word;
        TxIQa22.word = pTX->dTxIQa22[1].word;
        TxIQa12.word = pTX->dTxIQa12[1].word;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    // Add two cycles delay when using coefficients generated from the Calibration 
    // engine for RTL verification. This is only valid when the routine is called 
    // sample-by-sample (i.e. N=1) during the Tx Calibration loopback.
    //
    pTX->dTxIQa11[1].word = pTX->dTxIQa11[0].word;
    pTX->dTxIQa11[0].word = pTX->TxIQa11.word;

    pTX->dTxIQa22[1].word = pTX->dTxIQa22[0].word;
    pTX->dTxIQa22[0].word = pTX->TxIQa22.word;

    pTX->dTxIQa12[1].word = pTX->dTxIQa12[0].word;
    pTX->dTxIQa12[0].word = pTX->TxIQa12.word;


    for (k=0; k<N; k++)
    {
        yR[k].word = xR[k].w10 + ((xI[k].w10 * TxIQa12.w5) >> 8);
        yI[k].word = xI[k].w10 + ((xR[k].w10 * TxIQa12.w5) >> 8);

        yR[k].word = yR[k].w11 + ((yR[k].w11 * TxIQa11.w5) >> 7);	
        yI[k].word = yI[k].w11 + ((yI[k].w11 * TxIQa22.w5) >> 7);	

        if (abs(yR[k].word)>511) yR[k].word = (yR[k].word<0) ? -511:511;
        if (abs(yI[k].word)>511) yI[k].word = (yI[k].word<0) ? -511:511;
    }
}
// end of Tamale_TxIQ()


// ================================================================================================
// FUNCTION : Tamale_Prd()
// ------------------------------------------------------------------------------------------------
// Purpose   : This function is a placeholder for a predistortion function. It currently
//             performs a translation from two's-complement to unsigned notation.
// Parameters: N       -- length of input/output arrays (model input)
//             PrdInR  -- PrD in,  real, 10 bits (circuit input @ 80MHz)
//             PrdInI  -- PrD in,  imag, 10 bits (circuit input @ 80MHz)
//             PrdOutR -- PrD out, real, 10 bits (circuit output @ 80MHz)
//             PrdOutI -- PrD out, imag, 10 bits (circuit output @ 80MHz)
// ================================================================================================
void Tamale_Prd(int N, wiWORD PrdInR[], wiWORD PrdInI[], wiUWORD PrdOutR[], wiUWORD PrdOutI[])
{
    const unsigned int cosLUT[8] = {768, 693, 512, 331, 256, 331, 512, 693}; // +10 MHz tone
    const unsigned int sinLUT[8] = {512, 693, 768, 693, 512, 331, 256, 331};
    int k;

    TamaleRegisters_t *pReg = Tamale_Registers();
    Tamale_TxState_t  *pTX  = Tamale_TxState();

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  TX Digital Predistortion (call only if used)
    //
    if (pReg->PrdDACSrcR.w2 == 3 || pReg->PrdDACSrcI.w2 == 3)
    {
        Tamale_TX_DPD(N, PrdInR, PrdInI, pTX->sReal, pTX->sImag);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  TX Output Multiplexer
    //
    //  The DAC input for each of the real and imaginary terms (I and Q) are driver from
    //  a multiplexer which selects a source for each independently. The four inputs are
    //
    //    0 : Normal baseband signal (default)
    //    1 : Test port input (not modeled here)
    //    2 : +10 MHz tone generator using the look-up table above
    //    3 : Digital predistortion
    //
    switch (pReg->PrdDACSrcR.w2)
    {
        case 0: for (k=0; k<N; k++) PrdOutR[k].word = PrdInR[k].w10 + 512; break;
        case 1: break;
        case 2: for (k=0; k<N; k++) PrdOutR[k].word = cosLUT[k%8]; break;
        case 3: for (k=0; k<N; k++) PrdOutR[k].word = pTX->sReal[k].w10 + 512; break;
    }
    switch (pReg->PrdDACSrcI.w2)
    {
        case 0: for (k=0; k<N; k++) PrdOutI[k].word = PrdInI[k].w10 + 512; break;
        case 1: break;
        case 2: for (k=0; k<N; k++) PrdOutI[k].word = sinLUT[k%8]; break;
        case 3: for (k=0; k<N; k++) PrdOutI[k].word = pTX->sImag[k].w10 + 512; break;
    }
}
// end of Tamale_Prd()


// ================================================================================================
// FUNCTION  : Tamale_DAC()
// ------------------------------------------------------------------------------------------------
// Parameters: N  -- Number of input samples in {uR,uI}
//             uR -- Real DAC input
//             uI -- Imaginary DAC input
//             s  -- DAC output signal (analog)
//             M  -- Oversample rate of s relative to {uR,uI}
// ================================================================================================
void Tamale_DAC(int N, wiUWORD uR[], wiUWORD uI[], wiComplex s[], int M)
{
    Tamale_TxState_t *pTX = Tamale_TxState();
    int k, i;
    
    if (M==1) // normal output
    {
        for (k=0; k<N; k++)
        {
            s[k].Real = pTX->DAC.c * (signed int)((uR[k].w10 & pTX->DAC.Mask) - 512);
            s[k].Imag = pTX->DAC.c * (signed int)((uI[k].w10 & pTX->DAC.Mask) - 512);
        }
    }
    else // oversampled output
    {
        for (k=0; k<N; k++) 
		{
            s[k*M].Real = pTX->DAC.c * (signed int)((uR[k].w10 & pTX->DAC.Mask) - 512);
            s[k*M].Imag = pTX->DAC.c * (signed int)((uI[k].w10 & pTX->DAC.Mask) - 512);

            for (i=1; i<M; i++)
                s[k*M+i] = s[k*M];
        }
    }
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Quadrature Mismatch
	//
	//  Amplitude mismatch between the quadrature data converters is modelled below. The
	//  model accepts an input specification on the level of mismatch in dB and coverts
	//  it to quadrature gains in a manner that maintains the total rms power level.
	//
    if (pTX->DAC.IQdB)
    {
        double a = atan(pow(10, pTX->DAC.IQdB/20.0));
        double aReal = sqrt(2.0) * sin(a);
        double aImag = sqrt(2.0) * cos(a);
        
        for (k=0; k<N*M; k++)
        {
            s[k].Real *= aReal;
            s[k].Imag *= aImag;
        }
    }
}
// end of Tamale_DAC()

// ================================================================================================
// FUNCTION  : Tamale_aTX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-Level Legacy OFDM TX routine
// Parameters: RATE     -- Data rate field from PHY Header
//             LENGTH   -- Number of PSDU bytes (in MAC_TX_D)
//             MAC_TX_D -- Data bytes transferred from MAC, including header
//             Nx       -- Number of transmit samples (by reference)
//             x        -- Transmit signal array/DAC output (by reference)
//             M        -- Oversample rate for analog signal
// ================================================================================================
wiStatus Tamale_aTX(wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M)
{
    const wiUWORD RATE_0xD = {0xD};  // RATE = 0xD (6 Mbps)
    const wiBoolean Greenfield = 0;  // Set Greenfield flag to 0 for any legacy format packets

    unsigned int db; // array offset: (scrambled) data bits
    unsigned int dc; // array offset: code bits and interleaver output
    unsigned int dv; // array offset: PSK/QAM mapper [64 subcarriers/symbol]
    unsigned int dx; // array offset: symbol generator (IFFT/preamble output) [80T/symbol]

    unsigned int i, N_BPSC, N_DBPS;
      
    Tamale_TxState_t  *pTX  = Tamale_TxState();

    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  Initialize TX at the start of the packet
	//
    pTX->Encoder_Shift_Register.w7 = 0x00; // reset the encoder
    pTX->Pilot_Shift_Register.w7   = 0x7F; // reset the pilot tone polarity PRBS

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Lookup RATE specific parameters
	//
	//		N_BPSC -- Number of data bits per subcarrier
	//      N_DBPS -- Number of data bits per OFDM symbol
	//
    switch (RATE.w4)
    {
        case 0xD: N_BPSC = 1; N_DBPS =  24; break;
        case 0xF: N_BPSC = 1; N_DBPS =  36; break;
        case 0x5: N_BPSC = 2; N_DBPS =  48; break;
        case 0x7: N_BPSC = 2; N_DBPS =  72; break;
        case 0x9: N_BPSC = 4; N_DBPS =  96; break;
        case 0xB: N_BPSC = 4; N_DBPS = 144; break;
        case 0x1: N_BPSC = 6; N_DBPS = 192; break;
        case 0x3: N_BPSC = 6; N_DBPS = 216; break;
        default:
            Tamale_Registers()->TxFault.word = 1;
            STATUS(WI_ERROR_UNDEFINED_CASE);
            return WI_SUCCESS;
    }

    if (LENGTH.w12 == 0) Tamale_Registers()->TxFault.word = 1;

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Compute DATA field length
	//
    //  The data field contains 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits, and 
	//  enough pad bits to fill a whole number of OFDM symbols, each containing N_DBPS 
	//  bits.
    //
    //  Allocate memory based on computed packet length
	//
    pTX->N_SYM = (16 + 8*LENGTH.w12 + 6 + (N_DBPS-1)) / N_DBPS;
    pTX->PacketDuration = 20.0e-6 + 4.0e-6 * (double)(pTX->N_SYM);

    XSTATUS( Tamale_TX_AllocateMemory(pTX->N_SYM, N_BPSC, N_DBPS, M, 0, 0) );

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Stream out the preamble from a lookup table
    //  Zero-pad the first aPadFront samples.
	//
    for (dx=0; dx < pTX->aPadFront; dx++)
        pTX->xReal[dx].word = pTX->xImag[dx].word = 0;

    Tamale_L_Preamble(pTX->xReal+dx, pTX->xImag+dx);
	dx += 320;

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Generate the SIGNAL symbol at 6 Mbps
	//
    Tamale_L_SIG      (RATE, LENGTH, pTX->a, pTX->b);
    db = dc = dv = 0;

    Tamale_Encoder    (RATE_0xD, pTX->b+db, pTX->c+dc, 24, 0);
    Tamale_Interleaver(RATE_0xD, pTX->c+dc, pTX->p+dc, 0, 0, 0);
    Tamale_Modulator  (RATE_0xD, pTX->p+dc, pTX->vReal+dv, pTX->vImag+dv, 0, 0, Greenfield, 0);
    Tamale_FFT        (0, pTX->vReal, pTX->vImag, NULL, NULL, pTX->xReal+dx, pTX->xImag+dx, NULL, NULL);

    dx += 80;  db += 24;  dc += 48;  dv += 64;
   
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Create DATA field OFDM Symbols
	//
    for (i=1; i<=pTX->N_SYM; i++)
    {
        wiBoolean TxDone = (i==pTX->N_SYM) ? 1:0; // flag for last symbol

        Tamale_DataGenerator(i==1, MAC_TX_D, pTX->a+db, pTX->b+db, N_DBPS, TxDone);
        Tamale_Encoder      (RATE, pTX->b+db, pTX->c+dc, N_DBPS, 0);
        Tamale_Interleaver  (RATE, pTX->c+dc, pTX->p+dc, 0, 0, 0);
        Tamale_Modulator    (RATE, pTX->p+dc, pTX->vReal+dv, pTX->vImag+dv, 0, 0, Greenfield, i-1);
        Tamale_FFT          (0, pTX->vReal+dv, pTX->vImag+dv, NULL, NULL, pTX->xReal+dx, pTX->xImag+dx, NULL, NULL);

        db += N_DBPS;
        dc += 48 * N_BPSC;
        dv += 64;
		dx += 80;
    }

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Front/Back Padding
	//
	//  The start of the packet is offset and the ending padded to accomodate various
	//  simulation objectives including providing time for PHY startup before the start
	//  of the packet, providing extra samples to accomodate FIR processes, and to 
	//  position the packet in a signal stream containing other packets.
	//
	//  The padding is controlled by aPadFront and aPadBack which have units of 50 ns.
	//  OfsEOP is the approximate end of packet at the final sample rate (80MHz for M=1).
	//
    for (i=0; i<pTX->aPadBack; i++)
    {
        pTX->xReal[dx+i].word = pTX->xImag[dx+i].word = 0;
    }
    pTX->OfsEOP = M*(4*dx + 24); // approximate end-of-packet

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Stream out data after FFT
	//
	//  Processing after the FFT are design-specific steps for frequency translation,
	//  filtering, and data word format conversion.
	//
    Tamale_Upsam     (pTX->Nx, pTX->xReal, pTX->xImag, pTX->Nz, pTX->yReal, pTX->yImag);
    Tamale_Upmix     (pTX->Nz, pTX->yReal, pTX->yImag, pTX->zReal, pTX->zImag);
    Tamale_TxIQSource(pTX->Nz, pTX->zReal, pTX->zImag, pTX->qReal, pTX->qImag);
    Tamale_TxIQ      (pTX->Nz, pTX->qReal, pTX->qImag, pTX->rReal, pTX->rImag);
    Tamale_Prd       (pTX->Nz, pTX->rReal, pTX->rImag, pTX->uReal, pTX->uImag);
    Tamale_DAC       (pTX->Nz, pTX->uReal, pTX->uImag, pTX->d, M);

    if (Nx) *Nx = pTX->Nd;
    if (x)  *x  = pTX->d; 

    return WI_SUCCESS;
}
// end of Tamale_aTX()


// ================================================================================================
// FUNCTION  : Tamale_nTX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-Level High Throughput Mixed Mode OFDM TX routine
// Parameters: MCS         -- Data rate field from PHY Header
//             LENGTH      -- Number of PSDU bytes (in MAC_TX_D)
//             MAC_TX_D    -- Data bytes transferred from MAC, including header
//             Nx          -- Number of transmit samples (by reference)
//             x           -- Transmit signal array/DAC output (by reference)
//             M           -- Oversample rate for analog signal
//             TxOpProtect -- Enable TXOP protection for mixed-mode packets
//             Greenfield  -- Transmit using greenfield instead of mixed-mode format
//             Aggregation -- Indicates an A-MPDU format packet
// ================================================================================================
wiStatus Tamale_nTX( wiUWORD MCS, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M, 
                     wiBoolean TxOpProtect, wiBoolean Greenfield, wiBoolean Aggregation )
{
    const wiUWORD RATE_0xD = {0xD}; // RATE = 0xD (6 Mbps)

    unsigned int db; // array offset: (scrambled) data bits
    unsigned int dc; // array offset: code bits and interleaver output
    unsigned int dv; // array offset: PSK/QAM mapper [64 subcarriers/symbol]
    unsigned int dx; // array offset: symbol generator (IFFT/preamble output) [80T/symbol]

    unsigned int i, N_BPSC, N_DBPS;
        
    Tamale_TxState_t  *pTX  = Tamale_TxState();

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Initialize TX at the start of the packet
	//
    pTX->Encoder_Shift_Register.w7 = 0x00; // reset the encoder
    pTX->Pilot_Shift_Register.w7   = 0x7F; // reset the pilot tone polarity PRBS
	
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Lookup RATE specific parameters
	//
	//		N_BPSC -- Number of data bits per subcarrier
	//      N_DBPS -- Number of data bits per OFDM symbol
	//
    switch (MCS.w7)
    {
        case 0: N_BPSC = 1; N_DBPS =  26; break;
	    case 1: N_BPSC = 2; N_DBPS =  52; break;
		case 2: N_BPSC = 2; N_DBPS =  78; break;
		case 3: N_BPSC = 4; N_DBPS = 104; break;
		case 4: N_BPSC = 4; N_DBPS = 156; break;
		case 5: N_BPSC = 6; N_DBPS = 208; break;
		case 6: N_BPSC = 6; N_DBPS = 234; break;
		case 7: N_BPSC = 6; N_DBPS = 260; break;
        default: 
            Tamale_Registers()->TxFault.word = 1;
            STATUS(WI_ERROR_UNDEFINED_CASE);
            return WI_SUCCESS;
    }

    if (LENGTH.w16 == 0) Tamale_Registers()->TxFault.word = 1;
    
    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  Compute DATA field length
	//
    //  The data field contains 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits, and 
	//  enough pad bits to fill a whole number of OFDM symbols, each containing N_DBPS 
	//  bits.
	//
    //  Allocate memory based on computed packet length
	//
    pTX->N_SYM = (16 + 8*LENGTH.w16 + 6 + (N_DBPS-1)) / N_DBPS;
    if (Greenfield) pTX->PacketDuration = 24.0e-6 + 4.0e-6 * (double)(pTX->N_SYM);
    else            pTX->PacketDuration = 36.0e-6 + 4.0e-6 * (double)(pTX->N_SYM);

    XSTATUS( Tamale_TX_AllocateMemory(pTX->N_SYM, N_BPSC, N_DBPS, M, 1, Greenfield) );

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Zero-pad the first aPadFront samples.
	//
	for (dx=0; dx < pTX->aPadFront; dx++)
		pTX->xReal[dx].word = pTX->xImag[dx].word = 0;

	/////////////////////////////////////////////////////////////////////////////////////
    //
    if (Greenfield)
    {   
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Greenfield HT Format
		//
		//  [HT Preamble]+[HT-SIG]+[HT-DATA]
		//  Stream out Greenfield HT preamble from LUT (320T samples)
		//
        Tamale_HT_Preamble(pTX->xReal+dx, pTX->xImag+dx, 1);
		dx += 320;
       
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Generate HT-SIG
        //
		//  This contains two symbols: HT-SIG1 and HT-SIG2. For each, the data
		//  subcarriers are rotated BPSK; no rotation is applied to pilot tones.
		//  HT signal scaling is implemented in the Modulator block.
		//
        Tamale_HT_SIG(MCS, LENGTH, Aggregation, pTX->a, pTX->b); 
        db = dc = dv = 0;
          
		for (i=0; i<2; i++)
		{
			Tamale_Encoder    (RATE_0xD, pTX->b+db, pTX->c+dc, 24, 0);   // HT_SIG, Legacy Mode Encoding;
			Tamale_Interleaver(RATE_0xD, pTX->c+dc, pTX->p+dc, 0, 0, 0); // Tx, Legacy Mode, I-Output  
			Tamale_Modulator  (RATE_0xD, pTX->p+dc, pTX->vReal+dv, pTX->vImag+dv, 0, 1, Greenfield, 0);  // nMode=0; HT-SIG, symbol index; 
			Tamale_FFT        (0, pTX->vReal+dv, pTX->vImag+dv,NULL, NULL, pTX->xReal+dx, pTX->xImag+dx, NULL, NULL);

			dx += 80;  db += 24;  dc += 48;  dv += 64;
		}
    }
    else
    {
		/////////////////////////////////////////////////////////////////////////////////
		//
		//  Mixed-Mode HT Format
		//
		//  [Legacy Preamble]+[L-SIG]+[HT-SIG]+[HT Preamble]+[HT-DATA]
		//  Stream out legacy preamble from LUT (320T samples)
		//
        Tamale_L_Preamble(pTX->xReal+dx, pTX->xImag+dx);
		dx += 320;
        
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  L-SIG Symbol for Protection
        //
        //  The L-SIG field is generated with a 6 Mbps rate and LENGTH sized to fit the
        //  packet duration (including the MAC Duration if TXOP_Protection is set
        //
        //  This includes 1 HT-STF, 1 HT-LTF and 2 symbols for HT_SIG. Based on IEEE 802.11n
        //  sec. 9.13.5 L-SIG TXOP protection, the TXOP is override by DurationID in the MAC
        //  Header. TXOP protection is applied only if requested by the MAC in the PHY header;
        //  otherwise, the calculation proceeds as though DurationID = 0. Units for Duration
        //  are microseconds (802.11n section 7.1.4.2).
        //
        //  For A-MPDU, a 4 byte delminator is included. The LSB is transmitted first.
        //
        {
            wiUWORD L_LENGTH, DurationID;

            if (TxOpProtect)
            {
                DurationID.word = (MAC_TX_D[3].w7 << 8) | MAC_TX_D[2].w8;
                
                if (Aggregation) {
                    Tamale_Registers()->TxFault.word = 1; // Tamale does not allow TxOpProtect with A-MPDU
                    return WI_SUCCESS;
                }
            }
            else DurationID.word = 0;

            L_LENGTH.word = 3*(pTX->N_SYM + 1 + 1 + 2 + ((DurationID.w16 + 3)/4) ) - 3; // -3 corresponds to the change for D 5.02 
            if (L_LENGTH.word > 4095) L_LENGTH.word = 4095;

            Tamale_L_SIG(RATE_0xD, L_LENGTH, pTX->a, pTX->b);
            db = dc = dv = 0;

			Tamale_Encoder    (RATE_0xD, pTX->b+db, pTX->c+dc, 24, 0);
			Tamale_Interleaver(RATE_0xD, pTX->c+dc, pTX->p+dc, 0, 0, 0);
			Tamale_Modulator  (RATE_0xD, pTX->p+dc, pTX->vReal+dv, pTX->vImag+dv, 0, 0, Greenfield, 0);
			Tamale_FFT        (0, pTX->vReal+dv, pTX->vImag+dv, NULL, NULL, pTX->xReal+dx, pTX->xImag+dx, NULL, NULL);

			dx += 80;  db += 24;  dc += 48;  dv += 64;
		}	        
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Generate HT-SIG
        //
		//  This contains two symbols: HT-SIG1 and HT-SIG2. For each, the data
		//  subcarriers are rotated BPSK; no rotation is applied to pilot tones.
		//
        Tamale_HT_SIG(MCS, LENGTH, Aggregation, pTX->a+db, pTX->b+db); // data bits for both symbols

		for (i=1; i<=2; i++)
		{
			Tamale_Encoder    (RATE_0xD, pTX->b+db, pTX->c+dc, 24, 0);
			Tamale_Interleaver(RATE_0xD, pTX->c+dc, pTX->p+dc, 0, 0, 0);
			Tamale_Modulator  (RATE_0xD, pTX->p+dc, pTX->vReal+dv, pTX->vImag+dv, 0, 1, Greenfield, 0);
			Tamale_FFT        (0, pTX->vReal+dv, pTX->vImag+dv, NULL, NULL, pTX->xReal+dx, pTX->xImag+dx, NULL, NULL);
			
            dx += 80;  db += 24;  dc += 48;  dv += 64;
		}       
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Generate HT Preamble
        //
		//  Put the HT preamble into the "x" array. For the modes and MCS supported here,
        //  the preamble is 4 HT-STF symbols (4 us) and 1 HT-LTF symbol (8 us), for a
        //  total of 160T at 20 MHz.
        //
        Tamale_HT_Preamble(pTX->xReal+dx, pTX->xImag+dx, 0);
		dx += 160;
    }

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Create DATA field HT-OFDM Symbols
	//
    for (i=1; i<=pTX->N_SYM; i++)
    {
        wiBoolean FirstSym = i==1 ? 1:0;          // flag for first symbol
        wiBoolean TxDone   = i==pTX->N_SYM ? 1:0; // flag for last symbol

        Tamale_DataGenerator(FirstSym, MAC_TX_D, pTX->a+db, pTX->b+db, N_DBPS, TxDone);
        Tamale_Encoder      (MCS, pTX->b+db,  pTX->c+dc, N_DBPS, 1);
        Tamale_Interleaver  (MCS, pTX->c+dc, pTX->p+dc, 0, 1, 0);
        Tamale_Modulator    (MCS, pTX->p+dc, pTX->vReal+dv, pTX->vImag+dv, 1, 0, Greenfield, i-1);
        Tamale_FFT          (0, pTX->vReal+dv, pTX->vImag+dv, NULL, NULL, pTX->xReal+dx, pTX->xImag+dx, NULL, NULL);             

        db += N_DBPS;
        dc += 52*N_BPSC;
        dv += 64;
		dx += 80;
    }

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Front/Back Padding
	//
	//  The start of the packet is offset and the ending padded to accomodate various
	//  simulation objectives including providing time for PHY startup before the start
	//  of the packet, providing extra samples to accomodate FIR processes, and to 
	//  position the packet in a signal stream containing other packets.
	//
	//  The padding is controlled by aPadFront and aPadBack which have units of 50 ns.
	//  OfsEOP is the approximate end of packet at the final sample rate (80MHz for M=1).
	//
    for (i=0; i<pTX->aPadBack; i++)
	{
        pTX->xReal[dx+i].word = pTX->xImag[dx+i].word = 0;
	}
    pTX->OfsEOP = M * (4*dx + 24);

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Stream out data after FFT
	//
	//  Processing after the FFT are design-specific steps for frequency translation,
	//  filtering, and data word format conversion.
	//
    Tamale_Upsam     (pTX->Nx, pTX->xReal, pTX->xImag, pTX->Nz, pTX->yReal, pTX->yImag);
    Tamale_Upmix     (pTX->Nz, pTX->yReal, pTX->yImag, pTX->zReal, pTX->zImag);
    Tamale_TxIQSource(pTX->Nz, pTX->zReal, pTX->zImag, pTX->qReal, pTX->qImag);
    Tamale_TxIQ      (pTX->Nz, pTX->qReal, pTX->qImag, pTX->rReal, pTX->rImag);
    Tamale_Prd       (pTX->Nz, pTX->rReal, pTX->rImag, pTX->uReal, pTX->uImag);
    Tamale_DAC       (pTX->Nz, pTX->uReal, pTX->uImag, pTX->d, M);

    if (Nx) *Nx = pTX->Nd;
    if (x)  *x  = pTX->d;
 
    return WI_SUCCESS;
}
// end of Tamale_nTX()


// ================================================================================================
// FUNCTION  : Tamale_TX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// ================================================================================================
wiStatus Tamale_TX_Restart(void)
{
    Tamale_TxState_t *pTX = Tamale_TxState();
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Define DAC Mask and Scaling
    //
    //  Mask: Bits to pass; force LSBs to zero to reduce resolution from 10 bits
    //  c   : Signal scaling...full scale input range divided by 1024 (2^10 bits)
    //
    pTX->DAC.Mask = ((1 << pTX->DAC.nEff) - 1) << (10 - pTX->DAC.nEff);
    pTX->DAC.c    = pTX->DAC.Vpp / 1024.0;

    return WI_SUCCESS;
}
// end of Tamale_TX_Restart()


// ================================================================================================
// FUNCTION : Tamale_TX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Tamale_TX_FreeMemory(Tamale_TxState_t *pTX, wiBoolean ClearPointerOnly)
{
    #define FREE(ptr)                                        \
    {                                                        \
        if (!ClearPointerOnly && (ptr != NULL)) wiFree(ptr); \
        ptr = NULL;                                          \
    }

    FREE(pTX->a);       pTX->Na = 0;
    FREE(pTX->b);       pTX->Nb = 0;
    FREE(pTX->c);       pTX->Nc = 0;
    FREE(pTX->p);       pTX->Np = 0;
    FREE(pTX->cReal);   pTX->Nc = 0;
    FREE(pTX->cImag);   
    FREE(pTX->vReal);   pTX->Nv = 0;
    FREE(pTX->vImag);   
    FREE(pTX->xReal);   pTX->Nx = 0;
    FREE(pTX->xImag);   pTX->Nx = 0;
    FREE(pTX->yReal);   pTX->Ny = 0;
    FREE(pTX->yImag);   pTX->Ny = 0;
    FREE(pTX->zReal);   pTX->Nz = 0;
    FREE(pTX->zImag);   pTX->Nz = 0;
    FREE(pTX->qReal);   
    FREE(pTX->qImag);   
    FREE(pTX->rReal);   
    FREE(pTX->rImag);   
    FREE(pTX->sReal);   
    FREE(pTX->sImag);   
    FREE(pTX->uReal);   
    FREE(pTX->uImag);   
    FREE(pTX->d);       pTX->Nd = 0;
    FREE(pTX->traceTx); 

    #undef FREE
    return WI_SUCCESS;
}
// end of Tamale_TX_FreeMemory()


// ================================================================================================
// FUNCTION : Tamale_TX_AllocateMemory()
// Purpose  : Allocate Memory for OFDM transmissions according to nMode
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Tamale_TX_AllocateMemory(int N_SYM, int N_BPSC, int N_DBPS, int M, wiBoolean nMode, wiBoolean Greenfield)
{
    Tamale_TxState_t *pTX = Tamale_TxState();

    // Determine if a new memory allocation is required
    //
    if ((pTX->LastAllocation.Fn      != (wiFunction)Tamale_TX_AllocateMemory) ||
        (pTX->LastAllocation.Size[0] != N_SYM )    ||
        (pTX->LastAllocation.Size[1] != N_BPSC)    ||
        (pTX->LastAllocation.Size[2] != N_DBPS)    ||
        (pTX->LastAllocation.Size[3] != M     )    ||
        (pTX->LastAllocation.Size[4] != nMode )    ||
        (pTX->LastAllocation.Size[5] != Greenfield)||
        (pTX->LastAllocation.Size[6] != (int)pTX->aPadFront) ||
        (pTX->LastAllocation.Size[7] != (int)pTX->aPadBack)
        )
    {
        pTX->LastAllocation.Fn      = (wiFunction)Tamale_TX_AllocateMemory;
        pTX->LastAllocation.Size[0] = N_SYM;
        pTX->LastAllocation.Size[1] = N_BPSC;
        pTX->LastAllocation.Size[2] = N_DBPS;
        pTX->LastAllocation.Size[3] = M;
        pTX->LastAllocation.Size[4] = nMode;
        pTX->LastAllocation.Size[5] = Greenfield;
        pTX->LastAllocation.Size[6] = pTX->aPadFront;
        pTX->LastAllocation.Size[7] = pTX->aPadBack;
    }
    else 
    {
        return WI_SUCCESS;
    }

    XSTATUS( Tamale_TX_FreeMemory(pTX, 0) )

    if (nMode) // HT Mix Mode; Enough for GF Mode
    {
        pTX->Na = 24*3 + (N_SYM+1)*N_DBPS; // L_SIG + HT-SIG1 + HT-SIG2 + DATA
        pTX->Nb = pTX->Na;
        pTX->Nc = 48*3 + (52 * (N_SYM+1) * N_BPSC);
        pTX->Np = 48 + pTX->Nc;
        if (Greenfield) pTX->Nx = 80*(N_SYM + 6) + pTX->aPadFront + pTX->aPadBack + 82;
        else            pTX->Nx = 80*(N_SYM + 9) + pTX->aPadFront + pTX->aPadBack + 82;
    }
    else  // Legacy Mode;
    {
        pTX->Na = 24 + (N_SYM+1)*N_DBPS;
        pTX->Nb = pTX->Na;
        pTX->Nc = 48 + (48 * (N_SYM+1) * N_BPSC);
        pTX->Np = 48 + pTX->Nc;
        pTX->Nx = 80*(N_SYM + 5) + pTX->aPadFront + pTX->aPadBack + 82;
    }   
    pTX->Nv = pTX->Nx;
    pTX->Ny = 4 * (pTX->Nx + 10);
    pTX->Nz = 4 * (pTX->Nx + 10);
    pTX->Nd = M * pTX->Nz;

    pTX->a     = (wiUWORD  *)wiCalloc(pTX->Na, sizeof(wiUWORD));
    pTX->b     = (wiUWORD  *)wiCalloc(pTX->Nb, sizeof(wiUWORD));
    pTX->c     = (wiUWORD  *)wiCalloc(pTX->Nc, sizeof(wiUWORD));
    pTX->p     = (wiUWORD  *)wiCalloc(pTX->Np, sizeof(wiUWORD));
    pTX->vReal = (wiWORD   *)wiCalloc(pTX->Nx, sizeof(wiWORD ));
    pTX->vImag = (wiWORD   *)wiCalloc(pTX->Nx, sizeof(wiWORD ));
    pTX->xReal = (wiWORD   *)wiCalloc(pTX->Nx, sizeof(wiWORD ));
    pTX->xImag = (wiWORD   *)wiCalloc(pTX->Nx, sizeof(wiWORD ));
    pTX->yReal = (wiWORD   *)wiCalloc(pTX->Ny, sizeof(wiWORD ));
    pTX->yImag = (wiWORD   *)wiCalloc(pTX->Ny, sizeof(wiWORD ));
    pTX->zReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->zImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->qReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->qImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->rReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->rImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->sReal = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->sImag = (wiWORD   *)wiCalloc(pTX->Nz, sizeof(wiWORD ));
    pTX->uReal = (wiUWORD  *)wiCalloc(pTX->Nz, sizeof(wiUWORD));
    pTX->uImag = (wiUWORD  *)wiCalloc(pTX->Nz, sizeof(wiUWORD));
    pTX->d     = (wiComplex*)wiCalloc(pTX->Nd, sizeof(wiComplex));
    pTX->traceTx = (Tamale_traceTxType *)wiCalloc(pTX->Nz, sizeof(Tamale_traceTxType));

    if (!pTX->a  || !pTX->b  || !pTX->c  || !pTX->p  || !pTX->xReal || !pTX->xImag 
        || !pTX->yReal || !pTX->yImag || !pTX->zReal || !pTX->zImag || !pTX->uReal || !pTX->uImag 
        || !pTX->qReal || !pTX->qImag || !pTX->rReal || !pTX->rImag || !pTX->sReal || !pTX->sImag 
        || !pTX->d || !pTX->traceTx)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    return WI_SUCCESS;
}
// end of Tamale_TX_AllocateMemory()


// ================================================================================================
// FUNCTION  : Tamale_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Baseband Top-level TX routine
// Parameters: Length   -- Number of bytes in MAC_TX_D
//             MAC_TX_D -- Data bytes transferred from MAC, including header
//             Nx       -- Number of transmit samples (by reference)
//             x        -- Transmit signal array/DAC output (by reference)
//             M        -- Oversample rate for analog signal
// ================================================================================================
wiStatus Tamale_TX(int Length, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M)
{
    Tamale_TxState_t  *pTX  = Tamale_TxState();
    TamaleRegisters_t *pReg = Tamale_Registers();
    Tamale_DvState_t  *pDV  = Tamale_DvState();
    unsigned int kStart = 0, kEnd = 0;

    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  Error/Range Checking
	//
    if (InvalidRange(Length, 8, 65535+8)) return STATUS(WI_ERROR_PARAMETER1);
    if (!MAC_TX_D)                        return STATUS(WI_ERROR_PARAMETER3);
    if (!Nx)                              return STATUS(WI_ERROR_PARAMETER4);
    if (!x)                               return STATUS(WI_ERROR_PARAMETER5);
    if (InvalidRange(M, 1, 40000))        return STATUS(WI_ERROR_PARAMETER6); 

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Copy the Register Map
    //
    //  Makes a copy of the current register map into the DV structure. This can be used
    //  to recreate the initial state if register values change during the simulation.
    //  For example, the TX_PRBS state is updated following an OFDM packet transmission.
    //  This record contains the initial value.
    //
    if (!pDV->NoRegMapUpdateOnTx && !wiProcess_ThreadIndex())
    {
        XSTATUS( Tamale_WriteRegisterMap( pDV->RegMap ) );
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Store Header in Read-Only Registers.
    //  Record a pointer to the input buffer (for trace).
    //
    if (!pTX->NoTxHeaderUpdate)
    {
        pReg->TxHeader1.word = MAC_TX_D[0].w8;
        pReg->TxHeader2.word = MAC_TX_D[1].w8;
        pReg->TxHeader3.word = MAC_TX_D[2].w8;
        pReg->TxHeader4.word = MAC_TX_D[3].w8;
        pReg->TxHeader5.word = MAC_TX_D[4].w8;
        pReg->TxHeader6.word = MAC_TX_D[5].w8;
        pReg->TxHeader7.word = MAC_TX_D[6].w8;
        pReg->TxHeader8.word = MAC_TX_D[7].w8;
    }        

    pTX->MAC_TX_D = MAC_TX_D;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Read the Mode/Format Word and Extract Parameters from the Header
    //
    pTX->TxMode = (MAC_TX_D[1].w8 >> 6) & 0x3;

    switch (pTX->TxMode)
    {
        case 0: // 802.11a
        {
            pTX->RATE.word   =  (MAC_TX_D[2].w8 & 0xF0) >> 4;
            pTX->LENGTH.word = ((MAC_TX_D[2].w8 & 0x0F) << 8) | MAC_TX_D[3].w8;

            XSTATUS( Tamale_aTX(pTX->RATE, pTX->LENGTH, MAC_TX_D+8, Nx, x, M) );
            break;
        }

        case 2: // 802.11b
        {
            pTX->RATE.word   =  (MAC_TX_D[2].w8 & 0xF0) >> 4;
            pTX->LENGTH.word = ((MAC_TX_D[2].w8 & 0x0F) << 8) | MAC_TX_D[3].w8;

            XSTATUS( Tamale_bTX(pTX->RATE, pTX->LENGTH, MAC_TX_D+8, Nx, x, M) );
            break;
        }

        case 3: // 802.11n
        {
            wiBoolean TxOpProtect =  MAC_TX_D[5].b2;
            wiBoolean Greenfield  =  MAC_TX_D[5].b3;
            wiBoolean Aggregation =  MAC_TX_D[6].b5;

            pTX->RATE.word   =  MAC_TX_D[2].w7;
            pTX->LENGTH.word = (MAC_TX_D[3].w8 << 8) | MAC_TX_D[4].w8;

            XSTATUS( Tamale_nTX(pTX->RATE, pTX->LENGTH, MAC_TX_D+8, Nx, x, M, TxOpProtect, Greenfield, Aggregation) );
            break;
        }

        default: 
            pReg->TxFault.word = 1;
            return STATUS(WI_ERROR_UNDEFINED_CASE);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Estimate Timing Shift
    //
    //  This is the delay from the start of the waveform to the start of the packet
    //
    if (!pTX->FixedTimingShift)
    {
        double tOFDM  = 350.0e-9; // filter + pipeline delay for OFDM
        double tCCK   = 461.4e-9; // filter + pipeline delay for DSSS/CCK
        double tPadFront;         // delay for zero padding in front of the packet
        double T;                 // Baud rate sample period

        switch (pTX->TxMode)
        {
            case 0:
            case 3: 
                T = 1 / 20e6;
                tPadFront = (wiReal)pTX->aPadFront * T;
                pTX->TimingShift = tPadFront + tOFDM;
                kStart = (unsigned)(80.0e6 * (tPadFront + tOFDM) + 0.5);
                kEnd   = (unsigned)(80.0e6 * (tPadFront + tOFDM + pTX->PacketDuration - T) + 0.5);
                break;
            case 2: 
                T = 1 / 11e6;
                tPadFront = (wiReal)pTX->bPadFront * T;
                pTX->TimingShift = tPadFront + tCCK;
                kStart = (unsigned)(80.0e6 * (tPadFront + tCCK) + 0.5);
                kEnd   = (unsigned)(80.0e6 * (tPadFront + tCCK + pTX->PacketDuration - T) + 0.5);
                break;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Estimate Timing Limits (TxStart, TxEnd)
    //
    if (!pDV->FixedTxTimingLimits)
    {
        wiUInt TxStart, TxEnd;

        switch (pTX->TxMode)
        {
            case 0:
            case 3:
                TxStart = pDV->aTxStart + 8*pReg->aTxDelay.w6;
                TxEnd   = pDV->aTxEnd   + 8*pReg->aTxExtend.w2;

                pDV->TxStartMinT80 = TxStart - pDV->aTxStartTolerance;
                pDV->TxStartMaxT80 = TxStart + pDV->aTxStartTolerance;
                pDV->TxEndMinT80   = TxEnd - pDV->aTxEndTolerance;
                pDV->TxEndMaxT80   = TxEnd + pDV->aTxEndTolerance;
                break;
            
            case 2:
                TxStart = pDV->bTxStart + 8*pReg->bTxDelay.w6;
                TxEnd   = pDV->bTxEnd   + 8*pReg->bTxExtend.w2;

                pDV->TxStartMinT80 = TxStart - pDV->bTxStartTolerance;
                pDV->TxStartMaxT80 = TxStart + pDV->bTxStartTolerance;
                pDV->TxEndMinT80   = TxEnd - pDV->bTxEndTolerance;
                pDV->TxEndMaxT80   = TxEnd + pDV->bTxEndTolerance;
                break;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute Checksums
    //
    //  A checksum is computed on the Baud rate waveform and on the PrD output to allow
    //  external test routines to check for changes in the signal processing models.
    //
    //  Note that the Fletcher checksum is sensitive to the number of zeros so changing
    //  aPadFront and aPadBack will affect the result.
    //
    if (pTX->Checksum.Enable)
    {
        pTX->Checksum.MAC_TX_D = 0;
        wiMath_FletcherChecksum(  Length * sizeof(wiUWORD), MAC_TX_D,   &(pTX->Checksum.MAC_TX_D) );

        pTX->Checksum.a = 0;
        wiMath_FletcherChecksum( pTX->Na * sizeof(wiUWORD), pTX->a,     &(pTX->Checksum.a) );

        pTX->Checksum.u = 0;
        wiMath_FletcherChecksum( pTX->Nz * sizeof(wiUWORD), pTX->uReal, &(pTX->Checksum.u) );
        wiMath_FletcherChecksum( pTX->Nz * sizeof(wiUWORD), pTX->uImag, &(pTX->Checksum.u) );

        pTX->Checksum.v = 0;
        wiMath_FletcherChecksum( pTX->Nv * sizeof(wiWORD),  pTX->vReal, &(pTX->Checksum.v) );
        wiMath_FletcherChecksum( pTX->Nv * sizeof(wiWORD),  pTX->vImag, &(pTX->Checksum.v) );


        pTX->Checksum.x = 0;
        wiMath_FletcherChecksum( pTX->Nx * sizeof(wiWORD),  pTX->xReal, &(pTX->Checksum.x) );
        wiMath_FletcherChecksum( pTX->Nx * sizeof(wiWORD),  pTX->xImag, &(pTX->Checksum.x) );

        wiPrintf("Checksums = %08X %08X %08X %08X %08X\n", pTX->Checksum.MAC_TX_D, pTX->Checksum.a,
            pTX->Checksum.u, pTX->Checksum.v, pTX->Checksum.x);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Create Trace Markers
    //
    if (pTX->Trace.kMarker[1] < (unsigned)pTX->Nz) pTX->traceTx[pTX->Trace.kMarker[1]].Marker1 = 1;
    if (pTX->Trace.kMarker[2] < (unsigned)pTX->Nz) pTX->traceTx[pTX->Trace.kMarker[2]].Marker2 = 1;
    if (pTX->Trace.kMarker[3] < (unsigned)pTX->Nz) pTX->traceTx[pTX->Trace.kMarker[3]].Marker3 = 1;
    if (pTX->Trace.kMarker[4] < (unsigned)pTX->Nz) pTX->traceTx[pTX->Trace.kMarker[4]].Marker4 = 1;

    if (kStart < (unsigned)pTX->Nz) pTX->traceTx[kStart].PktStart = 1;
    if (kEnd   < (unsigned)pTX->Nz) pTX->traceTx[kEnd  ].PktEnd = 1;

    pDV->kEnd   = kEnd;
    pDV->kStart = kStart;

    return WI_SUCCESS;
}
// end of Tamale_TX()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

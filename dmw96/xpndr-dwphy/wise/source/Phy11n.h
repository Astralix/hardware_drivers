//
//  Phy11n.h -- 802.11n MIMO-OFDM Receiver (2006 Edition)
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//

#ifndef __PHY11N_H
#define __PHY11N_H

#include "wiType.h"
#include "wiMatrix.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//  Maximum number of transmit and receive antennas (streams)
//
#define PHY11N_TXMAX  4
#define PHY11N_RXMAX  4

//  Maximum number of code bits per subcarrier per spatial stream (CBPSS)
//  and maximum number of spatial streams (SS)
//
#define PHY11N_MAX_CBPSS (52*6)
#define PHY11N_MAX_SS    4
#define PHY11N_MAX_CONSTELLATION 64

//  Maximum number of iterations for turbo-processing
//
#define PHY11N_MAX_ITERATIONS 4

//  Size for the SOVA maximum window and "Position" circular buffer
//
#define PHY11N_MAX_SOVA_WINDOW 64
#define PHY11N_SOVA_POSITION_LENGTH (PHY11N_MAX_ITERATIONS + 4)

//  Max filename length for BF test vector file
#define PHY11N_BF_MAX_FILENAME 256

typedef enum Phy11nSymbolFormatE
{
    PHY11N_SYMBOL_IS_L_OFDM,
    PHY11N_SYMBOL_IS_LSIG,
    PHY11N_SYMBOL_IS_HTSIG,
    PHY11N_SYMBOL_IS_HT_OFDM,
} Phy11nSymbolFormat_t;

typedef enum Phy11nPhaseTrackingMethodE
{
    PHY11N_PHASETRACK_NONE     = 0,
    PHY11N_PHASETRACK_DEMAPPER = 1,
    PHY11N_PHASETRACK_GENIE    = 2,
    PHY11N_PHASETRACK_DEMAPPER_REJECTION = 3
} Phy11nPhaseTrackingMethod_t;

//  Symbol LLRs
//  The array dimensions are N_SS x N_CBPSS with the type defined for the maximum of each
//
typedef wiReal Phy11nSymbolLLR_t[PHY11N_MAX_SS][PHY11N_MAX_CBPSS];


//  MIMO Demapper Function Pointer
//
//  All demapper functions should follow the common prototype so they can be selected via
//  Phy11n_DemapperMethod(). The parameters are (Y, H, extrLLR, priorLLR, N_BPSC, Sn)
//
//      Y        -- received signal matrix
//      H        -- estimated channel matrix
//      extrLLR  -- output extrinsic soft bit information matrix
//      priorLLR -- input priori soft bit information matrix
//      N_BPSC   -- number of bits per symbol per stream (array)
//      Sn       -- estimated noise level (standard deviation)
//
typedef wiStatus (* Phy11nDemapperFn)(wiCMat *, wiCMat *, Phy11nSymbolLLR_t, Phy11nSymbolLLR_t, wiInt [], wiReal);


//===========================================================================================
//  TRANSMITTER PARAMETER STRUCTURE
//===========================================================================================
typedef struct
{
    int N_SS;                    // Number of spatial streams, given by MCS
    int N_STS;                   // Number of space-time streams, determined by STBC field in HT-SIG
    int N_TX;                    // Number of TX Ants
    
    //HT_SIG Parameters in HT Preambles, see March 06 spec. P. 185
    // totally 48 bits, divided into HT-SIG1 and HT-SIG2, each 24 bits
    int MCS;                  // 7 bits, modulation and coding schemes, see tables starting from March 06 spec. P.254
    int HT_LENGTH;            // 16 bits, number of bytes in PSDU, between 0-65535
    int STBC;                 // 2 bits, difference between N_STS and N_SS
    int N_HT_ELTF;            // 2 bits, # of extension spatial streams, N_ESS
    int N_HT_DLTF;            // 2 bits, # of extension spatial streams, N_ESS
    int N_SYM;                // Number of OFDM symbols for DATA  (with all spatial streams)
    int N_DBPS;               // Number of data bits per OFDM symbol (with all spatial streams)
    int N_CBPS;               // Number of coded bits per OFDM symbol (with all spatial streams)
    int N_TBPS;               // Number of total coded bits per subcarrier (with all spatial streams)
    int N_CBPSS[PHY11N_TXMAX];// Number of coded bits per OFDM symbol for each spatial stream
    int N_BPSC[PHY11N_TXMAX]; // Number of coded bits per subcarriers for each spatial stream
    int N_ES;                 // Number of FEC encoders
	int N_ESS;                // Number of extension spatial streams

    int CodeRate12;           // CodeRate * 12; i.e., for rate 1/2 code, CodeRate12 = 6

    int aPadFront, aPadBack;  // Zero padding on the front and back of the waveform

    wiBoolean ThreadIsInitialized;
    
    wiIntArray_t a;           // Serialized PSDU
    wiIntArray_t b;           // Scrambled PSDU
    wiIntArray_t c;           // Coded data bits

    int Nsp[4];               // number of bits per spatial stream
    int *stsp[4];             // Nss Streams after stream parser
    int *stit[4];             // Permuted coded bits after interleaver  

    int Nmp;
    wiComplex *stmp[4];          // Nss streams after QAM modulation
    wiComplex *stSTBC[4];       // N_STS streams after Space-time Block Coding
    wiComplex *stSM[4];         // N_TX streams after Spatial mapping, including Beamforming

    int Nx;
    wiComplex *x[PHY11N_TXMAX+1];
    
    wiBoolean FixedScramblerState; // Do not generate pseudo-random scrambler state
    wiBoolean ScramblerDisabled;   // Disable the scrambler
    wiBoolean InterleaverDisabled; // Disable the interleaver/permuter

    int PilotShiftRegister;       // State of the transmit pilot sign 
    int ScramblerShiftRegister;   // State of the transmit scrambler

    wiComplex IDFT_W[64];       // Inverse discrete Fourier transform weights

    wiBoolean dumpTX;
    int LastAllocationSize[8];

}  Phy11n_TxState_t;


typedef struct // Soft Output Viterbi Algorithm ////////////////////////////////////
{
    wiReal Le_S[2][64][PHY11N_MAX_SOVA_WINDOW + 1];
    wiReal Le_A[2][64][PHY11N_MAX_SOVA_WINDOW + 1];
    wiReal Le_B[2][64][PHY11N_MAX_SOVA_WINDOW + 1];

    wiInt bit_S[2][64][PHY11N_MAX_SOVA_WINDOW + 1];
    wiInt bit_A[2][64][PHY11N_MAX_SOVA_WINDOW + 1];
    wiInt bit_B[2][64][PHY11N_MAX_SOVA_WINDOW + 1];

    wiInt state[2][64][PHY11N_MAX_SOVA_WINDOW + 1];

    wiInt Position1[PHY11N_SOVA_POSITION_LENGTH];
    wiInt Position2[PHY11N_SOVA_POSITION_LENGTH];
    wiInt LastState;

} Phy11n_SOVA_t;

typedef struct // BCJR Decoder /////////////////////////////////////////////////////
{
    wiRealArray_t alpha[64];
    wiRealArray_t beta[64];

} Phy11n_BCJR_t;

typedef struct // List Sphere Decoder Parameters ///////////////////////////////////
{
	wiInt     NCand;          // Number of Candidates in the list of sphere decoding
	wiReal    alpha;          // Radius adjustment factor for list sphere decoding;
	wiReal    FixedRadius;    // FixRadius Value if alpha = 0.0;
	wiReal    *d2_lsd;		  // Euclidean distance metric of candidates
	wiInt     *b_lsd;         // bit map of list candidates
	wiInt     Nd;			  // size of d2_lsd; 
	wiInt     Nb;			  // size of b_lsd;

} Phy11n_ListSphereDecoder_t;

typedef struct // Single Tree Search (STS) Sphere Decoder //////////////////////////
{
	wiInt    LLRClip;        // 0: No Clipping, 1: LLR clipping
	wiReal   LMax;           // Clipping Level		
	wiInt    Sorted;           // 0: No sort 1: Ascending ordering
	wiInt    ModifiedQR;     // 0: Normal column-wise QR 1: Eli's modified QR on pairs of columns 
	wiInt    MMSE;           // 0: ZF-QR  1: MMSE-QR
	wiInt    Unbiased;       // 0: Biased MMSE  1: Unbiased MMSE (self-interference free)
	wiInt    GetStatistics;  // 1: Get the throughput statistics;
	wiReal	 AvgCycles;      // Average # of Cycles per search over a packet
	wiReal   AvgNodes;       // Average # of Nodes visited per search over a packet
	wiReal   AvgLeaves;      // Average # of leaves reached per search over a packet
	wiInt    NSTS;           // Number of STS searches per packet
	wiReal   *NCyclesPSTS;   // Number of Cycles per STS search
	wiReal   *PH2PSTS;		 // Power of channel estimation per subcarrier (STS search)
	wiReal   *R2RatioPSTS;  // Ratio of largest to smallest norm of H per subcarrier
	wiInt    sym;            // Current symbol;
	wiInt    Terminated;     // Early termination of tree search 
	wiReal   MaxCycles;      // MaxCycles to terminate the tree search
	wiInt    Hybrid;         // Hybrid solution of STS and MMSE 
	wiInt    BlockSchedule;  // Run time constraint on the whole block (symbol)
	wiReal   MeanCycles;     // Target average cycles per subcarrier
	wiReal   MinCycles;      // Target minimum cycles per cycles
	wiInt    Trunc[52];		 // truncated tree search (early termination)
	wiInt    Norm1;          // 0: Norm2 1: Norm1  2: INf-Norm;
	wiReal   LMax1;			 // Radius of Norm1 or Inf-Norm
	wiInt    ExactLLR;       // Use Norm2 in LLR calculation after the Norm1 in tree search
} Phy11n_SingleTreeSearchSphereDecoder_t;

typedef struct
{
	wiInt   Intgy; // y
	wiInt	IntgR;
	wiInt	Intgd;
	wiInt	Intgd2;
	wiInt	Intgn;
	wiInt	IntgM;
	wiInt   Shiftd; 
	wiInt   Shiftd2;	
	wiInt   FLScale;
}Phy11n_SingleTreeSearchSphereDecoderFxP_t;

typedef struct // Breadth-first Decoders ///////////////////////////////////////////
{
	wiInt    K,K1;           // Number of survivor paths
	wiInt    mode;           // BF demapper mode (depends on BF type)
	wiInt    normtype;       // 1:use L1 norm, 2:use L2 norm (CSESD only)
	wiInt    fixpt;          // fixed-point 0:disable, >0: #frac bits (CSESD only)
	wiInt    distlim;        // distance limit: 0:disable, >0:  #int bits (CSESD only)
	wiInt    TestVecSymbols; // number of ofdm symbols for test vectors (CSESD only)
	char     TestVecFile[PHY11N_BF_MAX_FILENAME]; // test vector output filename
	wiInt    nthreads;       // number of simulator threads
	void*    bfextra;        // pointer to BF struct (stats, etc.)

} Phy11n_BFDemapper_t;


//============================================================================================
//  RECEIVER PARAMETER STRUCTURE
//============================================================================================
typedef struct
{
    int N_SS;                    // Number of spatial streams, given by MCS
    int N_STS;                   // Number of space-time streams, determined by STBC field in HT-SIG
    int N_TX;                    // Number of TX Ants
    int N_RX;                    // Number of RX Ants
    
    wiUWORDArray_t PSDU;         // can be as long as 65535, data length given by HT_LENGTH in HT-SIG
    int SERVICE;                 // Service field (16-bits, typically 0), see March 06 spec. P. 197

    int MCS;                     // 7 bits, modulation and coding schemes, see tables starting from March 06 spec. P.254
    int HT_LENGTH;               // 16 bits, number of bytes in PSDU, between 0-65535
    int STBC;                    // 2 bits, difference between N_STS and N_SS

    int N_HT_DLTF;                // 2 bits, # of extension spatial streams, N_ESS
    int N_SYM;                   // Number of OFDM symbols for DATA (with all spatial streams)
    int N_DBPS;                  // Number of data bits per OFDM symbol (with all spatial streams)
    int N_CBPS;                  // Number of coded bits per OFDM symbol (with all spatial streams)
    int N_TBPS;                // Number of total coded bits per subcarrier (with all spatial streams)
    int N_CBPSS[4];              // Number of coded bits per OFDM symbol for each spatial stream
    int N_BPSC[4];               // Number of coded bits per subcarriers for each spatial stream
    int N_ES;                    // Number of FEC encoders

    int CodeRate12;           // CodeRate * 12, 12: uncoded, 6: 1/2, 8: 2/3, 9: 3/4, 10: 5/6
    int TotalIterations;         // total number of iterations for iterative decoding
    int DemapperType;       // MIMO demapper type: 0: ML, 1: MMSE
    int DecoderType;        // 0: Soft VA w/o iteation, 1: Hard VA w/o iteration
                            // 2: SOVA w/ iteration, 3: BCJR w/ iteration
    int CFOEstimation;      // 0: turn off CFO
                            // 1: use samples where frame sync starts till the end of short preamble to estimate offset
                             // 2: use only last three training symbols to estimate offset
    int EnableFrameSync;     // 0: use zero offest, 1: use estimated offset
    int FrameSyncDelay;
    int SyncShift;          // bias for frame sync position
    wiBoolean RandomFrameSyncStart; // random delay to start of frame sync (model variable CS+AGC delays)

    int PhaseTrackingType;

    wiBoolean ThreadIsInitialized;
    
    int *a;  int Na;       // Serialized PSDU
    int *b;  int Nb;       // Scrambled PSDU
    wiReal   *Lc;    int Nc;       // ~LLR for the coded bits
    wiReal   *Lc_e;  // feedback extrinsic LLR 
        
    wiReal *stsp[4]; int Nsp[4]; // input to stream deparser, sp: stream parser
    wiReal *stsp_e[4];           // feedback extrinsic LLR after RX stream parser
    wiReal *stit[4];             // input to deinterleaver
    wiReal *stit_e[4];           // feedback extrinsic LLR after RX interleaver

    wiComplex *stmp[4]; int Nmp; // Nss streams after QAM modulation

    wiComplex *y[4]; int Ny; // received baseband signal
    wiReal    NoiseRMS;        // rms noise (used as standard deviation)

    wiCMat    Y52;             // received data packet in frequency domain without pilot
    wiCMat    Y56;             // received data packet in frequency domain with pilot
    wiCMat    H52[52];         // channel matrix for each data subcarrier
    wiCMat    H56[56];         // channel matrix for each modulated subcarrier
    wiCMat    InitialH56[56];  // initial channel estimation matrix based on HT-LTF

    int IterationMethod;       // 0:frame-by-frame iteration  1:symbol-by-symbol iteration

    wiComplex DFT_W[64];       // Discrete Fourier transform weights

    struct
    {
        wiReal b, VCO;     // internal memory
        wiReal K1, K2;     // loop filter gains (proportional and integral)

    } DPLL;

    struct
    {
        int    Type;
        wiReal SoftProbTh;
    } ChanlTracking;

    Phy11n_SOVA_t                         *SOVA;
    Phy11n_BCJR_t                          BCJR;
    Phy11n_ListSphereDecoder_t             ListSphereDecoder;
    Phy11n_SingleTreeSearchSphereDecoder_t SingleTreeSearchSphereDecoder;
	Phy11n_SingleTreeSearchSphereDecoderFxP_t SingleTreeSearchSphereDecoderFxP;
    Phy11n_BFDemapper_t                    BFDemapper;

    int LastAllocationSize[6];

}  Phy11n_RxState_t;


//=================================================================================================
//
//  FUNCTION PROTOTYPES
//
//=================================================================================================
wiStatus Phy11n(int command, void *pData);

wiStatus Phy11n_InitializeThread(unsigned int ThreadIndex);
wiStatus Phy11n_CloseThread     (unsigned int ThreadIndex);

Phy11n_TxState_t * Phy11n_TxState(void);
Phy11n_RxState_t * Phy11n_RxState(void);

wiStatus Phy11n_TX_FreeMemory(Phy11n_TxState_t *pTX, wiBoolean ClearPointerOnly);
wiStatus Phy11n_RX_FreeMemory(Phy11n_RxState_t *pRX, wiBoolean ClearPointerOnly);
wiStatus Phy11n_TX_AllocateMemory(void);
wiStatus Phy11n_RX_AllocateMemory(void);

double   Phy11n_LogExpAdd(double a, double b);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Transmit functions in Phy11n_TX.c
//
wiStatus Phy11n_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms);
wiStatus Phy11n_TX_InsertLegacyPreamble(int dt);
wiStatus Phy11n_TX_InsertLSIG(int dt);
wiStatus Phy11n_TX_InsertHTSIG(int dt);
wiStatus Phy11n_TX_InsertHTPreamble(int dt);
wiStatus Phy11n_TX_Scrambler(int n, int a[], int b[]);
wiStatus Phy11n_TX_Encoder(int Nbits, int x[], int y[], int m);
wiStatus Phy11n_TX_StreamParser(int Nc, int c[], int *stsp[], int N_BPSC[], int N_SS);
wiStatus Phy11n_TX_Interleave(int N_CBPSS, int N_BPSC, int Iss ,int x[], int y[], int Nsp);
wiStatus Phy11n_TX_QAM(wiComplex Y[], int x[], int N_BPSC, int Nmp, int HT_SIG);
wiStatus Phy11n_TX_STBC(wiComplex *Y[], int N_STS, wiComplex *X[], int N_SS, int Nmp);
wiStatus Phy11n_TX_STEncode(wiComplex *Y1, wiComplex *Y2, wiComplex *X, int Nmp);
wiStatus Phy11n_TX_SpatialMap(wiComplex *Y[], int N_TX, wiComplex *X[], int N_STS, int Nmp);
wiStatus Phy11n_TX_Interleave_NonHT(int N_CBPS, int N_BPSC, int x[], int y[]);
wiStatus Phy11n_TX_IDFT(wiComplex X[], wiComplex y[]);
wiStatus Phy11n_PilotTonePhase(wiReal S[][4], int N_STS, int nSym, Phy11nSymbolFormat_t Format);


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Receiver functions in Phy11n_RX.c
//
wiStatus Phy11n_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[]);

wiStatus Phy11n_RX_Frame(void);
wiStatus Phy11n_RX_Symbol(void);

wiStatus Phy11n_RX_Init(void);
wiStatus Phy11n_TX_Init(void);// used for debug

wiStatus Phy11n_RX_Descrambler(int n, int a[], int b[]);
wiStatus Phy11n_RX_ViterbiDecoder(int NumBits, wiReal pInputLLR[], int OutputData[], int CodeRate12, 
                                  int TruncationDepth, wiBoolean HardDecoder);
wiStatus Phy11n_RX_StreamDeparser(int Nc, wiReal Lc[], wiReal *stsp[], int N_BPSCS[], int N_SS);
wiStatus Phy11n_RX_Deinterleave(int N_CBPSS, int N_BPSC, int Iss ,wiReal x[], wiReal y[], int Nsp);
wiStatus Phy11n_RX_QAM_Soft(wiComplex d, wiReal *p, wiReal noise_pow, wiInt N_BPSC);

wiStatus Phy11n_RX_StreamParser(int Nc, wiReal Lc[], wiReal *stsp[], int N_BPSC[], int N_SS);
wiStatus Phy11n_RX_Interleave(int N_CBPSS, int N_BPSC, int Iss ,wiReal x[], wiReal y[], int Nsp);

wiStatus Phy11n_RX_InitialChanlEstimation(wiComplex **X, int k0, wiCMat *C56, wiCMat *C52);

wiStatus Phy11n_RX_BCJR(int Nbits, wiReal L[], int x[], int m, wiReal extrinsic[]);
wiStatus Phy11n_RX_SOVA(int Nbits, wiReal L[], int OutputData[], int CodeRate12, int Depth, wiReal Le[], 
                        wiInt sym, wiInt N_SYM, wiInt *pLastState);

wiStatus Phy11n_RX_FrameSync_CoarseCFO( wiComplex *R[], wiInt SyncDelay, wiInt k0, 
                                        wiInt *time_offset, wiReal *CFO, wiInt FrameSync_ctrl, wiInt CFO_ctrl);
wiStatus Phy11n_RX_PhaseCorrection(wiReal VCOoutput, wiComplex Yf[4][56], wiCMat Y);
wiStatus Phy11n_RX_HardDemapLLR(wiReal LLR[], wiInt N_BPSC, wiComplex *x, wiReal PsThreshold, wiBoolean *StrongProb);

wiStatus Phy11n_RX_PhaseErrorDetector(wiReal *CarrierPhase, Phy11nSymbolLLR_t LLR, wiComplex Y[][56], 
                                      wiCMat *H56, wiInt nSym, int Method);

wiStatus Phy11n_RX_PilotChannelTracking(int nSym, wiComplex Y[][56], wiCMat H56[]);
wiStatus Phy11n_RX_DataChannelTracking_Genie(wiComplex Yf[][56], wiCMat H56[], wiInt nSym);
wiStatus Phy11n_RX_DataChannelTracking_LLR(wiComplex Yf[][56], Phy11nSymbolLLR_t SoInfo, wiCMat H56[]);

wiStatus Phy11n_RX_PosterioriProb(wiReal LLR[], wiInt N_BPSC, wiReal Ps[], wiComplex S[], wiComplex *xBar);

wiStatus Phy11n_RX_DFT (wiComplex x[], wiComplex Y[]);


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Functions in Phy11n_Demapper.c
//
wiStatus Phy11n_MMSE_SingleInversion(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, 
                                     Phy11nSymbolLLR_t PriorLLR, wiInt N_BPSC[], wiReal NoiseStd);

wiStatus Phy11n_DemapperMethod(int DemapperType, int Iteration, Phy11nDemapperFn *pFn);

wiStatus Phy11n_MIMO_MMSE_STBC(wiCMat Y52[], wiCMat H52[], Phy11nSymbolLLR_t ExtrinsicLLR[],
                               Phy11nSymbolLLR_t PriorLLR[], wiInt N_BPSC_i[], wiReal NoiseStd, wiInt STBC);
//
//  Additional demapper functions in Phy11n_SphereDecoder.c
//  and                              Phy11n_SphereDecoderFxP.c
//
wiStatus Phy11n_ListSphereDecoder(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                  Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);

wiStatus Phy11n_SingleTreeSearchSphereDecoder(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);

wiStatus Phy11n_SingleTreeSearchSphereDecoderFxP(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);

wiStatus Phy11n_SingleTreeSearchSphereDecoder_Hybrid(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);

wiStatus Phy11n_SingleTreeSearchSphereDecoderFxP_Hybrid(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);

//
//  Additional demapper functions in Phy11n_BFDemapper.cpp
//
wiStatus Phy11n_KbvtDemapper(wiCMat* Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);
wiStatus Phy11n_KbestDemapper(wiCMat* Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);
wiStatus Phy11n_LFSDemapper(wiCMat* Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std);
wiStatus Phy11n_MLDemapper(wiCMat* Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std); // reference depth-first ML demapper (SESD)

wiStatus Phy11n_BFDemapper_Start(int NumberOfThreads);
wiStatus Phy11n_BFDemapper_End(wiMessageFn);

void Phy11n_GetLFSDConfig(char* buf, int blen, int Nss, int Nbpsc, int K);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

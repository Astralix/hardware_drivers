#include <stdio.h>
#include <math.h>
#include "wiType.h"


#define   FFTWLn   15         //Word Length of general data in DataPath
#define   FFTBFWLn   17         //Word Length of butterfly/multiplier output
#define   FFTTFWLn   10         //Word Length of twiddle factors
#define   FFTTFFBn   8         //Number of Fractional Bits of Twiddle Factors

#define   FFTWL      w15         //Word Length of general data in DataPath

#define   FFTBFWL   w17         //Word Length of butterfly/multiplier output
#define   FFTBFWL2  w16
#define   FFTTFWL   w10         //Word Length of twiddle factors

#define   BFWLPLUSONE            w18   //Word Length used in complex multiplier
#define   BFWLPLUSTFWL         w27
#define   BFWLPLUSTFWLPLUSONE   w28

static void FFTFourPointButterfly(wiWORD* inr, wiWORD* ini, wiWORD* outr, wiWORD* outi);
static void FFTTwoPointButterfly(wiWORD *inr, wiWORD *ini, wiWORD *outr, wiWORD *outi);
static void FFTComplexMultiplier(wiWORD Ar, wiWORD Ai, wiWORD Br, wiWORD Bi, wiWORD* Yr, wiWORD* Yi);
static int  FFTSaturation(wiWORD x, int ifft);
static void fixFFT64(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft, int gi, int dmw96);
static void fixFFT128(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft, int gi);
static void fixFFT256(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft, int gi);

void Tamale_FFT_int ( int TxBRx, int BW, int GI, int dmw96, wiWORD *FFTTxDIR, wiWORD *FFTTxDII, wiWORD *FFTRxDIR, wiWORD *FFTRxDII,
		     wiWORD *FFTTxDOR,  wiWORD *FFTTxDOI, wiWORD *FFTRxDOR,  wiWORD *FFTRxDOI );

void Tamale_FFT ( int TxBRx,
                  wiWORD FFTTxDIR[],
                  wiWORD FFTTxDII[],
                  wiWORD FFTRxDIR[],
                  wiWORD FFTRxDII[],
                  wiWORD *FFTTxDOR,
                  wiWORD *FFTTxDOI,
                  wiWORD *FFTRxDOR,
                  wiWORD *FFTRxDOI )
{

      Tamale_FFT_int(TxBRx,
                 0,  //0-> 64 pt, 1 -> 128pt, 2 -> 256pt
                 1,  //0-> short gi, 1 -> long gi, only used when in IFFT mode. Ignored when in FFT mode.
                 1, // 1-> dmw96 64 pt FFT, 0 -> dmw98 FFT.
                 FFTTxDIR,
                 FFTTxDII,
                 FFTRxDIR,
                 FFTRxDII,
                 FFTTxDOR,
                 FFTTxDOI,
                 FFTRxDOR,
                 FFTRxDOI
                 );


}


void Tamale_FFT_int ( int TxBRx,
                      int BW,
                      int GI,// Only a valid input for IFFT.
                      int dmw96,
                      wiWORD *FFTTxDIR,
                      wiWORD *FFTTxDII,
                      wiWORD *FFTRxDIR,
                      wiWORD *FFTRxDII,
                      wiWORD *FFTTxDOR,
                      wiWORD *FFTTxDOI,
                      wiWORD *FFTRxDOR,
                      wiWORD *FFTRxDOI )
{
    int k;

    int outp_sz;


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
       if (BW == 0){  // Implementation for 20 MHz

         if (dmw96 == 1)
            outp_sz = 64;
         else
            outp_sz = 68;

         fixFFT64(FFTRxDIR, FFTRxDII, FFTRxDOR, FFTRxDOI, 0, GI, dmw96);

         for (k=0; k<outp_sz; k++)
         {
            FFTRxDOR[k].word = FFTRxDOR[k].w15 >> 4;
            FFTRxDOI[k].word = FFTRxDOI[k].w15 >> 4;
         }

       }
       else if (BW == 1) // Implementation for 40 MHz
       {
          fixFFT128(FFTRxDIR, FFTRxDII, FFTRxDOR, FFTRxDOI, 0,GI);

	      for (k=0; k<132; k++)
            {
              if ( (k == 61) || (k==66) || (k==76) || (k==81) )
		          printf("before:k=%d,FFTRxDOR = %d, FFTRxDOI = %d\n",k,FFTRxDOR[k].word,FFTRxDOI[k].word);

	          FFTRxDOR[k].word = FFTRxDOR[k].w15 >> 4;
              FFTRxDOI[k].word = FFTRxDOI[k].w15 >> 4;

              if ( (k == 61) || (k==66) || (k==76) || (k==81) )
		         printf("after:k=%d,FFTRxDOR = %d, FFTRxDOI = %d\n",k,FFTRxDOR[k].word,FFTRxDOI[k].word);

	        }
       }
       else if (BW == 2) // Implementation for 80 MHz
       {
	      fixFFT256(FFTRxDIR, FFTRxDII, FFTRxDOR, FFTRxDOI, 0,GI);

	      for (k=0; k<260; k++)
          {
	         FFTRxDOR[k].word = FFTRxDOR[k].w15 >> 4;
             FFTRxDOI[k].word = FFTRxDOI[k].w15 >> 4;

	      }
       }
    }
    else // transmitter (IFFT)
    {

      if ( BW == 0 ){ // Implementation for 20 MHz

        if (GI == 0)
	      outp_sz = 72; // short gi, # of samples per symbol
        else if (GI == 1)
          outp_sz = 80; // long gi, # of samples per symbol



         fixFFT64(FFTTxDIR, FFTTxDII, FFTTxDOR, FFTTxDOI, 1, GI, dmw96);

         for (k=0; k<outp_sz; k++)
         {
            if (FFTTxDOR[k].w15 != FFTTxDOR[k].w10) FFTTxDOR[k].word = (FFTTxDOR[k].word<0)? -512:511;
            if (FFTTxDOI[k].w15 != FFTTxDOI[k].w10) FFTTxDOI[k].word = (FFTTxDOI[k].word<0)? -512:511;
         }

       }
      else if (BW == 1) { // Implementation for 40 MHz

        if (GI == 0)
	      outp_sz = 144; // short gi, # of samples per symbol
        else if (GI == 1)
          outp_sz = 160; // long gi, # of samples per symbol

         fixFFT128(FFTTxDIR, FFTTxDII, FFTTxDOR, FFTTxDOI, 1, GI);

         for (k=0; k<outp_sz; k++)
         {
            if (FFTTxDOR[k].w15 != FFTTxDOR[k].w10) FFTTxDOR[k].word = (FFTTxDOR[k].word<0)? -512:511;
            if (FFTTxDOI[k].w15 != FFTTxDOI[k].w10) FFTTxDOI[k].word = (FFTTxDOI[k].word<0)? -512:511;
         }
      }
      else if (BW == 2) { // Implementation for 80 MHz

        if (GI == 0)
	      outp_sz = 288; // short gi, # of samples per symbol
        else if (GI == 1)
          outp_sz = 320; // long gi, # of samples per symbol

        fixFFT256(FFTTxDIR, FFTTxDII, FFTTxDOR, FFTTxDOI, 1,GI);

        for (k=0; k<outp_sz; k++)
        {
            if (FFTTxDOR[k].w15 != FFTTxDOR[k].w10) {
                    FFTTxDOR[k].word = (FFTTxDOR[k].word<0)? -512:511;
                    printf("SATURATION!\n");
		     }

            if (FFTTxDOI[k].w15 != FFTTxDOI[k].w10) {
				   printf("SATURATION!\n");
                   FFTTxDOI[k].word = (FFTTxDOI[k].word<0)? -512:511;
		    }
        }
      }
    }
}

// ================================================================================================
// fixFFT64
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static void fixFFT64(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft,int gi, int dmw96)
{
    int i, j, m;
    wiWORD BFIr[4], BFIi[4], BFOr[4], BFOi[4];
    wiWORD memr[64], memi[64];
    wiWORD cmplx_out_r, cmplx_out_i;

    wiWORD out1r[64], out1i[64];
    wiWORD out2r[4], out2i[4];
    int ival;

    int stage0_shift, stage1_shift, stage2_shift;

	int iord0[4][16] = {
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
	{16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
	{32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47},
	{48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63}};

	int iord0_flip[4][16] = {
	{32,33,34,35,36,37,38,39,40,41,
	42,43,44,45,46,47},
	{48,49,50,51,52,53,54,55,56,57,
	58,59,60,61,62,63},
	{0,1,2,3,4,5,6,7,8,9,
	10,11,12,13,14,15},
	{16,17,18,19,20,21,22,23,24,25,
	26,27,28,29,30,31}};

	int iord1[4][16] = {
	{0,1,2,3,16,17,18,19,32,33,34,35,48,49,50,51},
	{4,5,6,7,20,21,22,23,36,37,38,39,52,53,54,55},
	{8,9,10,11,24,25,26,27,40,41,42,43,56,57,58,59},
	{12,13,14,15,28,29,30,31,44,45,46,47,60,61,62,63}};

	int iord2[4][64] = {
	{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60},
	{1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61},
	{2,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62},
	{3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63}};


	int iord3[4][64] = {
	{56,24,52,20,56,24,52,20,0,16,32,48,4,20,36,52,8,24,40,12,28,44,60,0,16,
	32,48,4,20,36,52,8,40,56,12,28,44,60,0,16,32,48,4,20,36,8,24,40,56,12,
	28,44,60,0,16,32,48,4,36,52,8,24,40,56},
	{57,25,53,21,57,25,53,21,1,17,33,49,5,21,37,53,9,25,41,13,29,45,61,1,17,
	33,49,5,21,37,53,9,41,57,13,29,45,61,1,17,33,49,5,21,37,9,25,41,57,13,
	29,45,61,1,17,33,49,5,37,53,9,25,41,57},
	{58,26,54,22,58,26,54,22,2,18,34,50,6,22,38,54,10,26,42,14,30,46,62,2,18,
	34,50,6,22,38,54,10,42,58,14,30,46,62,2,18,34,50,6,22,38,10,26,42,58,14,
	30,46,62,2,18,34,50,6,38,54,10,26,42,58},
	{59,27,55,23,59,27,55,23,3,19,35,51,7,23,39,55,11,27,43,15,31,47,63,3,19,
	35,51,7,23,39,55,11,43,59,15,31,47,63,3,19,35,51,7,23,39,11,27,43,59,15,
	31,47,63,3,19,35,51,7,39,55,11,27,43,59}};

	int iord3_dmw96[4][64] = {
	{0,16,32,48,4,20,36,52,8,24,40,56,12,28,44,60,0,16,32,48,4,20,36,52,8,
	24,40,56,12,28,44,60,0,16,32,48,4,20,36,52,8,24,40,56,12,28,44,60,0,16,
	32,48,4,20,36,52,8,24,40,56,12,28,44,60},
	{1,17,33,49,5,21,37,53,9,25,41,57,13,29,45,61,1,17,33,49,5,21,37,53,9,
	25,41,57,13,29,45,61,1,17,33,49,5,21,37,53,9,25,41,57,13,29,45,61,1,17,
	33,49,5,21,37,53,9,25,41,57,13,29,45,61},
	{2,18,34,50,6,22,38,54,10,26,42,58,14,30,46,62,2,18,34,50,6,22,38,54,10,
	26,42,58,14,30,46,62,2,18,34,50,6,22,38,54,10,26,42,58,14,30,46,62,2,18,
	34,50,6,22,38,54,10,26,42,58,14,30,46,62},
	{3,19,35,51,7,23,39,55,11,27,43,59,15,31,47,63,3,19,35,51,7,23,39,55,11,
	27,43,59,15,31,47,63,3,19,35,51,7,23,39,55,11,27,43,59,15,31,47,63,3,19,
	35,51,7,23,39,55,11,27,43,59,15,31,47,63}};

	int oord4[68] = {58,27,52,21,58,27,52,21,2,18,34,50,6,22,38,54,10,26,42,14,30,46,62,3,19,35,51,7,23,39,55,11,43,59,15,31,47,63,0,16,
	32,48,4,20,36,8,24,40,56,12,28,44,60,1,17,33,49,5,37,53,9,25,41,57,13,29,45,61};

	int WrEn[64] = {0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

	int WrEn_dmw96[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

	int out_num[64] = {2,3,0,1,2,3,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1};

	int out_num_dmw96[64] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

    wiWORD TFr[3][16] = {
           {256,255,251,245,237,226,213,198,181,162,142,121,98,74,50,25},
           {256,251,237,213,181,142,98,50,0,-50,-98,-142,-181,-213,-237,-251},
           {256,245,213,162,98,25,-50,-121,-181,-226,-251,-255,-237,-198,-142,-74}};

    wiWORD TFi[3][16] = {
           {0,-25,-50,-74,-98,-121,-142,-162,-181,-198,-213,-226,-237,-245,-251,-255},
           {0,-50,-98,-142,-181,-213,-237,-251,-256,-251,-237,-213,-181,-142,-98,-50},
           {0,-74,-142,-198,-237,-255,-251,-226,-181,-121,-50,25,98,162,213,245}};

	int oord_sh[72] = {11,27,43,59,15,31,47,63,0,16,32,48,4,20,36,52,8,24,40,56,12,28,44,60,1,17,33,49,5,21,37,53,9,25,41,57,13,29,45,61,
	2,18,34,50,6,22,38,54,10,26,42,58,14,30,46,62,3,19,35,51,7,23,39,55,11,27,43,59,15,31,47,63};

	int oord_lg[80] = {3,19,35,51,7,23,39,55,11,27,43,59,15,31,47,63,0,16,32,48,4,20,36,52,8,24,40,56,12,28,44,60,1,17,33,49,5,21,37,53,
	9,25,41,57,13,29,45,61,2,18,34,50,6,22,38,54,10,26,42,58,14,30,46,62,3,19,35,51,7,23,39,55,11,27,43,59,15,31,47,63};


    if (dmw96)
    {
    	if (ifft == 0)
    	{
    		stage0_shift = 0;
    		stage1_shift = 0;
    		stage2_shift = 0;
    	}
    	else
    	{
    		stage0_shift = 1;
    		stage1_shift = 2;
    		stage2_shift = 1;
    	}
	}
	else
	{
		if (ifft == 0)
		{
		   stage0_shift = 2;
		   stage1_shift = 2;
		   stage2_shift = 0;
	    }
	    else
	    {
		   stage0_shift = 1;
		   stage1_shift = 1;
		   stage2_shift = 0;
	    }
    }


    if (ifft)
    {
      for (i=0; i<64; i++)
      {

			memr[i].word = xr[i].word;
            memi[i].word = -xi[i].word;


      }
    }
    else
    {
	   for (i=0;i<64;i++)
	   {
            memr[i].word = xr[i].word;
            memi[i].word = xi[i].word;

           // printf("i=%d,memr=%d,memi=%d\n",i,memr[i].word,memi[i].word);

	   }
    }


	ival = 16;
    //1st Radix4 stage
    for (i=0; i<16; i++)
    {
        for (j=0; j<4; j++)
        {
          if (ifft)
          {
            BFIr[j].word = memr[iord0_flip[j][i]].word;
            BFIi[j].word = memi[iord0_flip[j][i]].word;
	        if (i==ival)
	             printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord0[j][i],BFIr[j].word,BFIi[j].word);
	      }
	      else
	      {
            BFIr[j].word = memr[iord0[j][i]].word;
            BFIi[j].word = memi[iord0[j][i]].word;
	        if (i==ival)
	             printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord0[j][i],BFIr[j].word,BFIi[j].word);
	      }
	    }

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	    for (j=0;j<4;j++)
	          if (i==ival)
	                printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        for (j=0; j<4; j++){

            if (j==0){
    	    	    memr[iord0[j][i]].word = BFOr[j].word;
            		memi[iord0[j][i]].word = BFOi[j].word;

			        if (i==ival)
			              printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

  	    	}
	    	else{
					m = i % 16;

	                FFTComplexMultiplier(BFOr[j], BFOi[j], TFr[j-1][m], TFi[j-1][m], &cmplx_out_r, &cmplx_out_i);
                    memr[iord0[j][i]] = cmplx_out_r;
                    memi[iord0[j][i]] = cmplx_out_i;

                    if (i==ival)
                          printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);
	    	}
	    }

    }

    for (j=0; j<64; j++)
	{
		  memr[j].word = FFTSaturation(memr[j], stage0_shift);
		  memi[j].word = FFTSaturation(memi[j], stage0_shift);
	}

    for (i=0;i<16;i++){
	     for (j=0;j<4;j++){
	       if (i==ival){
	         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord0[j][i]].word,memi[iord0[j][i]].word);
	      }
		 }
    }

    ival = 16;
    //2nd Radix 4 Stage
    for (i=0; i<16; i++)
    {
        for (j=0; j<4; j++)
        {   // Read butterfly inputs from memory at the specified order.
            BFIr[j].word = memr[iord1[j][i]].word;
            BFIi[j].word = memi[iord1[j][i]].word;
	        if (i==ival)
	             printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord1[j][i],BFIr[j].word,BFIi[j].word);

	    }

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	    for (j=0;j<4;j++)
	        if (i==ival)
	             printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        for (j=0; j<4; j++){

            if (j==0){
    	    	    memr[iord1[j][i]].word = BFOr[j].word;
            		memi[iord1[j][i]].word = BFOi[j].word;

			        if (i==ival)
			             printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

  	    	}
	    	else{
				   m = i % 4;
				   if (i == ival)
				         printf("i=%d,j=%d,TFr = %d,TFi = %d\n",i,j,TFr[j-1][m*4].word,TFi[j-1][m*4].word);

	                FFTComplexMultiplier(BFOr[j], BFOi[j], TFr[j-1][m*4], TFi[j-1][m*4], &cmplx_out_r, &cmplx_out_i);
                    memr[iord1[j][i]] = cmplx_out_r;
                    memi[iord1[j][i]] = cmplx_out_i;

                    if (i==ival)
                         printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);
	    	}
	    }
     }



    for (j=0; j<64; j++)
	{
		  memr[j].word = FFTSaturation(memr[j], stage1_shift);
		  memi[j].word = FFTSaturation(memi[j], stage1_shift);
	}

    for (i=0;i<16;i++){
	     for (j=0;j<4;j++){
	       if (i==ival){
	         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord1[j][i]].word,memi[iord1[j][i]].word);
	      }
		 }
    }


   ival = 65;
    //3rd Radix4 Stage
   if (ifft)
   {
	    for (i=0; i<16; i++)
	    {
        		for (j=0; j<4; j++)
        		{
        		    BFIr[j].word = memr[iord2[j][i]].word;
        		    BFIi[j].word = memi[iord2[j][i]].word;

	    		 	if (i==ival)
	    		 	     printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord3[j][i],BFIr[j].word,BFIi[j].word);
        		}

        		FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

				for (j=0;j<4;j++)
				    if (i==ival)
				         printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        		for (j=0; j<4; j++)
        		{
        		    memr[iord2[j][i]].word = BFOr[j].word;
        		    memi[iord2[j][i]].word = BFOi[j].word;
        		}
		}
   }
   else
   {
	   if (dmw96)
	   {
         for (i=0; i<64; i++)
         {
           for (j=0; j<4; j++)
           {
            BFIr[j].word = memr[iord3_dmw96[j][i]].word;
            BFIi[j].word = memi[iord3_dmw96[j][i]].word;
	        if (i==ival)
	            printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord3[j][i],BFIr[j].word,BFIi[j].word);

	       }

           if (WrEn_dmw96[i] == 0)
       	    FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
           else
		   {
		   	for (j=0;j<4;j++)
		 	{
		 			BFOr[j].word = BFIr[j].word;
					BFOi[j].word = BFIi[j].word;
		  	}
		   }

	       for (j=0;j<4;j++)
	           if (i==ival)
	                 printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

           out1r[i].word = BFOr[out_num_dmw96[i]].word;
    	   out1i[i].word = BFOi[out_num_dmw96[i]].word;

           for (j=0; j<4; j++){

    	    	    memr[iord3_dmw96[j][i]].word = BFOr[j].word;
            		memi[iord3_dmw96[j][i]].word = BFOi[j].word;

                    if (i==ival)
                     printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);
	       }
	     }
       }
       else
       {
         for (i=0; i<64; i++)
         {
           for (j=0; j<4; j++)
           {   // Read butterfly inputs from memory at the specified order.
            BFIr[j].word = memr[iord3[j][i]].word;
            BFIi[j].word = memi[iord3[j][i]].word;
	        if (i==ival)
	            printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord3[j][i],BFIr[j].word,BFIi[j].word);

	       }

           if (WrEn[i] == 0)
       	    FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
           else
		   {
		   	for (j=0;j<4;j++)
		 	{
		 			BFOr[j].word = BFIr[j].word;
					BFOi[j].word = BFIi[j].word;
		  	}
		   }

	       for (j=0;j<4;j++)
	           if (i==ival)
	                 printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

           out1r[i].word = BFOr[out_num[i]].word;
    	   out1i[i].word = BFOi[out_num[i]].word;

	      //printf("i=%d,outnum=%d,out1r = %d,out1i = %d\n",i,out_num[i],out1r[i].word,out1i[i].word);

           for (j=0; j<4; j++){

    	    	    memr[iord3[j][i]].word = BFOr[j].word;
            		memi[iord3[j][i]].word = BFOi[j].word;

                    if (i==ival)
                     printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);
	       }
	     }
	   }
   }


    for (i=0; i<64; i++)
    {
    		  memr[i].word = FFTSaturation(memr[i], stage2_shift);
    		  memi[i].word = FFTSaturation(memi[i], stage2_shift);
    }

	if (ifft == 0)
	{
    	  for (i=0; i<64; i++)
		  {
    		  out1r[i].word = FFTSaturation(out1r[i], stage2_shift);
    		  out1i[i].word = FFTSaturation(out1i[i], stage2_shift);

	      }

	      if (dmw96 == 0)
	      {
    	  	for (i=0;i<4;i++) {
		 		out2r[i].word = memr[oord4[64+i]].word;
		 		out2i[i].word = memi[oord4[64+i]].word;
    	  	}
	      }
    }



	if (ifft)
	{
	  if (gi == 0)
	  {
	    for (i=0;i<72;i++)
	    {
           yr[i].word = memr[oord_sh[i]].word;
           yi[i].word = -1*memi[oord_sh[i]].word;
        }
      }
      else
	  {
		for (i=0;i<80;i++)
		{
           yr[i].word = memr[oord_lg[i]].word;
           yi[i].word = -1*memi[oord_lg[i]].word;
	    }
	  }
    }
    else
    {
	   if (dmw96)
	   {
		 for (i=0;i<64;i++)
		 {
			yr[i].word = out1r[i].word;
			yi[i].word = out1i[i].word;
    	 }
       }
       else
       {
		 for (i=0;i<68;i++)
		 {
			if (i < 64)
			{
				yr[i].word = out1r[i].word;
				yi[i].word = out1i[i].word;
		    }
		    else
		    {
				yr[i].word = out2r[i-64].word;
				yi[i].word = out2i[i-64].word;
		    }
    	 }
	   }
	}
}



static void fixFFT128(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft, int gi)
{
    int i, j;
	int ival;
	int m;
	wiWORD tmp;
    wiWORD BFIr[4], BFIi[4], BFOr[4], BFOi[4];
    wiWORD memr[128], memi[128];

    wiWORD cmplx_out_r, cmplx_out_i;

    wiWORD out1r[64], out1i[64];
    wiWORD out2r[68], out2i[68];

    wiWORD ifft_outr[128];
    wiWORD ifft_outi[128];

    int iord0[2][64] = {
       {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
        49,50,51,52,53,54,55,56,57,58,59,60,61,62,63},
       {64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,
        107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127}};

	int iord0_flip[2][64] = {
	{64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,
	114,115,116,117,118,119,120,121,122,123,124,125,126,127},
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,
	50,51,52,53,54,55,56,57,58,59,60,61,62,63}};

    int iord1[4][32] = {
      {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79},
      {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95},
      {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111},
      {48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127}};

    int iord2[4][32] = {
      {0,1,2,3,16,17,18,19,32,33,34,35,48,49,50,51,64,65,66,67,80,81,82,83,96,97,98,99,112,113,114,115},
      {4,5,6,7,20,21,22,23,36,37,38,39,52,53,54,55,68,69,70,71,84,85,86,87,100,101,102,103,116,117,118,119},
      {8,9,10,11,24,25,26,27,40,41,42,43,56,57,58,59,72,73,74,75,88,89,90,91,104,105,106,107,120,121,122,123},
      {12,13,14,15,28,29,30,31,44,45,46,47,60,61,62,63,76,77,78,79,92,93,94,95,108,109,110,111,124,125,126,127}};

	int iord3[4][64] = {
	{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124},
	{1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,73,77,81,85,89,93,97,101,105,109,113,117,121,125},
	{2,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,66,70,74,78,82,86,90,94,98,102,106,110,114,118,122,126},
	{3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63,67,71,75,79,83,87,91,95,99,103,107,111,115,119,123,127}};

    int iord4[4][66] = {
      {84,112,104,84,76,104,84,112,104,84,0,64,16,80,32,96,48,112,4,68,20,36,100,52,116,8,72,24,88,40,104,56,120,12,76,28,92,44,108,60,124,0,64,16,80,32,96,48,4,68,
       20,84,36,100,52,116,8,72,24,88,40,56,120,12,76,28},
      {85,113,105,85,77,105,85,113,105,85,1,65,17,81,33,97,49,113,5,69,21,37,101,53,117,9,73,25,89,41,105,57,121,13,77,29,93,45,109,61,125,1,65,17,81,33,97,49,5,69,
       21,85,37,101,53,117,9,73,25,89,41,57,121,13,77,29},
      {86,114,106,86,78,106,86,114,106,86,2,66,18,82,34,98,50,114,6,70,22,38,102,54,118,10,74,26,90,42,106,58,122,14,78,30,94,46,110,62,126,2,66,18,82,34,98,50,6,70,
       22,86,38,102,54,118,10,74,26,90,42,58,122,14,78,30},
      {87,115,107,87,79,107,87,115,107,87,3,67,19,83,35,99,51,115,7,71,23,39,103,55,119,11,75,27,91,43,107,59,123,15,79,31,95,47,111,63,127,3,67,19,83,35,99,51,7,71,
       23,87,39,103,55,119,11,75,27,91,43,59,123,15,79,31}};


	int oord[128] = {0,32,64,96,8,40,72,104,16,48,80,112,24,56,88,120,2,34,66,98,10,42,74,106,18,50,82,114,26,58,90,122,4,36,68,100,12,44,76,108,20,52,84,116,28,60,92,124,6,38,70,102,14,46,78,110,22,54,86,118,30,62,94,126,1,33,65,97,9,41,73,105,17,49,81,113,25,57,89,121,
	3,35,67,99,11,43,75,107,19,51,83,115,27,59,91,123,5,37,69,101,13,45,77,109,21,53,85,117,29,61,93,125,7,39,71,103,15,47,79,111,23,55,87,119,31,63,95,127};

	int oord4[132] = {86,115,107,84,76,105,86,115,107,84,2,66,18,82,34,98,50,114,6,70,22,38,102,54,118,10,74,26,90,42,106,58,122,14,78,30,94,46,110,62,126,3,67,19,83,35,99,51,7,71,23,87,39,103,55,119,11,75,27,91,43,59,123,15,79,31,95,47,111,63,127,0,64,16,80,32,96,48,112,4,
	68,20,36,100,52,116,8,72,24,88,40,104,56,120,12,28,92,44,108,60,124,1,65,17,81,33,97,49,113,5,69,21,85,37,101,53,117,9,73,25,89,41,57,121,13,77,29,93,45,109,61,125};

     wiWORD TF128_r[64] = {
           256,256,255,253,251,248,245,241,237,231,226,220,213,206,198,190,181,172,162,152,142,132,121,109,98,86,74,62,50,38,25,13,0,-13,-25,-38,-50,-62,-74,-86,-98,-109,-121,-132,-142,-152,-162,-172,-181,-190,-198,-206,-213,-220,-226,-231,-237,-241,-245,-248,-251,-253,-255,-256};

     wiWORD TF128_i[64] = {
            0,-13,-25,-38,-50,-62,-74,-86,-98,-109,-121,-132,-142,-152,-162,-172,-181,-190,-198,-206,-213,-220,-226,-231,-237,-241,-245,-248,-251,-253,-255,-256,-256,-256,-255,-253,-251,-248,-245,-241,-237,-231,-226,-220,-213,-206,-198,-190,-181,-172,-162,-152,-142,-132,-121,-109,-98,-86,-74,-62,-50,-38,-25,-13};

     wiWORD TF64_r[3][16] = {
           {256,255,251,245,237,226,213,198,181,162,142,121,98,74,50,25},
           {256,251,237,213,181,142,98,50,0,-50,-98,-142,-181,-213,-237,-251},
           {256,245,213,162,98,25,-50,-121,-181,-226,-251,-255,-237,-198,-142,-74}};

     wiWORD TF64_i[3][16] = {
           {0,-25,-50,-74,-98,-121,-142,-162,-181,-198,-213,-226,-237,-245,-251,-255},
           {0,-50,-98,-142,-181,-213,-237,-251,-256,-251,-237,-213,-181,-142,-98,-50},
           {0,-74,-142,-198,-237,-255,-251,-226,-181,-121,-50,25,98,162,213,245}};

	int out_num[64] = {2,3,3,0,0,1,2,3,3,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};

	int WrEn[64] = {0,0,0,1,0,1,1,1,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

    // Read in the input array into memory.
    // For ifft we need take the conjugate of the input.
    // Input read order into the memory will change for ifft.

    if (ifft)
    {
    	for (i=0; i<128; i++)
    	{
            memr[i].word = xr[i].word;
            memi[i].word = -xi[i].word;
           //if (i==64) printf("i=%d,memr = %d,memi = %d\n",i,memr[i].word,memi[i].word);
    	}
	}
	else
	{
    	for (i=0; i<128; i++)
    	{
            memr[i].word = xr[i].word;
            memi[i].word = xi[i].word;
    	}
	}

    // First stage, Radix 2
    ival = 64;


    for (i=0; i<64; i++)
    {
        for (j=0; j<2; j++)
        {
	        tmp.word = iord0_flip[j][i];

		  if (ifft)
		  {
            BFIr[j].word = memr[iord0_flip[j][i]].word;
            BFIi[j].word = memi[iord0_flip[j][i]].word;
	      }
	      else
	      {
            BFIr[j].word = memr[iord0[j][i]].word;
            BFIi[j].word = memi[iord0[j][i]].word;
	      }
	        if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);

	    }

        FFTTwoPointButterfly(BFIr, BFIi, BFOr, BFOi);


        for (j=0; j<2; j++)
        {
           if (j==0)
           {
    	    	            memr[iord0[j][i]].word = BFOr[j].word;
            		        memi[iord0[j][i]].word = BFOi[j].word;
			                if (i==ival) printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].w20,BFOi[j].w20);

  	       }
	       else
           {

	          		FFTComplexMultiplier(BFOr[j], BFOi[j], TF128_r[i], TF128_i[i], &cmplx_out_r, &cmplx_out_i);

                    memr[iord0[j][i]] = cmplx_out_r;
                  	memi[iord0[j][i]] = cmplx_out_i;

                    if (i==ival)
                     printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.w20,cmplx_out_i.w20);


	       }

         }

   }

  if (ifft == 0)
  {
   	for (j=0; j<128; j++)
   	{
        memr[j].word = FFTSaturation(memr[j], 1); // originally set to shift down by 1.
        memi[j].word = FFTSaturation(memi[j], 1);
   	}
  }
  else
  {
   	for (j=0; j<128; j++)
   	{
        memr[j].word = FFTSaturation(memr[j], 2); // originally set to shift down by 1.
        memi[j].word = FFTSaturation(memi[j], 2);
   	}
  }

  for (i=0;i<64;i++){
    for (j=0;j<2;j++){
      if (i==ival){
      printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord0[j][i]].w20,memi[iord0[j][i]].w20);
     }
   }
  }


   ival = 3;
    // Second Stage (Radix 4)
    for (i=0; i<32; i++)
    {
        for (j=0;j<4;j++){
            BFIr[j].word = memr[iord1[j][i]].word;
            BFIi[j].word = memi[iord1[j][i]].word;

	        tmp.word = iord1[j][i];
	        if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);

	    }

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	    for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);


        for (j=0;j<4;j++){

        	if (j==0) {
				    memr[iord1[j][i]].word = BFOr[0].word;
        	        memi[iord1[j][i]].word = BFOi[0].word;

			        if (i==ival) printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);
		    }
        	else
                {
		        m = i % 16;
				FFTComplexMultiplier(BFOr[j], BFOi[j], TF64_r[j-1][m], TF64_i[j-1][m], &cmplx_out_r, &cmplx_out_i);
				memr[iord1[j][i]] = cmplx_out_r;
				memi[iord1[j][i]] = cmplx_out_i;

                if (i==ival)
                      printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);

		   }
        }
    }

  if (ifft == 0)
  {
    for (j=0; j<128; j++)
    {
        memr[j].word = FFTSaturation(memr[j], 2);
        memi[j].word = FFTSaturation(memi[j], 2);
    }
  }
  else
  {
    for (j=0; j<128; j++)
    {
        memr[j].word = FFTSaturation(memr[j], 1);
        memi[j].word = FFTSaturation(memi[j], 1);
    }
  }

   for (i=0;i<32;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord1[j][i]],memi[iord1[j][i]]);
      }
     }
   }


    ival = 0;
    // Third Stage (Radix 4)
    for (i=0; i<32; i++)
    {
	    for (j=0;j<4;j++){
			BFIr[j].word = memr[iord2[j][i]].word;
			BFIi[j].word = memi[iord2[j][i]].word;

	        tmp.word = iord2[j][i];
	        if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);
	    }

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	    for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);


        for (j=0;j<4;j++){
			if (j==0){
        	 	  memr[iord2[j][i]].word = BFOr[0].word;
        		  memi[iord2[j][i]].word = BFOi[0].word;

			      if (i==ival) printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

			}
			else {
			  m = i % 4;

			  FFTComplexMultiplier(BFOr[j], BFOi[j], TF64_r[j-1][m*4], TF64_i[j-1][m*4], &cmplx_out_r, &cmplx_out_i);
			  memr[iord2[j][i]] = cmplx_out_r;
			  memi[iord2[j][i]] = cmplx_out_i;

              if (i==ival)
                      printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);

			}
        }
    }

  if (ifft == 0)
  {
    for (j=0; j<128; j++)
    {
        memr[j].word = FFTSaturation(memr[j], 2);
        memi[j].word = FFTSaturation(memi[j], 2);

    }
  }
  else
  {
    for (j=0; j<128; j++)
    {
        memr[j].word = FFTSaturation(memr[j], 1);
        memi[j].word = FFTSaturation(memi[j], 1);

    }
  }

   for (i=0;i<32;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord2[j][i]],memi[iord2[j][i]]);
      }
     }
   }

    ival = 65;
    // Fourth State (Radix 4)

    if (ifft == 0)
    {
    	for (i=0; i<64; i++)
    	{
    	    for (j=0; j<4; j++)
    	    {
		        BFIr[j].word = memr[iord4[j][i]].word;
    	        BFIi[j].word = memi[iord4[j][i]].word;

		        if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord4[j][i],BFIr[j].word,BFIi[j].word);
    	    }

			if (WrEn[i] == 0)
				FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
    	    else
			{
				for (j=0;j<4;j++)
					{
					BFOr[j].word = BFIr[j].word;
					BFOi[j].word = BFIi[j].word;
				}
			}
		   // for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);


    	    out1r[i].word = BFOr[out_num[i]].word;
    	    out1i[i].word = BFOi[out_num[i]].word;

    	   // printf("i=%d,out_num=%d,out1r = %d, out1i = %d\n",i,out_num[i],out1r[i].word,out1i[i].word);

    	    for (j=0; j<4; j++)
    	    {
    	        memr[iord4[j][i]].word = BFOr[j].word;
    	        memi[iord4[j][i]].word = BFOi[j].word;

				if (i==ival) printf("i=%d,j=%d,iord=%d,BFOr = %d, BFOi = %d\n",i,j, iord4[j][i],memr[iord4[j][i]].word, memi[iord4[j][i]].word);
			}
        }
    	for (i=0; i<68; i++)
    	{
	 		out2r[i].word = memr[oord4[64+i]].word;
	 		out2i[i].word = memi[oord4[64+i]].word;
   			//  printf("i=%d,oord4=%d,out2r = %d, out2i = %d\n",i,oord4[64+i],out2r[i].word,out2i[i].word);
    	}
    }
    else
    {
	    for (i=0; i<32; i++)
	    {
        	for (j=0; j<4; j++)
        	{
        	    BFIr[j].word = memr[iord3[j][i]].word;
        	    BFIi[j].word = memi[iord3[j][i]].word;

	    	 	if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,iord3[j][i],BFIr[j].word,BFIi[j].word);
        	}

        	FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

			for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        	for (j=0; j<4; j++)
        	{
        	    memr[iord3[j][i]].word = BFOr[j].word;
        	    memi[iord3[j][i]].word = BFOi[j].word;
        	}
		}
	}

    for (j=0; j<128; j++)
    {
		//if (j == 59)
        //         printf("memr = %d,memi = %d\n",memr[j].word,memi[j].word);

        memr[j].word = FFTSaturation(memr[j], 0);
        memi[j].word = FFTSaturation(memi[j], 0);

    }

    for (i=0;i<64;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord4[j][i]].word,memi[iord4[j][i]].word);
      }
	 }
    }



  if (ifft)
  {
      for (i=0; i<128; i++)      //if ifft, conjugate output
      {

            ifft_outr[oord[i]].word = memr[i].word;
            ifft_outi[oord[i]].word = -memi[i].word;
      }

	 if (gi == 0)
	 {
	   for (i=0;i<144;i++)
	   {
	     if (i<16)
	     {
	          yr[i] = ifft_outr[112+i]; // Append last 32 outputs to the front if short gi.
              yi[i] = ifft_outi[112+i];
         }
         else
	     {
              yr[i] = ifft_outr[i-16];
              yi[i] = ifft_outi[i-16];
	     }
       }
     }
     else
	 {
	   for (i=0;i<160;i++)
	   {
	      if (i<32)
	      {
	        yr[i] = ifft_outr[96+i]; // Append last 64 outputs to the front if long gi
            yi[i] = ifft_outi[96+i];
          }
          else
	      {
              yr[i] = ifft_outr[i-32];
              yi[i] = ifft_outi[i-32];
	      }

	      if (i<10)
             printf("i=%d, yr = %d, yi = %d\n",i,yr[i].word,yi[i].word);

       }
	 }

  }
  else
  {
    for (i=0; i<132; i++)      //if fft, flip output
    {
      if (i < 64)
      {
	          yr[i].word = out1r[i].word;
    	      yi[i].word = out1i[i].word;
		     // printf("output:i=%d,yr = %d, yi = %d\n",i,yr[i].word,yi[i].word);
      }
      else
      {
	      yr[i].word = out2r[i-64].word;
	      yi[i].word = out2i[i-64].word;
      }
    }
  }
}

static void fixFFT256(wiWORD *xr, wiWORD *xi, wiWORD *yr, wiWORD *yi, int ifft, int gi)
{
    int i, j;
	int m;
    wiWORD BFIr[4], BFIi[4], BFOr[4], BFOi[4];
    wiWORD memr[256], memi[256];

    wiWORD cmplx_out_r, cmplx_out_i;

    wiWORD out1r[74], out1i[74];
    wiWORD out2r[186], out2i[186];
    wiWORD tmp;
    // Order of inputs and outputs for the 1st radix4 butterfly stage.

int WrEn[74] = {0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int iord0[4][64] = {
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
60,61,62,63},
{64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,
84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,
104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,
124,125,126,127},
{128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,
148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
188,189,190,191},
{192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,
252,253,254,255}};


    // Order of the inputs and outputs for the 2nd radix4 butterfly stage.
int iord1[4][64] = {
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,64,65,66,67,
68,69,70,71,72,73,74,75,76,77,78,79,128,129,130,131,132,133,134,135,
136,137,138,139,140,141,142,143,192,193,194,195,196,197,198,199,200,201,202,203,
204,205,206,207},
{16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,80,81,82,83,
84,85,86,87,88,89,90,91,92,93,94,95,144,145,146,147,148,149,150,151,
152,153,154,155,156,157,158,159,208,209,210,211,212,213,214,215,216,217,218,219,
220,221,222,223},
{32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,96,97,98,99,
100,101,102,103,104,105,106,107,108,109,110,111,160,161,162,163,164,165,166,167,
168,169,170,171,172,173,174,175,224,225,226,227,228,229,230,231,232,233,234,235,
236,237,238,239},
{48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,112,113,114,115,
116,117,118,119,120,121,122,123,124,125,126,127,176,177,178,179,180,181,182,183,
184,185,186,187,188,189,190,191,240,241,242,243,244,245,246,247,248,249,250,251,
252,253,254,255}};

    // Order of the inputs and outputs for the 3rd radix4 butterfly stage.

int iord2[4][64] = {
{0,1,2,3,16,17,18,19,32,33,34,35,48,49,50,51,64,65,66,67,
80,81,82,83,96,97,98,99,112,113,114,115,128,129,130,131,144,145,146,147,
160,161,162,163,176,177,178,179,192,193,194,195,208,209,210,211,224,225,226,227,
240,241,242,243},
{4,5,6,7,20,21,22,23,36,37,38,39,52,53,54,55,68,69,70,71,
84,85,86,87,100,101,102,103,116,117,118,119,132,133,134,135,148,149,150,151,
164,165,166,167,180,181,182,183,196,197,198,199,212,213,214,215,228,229,230,231,
244,245,246,247},
{8,9,10,11,24,25,26,27,40,41,42,43,56,57,58,59,72,73,74,75,
88,89,90,91,104,105,106,107,120,121,122,123,136,137,138,139,152,153,154,155,
168,169,170,171,184,185,186,187,200,201,202,203,216,217,218,219,232,233,234,235,
248,249,250,251},
{12,13,14,15,28,29,30,31,44,45,46,47,60,61,62,63,76,77,78,79,
92,93,94,95,108,109,110,111,124,125,126,127,140,141,142,143,156,157,158,159,
172,173,174,175,188,189,190,191,204,205,206,207,220,221,222,223,236,237,238,239,
252,253,254,255}};

    // Order of the inputs and outputs for the 4th radix4 butterfly stage.
int iord3[4][64] = {
{0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,
80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,140,144,148,152,156,
160,164,168,172,176,180,184,188,192,196,200,204,208,212,216,220,224,228,232,236,
240,244,248,252},
{1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,73,77,
81,85,89,93,97,101,105,109,113,117,121,125,129,133,137,141,145,149,153,157,
161,165,169,173,177,181,185,189,193,197,201,205,209,213,217,221,225,229,233,237,
241,245,249,253},
{2,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,66,70,74,78,
82,86,90,94,98,102,106,110,114,118,122,126,130,134,138,142,146,150,154,158,
162,166,170,174,178,182,186,190,194,198,202,206,210,214,218,222,226,230,234,238,
242,246,250,254},
{3,7,11,15,19,23,27,31,35,39,43,47,51,55,59,63,67,71,75,79,
83,87,91,95,99,103,107,111,115,119,123,127,131,135,139,143,147,151,155,159,
163,167,171,175,179,183,187,191,195,199,203,207,211,215,219,223,227,231,235,239,
243,247,251,255}};

int iord4[4][74] = {
{100,92,100,92,224,216,224,216,100,92,100,92,0,64,128,192,16,80,144,208,
32,96,160,224,48,112,176,240,4,68,132,196,20,84,148,212,36,164,228,52,
116,180,244,8,72,136,200,24,88,152,216,40,104,168,232,56,120,184,248,12,
76,140,204,28,156,220,44,108,172,236,60,124,188,252},
{101,93,101,93,225,217,225,217,101,93,101,93,1,65,129,193,17,81,145,209,
33,97,161,225,49,113,177,241,5,69,133,197,21,85,149,213,37,165,229,53,
117,181,245,9,73,137,201,25,89,153,217,41,105,169,233,57,121,185,249,13,
77,141,205,29,157,221,45,109,173,237,61,125,189,253},
{102,94,102,94,226,218,226,218,102,94,102,94,2,66,130,194,18,82,146,210,
34,98,162,226,50,114,178,242,6,70,134,198,22,86,150,214,38,166,230,54,
118,182,246,10,74,138,202,26,90,154,218,42,106,170,234,58,122,186,250,14,
78,142,206,30,158,222,46,110,174,238,62,126,190,254},
{103,95,103,95,227,219,227,219,103,95,103,95,3,67,131,195,19,83,147,211,
35,99,163,227,51,115,179,243,7,71,135,199,23,87,151,215,39,167,231,55,
119,183,247,11,75,139,203,27,91,155,219,43,107,171,235,59,123,187,251,15,
79,143,207,31,159,223,47,111,175,239,63,127,191,255}};

int iord0_flip[4][64] = {
{128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,
148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
188,189,190,191},
{192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,
252,253,254,255},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,
20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,
60,61,62,63},
{64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,
84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,
104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,
124,125,126,127}};

wiWORD TFr[3][64] = {
{256,256,256,255,255,254,253,252,251,250,248,247,245,243,241,239,237,234,231,229,226,223,220,216,213,209,206,202,198,194,190,185,181,177,172,167,162,157,152,147,142,137,132,126,121,115,109,104,98,92,86,80,74,68,62,56,50,44,38,31,25,19,13,6},
{256,256,255,253,251,248,245,241,237,231,226,220,213,206,198,190,181,172,162,152,142,132,121,109,98,86,74,62,50,38,25,13,0,-13,-25,-38,-50,-62,-74,-86,-98,-109,-121,-132,-142,-152,-162,-172,-181,-190,-198,-206,-213,-220,-226,-231,-237,-241,-245,-248,-251,-253,-255,-256},
{256,255,253,250,245,239,231,223,213,202,190,177,162,147,132,115,98,80,62,44,25,6,-13,-31,-50,-68,-86,-104,-121,-137,-152,-167,-181,-194,-206,-216,-226,-234,-241,-247,-251,-254,-256,-256,-255,-252,-248,-243,-237,-229,-220,-209,-198,-185,-172,-157,-142,-126,-109,-92,-74,-56,-38,-19}};

wiWORD TFi[3][64] = {
{0,-6,-13,-19,-25,-31,-38,-44,-50,-56,-62,-68,-74,-80,-86,-92,-98,-104,-109,-115,-121,-126,-132,-137,-142,-147,-152,-157,-162,-167,-172,-177,-181,-185,-190,-194,-198,-202,-206,-209,-213,-216,-220,-223,-226,-229,-231,-234,-237,-239,-241,-243,-245,-247,-248,-250,-251,-252,-253,-254,-255,-255,-256,-256},
{0,-13,-25,-38,-50,-62,-74,-86,-98,-109,-121,-132,-142,-152,-162,-172,-181,-190,-198,-206,-213,-220,-226,-231,-237,-241,-245,-248,-251,-253,-255,-256,-256,-256,-255,-253,-251,-248,-245,-241,-237,-231,-226,-220,-213,-206,-198,-190,-181,-172,-162,-152,-142,-132,-121,-109,-98,-86,-74,-62,-50,-38,-25,-13},
{0,-19,-38,-56,-74,-92,-109,-126,-142,-157,-172,-185,-198,-209,-220,-229,-237,-243,-248,-252,-255,-256,-256,-254,-251,-247,-241,-234,-226,-216,-206,-194,-181,-167,-152,-137,-121,-104,-86,-68,-50,-31,-13,6,25,44,62,80,98,115,132,147,162,177,190,202,213,223,231,239,245,250,253,255}};


    // Output order for the FFT. Currently set to natural order but will change in the future.
int oord[256] = {0,64,128,192,16,80,144,208,32,96,160,224,48,112,176,240,4,68,132,196,20,84,148,212,36,100,164,228,52,116,180,244,8,72,136,200,24,88,152,216,
		40,104,168,232,56,120,184,248,12,76,140,204,28,92,156,220,44,108,172,236,60,124,188,252,1,65,129,193,17,81,145,209,33,97,161,225,49,113,177,241,
		5,69,133,197,21,85,149,213,37,101,165,229,53,117,181,245,9,73,137,201,25,89,153,217,41,105,169,233,57,121,185,249,13,77,141,205,29,93,157,221,
		45,109,173,237,61,125,189,253,2,66,130,194,18,82,146,210,34,98,162,226,50,114,178,242,6,70,134,198,22,86,150,214,38,102,166,230,54,118,182,246,
		10,74,138,202,26,90,154,218,42,106,170,234,58,122,186,250,14,78,142,206,30,94,158,222,46,110,174,238,62,126,190,254,3,67,131,195,19,83,147,211,
		35,99,163,227,51,115,179,243,7,71,135,199,23,87,151,215,39,103,167,231,55,119,183,247,11,75,139,203,27,91,155,219,43,107,171,235,59,123,187,251,
		15,79,143,207,31,95,159,223,47,111,175,239,63,127,191,255};


int oord4[260] = {102,94,103,95,224,216,225,217,102,94,103,95,2,66,130,194,18,82,146,210,34,98,162,226,50,114,178,242,6,70,134,198,22,86,150,214,38,166,230,54,
118,182,246,10,74,138,202,26,90,154,218,42,106,170,234,58,122,186,250,14,78,142,206,30,158,222,46,110,174,238,62,126,190,254,3,67,131,195,19,83,
147,211,35,99,163,227,51,115,179,243,7,71,135,199,23,87,151,215,39,167,231,55,119,183,247,11,75,139,203,27,91,155,219,43,107,171,235,59,123,187,
251,15,79,143,207,31,159,223,47,111,175,239,63,127,191,255,0,64,128,192,16,80,144,208,32,96,160,48,112,176,240,4,68,132,196,20,84,148,212,36,
100,164,228,52,116,180,244,8,72,136,200,24,88,152,40,104,168,232,56,120,184,248,12,76,140,204,28,92,156,220,44,108,172,236,60,124,188,252,1,65,
129,193,17,81,145,209,33,97,161,49,113,177,241,5,69,133,197,21,85,149,213,37,101,165,229,53,117,181,245,9,73,137,201,25,89,153,41,105,169,233,
57,121,185,249,13,77,141,205,29,93,157,221,45,109,173,237,61,125,189,253};

int oord_lg[320] = {3,67,131,195,19,83,147,211,35,99,163,227,51,115,179,243,7,71,135,199,23,87,151,215,39,103,167,231,55,119,183,247,11,75,139,203,27,91,155,219,
43,107,171,235,59,123,187,251,15,79,143,207,31,95,159,223,47,111,175,239,63,127,191,255,0,64,128,192,16,80,144,208,32,96,160,224,48,112,176,240,
4,68,132,196,20,84,148,212,36,100,164,228,52,116,180,244,8,72,136,200,24,88,152,216,40,104,168,232,56,120,184,248,12,76,140,204,28,92,156,220,
44,108,172,236,60,124,188,252,1,65,129,193,17,81,145,209,33,97,161,225,49,113,177,241,5,69,133,197,21,85,149,213,37,101,165,229,53,117,181,245,
9,73,137,201,25,89,153,217,41,105,169,233,57,121,185,249,13,77,141,205,29,93,157,221,45,109,173,237,61,125,189,253,2,66,130,194,18,82,146,210,
34,98,162,226,50,114,178,242,6,70,134,198,22,86,150,214,38,102,166,230,54,118,182,246,10,74,138,202,26,90,154,218,42,106,170,234,58,122,186,250,
14,78,142,206,30,94,158,222,46,110,174,238,62,126,190,254,3,67,131,195,19,83,147,211,35,99,163,227,51,115,179,243,7,71,135,199,23,87,151,215,
39,103,167,231,55,119,183,247,11,75,139,203,27,91,155,219,43,107,171,235,59,123,187,251,15,79,143,207,31,95,159,223,47,111,175,239,63,127,191,255};

int oord_sh[288] = {11,75,139,203,27,91,155,219,43,107,171,235,59,123,187,251,15,79,143,207,31,95,159,223,47,111,175,239,63,127,191,255,0,64,128,192,16,80,144,208,
32,96,160,224,48,112,176,240,4,68,132,196,20,84,148,212,36,100,164,228,52,116,180,244,8,72,136,200,24,88,152,216,40,104,168,232,56,120,184,248,
12,76,140,204,28,92,156,220,44,108,172,236,60,124,188,252,1,65,129,193,17,81,145,209,33,97,161,225,49,113,177,241,5,69,133,197,21,85,149,213,
37,101,165,229,53,117,181,245,9,73,137,201,25,89,153,217,41,105,169,233,57,121,185,249,13,77,141,205,29,93,157,221,45,109,173,237,61,125,189,253,
2,66,130,194,18,82,146,210,34,98,162,226,50,114,178,242,6,70,134,198,22,86,150,214,38,102,166,230,54,118,182,246,10,74,138,202,26,90,154,218,
42,106,170,234,58,122,186,250,14,78,142,206,30,94,158,222,46,110,174,238,62,126,190,254,3,67,131,195,19,83,147,211,35,99,163,227,51,115,179,243,
7,71,135,199,23,87,151,215,39,103,167,231,55,119,183,247,11,75,139,203,27,91,155,219,43,107,171,235,59,123,187,251,15,79,143,207,31,95,159,223,
47,111,175,239,63,127,191,255};

int out_num[74] = {2,2,3,3,0,0,1,1,2,2,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};

    // Read in the input array into memory.
    // For ifft we need take the conjugate of the input.
    // Input read order into the memory will change for ifft.
int ival; ival = 256;
    if (ifft)
    {
        for (i=0; i<256; i++)
        {
            memr[i].word = xr[i].word;
            memi[i].word = -xi[i].word;

			if (i == ival)
               printf("%d,mem_r[%d]=%d,mem_i[%d]=%d\n",i,i,memr[i].word,i,memi[i].word);
        }
    }
    else
    {
        for (i=0; i<256; i++)
        {
            memr[i].word = xr[i].word;
            memi[i].word = xi[i].word;
	   // printf("%d,mem_r[%d]=%d,mem_i[%d]=%d\n",i,i,memr[i].word,i,memi[i].word);
	   // printf("%d,x_r[%d]=%d,x_i[%d]=%d\n",i,i,xr[i].word,i,xi[i].word);

        }
    }


    ival = 16;
    // First stage of 256 point Radix DIF FFT implementation.
    for (i=0; i<64; i++)
    {
        for (j=0; j<4; j++)
        {   // Read butterfly inputs from memory at the specified order.

	  if (ifft){

	    tmp.word = iord0_flip[j][i];
            BFIr[j].word = memr[iord0_flip[j][i]].word;
            BFIi[j].word = memi[iord0_flip[j][i]].word;

	     if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].w15,BFIi[j].w15);
	  }
          else
	  {
	    tmp.word = iord0[j][i];
            BFIr[j].word = memr[iord0[j][i]].word;
            BFIi[j].word = memi[iord0[j][i]].word;

	     if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);

	  }

	}

	  // Do a 4 point butterfly.
        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        for (j=0; j<4; j++){

            if (j==0){ // First butterfly output has no twiddle multiplications.
    	    	    memr[iord0[j][i]].word = BFOr[j].word;
            		memi[iord0[j][i]].word = BFOi[j].word;
			 if (i==ival) printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);
  	    	}
	    	else{
		  if (i == ival) printf("i=%d,j=%d,TFr = %d, TFi = %d\n",i,j,TFr[j-1][i].word,TFi[j-1][i].word);
		  // FFTComplexMultiplier(BFOr[j], BFOi[j], TFr, TFi, memr+iord0[j][i], memi+iord0[j][i]);
	          FFTComplexMultiplier(BFOr[j], BFOi[j], TFr[j-1][i], TFi[j-1][i], &cmplx_out_r, &cmplx_out_i);
                  memr[iord0[j][i]] = cmplx_out_r;
                  memi[iord0[j][i]] = cmplx_out_i;
                  if (i==ival)
                     printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);
	    	}//end if(j=0


	}//end for (j=0..

    }// end for (i=0

    if (ifft == 0){
      for (j=0; j<256; j++)
	{
	  memr[j].word = FFTSaturation(memr[j], 2);
	  memi[j].word = FFTSaturation(memi[j], 2);
	}
    }
    else {
      for (j=0; j<256; j++)
	{
	  memr[j].word = FFTSaturation(memr[j], 2);
	  memi[j].word = FFTSaturation(memi[j], 2);
	}
    }

for (i=0;i<64;i++){
  for (j=0;j<4;j++){
    if (i==ival){
      printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord0[j][i]],memi[iord0[j][i]]);
    }
  }
 }

/*for (i=0;i<256;i++)
   {
     printf("i=%d,memr = %d, memi = %d\n",i,memr[i].word,memi[i].word);
     }*/


ival = 65;
    // Second stage.
    for (i=0; i<64; i++)
    {
        for (j=0;j<4;j++){
            BFIr[j].word = memr[iord1[j][i]].word;
            BFIi[j].word = memi[iord1[j][i]].word;
	     tmp.word = iord1[j][i];
	     if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);
	    }

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        for (j=0;j<4;j++){

        	if (j==0) {
			memr[iord1[j][i]].word = BFOr[0].word;
        	        memi[iord1[j][i]].word = BFOi[0].word;
			 if (i==ival) printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);
			}
        	else{

		        m = i%16;
			FFTComplexMultiplier(BFOr[j], BFOi[j], TFr[j-1][m*4], TFi[j-1][m*4], &cmplx_out_r, &cmplx_out_i);
			memr[iord1[j][i]] = cmplx_out_r;
			memi[iord1[j][i]] = cmplx_out_i;
                        if (i==ival)
                          printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);
		    }
        }

    }


    if (ifft == 0){
      for (j=0; j<256; j++)
	{
	  memr[j].word = FFTSaturation(memr[j], 2);
	  memi[j].word = FFTSaturation(memi[j], 2);
	}
    }
    else {
      for (j=0; j<256; j++)
	{
	  memr[j].word = FFTSaturation(memr[j], 1);
	  memi[j].word = FFTSaturation(memi[j], 1);
	}
    }


   for (i=0;i<64;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord1[j][i]],memi[iord1[j][i]]);
      }
     }
   }

   /*for (i=0;i<256;i++)
   {
     printf("i=%d,memr = %d, memi = %d\n",i,memr[i].word,memi[i].word);
     }*/

ival = 65;
    // Third stage.
    for (i=0; i<64; i++)
    {
		for (j=0;j<4;j++){
			BFIr[j].word = memr[iord2[j][i]].word;
			BFIi[j].word = memi[iord2[j][i]].word;
	                tmp.word = iord2[j][i];
	                if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);
		}

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        for (j=0;j<4;j++){
			if (j==0){
        	 	  memr[iord2[j][i]].word = BFOr[0].word;
        		  memi[iord2[j][i]].word = BFOi[0].word;
			 if (i==ival) printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,BFOr[j].word,BFOi[j].word);
			}
			else {
			  m = i%4;
			  FFTComplexMultiplier(BFOr[j], BFOi[j], TFr[j-1][m*16], TFi[j-1][m*16], &cmplx_out_r, &cmplx_out_i);
			  memr[iord2[j][i]] = cmplx_out_r;
			  memi[iord2[j][i]] = cmplx_out_i;
                          if (i==ival)
                            printf("i=%d,j=%d,cmplx_outr = %d,cmplx_outi = %d\n",i,j,cmplx_out_r.word,cmplx_out_i.word);
			}
		}
    }




   /*  for (i=0;i<256;i++)
   {
     printf("stage3: i=%d,memr = %d, memi = %d\n",i,memr[i].word,memi[i].word);
     } */

    if (ifft == 0){
      for (j=0; j<256; j++)
	{
	  memr[j].word = FFTSaturation(memr[j], 2);
	  memi[j].word = FFTSaturation(memi[j], 2);
	}
    }
    else {
      for (j=0; j<256; j++)
	{
	  memr[j].word = FFTSaturation(memr[j], 1);
	  memi[j].word = FFTSaturation(memi[j], 1);
	}
    }

   for (i=0;i<64;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord2[j][i]],memi[iord2[j][i]]);
      }
     }
   }

ival=65;

  if (ifft == 0)
  {
    // the 4th stage: No complex multiplication needed for this stage
      for (i=0; i<74; i++)
      {
        for (j=0; j<4; j++)
        {
	    BFIr[j].word = memr[iord4[j][i]].word;
            BFIi[j].word = memi[iord4[j][i]].word;

	    tmp.word = iord4[j][i];
	    if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);

        }

		if (WrEn[i] == 0)
				FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);
    	else
		{
				for (j=0;j<4;j++)
				{
					BFOr[j].word = BFIr[j].word;
					BFOi[j].word = BFIi[j].word;
				}
		}


	for (j=0;j<4;j++) if (i == ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        out1r[i].word = BFOr[out_num[i]].word;
        out1i[i].word = BFOi[out_num[i]].word;

      // if (i < ival)
      if (i==ival)
		printf("i=%d, out_num=%d,out1r = %d, out1i = %d\n",i,out_num[i],BFOr[out_num[i]].word,BFOi[out_num[i]].word);

        for (j=0; j<4; j++)
        {
            memr[iord4[j][i]].word = BFOr[j].word;
            memi[iord4[j][i]].word = BFOi[j].word;
        }
      }

      for (i=0; i<186; i++)
      {
		out2r[i].word = memr[oord4[74+i]].word;
		out2i[i].word = memi[oord4[74+i]].word;
		//	printf("i=%d, raddr = %d, Out2r[%d] = %d,Out2i[%d] = %d\n",i,oord4[74+i],i,out2r[i].word,i,out2i[i].word);
      }
  }
  else
  {
    for (i=0; i<64; i++)
    {
        for (j=0; j<4; j++)
        {
            BFIr[j].word = memr[iord3[j][i]].word;
            BFIi[j].word = memi[iord3[j][i]].word;
	     tmp.word = iord3[j][i];
	     if (i==ival) printf("i=%d,j=%d,iord=%d,BFIr = %d,BFIi = %d\n",i,j,tmp.word,BFIr[j].word,BFIi[j].word);
        }

        FFTFourPointButterfly(BFIr, BFIi, BFOr, BFOi);

	for (j=0;j<4;j++) if (i==ival)  printf("i=%d,j=%d,BFOr = %d,BFOi = %d\n",i,j,BFOr[j].word,BFOi[j].word);

        for (j=0; j<4; j++)
        {
            memr[iord3[j][i]].word = BFOr[j].word;
            memi[iord3[j][i]].word = BFOi[j].word;


        }
    }

  }

    for (j=0; j<256; j++)
    {
        memr[j].word = FFTSaturation(memr[j], 0);
        memi[j].word = FFTSaturation(memi[j], 0);
    }

    /*    for (i=0;i<256;i++)
   {
     printf("stage4: i=%d,memr = %d, memi = %d\n",i,memr[i].word,memi[i].word);
     }*/

  if (ifft == 1)
  {
    for (i=0;i<64;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord3[j][i]],memi[iord3[j][i]]);
      }
     }
   }
  }
  else if (ifft == 0)
  {
    for (i=0;i<74;i++){
     for (j=0;j<4;j++){
       if (i==ival){
         printf("i=%d,j%d, sat_outr = %d, sat_outi = %d\n",i,j,memr[iord4[j][i]],memi[iord4[j][i]]);
      }
     }
   }
  }





    if (ifft)
    {

	  if (gi == 0)
	  {
	    for (i=0;i<288;i++)
	    {
           yr[i].word = memr[oord_sh[i]].word;
           yi[i].word = -1*memi[oord_sh[i]].word;
        }
      }
      else
	  {
		for (i=0;i<320;i++)
		{
           yr[i].word = memr[oord_lg[i]].word;
           yi[i].word = -1*memi[oord_lg[i]].word;
        //   if ( i < 13)
        //   printf("i=%d,j=%d,yr = %d, yi = %d\n",i,j,yr[i].word,yi[i].word);
	    }
	  }
    }
    else
    {
        for (i=0; i<260; i++)      //if ifft, conjugate output
        {
          if (i < 74)
          {
            yr[i].word = out1r[i].word;
            yi[i].word = out1i[i].word;
	      }
	      else
	      {
			yr[i].word = out2r[i-74].word;
			yi[i].word = out2i[i-74].word;
	      }
	  // printf("i=%d,yr=%d,yi=%d.\n",i,yr[i].word,yi[i].word);
	   }
    }
}

static void FFTTwoPointButterfly(wiWORD *inr, wiWORD *ini, wiWORD *outr, wiWORD *outi)
{
  // Radix2 stage adds 1 bit to precision.
    outr[0].word = inr[0].FFTBFWL2 + inr[1].FFTBFWL2;
    outi[0].word = ini[0].FFTBFWL2 + ini[1].FFTBFWL2;

    outr[1].word = inr[0].FFTBFWL2 - inr[1].FFTBFWL2;
    outi[1].word = ini[0].FFTBFWL2 - ini[1].FFTBFWL2;
}


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
